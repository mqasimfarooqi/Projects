#include <Packet.h>
#include <iostream>
#include <IPv4Layer.h>
#include <UdpLayer.h>
#include <TcpLayer.h>
#include "SteeringRuntime.hpp"
#include "../SteeringRule/SteeringRule.hpp"
#include "../SteeringTarget/SteeringTarget.hpp"
#include "../Generic/Generic.hpp"

using namespace std;

bool SteeringRuntime::addRule(Protocol protocol, uint16_t port, pcpp::IPv4Address address, SteeringTarget target) {
    if (protocol._value != Protocol::TCP4 && protocol._value != Protocol::UDP4) {
        throw InvalidProtocolException();
    }

    std::string ruleId = generateRuleId(protocol, port, address);
    if (m_rules.count(ruleId) > 0) {
        throw DuplicatedTargetException();
    }

    std::shared_ptr<SteeringRule> rule = std::make_shared<SteeringRule>(protocol, target, port, address);
    oneapi::tbb::concurrent_hash_map<std::string, std::shared_ptr<SteeringRule>>::accessor acc;
    m_rules.insert(acc, ruleId);
    acc->second = rule;
    
    return true;
}

bool SteeringRuntime::addRule(Protocol protocol, SteeringTarget target) {
    return addRule(protocol, 0, pcpp::IPv4Address(), target);
}

bool SteeringRuntime::addRule(Protocol protocol, uint16_t port, SteeringTarget target) {
    return addRule(protocol, port, pcpp::IPv4Address(), target);
}

bool SteeringRuntime::removeRule(Protocol protocol, uint16_t port, pcpp::IPv4Address address) {
    SteeringTarget target(address, port);
    if (protocol._value != Protocol::TCP4 && protocol._value != Protocol::UDP4) {
        throw InvalidProtocolException();
    }

    std::string ruleId = generateRuleId(protocol, port, address);

    if (m_rules.count(ruleId) > 0) {
        m_rules.erase(ruleId);
        return true;
    }

    return false;
}

std::shared_ptr<const SteeringRule> SteeringRuntime::ruleSearch(pcpp::Packet& packet) {
    if (!packet.isPacketOfType(Protocol::TCP4) && !packet.isPacketOfType(Protocol::UDP4)) {
        throw InvalidProtocolException();
    }
    
    for (const auto& rulePair : m_rules) {
        const std::shared_ptr<const SteeringRule>& rule = rulePair.second;
        if (packetMatchesRule(packet, *rule)) {
            return rule;
        }
    }

    return nullptr;
}

bool SteeringRuntime::removeRule(Protocol protocol) {
    return removeRule(protocol, 0, pcpp::IPv4Address());
}

bool SteeringRuntime::removeRule(Protocol protocol, uint16_t port) {
    return removeRule(protocol, port, pcpp::IPv4Address());
}

void SteeringRuntime::reset() {
    m_rules.clear();
}

size_t SteeringRuntime::ruleCount() {
    return m_rules.size();
}