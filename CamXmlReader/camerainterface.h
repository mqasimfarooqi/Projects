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
#include "gigevheaders.h"

class CameraInterface : public QObject
{
    Q_OBJECT

public:

    explicit CameraInterface(QObject *parent = nullptr);
    CameraInterface(QUdpSocket *socket);

    /* Send a command to specified address and port. */
    bool camSendCmd(quint32 cmdType, QByteArray *cmdSpecificData, const QHostAddress *addr, quint16 port, quint16 reqId);

    /* Receive a packet from UDP socket. */
    /* NOTE: This function allocates memory for command specific acknowledge. */
    bool camReceiveAck(QByteArray *rawSocketData, strNonStdGvcpAckHdr& ackHeader);

private:
    QUdpSocket *mUdpSock;

};

#endif // CAMERAINTERFACE_H
