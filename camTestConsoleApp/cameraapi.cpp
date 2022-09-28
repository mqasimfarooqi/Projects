#include "cameraapi.h"
#include "gvcp/gvcp.h"
#include "gvsp/gvsp.h"
#include "packethandler.h"
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

    while (mGvspSock.hasPendingDatagrams()) {
        mMutex.lock();
        mStreamReceiveQueue.enqueue(mGvspSock.receiveDatagram());
        mMutex.unlock();

        emit signalDatagramEnqueued();
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

        qDebug() << "Executing resend routine for block ID = " << resendHdr.blockIdRes
                 << " from packet " << resendHdr.firstPktId << " to " << resendHdr.lastPktId;

        /* Transmit a request to resubmit. */
        cameraRequestResend(resendHdr);
    }
}

quint32 cameraApi::cameraXmlFetchChildElementValue(const QDomNode& parent, const QString& tagName, QString& value) {
    quint32 error = CAMERA_API_STATUS_FAILED;
    QDomNodeList nodeList;

    /* Find the value of a child element with specified tag. */
    nodeList = parent.toElement().elementsByTagName(tagName);
    if (nodeList.count() == 1) {
        value = nodeList.at(0).childNodes().at(0).nodeValue();
        error = CAMERA_API_STATUS_SUCCESS;
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

quint32 cameraApi::cameraReadXmlFileFromDevice() {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
    QList<QByteArray> url;
    QNetworkDatagram datagram;
    QByteArray first_url;
    QByteArray xml_data;

    /* Fetch first URL. */
    error = cameraFetchFirstUrl(first_url);
    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Identify the location in which XML file is placed. */
        if (first_url.contains("Local:")) {

            /* Split the URL using standard delimiter. */
            url = first_url.mid(0, first_url.indexOf('\0')).split(';');

            if ((error == CAMERA_API_STATUS_SUCCESS) && (url.count() == 3)) {
                error = cameraXmlFetchXmlFromDevice(url[0], url[1], url[2], xml_data);
            } else {

                qDebug() << "Error: Correct URL is not found.";
                error = CAMERA_API_STATUS_FAILED;
            }

        } else if (first_url.contains("http:")) {

            qDebug() << "Unsupported location of XML file.";
            error = CAMERA_API_STATUS_FAILED;
        } else {

            qDebug() << "Unsupported location of XML file.";
            error = CAMERA_API_STATUS_FAILED;
        }
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Set content of the document. */
        if(!mCamXmlFile.setContent(xml_data)) {

            qDebug() << "Error: Unable to correctly parse the xml file fetched from device.";
            error = CAMERA_API_STATUS_FAILED;
        }
    }

    return error;
}

/* This function reads an attribute from the device using XML which was previously fetched. */
quint32 cameraApi::cameraReadCameraAttribute(const QList<QString>& attributeList, QList<QByteArray>& registerValues) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
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
        error = CAMERA_API_STATUS_FAILED;
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Traverse through the list and find the address. */
        for (int var = 0; var < attributeList.count() && !error; var++) {

            /* Fetch the element with specified name. */
            error = cameraXmlFetchAttrElement(attributeList.at(var), tempNode);

            if (error == CAMERA_API_STATUS_SUCCESS) {

                /* Fetch register name from this element. */
                error = cameraXmlFetchChildElementValue(tempNode, "pValue", tempStr);

                if (error == CAMERA_API_STATUS_SUCCESS) {

                    /* Fetch the register element. */
                    error = cameraXmlFetchAttrElement(tempStr, tempNode);
                }

                if ((tempNode.toElement().nodeName() == "IntConverter") ||
                    (tempNode.toElement().nodeName() == "Integer")) {

                    /* Fetch register name from this element. */
                    error = cameraXmlFetchChildElementValue(tempNode, "pValue", tempStr);

                    if (error == CAMERA_API_STATUS_SUCCESS) {

                        /* Fetch the register element. */
                        error = cameraXmlFetchAttrElement(tempStr, tempNode);
                    }
                } else if (tempNode.toElement().nodeName() == "StructEntry") {

                    tempNode = tempNode.parentNode();
                    tempNode.toElement().nodeValue();
                    qDebug() << "Struct entry found.";
                }

                if (error == CAMERA_API_STATUS_SUCCESS) {

                    /* Fetch the child element of node with the name "Address" */
                    error = cameraXmlFetchChildElementValue(tempNode, "Address", tempStr);
                }

                if (error == CAMERA_API_STATUS_SUCCESS) {
                    /* Convert address to uint32. */
                    address = byteArrayToUint32(QByteArray::fromHex(QByteArray::fromStdString(tempStr.toStdString())));

                    /* Fetch length of the register. */
                    error = cameraXmlFetchChildElementValue(tempNode, "Length", tempStr);

                    if (error == CAMERA_API_STATUS_SUCCESS) {
                        /* Convert length to uint. */
                        length = tempStr.toInt();
                    }
                }

                if (error == CAMERA_API_STATUS_SUCCESS) {
                    QByteArray tempData;

                    if (length > sizeof(quint32)) {
                        /* If length is greater than 4, then its a memory block and not a register. */
                        error = cameraReadMemoryBlock(address, length, tempData);

                        if (error == CAMERA_API_STATUS_SUCCESS) {
                            registerValues.append(tempData);
                        }
                    } else {

                        /* Set the header. */
                        tempHdr.registerAddress = address;
                        tempRegisterVal.clear();

                        /* Read register value. */
                        error = cameraReadRegisterValue(QList<strGvcpCmdReadRegHdr>() << tempHdr, tempRegisterVal);

                        if (error == CAMERA_API_STATUS_SUCCESS) {

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

quint32 cameraApi::cameraWriteCameraAttribute(const QList<QString>& attributeList, const QList<QByteArray>& regValues) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
    QDomElement root;
    QDomNodeList tempList;
    QDomNode tempNode;
    quint32 address = 0, length = 0;
    QString tempStr;
    strGvcpCmdWriteRegHdr writeUnit;

    /* Check to see if this function is provided with a valid XML file. */
    if (!mCamXmlFile.isDocument()) {

        qDebug() << "XML file for the camera is not a valid file.";
        error = CAMERA_API_STATUS_FAILED;
    }

    if (attributeList.count() != regValues.count()) {

        qDebug() << "Not all the values against each attribute is provided.";
        error = CAMERA_API_STATUS_FAILED;
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Traverse through the list and find the address. */
        for (int var = 0; var < attributeList.count() && !error; var++) {

            /* Fetch the element with specified name. */
            error = cameraXmlFetchAttrElement(attributeList.at(var), tempNode);

            if (error == CAMERA_API_STATUS_SUCCESS) {

                /* Fetch register name from this element. */
                error = cameraXmlFetchChildElementValue(tempNode, "pValue", tempStr);

                if (error == CAMERA_API_STATUS_SUCCESS) {

                    /* Fetch the register element. */
                    error = cameraXmlFetchAttrElement(tempStr, tempNode);
                }

                if (error == CAMERA_API_STATUS_SUCCESS) {

                    if ((tempNode.toElement().nodeName() == "IntConverter") ||
                        (tempNode.toElement().nodeName() == "Integer")) {

                        /* Fetch register name from this element. */
                        error = cameraXmlFetchChildElementValue(tempNode, "pValue", tempStr);

                        if (error == CAMERA_API_STATUS_SUCCESS) {

                            /* Fetch the register element. */
                            error = cameraXmlFetchAttrElement(tempStr, tempNode);
                        }
                    }

                    if (error == CAMERA_API_STATUS_SUCCESS) {

                        /* Fetch the child element of node with the name "Address" */
                        error = cameraXmlFetchChildElementValue(tempNode, "Address", tempStr);
                    }
                }

                if (error == CAMERA_API_STATUS_SUCCESS) {

                    /* Convert address to uint32. */
                    address = byteArrayToUint32(QByteArray::fromHex(QByteArray::fromStdString(tempStr.toStdString())));

                    /* Fetch length of the register. */
                    error = cameraXmlFetchChildElementValue(tempNode, "Length", tempStr);

                    if (error == CAMERA_API_STATUS_SUCCESS) {
                        /* Convert length to uint. */
                        length = tempStr.toInt();
                    }
                }

                if (error == CAMERA_API_STATUS_SUCCESS) {

                    if (length > sizeof(quint32)) {

                        qDebug() << "Not yet supported to write memory greater than " << sizeof(quint32);

                    } else {

                        /* Set the header. */
                        writeUnit.registerAddress = address;
                        writeUnit.registerData = byteArrayToUint32(regValues.at(var));

                        /* Read register value. */
                        error = cameraWriteRegisterValue(QList<strGvcpCmdWriteRegHdr>() << writeUnit);
                        if (error == CAMERA_API_STATUS_FAILED) {

                            qDebug() << "Unable to write register at address: " << writeUnit.registerAddress;
                        }
                    }
                }
            }
        }
    }

    return error;
}

quint32 cameraApi::cameraWriteRegisterValue(const QList<strGvcpCmdWriteRegHdr>& writeUnits) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
    QByteArray cmdSpecificData;
    strNonStdGvcpAckHdr ackHdr;
    QNetworkDatagram datagram;
    quint16 reqId = (QDateTime::currentMSecsSinceEpoch() % 50) + 1;

    /* Initialize memory to 0. */
    memset(&ackHdr, 0x00, sizeof(strNonStdGvcpAckHdr));
    cmdSpecificData.clear();

    if (writeUnits.isEmpty()) {

        qDebug() << "Error: No data is provided in the array to write.";
        error = CAMERA_API_STATUS_FAILED;
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Copy data into the array. */
        for (int counter = 0; counter < writeUnits.length(); counter++) {
            cmdSpecificData.append((char *)&writeUnits.at(counter), sizeof(strGvcpCmdWriteRegHdr));
        }

        /* Send command to the specified address. */
        mVectorPendingReq.push_front(reqId);
        error = gvcpSendCmd(mGvcpSock, GVCP_WRITEREG_CMD, cmdSpecificData, mCamIPAddr, GVCP_DEFAULT_UDP_PORT, reqId);
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Receive data in the array. */
        error = CAMERA_API_STATUS_FAILED;
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

    if ((error == CAMERA_API_STATUS_SUCCESS) && ackHdr.cmdSpecificAckHdr) {

        /* Free memory allocated by ack header. */
        gvcpFreeAckMemory(ackHdr);
    }

    return error;
}

quint32 cameraApi::cameraFetchAck(strNonStdGvcpAckHdr& ackHdr, const quint16 reqId)
{
    quint32 error;

    /* Receive the next pending ack. */
    error = gvcpReceiveAck(mGvcpSock, ackHdr);
    if ((ackHdr.genericAckHdr.ackId == reqId) && (error == CAMERA_API_STATUS_SUCCESS)) {

        /* Remove the pending request ID stored in vector. */
        mVectorPendingReq.removeOne(ackHdr.genericAckHdr.ackId);
        mVectorPendingReq.squeeze();

    } else {

        if (ackHdr.cmdSpecificAckHdr) {

            /* Free memory space acquired by command specific ack header for packed with undesired req_id. */
            gvcpFreeAckMemory(ackHdr);
        }
    }

    return error;
}

quint32 cameraApi::cameraReadMemoryBlock(const quint32 address, const quint16 size, QByteArray& returnedData) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
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

    while ((readMemHdr.address < (address + size)) && (error == CAMERA_API_STATUS_SUCCESS)) {

        /* Append data set for URL header in generic command specidifc data array. */
        cmdSpecificData.append((const char *)&readMemHdr, sizeof(readMemHdr));

        /* Send command to the specified address. */
        mVectorPendingReq.push_front(reqId);
        error = gvcpSendCmd(mGvcpSock, GVCP_READMEM_CMD, cmdSpecificData, mCamIPAddr, GVCP_DEFAULT_UDP_PORT, reqId);
        if (error == CAMERA_API_STATUS_SUCCESS) {

            /* Receive data in the array. */
            error = CAMERA_API_STATUS_FAILED;
            for (retry_count = 0;
                (retry_count < CAMERA_MAX_ACK_FETCH_RETRY_COUNT) && (error == CAMERA_API_STATUS_FAILED);
                (retry_count++)) {

                /* Wait for the signal, for reception of first URL. */
                if (mGvcpSock.waitForReadyRead(CAMERA_WAIT_FOR_ACK_MS)) {

                    /* Receive the next pending ack. */
                    error = cameraFetchAck(ackHdr, reqId);
                }
            }
        }

        if (error == CAMERA_API_STATUS_SUCCESS) {

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

        } else {

            /* Memory block missed from between */
            break;
        }
    }

    if (error == CAMERA_API_STATUS_FAILED) {

        qDebug() << "Error: Corrupted block. Could not fetch the file correctly.";
    }

    return error;
}

quint32 cameraApi::cameraFetchFirstUrl(QByteArray& byteArray) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
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
    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Receive data in the array. */
        error = CAMERA_API_STATUS_FAILED;
        for (int retryCount = 0;
            (retryCount < CAMERA_MAX_ACK_FETCH_RETRY_COUNT) && (error == CAMERA_API_STATUS_FAILED);
            (retryCount++)) {

            /* Wait for the signal, for reception of first URL. */
            if (mGvcpSock.waitForReadyRead(CAMERA_WAIT_FOR_ACK_MS)) {

                /* Receive the next pending ack. */
                error = cameraFetchAck(ackHdr, reqId);
            }
        }
    }

    if ((error == CAMERA_API_STATUS_SUCCESS) && (ackHdr.cmdSpecificAckHdr != nullptr)) {

        /* Now we have first URL. */
        readMemAckHdr = (strGvcpAckMemReadHdr *)ackHdr.cmdSpecificAckHdr;

        /* Append data into the byte array, and return it. */
        byteArray.append(readMemAckHdr->data);

        /* Free memory allocated by ack header. */
        gvcpFreeAckMemory(ackHdr);
    }

    return error;
}

quint32 cameraApi::cameraXmlFetchAttrElement(const QString &attributeName, QDomNode& node) {
    quint32 error = CAMERA_API_STATUS_FAILED;
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
                    error = CAMERA_API_STATUS_SUCCESS;
                    break;
                }
            }

            /* Check to see if correct element has been found. */
            if (error == CAMERA_API_STATUS_SUCCESS) {

                break;
            }
        }
    }

    return error;
}

