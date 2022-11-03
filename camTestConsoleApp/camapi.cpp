#include "camapi.h"
#include "gvcp/gvcp.h"
#include "gvsp/gvsp.h"
#include "packethandler.h"
#include "quazip/JlCompress.h"

CamApi::CamApi(const QHostAddress hostIPv4Addr, QObject *parent)
    : QObject{parent}, mHostIP(hostIPv4Addr) {

    mCamProps.statusFlags = 0;
    mCamProps.streamChannelIdx = 0;
}

void CamApi::slotGvspReadyRead() {

    /* Keep receiving packets until there are pending datagrams. */
    while (mGvspSock.hasPendingDatagrams()) {

        /* Emit a signal with the received datagram. */
        emit signalDatagramEnqueued(mGvspSock.receiveDatagram());
    }
}

void CamApi::slotImageReceived(quint8 *dataPtr) {
    /* This function gives the data pointer to the received image buffer. */

#if 0
    static bool bla = false;
    static quint32 previous = 0;
    static QElapsedTimer timer;
    quint32 elapsed;

    if (!bla) {
        timer.restart();
        bla = true;
    } else {
        elapsed = timer.elapsed();
        qDebug() << "Time took to complete packet processing = " << elapsed - previous;
        previous = elapsed;
    }
#endif
}

void CamApi::slotCameraHeartBeat() {
    quint32 error;
    QList<QByteArray> values;
    quint32 val;

    /* Maintain heartbeat by sending sending a control message. */
    error = ReadCameraAttribute(QList<QString>() << "GevCCP", values);

    if (error != CAMERA_API_STATUS_SUCCESS) {

        /* Try to acquire control again. */
        val = qToBigEndian(0x2);
        error = WriteCameraAttribute(QList<QString>() << "GevCCP",
                                     QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint32)));

        if (error != CAMERA_API_STATUS_SUCCESS) {

            qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Failed to acquire control channel";
        }
    }
}

void CamApi::slotRequestResendRoutine(quint16 blockID) {
#if (CAMERA_API_ENABLE_RESEND == 1)
    strGvcpCmdPktResendHdr resendHdr;
    quint32 firstEmpty;
    quint32 packetIdx = 1;
    quint32 expectedNoOfPackets;
    strGvspImageDataLeaderHdr imgLeaderHdr;
    QHash<quint32, quint8*> frameHT;

    /* Retreive the frame from hash table. */
    mLock.lockForRead();
    if (mStreamHashTable.contains(blockID)) {
        frameHT = mStreamHashTable[blockID];
    }
    mLock.unlock();

    if (!frameHT.isEmpty()) {

        /* Extract image leader from the frame. */
        memcpy(&imgLeaderHdr, frameHT.value(0), sizeof(strGvspImageDataLeaderHdr));

        /* Calculate expected number of packets from the retrieved information. */
        expectedNoOfPackets = ((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) /
                               (mCamProps.streamPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE));
        ((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) % (mCamProps.streamPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)) ?
                    expectedNoOfPackets++ : expectedNoOfPackets;

        /* Adding 2 for header and trailer. */
        expectedNoOfPackets += 2;

        /* Run till (expectedNoOfPackets - 1) because index of packets starts from 0. */
        for (; packetIdx < (expectedNoOfPackets - 1); packetIdx++) {
            if (frameHT[packetIdx] == nullptr) {
                break;
            }
        }
        firstEmpty = packetIdx;

        /* Construct resend header. */
        resendHdr.blockIdRes = blockID;
        resendHdr.firstPktId = firstEmpty;
        resendHdr.lastPktId = expectedNoOfPackets - 1;
        resendHdr.streamChannelIdx = mCamProps.streamChannelIdx;

        qInfo() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Executing resend routine for block ID = " << resendHdr.blockIdRes
                << " from packet " << resendHdr.firstPktId << " to " << resendHdr.lastPktId;

        /* Transmit a request to resubmit. */
        RequestResend(resendHdr);
    }
#endif
}

