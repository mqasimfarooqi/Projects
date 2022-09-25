#include "cameraapi.h"
#include "gvcp/gvcp.h"
#include "gvsp/gvsp.h"
#include "quazip/JlCompress.h"

cameraApi::cameraApi(const QHostAddress hostIP, const QHostAddress camIP, const quint16 hostPort, QObject *parent)
    : QObject{parent}, mHostIPAddr(hostIP), mCamIPAddr(camIP), mGvcpHostPort(hostPort) {

    mVectorPendingReq.clear();
    mCamProps.statusFlags = 0;
    mCamProps.streamChannelIdx = 0;

    QThread::currentThread()->setObjectName("Control Thread");
    mStreamingThread.setObjectName("Streaming Thread");
}

void cameraApi::slotGvspReadyRead() {
    QNetworkDatagram gvspPkt;
    strGvspDataBlockHdr streamHdr;
    strGvspGenericDataLeaderHdr leader;
    strGvspGenericDataTrailerHdr trailer;
    strGvspDataBlockExtensionHdr extHdr;
    strGvspImageDataLeaderHdr imgLeaderHdr;
    strGvspImageDataTrailerHdr imgTrailerHdr;
    QByteArray dataArr;
    quint8 *dataPtr;
    quint32 expectedNoOfPackets;
    quint16 leastBlockID;
    QList<quint16> keys;
    QHash<quint32, QByteArray> frameHT;

    /* Receive the datagram. */
    gvspPkt = mGvspSock.receiveDatagram();
    dataArr = gvspPkt.data();
    dataPtr = (quint8 *)dataArr.data();

    /* Remove the first entry if max enteries become greater than CAMERA_FRAME_BUFFER_MAXSIZE. */
    if (mStreamHT.count() > CAMERA_MAX_FRAME_BUFFER_SIZE) {
        keys = mStreamHT.keys();
        leastBlockID = keys.constFirst();
        for (int counter = 0; counter < keys.size(); counter++) {
            if (keys.at(counter) < leastBlockID) {
                leastBlockID = keys.at(counter);
            }
        }
        qDebug() << "Removing hashtable entry with block ID = " << leastBlockID;
        mStreamHT.remove(leastBlockID);
    }

    /* Populate generic stream header. */
    streamHdr = gvspPopulateGenericDataHdrFromNetwork(dataPtr);
    dataPtr += sizeof(strGvspDataBlockHdr);

    /* Parse extended generic header if it is preasent. */
    if (streamHdr.extId_res_pktFmt_pktID$res & GVSP_DATA_BLOCK_HDR_EI_RES_PKTFMT_PKTID_EI) {

        extHdr = gvspPopulateGenericDataExtensionHdrFromNetwork(dataPtr);
        dataPtr += sizeof(strGvspDataBlockExtensionHdr);
    }

    /* Switch on the bases of packet format. */
    switch ((streamHdr.extId_res_pktFmt_pktID$res & GVSP_DATA_BLOCK_HDR_EI_RES_PKTFMT_PKTID_PKTFMT)
             >> GVSP_DATA_BLOCK_HDR_EI_RES_PKTFMT_PKTID_PKTFMT_SHIFT) {
    case GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_LEADER_FORMAT:
        leader.payloadTypeSpecific = qFromBigEndian(*(quint16 *)(dataPtr + strGvspGenericDataLeaderHdrPAYLOADTYPESPECIFIC));
        leader.payloadType = qFromBigEndian(*(quint16 *)(dataPtr + strGvspGenericDataLeaderHdrPAYLOADTYPE));

        switch (leader.payloadType) {
        case GVSP_DATA_BLOCK_HDR_PAYLOAD_TYPE_IMAGE:
            imgLeaderHdr = gvspPopulateImageLeaderHdrFromNetwork(dataPtr);

            expectedNoOfPackets = (((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) /
                                    (mCamProps.streamPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)) + 1);
            ((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) % (mCamProps.streamPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)) ?
                        expectedNoOfPackets++ : expectedNoOfPackets;

            frameHT.insert(streamHdr.extId_res_pktFmt_pktID$res & 0x00FFFFFF,
                           QByteArray((char *)&imgLeaderHdr, sizeof(strGvspImageDataLeaderHdr)));
            frameHT.reserve(expectedNoOfPackets);
            mStreamHT.insert(streamHdr.blockId$flag, frameHT);

            break;
        }
        break;
    case GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_TRAILER_FORMAT:
        trailer.reserved = qFromBigEndian(*(quint16 *)(dataPtr + strGvspGenericDataTrailerHdrRESERVED));
        trailer.payloadType = qFromBigEndian(*(quint16 *)(dataPtr + strGvspGenericDataTrailerHdrPAYLOADTYPE));

        switch (trailer.payloadType) {
        case GVSP_DATA_BLOCK_HDR_PAYLOAD_TYPE_IMAGE:
            imgTrailerHdr = gvspPopulateImageTrailerHdrFromNetwork(dataPtr);

            if (mStreamHT.contains(streamHdr.blockId$flag)) {
                mStreamHT[streamHdr.blockId$flag].insert(streamHdr.extId_res_pktFmt_pktID$res & 0x00FFFFFF, dataArr);

                memcpy(&imgLeaderHdr, mStreamHT[streamHdr.blockId$flag].value(0).data(), sizeof(strGvspImageDataLeaderHdr));
                expectedNoOfPackets = (((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) /
                                        (mCamProps.streamPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)) + 1);
                ((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) % (mCamProps.streamPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)) ?
                            expectedNoOfPackets++ : expectedNoOfPackets;

                if (mStreamHT[streamHdr.blockId$flag].count() < (int)expectedNoOfPackets) {
                    mPktResendBlockIDQueue.enqueue(streamHdr.blockId$flag);
                    emit signalResendRequested();
                } else {
                    qDebug() << "Packet received completely with block ID = " << streamHdr.blockId$flag;
                    mStreamHT.remove(streamHdr.blockId$flag);
                }
            }
            break;
        }
        break;
    case GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_PAYLOAD_FORMAT_GENERIC:
        if (mStreamHT.contains(streamHdr.blockId$flag) && (dataArr.size() > (int)sizeof(strGvspDataBlockHdr))) {
            mStreamHT[streamHdr.blockId$flag].insert(streamHdr.extId_res_pktFmt_pktID$res & 0x00FFFFFF,
                                                    QByteArray((char *)dataPtr + sizeof(strGvspDataBlockHdr),
                                                               (mCamProps.streamPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)));
        }
        break;
    }
}

