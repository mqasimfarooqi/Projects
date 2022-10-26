#ifndef CAMAPI_H
#define CAMAPI_H

#include <QObject>
#include <QUdpSocket>
#include <QtXml>
#include <QDateTime>
#include <QTimer>
#include "gvcp/gvcpHeaders.h"
#include "packethandler.h"

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define BIT(x) (1 << x)
#define CAMERA_GVCP_BIND_PORT (0)
#define CAMERA_GVSP_BIND_PORT (0)
#define CAMERA_API_ENABLE_RESEND (0)
#define CAMERA_API_ENABLE_DISJOINT_PARCER (0)
#define CAMERA_MAX_ACK_FETCH_RETRY_COUNT (3)
#define CAMERA_MAX_FRAME_BUFFER_SIZE (3)
#define CAMERA_WAIT_FOR_ACK_MS (25)
#define CAMERA_GVSP_PAYLOAD_SIZE (5000)

#define CAMERA_STATUS_FLAGS_INITIALIZED BIT(0)

#define CAMERA_API_STATUS_SUCCESS (0)
#define CAMERA_API_STATUS_FAILED (1)
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

class CamApi : public QObject
{
    Q_OBJECT

public:
    explicit CamApi(const QHostAddress hostIPv4Addr, QObject *parent = nullptr);

signals:
    void signalDatagramEnqueued(QNetworkDatagram datagram);

public slots:
    void slotRequestResendRoutine(quint16 blockID);
    void slotImageReceived(quint8 *dataPtr);
    void slotCameraHeartBeat();
    void slotGvspReadyRead();

public:
    /* Public functions that need to be exposed for applications. */
    quint32 ReadCameraAttribute(const QList<QString>& attributeList, QList<QByteArray>& regValues);
    quint32 WriteCameraAttribute(const QList<QString>& attributeList, const QList<QByteArray>& regValues);
    quint32 WriteRegisterValue(const QList<strGvcpCmdWriteRegHdr>& writeUnits);
    quint32 ReadRegisterValue(const QList<strGvcpCmdReadRegHdr>& addressList, QList<quint32>& regValues);
    quint32 DiscoverDevice(const QHostAddress& destAddr, QList<strGvcpAckDiscoveryHdr>& discAckHdr);
    quint32 InitializeDevice(const QHostAddress& camIP);
    quint32 StartStream();
    quint32 StopStream();

    /* Getters and setters */
    quint8 StatusFlags() const;

private:
    /* Private camera functionalities for this API. */
    quint32 ReadXmlFileFromDevice();
    quint32 RequestResend(const strGvcpCmdPktResendHdr& cmdHdr);
    quint32 FetchAck(strNonStdGvcpAckHdr& ackHdr, const quint16 reqId);
    quint32 ReadMemoryBlock(const quint32 address, const quint16 size, QByteArray& returnedData);
    quint32 XmlFetchXmlFromDevice(const QByteArray& fileName, const QByteArray& startAddress, const QByteArray size, QByteArray& xmlData);
    quint32 XmlFetchAttrElement(const QString& attributeName, QDomNode& node);
    quint32 XmlFetchChildElementValue(const QDomNode& parent, const QString& tagName, QString& value);
    quint32 XmlFetchAttrRegister(const QString& nodeName, QDomNode& regNode);

    /* Private variables. */
    const QHostAddress mHostIP;
    QHostAddress mCamIP;
    QTimer mHeartBeatTimer;
    QUdpSocket mGvcpSock;
    QUdpSocket mGvspSock;
    QDomDocument mCamXmlFile;
    CameraProperties mCamProps;
    QReadWriteLock mLock;
    QThread mStreamProducerThread;
#if (CAMERA_API_ENABLE_DISJOINT_PARCER == 1)
    QThread *mStreamConsumerThread;
#endif
    PacketHandler *mStreamConsumerObject;
    QList<quint8*> mStreamPreAllocatedBuffers;
    QHash<quint16, QHash<quint32, quint8*>> mStreamHashTable;
};

#endif // CAMAPI_H
