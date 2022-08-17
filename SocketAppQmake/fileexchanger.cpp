#include "fileexchanger.h"

FileExchanger::FileExchanger(const QHostAddress addr, const bool send, const QString prot,
                             const QString port, const QString filePath) :
                             send(send), addr(addr),filePath(filePath)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<protocol>();

    proto = static_cast<protocol>(metaEnum.keyToValue(prot.toStdString().c_str()));
    this->port = port.toInt();

    /* These will be set by the objects created at runtime. */
    this->socket = NULL;
    this->server = NULL;
}

void FileExchanger::slotReceivedPacket() {

    QFile file(filePath);

    if (proto == UDP) {

        QUdpSocket *udpSock = dynamic_cast<QUdpSocket*>(socket);
        QNetworkDatagram datagram = udpSock->receiveDatagram();

        if (datagram.isValid()) {

            qInfo() << "Received from " << datagram.senderAddress() << datagram.senderPort() <<  QThread::currentThread();

            /* Try to open the file for writing. */
            if (file.open(QFile::WriteOnly | QFile::Text)) {

                /* File opened successfully, now write to it. */
                file.write(datagram.data());

                /* Sending a signal that file is received and saved. */
                emit sigReceiveFinished();
            }
        }
    } else if (proto == TCP) {

        /* Check to see if server is valid. */
        if (server) {

            /* Get incoming connection. */
            socket = server->nextPendingConnection();

            /* Try to open the file for writing. */
            if (file.open(QFile::WriteOnly | QFile::Text)) {

                /* Wait until bytes are written. */
                while (!socket->waitForReadyRead());

                qInfo() << "Number of bytes = " << socket->bytesAvailable();

                /* File opened successfully, now write to it. */
                file.write(socket->readAll());

                /* Set the socket to null again. */
                socket = NULL;

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

    /* Attempt to oepn the file. */
    if (!error && file.open(QFile::ReadOnly | QFile::Text)) {

        /* Read all the data from file. */
        bytes = file.readAll();
    } else {

        qInfo() << "Error opening the file." <<  QThread::currentThread();
        error = true;
    }

    if (proto == UDP) {

        /* Send a signal that the process of sending file has been started. */
        emit sigSendStarted();

        socket = new QUdpSocket();
        QUdpSocket *udpSock = dynamic_cast<QUdpSocket*>(socket);

        if (!error) {

            /* Write a UDP datagram. */
            if (!(udpSock->writeDatagram(bytes, addr, port) > 0)) {

                /* Enter this when bytes written are <= 0. */
                qInfo() << "Error sending file." <<  QThread::currentThread();
                error = true;
            } else {

                /* Bytes sent successfully. */
                emit sigSendFinished();
            }
        }
    } else if (proto == TCP) {

        /* Send a signal that the process of sending file has been started. */
        emit sigSendStarted();

        socket = new QTcpSocket();
        QTcpSocket *tcpSock = dynamic_cast<QTcpSocket*>(socket);

        if (!error) {

            /* Attempt to connect to host. */
            tcpSock->connectToHost(addr, port);

            if (!tcpSock->waitForConnected(1000)) {
                qInfo() << "Error connecting to host." <<  QThread::currentThread();
                error = true;
            }
        }

        if (!error) {

            /* Write bytes to the socket. */
            if (!(tcpSock->write(bytes) > 0)) {

                /* Reach this point if the bytes written are 0. */
                qInfo() << "Error sending file." <<  QThread::currentThread();
                error = true;
            } else {

                /* Wait for the bytes to be written to the socket. */
                tcpSock->waitForBytesWritten();
                emit sigSendFinished();
            }

            tcpSock->close();
        }

    } else if (proto == FTP) {
        /* Implement FTP here. */
    }

    /* Explicitly close the file. Although it is done when the thread is killed. */
    file.close();

    /* Delete socket if its a valid pointer. */
    if (socket) {
        delete(socket);
        socket = NULL;
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

        /* Bind UDP socket to an IP and Port. */
        if (!udpSock->bind(addr, port)) {

            qInfo() << "Error binding to add/port." <<  QThread::currentThread();
            error = true;
        }

        /* Make appropriate connections. */
        if (!error) {
            connect(udpSock, &QUdpSocket::readyRead, this, &FileExchanger::slotReceivedPacket, Qt::DirectConnection);
            connect(this, &FileExchanger::sigReceiveFinished, loop.data(), &QEventLoop::quit, Qt::DirectConnection);
        }

        if (!error) {

            /* Enter loop and wait for the packet to be received. */
            loop->exec();
        }

    } else if (proto == TCP) {

        /* Sending a signal that thread has started. */
        emit sigReceiveStarted();
        server = new QTcpServer();

        /* Implement a callback for incoming connection. */
        connect(server, &QTcpServer::newConnection, this, &FileExchanger::slotReceivedPacket, Qt::DirectConnection);
        connect(this, &FileExchanger::sigReceiveFinished, loop.data(), &QEventLoop::quit, Qt::DirectConnection);

        /* Start listening to IP and port. */
        if (!server->listen(addr, port)) {
            qInfo() << "Could not start TCP server." <<  QThread::currentThread();
            error = true;
        }

        if (!error) {

            qInfo() << "Server running on." <<  QThread::currentThread();

            /* Enter loop and wait for file reception. */
            loop->exec();
        }

        if (server) {
            delete(server);
            server = NULL;
        }

    } else if (proto == FTP) {
        /* Implement FTP here. */
    }

    /* Delete socket if its a valid pointer. */
    if (socket) {
        delete(socket);
        socket = NULL;
    }

    return error;
}

void FileExchanger::run() {
    bool error = false;

    if (send) {

        /* Send file based on the protocol selected. */
        error = sendFile(proto);
    } else {

        /* Send file based on the protocol selected. */
        error = receiveFile(proto);
    }

    if (error) {

        /* Iff either send or receive reports an error. */
        qInfo() << "Error occured. " <<  QThread::currentThread();
    }
}
