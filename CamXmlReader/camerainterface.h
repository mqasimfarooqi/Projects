#ifndef CAMERAINTERFACE_H
#define CAMERAINTERFACE_H

#include <QObject>
#include <QThreadPool>
#include <QRunnable>
#include <QUdpSocket>
#include <QDebug>
#include <QNetworkInterface>
#include <QtEndian>
#include <QRandomGenerator>
#include <QtNetwork>
#include "gvcpHeaders.h"

class CameraInterface : public QObject
{
    Q_OBJECT

public:
    /* Send a command to specified address and port. */
    static bool camSendCmd(QUdpSocket *udpSock, quint32 cmdType, QByteArray *cmdSpecificData,
                           const QHostAddress *addr, quint16 port, quint16 reqId);

    /* Receive a packet from UDP socket. */
    /* NOTE: This function allocates memory for command specific acknowledge. */
    static bool camReceiveAck(QUdpSocket *udpSock, QByteArray *rawSocketData,
                              strNonStdGvcpAckHdr& ackHeader);

private:
    explicit CameraInterface(QObject *parent = nullptr);

};

#endif // CAMERAINTERFACE_H
