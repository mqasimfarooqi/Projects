#ifndef CAMERAAPI_H
#define CAMERAAPI_H

#include <QObject>
#include <QUdpSocket>
#include <QtXml>
#include <QDateTime>
#include <QTimer>
#include "gvcp/gvcpHeaders.h"
#include "gvsp/gvsp.h"

#define BIT(x) (1 << x)
#define CAMERA_MAX_ACK_FETCH_RETRY_COUNT (3)
#define CAMERA_STATUS_FLAGS_INITIALIZED BIT(0)
#define CAMERA_MAX_FRAME_BUFFER_SIZE (10)
#define CAMERA_WAIT_FOR_ACK_MS (100)
#define CAMERA_GVSP_PAYLOAD_SIZE (7000)

/* SwissKnife is not supported. */
const QList<QString> lookupTags = {
    "IntReg",
    "Integer",
    "StringReg",
    "MaskedIntReg",
    "StructReg"
};

/* General camera properties structure. */
typedef struct {
    quint8 statusFlags;
    quint16 streamPktSize;

} cameraProperties;

class cameraApi : public QObject
{
    Q_OBJECT

public:
    explicit cameraApi(const QHostAddress hostIP, const QHostAddress camIP, const quint16 hostPort, QObject *parent = nullptr);

signals:
    void signalResendRequested();

public slots:
    void slotGvspReadyRead();
    void slotCameraHeartBeat();
    void slotRequestResendRoutine();

public:
    /* Public functions that need to be exposed for applications. */
    bool cameraReadCameraAttribute(const QList<QString>& attributeList, QList<QByteArray>& regValues);
    bool cameraWriteCameraAttribute(const QList<QString>& attributeList, const QList<QByteArray>& regValues);
    bool cameraWriteRegisterValue(const QList<strGvcpCmdWriteRegHdr>& writeUnits);
    bool cameraReadRegisterValue(const QList<strGvcpCmdReadRegHdr> addressList, QList<quint32>& regValues);
    bool cameraDiscoverDevice(const QHostAddress destAddr, strGvcpAckDiscoveryHdr& discAckHdr);
    bool cameraStartStream(const quint16 streamHostPort);
    bool cameraInitializeDevice();

    /* Getters and setters */
    quint8 camStatusFlags() const;

private:
    /* Private camera functionalities for this API. */
    bool cameraReadXmlFileFromDevice();
    bool cameraRequestResend(const strGvcpCmdPktResendHdr& cmdHdr);
    bool cameraAppendResendQueue(const strGvcpCmdPktResendHdr& cmdHdr);
    bool cameraFetchAck(strNonStdGvcpAckHdr& ack_hdr, const quint16 reqId);
    bool cameraReadMemoryBlock(const quint32 address, const quint16 size, QByteArray& returnedData);
    bool cameraFetchFirstUrl(QByteArray& byteArray);
    bool cameraXmlFetchXmlFromDevice(const QByteArray fileName, const QByteArray startAddress, const QByteArray size, QByteArray& xmlData);
    bool cameraXmlFetchAttrElement(const QString& attributeName, QDomNode& node);
    bool cameraXmlFetchChildElementValue(const QDomNode& parent, const QString& tagName, QString& value);

    /* Private variables. */
    const QHostAddress mHostIPAddr;
    const QHostAddress mCamIPAddr;
    const quint16 mGvcpHostPort;
    QThread mStreamingThread;
    QTimer mHeartBeatTimer;
    QUdpSocket mGvcpSock;
    QUdpSocket mGvspSock;
    QDomDocument mCamXmlFile;
    cameraProperties mCamProps;
    QVector<quint8> mVectorPendingReq;
    QQueue<quint16> mPktResendBlockIDQueue;
    QHash<quint16, QHash<quint32, QByteArray>> mStreamHT;
};

#endif // CAMERAAPI_H