void cameraApi::slotCameraHeartBeat() {
    strGvcpCmdReadRegHdr hdr;
    QList<quint32> value;

    hdr.registerAddress = 0xA00;
    cameraReadRegisterValue(QList<strGvcpCmdReadRegHdr>() << hdr, value);
}

void cameraApi::slotRequestResendRoutine() {
    strGvcpCmdPktResendHdr resendHdr;
    quint32 firstEmpty;
    quint32 packetIdx = 1;
    quint32 expectedNoOfPackets;
    quint16 blockID;
    strGvspImageDataLeaderHdr imgLeaderHdr;
    QHash<quint32, QByteArray> frameHT;

    while (!mPktResendBlockIDQueue.empty()) {
        blockID = mPktResendBlockIDQueue.dequeue();
        frameHT = mStreamHT[blockID];

        memcpy(&imgLeaderHdr, frameHT.value(0).data(), sizeof(strGvspImageDataLeaderHdr));
        expectedNoOfPackets = (((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) /
                                (mCamProps.streamPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)) + 1);
        ((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) % (mCamProps.streamPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)) ?
                    expectedNoOfPackets++ : expectedNoOfPackets;

#if 0 /* This part of the code is omitted because camera is unable to respond this fast to the exact packets missed. */
        quint32 lastEmpty;
        while (packetIdx < expectedNoOfPackets) {
            for (; packetIdx < expectedNoOfPackets; packetIdx++) {
                if (frameHT[packetIdx].isEmpty()) {
                    break;
                }
            }
            firstEmpty = packetIdx;
            for (packetIdx = firstEmpty; packetIdx < expectedNoOfPackets; packetIdx++) {
                if (!frameHT[packetIdx].isEmpty()) {
                    break;
                }
            }
            lastEmpty = packetIdx;

            resendHdr.blockIdRes = blockID;
            resendHdr.firstPktId = firstEmpty;
            resendHdr.lastPktId = lastEmpty;
            resendHdr.streamChannelIdx = mCamProps.streamChannelIdx;
        }
#else
        for (; packetIdx < expectedNoOfPackets; packetIdx++) {
            if (frameHT[packetIdx].isEmpty()) {
                break;
            }
        }
        firstEmpty = packetIdx;

        resendHdr.blockIdRes = blockID;
        resendHdr.firstPktId = firstEmpty;
        resendHdr.lastPktId = expectedNoOfPackets;
        resendHdr.streamChannelIdx = mCamProps.streamChannelIdx;
#endif

        /* Transmit a request to resubmit. */
        cameraRequestResend(resendHdr);

        qDebug() << "Size of Stream Hash Table = " << mStreamHT.count();
        qDebug() << "Slot called with BlockID = " << blockID;
    }
}

