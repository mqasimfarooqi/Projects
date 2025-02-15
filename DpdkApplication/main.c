#include <arpa/inet.h>
#include <getopt.h>
#include <pthread.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Logger macro
#define LOG(msg, ...) logMessage(getFilename(__FILE__), __LINE__, __func__, msg, ##__VA_ARGS__)

// Global log file pointer
FILE *logFile = NULL;

// Mutex for writing in a log file
static pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

// Constants
#define MAX_PKT_BURST 64
#define SUPPORTED_PORT_COUNT 2
#define MIN_RX_TX_QUEUES 1
#define RX_DESC_SIZE 2048
#define TX_DESC_SIZE 2048
#define MBUF_POOL_SIZE 16384
#define MEMPOOL_CACHE_SIZE 512
#define RATE_INTERVAL 1
#define STATS_PRINT_INTERVAL 1
#define MAX_BANDWIDTH_LIMIT 100000000
#define MAX_DROP_IPS 10
#define DEFAULT_CORE_MASK "0x7"
#define LOG_FILE "DpdkApp.log"

// Command-line options
const struct option longOptions[] = {
    {"help", no_argument, 0, 'h'},
    {"list", no_argument, 0, 'L'},
    {"device-list", required_argument, 0, 'd'},
    {"address-list", required_argument, 0, 'a'},
    {"bandwidth", required_argument, 0, 'b'},
    {0, 0, 0, 0}};

// Structure to hold port information
typedef struct PortInfo
{
    pthread_mutex_t mutex;
    struct rte_mempool *mbufPool;
    struct rte_eth_stats stats;
    struct in_addr dropIPs[MAX_DROP_IPS];
    size_t portId;
    size_t rxTxQueues;
    size_t rxBytesRate;
    size_t txBytesRate;
    size_t rxPacketRate;
    size_t txPacketRate;
    size_t droppedPackets;
    size_t bandwidthLimit;
    size_t bandwidthRate;
    size_t dropIpsCount;
} PortInfo;

// Forward declarations
static int portThread(void *arg);
const char *getFilename(const char *path);
void logMessage(const char *file, int line, const char *func, const char *message, ...);
int initializePort(PortInfo *portInfo, struct rte_mempool *mbufPool);
void listPorts();
int processPacket(struct rte_mbuf *mbuf, PortInfo *port, struct in_addr *dropAddrs, size_t dropAddrCount);
void printStats(PortInfo *port);
void printUsage(const char *progName);
void *rateThread(void *arg);
void *statsThread(void *arg);

