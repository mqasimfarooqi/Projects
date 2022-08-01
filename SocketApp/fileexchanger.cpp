#include "fileexchanger.h"

FileExchanger::FileExchanger(const QHostAddress addr, const bool send, const bool udp,
                             const quint16 port, const QString filePath) :
                             addr(addr), port(port), send(send), udp(udp),filePath(filePath)
{}

void FileExchanger::readyRead() {
    qInfo() << "Packet received." <<  QThread::currentThread();
}

bool FileExchanger::sendFile(bool udp) {
    QUdpSocket *udpSock;
    QTcpSocket *tcpSock;
    QFile file(filePath);
    QByteArray bytes;
    bool error = false;

    if (udp) {
        udpSock = new QUdpSocket();
//        udpSock->deleteLater();

        if (!error && file.open(QFile::ReadOnly | QFile::Text)) {
            bytes = file.readAll();
        } else {
            qInfo() << "Error opening the file." <<  QThread::currentThread();
            error = true;
        }

        connect(udpSock, &QUdpSocket::bytesWritten, this, &FileExchanger::readyRead, Qt::QueuedConnection);

        if (!error) {
            if (!(udpSock->writeDatagram(bytes, addr, port) > 0)) {
                qInfo() << "Error sending file." <<  QThread::currentThread();
            }
        }
    }
    else {
        tcpSock = new QTcpSocket();
        tcpSock->deleteLater();
    }

    file.close();

    return error;
}

bool FileExchanger::receiveFile(bool udp) {
    QUdpSocket *udpSock;
    QTcpSocket *tcpSock;
    QFile file(filePath);
    bool error = false;

    if (udp) {
        udpSock = new QUdpSocket();
        //udpSock->deleteLater();

        if (!udpSock->bind(addr, port)) {
            qInfo() << "Error binding to add/port." <<  QThread::currentThread();
            error = true;
        }

        if (!error) {
            connect(udpSock, &QUdpSocket::readyRead, this, &FileExchanger::readyRead, Qt::QueuedConnection);
        }

    } else {
        tcpSock = new QTcpSocket();
        tcpSock->deleteLater();
    }

    while (1) {
        QThread::currentThread()->sleep(10);
    }


    return error;
}

void FileExchanger::run() {
    bool error = false;

    /* Sending a signal that thread has started. */
    emit sigStarted();

    if (send) {
        qInfo() << "Sending a file on: " <<  QThread::currentThread();
        error = sendFile(true);
    } else {
        qInfo() << "Receiving a file on: " <<  QThread::currentThread();
        error = receiveFile(true);
    }

    if (error) {
        qInfo() << "Error occured. " <<  QThread::currentThread();
    }

    /* Sending a signal that thread is finished. */
    emit sigFinished();
}
