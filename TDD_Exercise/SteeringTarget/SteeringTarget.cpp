#include <Packet.h>
#include "SteeringTarget.hpp"
#include "../Generic/Generic.hpp"

SteeringTarget::SteeringTarget(pcpp::IPv4Address address, uint16_t port) {
    if ((address.toInt() == 0) || (port == 0)) {
        throw InvalidArgumentException();
    } else {
        m_addr = address;
        m_port = port;
    }
}

pcpp::IPv4Address SteeringTarget::getAddress() const {
    return m_addr;
}

void SteeringTarget::setAddress(pcpp::IPv4Address address) {
    if (address.toInt() == 0) {
        throw InvalidArgumentException();
    } else {
        m_addr = address;
    }
}

uint16_t SteeringTarget::getPort() const {
    return m_port;
}

void SteeringTarget::setPort(uint16_t port) {
    if (port == 0) {
        throw InvalidArgumentException();
    } else {
        m_port = port;
    }
}

bool SteeringTarget::operator==(const SteeringTarget& obj) const {
    if ((obj.getAddress() == m_addr) && (obj.getPort() == m_port))
        return true;
    else
        return false;
}