// Main function
int main(int argc, char *argv[])
{
    struct rte_mempool *mbufPool;
    struct rte_mbuf *bufferArray[MAX_PKT_BURST];
    struct rte_mbuf *filteredBufferArray[MAX_PKT_BURST];
    struct in_addr dropIPs[MAX_DROP_IPS];
    size_t bandwidthLimit = MAX_BANDWIDTH_LIMIT;
    size_t portIds[SUPPORTED_PORT_COUNT] = {0};
    size_t portsCount = 0;
    size_t receivedPackets = 0;
    size_t transmittedPackets = 0;
    size_t packetsToTransmit = 0;
    time_t currentTime;
    pthread_t rateThreadId;
    pthread_t statsThreadId;
    char *portList = NULL;
    char *addrList = NULL;
    char *coreMask = DEFAULT_CORE_MASK;
    int success = 0;
    int opt = 0;
    int optionIndex = 0;
    int dropAddrCount = 0;

    // Open log file
    logFile = fopen(LOG_FILE, "a");
    if (!logFile)
    {
        printf("Error opening log file\n");
        return 1;
    }
    LOG("Log file opened successfully");

    // Parse command-line arguments, including -c
    while ((opt = getopt_long(argc, argv, "hL:d:a:b:", longOptions, &optionIndex)) != -1)
    {
        switch (opt)
        {
        case 'h':
            printUsage(argv[0]);
            return 0;
        case 'L':
            listPorts();
            return 0;
        case 'd':
            portList = optarg;
            break;
        case 'a':
            addrList = optarg;
            break;
        case 'b':
            bandwidthLimit = strtoull(optarg, NULL, 10);
            break;
            break;
        default:
            printUsage(argv[0]);
            return 1;
        }
    }

    // Create dpdkArgv, using the provided core mask
    size_t dpdkArgc = 3;
    char *dpdkArgv[] = {"DpdkApp", "-c", coreMask};
    success = rte_eal_init(dpdkArgc, dpdkArgv);
    if (success < 0)
    {
        printf("Error initializing EAL: %s\n", rte_strerror(-success));
        LOG("Error initializing EAL: %s", rte_strerror(-success));
        return 1;
    }

    // Check if the required -d option was provided
    if (portList == NULL)
    {
        printf("Error: -d option is required\n");
        LOG("Error: -d option is required");
        printUsage(argv[0]);
        return 1;
    }

    // Check if the required -b option was provided
    if (bandwidthLimit > MAX_BANDWIDTH_LIMIT)
    {
        printf("Error: -b option must be less than or equal to %d\n", MAX_BANDWIDTH_LIMIT);
        LOG("Error: -b option must be less than or equal to %d", MAX_BANDWIDTH_LIMIT);
        printUsage(argv[0]);
        return 1;
    }

    // Parse the port list
    char *token = strtok(portList, ",");
    while (token != NULL && portsCount < SUPPORTED_PORT_COUNT)
    {
        portIds[portsCount++] = atoi(token);
        token = strtok(NULL, ",");
    }

    // Check if exactly two ports were specified
    if (portsCount != SUPPORTED_PORT_COUNT)
    {
        printf("Error: exactly two ports must be specified\n");
        LOG("Error: exactly two ports must be specified");
        return 1;
    }

    // Parse the address list
    if (addrList != NULL)
    {
        token = strtok(addrList, ",");
        while (token != NULL && dropAddrCount < MAX_DROP_IPS)
        {
            if (inet_aton(token, &dropIPs[dropAddrCount]) == 0)
            {
                printf("Error: invalid IP address %s\n", token);
                LOG("Error: invalid IP address %s", token);
                return 1;
            }
            dropAddrCount++;
            token = strtok(NULL, ",");
        }
    }

    // Create mbuf pool
    mbufPool = rte_mempool_create("mbufPool", MBUF_POOL_SIZE, RTE_MBUF_DEFAULT_BUF_SIZE, MEMPOOL_CACHE_SIZE,
                                  sizeof(struct rte_pktmbuf_pool_private), rte_pktmbuf_pool_init, NULL,
                                  rte_pktmbuf_init, NULL, rte_socket_id(), 0);
    if (!mbufPool)
    {
        printf("Error creating mbuf pool\n");
        LOG("Error creating mbuf pool");
        return 1;
    }

    PortInfo ports[SUPPORTED_PORT_COUNT];
    for (size_t portIndex = 0; portIndex < SUPPORTED_PORT_COUNT; portIndex++)
    {
        ports[portIndex].portId = portIds[portIndex];
        ports[portIndex].mbufPool = mbufPool;
        ports[portIndex].rxTxQueues = MIN_RX_TX_QUEUES;
        ports[portIndex].rxBytesRate = 0;
        ports[portIndex].txBytesRate = 0;
        ports[portIndex].rxPacketRate = 0;
        ports[portIndex].txPacketRate = 0;
        ports[portIndex].droppedPackets = 0;
        ports[portIndex].dropIpsCount = dropAddrCount;
        ports[portIndex].bandwidthLimit = bandwidthLimit;
        ports[portIndex].mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
        memcpy(ports[portIndex].dropIPs, dropIPs, sizeof(dropIPs));
        success = initializePort(&ports[portIndex], mbufPool);
        if (success != 0)
        {
            printf("Error initializing port %lu\n", ports[portIndex].portId);
            LOG("Error initializing port %lu", ports[portIndex].portId);
            return 1;
        }
    }

    // Create rate thread
    if (pthread_create(&rateThreadId, NULL, rateThread, ports) != 0)
    {
        printf("Error creating rate calculation thread\n");
        LOG("Error creating rate calculation thread");
        return 1;
    }

    // Create stats thread
    if (pthread_create(&statsThreadId, NULL, statsThread, ports) != 0)
    {
        printf("Error creating stats thread\n");
        LOG("Error creating stats thread");
        return 1;
    }

    // Get the number of available cores
    size_t numCores = rte_lcore_count();
    printf("Number of available cores: %lu\n", numCores);
    LOG("Number of available cores: %lu", numCores);

    size_t lcoreId = 1;
    for (size_t portIndex = 0; portIndex < SUPPORTED_PORT_COUNT; portIndex++)
    {
        if (lcoreId < numCores)
        {
            success = rte_eal_remote_launch(portThread, &ports[portIndex], lcoreId);
            if (success != 0)
            {
                printf("Failed to launch worker thread for port %zu on core %lu.\n", portIndex, lcoreId);
                LOG("Failed to launch worker thread for port %zu on core %lu", portIndex, lcoreId);
                return -1;
            }
            printf("Launched worker thread for port %zu on core %lu.\n", portIndex, lcoreId);
            LOG("Launched worker thread for port %zu on core %lu.", portIndex, lcoreId);
            lcoreId++;
        }
        else
        {
            success = rte_eal_remote_launch(portThread, &ports[portIndex], numCores - 1);
            if (success != 0)
            {
                printf("Failed to launch worker thread for port %zu on core %lu.\n", portIndex, numCores - 1);
                LOG("Failed to launch worker thread for port %zu on core %lu.", portIndex, numCores - 1);
                return -1;
            }
            printf("Launched worker thread for port %zu on core %lu (shared).\n", portIndex, numCores - 1);
            LOG("Launched worker thread for port %zu on core %lu (shared).", portIndex, numCores - 1);
        }
    }

    LOG("Application started");

    // Wait for all lcores to finish
    for (size_t i = 1; i < lcoreId; i++)
    {
        rte_eal_wait_lcore(i);
    }

    // Close log file
    fclose(logFile);

    return 0;
}

