#include <iostream>
#include <signal.h>
#include <string.h>
#include <IpAddress.h>
#include <IPv4Layer.h>
#include <TcpLayer.h>
#include <UdpLayer.h>
#include <Packet.h>
#include <PcapLiveDevice.h>
#include <PcapLiveDeviceList.h>
#include <ndpi_api.h>
#include <ndpi_typedefs.h>
#include <unordered_map>

/* Command line argument macros. */
#define TOKEN__INPUT_NW_IFACE               "-i"
#define TOKEN__MAX_PKTS                     "-N"

/* Function retrun macros. */
#define STATUS_FAIL                         (1)
#define STATUS_SUCCESS                      (0)

/* Connection tracking status macros. */
#define CT_DET_DONE                         (1)
#define CT_DET_UNKNOWN                      (0)
#define CT_DET_FAILED                       (-1)

/* Display information struct. */
struct display_info {
    uint16_t cxn_id;
    std::string ctg;
    std::string proto;
    std::string domain;
};

/* Connection tracking entry. */
struct db_ndpi_ct_info {
    uint8_t ct_status;
    uint8_t pkts_inj_cnt;
    uint16_t cxn_id;
    struct ndpi_flow_struct flow;
};

/* nDPI database. */
struct db_ndpi {
    uint8_t max_pkts;
    uint16_t glob_cxn_id;
    struct ndpi_detection_module_struct *ndpi_det;
    std::unordered_map<std::string, struct db_ndpi_ct_info> map_ndpi;
};

/* Callbacks. */
static void cb_on_pkt_recv (pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie);
static void cb_sig_term (int sig);

/* Globak display structure. */
static std::unordered_map<uint16_t, struct display_info> glb_display_hash_map;

int main(int argc, char* argv[]) {
    uint8_t status = STATUS_SUCCESS;
    pcpp::PcapLiveDevice *iface = NULL;
    pcpp::IPcapDevice::PcapStats stats = { 0 };
    std::string if_name = "";
    struct db_ndpi ndpi = { 0 };

    /* Parse command line arguments */
    for (int counter = 0; counter < argc; counter++) {
        if (!strcmp(argv[counter], TOKEN__INPUT_NW_IFACE)) {
            std::cout << "Input interface name: " << argv[counter + 1] << std::endl;
            if_name = argv[counter + 1];
        }

        if (!strcmp(argv[counter], TOKEN__MAX_PKTS)) {
            std::cout << "Max packets: " << argv[counter + 1] << std::endl;
            ndpi.max_pkts =  std::stoi(argv[counter + 1]);
            if (ndpi.max_pkts <= 0) {
                std::cout << "Max packets defaulting to 1." << std::endl;
            }
        }
    }

    /* Get a list of all the interfaces. */
    iface = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(if_name);
    if (iface == NULL) {
        status = STATUS_FAIL;
        std::cout << "Unable to get device by the specified name." << std::endl;
    }

    if (status == STATUS_SUCCESS) {
        if (!iface->open()) {
            status = STATUS_FAIL;
            std::cout << "Unable to open capturing device." << std::endl;
        }
    }

    /* NDPI initializaion. */
    if (status == STATUS_SUCCESS) {
        ndpi.ndpi_det = ndpi_init_detection_module(ndpi_no_prefs);
        if (ndpi.ndpi_det != NULL) {
            NDPI_PROTOCOL_BITMASK all;
            NDPI_BITMASK_SET_ALL(all);
            if (ndpi_set_protocol_detection_bitmask2(ndpi.ndpi_det, &all) != STATUS_SUCCESS) {
                std::cout << "Unable to set protocol bitmask." << std::endl;
                status = STATUS_FAIL;
            } else {
                ndpi_finalize_initialization(ndpi.ndpi_det);
            }
        } else {
            status = STATUS_FAIL;
            std::cout << "Unable to initialize nDPI." << std::endl;
        }
    }

    if (status == STATUS_SUCCESS) {
        if (iface->startCapture(cb_on_pkt_recv, &ndpi)) {
            std::cout << "Capture started successfully." << std::endl;
        } else {
            status = STATUS_FAIL;
        }
    }

    /* Wait for the termination request. */
    if (status == STATUS_SUCCESS) {
        signal(SIGINT, cb_sig_term);
        while (true);
    }

    return status;
}

