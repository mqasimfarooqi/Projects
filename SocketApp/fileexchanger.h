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

class FileExchanger : public QObject, public QRunnable
{
    Q_OBJECT
public:

    enum protocol {
        TCP,
        UDP
    };

    FileExchanger(const QHostAddress addr, const bool send, const protocol proto,
                  const quint16 port, const QString filePath);

    // QRunnable interface
public:
    void run();
    bool sendFile(protocol proto);
    bool receiveFile(protocol proto);

public slots:
    void receivedPacket();

signals:
    void sigStarted();
    void sigFinished();

private:
    QAbstractSocket *socket;
    const bool send;
    const protocol proto;
    const QHostAddress addr;
    const quint16 port;
    const QString filePath;
};

#endif // FILEEXCHANGER_H
