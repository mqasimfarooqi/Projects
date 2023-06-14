#include <EthLayer.h>
#include <IPv4Layer.h>
#include <TcpLayer.h>
#include <UdpLayer.h>
#include "gtest/gtest.h"
#include "ProtocolUtil.hpp"

class ProtocolUtilTest : public ::testing::Test {
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

TEST_F(ProtocolUtilTest, TCPTest) {
  EXPECT_EQ(ProtocolUtil::detect(packet)._value, Protocol::TCP4);
}

TEST_F(ProtocolUtilTest, UDPTest) {
  packet.removeLastLayer();
  packet.addLayer(new pcpp::UdpLayer(PORT_SRC_DUMMY, port), true);
  EXPECT_EQ(ProtocolUtil::detect(packet)._value, Protocol::UDP4);
}

TEST_F(ProtocolUtilTest, UnknownTest) {
  packet.removeLastLayer();
  EXPECT_EQ(ProtocolUtil::detect(packet)._value, Protocol::UNKNOWN);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}