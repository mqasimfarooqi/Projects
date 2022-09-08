#ifndef CAMERAAPI_H
#define CAMERAAPI_H

#include <QObject>
#include <QUdpSocket>
#include <QtXml>
#include <QDateTime>
#include <QTimer>
#include "gvcp/gvcpHeaders.h"

#define BIT(x) (1 << x)
#define MAX_ACK_FETCH_RETRY_COUNT (3)
#define CAM_STATUS_FLAGS_INITIALIZED BIT(0)

const QList<QString> lookupTags = {
    "IntReg",
    "Integer",
    "StringReg",
    "MaskedIntReg"
};

class cameraApi : public QObject
{
    Q_OBJECT
public:
    explicit cameraApi(const QHostAddress hostIP, const quint16 hostPort, const QHostAddress camIP, QObject *parent = nullptr);

signals:
    void signalGvcpPacketReceived();
    void signalGvspPacketReceived();

public slots:
    void slotGvcpReadyRead();
    void slotGvspReadyRead();
    void slotCameraHeartBeat();

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
    bool cameraFetchAck(strNonStdGvcpAckHdr& ack_hdr, const quint16 reqId);
    bool cameraReadMemoryBlock(const quint32 address, const quint16 size, QByteArray& returnedData);
    bool cameraFetchFirstUrl(QByteArray& byteArray);
    bool cameraXmlFetchXmlFromDevice(const QByteArray fileName, const QByteArray startAddress, const QByteArray size, QByteArray& xmlData);
    bool cameraXmlFetchAttrElement(const QString& attributeName, QDomNode& node);
    bool cameraXmlFetchChildElementValue(const QDomNode& parent, const QString& tagName, QString& value);

    /* Private variables. */
    const QHostAddress mHostIPAddr;
    const quint16 mGvcpHostPort;
    const QHostAddress mCamIPAddr;
    QUdpSocket mGvcpSock;
    QUdpSocket mGvspSock;
    QThread streamingThread;
    QDomDocument mCamXmlFile;
    QTimer mHeartBeatTimer;
    QVector<quint8> mVectorPendingReq;
    quint8 mCamStatusFlags;
};

#endif // CAMERAAPI_H
