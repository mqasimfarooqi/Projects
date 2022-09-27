#ifndef PACKETHANDLER_H
#define PACKETHANDLER_H

#include <QObject>
#include <QDebug>
#include <QtNetwork>
#include <QMutex>

/* General camera properties structure. */
typedef struct {
    quint8 statusFlags;
    quint16 streamPktSize;
    quint16 streamChannelIdx;

} CameraProperties;

class PacketHandler : public QObject
{
    Q_OBJECT

public:
    explicit PacketHandler(QHash<quint16, QHash<quint32, QByteArray>> *streamHT,
                           quint16 streamPktSize, QMutex *mutex,
                           QQueue<quint16> *blockIDQueue, QQueue<QNetworkDatagram> *datagramQueue,
                           QObject *parent = nullptr);

signals:
    void signalRequestResend();

public slots:
    void slotServicePendingPackets();

private:
    quint16 mPktSize;
    QMutex *mMutexPtr;
    QUdpSocket *mStreamSocketPtr;
    QQueue<quint16> *mQueueFrameResendBlockID;
    QQueue<QNetworkDatagram> *mQueueDatgrams;
    QHash<quint16, QHash<quint32, QByteArray>> *mStreamHashTablePtr;
};

#endif // PACKETHANDLER_H