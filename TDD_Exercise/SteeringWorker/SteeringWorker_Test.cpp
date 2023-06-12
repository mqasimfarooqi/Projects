#include <iostream>
#include <IPv4Layer.h>
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "../SteeringTarget/SteeringTarget.hpp"
#include "../SteeringRuntime/SteeringRuntime.hpp"
#include "SteeringWorker.hpp"

using namespace std;

class SteeringRuntimeMock : public SteeringRuntime {
    public:
    MOCK_METHOD(std::shared_ptr<const SteeringRule>, ruleSearch, (pcpp::Packet& packet), (override));
};

class SteeringWorkerTest : public ::testing::Test {
protected:
    SteeringRuntimeMock steeringRuntimeMock;
    SteeringRuntime steeringRuntime;
    pcpp::Packet packet;
    Protocol protocol;
    uint16_t port;
    pcpp::IPv4Address address;

    void SetUp() override {
        address = pcpp::IPv4Address("192.168.10.10");
        protocol = TCP4;
        port = 8080;
        packet.addLayer(new pcpp::EthLayer(MAC_SRC_DUMMY, MAC_DST_DUMMY, pcpp::Ethernet), true);
        packet.addLayer(new pcpp::IPv4Layer(IP_SRC_DUMMY, address), true);
        packet.addLayer(new pcpp::TcpLayer(PORT_SRC_DUMMY, port), true);
    }

    void TearDown() override {
        packet.getRawPacket()->clear();
    }
};

TEST_F(SteeringWorkerTest, Process_CalledRuleSearch) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.10"), 9090);

    steeringRuntimeMock.addRule(protocol, port, address, steeringTarget);
    SteeringWorker steeringWorker(steeringRuntimeMock);

    EXPECT_CALL(steeringRuntimeMock, ruleSearch(::testing::Ref(packet)))
    .WillOnce(::testing::Return(std::shared_ptr<const SteeringRule>()));
    
    EXPECT_FALSE(steeringWorker.process(packet));
}

TEST_F(SteeringWorkerTest, Process_ValidPacketMatch) {
    SteeringRuntimeMock steeringRuntimeMock;
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.10"), 9090);

    steeringRuntime.addRule(protocol, port, address, steeringTarget);
    SteeringWorker steeringWorker(steeringRuntime);
    EXPECT_TRUE(steeringWorker.process(packet));
}

TEST_F(SteeringWorkerTest, Process_InvalidPacket) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.10"), 9090);
    steeringRuntime.addRule(protocol, port, address, steeringTarget);
    SteeringWorker steeringWorker(steeringRuntime);
    
    packet.removeLastLayer();
    ASSERT_THROW(steeringWorker.process(packet), DropPacketException);
}

TEST_F(SteeringWorkerTest, Steer_InvalidPacket) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.10"), 9090);
    steeringRuntime.addRule(protocol, port, address, steeringTarget);
    SteeringWorker steeringWorker(steeringRuntime);
    
    packet.removeLastLayer();
    ASSERT_THROW(steeringWorker.steer(packet, steeringTarget), InvalidProtocolException);
}

TEST_F(SteeringWorkerTest, Steer_TCPPortMangle) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.10"), 9090);
    steeringRuntime.addRule(protocol, port, address, steeringTarget);
    SteeringWorker steeringWorker(steeringRuntime);

    steeringWorker.steer(packet, steeringTarget);
    EXPECT_EQ(packet.getLayerOfType<pcpp::TcpLayer>()->getTcpHeader()->portDst, htobe16(steeringTarget.getPort()));
}

TEST_F(SteeringWorkerTest, Steer_IPMangle) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.10"), 9090);
    steeringRuntime.addRule(protocol, port, address, steeringTarget);
    SteeringWorker steeringWorker(steeringRuntime);

    steeringWorker.steer(packet, steeringTarget);
    EXPECT_EQ(packet.getLayerOfType<pcpp::IPv4Layer>()->getIPv4Header()->ipDst, steeringTarget.getAddress().toInt());
}

TEST_F(SteeringWorkerTest, Steer_UDPPortMangle) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.10"), 9090);
    steeringRuntime.addRule(protocol, port, address, steeringTarget);
    SteeringWorker steeringWorker(steeringRuntime);

    packet.removeLastLayer();
    packet.addLayer(new pcpp::UdpLayer(44444, port), true);
    steeringWorker.steer(packet, steeringTarget);
    EXPECT_EQ(packet.getLayerOfType<pcpp::UdpLayer>()->getUdpHeader()->portDst, htobe16(steeringTarget.getPort()));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
