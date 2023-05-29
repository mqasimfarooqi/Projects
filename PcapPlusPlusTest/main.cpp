#include <iostream>
#include <IpAddress.h>
#include <VlanLayer.h>
#include <EthLayer.h>
#include <UdpLayer.h>
#include <DnsLayer.h>
#include <IPv4Layer.h>
#include <IPv6Layer.h>
#include <Packet.h>
#include <Logger.h>
#include <PcapFileDevice.h>

/* Command line argument tokens. */
#define TOKEN__INPUT_FILE_PATH "-i"
#define TOKEN__OUTPUT_FILE_PATH "-o"
#define TOKEN__VLAN "--vlan"
#define TOKEN__IP_VERSION "--ip-version"
#define TOKEN__TTL "--ttl"
#define TOKEN__DNS_ADDR "--dns-addr"
#define TOKEN__DNS_PORT "--dns-port"

/* Metadata fields macros. */
#define MD_FIELD_INVALID (-1)
#define MD_DROP (1 << 0)
#define MD_DNS_MOD (1 << 1)

/* Metadata fields ID macros. */
#define ID_DL_PROTO (1)
#define ID_NW_PROTO (2)
#define ID_NW_TTL (3)
#define ID_DL_VLAN_ID (4)
#define ID_NW_DNS_ADDR (5)
#define ID_NW_DNS_PORT (6)
#define ID_TP_PROTO (7)

#define MD_MSK_PASSTHROUGH (1)
#define MD_MSK_BLOCK (0)

/* Network protocol cli macros. */
#define NW_PROTO_IPV4 "4"
#define NW_PROTO_IPV6 "6"

/* A singla instance of match action that is added in packet metadata. */
struct pkt_match_act {
    uint8_t id;
    uint64_t mask;
    uint64_t val;
};

/* Metadata to be attached with the packet. */
struct pkt_metadata {
    uint8_t pkt_verdict;
    struct pkt_match_act mat_act_pl;
};

/* Create my own structe to capture the statistics of packts. */
struct pkt_stats {

    /* Processed packet stats.*/
    uint32_t proc_pkt;
    uint32_t proc_bytes;

    /* Dropped packet stats. */
    uint32_t drop_pkt;
    uint32_t drop_bytes;

    /* Written packet stats. */
    uint32_t written_pkt;
    uint32_t written_bytes;

    /* DNS modified packet stats. */
    uint32_t dns_mod_pkt;
};

/* A callback + meta to be inserted into the packet processing pipeline. */
struct pp_pl_unit {
    void (*cb)(pcpp::Packet& prsd_pkt, struct pkt_metadata& metadata);
    struct pkt_metadata cb_data;
};

/* This struct defines the pieline in different layers of the OSI model. */
struct layer_cb {
    std::vector<pp_pl_unit> dl_pipeline;
    std::vector<pp_pl_unit> nw_pipeline;
    std::vector<pp_pl_unit> tp_pipeline;
};

/* Different callbacks inserted in the packet processing pipeline. */
void pl_proto_filter (pcpp::Packet& prsd_pkt, struct pkt_metadata& metadata);
void pl_dec_ttl (pcpp::Packet& prsd_pkt, struct pkt_metadata& metadata);
void pl_vlan_proc (pcpp::Packet& prsd_pkt, struct pkt_metadata& metadata);
void pl_mod_dns (pcpp::Packet& prsd_pkt, struct pkt_metadata& metadata);

