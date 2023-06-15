#ifndef GENERIC_HPP
#define GENERIC_HPP

#include <exception>
#include <EthLayer.h>
#include <IPv4Layer.h>
#include "enum.hpp"

#define MAC_SRC_DUMMY           (pcpp::MacAddress({0x01,0x02,0x03,0x04,0x05,0x06}))
#define MAC_DST_DUMMY           (pcpp::MacAddress({0x11,0x22,0x33,0x44,0x55,0x66}))
#define IP_SRC_DUMMY            (pcpp::IPv4Address("172.19.17.10"))
#define PORT_SRC_DUMMY          (25252)

BETTER_ENUM(Protocol, int, TCP4 = 0x08, TCP6, UDP4 = 0x10, UDP6, UNKNOWN = 0x00)

class DropPacketException : public std::exception {
public:
    explicit DropPacketException() {}

    const char* what() const noexcept override {
        return "DropPacketException occurred";
    }
};

class DuplicatedTargetException : public std::exception {
public:
    explicit DuplicatedTargetException() {}

    const char* what() const noexcept override {
        return "DuplicatedTargetException occurred";
    }
};

class InvalidArgumentException : public std::exception {
public:
    explicit InvalidArgumentException() {}

    const char* what() const noexcept override {
        return "InvalidArgumentException occurred";
    }
};

class InvalidProtocolException : public std::exception {
public:
    explicit InvalidProtocolException() {}

    const char* what() const noexcept override {
        return "InvalidProtocolException occurred";
    }
};

#endif /* !GENERIC_HPP */