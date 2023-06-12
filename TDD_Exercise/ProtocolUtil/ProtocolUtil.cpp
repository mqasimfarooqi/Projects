#include <UdpLayer.h>
#include <TcpLayer.h>
#include <Packet.h>
#include "ProtocolUtil.hpp"
#include "../Generic/Generic.hpp"

Protocol ProtocolUtil::detect (pcpp::Packet &packet) {
    if (packet.isPacketOfType(pcpp::TCP)) {
        return TCP4;
    } else if (packet.isPacketOfType(pcpp::UDP)) {
        return UDP4;
    } else {
        return UNKNOWN;
    }
}