int main(int argc, char* argv[])
{
    /* Declaring/Initializing local variables. */
    pcpp::RawPacket rawPacket;
    pcpp::Layer *dl_ptr = NULL;
    char in_file[25] = { 0 };
    char out_file[25] = { 0 };
    struct pp_pl_unit pl_unit;
    struct layer_cb pp_pipeline;
    struct pkt_stats stats = { 0 };

    /* Add a rule to drop all the packets that are not eth packets. */
    pl_unit = { 0 };
    pl_unit.cb = pl_proto_filter;
    pl_unit.cb_data.mat_act_pl.id = ID_DL_PROTO;
    pl_unit.cb_data.mat_act_pl.val = pcpp::Ethernet;
    pl_unit.cb_data.mat_act_pl.mask = MD_MSK_PASSTHROUGH;
    pp_pipeline.dl_pipeline.push_back(pl_unit);

    /* Add a rule to drop all the packets that are ICMP packets. */
    pl_unit = { 0 };
    pl_unit.cb = pl_proto_filter;
    pl_unit.cb_data.mat_act_pl.id = ID_NW_PROTO;
    pl_unit.cb_data.mat_act_pl.val = pcpp::ICMP;
    pl_unit.cb_data.mat_act_pl.mask = MD_MSK_BLOCK;
    pp_pipeline.tp_pipeline.push_back(pl_unit);

    /* Parsing command line arguments */
    for (int counter = 0; counter < argc; counter++) {

        if (!strcmp(argv[counter], TOKEN__INPUT_FILE_PATH)) {
            std::cout << "Input file: " << argv[counter + 1] << std::endl;
            strcpy(in_file, argv[counter + 1]);
        }
        else if (!strcmp(argv[counter], TOKEN__OUTPUT_FILE_PATH)) {
            std::cout << "Output file: " << argv[counter + 1] << std::endl;
            strcpy(out_file, argv[counter + 1]);
        }
        else if (!strcmp(argv[counter], TOKEN__VLAN)) {
            std::cout << "Vlan value: " << argv[counter + 1] << std::endl;

            /* Vlan ID filter. */
            pl_unit = { 0 };
            pl_unit.cb = pl_vlan_proc;
            pl_unit.cb_data.mat_act_pl.id = ID_DL_VLAN_ID;
            pl_unit.cb_data.mat_act_pl.val = std::stoi(argv[counter + 1]);
            pl_unit.cb_data.mat_act_pl.mask = MD_MSK_PASSTHROUGH;
            pp_pipeline.nw_pipeline.push_back(pl_unit);
        }
        else if (!strcmp(argv[counter], TOKEN__IP_VERSION)) {
            std::cout << "IP Version value: " << argv[counter + 1] << std::endl;

            /* Network protocol filter. */
            pl_unit = { 0 };
            pl_unit.cb = pl_proto_filter;
            pl_unit.cb_data.mat_act_pl.id = ID_NW_PROTO;
            if (!strcmp(argv[counter + 1], NW_PROTO_IPV4)) { pl_unit.cb_data.mat_act_pl.val = pcpp::IPv4; }
            else if (!strcmp(argv[counter + 1], NW_PROTO_IPV6)) { pl_unit.cb_data.mat_act_pl.val = pcpp::IPv6; }
            pl_unit.cb_data.mat_act_pl.mask = MD_MSK_PASSTHROUGH;
            pp_pipeline.nw_pipeline.push_back(pl_unit);
        }
        else if (!strcmp(argv[counter], TOKEN__TTL)) {
            std::cout << "TTL value: " << argv[counter + 1] << std::endl;

            /* TTL processing instance. */
            pl_unit = { 0 };
            pl_unit.cb = pl_dec_ttl;
            pl_unit.cb_data.mat_act_pl.id = ID_NW_TTL;
            pl_unit.cb_data.mat_act_pl.val = std::stoi(argv[counter + 1]);
            pp_pipeline.nw_pipeline.push_back(pl_unit);
        }
        else if (!strcmp(argv[counter], TOKEN__DNS_ADDR)) {
            std::cout << "DNS Address value: " << argv[counter + 1] << std::endl;

            /* DNS: Address processing. */
            pl_unit = { 0 };
            pl_unit.cb = pl_mod_dns;
            pl_unit.cb_data.mat_act_pl.id = ID_NW_DNS_ADDR;
            pcpp::IPv4Address addr(argv[counter + 1]);
            pl_unit.cb_data.mat_act_pl.val = addr.toInt();
            pp_pipeline.tp_pipeline.push_back(pl_unit);
        }
        else if (!strcmp(argv[counter], TOKEN__DNS_PORT)) {
            std::cout << "DNS Port value: " << argv[counter + 1] << std::endl;

            /* DNS: Port processing. */
            pl_unit = { 0 };
            pl_unit.cb = pl_mod_dns;
            pl_unit.cb_data.mat_act_pl.id = ID_NW_DNS_PORT;
            pl_unit.cb_data.mat_act_pl.val = std::stoi(argv[counter + 1]);
            pp_pipeline.tp_pipeline.push_back(pl_unit);
        }
    }

    /* Instantiate a pcap file reader and open the input pcap file. */
    pcpp::PcapNgFileReaderDevice reader(in_file);
    if (!reader.open()) {
        std::cout << "Error opening the input pcap file" << std::endl;
        return 1;
    }

    /* Instantiate a pcap file writer and open the input pcap file. */
    pcpp::PcapNgFileWriterDevice pcapWriter(out_file);
    if (!pcapWriter.open()) {
        std::cout << "Error opening the output pcap file" << std::endl;
        return 1;
    }

    /* Start reading the packets in file. */
    while (reader.getNextPacket(rawPacket)) {

        /* Parse the raw packet into a parsed packet. */
        pcpp::Packet parsedPacket(&rawPacket);

        /* Data link layer processing. */
        for (uint8_t counter = 0; counter < pp_pipeline.dl_pipeline.size(); counter++) {
            pl_unit = { 0 };
            pl_unit = pp_pipeline.dl_pipeline.at(counter);
            pl_unit.cb(parsedPacket, pl_unit.cb_data);

            if (pl_unit.cb_data.pkt_verdict & MD_DROP) {
                break;
            }
        }

        /* Network layer processing. */
        if (!(pl_unit.cb_data.pkt_verdict & MD_DROP)) {
            for (uint8_t counter = 0; counter < pp_pipeline.nw_pipeline.size(); counter++) {
                pl_unit = { 0 };
                pl_unit = pp_pipeline.nw_pipeline.at(counter);
                pl_unit.cb(parsedPacket, pl_unit.cb_data);

                if (pl_unit.cb_data.pkt_verdict & MD_DROP) {
                    break;
                }
            }
        }

        /* Transport layer processing. */
        if (!(pl_unit.cb_data.pkt_verdict & MD_DROP)) {
            for (uint8_t counter = 0; counter < pp_pipeline.tp_pipeline.size(); counter++) {
                pl_unit = { 0 };
                pl_unit = pp_pipeline.tp_pipeline.at(counter);
                pl_unit.cb(parsedPacket, pl_unit.cb_data);

                if (pl_unit.cb_data.pkt_verdict & MD_DROP) {
                    break;
                }
            }
        }

        if (!(pl_unit.cb_data.pkt_verdict & MD_DROP)) {

            /* Write packet in the output pcap file. */
            pcapWriter.writePacket(*parsedPacket.getRawPacket());
        }

        /* Update stats. */
        dl_ptr = parsedPacket.getFirstLayer();
        stats.proc_pkt++;
        stats.proc_bytes += dl_ptr->getDataLen();

        /* If the packet was marked for drop, increase drop count. */
        if (pl_unit.cb_data.pkt_verdict & MD_DROP) {
            stats.drop_pkt++;
            stats.drop_bytes += dl_ptr->getDataLen();
        } else {

            /* If any DNS modification is done, increase dns mod counter. */
            if (pl_unit.cb_data.pkt_verdict & MD_DNS_MOD) {
                stats.dns_mod_pkt++;
                stats.dns_mod_pkt += dl_ptr->getDataLen();
            }

            /* If the packet was not dropped, mark it as a written packet. */
            stats.written_pkt++;
            stats.written_bytes += dl_ptr->getDataLen();
        }
    }

    /* Display the stats at the end. */
    std::cout << std::endl;
    std::cout << "Processed Packets: " << stats.proc_pkt << " | " << stats.proc_bytes << " bytes" << std::endl;
    std::cout << "Dropped Packets: " << stats.drop_pkt << " | " << stats.drop_bytes << " bytes" << std::endl;
    std::cout << "Written Packets: " << stats.written_pkt << " | " << stats.written_bytes << " bytes" << std::endl;

    return 0;
}