bool cameraApi::cameraXmlFetchChildElementValue(const QDomNode& parent, const QString& tagName, QString& value) {
    bool error = true;
    QDomNodeList nodeList;

    /* Find the value of a child element with specified tag. */
    nodeList = parent.toElement().elementsByTagName(tagName);
    if (nodeList.count() == 1) {
        value = nodeList.at(0).childNodes().at(0).nodeValue();
        error = false;
    }

    return error;
}

quint8 cameraApi::camStatusFlags() const {
    return mCamProps.statusFlags;
}

quint32 byteArrayToUint32(const QByteArray& bytes) {
    QByteArray byteArray;
    quint32 number = 0;

    if (bytes.count() > 4) {
        byteArray = bytes.mid(bytes.count() - 4);
    } else {
        byteArray = bytes;
    }

    QDataStream stream(byteArray);
    for (ulong var = byteArray.size(); var > 0; var--) {
        stream >> *((quint8 *)&number + (var - 1));
    }

    return number;
}

bool cameraApi::cameraReadXmlFileFromDevice() {
    Q_ASSERT_X(mGvcpSock.isValid(), __FUNCTION__, "GVSP socet for UDP is invalid.");
    Q_ASSERT_X(!mCamIPAddr.isNull(), __FUNCTION__, "Camera IP address is not set.");

    bool error = false;
    QList<QByteArray> url;
    QNetworkDatagram datagram;
    QByteArray first_url;
    QByteArray xml_data;

    /* Fetch first URL. */
    error = cameraFetchFirstUrl(first_url);

    if (!error) {

        /* Identify the location in which XML file is placed. */
        if (first_url.contains("Local:")) {

            /* Split the URL using standard delimiter. */
            url = first_url.mid(0, first_url.indexOf('\0')).split(';');

            if (!error && (url.count() == 3)) {
                error = cameraXmlFetchXmlFromDevice(url[0], url[1], url[2], xml_data);
            } else {

                qDebug() << "Error: Correct URL is not found.";
                error = true;
            }

        } else if (first_url.contains("http:")) {

            qDebug() << "Unsupported location of XML file.";
            error = true;
        } else {

            qDebug() << "Unsupported location of XML file.";
            error = true;
        }
    }

    if (!error) {

        /* Set content of the document. */
        if(!mCamXmlFile.setContent(xml_data)) {

            qDebug() << "Error: Unable to correctly parse the xml file fetched from device.";
            error = true;
        }
    }

    return error;
}

/* This function reads an attribute from the device using XML which was previously fetched. */
bool cameraApi::cameraReadCameraAttribute(const QList<QString>& attributeList, QList<QByteArray>& registerValues) {
    bool error = false;
    QDomElement root;
    QDomNodeList tempList;
    QDomNode tempNode;
    quint32 address = 0, length = 0;
    QString tempStr;
    strGvcpCmdReadRegHdr tempHdr;
    QList<quint32> tempRegisterVal;

    /* Check to see if this function is provided with a valid XML file. */
    if (!mCamXmlFile.isDocument()) {

        qDebug() << "XML file for the camera is not a valid file.";
        error = true;
    }

    if (!error) {

        /* Traverse through the list and find the address. */
        for (int var = 0; var < attributeList.count() && !error; var++) {

            /* Fetch the element with specified name. */
            error = cameraXmlFetchAttrElement(attributeList.at(var), tempNode);

            if (!error) {
                /* Fetch the child element of node with the name "Address" */
                error = cameraXmlFetchChildElementValue(tempNode, "Address", tempStr);

                if (!error) {
                    /* Convert address to uint32. */
                    address = byteArrayToUint32(QByteArray::fromHex(QByteArray::fromStdString(tempStr.toStdString())));

                    /* Fetch length of the register. */
                    error = cameraXmlFetchChildElementValue(tempNode, "Length", tempStr);

                    if (!error) {
                        /* Convert length to uint. */
                        length = tempStr.toInt();
                    }
                }

                if (!error) {
                    QByteArray tempData;

                    if (length > sizeof(quint32)) {
                        /* If length is greater than 4, then its a memory block and not a register. */
                        error = cameraReadMemoryBlock(address, length, tempData);

                        if (!error) {
                            registerValues.append(tempData);
                        }
                    } else {

                        /* Set the header. */
                        tempHdr.registerAddress = address;
                        tempRegisterVal.clear();

                        /* Read register value. */
                        error = cameraReadRegisterValue(QList<strGvcpCmdReadRegHdr>() << tempHdr, tempRegisterVal);

                        if (!error) {

                            /* Store register value. */
                            tempData.append((char *)&tempRegisterVal.at(0), sizeof(quint32));
                            registerValues.append(tempData);
                        }
                    }
                }
            }
        }
    }
    return error;
}

