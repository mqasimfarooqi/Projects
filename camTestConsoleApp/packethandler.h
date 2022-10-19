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
    explicit PacketHandler(QHash<quint16, QHash<quint32, quint8 *>> *streamHT, quint16 streamPktSize,
                           QReadWriteLock *lock, quint32 expectedImageSize,
                           QList<quint8*> rawDataBuffer, QObject *parent = nullptr);

signals:
    void signalRequestResend(quint16 blockID);
    void signalImageAcquisitionComplete(quint8 *dataPtr);

public slots:
    void slotServicePendingPackets(QNetworkDatagram gvspPkt);

public:
    void HashTableCleanup(quint32 entries);

private:
    quint16 mPktSize;
    quint32 mExpectedImageSize;
    QList<quint8*> mRawDataBuffer;
    QReadWriteLock *mLockPtr;
    QHash<quint16, QHash<quint32, quint8*>> *mStreamHashTablePtr;
};

#endif // PACKETHANDLER_H