quint32 cameraApi::cameraReadRegisterValue(const QList<strGvcpCmdReadRegHdr> addressList, QList<quint32>& regValues) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
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
        error = CAMERA_API_STATUS_FAILED;
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Copy data into the array. */
        for (int counter = 0; counter < addressList.length(); counter++) {
            cmdSpecificData.append((char *)&addressList.at(counter), sizeof(strGvcpCmdReadRegHdr));
        }

        /* Send command to the specified address. */
        mVectorPendingReq.push_front(reqId);
        error = gvcpSendCmd(mGvcpSock, GVCP_READREG_CMD, cmdSpecificData, mCamIPAddr, GVCP_DEFAULT_UDP_PORT, reqId);
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Receive data in the array. */
        error = CAMERA_API_STATUS_FAILED;
        for (int retryCount = 0;
            (retryCount < CAMERA_MAX_ACK_FETCH_RETRY_COUNT) && (error == CAMERA_API_STATUS_FAILED);
            (retryCount++)) {

            /* Wait for the signal, for reception of first URL. */
            if (mGvcpSock.waitForReadyRead(CAMERA_WAIT_FOR_ACK_MS)) {

                /* Receive the next pending ack. */
                error = cameraFetchAck(ackHdr, reqId);
            }
        }
    }

    if ((error == CAMERA_API_STATUS_SUCCESS) && ackHdr.cmdSpecificAckHdr) {

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
quint32 cameraApi::cameraDiscoverDevice(const QHostAddress destAddr, strGvcpAckDiscoveryHdr& discAckHdr) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
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

quint32 cameraApi::cameraRequestResend(const strGvcpCmdPktResendHdr& cmdHdr) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
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

quint32 cameraApi::cameraStartStream(const quint16 streamHostPort)
{
    quint32 error;
    QList<QByteArray> values;
    QThread *streamWorker;
    PacketHandler *streamHandler;
    quint32 val;

    /* Write the destination address Gvsp packets. */
    val = qToBigEndian(mHostIPAddr.toIPv4Address());
    error = cameraWriteCameraAttribute(QList<QString>() << "GevSCDA",
                                       QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint32)));

    if (error == CAMERA_API_STATUS_SUCCESS) {

        val = qToBigEndian(CAMERA_GVSP_PAYLOAD_SIZE);
        error = cameraWriteCameraAttribute(QList<QString>() << "GevSCPSPacketSize",
                                           QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint32)));
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        error = cameraReadCameraAttribute(QList<QString>() << "GevSCPSPacketSize", values);
        mCamProps.streamPktSize = qFromBigEndian((quint16)byteArrayToUint32(values.first().left(2)));
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Write the port to listen to for Gvsp packets. */
        val = qToBigEndian(streamHostPort);
        error = cameraWriteCameraAttribute(QList<QString>() << "GevSCPHostPort",
                                           QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint16)));
        if (error == CAMERA_API_STATUS_SUCCESS) {

            /* Initialize the Host Gvcp UDP socket. */
            if (!(mGvspSock.bind(mHostIPAddr, streamHostPort))) {

                qDebug() << "Could not bind socket to GVSP ip/port.";
                error = CAMERA_API_STATUS_FAILED;

            } else {

                for (quint8 counter = 0; counter < CAMERA_MAX_WORKER_THREAD_COUNT; counter++) {
                    streamWorker = new QThread();
                    streamHandler = new PacketHandler(&mStreamHT, mCamProps.streamPktSize, &mMutex, &mPktResendBlockIDQueue, &mStreamReceiveQueue);

                    streamWorker->setObjectName("Stream Worker " + QString::number(counter));
                    streamHandler->moveToThread(streamWorker);

                    connect(this, &cameraApi::signalDatagramEnqueued, streamHandler, &PacketHandler::slotServicePendingPackets, Qt::QueuedConnection);
                    connect(streamHandler, &PacketHandler::signalRequestResend, this, &cameraApi::slotRequestResendRoutine, Qt::QueuedConnection);

                    streamWorker->start();

                    qDebug() << "Thread created with name = " << streamWorker;
                }

                /* Make relevant connections. */
                connect(&mGvspSock, &QUdpSocket::readyRead, this, &cameraApi::slotGvspReadyRead, Qt::DirectConnection);

                /* Move the socket to another thread and start. */
                mGvspSock.moveToThread(&mStreamingThread);
                mStreamingThread.start();
            }
        }
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {
        val = qToBigEndian(0x1);
        error = cameraWriteCameraAttribute(QList<QString>() << "AcquisitionStart",
                                           QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint32)));
    }

    return error;
}

