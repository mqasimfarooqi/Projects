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
    MOCK_METHOD(bool, addRule, (Protocol protocol, uint16_t port, pcpp::IPv4Address address, SteeringTarget target), (override));
};

class SteeringWorkerTest : public ::testing::Test {
protected:
    SteeringRuntimeMock steeringRuntimeMock;
    pcpp::Packet packet;
    Protocol protocol = Protocol::TCP4;
    uint16_t port;
    pcpp::IPv4Address address;

    void SetUp() override {
        address = pcpp::IPv4Address("192.168.10.10");
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

    EXPECT_CALL(steeringRuntimeMock, addRule(protocol, port, address, steeringTarget))
    .WillOnce(::testing::Return(true));

    steeringRuntimeMock.addRule(protocol, port, address, steeringTarget);
    SteeringWorker steeringWorker(steeringRuntimeMock);

    EXPECT_CALL(steeringRuntimeMock, ruleSearch(::testing::Ref(packet)))
    .WillOnce(::testing::Return(std::shared_ptr<const SteeringRule>()));
    
    EXPECT_FALSE(steeringWorker.process(packet));
}

TEST_F(SteeringWorkerTest, Process_ValidPacketMatch) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.10"), 9090);

    EXPECT_CALL(steeringRuntimeMock, addRule(protocol, port, address, steeringTarget))
    .WillOnce(::testing::Return(true));
    steeringRuntimeMock.addRule(protocol, port, address, steeringTarget);

    SteeringWorker steeringWorker(steeringRuntimeMock);

    EXPECT_CALL(steeringRuntimeMock, ruleSearch(::testing::Ref(packet)))
    .WillOnce(::testing::Return(std::shared_ptr<const SteeringRule>(new SteeringRule(protocol, steeringTarget, port, address))));
    EXPECT_TRUE(steeringWorker.process(packet));
}

TEST_F(SteeringWorkerTest, Process_InvalidPacket) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.10"), 9090);

    EXPECT_CALL(steeringRuntimeMock, addRule(protocol, port, address, steeringTarget))
    .WillOnce(::testing::Return(true));
    steeringRuntimeMock.addRule(protocol, port, address, steeringTarget);
    SteeringWorker steeringWorker(steeringRuntimeMock);
    
    packet.removeLastLayer();
    EXPECT_CALL(steeringRuntimeMock, ruleSearch(::testing::Ref(packet)))
    .WillOnce(::testing::Throw(DropPacketException()));
    ASSERT_THROW(steeringWorker.process(packet), DropPacketException);
}

TEST_F(SteeringWorkerTest, Steer_InvalidPacket) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.10"), 9090);
    EXPECT_CALL(steeringRuntimeMock, addRule(protocol, port, address, steeringTarget))
    .WillOnce(::testing::Return(true));
    steeringRuntimeMock.addRule(protocol, port, address, steeringTarget);
    SteeringWorker steeringWorker(steeringRuntimeMock);
    
    packet.removeLastLayer();
    ASSERT_THROW(steeringWorker.steer(packet, steeringTarget), InvalidProtocolException);
}

TEST_F(SteeringWorkerTest, Steer_TCPPortMangle) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.10"), 9090);
    EXPECT_CALL(steeringRuntimeMock, addRule(protocol, port, address, steeringTarget))
    .WillOnce(::testing::Return(true));
    steeringRuntimeMock.addRule(protocol, port, address, steeringTarget);
    SteeringWorker steeringWorker(steeringRuntimeMock);

    steeringWorker.steer(packet, steeringTarget);
    EXPECT_EQ(packet.getLayerOfType<pcpp::TcpLayer>()->getTcpHeader()->portDst, htobe16(steeringTarget.getPort()));
}

TEST_F(SteeringWorkerTest, Steer_IPMangle) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.10"), 9090);
    EXPECT_CALL(steeringRuntimeMock, addRule(protocol, port, address, steeringTarget))
    .WillOnce(::testing::Return(true));
    steeringRuntimeMock.addRule(protocol, port, address, steeringTarget);
    SteeringWorker steeringWorker(steeringRuntimeMock);

    steeringWorker.steer(packet, steeringTarget);
    EXPECT_EQ(packet.getLayerOfType<pcpp::IPv4Layer>()->getIPv4Header()->ipDst, steeringTarget.getAddress().toInt());
}

TEST_F(SteeringWorkerTest, Steer_UDPPortMangle) {
    SteeringTarget steeringTarget(pcpp::IPv4Address("172.19.17.10"), 9090);
    EXPECT_CALL(steeringRuntimeMock, addRule(protocol, port, address, steeringTarget))
    .WillOnce(::testing::Return(true));
    steeringRuntimeMock.addRule(protocol, port, address, steeringTarget);
    SteeringWorker steeringWorker(steeringRuntimeMock);

    packet.removeLastLayer();
    packet.addLayer(new pcpp::UdpLayer(44444, port), true);
    steeringWorker.steer(packet, steeringTarget);
    EXPECT_EQ(packet.getLayerOfType<pcpp::UdpLayer>()->getUdpHeader()->portDst, htobe16(steeringTarget.getPort()));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
