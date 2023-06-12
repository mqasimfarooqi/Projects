#ifndef STEERING_RUNTIME_HPP
#define STEERING_RUNTIME_HPP

#include <Packet.h>
#include <IPv4Layer.h>
#include <TcpLayer.h>
#include <UdpLayer.h>
#include <unordered_map>
#include <memory>
#include <iostream>
#include "../SteeringRule/SteeringRule.hpp"
#include "../SteeringTarget/SteeringTarget.hpp"
#include "../Generic/Generic.hpp"

class SteeringRuntime {
    
public:
    SteeringRuntime() {};

    bool addRule(Protocol protocol, SteeringTarget target);
    bool addRule(Protocol protocol, uint16_t port, SteeringTarget target);
    bool addRule(Protocol protocol, uint16_t port, pcpp::IPv4Address address, SteeringTarget target);

    bool removeRule(Protocol protocol);
    bool removeRule(Protocol protocol, uint16_t port);
    bool removeRule(Protocol protocol, uint16_t port, pcpp::IPv4Address address);

    void reset();
    size_t ruleCount();

    virtual std::shared_ptr<const SteeringRule> ruleSearch(pcpp::Packet& packet);

private:
    std::unordered_map<std::string, std::shared_ptr<SteeringRule>> m_rules;
    std::string generateRuleId(Protocol protocol, uint16_t port, pcpp::IPv4Address address) {
        return std::to_string(protocol) + std::to_string(port) + address.toString();
    }
    bool packetMatchesRule(pcpp::Packet& packet, const SteeringRule& rule) {
        if (packet.isPacketOfType(rule.getProtocol())) {
            if (packet.isPacketOfType(pcpp::TCP)) {
                if (packet.getLayerOfType<pcpp::TcpLayer>()->getTcpHeader()->portDst == htobe16(rule.getPort())) {
                    if (packet.getLayerOfType<pcpp::IPv4Layer>()->getIPv4Header()->ipDst == rule.getAddress().toInt()) {
                        return true;
                    }
                }
            } else {
                if (packet.getLayerOfType<pcpp::UdpLayer>()->getUdpHeader()->portDst == htobe16(rule.getPort())) {
                    if (packet.getLayerOfType<pcpp::IPv4Layer>()->getIPv4Header()->ipDst == rule.getAddress().toInt()) {
                        return true;
                    }
                }
            }
        }

        return false;
    }
};


#endif /* !STEERING_RUNTIME_HPP */