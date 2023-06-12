#include "gtest/gtest.h"
#include "SteeringTarget.hpp"

TEST(ArgumentTest, ConstructorException) {
    std::string ip = "0.0.0.0";
    uint16_t port = 0;
    
    ASSERT_THROW(SteeringTarget(pcpp::IPv4Address(ip), port), InvalidArgumentException);
}

TEST(ArgumentTest, IPException) {
    std::string ip = "192.168.10.10";
    uint16_t port = 5555;
    SteeringTarget tgt(pcpp::IPv4Address(ip), port);

    ASSERT_THROW(tgt.setAddress(pcpp::IPv4Address("0.0.0.0")), InvalidArgumentException);
    ASSERT_EQ(tgt.getAddress(), ip);
}

TEST(ArgumentTest, PortException) {
    std::string ip = "192.168.10.10";
    uint16_t port = 5555;
    SteeringTarget tgt(pcpp::IPv4Address(ip), port);

    ASSERT_THROW(tgt.setPort(0), InvalidArgumentException);
    ASSERT_EQ(tgt.getPort(), port);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}