#include "fileexchanger.h"

FileExchanger::FileExchanger(const QHostAddress addr, const bool send, const protocol proto,
                             const quint16 port, const QString filePath) :
                             addr(addr), port(port), send(send), proto(proto),filePath(filePath)
{}

void FileExchanger::receivedPacket() {

    QFile file(filePath);
    bool error = false;

    if (proto == UDP) {
        QUdpSocket *udpSock = dynamic_cast<QUdpSocket*>(socket);
        QNetworkDatagram datagram = udpSock->receiveDatagram();
        if (datagram.isValid()) {
            qInfo() << "Received from " << datagram.senderAddress() << datagram.senderPort() <<  QThread::currentThread();
            if (file.open(QFile::WriteOnly | QFile::Text)) {
                file.write(datagram.data());

                /* Sending a signal that thread is finished. */
                emit receiveFinished();
            }
        }
    }

    file.close();
}

bool FileExchanger::sendFile(protocol proto) {
    QFile file(filePath);
    QByteArray bytes;
    bool error = false;

    emit sendStarted();

    if (!error && file.open(QFile::ReadOnly | QFile::Text)) {
        bytes = file.readAll();
    } else {
        qInfo() << "Error opening the file." <<  QThread::currentThread();
        error = true;
    }

    if (proto == UDP) {
        socket = new QUdpSocket();
        QUdpSocket *udpSock = dynamic_cast<QUdpSocket*>(socket);

        if (!error) {
            if (!(udpSock->writeDatagram(bytes, addr, port) > 0)) {
                qInfo() << "Error sending file." <<  QThread::currentThread();
                error = true;
            } else {
                emit sendFinished();
            }
        }
    } else if (proto == TCP) {
        socket = new QTcpSocket();
        QTcpSocket *tcpSock = dynamic_cast<QTcpSocket*>(socket);
    }

    file.close();

    if (socket) {
        delete(socket);
    }

    return error;
}

bool FileExchanger::receiveFile(protocol proto) {
    bool error = false;

    /* Sending a signal that thread has started. */
    emit receiveStarted();

    if (proto == UDP) {
        socket = new QUdpSocket();
        QUdpSocket *udpSock = dynamic_cast<QUdpSocket*>(socket);

        if (!udpSock->bind(addr, port)) {
            qInfo() << "Error binding to add/port." <<  QThread::currentThread();
            error = true;
        }

        if (!error) {
            connect(udpSock, &QUdpSocket::readyRead, this, &FileExchanger::receivedPacket, Qt::DirectConnection);
        }

        if (!error) {
            while(!udpSock->hasPendingDatagrams()) {
                /* Do nothing while waiting for a datagram. */
            }

            emit udpSock->readyRead();
        }

    } else if (proto == TCP) {
        socket = new QTcpSocket();
        QTcpSocket *tcpSock = dynamic_cast<QTcpSocket*>(socket);
    }

    if (socket) {
        delete(socket);
    }

    return error;
}

void FileExchanger::run() {
    bool error = false;

    if (send) {
        qInfo() << "Sending a file on: " <<  QThread::currentThread();
        error = sendFile(UDP);
    } else {
        qInfo() << "Receiving a file on: " <<  QThread::currentThread();
        error = receiveFile(UDP);
    }

    if (error) {
        qInfo() << "Error occured. " <<  QThread::currentThread();
    }
}