quint32 CamApi::XmlFetchChildElementValue(const QDomNode& parent, const QString& tagName, QString& value) {
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

quint8 CamApi::StatusFlags() const {
    return mCamProps.statusFlags;
}

quint32 byteArrayToUint32(const QByteArray& bytes) {
    QByteArray byteArray;
    quint32 number = 0;

    /* Take only the first four bytes from byte array. */
    if (bytes.count() > 4) {
        byteArray = bytes.mid(bytes.count() - 4);
    } else {
        byteArray = bytes;
    }

    /* Push bytes to the "number" variable. */
    QDataStream stream(byteArray);
    for (ulong var = byteArray.size(); var > 0; var--) {
        stream >> *((quint8 *)&number + (var - 1));
    }

    return number;
}

quint32 CamApi::ReadXmlFileFromDevice() {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
    QList<QByteArray> url;
    QNetworkDatagram datagram;
    QByteArray first_url;
    QByteArray xml_data;

    /* Fetch first URL. */
    error = ReadMemoryBlock((quint32)DEVICE_FIRST_URL_ADDRESS, (quint16)DEVICE_URL_ADDRESS_REG_LENGTH, first_url);

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Identify the location in which XML file is placed. */
        if (first_url.contains("Local:")) {

            /* Split the URL using standard delimiter. */
            url = first_url.mid(0, first_url.indexOf('\0')).split(';');

            if ((error == CAMERA_API_STATUS_SUCCESS) && (url.count() == 3)) {

                /* Fetch the XML file from camera. */
                error = XmlFetchXmlFromDevice(url[0], url[1], url[2], xml_data);
            } else {

                qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Correct URL is not found.";
                error = CAMERA_API_STATUS_FAILED;
            }

        } else if (first_url.contains("http:")) {

            qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Unsupported location of XML file.";
            error = CAMERA_API_STATUS_FAILED;
        } else {

            qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Unsupported location of XML file.";
            error = CAMERA_API_STATUS_FAILED;
        }
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Set content of the document. */
        if(!mCamXmlFile.setContent(xml_data)) {

            qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Unable to correctly parse the xml file fetched from device.";
            error = CAMERA_API_STATUS_FAILED;
        }
    }

    return error;
}

/* This function reads an attribute from the device using XML which was previously fetched. */
quint32 CamApi::ReadCameraAttribute(const QList<QString>& attributeList, QList<QByteArray>& registerValues) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
    QDomElement root;
    QDomNodeList tempList;
    QDomNode tempNode;
    quint32 address = 0, length = 0;
    QString tempStr;
    QByteArray tempData;
    strGvcpCmdReadRegHdr tempHdr;
    QList<quint32> tempRegisterVal;

    if (!(mCamProps.statusFlags & CAMERA_STATUS_FLAGS_INITIALIZED)) {

        error = CAMERA_API_STATUS_CAMERA_UNINITIALIZED;
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Check to see if this function is provided with a valid XML file. */
        if (!mCamXmlFile.isDocument()) {

            qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "XML file for the camera is not a valid file.";
            error = CAMERA_API_STATUS_FAILED;
        }
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Traverse through the list and find the address. */
        for (int var = 0; var < attributeList.count() && !error; var++) {

            /* Fetch the register containing address and length. */
            error = XmlFetchAttrRegister(attributeList.at(var), tempNode);

            if (error == CAMERA_API_STATUS_SUCCESS) {

                /* Fetch length of the register. */
                error = XmlFetchChildElementValue(tempNode, "Address", tempStr);
            }

            if (error == CAMERA_API_STATUS_SUCCESS) {
                /* Convert address to uint32. */
                address = byteArrayToUint32(QByteArray::fromHex(QByteArray::fromStdString(tempStr.toStdString())));

                /* Fetch length of the register. */
                error = XmlFetchChildElementValue(tempNode, "Length", tempStr);
            }

            if (error == CAMERA_API_STATUS_SUCCESS) {

                /* Convert length to uint. */
                length = tempStr.toInt();

                if (length > sizeof(quint32)) {
                    /* If length is greater than 4, then its a memory block and not a register. */
                    error = ReadMemoryBlock(address, length, tempData);

                } else {

                    /* Set the header. */
                    tempHdr.registerAddress = address;
                    tempRegisterVal.clear();

                    /* Read register value. */
                    error = ReadRegisterValue(QList<strGvcpCmdReadRegHdr>() << tempHdr, tempRegisterVal);

                    if (error == CAMERA_API_STATUS_SUCCESS) {

                        if (tempRegisterVal.count() != 0) {

                            /* Store register value. */
                            tempData.append((char *)&tempRegisterVal.at(0), sizeof(quint32));
                        } else {

                            qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Data not received from attribute " << attributeList.at(var);
                        }
                    }
                }

                if (error == CAMERA_API_STATUS_SUCCESS) {

                    registerValues.append(tempData);
                }
            }
        }
    }

    return error;
}