/* Callback for TTL processing. */
void pl_dec_ttl (pcpp::Packet& prsd_pkt, struct pkt_metadata& metadata) {
    pcpp::iphdr *ipv4_hdr;
    pcpp::ip6_hdr *ipv6_hdr;
    pcpp::IPv4Layer *ipv4_layer;
    pcpp::IPv6Layer *ipv6_layer;

    /* Parse ipv4 layer. */
    if (prsd_pkt.isPacketOfType(pcpp::IPv4)) {

        ipv4_layer = prsd_pkt.getLayerOfType<pcpp::IPv4Layer>();

        if (ipv4_layer != NULL) {
            ipv4_hdr = ipv4_layer->getIPv4Header();

            /* Check if its a tunneled packet. */
            if (ipv4_hdr->protocol == pcpp::PACKETPP_IPPROTO_GRE) {
                            
                /* For simplicity, only a single layer of tunneling is supported. */
                ipv4_layer = prsd_pkt.getNextLayerOfType<pcpp::IPv4Layer>(ipv4_layer);

                if (ipv4_layer != NULL) {
                    
                    /* Get next IPv4 header. */
                    ipv4_hdr = ipv4_layer->getIPv4Header();
                }
            }

            /* Perform processing on TTL value. */
            if (ipv4_hdr->timeToLive <= metadata.mat_act_pl.val) {

                /* Mark the packet for drop. */
                metadata.pkt_verdict |= MD_DROP;
            } else {

                /* Overwrite TTL value. */
                ipv4_hdr->timeToLive -= metadata.mat_act_pl.val;
            }
        }
    } else if (prsd_pkt.isPacketOfType(pcpp::IPv6)) {

        /* Parse ipv6 layer. */
        ipv6_layer = prsd_pkt.getLayerOfType<pcpp::IPv6Layer>();

        if (ipv6_layer != NULL) {

            ipv6_hdr = ipv6_layer->getIPv6Header();

            /* Perform processing on hop limit value. */
            if (ipv6_hdr->hopLimit <= metadata.mat_act_pl.val) {

                /* Mark the packet for drop. */
                metadata.pkt_verdict |= MD_DROP;
            } else {

                /* Overwrite hop limit value. */
                ipv6_hdr->hopLimit -= metadata.mat_act_pl.val;
            }
        }
    }

    return;
}

