#ifndef CAMETHINTERFACE_H
#define CAMETHINTERFACE_H

#include <QObject>
#include <QUdpSocket>
#include <QDebug>
#include <QtNetwork>
#include <QDataStream>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define CAMERA_ETHERNET_INTERFACE_STATUS_SUCCESS (0)
#define CAMERA_ETHERNET_INTERFACE_STATUS_FAILED (1)

class CamEthernetInterface
{

public:
    /* Send a command to specified address and port. */
    static bool camEthernetSendCmd(QUdpSocket& udpSock, QNetworkDatagram& datagram);

    /* Receive a packet from UDP socket overloaded. */
    static bool camEthernetReceiveAck(QUdpSocket& udpSock, QNetworkDatagram& rawSocketData);
};

#endif // CAMETHINTERFACE_H
