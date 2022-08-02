#include "fileexchanger.h"

FileExchanger::FileExchanger(const QHostAddress addr, const bool send, const QString prot,
                             const QString port, const QString filePath) :
                             addr(addr), send(send),filePath(filePath)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<protocol>();

    proto = static_cast<protocol>(metaEnum.keyToValue(prot.toStdString().c_str()));
    this->port = port.toInt();
    this->socket = 0;
}

void FileExchanger::slotReceivedPacket() {

    QFile file(filePath);
    bool error = false;

    if (proto == UDP) {
        QUdpSocket *udpSock = dynamic_cast<QUdpSocket*>(socket);
        QNetworkDatagram datagram = udpSock->receiveDatagram();
        if (datagram.isValid()) {
            qInfo() << "Received from " << datagram.senderAddress() << datagram.senderPort() <<  QThread::currentThread();
            if (file.open(QFile::WriteOnly | QFile::Text)) {
                file.write(datagram.data());

                /* Sending a signal that file is received and saved. */
                emit sigReceiveFinished();
            }
        }
    }

    file.close();
}

bool FileExchanger::sendFile(protocol proto) {
    QFile file(filePath);
    QByteArray bytes;
    bool error = false;

    if (!error && file.open(QFile::ReadOnly | QFile::Text)) {
        bytes = file.readAll();
    } else {
        qInfo() << "Error opening the file." <<  QThread::currentThread();
        error = true;
    }

    if (proto == UDP) {
        emit sigSendStarted();
        socket = new QUdpSocket();
        QUdpSocket *udpSock = dynamic_cast<QUdpSocket*>(socket);

        if (!error) {
            if (!(udpSock->writeDatagram(bytes, addr, port) > 0)) {
                qInfo() << "Error sending file." <<  QThread::currentThread();
                error = true;
            } else {
                emit sigSendFinished();
            }
        }
    } else if (proto == TCP) {

    } else if (proto == FTP) {

    }

    file.close();

    if (socket) {
        delete(socket);
    }

    return error;
}

bool FileExchanger::receiveFile(protocol proto) {
    bool error = false;
    QScopedPointer<QEventLoop> loop(new QEventLoop);

    if (proto == UDP) {
        /* Sending a signal that thread has started. */
        emit sigReceiveStarted();

        socket = new QUdpSocket();
        QUdpSocket *udpSock = dynamic_cast<QUdpSocket*>(socket);

        if (!udpSock->bind(addr, port)) {
            qInfo() << "Error binding to add/port." <<  QThread::currentThread();
            error = true;
        }

        if (!error) {
            connect(udpSock, &QUdpSocket::readyRead, this, &FileExchanger::slotReceivedPacket, Qt::DirectConnection);
            connect(this, &FileExchanger::sigReceiveFinished, loop.data(), &QEventLoop::quit, Qt::DirectConnection);
        }

        if (!error) {

            /* Enter loop and wait for the packet to be received. */
            loop->exec();
        }

    } else if (proto == TCP) {

    } else if (proto == FTP) {

    }

    if (socket)
        delete(socket);

    return error;
}

void FileExchanger::run() {
    bool error = false;

    if (send) {
        error = sendFile(proto);
    } else {
        error = receiveFile(proto);
    }

    if (error) {
        qInfo() << "Error occured. " <<  QThread::currentThread();
    }
}