// Per-port thread function
static int portThread(void *arg)
{
    struct PortInfo *portInfo = (struct PortInfo *)arg;
    unsigned lcore_id = rte_lcore_id();
    size_t otherPortIndex = (portInfo->portId == 0) ? 1 : 0;
    struct PortInfo *ports = (struct PortInfo *)arg;

    if (portInfo->portId == 0)
    {
        ports = portInfo;
    }
    else
    {
        ports = portInfo - 1;
    }

    printf("Starting thread for port %lu on lcore %u\n", portInfo->portId, lcore_id);
    LOG("Starting thread for port %lu on lcore %u", portInfo->portId, lcore_id);

    struct rte_mbuf *bufferArray[MAX_PKT_BURST];
    struct rte_mbuf *filteredBufferArray[MAX_PKT_BURST];

    size_t receivedPackets;
    size_t transmittedPackets;
    size_t packetsToTransmit = 0;

    while (1)
    {
        for (size_t queue = 0; queue < portInfo->rxTxQueues; queue++)
        {
            receivedPackets = rte_eth_rx_burst(portInfo->portId, queue, bufferArray, MAX_PKT_BURST);
            if (receivedPackets > 0)
            {
                for (size_t counter = 0; counter < receivedPackets; counter++)
                {
                    pthread_mutex_lock(&portInfo->mutex);
                    portInfo->bandwidthRate += rte_pktmbuf_pkt_len(bufferArray[counter]);
                    pthread_mutex_unlock(&portInfo->mutex);
                    if ((processPacket(bufferArray[counter], portInfo, portInfo->dropIPs, portInfo->dropIpsCount) != 0) ||
                        (portInfo->bandwidthRate > portInfo->bandwidthLimit))
                    {
                        rte_pktmbuf_free(bufferArray[counter]);
                        portInfo->droppedPackets++;
                        continue;
                    }
                    filteredBufferArray[packetsToTransmit++] = bufferArray[counter];
                }
            }

            if (packetsToTransmit > 0)
            {
                transmittedPackets = rte_eth_tx_burst(ports[otherPortIndex].portId, queue, filteredBufferArray, packetsToTransmit);
                if (transmittedPackets < packetsToTransmit)
                {
                    for (size_t counter = transmittedPackets; counter < packetsToTransmit; counter++)
                    {
                        rte_pktmbuf_free(filteredBufferArray[counter]);
                    }
                }
                packetsToTransmit = 0;
            }
        }
    }
    return 0;
}

// Function to get filename from path
const char *getFilename(const char *path)
{
    const char *lastSlash = strrchr(path, '/');
    const char *lastBackslash = strrchr(path, '\\');
    const char *filenameStart = (lastSlash > lastBackslash) ? lastSlash : lastBackslash;
    return (filenameStart == NULL) ? path : filenameStart + 1;
}

// Function to log messages
void logMessage(const char *file, int line, const char *func, const char *message, ...)
{
    if (logFile)
    {
        pthread_mutex_lock(&logMutex);
        va_list args;
        va_start(args, message);
        char timestamp[20];
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
        fprintf(logFile, "[%s] %s:%d (%s): ", timestamp, file, line, func);
        vfprintf(logFile, message, args);
        fprintf(logFile, "\n");
        fflush(logFile);
        va_end(args);
        pthread_mutex_unlock(&logMutex);
    }
}

