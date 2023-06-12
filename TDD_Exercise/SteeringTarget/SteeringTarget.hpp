#ifndef STEERING_TARGET_HPP
#define STEERING_TARGET_HPP

#include <Packet.h>
#include <IPv4Layer.h>
#include "../Generic/Generic.hpp"

class SteeringTarget {

    public:
    SteeringTarget(pcpp::IPv4Address address, uint16_t port);

    /* Setters. */
    void setAddress(pcpp::IPv4Address address);
    void setPort(uint16_t port);

    /* Getters. */
    pcpp::IPv4Address getAddress() const;
    uint16_t getPort() const;

    /* Operator overloading. */
    bool operator==(const SteeringTarget& obj);

    private:
    pcpp::IPv4Address m_addr;
    uint16_t m_port;
};

#endif /* !STEERING_TARGET_HPP */