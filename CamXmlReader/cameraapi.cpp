#include "cameraapi.h"
#include "camerainterface.h"
#include "quazip/JlCompress.h"

cameraApi::cameraApi(QObject *parent)
    : QObject{parent}
{
}

cameraApi::cameraApi(QUdpSocket *udpSocket, QString addr, quint8 command)
{
    mUdpSock = udpSocket;
    mCommand = command;
    mAddr = addr;
}

void cameraApi::slotReadyRead()
{

    /* Emit the signal taht the ack has been received for a pending req_id. */
    emit signalAckReceived();

}

void cameraApi::run()
{
    QHostAddress addr;
    QRandomGenerator rand;
    bool error = false;
    QByteArray regValues;
    quint32_be regs[1] = {(quint32_be)0x0D00};

    /* Set address to listen for acks */
    addr.setAddress("172.19.17.20");

    /* Bound to the maximum range of UDP port. */
    rand.bounded(100, 65530);

    /* Bind UDP socket to an address. */
    if (!(mUdpSock->bind(addr, rand.generate()))) {

        qDebug() << "Could not bind socket to an address/port.";
        error = true;
    }

    /* Set address where the packet should be sent. */
    addr.setAddress("172.19.17.21");

    if (!error) {
        /* This is the function that will execute desired commands. */
        switch (mCommand) {
        case CAMERA_API_COMMAND_READXML:
            //cameraXmlReadXmlFileFromDevice(mXmlDoc, addr);
            //cameraXmlReadCameraAttribute("DeviceVendorName", mXmlDoc);


            cameraReadRegisterValue(addr, regs, sizeof(regs), regValues);
            qDebug() << regValues.toHex();

            /* Set address */
            //cameraDiscoverDevice(addr);

            break;
        }
    }

    mUdpSock->abort();
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

bool cameraApi::cameraXmlReadXmlFileFromDevice(QDomDocument& xmlFile, const QHostAddress& destAddr)
{
    bool error = false;
    QList<QByteArray> url;
    QNetworkDatagram datagram;
    QByteArray first_url;
    QByteArray xml_data;

    if (!mUdpSock->isValid()) {
        error = true;
        qDebug() << "Invalid UDP socket pointer";
    }

    if (!error) {

        /* Make relevant connections. */
        connect(mUdpSock, &QUdpSocket::readyRead, this, &cameraApi::slotReadyRead, Qt::DirectConnection);
    }


    if (!error) {

        /* Fetch first URL. */
        error = cameraFetchFirstUrl(destAddr, first_url);
    }

    if (!error) {

        /* Identify the location in which XML file is placed. */
        if (first_url.contains("Local:")) {

            /* Split the path, address and size. */
            url = first_url.mid(0, first_url.indexOf('\0')).split(';');
            error = cameraFetchXmlFromDevice(url[0], url[1], url[2], destAddr, xml_data);

        } else if (first_url.contains("http:")) {
            qDebug() << "Unsupported location.";
        } else {
            qDebug() << "Unsupported location.";
        }
    }

    if (!error) {
        if(!xmlFile.setContent(xml_data)) {
            error = true;
        }
    }

    return (error);
}

bool cameraApi::cameraXmlReadCameraAttribute(const QString feature, const QDomDocument& xmlFile)
{
    bool error = false;
    QDomElement root;

    if (xmlFile.isDocument()) {
        root = xmlFile.firstChildElement();

        QDomNodeList items = root.elementsByTagName("Address");
        for (int i = 0; i < items.count(); i++) {
            QDomNode itemnode = items.at(i);
            if (!QString::compare(itemnode.parentNode().attributes().item(0).nodeValue(), feature)) {
                qDebug() << itemnode.childNodes().item(0).nodeValue();
            }
        }
    }

    return error;
}

bool cameraApi::cameraFetchAck(strNonStdGvcpAckHdr& ackHdr, const quint16 reqId)
{
    bool error = true;

    /* Receive the next pending ack. */
    error = CameraInterface::camReceiveAck(mUdpSock, ackHdr);

    /* If correct ack is received */
    if ((ackHdr.genericAckHdr.ackId == reqId) && !error) {

        /* Remove the pending request ID stored in vector. */
        mVectorPendingReq.removeOne(ackHdr.genericAckHdr.ackId);
        mVectorPendingReq.squeeze();

        error = false;

    } else {

        if (ackHdr.cmdSpecificAckHdr) {

            /* Free memory space acquired by command specific ack header for packed with undesired req_id. */
            free(ackHdr.cmdSpecificAckHdr);
        }
    }

    return error;
}

bool cameraApi::cameraReadMemoryBlock(const quint32 address, const quint16 size,
                                       const QHostAddress destAddr, QByteArray& returnedData)
{
    bool error = false;
    QByteArray cmdSpecificData;
    strGvcpCmdReadMemHdr hdr;
    strNonStdGvcpAckHdr ackHdr;
    strGvcpAckMemReadHdr *readMemAckHdr;
    quint16 reqId = 2;
    quint16 bytesRead = 0;
    quint32 retry_count = 0;

    /* General precautions. */
    returnedData.clear();

    /* Set the address for first iteration. */
    hdr.address = address;

    /* Set bytes to read in first iteration. */
    hdr.count = (GVCP_MAX_PAYLOAD_LENGTH < size ? GVCP_MAX_PAYLOAD_LENGTH : size);

    while ((hdr.address < (address + size)) && !error) {

        /* Append data set for URL header in generic command specidifc data array. */
        cmdSpecificData.append((const char *)&hdr, sizeof(hdr));

        /* Send command to the specified address. */
        mVectorPendingReq.push_front(reqId);
        error = CameraInterface::camSendCmd(mUdpSock, GVCP_READMEM_CMD, &cmdSpecificData, &destAddr, GVCP_DEFAULT_UDP_PORT, reqId);

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
            hdr.address += hdr.count;

            /* Increase bytes read counter. */
            bytesRead += hdr.count;

            /* Set the next number of bytes to be read. */
            hdr.count = (((hdr.address + GVCP_MAX_PAYLOAD_LENGTH) < (address + size)) ?
                             GVCP_MAX_PAYLOAD_LENGTH : size - bytesRead);

            /* Append data to the byte_array to be returned. */
            readMemAckHdr = (strGvcpAckMemReadHdr *)ackHdr.cmdSpecificAckHdr;
            returnedData.append(readMemAckHdr->data);

            /* Free up memory space acquired by command specific header. */
            if (ackHdr.cmdSpecificAckHdr) {

                /* Free memory allocated by ack header. */
                delete((strGvcpAckMemReadHdr *)ackHdr.cmdSpecificAckHdr);
            }

            /* Increase the request ID for next transation. */
            reqId++;

            /* Clear buffers for next command processing. */
            cmdSpecificData.clear();
        }
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
    memset(&cmdHdr, 0x00, sizeof(cmdHdr));

    /* Set properties of header to fetch URL. */
    cmdHdr.address = DEVICE_FIRST_URL_ADDRESS;
    cmdHdr.count = DEVICE_URL_ADDRESS_REG_LENGTH;

    /* Append data set for URL header in generic command specidifc data array. */
    cmdSpecificData.append((const char *)&cmdHdr, sizeof(cmdHdr));

    /* Send command to the specified address. */
    mVectorPendingReq.push_front(reqId);
    error = CameraInterface::camSendCmd(mUdpSock, GVCP_READMEM_CMD, &cmdSpecificData, &destAddr, GVCP_DEFAULT_UDP_PORT, reqId);

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
        readMemAckHdr = (strGvcpAckMemReadHdr *)ackHdr.cmdSpecificAckHdr;

        /* Append data into the byte array, and return it. */
        byteArray.append(readMemAckHdr->data);

        /* Free memory allocated by ack header. */
        free(ackHdr.cmdSpecificAckHdr);
    }

    return error;
}

