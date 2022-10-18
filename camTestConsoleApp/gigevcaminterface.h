#ifndef GIGEVCAMINTERFACE_H
#define GIGEVCAMINTERFACE_H

#include <QObject>
#include <QUdpSocket>
#include <QDebug>
#include <QtNetwork>
#include <QDataStream>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define CAMERA_INTERFACE_STATUS_SUCCESS (0)
#define CAMERA_INTERFACE_STATUS_FAILED (1)

class gigevCamInterface
{

public:
    /* Send a command to specified address and port. */
    static bool camGigeVEthSendCmd(QUdpSocket& udpSock, QNetworkDatagram& datagram);

    /* Receive a packet from UDP socket overloaded. */
    static bool camGigeVEthReceiveAck(QUdpSocket& udpSock, QNetworkDatagram& rawSocketData);
};

#endif // GIGEVCAMINTERFACE_H