quint32 CamApi::XmlFetchAttrRegister(const QString& nodeName, QDomNode& regNode) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
    QString tempStr;

    /* Fetch the element with specified name. */
    error = XmlFetchAttrElement(nodeName, regNode);

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Fetch register name from this element. */
        error = XmlFetchChildElementValue(regNode, "pValue", tempStr);
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Fetch the register element. */
        error = XmlFetchAttrElement(tempStr, regNode);
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        if ((regNode.toElement().nodeName() == "IntConverter") ||
                (regNode.toElement().nodeName() == "Integer")) {

            /* Fetch register name from this element. */
            error = XmlFetchChildElementValue(regNode, "pValue", tempStr);

            if (error == CAMERA_API_STATUS_SUCCESS) {

                /* Fetch the register element. */
                error = XmlFetchAttrElement(tempStr, regNode);
            }

        } else if (regNode.toElement().nodeName() == "Converter") {

            /* Fetch register name from this element. */
            error = XmlFetchChildElementValue(regNode, "pValue", tempStr);

            if (error == CAMERA_API_STATUS_SUCCESS) {

                /* Fetch the register element. */
                error = XmlFetchAttrElement(tempStr, regNode);
            }
        }
    }

    return error;
}

quint32 CamApi::WriteCameraAttribute(const QList<QString>& attributeList, const QList<QByteArray>& regValues) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
    QDomElement root;
    QDomNodeList tempList;
    QDomNode tempNode;
    quint32 address = 0, length = 0;
    QString tempStr;
    strGvcpCmdWriteRegHdr writeUnit;

    if (!(mCamProps.statusFlags & CAMERA_STATUS_FLAGS_INITIALIZED)) {

        error = CAMERA_API_STATUS_CAMERA_UNINITIALIZED;
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Check to see if this function is provided with a valid XML file. */
        if (!mCamXmlFile.isDocument()) {

            qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "XML file for the camera is not a valid file.";
            error = CAMERA_API_STATUS_FAILED;
        }
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        if (attributeList.count() != regValues.count()) {

            qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Not all the values against each attribute is provided.";
            error = CAMERA_API_STATUS_FAILED;
        }
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Traverse through the list and find the address. */
        for (int var = 0; var < attributeList.count() && !error; var++) {

            /* Fetch the register containing address and length. */
            error = XmlFetchAttrRegister(attributeList.at(var), tempNode);

            if (error == CAMERA_API_STATUS_SUCCESS) {

                /* Fetch length of the register. */
                error = XmlFetchChildElementValue(tempNode, "Address", tempStr);
            }

            if (error == CAMERA_API_STATUS_SUCCESS) {

                /* Convert address to uint32. */
                address = byteArrayToUint32(QByteArray::fromHex(QByteArray::fromStdString(tempStr.toStdString())));

                /* Fetch length of the register. */
                error = XmlFetchChildElementValue(tempNode, "Length", tempStr);
            }

            if (error == CAMERA_API_STATUS_SUCCESS) {

                /* Convert length to uint. */
                length = tempStr.toInt();

                if (length > sizeof(quint32)) {

                    qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Not yet supported to write memory greater than " << sizeof(quint32);
                } else {

                    /* Set the header. */
                    writeUnit.registerAddress = address;
                    writeUnit.registerData = byteArrayToUint32(regValues.at(var));

                    /* Read register value. */
                    error = WriteRegisterValue(QList<strGvcpCmdWriteRegHdr>() << writeUnit);
                    if (error == CAMERA_API_STATUS_FAILED) {

                        qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Unable to write register at address: " << writeUnit.registerAddress;
                    }
                }
            }
        }
    }

    return error;
}

