#ifndef CAMERAAPI_H
#define CAMERAAPI_H

#include <QObject>
#include <QUdpSocket>
#include <QtXml>
#include <QDateTime>
#include <QTimer>
#include "gvcp/gvcpHeaders.h"
#include "gvsp/gvsp.h"
#include "packethandler.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc/types_c.h>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define BIT(x) (1 << x)
#define CAMERA_GVCP_BIND_PORT (0)
#define CAMERA_GVSP_BIND_PORT (0)
#define CAMERA_MAX_WORKER_THREAD_COUNT (1)
#define CAMERA_MAX_ACK_FETCH_RETRY_COUNT (3)
#define CAMERA_MAX_FRAME_BUFFER_SIZE (3)
#define CAMERA_WAIT_FOR_ACK_MS (100)
#define CAMERA_GVSP_PAYLOAD_SIZE (8500)

#define CAMERA_STATUS_FLAGS_INITIALIZED BIT(0)

#define CAMERA_API_STATUS_SUCCESS (0)
#define CAMERA_API_STATUS_FAILED (1)
#define CAMERA_API_ENABLE_RESEND (0)
#define CAMERA_API_STATUS_CAMERA_UNINITIALIZED (2)

/* SwissKnife is not supported. */
const QList<QString> lookupTags = {
    "IntReg",
    "Integer",
    "StringReg",
    "MaskedIntReg",
    "StructReg",
    "Enumeration",
    "IntConverter",
    "StructEntry",
    "Command",
    "Converter"
};

/* General camera properties structure. */
typedef struct {
    quint8 statusFlags;
    quint16 streamPktSize;
    quint16 streamChannelIdx;
    quint32 imageWidth;
    quint32 imageHeight;

} CameraProperties;

class cameraApi : public QObject
{
    Q_OBJECT

public:
    explicit cameraApi(const QHostAddress hostIPv4Addr, QObject *parent = nullptr);

signals:
    void signalDatagramEnqueued();

public slots:
    void slotCameraHeartBeat();
    void slotRequestResendRoutine();
    void slotGvspReadyRead();
    void slotImageReceived();

public:
    /* Public functions that need to be exposed for applications. */
    quint32 cameraReadCameraAttribute(const QList<QString>& attributeList, QList<QByteArray>& regValues);
    quint32 cameraWriteCameraAttribute(const QList<QString>& attributeList, const QList<QByteArray>& regValues);
    quint32 cameraWriteRegisterValue(const QList<strGvcpCmdWriteRegHdr>& writeUnits);
    quint32 cameraReadRegisterValue(const QList<strGvcpCmdReadRegHdr>& addressList, QList<quint32>& regValues);
    quint32 cameraDiscoverDevice(const QHostAddress& destAddr, QList<strGvcpAckDiscoveryHdr>& discAckHdr);
    quint32 cameraInitializeDevice(const QHostAddress& camIP);
    quint32 cameraStartStream();
    quint32 cameraStopStream();

    /* Getters and setters */
    quint8 camStatusFlags() const;

private:
    /* Private camera functionalities for this API. */
    quint32 cameraReadXmlFileFromDevice();
    quint32 cameraRequestResend(const strGvcpCmdPktResendHdr& cmdHdr);
    quint32 cameraFetchAck(strNonStdGvcpAckHdr& ack_hdr, const quint16 reqId);
    quint32 cameraReadMemoryBlock(const quint32 address, const quint16 size, QByteArray& returnedData);
    quint32 cameraXmlFetchXmlFromDevice(const QByteArray& fileName, const QByteArray& startAddress, const QByteArray size, QByteArray& xmlData);
    quint32 cameraXmlFetchAttrElement(const QString& attributeName, QDomNode& node);
    quint32 cameraXmlFetchChildElementValue(const QDomNode& parent, const QString& tagName, QString& value);
    quint32 cameraXmlFetchAttrRegister(const QString& nodeName, QDomNode& regNode);

    /* Private variables. */
    const QHostAddress mHostIP;
    QHostAddress mCamIP;
    QTimer mHeartBeatTimer;
    QUdpSocket mGvcpSock;
    QUdpSocket mGvspSock;
    QDomDocument mCamXmlFile;
    CameraProperties mCamProps;
    QReadWriteLock mHashLocker;
    QReadWriteLock mQueueLocker;
    QThread mStreamingThread;
    Mat mImageBuffer;
    QList<QThread *> mListStreamWorkingThread;
    QList<PacketHandler *> mListPacketHandlers;
    QList<quint8*> mStreamPreAllocatedBuffers;
    QQueue<QNetworkDatagram> mStreamReceiveQueue;
    QQueue<quint16> mPktResendBlockIDQueue;
    QHash<quint16, QHash<quint32, quint8*>> mStreamHT;
};

#endif // CAMERAAPI_H