bool cameraApi::cameraReadRegisterValue(const QHostAddress &destAddr, const quint32_be regs[], quint32 regsArraySize, QByteArray& values)
{
    bool error = false;
    QByteArray cmdSpecificData;
    strGvcpCmdReadRegHdr regReadCmdHdr;
    strNonStdGvcpAckHdr ackHdr;
    strGvcpAckRegReadHdr *regReadAckHdr;
    QNetworkDatagram datagram;
    quint16 reqId = 1;

    /* Clear buffers for command processing. */
    cmdSpecificData.clear();

    /* Set register addresses to read. */
    regReadCmdHdr.registerAddress = new quint32_be(regsArraySize);

    /* Copy data into the array. */
    memcpy(regReadCmdHdr.registerAddress, regs, regsArraySize);

    /* Append data set for URL header in generic command specidifc data array. */
    for (quint32 i = 0; i < regsArraySize; i++) {
        cmdSpecificData.append(*((quint8 *)regReadCmdHdr.registerAddress + i));
    }

    /* Send command to the specified address. */
    mVectorPendingReq.push_front(reqId);
    error = CameraInterface::camSendCmd(mUdpSock, GVCP_READREG_CMD, &cmdSpecificData, &destAddr, GVCP_DEFAULT_UDP_PORT, reqId);

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

        /* Clear the bytearray before writing data. */
        values.clear();

        /* Append data into the byte array, and return it. */
        values.append(regReadAckHdr->registerData);

        /* Free memory allocated by ack header. */
        free(ackHdr.cmdSpecificAckHdr);
    }

    return error;
}