bool cameraApi::cameraWriteCameraAttribute(const QList<QString>& attributeList, const QList<QByteArray>& regValues) {
    bool error = false;
    QDomElement root;
    QDomNodeList tempList;
    QDomNode tempNode;
    quint32 address = 0, length = 0;
    QString tempStr;
    strGvcpCmdWriteRegHdr writeUnit;

    /* Check to see if this function is provided with a valid XML file. */
    if (!mCamXmlFile.isDocument()) {

        qDebug() << "XML file for the camera is not a valid file.";
        error = true;
    }

    if (attributeList.count() != regValues.count()) {

        qDebug() << "Not all the values against each attribute is provided.";
        error = true;
    }

    if (!error) {

        /* Traverse through the list and find the address. */
        for (int var = 0; var < attributeList.count() && !error; var++) {

            /* Fetch the element with specified name. */
            error = cameraXmlFetchAttrElement(attributeList.at(var), tempNode);

            if (!error) {
                /* Fetch the child element of node with the name "Address" */
                error = cameraXmlFetchChildElementValue(tempNode, "Address", tempStr);

                if (!error) {
                    /* Convert address to uint32. */
                    address = byteArrayToUint32(QByteArray::fromHex(QByteArray::fromStdString(tempStr.toStdString())));

                    /* Fetch length of the register. */
                    error = cameraXmlFetchChildElementValue(tempNode, "Length", tempStr);

                    if (!error) {
                        /* Convert length to uint. */
                        length = tempStr.toInt();
                    }
                }

                if (!error) {

                    if (length > sizeof(quint32)) {

                        qDebug() << "Not yet supported to write memory greater than " << sizeof(quint32);

                    } else {

                        /* Set the header. */
                        writeUnit.registerAddress = address;
                        writeUnit.registerData = byteArrayToUint32(regValues.at(var));

                        /* Read register value. */
                        error = cameraWriteRegisterValue(QList<strGvcpCmdWriteRegHdr>() << writeUnit);

                        if (error) {

                            qDebug() << "Unable to write register at address: " << writeUnit.registerAddress;
                        }
                    }
                }
            }
        }
    }

    return error;
}

bool cameraApi::cameraWriteRegisterValue(const QList<strGvcpCmdWriteRegHdr>& writeUnits) {
    bool error = false;
    QByteArray cmdSpecificData;
    strNonStdGvcpAckHdr ackHdr;
    QNetworkDatagram datagram;
    quint16 reqId = (QDateTime::currentMSecsSinceEpoch() % 50) + 1;

    /* Initialize memory to 0. */
    memset(&ackHdr, 0x00, sizeof(strNonStdGvcpAckHdr));
    cmdSpecificData.clear();

    if (writeUnits.isEmpty()) {

        qDebug() << "Error: No data is provided in the array to write.";
        error = true;
    }

    if (!error) {

        /* Copy data into the array. */
        for (int counter = 0; counter < writeUnits.length(); counter++) {
            cmdSpecificData.append((char *)&writeUnits.at(counter), sizeof(strGvcpCmdWriteRegHdr));
        }

        /* Send command to the specified address. */
        mVectorPendingReq.push_front(reqId);
        error = gvcpSendCmd(mGvcpSock, GVCP_WRITEREG_CMD, cmdSpecificData, mCamIPAddr, GVCP_DEFAULT_UDP_PORT, reqId);
    }

    if (!error) {

        /* Receive data in the array. */
        error = true;
        for (int retryCount = 0;
            (retryCount < CAMERA_MAX_ACK_FETCH_RETRY_COUNT) && error;
            (retryCount++)) {

            /* Wait for the signal, for reception of first URL. */
            if (mGvcpSock.waitForReadyRead(CAMERA_WAIT_FOR_ACK_MS)) {

                /* Receive the next pending ack. */
                error = cameraFetchAck(ackHdr, reqId);
            }
        }
    }

    if (!error && ackHdr.cmdSpecificAckHdr) {

        /* Free memory allocated by ack header. */
        gvcpFreeAckMemory(ackHdr);
    }

    return error;
}