quint32 CamApi::WriteRegisterValue(const QList<strGvcpCmdWriteRegHdr>& writeUnits) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
    QByteArray cmdSpecificData;
    strNonStdGvcpAckHdr ackHdr;
    QNetworkDatagram datagram;
    quint16 reqId = (QDateTime::currentMSecsSinceEpoch() % 50) + 1;

    if (!(mCamProps.statusFlags & CAMERA_STATUS_FLAGS_INITIALIZED)) {

        error = CAMERA_API_STATUS_CAMERA_UNINITIALIZED;
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Initialize memory to 0. */
        memset(&ackHdr, 0x00, sizeof(strNonStdGvcpAckHdr));
        cmdSpecificData.clear();

        if (writeUnits.isEmpty()) {

            qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "No data is provided in the array to write.";
            error = CAMERA_API_STATUS_FAILED;
        }
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Copy data into the array. */
        for (int counter = 0; counter < writeUnits.length(); counter++) {
            cmdSpecificData.append((char *)&writeUnits.at(counter), sizeof(strGvcpCmdWriteRegHdr));
        }

        /* Send command to the specified address. */
        error = gvcpSendCmd(mGvcpSock, GVCP_WRITEREG_CMD, cmdSpecificData, mCamIP, GVCP_DEFAULT_UDP_PORT, reqId);
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
                error = FetchAck(ackHdr, reqId);
            }
        }
    }

    if ((error == CAMERA_API_STATUS_SUCCESS) && ackHdr.cmdSpecificAckHdr) {

        /* Free memory allocated by ack header. */
        gvcpFreeAckMemory(ackHdr);
    }

    return error;
}

quint32 CamApi::FetchAck(strNonStdGvcpAckHdr& ackHdr, const quint16 reqId)
{
    quint32 error;

    /* Receive the next pending ack. */
    error = gvcpReceiveAck(mGvcpSock, ackHdr);

    /* Any failure found? */
    if ((ackHdr.genericAckHdr.ackId != reqId) || (error != CAMERA_API_STATUS_SUCCESS)) {

        /* Free memory space acquired by command specific ack header for packed with undesired req_id. */
        gvcpFreeAckMemory(ackHdr);
    }

    return error;
}

