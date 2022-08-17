#ifndef CAMERA_API_H
#define CAMERA_API_H

#include <QObject>
#include <QRunnable>
#include <QUdpSocket>
#include <QEventLoop>
#include <QMutex>
#include <QString>
#include <QFile>
#include <QtXml>
#include <QThreadPool>
#include "camerainterface.h"
#include "quazip/JlCompress.h"

#define CAMERA_API_COMMAND_READXML (1)

class camera_api : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit camera_api(QObject *parent = nullptr);
    camera_api(QUdpSocket *udp_socket, QString addr, quint8 command);

signals:
    void signalAckReceived();

public slots:
    void slotReadyRead();

public:
    void run();
    quint32 byteArrayToUint32(const QByteArray &bytes);

private:
    /* Private camera functionalities. */
    bool cameraXmlReadXmlFileFromDevice(QDomDocument& xmlFile);
    bool cameraXmlReadCameraAttribute(const QString feature, const QDomDocument& xmlFile);
    bool cameraFetchAck(strNonStdGvcpAckHdr& ack_hdr, const quint16 reqId);
    bool cameraReadMemoryBlock(const quint32 address, const quint16 size, const QHostAddress addr, QByteArray& returnedData);
    bool cameraFetchFirstUrl(QHostAddress addr, QByteArray& byteArray);
    bool cameraFetchXmlFromDevice(QByteArray fileName, QByteArray startAddress, QByteArray size, QHostAddress addr, QByteArray& xmlData);

    /* Private variables. */
    /* This class will thread send/receive */
    CameraInterface *mCamera;
    QUdpSocket *mUdpSock;
    QVector<quint16> mVectorPendingReq;
    QString mAddr;
    quint8 mCommand;
};

#endif // CAMERA_API_H