// Function to initialize a port
int initializePort(PortInfo *portInfo, struct rte_mempool *mbufPool)
{
    struct rte_eth_conf portConfig = {
        .rxmode = {
            .split_hdr_size = 0,
        },
    };
    int success;
    size_t portId = portInfo->portId;
    struct rte_eth_dev_info devInfo;

    LOG("Initializing port %zu...", portId);

    if (rte_eth_dev_info_get(portId, &devInfo) != 0)
    {
        LOG("Error getting device info for port %zu", portId);
        return -1;
    }

    LOG("Port %zu device info retrieved successfully.", portId);

    // Keep rx/tx queues equal to simplify logic
    if (devInfo.max_tx_queues != devInfo.max_rx_queues)
    {
        portInfo->rxTxQueues = (devInfo.max_tx_queues < devInfo.max_rx_queues) ? devInfo.max_tx_queues : devInfo.max_rx_queues;
        LOG("Adjusted RX/TX queue count to %u (max RX: %u, max TX: %u) for port %zu due to mismatch.", portInfo->rxTxQueues, devInfo.max_rx_queues, devInfo.max_tx_queues, portId);
    }
    else
    {
        LOG("RX/TX queue counts match: %u for port %zu.", devInfo.max_rx_queues, portId);
    }

    success = rte_eth_dev_configure(portId, portInfo->rxTxQueues, portInfo->rxTxQueues, &portConfig);
    if (success < 0)
    {
        LOG("Error configuring port %zu: %s", portId, rte_strerror(-success));
        return success;
    }

    LOG("Port %zu configured successfully with %u RX/TX queues.", portId, portInfo->rxTxQueues);

    for (size_t queue = 0; queue < portInfo->rxTxQueues; queue++)
    {
        success = rte_eth_rx_queue_setup(portId, queue, RX_DESC_SIZE, rte_eth_dev_socket_id(portId), NULL, mbufPool);
        if (success < 0)
        {
            LOG("Error configuring RX queue %zu for port %zu: %s", queue, portId, rte_strerror(-success));
            return success;
        }
        LOG("RX queue %zu for port %zu configured successfully.", queue, portId);
    }

    for (size_t queue = 0; queue < portInfo->rxTxQueues; queue++)
    {
        success = rte_eth_tx_queue_setup(portId, queue, TX_DESC_SIZE, rte_eth_dev_socket_id(portId), NULL);
        if (success < 0)
        {
            LOG("Error configuring TX queue %zu for port %zu: %s", queue, portId, rte_strerror(-success));
            return success;
        }
        LOG("TX queue %zu for port %zu configured successfully.", queue, portId);
    }

    success = rte_eth_dev_start(portId);
    if (success < 0)
    {
        LOG("Error starting port %zu: %s", portId, rte_strerror(-success));
        return success;
    }

    LOG("Port %zu started successfully.", portId);

    success = rte_eth_promiscuous_enable(portId);
    if (success < 0)
    {
        LOG("Error enabling promiscuous mode for port %zu: %s", portId, rte_strerror(-success));
        return success;
    }

    LOG("Promiscuous mode enabled for port %zu.", portId);

    LOG("Port %zu initialization complete.", portId);

    return 0;
}

// Function to list available DPDK ports
void listPorts()
{
    int success = 0;
    size_t dpdkArgc = 3;
    char *dpdkArgv[] = {"DpdkApp", "-c", DEFAULT_CORE_MASK};
    success = rte_eal_init(dpdkArgc, dpdkArgv);
    if (success < 0)
    {
        printf("Error initializing EAL: %s\n", rte_strerror(-success));
        LOG("Error initializing EAL: %s", rte_strerror(-success));
        return;
    }

    size_t ports = rte_eth_dev_count_avail();
    printf("Available DPDK ports:\n");
    for (size_t port = 0; port < ports; port++)
    {
        struct rte_eth_dev_info devInfo;
        if (rte_eth_dev_info_get(port, &devInfo) == 0)
        {
            printf("Port %lu Info:\n", port);
            printf("  Driver name: %s\n", devInfo.driver_name);
            printf("  Min RX bufsize: %u\n", devInfo.min_rx_bufsize);
            printf("  Max RX pktlen: %u\n", devInfo.max_rx_pktlen);
            printf("  Max RX queues: %u\n", devInfo.max_rx_queues);
            printf("  Max TX queues: %u\n", devInfo.max_tx_queues);
            printf("  Speed capabilities: %" PRIx32 "\n", devInfo.speed_capa);
        }
    }
}

