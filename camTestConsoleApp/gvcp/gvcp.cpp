#include "gvcp.h"
#include "caminterface.h"

unsigned char gvcpReverseBits(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

void gvcpFreeAckMemory(strNonStdGvcpAckHdr& ackHeader) {

    if (ackHeader.cmdSpecificAckHdr != nullptr) {
        switch (ackHeader.genericAckHdr.acknowledge) {
        case GVCP_DISCOVERY_ACK:
            delete((strGvcpAckDiscoveryHdr *)ackHeader.cmdSpecificAckHdr);
            break;
        case GVCP_READMEM_ACK:
            delete((strGvcpAckMemReadHdr *)ackHeader.cmdSpecificAckHdr);
            break;
        case GVCP_READREG_ACK:
            delete((strGvcpAckRegReadHdr *)ackHeader.cmdSpecificAckHdr);
            break;
        case GVCP_WRITEREG_ACK:
            delete((strGvcpAckRegWriteHdr *)ackHeader.cmdSpecificAckHdr);
            break;
        }
        ackHeader.cmdSpecificAckHdr = nullptr;
    }
}

void gvcpMakeCommandSpecificAckHeader(strNonStdGvcpAckHdr& ackHeader, QByteArray& dataArray) {

    char *dataPtr = dataArray.data() + sizeof(strGvcpAckHdr);

    if (ackHeader.genericAckHdr.acknowledge == GVCP_DISCOVERY_ACK) {

        /* Allocate memory for the discovery ack header structure. */
        ackHeader.ackHdrType = GVCP_DISCOVERY_ACK;
        ackHeader.cmdSpecificAckHdr = new strGvcpAckDiscoveryHdr();
        strGvcpAckDiscoveryHdr *discPtr = (strGvcpAckDiscoveryHdr *)ackHeader.cmdSpecificAckHdr;
        memset(discPtr, 0x00, sizeof(*discPtr));

        /* Copy members of discovery ack in the discovery structure. */
        discPtr->specVersionMajor = qFromBigEndian(*(quint16 *)(dataPtr + strGvcpAckDiscoveryHdrSPECVERSIONMAJOR));
        discPtr->specVersionMinor = qFromBigEndian(*(quint16 *)(dataPtr + strGvcpAckDiscoveryHdrSPECVERSIONMINOR));
        discPtr->deviceMode = qFromBigEndian(*(quint32 *)(dataPtr + strGvcpAckDiscoveryHdrDEVICEMODE));
        discPtr->reserved0 = qFromBigEndian(*(quint16 *)(dataPtr + strGvcpAckDiscoveryHdrRESERVED0));
        discPtr->deviceMacAddrHigh = qFromBigEndian(*(quint16 *)(dataPtr + strGvcpAckDiscoveryHdrDEVICEMACADDRHIGH));
        discPtr->deviceMacAddrLow = qFromBigEndian(*(quint32 *)(dataPtr + strGvcpAckDiscoveryHdrDEVICEMACADDRLOW));
        discPtr->ipConfigOptions = qFromBigEndian(*(quint32 *)(dataPtr + strGvcpAckDiscoveryHdrIPCONFIGOPTIONS));
        discPtr->ipConfigCurrent = qFromBigEndian(*(quint32 *)(dataPtr + strGvcpAckDiscoveryHdrIPCONFIGCURRENT));
        for (quint8 counter = 0; counter < 4; counter++) {
            discPtr->reserved1[counter] = *(dataPtr + counter + strGvcpAckDiscoveryHdrRESERVED1);
            discPtr->reserved2[counter] = *(dataPtr + counter + strGvcpAckDiscoveryHdrRESERVED2);
            discPtr->reserved3[counter] = *(dataPtr + counter + strGvcpAckDiscoveryHdrRESERVED3);
        }
        discPtr->currentIp = qFromBigEndian(*(quint32 *)(dataPtr + strGvcpAckDiscoveryHdrCURRENTIP));
        discPtr->currentSubnetMask = qFromBigEndian(*(quint32 *)(dataPtr + strGvcpAckDiscoveryHdrCURRENTSUBNETMASK));
        discPtr->defaultGateway = qFromBigEndian(*(quint32 *)(dataPtr + strGvcpAckDiscoveryHdrDEFAULTGATEWAY));
        for (quint8 counter = 0; counter < 32; counter++) {
            discPtr->manufacturerName[counter] = *(dataPtr + counter + strGvcpAckDiscoveryHdrMANUFACTURERNAME);
            discPtr->modelName[counter] = *(dataPtr + counter + strGvcpAckDiscoveryHdrMODELNAME);
            discPtr->deviceVersion[counter] = *(dataPtr + counter + strGvcpAckDiscoveryHdrDEVICEVERSION);
        }
        for (quint8 counter = 0; counter < 48; counter++) {
            discPtr->manufacturerSpecificInfo[counter] = *(dataPtr + counter + strGvcpAckDiscoveryHdrMANUFACTURERSPECIFICINFO);
        }
        for (quint8 counter = 0; counter < 16; counter++) {
            discPtr->serialNumber[counter] = *(dataPtr + counter + strGvcpAckDiscoveryHdrSERIALNUMBER);
            discPtr->userDefinedName[counter] = *(dataPtr + counter + strGvcpAckDiscoveryHdrUSERDEFINEDNAME);
        }

    } else if (ackHeader.genericAckHdr.acknowledge == GVCP_READMEM_ACK) {

        /* Allocate memory for read memory ack header. */
        ackHeader.ackHdrType = GVCP_READMEM_ACK;
        ackHeader.cmdSpecificAckHdr = new strGvcpAckMemReadHdr();
        strGvcpAckMemReadHdr *readMemAckPtr = (strGvcpAckMemReadHdr *)ackHeader.cmdSpecificAckHdr;
        memset(&readMemAckPtr->address, 0x00, sizeof(readMemAckPtr->address));
        readMemAckPtr->data.clear();

        /* Copy data into the read memory ack header. */
        readMemAckPtr->address = qFromBigEndian(*(quint32 *)(dataPtr + strGvcpAckMemReadHdrADDRESS));
        readMemAckPtr->data.append(dataPtr + strGvcpAckMemReadHdrDATA,
                                   ackHeader.genericAckHdr.length - strGvcpAckMemReadHdrDATA);

    } else if (ackHeader.genericAckHdr.acknowledge == GVCP_READREG_ACK) {

        /* Allocate memory for read register ack header. */
        ackHeader.ackHdrType = GVCP_READREG_ACK;
        ackHeader.cmdSpecificAckHdr = new strGvcpAckRegReadHdr();
        strGvcpAckRegReadHdr *readRegAckPtr = (strGvcpAckRegReadHdr *)ackHeader.cmdSpecificAckHdr;
        readRegAckPtr->registerData = new quint32(ackHeader.genericAckHdr.length/sizeof(quint32));

        /* Copy contents from datagram into the read reg ack header. */
        quint32 tempVal = 0;
        for (quint32 counter = 0; counter < (ackHeader.genericAckHdr.length) / sizeof(quint32); counter++) {
            tempVal = qFromBigEndian(*(quint32 *)((quint32 *)dataPtr + strGvcpAckRegReadHdrREGISTERDATA + counter));
            readRegAckPtr->registerData[counter] = tempVal;
        }
    } else if (ackHeader.genericAckHdr.acknowledge == GVCP_WRITEREG_ACK) {

        /* Allocate memory for write reg ack header. */
        ackHeader.ackHdrType = GVCP_WRITEREG_ACK;
        ackHeader.cmdSpecificAckHdr = new strGvcpAckRegWriteHdr();
        strGvcpAckRegWriteHdr *writeRegAckPtr = (strGvcpAckRegWriteHdr *)ackHeader.cmdSpecificAckHdr;

        /* Copy contents from datagram to write register ack header. */
        writeRegAckPtr->reserved = qFromBigEndian(*(quint16 *)(dataPtr + strGvcpAckRegWriteHdrRESERVED));
        writeRegAckPtr->index = qFromBigEndian(*(quint16 *)(dataPtr + strGvcpAckRegWriteHdrINDEX));
    }
}

void gvcpMakeCmdSepcificCmdHeader(QByteArray& dataArray, const quint16 cmdType, const QByteArray& cmdSpecificData) {

    const char *dataPtr = cmdSpecificData.data();

    /* Check the correct command type and populate the command specific header. */
    if (cmdType == GVCP_DISCOVERY_CMD) {
        /* No command specific data needs to be appended for this command. */

    } else if (cmdType == GVCP_READREG_CMD) {
        quint32 regAddr = 0;

        /* Store the data in a temp buffer. */
        for (quint16 counter = 0; counter < cmdSpecificData.length() / sizeof(quint32); counter++) {
            regAddr = qToBigEndian(*((quint32 *)dataPtr + counter));

            /* Append data to the datagram. */
            dataArray.append((char *)&regAddr, sizeof(quint32));
        }

    } else if (cmdType == GVCP_READMEM_CMD) {
        strGvcpCmdReadMemHdr readMemHdr;

        /* Store data in the read memory command header buffer. */
        readMemHdr.address = qToBigEndian(*(quint32 *)(dataPtr + strGvcpCmdReadMemHdrADDRESS));
        readMemHdr.reserved = qToBigEndian(*(quint16 *)(dataPtr + strGvcpCmdReadMemHdrRESERVED));
        readMemHdr.count = qToBigEndian(*(quint16 *)(dataPtr + strGvcpCmdReadMemHdrCOUNT));

        /* Append data to the datagram. */
        dataArray.append((char *)&readMemHdr, sizeof(strGvcpCmdReadMemHdr));

    } else if (cmdType == GVCP_WRITEREG_CMD) {
        strGvcpCmdWriteRegHdr writeRegHdr[cmdSpecificData.length()/sizeof(strGvcpCmdWriteRegHdr)];

        /* Store data in the write register command header buffer. */
        for (quint32 counter = 0; counter < cmdSpecificData.length()/sizeof(strGvcpCmdWriteRegHdr); counter++) {
            writeRegHdr[counter].registerAddress = qToBigEndian(*(quint32 *)((quint32 *)dataPtr + 0 + counter*(sizeof(strGvcpCmdWriteRegHdr)/sizeof(quint32))));
            writeRegHdr[counter].registerData = qToBigEndian(*(quint32 *)((quint32 *)dataPtr + 1 + counter*(sizeof(strGvcpCmdWriteRegHdr)/sizeof(quint32))));
        }

        /* Append data to the datagram. */
        for (quint32 counter = 0; counter < cmdSpecificData.length()/sizeof(strGvcpCmdWriteRegHdr); counter++) {
            dataArray.append((char *)&writeRegHdr[counter], sizeof(strGvcpCmdReadMemHdr));
        }
    } else if (cmdType == GVCP_PACKETRESEND_CMD) {
        strGvcpCmdPktResendHdr pktResendCmdHdr;

        /* Store data in the packet resend command header buffer. */
        pktResendCmdHdr.streamChannelIdx = qToBigEndian(*(quint16 *)(dataPtr + strGvcpCmdPktResendHdrSTREAMCHANNELIDX));
        pktResendCmdHdr.blockIdRes = qToBigEndian(*(quint16 *)(dataPtr + strGvcpCmdPktResendHdrBLOCKIDRES));
        pktResendCmdHdr.firstPktId = qToBigEndian(*(quint32 *)(dataPtr + strGvcpCmdPktResendHdrFIRSTPKTID));
        pktResendCmdHdr.lastPktId = qToBigEndian(*(quint32 *)(dataPtr + strGvcpCmdPktResendHdrLASTPKTID));

        /* Append data to the datagram. */
        dataArray.append((char *)&pktResendCmdHdr, sizeof(strGvcpCmdPktResendHdr));
    }
}

void gvcpMakeGenericCmdHeader(const quint16 cmdType, QByteArray& dataArray, const QByteArray& cmdSpecificData, const quint16 reqId) {

    strGvcpCmdHdr genericHdr;

    /* Initialize memory to 0. */
    memset(&genericHdr, 0x00, sizeof(genericHdr));

    /* Construct generic header for a specific command type. */
    switch (cmdType) {

    case GVCP_DISCOVERY_CMD:
        /* Construct generic header for command [GVCP_DISCOVERY_CMD]. */
        genericHdr.flag = gvcpReverseBits(GVCP_CMD_FLAG_ACKNOWLEDGE | GVCP_CMD_FLAG_DISCOVERY_BROADCAST_ACK);
        break;

    case GVCP_READMEM_CMD:
        /* Construct generic header for command [GVCP_READMEM_CMD]. */
        genericHdr.flag = gvcpReverseBits(GVCP_CMD_FLAG_ACKNOWLEDGE);
        break;

    case GVCP_WRITEREG_CMD:
        /* Construct generic header for command [GVCP_WRITEREG_CMD]. */
        genericHdr.flag = gvcpReverseBits(GVCP_CMD_FLAG_ACKNOWLEDGE);
        break;

    case GVCP_READREG_CMD:
        /* Construct generic header for command [GVCP_READREG_CMD]. */
        genericHdr.flag = gvcpReverseBits(GVCP_CMD_FLAG_ACKNOWLEDGE);
        break;
    }

    /* Populate the attributes that are same in alomst every command. */
    genericHdr.keyCode = GVCP_CMD_HARDCODED_KEYCODE;
    genericHdr.command = qToBigEndian(cmdType);
    if (cmdSpecificData != nullptr)
        genericHdr.length = qToBigEndian((quint16)cmdSpecificData.length());
    genericHdr.reqId = qToBigEndian(reqId);

    /* Append generic header to the datagram. */
    dataArray.append((char *)&genericHdr, sizeof(genericHdr));
}

bool gvcpReceiveAck(QUdpSocket& udpSock, strNonStdGvcpAckHdr& ackHeader) {

    bool error = false;
    QByteArray tempArray;
    QNetworkDatagram datagram = { 0 };

    /* Receive datagram. */
    error = caminterface::camGigeVEthReceiveAck(udpSock, datagram);

    if (!error && datagram.data().length()) {

        /* A pointer pointing to the data of byte array. */
        tempArray.append(datagram.data());

        /* Copy contents of generic ack header. */
        memset(&ackHeader, 0x00, sizeof(ackHeader));

        ackHeader.genericAckHdr.status = qFromBigEndian(*(quint16 *)(tempArray.data() + strGvcpAckHdrSTATUS));
        ackHeader.genericAckHdr.acknowledge = qFromBigEndian(*(quint16 *)(tempArray.data() + strGvcpAckHdrACKNOWLEDGE));
        ackHeader.genericAckHdr.length = qFromBigEndian(*(quint16 *)(tempArray.data() + strGvcpAckHdrLENGTH));
        ackHeader.genericAckHdr.ackId = qFromBigEndian(*(quint16 *)(tempArray.data() + strGvcpAckHdrACKID));
    }

    if (!error && (ackHeader.genericAckHdr.length > 0) && tempArray.length()) {

        gvcpMakeCommandSpecificAckHeader(ackHeader, tempArray);
    } else {

        qDebug() << "Error: Command specific data in an ack cannot be empty";
        error = true;
    }

    if (error) {

        /* Was memory allocated for a command specific header? */
        if (ackHeader.cmdSpecificAckHdr) {

            /* Free the required memory in case of error. */
            gvcpFreeAckMemory(ackHeader);
        }
    }

    return (error);
}

bool gvcpSendCmd(QUdpSocket& udpSock, const quint16 cmdType, const QByteArray &cmdSpecificData,
                 const QHostAddress &destAddr, const quint16 port, const quint16 reqId)
{
    bool error = false;
    QNetworkDatagram datagram;
    QByteArray dataArray;

    /* Check to see if the pointer is not null. */
    if (!udpSock.isValid()) {
        error = true;
        qDebug() << "Error: Udp socket pointer is invalid.";
    }

    if (!error) {

        /* Popoulate the generic header with data. */
        gvcpMakeGenericCmdHeader(cmdType, dataArray, cmdSpecificData, reqId);

        /* Append only if data length is greater than 0. */
        if (cmdSpecificData.length() > 0) {

            /* Populate command specific headers and append in the array. */
            gvcpMakeCmdSepcificCmdHeader(dataArray, cmdType, cmdSpecificData);
        }

        datagram.setDestination(destAddr, port);
        datagram.setData(dataArray);
        error = caminterface::camGigeVEthSendCmd(udpSock, datagram);
    }

    return error;
}








