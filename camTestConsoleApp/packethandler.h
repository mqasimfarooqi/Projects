#ifndef PACKETHANDLER_H
#define PACKETHANDLER_H

#include <QObject>
#include <QDebug>
#include <QtNetwork>
#include <QMutex>

class PacketHandler : public QObject
{
    Q_OBJECT

public:
    explicit PacketHandler(QHash<quint16, QHash<quint32, QByteArray>> *streamHT,
                           quint16 streamPktSize, QReadWriteLock *hashLocker, QReadWriteLock *queueLocker,
                           QQueue<quint16> *blockIDQueue, QQueue<QNetworkDatagram> *datagramQueue,
                           QObject *parent = nullptr);

signals:
    void signalRequestResend();

public slots:
    void slotServicePendingPackets();

private:
    quint16 mPktSize;
    QReadWriteLock *mHashLockerPtr;
    QReadWriteLock *mQueueLockerPtr;
    QUdpSocket *mStreamSocketPtr;
    QQueue<quint16> *mQueueFrameResendBlockID;
    QQueue<QNetworkDatagram> *mQueueDatgrams;
    QHash<quint16, QHash<quint32, QByteArray>> *mStreamHashTablePtr;
};

#endif // PACKETHANDLER_H