/* Callback for VLAN processing. */
void pl_vlan_proc (pcpp::Packet& prsd_pkt, struct pkt_metadata& metadata) {
    pcpp::VlanLayer *vlan_layer = NULL;

    /* Get VLAN layer if there is any. */
    vlan_layer = prsd_pkt.getLayerOfType<pcpp::VlanLayer>();

    /* Check to see if we got a valid VLAN layer. */
    if (vlan_layer != NULL) {

        /* Compare VLAN ID with the value specified by user. */
        if ((vlan_layer->getVlanID() == metadata.mat_act_pl.val) ^ metadata.mat_act_pl.mask) {

            /* Drop if VLAN ID does not match. */
            metadata.pkt_verdict |= MD_DROP;
        }
    }

    return;
}

/* Callback for DNS processing. */
void pl_mod_dns (pcpp::Packet& prsd_pkt, struct pkt_metadata& metadata) {
    pcpp::DnsLayer *dns_layer = NULL;
    pcpp::IPv4Layer *ip_layer = NULL;
    pcpp::iphdr *ip_hdr = NULL;
    pcpp::udphdr *udp_hdr = NULL;
    pcpp::UdpLayer *udp_layer = NULL;

    /* Parse UDP Layer if there is any. */
    udp_layer = prsd_pkt.getLayerOfType<pcpp::UdpLayer>();
    if (udp_layer != NULL) {

        /* Parse DNS Layer if there is any. */
        dns_layer = prsd_pkt.getLayerOfType<pcpp::DnsLayer>();
        if (dns_layer != NULL) {

            if (metadata.mat_act_pl.id == ID_NW_DNS_ADDR) {

                /* Get IPv4 layer. */
                ip_layer = prsd_pkt.getLayerOfType<pcpp::IPv4Layer>();

                if (ip_layer != NULL) {

                    /* Parse IPv4 header. */
                    ip_hdr = ip_layer->getIPv4Header();

                    if (ip_hdr != NULL) {

                        /* Check if its a tunneled packet. */
                        if (ip_hdr->protocol == pcpp::PACKETPP_IPPROTO_GRE) {
                            
                            /* For simplicity, only a single layer of tunneling is supported. */
                            ip_layer = prsd_pkt.getNextLayerOfType<pcpp::IPv4Layer>(ip_layer);

                            if (ip_layer != NULL) {
                                
                                /* Get next IPv4 header. */
                                ip_hdr = ip_layer->getIPv4Header();
                            }
                        }

                        /* Overwrite the IP. */
                        dns_layer->getDnsHeader()->queryOrResponse ? ip_hdr->ipDst = metadata.mat_act_pl.val :
                                                                     ip_hdr->ipSrc = metadata.mat_act_pl.val;

                        /* Mark the packet as DNS modified. */
                        metadata.pkt_verdict |= MD_DNS_MOD;
                    }
                }
                    
            } else if (metadata.mat_act_pl.id == ID_NW_DNS_PORT) {

                /* Parse UDP header. */
                udp_hdr = udp_layer->getUdpHeader();

                if (udp_hdr != NULL) {

                    /* Over write UDP port. */
                    (dns_layer->getDnsHeader()->queryOrResponse) ? udp_hdr->portDst = htobe16((uint16_t)metadata.mat_act_pl.val) :
                                                                   udp_hdr->portSrc = htobe16((uint16_t)metadata.mat_act_pl.val);

                    /* Mark the packet as DNS modified. */
                    metadata.pkt_verdict |= MD_DNS_MOD;
                }
            }
        }
    }

    return;
}

/* Callback for protocol processing. */
void pl_proto_filter (pcpp::Packet& prsd_pkt, struct pkt_metadata& metadata) {
    pcpp::Layer *tmp_layer = prsd_pkt.getFirstLayer();

    if (tmp_layer != NULL) {

        /* Filter for data link layer protocol. */
        if (metadata.mat_act_pl.id == ID_DL_PROTO) {

            if ((tmp_layer->getProtocol() == metadata.mat_act_pl.val) ^ metadata.mat_act_pl.mask) {

                metadata.pkt_verdict |= MD_DROP;
            }
        }

        /* Filter for network layer protocol. */
        if (metadata.mat_act_pl.id == ID_NW_PROTO) {

            /* Check if packet contains the specified protocol. */
            if (prsd_pkt.isPacketOfType(metadata.mat_act_pl.val) ^ metadata.mat_act_pl.mask) {

                metadata.pkt_verdict |= MD_DROP;
            }
        }
    }

    return;
}