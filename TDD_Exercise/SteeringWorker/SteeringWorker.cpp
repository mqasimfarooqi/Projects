#include <Packet.h>
#include <iostream>
#include "SteeringWorker.hpp"
#include "../Generic/Generic.hpp"

using namespace std;

bool SteeringWorker::process(pcpp::Packet &packet) {
    shared_ptr<const SteeringRule> rule;
    try {
        rule = m_SteeringRuntime.ruleSearch(packet);
    } catch (const exception& e) {
        throw DropPacketException();
    }
    
    if (rule != nullptr) {
        if (rule->getProtocol()._value == Protocol::TCP4) {
            packet.getLayerOfType<pcpp::TcpLayer>()->getTcpHeader()->portDst = htobe16(rule->getTarget().getPort());
        } else {
            packet.getLayerOfType<pcpp::UdpLayer>()->getUdpHeader()->portDst = htobe16(rule->getTarget().getPort());
        }
        packet.getLayerOfType<pcpp::IPv4Layer>()->getIPv4Header()->ipDst = rule->getTarget().getAddress().toInt();
        return true;
    }

    return false;
}

void SteeringWorker::steer(pcpp::Packet &packet, SteeringTarget &target) {
    if (!packet.isPacketOfType(Protocol::TCP4) && !packet.isPacketOfType(Protocol::UDP4)) {
        throw InvalidProtocolException();
    }

    if (packet.isPacketOfType(pcpp::IPv4)) {
        if (target.getAddress().toInt() != 0) {
            packet.getLayerOfType<pcpp::IPv4Layer>()->getIPv4Header()->ipDst = target.getAddress().toInt();
        }
    }

    if (target.getPort() != 0) {
        if (packet.isPacketOfType(Protocol::TCP4)) {
            packet.getLayerOfType<pcpp::TcpLayer>()->getTcpHeader()->portDst = htobe16(target.getPort());
        } else {
            packet.getLayerOfType<pcpp::UdpLayer>()->getUdpHeader()->portDst = htobe16(target.getPort());
        }
    }
}