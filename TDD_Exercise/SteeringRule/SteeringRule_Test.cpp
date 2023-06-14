#include <EthLayer.h>
#include <IPv4Layer.h>
#include <TcpLayer.h>
#include <UdpLayer.h>
#include "gtest/gtest.h"
#include "SteeringRule.hpp"

using namespace std;

class SteeringRuleTest : public ::testing::Test {
protected:
    pcpp::Packet packet;
    Protocol protocol = Protocol::TCP4;
    uint16_t port;
    pcpp::IPv4Address address;

    void SetUp() override {
        address = pcpp::IPv4Address("192.168.0.1");
        protocol = Protocol::TCP4;
        port = 8080;
        packet.addLayer(new pcpp::EthLayer(MAC_SRC_DUMMY, MAC_DST_DUMMY, pcpp::Ethernet), true);
        packet.addLayer(new pcpp::IPv4Layer(IP_SRC_DUMMY, address), true);
        packet.addLayer(new pcpp::TcpLayer(PORT_SRC_DUMMY, port), true);
    }

    void TearDown() override {
        packet.getRawPacket()->clear();
    }
};

TEST_F(SteeringRuleTest, DataTest_ConstructorTest) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("192.168.10.20"), 12345);
    SteeringRule rule1(protocol, steeringTarget, port, address);

    ASSERT_EQ(steeringTarget == rule1.getTarget(), true);
    ASSERT_EQ(rule1.getProtocol(), protocol);
    ASSERT_EQ(rule1.getAddress(), address);
    ASSERT_EQ(rule1.getPort(), port);

    SteeringRule rule2(protocol, steeringTarget);
    ASSERT_EQ(rule2.getAddress(), pcpp::IPv4Address("0.0.0.0"));
    ASSERT_EQ(rule2.getPort(), 0);
}

TEST_F(SteeringRuleTest, DataTest_GetterTest) {
    SteeringTarget target(pcpp::IPv4Address("192.168.10.20"), 5555);
    SteeringRule rule(protocol, target, port, address);

    ASSERT_EQ(target == rule.getTarget(), true);
    ASSERT_EQ(rule.getProtocol(), protocol);
    ASSERT_EQ(rule.getAddress(), address);
    ASSERT_EQ(rule.getPort(), port);
}

TEST_F(SteeringRuleTest, GetIDTest) {
    SteeringTarget target(pcpp::IPv4Address("192.168.10.20"), 5555);
    SteeringRule rule1(protocol, target, port, address);
    SteeringRule rule2(protocol, target, port);
    SteeringRule rule3(protocol, target);

    EXPECT_EQ(rule1.getId(), to_string(protocol) + "-" + to_string(rule1.getPort()) + "-" + address.toString());
    EXPECT_EQ(rule2.getId(), to_string(protocol) + "-" + to_string(rule1.getPort()));
    EXPECT_EQ(rule3.getId(), to_string(protocol));
}

TEST_F(SteeringRuleTest, MatchesTest) {
    SteeringTarget target(pcpp::IPv4Address("192.168.10.20"), 12345);
    SteeringRule rule1(protocol, target, port, address);
    SteeringRule rule2(protocol, target, port, pcpp::IPv4Address("172.19.17.20"));
    SteeringRule rule3(protocol, target, 1223);
    SteeringRule rule4(protocol, target);

    EXPECT_EQ(rule1.matches(packet), true);
    EXPECT_EQ(rule2.matches(packet), false);
    EXPECT_EQ(rule3.matches(packet), false);
    EXPECT_EQ(rule4.matches(packet), true);

    packet.removeLastLayer();
    EXPECT_EQ(rule1.matches(packet), false);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}