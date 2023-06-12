#include <EthLayer.h>
#include <IPv4Layer.h>
#include <TcpLayer.h>
#include <UdpLayer.h>
#include <memory>
#include "gtest/gtest.h"
#include "SteeringRuntime.hpp"

using namespace std;

class SteeringRuntimeTest : public ::testing::Test {
protected:
    SteeringRuntime steeringRuntime;
    pcpp::Packet packet;
    Protocol protocol;
    uint16_t port;
    pcpp::IPv4Address address;

    void SetUp() override {
        address = pcpp::IPv4Address("192.168.0.1");
        protocol = TCP4;
        port = 8080;
        packet.addLayer(new pcpp::EthLayer(MAC_SRC_DUMMY, MAC_DST_DUMMY, pcpp::Ethernet), true);
        packet.addLayer(new pcpp::IPv4Layer(IP_SRC_DUMMY, address), true);
        packet.addLayer(new pcpp::TcpLayer(PORT_SRC_DUMMY, port), true);
    }

    void TearDown() override {
        packet.getRawPacket()->clear();
        steeringRuntime.reset();
    }
};

TEST_F(SteeringRuntimeTest, AddRule_ValidRule) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.20"), 9090);

    bool result = steeringRuntime.addRule(protocol, port, address, steeringTarget);

    EXPECT_TRUE(result);
    EXPECT_EQ(steeringRuntime.ruleCount(), 1);
}

TEST_F(SteeringRuntimeTest, AddRule_InvalidProtocol) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.20"), 9090);
    Protocol invalidProtocol = static_cast<Protocol>(100);

    EXPECT_THROW(steeringRuntime.addRule(invalidProtocol, 80, steeringTarget), InvalidProtocolException);
    EXPECT_EQ(steeringRuntime.ruleCount(), 0);
}

TEST_F(SteeringRuntimeTest, AddRule_DuplicatedTarget) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.20"), 9090);

    steeringRuntime.addRule(protocol, port, address, steeringTarget);

    EXPECT_THROW(steeringRuntime.addRule(protocol, port, address, steeringTarget), DuplicatedTargetException);
    EXPECT_EQ(steeringRuntime.ruleCount(), 1);
}

TEST_F(SteeringRuntimeTest, RemoveRule_ExistingRule) {
    SteeringTarget steeringTarget(address, port);

    steeringRuntime.addRule(protocol, port, address, steeringTarget);
    bool result = steeringRuntime.removeRule(protocol, port, address);

    EXPECT_TRUE(result);
    EXPECT_EQ(steeringRuntime.ruleCount(), 0);
}

TEST_F(SteeringRuntimeTest, RemoveRule_NonExistingRule) {
    bool result = steeringRuntime.removeRule(protocol, port, address);

    EXPECT_FALSE(result);
    EXPECT_EQ(steeringRuntime.ruleCount(), 0);
}

TEST_F(SteeringRuntimeTest, RuleSearch_MatchingRule) {
    SteeringTarget steeringTarget(address, port);
    std::shared_ptr<SteeringRule> rule = std::make_shared<SteeringRule>(protocol, steeringTarget, port, address);

    steeringRuntime.addRule(protocol, port, address, steeringTarget);

    std::shared_ptr<const SteeringRule> matchedRule = steeringRuntime.ruleSearch(packet);

    EXPECT_NE(matchedRule, nullptr);
    EXPECT_EQ(matchedRule->getAddress().toString(), rule->getAddress().toString());
    EXPECT_EQ(matchedRule->getPort(), rule->getPort());
    EXPECT_EQ(matchedRule->getProtocol(), rule->getProtocol());
}

TEST_F(SteeringRuntimeTest, RuleSearch_NoMatchingRule) {
    SteeringTarget steeringTarget(address, port);
    std::shared_ptr<SteeringRule> rule = std::make_shared<SteeringRule>(protocol, steeringTarget, port, address);

    steeringRuntime.addRule(protocol, port, address, steeringTarget);
    packet.getLayerOfType<pcpp::IPv4Layer>()->getIPv4Header()->ipDst = pcpp::IPv4Address("172.19.17.20").toInt();
    std::shared_ptr<const SteeringRule> matchedRule = steeringRuntime.ruleSearch(packet);
    EXPECT_EQ(matchedRule, nullptr);
}

TEST_F(SteeringRuntimeTest, RuleCount_IncreaseDecrease) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.20"), 9090);

    steeringRuntime.addRule(protocol, port, address, steeringTarget);
    EXPECT_EQ(steeringRuntime.ruleCount(), 1);

    bool result = steeringRuntime.removeRule(protocol, port, address);
    EXPECT_EQ(result, true);
    EXPECT_EQ(steeringRuntime.ruleCount(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}