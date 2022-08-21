#include "camerainterface.h"
#include <QSequentialIterable>

CameraInterface::CameraInterface(QObject *parent)
    : QObject{parent} {
}

unsigned char reverseBits(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

/* This function expects header and data to be in big endian format. */
bool CameraInterface::camSendCmd(QUdpSocket *udpSock, const quint32 cmdType,
                                 const QByteArray *cmdSpecificData, const QHostAddress *destAddr,
                                 const quint16 port, const quint16 reqId) {

    bool error = false;
    QByteArray datagram = { 0 };
    strGvcpCmdHdr genericHdr;

    /* Check to see if the pointer is not null. */
    if (!udpSock->isValid()) {
        error = true;
        qDebug() << "mUdpSock pointer is invalid.";
    }

    if (!error) {

        /* Initialize memory to 0. */
        memset(&genericHdr, 0x00, sizeof(genericHdr));

        /* Construct generic header for a specific command type. */
        switch (cmdType) {

        case GVCP_DISCOVERY_CMD:
            /* Construct generic header for command [GVCP_DISCOVERY_CMD]. */
            genericHdr.keyCode = GVCP_CMD_HARDCODED_KEYCODE;
            genericHdr.command = cmdType;
            genericHdr.flag = reverseBits(GVCP_CMD_FLAG_ACKNOWLEDGE | GVCP_CMD_FLAG_DISCOVERY_BROADCAST_ACK);
            if (cmdSpecificData != nullptr)
                genericHdr.length = cmdSpecificData->length();
            genericHdr.reqId = reqId;

            /* Append generic header to the byte array. */
            datagram.append((char *)&genericHdr, sizeof(genericHdr));

            break;

        case GVCP_READMEM_CMD:
            /* Construct generic header for command [GVCP_READMEM_CMD]. */
            genericHdr.keyCode = GVCP_CMD_HARDCODED_KEYCODE;
            genericHdr.command = cmdType;
            genericHdr.flag = reverseBits(GVCP_CMD_FLAG_ACKNOWLEDGE);
            if (cmdSpecificData != nullptr)
                genericHdr.length = cmdSpecificData->length();
            genericHdr.reqId = reqId;

            /* Append generic header to the byte array. */
            datagram.append((char *)&genericHdr, sizeof(genericHdr));

            break;

        case GVCP_WRITEREG_CMD:
            /* Construct generic header for command [GVCP_WRITEREG_CMD]. */
            genericHdr.keyCode = GVCP_CMD_HARDCODED_KEYCODE;
            genericHdr.command = cmdType;
            genericHdr.flag = reverseBits(GVCP_CMD_FLAG_ACKNOWLEDGE);
            if (cmdSpecificData != nullptr)
                genericHdr.length = cmdSpecificData->length();
            genericHdr.reqId = reqId;

            /* Append generic header to the byte array. */
            datagram.append((char *)&genericHdr, sizeof(genericHdr));

            break;

        case GVCP_READREG_CMD:
            /* Construct generic header for command [GVCP_READREG_CMD]. */
            genericHdr.keyCode = GVCP_CMD_HARDCODED_KEYCODE;
            genericHdr.command = cmdType;
            genericHdr.flag = reverseBits(GVCP_CMD_FLAG_ACKNOWLEDGE);
            if (cmdSpecificData != nullptr)
                genericHdr.length = cmdSpecificData->length();
            genericHdr.reqId = reqId;

            /* Append generic header to the byte array. */
            datagram.append((char *)&genericHdr, sizeof(genericHdr));

            break;
        }

        /* Guarding checks to see if we have valid data in the buffer. */
        if ((cmdSpecificData != nullptr) && !error) {

            /* Append only if data length is greater than 0. */
            if (cmdSpecificData->length() > 0) {

                /* Append command specific data to the byte array. */
                datagram.append(*cmdSpecificData);
            }
        }

        /* Write datagram to the udp socket. */
        if(!(udpSock->writeDatagram(datagram, *destAddr, port) > 0)) {
            /* Enter this when bytes written are <= 0. */

            qDebug() << "Error sending datagram.";
            error = true;
        }
    }

    return error;
}

bool CameraInterface::camReceiveAck(QUdpSocket *udpSock, strNonStdGvcpAckHdr& ackHeader) {
    bool error = false;
    QByteArray tempArray;
    QNetworkDatagram datagram = { 0 };
    strGvcpAckMemReadHdr *readMemAck;
    strGvcpAckRegReadHdr *readRegAck = nullptr;

    /* Check to see if the pointer is not null. */
    if (!udpSock->isValid()) {
        error = true;
        qDebug() << "mUdpSock pointer is invalid.";
    }

    if (!error) {
        /* Enter only if mUdpSock is a valid pointer. */

        if (!(udpSock->hasPendingDatagrams())) {

            /* No datagrams are waiting to be read. */
            error = true;

        } else {

            /* Valid datagrams are waiting to be read. */
            datagram = udpSock->receiveDatagram();
        }
    }

    if (!(datagram.isValid()) && !error) {

        /* Set error to true because datagram is invalid. */
        error = true;
    }

    if (!error && datagram.data().length()) {

        /* A pointer pointing to the data of byte array. */
        tempArray.append(datagram.data());

        memset(&ackHeader, 0x00, sizeof(ackHeader));
        memcpy(&ackHeader.genericAckHdr, tempArray.data(), sizeof(ackHeader.genericAckHdr));
    }

    if (!error && (ackHeader.genericAckHdr.length > 0) && tempArray.length()) {

        switch (ackHeader.genericAckHdr.acknowledge) {

        case GVCP_DISCOVERY_ACK:
            ackHeader.ackHdrType = GVCP_DISCOVERY_ACK;
            ackHeader.cmdSpecificAckHdr = new strGvcpAckDiscoveryHdr();
            memset(ackHeader.cmdSpecificAckHdr, 0x00, sizeof(strGvcpAckDiscoveryHdr));
            memcpy(ackHeader.cmdSpecificAckHdr, tempArray.data() + sizeof(ackHeader.genericAckHdr),
                   sizeof(strGvcpAckDiscoveryHdr));
            break;

        case GVCP_READMEM_ACK:
            ackHeader.ackHdrType = GVCP_READMEM_ACK;
            ackHeader.cmdSpecificAckHdr = new strGvcpAckMemReadHdr();
            memset(ackHeader.cmdSpecificAckHdr, 0x00, sizeof(strGvcpAckMemReadHdr));
            readMemAck = (strGvcpAckMemReadHdr *)ackHeader.cmdSpecificAckHdr;

            /* Copy address. */
            memcpy(&readMemAck->address, tempArray.data() + sizeof(ackHeader.genericAckHdr), sizeof(readMemAck->address));

            /* Copy data. */
            readMemAck->data.append(tempArray.data() + sizeof(ackHeader.genericAckHdr) + sizeof(readMemAck->address),
                                      ackHeader.genericAckHdr.length - sizeof(readMemAck->address));

            break;

        case GVCP_READREG_ACK:
            ackHeader.ackHdrType = GVCP_READREG_ACK;
            ackHeader.cmdSpecificAckHdr = new strGvcpAckRegReadHdr();
            readRegAck = (strGvcpAckRegReadHdr *)ackHeader.cmdSpecificAckHdr;
            readRegAck->registerData.clear();

            /* Copy data. */
            readRegAck->registerData.append(tempArray.data() + sizeof(ackHeader.genericAckHdr));

            break;

        } /* Switch case end. */
    }

    if (error) {

        /* Was memory allocated for a command specific header? */
        if (ackHeader.cmdSpecificAckHdr) {

            /* Free the allocated memory. */
            free(ackHeader.cmdSpecificAckHdr);
        }
    }

    return (error);
}

bool CameraInterface::camReceiveAck(QUdpSocket *udpSock, QByteArray *rawSocketData)
{
    bool                    error = false;
    QNetworkDatagram        datagram = { 0 };

    /* Check to see if the pointer is not null. */
    if (!udpSock->isValid()) {
        error = true;
        qDebug() << "mUdpSock pointer is invalid.";
    }

    if (!error) {
        /* Enter only if mUdpSock is a valid pointer. */

        if (!(udpSock->hasPendingDatagrams())) {

            /* No datagrams are waiting to be read. */
            error = true;

        } else {

            /* Valid datagrams are waiting to be read. */
            datagram = udpSock->receiveDatagram();
        }
    }

    if (!(datagram.isValid())) {

        /* Set error to true because datagram is invalid. */
        error = true;
    }

    if (!error && rawSocketData) {
        /* Enter here only if the received datagram is valid. */

        rawSocketData->append(datagram.data());
    }

    return error;
}

