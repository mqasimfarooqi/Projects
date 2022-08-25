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

quint32 byteArrayToUint32(const QByteArray &bytes)
{
    auto count = bytes.size();
    if (count == 0 || count > 4) {
        return 0;
    }
    quint32 number = 0U;
    for (int i = 0; i < count; ++i) {
        auto b = static_cast<quint8>(bytes[count - 1 - i]);
        number += static_cast<quint32>(b << (8 * i));
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
                                          const QHostAddress destAddr, QList<quint32>& registerValues)
{
    bool error = false;
    bool featureFound;
    QDomElement root;
    QList<quint32> featureAddressList;

    /* Initialize list. */
    featureAddressList.clear();

    /* Check to see if this function is provided with a valid XML file. */
    if (xmlFile.isDocument()) {

        /* Start with the first child element. */
        root = xmlFile.firstChildElement();

        /* Fetch all elements which have an attribute named address. */
        QDomNodeList items = root.elementsByTagName("Address");

        /* This for loops simply goes through the list of features that need to be fetched. */
        for (quint16 featureCounter = 0; featureCounter < attributeList.count(); featureCounter++) {

            /* Change the status to feature not found here. */
            featureFound = false;

            /* Find the specific feature out of all the features fetched from XML file.  */
            for (int iterator = 0; iterator < items.count(); iterator++) {

                /* Store the current element in a buffer. */
                QDomNode itemnode = items.at(iterator);

                /* Check to see if the description of feature is same as provided in the list. */
                if (!QString::compare(itemnode.parentNode().attributes().item(0).nodeValue(), attributeList.at(featureCounter))) {

                    /* Feature has been found. */
                    featureFound = true;

                    /* Correct feature has been found, so take its address and store it in a buffer after converting to unsigned integer. */
                    featureAddressList.append(byteArrayToUint32(QByteArray::fromHex(QByteArray::fromStdString(itemnode.childNodes().item(0).nodeValue().toStdString()))));
                }
            }

            if (!featureFound) {

                qDebug() << "Address for " << attributeList.at(featureCounter) << " is not found in the XML file.";
            }
        }

        if (!error) {

            /* If error is not found, fetch register values. */
            error = cameraReadRegisterValue(destAddr, featureAddressList, registerValues);
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
    quint16 reqId = 1;

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
        for (int retryCount = 0, error = true;
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
    quint16 reqId = 2;
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
            for (retry_count = 0, error = true;
                (retry_count < MAX_ACK_FETCH_RETRY_COUNT) && error;
                (retry_count++)) {

                /* Wait for the signal, for reception of first URL. */
                if (mUdpSock->waitForReadyRead(100)) {

                    /* Receive the next pending ack. */
                    error = cameraFetchAck(ackHdr, reqId);
                }
            }

            if (retry_count == MAX_ACK_FETCH_RETRY_COUNT) {

                /* Memory block missed from between */
                error = true;
                break;
            }
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
    quint16 reqId = 1;

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
        for (int retryCount = 0, error = true;
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

bool cameraApi::cameraReadRegisterValue(const QHostAddress &destAddr, const QList<quint32> addressList, QList<quint32>& regValues)
{
    bool error = false;
    QByteArray cmdSpecificData;
    strGvcpCmdReadRegHdr regReadCmdHdr;
    strNonStdGvcpAckHdr ackHdr;
    strGvcpAckRegReadHdr *regReadAckHdr;
    QNetworkDatagram datagram;
    quint16 reqId = 1;

    /* Initialize memory to 0. */
    memset(&ackHdr, 0x00, sizeof(*(&ackHdr)));
    cmdSpecificData.clear();

    /* Check to see if the address list is empty. */
    if (addressList.empty()) {

        qDebug() << "Error: Feature list is empty";
        error = true;
    }

    if (!error) {

        /* Set register addresses to read. */
        regReadCmdHdr.registerAddress = new quint32(addressList.count());
        for (int counter = 0; counter < addressList.count(); counter++) {
            regReadCmdHdr.registerAddress[counter] = addressList.at(counter);
        }

        /* Append data set for URL header in generic command specidifc data array. */
        for (int counter = 0; counter < addressList.count(); counter++) {
            cmdSpecificData.append((char *)&regReadCmdHdr.registerAddress[counter], sizeof(quint32));
        }

        /* Free up memory space allocated earlier. */
        delete(regReadCmdHdr.registerAddress);

        /* Send command to the specified address. */
        mVectorPendingReq->push_front(reqId);
        error = gvcpSendCmd(mUdpSock, GVCP_READREG_CMD, cmdSpecificData, destAddr, GVCP_DEFAULT_UDP_PORT, reqId);
    }

    if (!error) {

        /* Receive data in the array. */
        for (int retryCount = 0, error = true;
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
bool cameraApi::cameraDiscoverDevice(const QHostAddress& addr, strNonStdGvcpAckHdr& ackHdr)
{
    bool error = false;
    QByteArray cmdSpecificData;
    quint16 reqId = 1;

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
        for (int retryCount = 0, error = true; retryCount < MAX_ACK_FETCH_RETRY_COUNT; retryCount++) {

            /* Wait for the signal,. */
            if (mUdpSock->waitForReadyRead(100)) {

                /* Receive the next pending ack. */
                error = cameraFetchAck(ackHdr, reqId);

                /* Check to see if correct acknowledgement packet has been found. */
                if (!error) {

                    break;
                }
            }
        }
    }

    if (error) {

        qDebug() << "Error: Correct acknowledgement packet is not received.";
    }

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
    const quint32 startAddressReadFromUrl = byteArrayToUint32(QByteArray::fromHex(startAddress));
    const quint16 sizeReadFromUrl = byteArrayToUint32(QByteArray::fromHex(size));

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