bool cameraApi::cameraFetchAck(strNonStdGvcpAckHdr& ackHdr, const quint16 reqId)
{
    bool error = true;

    /* Receive the next pending ack. */
    error = gvcpReceiveAck(mGvcpSock, ackHdr);

    /* If correct ack is received */
    if ((ackHdr.genericAckHdr.ackId == reqId) && !error) {

        /* Remove the pending request ID stored in vector. */
        mVectorPendingReq.removeOne(ackHdr.genericAckHdr.ackId);
        mVectorPendingReq.squeeze();

        error = false;

    } else {

        if (ackHdr.cmdSpecificAckHdr) {

            /* Free memory space acquired by command specific ack header for packed with undesired req_id. */
            gvcpFreeAckMemory(ackHdr);
        }
    }

    return error;
}

bool cameraApi::cameraReadMemoryBlock(const quint32 address, const quint16 size, QByteArray& returnedData) {
    bool error = false;
    QByteArray cmdSpecificData;
    strGvcpCmdReadMemHdr readMemHdr;
    strNonStdGvcpAckHdr ackHdr;
    quint16 reqId = (QDateTime::currentMSecsSinceEpoch() % 50) + 1;
    quint16 bytesRead = 0;
    quint32 retry_count = 0;

    /* General precautions. */
    returnedData.clear();
    memset(&readMemHdr, 0x00, sizeof(strGvcpCmdReadMemHdr));

    /* Build the command specific header. */
    readMemHdr.address = (address);
    readMemHdr.count = (GVCP_MAX_PAYLOAD_LENGTH < size ? (quint16)GVCP_MAX_PAYLOAD_LENGTH : size);

    while ((readMemHdr.address < (address + size)) && !error) {

        /* Append data set for URL header in generic command specidifc data array. */
        cmdSpecificData.append((const char *)&readMemHdr, sizeof(readMemHdr));

        /* Send command to the specified address. */
        mVectorPendingReq.push_front(reqId);
        error = gvcpSendCmd(mGvcpSock, GVCP_READMEM_CMD, cmdSpecificData, mCamIPAddr, GVCP_DEFAULT_UDP_PORT, reqId);

        if (!error) {

            /* Receive data in the array. */
            error = true;
            for (retry_count = 0;
                (retry_count < CAMERA_MAX_ACK_FETCH_RETRY_COUNT) && error;
                (retry_count++)) {

                /* Wait for the signal, for reception of first URL. */
                if (mGvcpSock.waitForReadyRead(CAMERA_WAIT_FOR_ACK_MS)) {

                    /* Receive the next pending ack. */
                    error = cameraFetchAck(ackHdr, reqId);
                }
            }
        }

        if (error) {

            /* Memory block missed from between */
            break;
        }

        if (!error) {

            /* Increase the address to point to new memory location. */
            readMemHdr.address += readMemHdr.count;

            /* Increase bytes read counter. */
            bytesRead += readMemHdr.count;

            /* Set the next number of bytes to be read. */
            readMemHdr.count = (((readMemHdr.address + GVCP_MAX_PAYLOAD_LENGTH) < (address + size)) ?
                                    GVCP_MAX_PAYLOAD_LENGTH : (size - bytesRead));

            /* Append data to the byte_array to be returned. */
            returnedData.append(((strGvcpAckMemReadHdr *)ackHdr.cmdSpecificAckHdr)->data);

            /* Free up memory space acquired by command specific header. */
            if (ackHdr.cmdSpecificAckHdr) {

                gvcpFreeAckMemory(ackHdr);
            }

            /* Increase the request ID for next transation. */
            reqId++;

            /* Clear buffers for next command processing. */
            cmdSpecificData.clear();
        }
    }

    if (error) {

        qDebug() << "Error: Corrupted block. Could not fetch the file correctly.";
    }

    return error;
}

