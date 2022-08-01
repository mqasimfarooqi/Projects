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

class FileExchanger : public QObject, public QRunnable
{
    Q_OBJECT
public:
    FileExchanger(const QHostAddress addr, const bool send, const bool udp,
                  const quint16 port, const QString filePath);

    // QRunnable interface
public:
    void run();
    bool sendFile(bool);
    bool receiveFile(bool);

public slots:
    void readyRead();

signals:
    void sigStarted();
    void sigFinished();

private:
    const bool send;
    const bool udp;
    const QHostAddress addr;
    const quint16 port;
    const QString filePath;
};

#endif // FILEEXCHANGER_H