quint32 CamApi::ReadMemoryBlock(const quint32 address, const quint16 size, QByteArray& returnedData) {
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
        error = gvcpSendCmd(mGvcpSock, GVCP_READMEM_CMD, cmdSpecificData, mCamIP, GVCP_DEFAULT_UDP_PORT, reqId);
        if (error == CAMERA_API_STATUS_SUCCESS) {

            /* Receive data in the array. */
            error = CAMERA_API_STATUS_FAILED;
            for (retry_count = 0;
                 (retry_count < CAMERA_MAX_ACK_FETCH_RETRY_COUNT) && (error == CAMERA_API_STATUS_FAILED);
                 (retry_count++)) {

                /* Wait for the signal, for reception of first URL. */
                if (mGvcpSock.waitForReadyRead(CAMERA_WAIT_FOR_ACK_MS)) {

                    /* Receive the next pending ack. */
                    error = FetchAck(ackHdr, reqId);
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

        qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Corrupted block. Could not fetch the file correctly.";
    }

    return error;
}

quint32 CamApi::XmlFetchAttrElement(const QString &attributeName, QDomNode& node) {
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

quint32 CamApi::ReadRegisterValue(const QList<strGvcpCmdReadRegHdr>& addressList, QList<quint32>& regValues) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
    QByteArray cmdSpecificData;
    strNonStdGvcpAckHdr ackHdr;
    strGvcpAckRegReadHdr *regReadAckHdr;
    QNetworkDatagram datagram;
    quint16 reqId = (QDateTime::currentMSecsSinceEpoch() % 50) + 1;

    if (!(mCamProps.statusFlags & CAMERA_STATUS_FLAGS_INITIALIZED)) {

        error = CAMERA_API_STATUS_CAMERA_UNINITIALIZED;
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Initialize memory to 0. */
        memset(&ackHdr, 0x00, sizeof(*(&ackHdr)));
        cmdSpecificData.clear();

        /* Check to see if the address list is empty. */
        if (addressList.empty()) {

            qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Feature list is empty";
            error = CAMERA_API_STATUS_FAILED;
        }
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Copy data into the array. */
        for (int counter = 0; counter < addressList.length(); counter++) {
            cmdSpecificData.append((char *)&addressList.at(counter), sizeof(strGvcpCmdReadRegHdr));
        }

        /* Send command to the specified address. */
        error = gvcpSendCmd(mGvcpSock, GVCP_READREG_CMD, cmdSpecificData, mCamIP, GVCP_DEFAULT_UDP_PORT, reqId);
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
                error = FetchAck(ackHdr, reqId);
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
quint32 CamApi::DiscoverDevice(const QHostAddress& destAddr, QList<strGvcpAckDiscoveryHdr>& discAckHdr) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
    strNonStdGvcpAckHdr ackHdr;
    strGvcpAckDiscoveryHdr *ptr;
    QByteArray cmdSpecificData;
    quint16 reqId = (QDateTime::currentMSecsSinceEpoch() % 50) + 1;

    if (!mGvcpSock.isValid()) {

        /* Initialize the Host Gvcp UDP socket. */
        if (!(mGvcpSock.bind(CAMERA_GVCP_BIND_PORT))) {

            qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Could not bind socket to GVCP ip/port.";
            error =  CAMERA_API_STATUS_FAILED;
        }
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Initialize memory to 0. */
        memset(&ackHdr, 0x00, sizeof(strNonStdGvcpAckHdr));
        cmdSpecificData.clear();

        /* Send command to the specified address. */
        error = gvcpSendCmd(mGvcpSock, GVCP_DISCOVERY_CMD, cmdSpecificData, destAddr, GVCP_DEFAULT_UDP_PORT, reqId);
    }

    while (error == CAMERA_API_STATUS_SUCCESS) {

        /* Wait for the signal. */
        if (mGvcpSock.waitForReadyRead(CAMERA_WAIT_FOR_ACK_MS)) {

            /* Receive the next pending ack. */
            error = gvcpReceiveAck(mGvcpSock, ackHdr);

            if (error == CAMERA_API_STATUS_SUCCESS) {

                ptr = (strGvcpAckDiscoveryHdr *)ackHdr.cmdSpecificAckHdr;
                discAckHdr.append(*ptr);
                gvcpFreeAckMemory(ackHdr);
            }

        } else {

            break;
        }
    }

    return error;
}

quint32 CamApi::RequestResend(const strGvcpCmdPktResendHdr& cmdHdr) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
    QByteArray cmdSpecificData;
    quint16 reqId = (QDateTime::currentMSecsSinceEpoch() % 50) + 1;

    /* Initialize memory to 0. */
    cmdSpecificData.clear();

    /* Append command specific header. */
    cmdSpecificData.append((char *)&cmdHdr, sizeof(strGvcpCmdPktResendHdr));

    /* Send command to the specified address. */
    error = gvcpSendCmd(mGvcpSock, GVCP_PACKETRESEND_CMD, cmdSpecificData, mCamIP, GVCP_DEFAULT_UDP_PORT, reqId);

    return error;
}

quint32 CamApi::StartStream() {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
    QList<QByteArray> values;
    quint8 *tempPtr;
    quint32 val;

    if (!(mCamProps.statusFlags & CAMERA_STATUS_FLAGS_INITIALIZED)) {

        error = CAMERA_API_STATUS_CAMERA_UNINITIALIZED;
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Write the destination address Gvsp packets. */
        val = qToBigEndian(mHostIP.toIPv4Address());
        error = WriteCameraAttribute(QList<QString>() << "GevSCDA",
                                     QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint32)));
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        val = qToBigEndian(CAMERA_GVSP_PAYLOAD_SIZE);
        error = WriteCameraAttribute(QList<QString>() << "GevSCPSPacketSize",
                                     QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint32)));
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        error = ReadCameraAttribute(QList<QString>() << "GevSCPSPacketSize", values);
        mCamProps.streamPktSize = qFromBigEndian((quint16)byteArrayToUint32(values.first().left(2)));
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        values.clear();
        error = ReadCameraAttribute(QList<QString>() << "Width", values);
        mCamProps.imageWidth = qFromBigEndian((quint32)byteArrayToUint32(values.first().left(4)));
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        values.clear();
        error = ReadCameraAttribute(QList<QString>() << "Height", values);
        mCamProps.imageHeight = qFromBigEndian((quint32)byteArrayToUint32(values.first().left(4)));
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Initialize the Host Gvcp UDP socket. */
        if (!(mGvspSock.bind(mHostIP, CAMERA_GVSP_BIND_PORT))) {

            qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Could not bind socket to GVSP ip/port.";
            error = CAMERA_API_STATUS_FAILED;

        } else {

            /* Write the port to listen to for Gvsp packets. */
            val = qToBigEndian(mGvspSock.localPort());
            error = WriteCameraAttribute(QList<QString>() << "GevSCPHostPort",
                                         QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint16)));

            for (quint8 counter = 0; counter < CAMERA_MAX_FRAME_BUFFER_SIZE; counter++) {
                tempPtr = new quint8[((mCamProps.imageHeight * mCamProps.imageWidth)) +
                                       sizeof(strGvspImageDataLeaderHdr) +
                                       sizeof(strGvspImageDataTrailerHdr)];
                mStreamPreAllocatedBuffers.append(tempPtr);
            }

            mStreamConsumerObject = new PacketHandler(&mStreamHashTable, mCamProps.streamPktSize, &mLock,
                                                      mCamProps.imageHeight * mCamProps.imageWidth,
                                                      mStreamPreAllocatedBuffers);

            connect(mStreamConsumerObject, &PacketHandler::signalRequestResend, this, &CamApi::slotRequestResendRoutine, Qt::QueuedConnection);
            connect(mStreamConsumerObject, &PacketHandler::signalImageAcquisitionComplete, this, &CamApi::slotImageReceived, Qt::QueuedConnection);

