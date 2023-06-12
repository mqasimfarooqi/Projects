#include <iostream>
#include <signal.h>
#include <string.h>
#include <IpAddress.h>
#include <IPv4Layer.h>
#include <TcpLayer.h>
#include <UdpLayer.h>
#include <Packet.h>
#include <PcapLiveDevice.h>
#include <PcapLiveDeviceList.h>
#include <unordered_map>
#include <gtest/gtest.h>
#include "Generic/Generic.hpp"
#include "ProtocolUtil/ProtocolUtil.hpp"
#include "SteeringTarget/SteeringTarget.hpp"
#include "SteeringRule/SteeringRule.hpp"

using namespace std;

#define AG_SUCCESS                      (0)
#define AG_FAIL                         (1)

int main(int argc, char* argv[]) {
    uint8_t status = AG_SUCCESS;

    return status;
}