// Function to process a packet
int processPacket(struct rte_mbuf *mbuf, PortInfo *port, struct in_addr *dropAddrs, size_t dropAddrCount)
{
    struct rte_ether_hdr *ethHdr = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);
    uint16_t etherType = rte_be_to_cpu_16(ethHdr->ether_type);
    if (etherType == RTE_ETHER_TYPE_IPV4)
    {
        struct rte_ipv4_hdr *ipHdr = rte_pktmbuf_mtod_offset(mbuf, struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));
        for (size_t i = 0; i < dropAddrCount; i++)
        {
            if (ipHdr->src_addr == dropAddrs[i].s_addr)
            {
                LOG("Dropping packet from IP address: %s (port %zu)", inet_ntoa(*(struct in_addr *)&ipHdr->src_addr), port->portId);
                return -1;
            }
        }
    }
    else
    {
        LOG("Dropping non-IPv4 packet (Ether type: 0x%04x) on port %zu.", etherType, port->portId);
        return -1;
    }

    return 0;
}

// Function to print port statistics
void printStats(PortInfo *port)
{
    struct rte_eth_stats stats;
    rte_eth_stats_get(port->portId, &stats);
    printf("Port %lu Stats:\n", port->portId);
    printf("  Bandwidth Limit: %lu Bps\n", port->bandwidthLimit);
    printf("  Packets Received: %" PRIu64 ", Rate: %" PRIu64 " pps, %" PRIu64 " Bps\n", stats.ipackets, port->rxPacketRate, port->rxBytesRate);
    printf("  Bytes Received: %" PRIu64 "\n", stats.ibytes);
    printf("  Packets Transmitted: %" PRIu64 ", Rate: %" PRIu64 " pps, %" PRIu64 " Bps\n", stats.opackets, port->txPacketRate, port->txBytesRate);
    printf("  Bytes Transmitted: %" PRIu64 "\n", stats.obytes);
    printf("  Erroneous Packets: %" PRIu64 "\n", stats.ierrors + stats.oerrors);
    printf("  Dropped Packets by HW: %" PRIu64 "\n", stats.imissed);
    printf("  Dropped Packets by SW: %" PRIu64 "\n", port->droppedPackets);
}

// Function to print usage information
void printUsage(const char *progName)
{
    printf("Usage: %s [OPTIONS]\n", progName);
    printf("Options:\n");
    printf("-h, --help\t\t\tPrint this help message\n");
    printf("-L, --list\t\t\tList available DPDK ports\n");
    printf("-b, --bandwidth\t\t\tBandwidth limit in bytes per second (default: 100000000)\n");
    printf("-d, --device-list\t\tComma-separated list of two DPDK port IDs (e.g 0,1)\n");
    printf("-a, --address-list\t\tComma-separated list of IP addresses to drop (e.g 192.168.1.10,192.168.1.11)\n");
}

// Thread function to calculate rates
void *rateThread(void *arg)
{
    PortInfo *currentPortInfo = (PortInfo *)arg;
    PortInfo previousPortInfo[SUPPORTED_PORT_COUNT] = {0};

    while (1)
    {
        // Sleep for configured interval
        sleep(RATE_INTERVAL);

        for (size_t port = 0; port < SUPPORTED_PORT_COUNT; port++)
        {
            // Fetch port data
            rte_eth_stats_get(currentPortInfo[port].portId, &currentPortInfo[port].stats);

            // Calculate rates
            currentPortInfo[port].rxBytesRate = currentPortInfo[port].stats.ibytes - previousPortInfo[port].stats.ibytes;
            currentPortInfo[port].txBytesRate = currentPortInfo[port].stats.obytes - previousPortInfo[port].stats.obytes;
            currentPortInfo[port].rxPacketRate = currentPortInfo[port].stats.ipackets - previousPortInfo[port].stats.ipackets;
            currentPortInfo[port].txPacketRate = currentPortInfo[port].stats.opackets - previousPortInfo[port].stats.opackets;

            // Reset forwarding bandwidth rate
            pthread_mutex_lock(&currentPortInfo[port].mutex);
            currentPortInfo[port].bandwidthRate = 0;
            pthread_mutex_unlock(&currentPortInfo[port].mutex);

            // Update previous stats
            previousPortInfo[port] = currentPortInfo[port];
        }
    }
}

// Thread function to print statistics
void *statsThread(void *arg)
{
    PortInfo *currentPortInfo = (PortInfo *)arg;

    while (1)
    {
        // Sleep for configured interval
        sleep(STATS_PRINT_INTERVAL);

        // Clear screen and print stats
        printf("\033[2J\033[1;1H");
        for (size_t port = 0; port < SUPPORTED_PORT_COUNT; port++)
        {
            printStats(&currentPortInfo[port]);
        }
    }
}