#if (CAMERA_API_ENABLE_DISJOINT_PARCER == 1)
            mStreamConsumerThread = new QThread();
            mStreamConsumerThread->setObjectName("Stream Worker");
            mStreamConsumerObject->moveToThread(mStreamConsumerThread);
            mStreamConsumerThread->start();
            connect(this, &CamApi::signalDatagramEnqueued, mStreamConsumerObject, &PacketHandler::slotServicePendingPackets, Qt::QueuedConnection);
#else
            connect(this, &CamApi::signalDatagramEnqueued, mStreamConsumerObject, &PacketHandler::slotServicePendingPackets, Qt::DirectConnection);
#endif
            /* Make relevant connections. */
            connect(&mGvspSock, &QUdpSocket::readyRead, this, &CamApi::slotGvspReadyRead, Qt::DirectConnection);

            /* Move the socket to another thread and start. */
            mGvspSock.moveToThread(&mStreamProducerThread);
            mStreamProducerThread.start();
        }
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {
        val = qToBigEndian(0x1);
        error = WriteCameraAttribute(QList<QString>() << "AcquisitionStart",
                                     QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint32)));
    }

    return error;
}

quint32 CamApi::StopStream() {
    quint32 error;
    quint32 val;

    val = qToBigEndian(0x1);
    error = WriteCameraAttribute(QList<QString>() << "AcquisitionStop",
                                 QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint32)));

