#ifndef CAMERAAPI_H
#define CAMERAAPI_H

#include <QObject>
#include <QUdpSocket>
#include <QtXml>
#include <QDateTime>
#include "gvcp/gvcpHeaders.h"

#define CAMERA_API_COMMAND_READXML (1)

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
    explicit cameraApi(QObject *parent = nullptr);
    cameraApi(QUdpSocket *udp_socket, QVector<quint8> *pendingReqVector);

signals:
    void signalAckReceived();

public slots:
    void slotReadyRead();

public:
    /* Public functions that need to be exposed for applications. */
    bool cameraReadXmlFileFromDevice(QDomDocument& xmlFile, const QHostAddress& destAddr);
    bool cameraReadCameraAttribute(const QList<QString>& attributeList, const QDomDocument& xmlFile, const QHostAddress destAddr, QList<QByteArray>& regValues);
    bool cameraWriteRegisterValue(const QHostAddress &destAddr, const QList<strGvcpCmdWriteRegHdr>& writeUnits);
    bool cameraReadRegisterValue(const QHostAddress& destAddr, const QList<strGvcpCmdReadRegHdr> addressList, QList<quint32>& regValues);
    bool cameraDiscoverDevice(const QHostAddress& destAddr, strGvcpAckDiscoveryHdr& discHdr);

private:
    /* Private camera functionalities for this API. */
    bool cameraFetchAck(strNonStdGvcpAckHdr& ack_hdr, const quint16 reqId);
    bool cameraReadMemoryBlock(const quint32 address, const quint16 size, const QHostAddress destAddr, QByteArray& returnedData);
    bool cameraFetchXmlFromDevice(const QByteArray fileName, const QByteArray startAddress, const QByteArray size, const QHostAddress destAddr, QByteArray& xmlData);
    bool cameraFetchFirstUrl(const QHostAddress& destAddr, QByteArray& byteArray);
    bool cameraXmlFetchAttrElement(const QString& attributeName, const QMap<QString, QDomNodeList>& camFeatures, QDomNode& node);

    /* Private variables. */
    QUdpSocket *mUdpSock;
    QVector<quint8> *mVectorPendingReq;
};

#endif // CAMERAAPI_H
