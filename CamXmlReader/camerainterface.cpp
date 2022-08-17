#include "camerainterface.h"
#include <QSequentialIterable>

CameraInterface::CameraInterface(QObject *parent)
    : QObject{parent} {
}

CameraInterface::CameraInterface(QUdpSocket *socket) {
    mUdpSock = socket;
}

unsigned char reverseBits(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

/* This function expects header and data to be in big endian format. */
bool CameraInterface::camSendCmd(quint32 cmdType, QByteArray *cmdSpecificData,
                                 const QHostAddress *addr, quint16 port,
                                 quint16 reqId) {

    bool error = false;
    QByteArray datagram = { 0 };
    strGvcpCmdHdr genericHdr;

    /* Initialize memory to 0. */
    memset(&genericHdr, 0x00, sizeof(genericHdr));

    /* Check to see if the pointer is not null. */
    if (!mUdpSock) {
        error = true;
        qDebug() << "mUdpSock pointer is invalid.";
    }

    if (!error) {

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
        if(!(mUdpSock->writeDatagram(datagram, *addr, port) > 0)) {
            /* Enter this when bytes written are <= 0. */

            qDebug() << "Error sending datagram.";
            error = true;
        }
    }

    return error;
}

bool CameraInterface::camReceiveAck(QByteArray *rawSocketData, strNonStdGvcpAckHdr& ackHeader) {
    bool                    error = false;
    char*                   dataPtr = nullptr;
    QByteArray              tempArray = { 0 };
    QNetworkDatagram        datagram = { 0 };

    /* Check to see if the pointer is not null. */
    if (!mUdpSock) {
        error = true;
        qDebug() << "mUdpSock pointer is invalid.";
    }

    if (!error) {
        /* Enter only if mUdpSock is a valid pointer. */

        if (!(mUdpSock->hasPendingDatagrams())) {

            /* No datagrams are waiting to be read. */
            error = true;

        } else {

            /* Valid datagrams are waiting to be read. */
            datagram = mUdpSock->receiveDatagram();
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

    if (!error) {

        /* Just create a temp array for storing payload. */
        tempArray.append(datagram.data());

        /* A pointer pointing to the data of byte array. */
        dataPtr = tempArray.data();

        memset(&ackHeader, 0x00, sizeof(ackHeader));
        memcpy(&ackHeader.genericAckHdr, dataPtr, sizeof(ackHeader.genericAckHdr));
    }

    if ((!error) && (ackHeader.genericAckHdr.length > 0)) {

        switch (ackHeader.genericAckHdr.acknowledge) {

        case GVCP_DISCOVERY_ACK:
            ackHeader.ackHdrType = GVCP_DISCOVERY_ACK;
            ackHeader.cmdSpecificAckHdr = malloc(sizeof(strGvcpAckDiscoveryHdr));
            memset(ackHeader.cmdSpecificAckHdr, 0x00, sizeof(strGvcpAckDiscoveryHdr));
            memcpy(ackHeader.cmdSpecificAckHdr, dataPtr + sizeof(ackHeader.genericAckHdr),
                   sizeof(strGvcpAckDiscoveryHdr));
            break;

        case GVCP_READMEM_ACK:
            ackHeader.ackHdrType = GVCP_READMEM_ACK;
            ackHeader.cmdSpecificAckHdr = malloc(sizeof(strGvcpAckMemReadHdr));
            memset(ackHeader.cmdSpecificAckHdr, 0x00, sizeof(strGvcpAckMemReadHdr));
            strGvcpAckMemReadHdr *readMemAck = (strGvcpAckMemReadHdr *)ackHeader.cmdSpecificAckHdr;

            /* Copy address. */
            memcpy(&readMemAck->address, dataPtr + sizeof(ackHeader.genericAckHdr), sizeof(readMemAck->address));

            /* Copy data. */
            readMemAck->data.append(dataPtr + sizeof(ackHeader.genericAckHdr) + sizeof(readMemAck->address),
                                      ackHeader.genericAckHdr.length - sizeof(readMemAck->address));

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