#if (CAMERA_API_ENABLE_DISJOINT_PARCER == 1)
    mStreamConsumerThread->exit();
    while(mStreamConsumerThread->isRunning());
    delete(mStreamConsumerThread);
#endif

    delete(mStreamConsumerObject);

    if (mStreamHashTable.count() > 0) {
        mStreamHashTable.clear();
    }

    return error;
}

quint32 CamApi::InitializeDevice(const QHostAddress& camIP) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
    QList<QByteArray> value;
    quint32 val = 0;

    /* Initialize the status of camera to 0. */
    mCamProps.statusFlags = 0;
    mCamProps.statusFlags |= CAMERA_STATUS_FLAGS_INITIALIZED;
    mCamIP = camIP;
    QThread::currentThread()->setObjectName("Control Thread");
    mStreamProducerThread.setObjectName("Streaming Thread");

    if (!mGvcpSock.isValid()) {

        /* Initialize the Host Gvcp UDP socket. */
        if (!(mGvcpSock.bind(CAMERA_GVCP_BIND_PORT))) {

            qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Could not bind socket to GVCP ip/port.";
            error =  CAMERA_API_STATUS_FAILED;
        }
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Fetch camera XML. */
        error = ReadXmlFileFromDevice();
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Acquire control channel. */
        val = qToBigEndian(0x2);
        error = WriteCameraAttribute(QList<QString>() << "GevCCP",
                                     QList<QByteArray>() << QByteArray::fromRawData((char *)&val, sizeof(quint32)));
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        /* Setting up heartbeat. */
        error = ReadCameraAttribute(QList<QString>() << "GevHeartbeatTimeout", value);
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        val = qFromBigEndian(byteArrayToUint32(value.first()));

        /* Set interval to the value fetched from camera. */
        mHeartBeatTimer.setInterval(val/8);

        /* Make relevant connections. */
        connect(&mHeartBeatTimer, &QTimer::timeout, this, &CamApi::slotCameraHeartBeat, Qt::ConnectionType::DirectConnection);

        /* Fire off the timer. */
        mHeartBeatTimer.start();
    }

    if (error == CAMERA_API_STATUS_SUCCESS) {

        qInfo() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Camera initialized successfully.";
    } else {

        mCamProps.statusFlags &= ~CAMERA_STATUS_FLAGS_INITIALIZED;
    }

    return error;
}

quint32 CamApi::XmlFetchXmlFromDevice(const QByteArray& fileName, const QByteArray& startAddress,
                                      const QByteArray size, QByteArray& xmlData) {
    quint32 error;
    QByteArray retrievedXml;
    QNetworkDatagram datagram;
    QStringList list;
    QFile file(fileName), xmlFile;
    QDataStream stream(&file);
    const quint32 startAddressReadFromUrl = byteArrayToUint32(QByteArray::fromHex(startAddress));
    const quint32 sizeReadFromUrl = (quint32)byteArrayToUint32(QByteArray::fromHex(size));

    /* Retrieve data from the memory blocks where XML file is stored in the device. */
    error = ReadMemoryBlock(startAddressReadFromUrl, sizeReadFromUrl, retrievedXml);
    if (error == CAMERA_API_STATUS_SUCCESS) {
        if (file.open(QIODevice::ReadWrite)) {
            stream.setByteOrder(QDataStream::LittleEndian);
            stream << retrievedXml;
        }

        /* Extract the fetched xml file which is in zip format. */
        list = JlCompress::extractDir(fileName, fileName + "_extracted");
        if (list.isEmpty()) {

            error = CAMERA_API_STATUS_FAILED;

        } else {

            /* Set the name for new XML file. */
            xmlFile.setFileName(list.first());
            if (xmlFile.open(QIODevice::ReadWrite)) {
                xmlData = xmlFile.readAll();
            }
        }
    }

    return error;
}
