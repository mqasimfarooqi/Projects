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

#define BIT(x) (1 << x)
#define CAMERA_MAX_WORKER_THREAD_COUNT (2)
#define CAMERA_MAX_ACK_FETCH_RETRY_COUNT (3)
#define CAMERA_STATUS_FLAGS_INITIALIZED BIT(0)
#define CAMERA_MAX_FRAME_BUFFER_SIZE (3)
#define CAMERA_WAIT_FOR_ACK_MS (100)
#define CAMERA_GVSP_PAYLOAD_SIZE (8500)

#define CAMERA_API_STATUS_SUCCESS (0)
#define CAMERA_API_STATUS_FAILED (1)

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
    "Command"
};

class cameraApi : public QObject
{
    Q_OBJECT

public:
    explicit cameraApi(const QHostAddress hostIP, const QHostAddress camIP, const quint16 hostPort, QObject *parent = nullptr);

signals:
    void signalResendRequested();
    void signalDatagramEnqueued();

public slots:
    void slotCameraHeartBeat();
    void slotRequestResendRoutine();
    void slotGvspReadyRead();

public:
    /* Public functions that need to be exposed for applications. */
    quint32 cameraReadCameraAttribute(const QList<QString>& attributeList, QList<QByteArray>& regValues);
    quint32 cameraWriteCameraAttribute(const QList<QString>& attributeList, const QList<QByteArray>& regValues);
    quint32 cameraWriteRegisterValue(const QList<strGvcpCmdWriteRegHdr>& writeUnits);
    quint32 cameraReadRegisterValue(const QList<strGvcpCmdReadRegHdr> addressList, QList<quint32>& regValues);
    quint32 cameraDiscoverDevice(const QHostAddress destAddr, strGvcpAckDiscoveryHdr& discAckHdr);
    quint32 cameraStartStream(const quint16 streamHostPort);
    quint32 cameraInitializeDevice();

    /* Getters and setters */
    quint8 camStatusFlags() const;

private:
    /* Private camera functionalities for this API. */
    quint32 cameraReadXmlFileFromDevice();
    quint32 cameraRequestResend(const strGvcpCmdPktResendHdr& cmdHdr);
    quint32 cameraFetchAck(strNonStdGvcpAckHdr& ack_hdr, const quint16 reqId);
    quint32 cameraReadMemoryBlock(const quint32 address, const quint16 size, QByteArray& returnedData);
    quint32 cameraFetchFirstUrl(QByteArray& byteArray);
    quint32 cameraXmlFetchXmlFromDevice(const QByteArray fileName, const QByteArray startAddress, const QByteArray size, QByteArray& xmlData);
    quint32 cameraXmlFetchAttrElement(const QString& attributeName, QDomNode& node);
    quint32 cameraXmlFetchChildElementValue(const QDomNode& parent, const QString& tagName, QString& value);

    /* Private variables. */
    const QHostAddress mHostIPAddr;
    const QHostAddress mCamIPAddr;
    const quint16 mGvcpHostPort;
    QMutex mMutex;
    QThread mStreamingThread;
    QList<QThread> mListStreamWorkingThread;
    QTimer mHeartBeatTimer;
    QUdpSocket mGvcpSock;
    QUdpSocket mGvspSock;
    QDomDocument mCamXmlFile;
    CameraProperties mCamProps;
    QVector<quint8> mVectorPendingReq;
    QQueue<QNetworkDatagram> mStreamReceiveQueue;
    QQueue<quint16> mPktResendBlockIDQueue;
    QHash<quint16, QHash<quint32, QByteArray>> mStreamHT;
};

#endif // CAMERAAPI_H
