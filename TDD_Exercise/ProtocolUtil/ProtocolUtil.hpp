#ifndef PROTOCOL_UTIL_HPP
#define PROTOCOL_UTIL_HPP

#include <Packet.h>
#include "../Generic/Generic.hpp"

class ProtocolUtil {
    private:
    ProtocolUtil();
    public:
    static Protocol detect(pcpp::Packet &packet);
};

#endif /* !PROTOCOL_UTIL_HPP */