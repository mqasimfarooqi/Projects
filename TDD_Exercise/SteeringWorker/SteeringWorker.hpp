#ifndef STEERING_WORKER_HPP
#define STEERING_WORKER_HPP

#include <Packet.h>
#include <EthLayer.h>
#include <IPv4Layer.h>
#include <TcpLayer.h>
#include <UdpLayer.h>
#include "../SteeringRuntime/SteeringRuntime.hpp"
#include "../Generic/Generic.hpp"

class SteeringWorker {

    public:
    SteeringWorker(SteeringRuntime& steeringRuntime) : m_SteeringRuntime(steeringRuntime) {};
    bool process(pcpp::Packet &packet);
    void steer(pcpp::Packet &packet, SteeringTarget &target);

    private:
    SteeringRuntime& m_SteeringRuntime;
};

#endif /* !STEERING_WORKER_HPP */