quint32 cameraApi::cameraInitializeDevice() {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
    QList<QByteArray> value;
    quint32 val = 0;

    /* Initialize the Host Gvcp UDP socket. */
    if (!(mGvcpSock.bind(mHostIPAddr, mGvcpHostPort))) {

        qDebug() << "Could not bind socket to GVCP ip/port.";
        error =  CAMERA_API_STATUS_FAILED;
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Fetch camera XML. */
        error = cameraReadXmlFileFromDevice();
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Setting up heartbeat. */
        error = cameraReadCameraAttribute(QList<QString>() << "GevHeartbeatTimeout", value);
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Acquire control channel. */
        val = qToBigEndian(0x2);
        error = cameraWriteCameraAttribute(QList<QString>() << "GevCCP",
                                           QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint32)));
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

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

    if (error == CAMERA_API_STATUS_SUCCESS) {

        qDebug() << "Camera initialized successfully.";

        mCamProps.statusFlags |= CAMERA_STATUS_FLAGS_INITIALIZED;
    }

    return error;
}

quint32 cameraApi::cameraXmlFetchXmlFromDevice(const QByteArray fileName, const QByteArray startAddress,
                                            const QByteArray size, QByteArray& xmlData) {
    quint32 error;
    QByteArray retrievedXml;
    QNetworkDatagram datagram;
    QStringList list;
    QFile file(fileName), xmlFile;
    QDataStream stream(&file);
    const quint32 startAddressReadFromUrl = byteArrayToUint32(QByteArray::fromHex(startAddress));
    const quint32 sizeReadFromUrl = (quint32)byteArrayToUint32(QByteArray::fromHex(size));

    /* Retrieve data from the memory blockes where XML file is stored in the device. */
    error = cameraReadMemoryBlock(startAddressReadFromUrl, sizeReadFromUrl, retrievedXml);
    if (error == CAMERA_API_STATUS_SUCCESS) {
        if (file.open(QIODevice::ReadWrite)) {
            stream.setByteOrder(QDataStream::LittleEndian);
            stream << retrievedXml;
        }

        list = JlCompress::extractDir(fileName, fileName + "_extracted");
        if (list.isEmpty()) {

            error = CAMERA_API_STATUS_FAILED;

        } else {

            xmlFile.setFileName(list.first());
            if (xmlFile.open(QIODevice::ReadWrite)) {
                xmlData = xmlFile.readAll();
            }
        }
    }

    return error;
}
