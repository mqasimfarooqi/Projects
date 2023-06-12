#ifndef STEERING_RULE_HPP
#define STEERING_RULE_HPP

#include <Packet.h>
#include <IPv4Layer.h>
#include "../SteeringTarget/SteeringTarget.hpp"
#include "../Generic/Generic.hpp"

class SteeringRule {

    public:
    SteeringRule(Protocol protocol, SteeringTarget target, uint16_t port = 0, pcpp::IPv4Address address = pcpp::IPv4Address("0.0.0.0"))
                : m_protocol(protocol), m_target(target), m_port(port), m_address(address) {};

    /* Getters. */
    Protocol getProtocol() const;
    pcpp::IPv4Address getAddress() const;
    uint16_t getPort() const;
    SteeringTarget getTarget() const;
    std::string getId();

    /* Business Logic. */
    bool matches(pcpp::Packet &packet);

    private:
    Protocol m_protocol;
    SteeringTarget m_target;
    uint16_t m_port;
    pcpp::IPv4Address m_address;
};

#endif /* !STEERING_RULE_HPP */