#ifndef CAMERAAPI_H
#define CAMERAAPI_H

#include <QObject>
#include <QUdpSocket>
#include <QEventLoop>
#include <QMutex>
#include <QString>
#include <QFile>
#include <QtXml>
#include <QThreadPool>
#include "gvcpHeaders.h"

#define CAMERA_API_COMMAND_READXML (1)

extern void camFreeAckMemory(strNonStdGvcpAckHdr& ackHeader);

class cameraApi : public QObject
{
    Q_OBJECT
public:
    explicit cameraApi(QObject *parent = nullptr);
    cameraApi(QUdpSocket *udp_socket, QString addr, quint8 command);

signals:
    void signalAckReceived();

public slots:
    void slotReadyRead();

public:
    /* Public functions that need to be exposed for applications. */
    bool cameraXmlReadXmlFileFromDevice(QDomDocument& xmlFile, const QHostAddress& destAddr);
    bool cameraXmlReadCameraAttribute(const QList<QString>& featureList, const QDomDocument& xmlFile, const QHostAddress destAddr, QByteArray& regValues);
    bool cameraWriteCameraAttribute(const QList<QString>& featureList, const QDomDocument& xmlFile, const QHostAddress destAddr, QByteArray& regValues);
    bool cameraWriteRegisterValue(const QHostAddress &destAddr, const strGvcpCmdWriteRegHdr writeUnits[], const quint32 regsArraySize);
    bool cameraDiscoverDevice(const QHostAddress& destAddr, strNonStdGvcpAckHdr& discHdr);

private:
    /* Private camera functionalities for this API. */
    bool cameraFetchAck(strNonStdGvcpAckHdr& ack_hdr, const quint16 reqId);
    bool cameraReadMemoryBlock(const quint32 address, const quint16 size, const QHostAddress destAddr, QByteArray& returnedData);
    bool cameraFetchXmlFromDevice(const QByteArray fileName, const QByteArray startAddress, const QByteArray size, const QHostAddress destAddr, QByteArray& xmlData);
    bool cameraFetchFirstUrl(const QHostAddress& destAddr, QByteArray& byteArray);
    bool cameraReadRegisterValue(const QHostAddress& destAddr, const QList<quint32> addressList, QByteArray& values);

    /* Private variables. */
    QUdpSocket *mUdpSock;
    QVector<quint8> mVectorPendingReq;
    QString mAddr;
    quint8 mCommand;
};

#endif // CAMERAAPI_H
