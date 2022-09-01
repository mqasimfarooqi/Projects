#include "cameraapi.h"
#include "gvcp/gvcp.h"
#include "quazip/JlCompress.h"

cameraApi::cameraApi(QObject *parent) : QObject{parent}{}
cameraApi::cameraApi(QUdpSocket *udpSocket, QVector<quint8> *pendingReqVector)
{
    mUdpSock = udpSocket;
    mVectorPendingReq = pendingReqVector;
}

void cameraApi::slotReadyRead()
{

    /* Emit the signal taht the ack has been received for a pending req_id. */
    emit signalAckReceived();
}

bool fetchChildElementValue(const QDomNode& parent, const QString& tagName, QString& value) {
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

quint32 byteArrayToAddr(const QByteArray& bytes)
{
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

bool cameraApi::cameraReadXmlFileFromDevice(QDomDocument& xmlFile, const QHostAddress& destAddr)
{
    bool error = false;
    QList<QByteArray> url;
    QNetworkDatagram datagram;
    QByteArray first_url;
    QByteArray xml_data;

    if (!mUdpSock->isValid()) {
        error = true;
        qDebug() << "Error: Invalid UDP socket pointer";
    }

    if (!error) {

        /* Make relevant connections. */
        connect(mUdpSock, &QUdpSocket::readyRead, this, &cameraApi::slotReadyRead, Qt::DirectConnection);

        /* Fetch first URL. */
        error = cameraFetchFirstUrl(destAddr, first_url);
    }

    if (!error) {

        /* Identify the location in which XML file is placed. */
        if (first_url.contains("Local:")) {

            /* Split the URL using standard delimiter. */
            url = first_url.mid(0, first_url.indexOf('\0')).split(';');

            if (!error && (url.count() == 3)) {
                error = cameraFetchXmlFromDevice(url[0], url[1], url[2], destAddr, xml_data);
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
        if(!xmlFile.setContent(xml_data)) {

            qDebug() << "Error: Unable to correctly parse the xml file fetched from device.";
            error = true;
        }
    }

    return (error);
}

/* This function reads an attribute from the device using XML which was previously fetched. */
bool cameraApi::cameraReadCameraAttribute(const QList<QString>& attributeList, const QDomDocument& xmlFile,
                                          const QHostAddress destAddr, QList<QByteArray>& registerValues)
{
    bool error = false;
    QDomElement root;
    QDomNodeList tempList;
    QDomNode tempNode;
    quint32 address = 0, length = 0;
    QString tempStr;
    strGvcpCmdReadRegHdr tempHdr;
    QList<quint32> tempRegisterVal;
    QList<quint32> featureAddressList;
    QMap<QString, QDomNodeList> camFeatures;

    /* Initialize list. */
    featureAddressList.clear();

    /* Check to see if this function is provided with a valid XML file. */
    if (!xmlFile.isDocument()) {

        error = true;
    }

    if (!error) {

        /* Start with the first child element. */
        root = xmlFile.firstChildElement();

        /* Parse the XML and get all the nodes with following tags. */
        for (int var = 0; var < lookupTags.count(); var++) {
            tempList = root.elementsByTagName(lookupTags.at(var));
            camFeatures.insert(lookupTags.at(var), tempList);
        }

        /* Traverse through the list and find the address. */
        for (int var = 0; var < attributeList.count() && !error; var++) {

            /* Fetch the element with specified name. */
            error = cameraXmlFetchAttrElement(attributeList.at(var), camFeatures, tempNode);

            if (!error) {
                /* Fetch the child element of node with the name "Address" */
                error = fetchChildElementValue(tempNode, "Address", tempStr);

                if (!error) {
                    /* Convert address to uint32. */
                    address = byteArrayToAddr(QByteArray::fromHex(QByteArray::fromStdString(tempStr.toStdString())));

                    /* Fetch length of the register. */
                    error = fetchChildElementValue(tempNode, "Length", tempStr);

                    if (!error) {
                        /* Convert length to uint. */
                        length = tempStr.toInt();
                    }
                }

                if (!error) {
                    QByteArray tempData;

                    if (length > sizeof(quint32)) {
                        /* If length is greater than 4, then its a memory block and not a register. */
                        error = cameraReadMemoryBlock(address, length, destAddr, tempData);

                        if (!error) {
                            registerValues.append(tempData);
                        }
                    } else {

                        /* Set the header. */
                        tempHdr.registerAddress = address;
                        tempRegisterVal.clear();

                        /* Read register value. */
                        error = cameraReadRegisterValue(destAddr, QList<strGvcpCmdReadRegHdr>() << tempHdr, tempRegisterVal);

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

bool cameraApi::cameraWriteRegisterValue(const QHostAddress &destAddr, const QList<strGvcpCmdWriteRegHdr>& writeUnits)
{
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
        mVectorPendingReq->push_front(reqId);
        error = gvcpSendCmd(mUdpSock, GVCP_WRITEREG_CMD, cmdSpecificData, destAddr, GVCP_DEFAULT_UDP_PORT, reqId);
    }

    if (!error) {

        /* Receive data in the array. */
        error = true;
        for (int retryCount = 0;
            (retryCount < MAX_ACK_FETCH_RETRY_COUNT) && error;
            (retryCount++)) {

            /* Wait for the signal, for reception of first URL. */
            if (mUdpSock->waitForReadyRead(100)) {

                /* Receive the next pending ack. */
                error = cameraFetchAck(ackHdr, reqId);
            }
        }
    }

    if (!error && ackHdr.cmdSpecificAckHdr) {

        /* Free memory allocated by ack header. */
        gvcpHelperFreeAckMemory(ackHdr);
    }

    return error;
}

bool cameraApi::cameraFetchAck(strNonStdGvcpAckHdr& ackHdr, const quint16 reqId)
{
    bool error = true;

    /* Receive the next pending ack. */
    error = gvcpReceiveAck(mUdpSock, ackHdr);

    /* If correct ack is received */
    if ((ackHdr.genericAckHdr.ackId == reqId) && !error) {

        /* Remove the pending request ID stored in vector. */
        mVectorPendingReq->removeOne(ackHdr.genericAckHdr.ackId);
        mVectorPendingReq->squeeze();

        error = false;

    } else {

        if (ackHdr.cmdSpecificAckHdr) {

            /* Free memory space acquired by command specific ack header for packed with undesired req_id. */
            gvcpHelperFreeAckMemory(ackHdr);
        }
    }

    return error;
}

bool cameraApi::cameraReadMemoryBlock(const quint32 address, const quint16 size,
                                       const QHostAddress destAddr, QByteArray& returnedData)
{
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
        mVectorPendingReq->push_front(reqId);
        error = gvcpSendCmd(mUdpSock, GVCP_READMEM_CMD, cmdSpecificData, destAddr, GVCP_DEFAULT_UDP_PORT, reqId);

        if (!error) {

            /* Receive data in the array. */
            error = true;
            for (retry_count = 0;
                (retry_count < MAX_ACK_FETCH_RETRY_COUNT) && error;
                (retry_count++)) {

                /* Wait for the signal, for reception of first URL. */
                if (mUdpSock->waitForReadyRead(100)) {

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

                gvcpHelperFreeAckMemory(ackHdr);
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

bool cameraApi::cameraFetchFirstUrl(const QHostAddress& destAddr, QByteArray& byteArray)
{
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
    mVectorPendingReq->push_front(reqId);
    error = gvcpSendCmd(mUdpSock, GVCP_READMEM_CMD, cmdSpecificData, destAddr, GVCP_DEFAULT_UDP_PORT, reqId);

    if (!error) {

        /* Receive data in the array. */
        error = true;
        for (int retryCount = 0;
            (retryCount < MAX_ACK_FETCH_RETRY_COUNT) && error;
            (retryCount++)) {

            /* Wait for the signal, for reception of first URL. */
            if (mUdpSock->waitForReadyRead(100)) {

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
        gvcpHelperFreeAckMemory(ackHdr);
    }

    return error;
}

bool cameraApi::cameraXmlFetchAttrElement(const QString& attributeName, const QMap<QString, QDomNodeList>& camFeatures, QDomNode& node)
{
    bool error = true;

    /* Traverse through the list and find all the elements with specified tags. */
    for (int var = 0; var < camFeatures.count(); var++) {
        QDomNodeList tempList = camFeatures.value(lookupTags.at(var));
        for (int counter = 0; counter < tempList.count(); counter++) {
            if(!QString::compare(tempList.at(counter).attributes().namedItem("Name").nodeValue(), attributeName)) {
                node = tempList.at(counter);
                error = false;
                break;
            }
        }
    }

    return error;
}

bool cameraApi::cameraReadRegisterValue(const QHostAddress &destAddr, const QList<strGvcpCmdReadRegHdr> addressList, QList<quint32>& regValues)
{
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
        mVectorPendingReq->push_front(reqId);
        error = gvcpSendCmd(mUdpSock, GVCP_READREG_CMD, cmdSpecificData, destAddr, GVCP_DEFAULT_UDP_PORT, reqId);
    }

    if (!error) {

        /* Receive data in the array. */
        error = true;
        for (int retryCount = 0;
            (retryCount < MAX_ACK_FETCH_RETRY_COUNT) && error;
            (retryCount++)) {

            /* Wait for the signal, for reception of first URL. */
            if (mUdpSock->waitForReadyRead(100)) {

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
        gvcpHelperFreeAckMemory(ackHdr);
    }

    return error;
}

/* This function discovers the device and populates the ack header. */
bool cameraApi::cameraDiscoverDevice(const QHostAddress& addr, strGvcpAckDiscoveryHdr& discAckHdr)
{
    bool error = false;
    strNonStdGvcpAckHdr ackHdr;
    QByteArray cmdSpecificData;
    quint16 reqId = (QDateTime::currentMSecsSinceEpoch() % 50) + 1;

    /* Just a safety check. */
    if (!mUdpSock->isValid()) {
        error = true;
        qDebug() << "Error: Invalid UDP socket pointer";
    }

    if (!error) {

        /* Make relevant connections. */
        connect(mUdpSock, &QUdpSocket::readyRead, this, &cameraApi::slotReadyRead, Qt::DirectConnection);

        /* Initialize memory to 0. */
        memset(&ackHdr, 0x00, sizeof(strNonStdGvcpAckHdr));
        cmdSpecificData.clear();

        /* Send command to the specified address. */
        mVectorPendingReq->push_front(reqId);
        error = gvcpSendCmd(mUdpSock, GVCP_DISCOVERY_CMD, cmdSpecificData, addr, GVCP_DEFAULT_UDP_PORT, reqId);
    }

    if (!error) {

        /* Try to fetch the replies for MAX_ACK_FETCH_RETRY_COUNT times. */
        error = true;
        for (int retryCount = 0;
            (retryCount < MAX_ACK_FETCH_RETRY_COUNT) && error;
            (retryCount++)) {

            /* Wait for the signal,. */
            if (mUdpSock->waitForReadyRead(100)) {

                /* Receive the next pending ack. */
                error = cameraFetchAck(ackHdr, reqId);
            }
        }
    }

    if (!error) {

        strGvcpAckDiscoveryHdr *ptr = (strGvcpAckDiscoveryHdr *)ackHdr.cmdSpecificAckHdr;
        memcpy(&discAckHdr, ptr, sizeof(strGvcpAckDiscoveryHdr));
        gvcpHelperFreeAckMemory(ackHdr);

    } else {

        qDebug() << "Error: Acknowledgement packet is not received.";
    }

    /* Disconnect before exiting. */
    mUdpSock->disconnect();
    this->disconnect();

    return error;
}

bool cameraApi::cameraFetchXmlFromDevice(const QByteArray fileName, const QByteArray startAddress,
                                         const QByteArray size, const QHostAddress destAddr,
                                         QByteArray& xmlData)
{
    bool error = false;
    QByteArray retrievedXml;
    QNetworkDatagram datagram;
    QStringList list;
    QFile file(fileName), xmlFile;
    QDataStream stream(&file);
    const quint32 startAddressReadFromUrl = byteArrayToAddr(QByteArray::fromHex(startAddress));
    const quint16 sizeReadFromUrl = byteArrayToAddr(QByteArray::fromHex(size));

    /* Retrieve data from the memory blockes where XML file is stored in the device. */
    error = cameraReadMemoryBlock(startAddressReadFromUrl, sizeReadFromUrl, destAddr, retrievedXml);
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
