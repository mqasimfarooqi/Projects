#ifndef CAMINTERFACE_H
#define CAMINTERFACE_H

#include <QObject>
#include <QUdpSocket>
#include <QDebug>
#include <QtNetwork>
#include <QDataStream>

class caminterface
{

public:
    /* Send a command to specified address and port. */
    static bool camGigeVEthSendCmd(QUdpSocket& udpSock, QNetworkDatagram& datagram);

    /* Receive a packet from UDP socket overloaded. */
    static bool camGigeVEthReceiveAck(QUdpSocket& udpSock, QNetworkDatagram& rawSocketData);
};

#endif // CAMINTERFACE_H