static void cb_on_pkt_recv (pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie) {
    pcpp::Packet parsedPacket(packet);
    pcpp::IPv4Layer *ipv4_layer = NULL;
    pcpp::TcpLayer *tcp_layer = NULL;
    pcpp::UdpLayer *udp_layer = NULL;
    pcpp::tcphdr *tcp_hdr = NULL;
    pcpp::udphdr *udp_hdr = NULL;
    pcpp::iphdr *ipv4_hdr = NULL;
    std::string hash = "";
    struct db_ndpi_ct_info ct_entry;
    struct display_info dp_struct;
    uint8_t status = STATUS_FAIL;
    ndpi_protocol protocol;
    char temp_char_str[100];
    struct db_ndpi *ndpi = (struct db_ndpi *)cookie;
    std::unordered_map<std::string, struct db_ndpi_ct_info>::iterator itr;

    /* Only do processing of packet if it has IPv4 layer. */
    ipv4_layer = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();
    if (ipv4_layer != NULL) {
        ipv4_hdr = ipv4_layer->getIPv4Header();
        if (parsedPacket.isPacketOfType(pcpp::TCP) || parsedPacket.isPacketOfType(pcpp::UDP)) {
            status = STATUS_SUCCESS;
        }

        /* Create a hash by simply ip src and dst in strings. */
        hash = std::to_string(ipv4_hdr->ipSrc) + std::to_string(ipv4_hdr->ipDst);
    }

    /* Continue processing only if the packet has TCP/UDP layer. */
    if (status == STATUS_SUCCESS) {
        tcp_layer = parsedPacket.getLayerOfType<pcpp::TcpLayer>();
        if (tcp_layer != NULL) {
            tcp_hdr = tcp_layer->getTcpHeader();
            hash += std::to_string(tcp_hdr->portSrc) + std::to_string(tcp_hdr->portDst);
        } else {
            udp_layer = parsedPacket.getLayerOfType<pcpp::UdpLayer>();
            if (udp_layer != NULL) {
                udp_hdr = udp_layer->getUdpHeader();
                hash += std::to_string(udp_hdr->portSrc) + std::to_string(udp_hdr->portDst);
            } else {
                /* If the packet does not contain TCP or UDP layers then dont pass it through nDPI. */
                status = STATUS_FAIL;
            }
        }
    }

    /* Enter this block only if the packet contains TCP/UDP layer. */
    if (status == STATUS_SUCCESS) {
        itr = ndpi->map_ndpi.find(hash);
        if (itr == ndpi->map_ndpi.end()) {
            /* An existing entry in hash table for this connection is not found, so,
               create a new ct entry. */
            memset(&ct_entry, 0x00, sizeof(struct db_ndpi_ct_info));
            protocol = ndpi_detection_process_packet(ndpi->ndpi_det, &ct_entry.flow,
                                                    ipv4_layer->getData(), ipv4_layer->getDataLen(),
                                                    packet->getPacketTimeStamp().tv_nsec, NULL);
            ct_entry.cxn_id = ndpi->glob_cxn_id++;
            ct_entry.pkts_inj_cnt++;
            if (protocol.app_protocol != NDPI_PROTOCOL_UNKNOWN) {
                ct_entry.ct_status = CT_DET_DONE;

                /* Populate the display structure and add an entry to display map. */
                dp_struct.ctg = ndpi_category_get_name(ndpi->ndpi_det, ct_entry.flow.category);
                dp_struct.cxn_id = ct_entry.cxn_id;
                dp_struct.domain = ct_entry.flow.host_server_name;
                dp_struct.proto = ndpi_get_proto_name(ndpi->ndpi_det, protocol.app_protocol);

                /* Insert this display structure in the display hash table. */
                glb_display_hash_map.insert(std::make_pair(dp_struct.cxn_id, dp_struct));
            } else {
                ct_entry.ct_status = CT_DET_UNKNOWN;
            }

            /* Add the CT entry to detect consecutive packets. */
            ndpi->map_ndpi.insert(std::make_pair(hash, ct_entry));
        } else if (itr->second.ct_status == CT_DET_UNKNOWN) {
            if (itr->second.pkts_inj_cnt < ndpi->max_pkts) {
                protocol = ndpi_detection_process_packet(ndpi->ndpi_det, &itr->second.flow,
                                                         ipv4_layer->getData(), ipv4_layer->getDataLen(),
                                                         packet->getPacketTimeStamp().tv_nsec, NULL);
                itr->second.pkts_inj_cnt++;
                if (protocol.app_protocol != NDPI_PROTOCOL_UNKNOWN) {
                    itr->second.ct_status = CT_DET_DONE;

                    /* Populate the display structure and add an entry to display map. */
                    dp_struct.ctg = ndpi_category_get_name(ndpi->ndpi_det, itr->second.flow.category);
                    dp_struct.cxn_id = itr->second.cxn_id;
                    dp_struct.domain = itr->second.flow.host_server_name;
                    dp_struct.proto = ndpi_get_proto_name(ndpi->ndpi_det, protocol.app_protocol);

                    /* Insert this display structure in the display hash table. */
                    glb_display_hash_map.insert(std::make_pair(dp_struct.cxn_id, dp_struct));
                }
            } else {
                /* This block is entered if the packets injected increases the max packets and proto remains unknown. */
                itr->second.ct_status = CT_DET_FAILED;

                /* Populate the display structure and add an entry to display map. */
                dp_struct.ctg = ndpi_category_get_name(ndpi->ndpi_det, NDPI_PROTOCOL_CATEGORY_UNSPECIFIED);
                dp_struct.cxn_id = itr->second.cxn_id;
                dp_struct.domain = "";
                dp_struct.proto = ndpi_get_proto_name(ndpi->ndpi_det, NDPI_PROTOCOL_UNKNOWN);

                /* Insert this display structure in the display hash table. */
                glb_display_hash_map.insert(std::make_pair(dp_struct.cxn_id, dp_struct));
            }
        }
    }
}

/* This function fetches entries from display has table and displays them. */
static void cb_sig_term (int sig) {
    std::unordered_map<uint16_t, struct display_info>::iterator itr;
    std::cout << "Connection ID, Protocol, Category, Domain" << std::endl;
    for (int ctr = 0; ctr < glb_display_hash_map.size(); ctr++) {
        itr = glb_display_hash_map.find(ctr);
        if (itr != glb_display_hash_map.end()) {
            std::cout << glb_display_hash_map.at(ctr).cxn_id << ",";
            std::cout << glb_display_hash_map.at(ctr).proto << ",";
            std::cout << glb_display_hash_map.at(ctr).ctg << ",";
            std::cout << glb_display_hash_map.at(ctr).domain << std::endl;
        }
    }
    exit (sig);
}