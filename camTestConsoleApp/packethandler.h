#ifndef PACKETHANDLER_H
#define PACKETHANDLER_H

#include <QObject>
#include <QDebug>
#include <QtNetwork>
#include <QMutex>
#include <opencv2/core.hpp>

using namespace cv;

class PacketHandler : public QObject
{
    Q_OBJECT

public:
    explicit PacketHandler(QHash<quint16, QHash<quint32, quint8 *>> *streamHT,
                           quint16 streamPktSize, QReadWriteLock *hashLocker, QReadWriteLock *queueLocker,
                           QQueue<quint16> *blockIDQueue, QQueue<QNetworkDatagram> *datagramQueue,
                           Mat *imageBuffer, quint32 expectedImageSize, QList<quint8*> rawDataBuffer,
                           QObject *parent = nullptr);

signals:
    void signalRequestResend();
    void signalImageAcquisitionComplete();

public slots:
    void slotServicePendingPackets();

public:
    void hashTableCleanup(quint32 entries);

private:
    quint16 mPktSize;
    quint32 mExpectedImageSize;
    Mat *mImageBuffer;
    QList<quint8*> mRawDataBuffer;
    QReadWriteLock *mHashLockerPtr;
    QReadWriteLock *mQueueLockerPtr;
    QUdpSocket *mStreamSocketPtr;
    QQueue<quint16> *mQueueFrameResendBlockID;
    QQueue<QNetworkDatagram> *mQueueDatgrams;
    QHash<quint16, QHash<quint32, quint8*>> *mStreamHashTablePtr;
};

#endif // PACKETHANDLER_H
