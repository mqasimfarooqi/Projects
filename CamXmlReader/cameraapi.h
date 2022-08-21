#ifndef CAMERAAPI_H
#define CAMERAAPI_H

#include <QObject>
#include <QRunnable>
#include <QUdpSocket>
#include <QEventLoop>
#include <QMutex>
#include <QString>
#include <QFile>
#include <QtXml>
#include <QThreadPool>
#include "gvcpHeaders.h"

#define CAMERA_API_COMMAND_READXML (1)

class cameraApi : public QObject, public QRunnable
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
    void run();
    bool cameraXmlReadXmlFileFromDevice(QDomDocument& xmlFile, const QHostAddress& destAddr);
    bool cameraXmlReadCameraAttribute(const QString feature, const QDomDocument& xmlFile);

private:
    /* Private camera functionalities for this API. */
    bool cameraFetchAck(strNonStdGvcpAckHdr& ack_hdr, const quint16 reqId);
    bool cameraReadMemoryBlock(const quint32 address, const quint16 size, const QHostAddress destAddr, QByteArray& returnedData);
    bool cameraFetchXmlFromDevice(QByteArray fileName, QByteArray startAddress, QByteArray size, QHostAddress destAddr, QByteArray& xmlData);
    bool cameraFetchFirstUrl(const QHostAddress& destAddr, QByteArray& byteArray);
    bool cameraReadRegisterValue(const QHostAddress& destAddr, const quint32_be regs[], quint32 regsArraySize, QByteArray& values);
    bool cameraDiscoverDevice(const QHostAddress& destAddr);

    /* Private variables. */
    /* This class will thread send/receive */
    QUdpSocket *mUdpSock;
    QDomDocument mXmlDoc;
    QVector<quint16> mVectorPendingReq;
    QString mAddr;
    quint8 mCommand;
};

#endif // CAMERAAPI_H
