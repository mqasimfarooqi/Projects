#ifndef CAMERAINTERFACE_H
#define CAMERAINTERFACE_H

#include <QObject>
#include <QThreadPool>
#include <QRunnable>
#include <QUdpSocket>
#include <QDebug>
#include <QNetworkInterface>
#include <QtEndian>
#include <QtNetwork>
#include <QDataStream>
#include "gvcpHeaders.h"

class CameraInterface
{

public:
    /* Send a command to specified address and port. */
    static bool camSendCmd(QUdpSocket *udpSock, const quint16 cmdType, const QByteArray& cmdSpecificData,
                           const QHostAddress& destAddr, const quint16 port, const quint16 reqId);

    /* Receive a packet from UDP socket. */
    /* NOTE: This function allocates memory for command specific acknowledge. */
    static bool camReceiveAck(QUdpSocket *udpSock, strNonStdGvcpAckHdr& ackHeader);

    /* Receive a packet from UDP socket. */
    /* NOTE: This function allocates memory for command specific acknowledge. */
    static bool camReceiveAck(QUdpSocket *udpSock, QByteArray& rawSocketData);
};

#endif // CAMERAINTERFACE_H