bool cameraApi::cameraFetchFirstUrl(QByteArray& byteArray)
{
    Q_ASSERT_X(mGvcpSock.isValid(), __FUNCTION__, "GVSP socekt for UDP is invalid.");
    Q_ASSERT_X(!mCamIPAddr.isNull(), __FUNCTION__, "Camera IP address is not set.");

    bool error = false;
    QByteArray cmdSpecificData;
    strGvcpCmdReadMemHdr cmdHdr;
    strNonStdGvcpAckHdr ackHdr;
    strGvcpAckMemReadHdr *readMemAckHdr;
    QNetworkDatagram datagram;
    quint16 reqId = (QDateTime::currentMSecsSinceEpoch() % 50) + 1;

    /* Clear buffers for command processing. */
    cmdSpecificData.clear();
    memset(&cmdHdr, 0x00, sizeof(*(&cmdHdr)));
    memset(&ackHdr, 0x00, sizeof(*(&ackHdr)));

    /* Set properties of header to fetch URL. */
    cmdHdr.address = (quint32)DEVICE_FIRST_URL_ADDRESS;
    cmdHdr.count = (quint16)DEVICE_URL_ADDRESS_REG_LENGTH;

    /* Append data set for URL header in generic command specidifc data array. */
    cmdSpecificData.append((const char *)&cmdHdr, sizeof(cmdHdr));

    /* Send command to the specified address. */
    mVectorPendingReq.push_front(reqId);
    error = gvcpSendCmd(mGvcpSock, GVCP_READMEM_CMD, cmdSpecificData, mCamIPAddr, GVCP_DEFAULT_UDP_PORT, reqId);

    if (!error) {

        /* Receive data in the array. */
        error = true;
        for (int retryCount = 0;
            (retryCount < CAMERA_MAX_ACK_FETCH_RETRY_COUNT) && error;
            (retryCount++)) {

            /* Wait for the signal, for reception of first URL. */
            if (mGvcpSock.waitForReadyRead(CAMERA_WAIT_FOR_ACK_MS)) {

                /* Receive the next pending ack. */
                error = cameraFetchAck(ackHdr, reqId);
            }
        }
    }

    if (!error && (ackHdr.cmdSpecificAckHdr != nullptr)) {

        /* Now we have first URL. */
        readMemAckHdr = (strGvcpAckMemReadHdr *)ackHdr.cmdSpecificAckHdr;

        /* Append data into the byte array, and return it. */
        byteArray.append(readMemAckHdr->data);

        /* Free memory allocated by ack header. */
        gvcpFreeAckMemory(ackHdr);
    }

    return error;
}

bool cameraApi::cameraXmlFetchAttrElement(const QString &attributeName, QDomNode& node) {
    bool error = true;
    QDomNodeList elementList;
    QDomElement root;

    /* Start with the first child element. */
    root = mCamXmlFile.firstChildElement();

    /* Parse the XML and get all the nodes with following tags. */
    for (int lookupTagsCounter = 0; lookupTagsCounter < lookupTags.count(); lookupTagsCounter++) {
        elementList = root.elementsByTagName(lookupTags.at(lookupTagsCounter));

        if (!elementList.isEmpty()) {

            for (int elementCounter = 0; elementCounter < elementList.count(); elementCounter++) {

                if(!QString::compare(elementList.at(elementCounter).attributes().namedItem("Name").nodeValue(), attributeName)) {

                    node = elementList.at(elementCounter);
                    error = false;
                    break;
                }
            }

            if (!error) {

                /* Check to see if correct element has been found. */
                break;
            }
        }
    }

    return error;
}

bool cameraApi::cameraReadRegisterValue(const QList<strGvcpCmdReadRegHdr> addressList, QList<quint32>& regValues) {
    bool error = false;
    QByteArray cmdSpecificData;
    strNonStdGvcpAckHdr ackHdr;
    strGvcpAckRegReadHdr *regReadAckHdr;
    QNetworkDatagram datagram;
    quint16 reqId = (QDateTime::currentMSecsSinceEpoch() % 50) + 1;

    /* Initialize memory to 0. */
    memset(&ackHdr, 0x00, sizeof(*(&ackHdr)));
    cmdSpecificData.clear();

    /* Check to see if the address list is empty. */
    if (addressList.empty()) {

        qDebug() << "Error: Feature list is empty";
        error = true;
    }

    if (!error) {

        /* Copy data into the array. */
        for (int counter = 0; counter < addressList.length(); counter++) {
            cmdSpecificData.append((char *)&addressList.at(counter), sizeof(strGvcpCmdReadRegHdr));
        }

        /* Send command to the specified address. */
        mVectorPendingReq.push_front(reqId);
        error = gvcpSendCmd(mGvcpSock, GVCP_READREG_CMD, cmdSpecificData, mCamIPAddr, GVCP_DEFAULT_UDP_PORT, reqId);
    }

    if (!error) {

        /* Receive data in the array. */
        error = true;
        for (int retryCount = 0;
            (retryCount < CAMERA_MAX_ACK_FETCH_RETRY_COUNT) && error;
            (retryCount++)) {

            /* Wait for the signal, for reception of first URL. */
            if (mGvcpSock.waitForReadyRead(CAMERA_WAIT_FOR_ACK_MS)) {

                /* Receive the next pending ack. */
                error = cameraFetchAck(ackHdr, reqId);
            }
        }
    }

    if (!error && ackHdr.cmdSpecificAckHdr) {

        /* Now we have first URL. */
        regReadAckHdr = (strGvcpAckRegReadHdr *)ackHdr.cmdSpecificAckHdr;

        /* Append register values received from the camera. */
        for (quint16 counter = 0; counter < ackHdr.genericAckHdr.length/sizeof(quint32); counter++) {
            regValues.append(regReadAckHdr->registerData[counter]);
        }

        /* Free memory allocated by ack header. */
        gvcpFreeAckMemory(ackHdr);
    }

    return error;
}