bool cameraApi::cameraDiscoverDevice(const QHostAddress& addr)
{
    bool error = false;
    QByteArray cmdSpecificData;
    quint16 reqId = 1;
    strNonStdGvcpAckHdr ackHdr;
    QRandomGenerator rand;
    strGvcpAckDiscoveryHdr *discoveryAckHdr;

    if (!mUdpSock->isValid()) {
        error = true;
        qDebug() << "Invalid UDP socket pointer";
    }

    if (!error) {

        /* Make relevant connections. */
        connect(mUdpSock, &QUdpSocket::readyRead, this, &cameraApi::slotReadyRead, Qt::DirectConnection);

        /* Clear buffers for command processing. */
        cmdSpecificData.clear();

        /* Send command to the specified address. */
        mVectorPendingReq.push_front(reqId);
        error = CameraInterface::camSendCmd(mUdpSock, GVCP_DISCOVERY_CMD, &cmdSpecificData, &addr, GVCP_DEFAULT_UDP_PORT, reqId);
    }

    if (!error) {

        /* Receive data in the array. */
        for (int retryCount = 0, error = true; retryCount < MAX_ACK_FETCH_RETRY_COUNT; retryCount++) {

            /* Wait for the signal, for reception of first URL. */
            if (mUdpSock->waitForReadyRead(100)) {

                /* Receive the next pending ack. */
                error = cameraFetchAck(ackHdr, reqId);

                /* Correct acknowledgement packet has been found. */
                if (!error)
                    break;
            }
        }
    }

    if (!error && ackHdr.cmdSpecificAckHdr) {
        discoveryAckHdr = (strGvcpAckDiscoveryHdr *)ackHdr.cmdSpecificAckHdr;

        qDebug() << "Discovery packet received at: " << discoveryAckHdr;
    }

    if (ackHdr.cmdSpecificAckHdr) {

        /* Free memory allocated by ack header. */
        free(ackHdr.cmdSpecificAckHdr);
    }

    return error;
}

bool cameraApi::cameraFetchXmlFromDevice(QByteArray fileName, QByteArray startAddress,
                                          QByteArray size, QHostAddress destAddr,
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
