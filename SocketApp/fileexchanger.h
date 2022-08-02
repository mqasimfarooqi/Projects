#ifndef FILEEXCHANGER_H
#define FILEEXCHANGER_H

#include <QObject>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QRunnable>
#include <QDebug>
#include <QThread>
#include <QString>
#include <QFile>
#include <QtNetwork>
#include <QEventLoop>

class FileExchanger : public QObject, public QRunnable
{
    Q_OBJECT

public:

    enum protocol {
        UDP,
        TCP,
        FTP
    }; Q_ENUM(protocol)

    FileExchanger(const QHostAddress addr, const bool send, const QString prot,
                  const QString port, const QString filePath);

public:
    void run();
    bool sendFile(protocol proto);
    bool receiveFile(protocol proto);

signals:
    void sigReceiveStarted();
    void sigReceiveFinished();
    void sigSendStarted();
    void sigSendFinished();

public slots:
    void slotReceivedPacket();

private:
    QAbstractSocket *socket;
    protocol proto;
    int port;
    const bool send;
    const QHostAddress addr;
    const QString filePath;
};

#endif // FILEEXCHANGER_H