/* This function discovers the device and populates the ack header. */
bool cameraApi::cameraDiscoverDevice(const QHostAddress destAddr, strGvcpAckDiscoveryHdr& discAckHdr) {
    Q_ASSERT_X(mGvcpSock.isValid(), __FUNCTION__, "GVSP socket for UDP is invalid.");
    Q_ASSERT_X(!destAddr.isNull(), __FUNCTION__, "Camera IP address is not set.");

    bool error = false;
    strNonStdGvcpAckHdr ackHdr;
    QByteArray cmdSpecificData;
    quint16 reqId = (QDateTime::currentMSecsSinceEpoch() % 50) + 1;

    /* Initialize memory to 0. */
    memset(&ackHdr, 0x00, sizeof(strNonStdGvcpAckHdr));
    cmdSpecificData.clear();

    /* Send command to the specified address. */
    mVectorPendingReq.push_front(reqId);
    error = gvcpSendCmd(mGvcpSock, GVCP_DISCOVERY_CMD, cmdSpecificData, destAddr, GVCP_DEFAULT_UDP_PORT, reqId);

    if (!error) {

        /* Try to fetch the replies for MAX_ACK_FETCH_RETRY_COUNT times. */
        error = true;
        for (int retryCount = 0;
            (retryCount < CAMERA_MAX_ACK_FETCH_RETRY_COUNT) && error;
            (retryCount++)) {

            /* Wait for the signal,. */
            if (mGvcpSock.waitForReadyRead(CAMERA_WAIT_FOR_ACK_MS)) {

                /* Receive the next pending ack. */
                error = cameraFetchAck(ackHdr, reqId);
            }
        }
    }

    if (!error) {

        strGvcpAckDiscoveryHdr *ptr = (strGvcpAckDiscoveryHdr *)ackHdr.cmdSpecificAckHdr;
        memcpy(&discAckHdr, ptr, sizeof(strGvcpAckDiscoveryHdr));
        gvcpFreeAckMemory(ackHdr);

    } else {

        qDebug() << "Error: Acknowledgement packet is not received.";
    }

    return error;
}

bool cameraApi::cameraRequestResend(const strGvcpCmdPktResendHdr& cmdHdr) {
    Q_ASSERT_X(mGvcpSock.isValid(), __FUNCTION__, "GVSP socket for UDP is invalid.");
    Q_ASSERT_X(!mCamIPAddr.isNull(), __FUNCTION__, "Camera IP address is not set.");

    bool error = false;
    QByteArray cmdSpecificData;
    quint16 reqId = (QDateTime::currentMSecsSinceEpoch() % 50) + 1;

    /* Initialize memory to 0. */
    cmdSpecificData.clear();

    /* Append command specific header. */
    cmdSpecificData.append((char *)&cmdHdr, sizeof(strGvcpCmdPktResendHdr));

    /* Send command to the specified address. */
    mVectorPendingReq.push_front(reqId);
    error = gvcpSendCmd(mGvcpSock, GVCP_PACKETRESEND_CMD, cmdSpecificData, mCamIPAddr, GVCP_DEFAULT_UDP_PORT, reqId);

    return error;
}

