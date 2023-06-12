#include <Packet.h>
#include <iostream>
#include <IPv4Layer.h>
#include <UdpLayer.h>
#include <TcpLayer.h>
#include "SteeringRule.hpp"
#include "../Generic/Generic.hpp"

using namespace std;

Protocol SteeringRule::getProtocol() const {
    return m_protocol;
}

pcpp::IPv4Address SteeringRule::getAddress() const {
    return m_address;
}

uint16_t SteeringRule::getPort() const {
    return m_port;
}

SteeringTarget SteeringRule::getTarget() const {
    return m_target;
}

std::string SteeringRule::getId() {
    std::string id = "";
    id += to_string(m_protocol);
    m_port != 0 ? id += "-" + to_string(m_port) : id;
    m_address.toInt() != 0 ? id += "-" + m_address.toString() : id;
    return id;
}

bool SteeringRule::matches(pcpp::Packet &packet) {
    pcpp::IPv4Layer *ip_layer = nullptr;

    if (!packet.isPacketOfType(m_protocol)) {
        return false;
    }

    ip_layer = packet.getLayerOfType<pcpp::IPv4Layer>();
    if (ip_layer) {
        if (!((ip_layer->getIPv4Header()->ipDst == m_address.toInt()) || (m_address.toInt() == 0))) {
            return false;
        }
    }

    if (m_protocol == pcpp::TCP) {
        if (!((packet.getLayerOfType<pcpp::TcpLayer>()->getTcpHeader()->portDst == htobe16(m_port)) || (m_port == 0))) {
            return false;
        }
    } else {
        if (!((packet.getLayerOfType<pcpp::UdpLayer>()->getUdpHeader()->portDst == htobe16(m_port)) || (m_port == 0))) {
            return false;
        }
    }

    return true;
}