bool cameraApi::cameraStartStream(const quint16 streamHostPort)
{
    bool error = false;
    QList<QByteArray> values;
    quint32 val;

    if (!error) {

        /* Write the destination address Gvsp packets. */
        val = qToBigEndian(mHostIPAddr.toIPv4Address());
        error = cameraWriteCameraAttribute(QList<QString>() << "GevSCDAReg",
                                           QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint32)));
    }

    if (!error) {

        /* Write the port to listen to for Gvsp packets. */
        val = qToBigEndian(streamHostPort);
        error = cameraWriteCameraAttribute(QList<QString>() << "GevSCPHostPortReg",
                                           QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint16)));

        if (!error) {

            /* Initialize the Host Gvcp UDP socket. */
            if (!(mGvspSock.bind(mHostIPAddr, streamHostPort))) {

                qDebug() << "Could not bind socket to GVSP ip/port.";
                error = true;

            } else {

                /* Make relevant connections. */
                connect(&mGvspSock, &QUdpSocket::readyRead, this, &cameraApi::slotGvspReadyRead, Qt::DirectConnection);

                /* Move the socket to another thread and start. */
                mGvspSock.moveToThread(&mStreamingThread);
                mStreamingThread.start();
            }
        }
    }

    if (!error) {
        val = qToBigEndian(CAMERA_GVSP_PAYLOAD_SIZE);
        error = cameraWriteCameraAttribute(QList<QString>() << "GevSCPSPacketSizeReg",
                                           QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint32)));
    }

    if (!error) {
        error = cameraReadCameraAttribute(QList<QString>() << "GevSCPSPacketSizeReg", values);
        mCamProps.streamPktSize = qFromBigEndian((quint16)byteArrayToUint32(values.first().left(2)));
    }

    if (!error) {
        val = qToBigEndian(0x1);
        error = cameraWriteCameraAttribute(QList<QString>() << "AcquisitionStartReg",
                                           QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint32)));
    }

    return error;
}

bool cameraApi::cameraInitializeDevice() {
    bool error = false;
    QList<QByteArray> value;
    quint32 val = 0;

    /* Initialize the Host Gvcp UDP socket. */
    if (!(mGvcpSock.bind(mHostIPAddr, mGvcpHostPort))) {

        qDebug() << "Could not bind socket to GVCP ip/port.";
        error = true;
    }

    if (!error) {

        /* Fetch camera XML. */
        error = cameraReadXmlFileFromDevice();
    }

    if (!error) {

        /* Setting up heartbeat. */
        error = cameraReadCameraAttribute(QList<QString>() << "GevHeartbeatTimeoutReg", value);
    }

    if (!error) {

        /* Acquire control channel. */
        val = qToBigEndian(0x2);
        error = cameraWriteCameraAttribute(QList<QString>() << "GevCCPReg",
                                           QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint32)));
    }

    if (!error) {

        val = qFromBigEndian(byteArrayToUint32(value.first()));

        /* Set interval to the value fetched from camera. */
        mHeartBeatTimer.setInterval(val/8);

        /* Make relevant connections. */
        connect(&mHeartBeatTimer, &QTimer::timeout, this, &cameraApi::slotCameraHeartBeat, Qt::ConnectionType::DirectConnection);

        /* Fire off the timer. */
        mHeartBeatTimer.start();

        /* Make connection to receive packet resend requests. */
        connect(this, &cameraApi::signalResendRequested, this, &cameraApi::slotRequestResendRoutine, Qt::ConnectionType::QueuedConnection);
    }

    if (!error) {

        qDebug() << "Camera initialized successfully.";

        mCamProps.statusFlags |= CAMERA_STATUS_FLAGS_INITIALIZED;
    }

    return error;
}

bool cameraApi::cameraXmlFetchXmlFromDevice(const QByteArray fileName, const QByteArray startAddress,
                                            const QByteArray size, QByteArray& xmlData) {
    bool error = false;
    QByteArray retrievedXml;
    QNetworkDatagram datagram;
    QStringList list;
    QFile file(fileName), xmlFile;
    QDataStream stream(&file);
    const quint32 startAddressReadFromUrl = byteArrayToUint32(QByteArray::fromHex(startAddress));
    const quint16 sizeReadFromUrl = (quint16)byteArrayToUint32(QByteArray::fromHex(size));

    /* Retrieve data from the memory blockes where XML file is stored in the device. */
    error = cameraReadMemoryBlock(startAddressReadFromUrl, sizeReadFromUrl, retrievedXml);
    if (!error) {
        if (file.open(QIODevice::ReadWrite)) {
            stream.setByteOrder(QDataStream::LittleEndian);
            stream << retrievedXml;
        }

        list = JlCompress::extractDir(fileName, fileName + "_extracted");
        if (list.isEmpty()) {
            error = true;
        } else {

            xmlFile.setFileName(list.first());
            if (xmlFile.open(QIODevice::ReadWrite)) {
                xmlData = xmlFile.readAll();
            }
        }
    }

    return error;
}
