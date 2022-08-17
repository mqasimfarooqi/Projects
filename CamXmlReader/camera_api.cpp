#include "camera_api.h"

camera_api::camera_api(QObject *parent)
    : QObject{parent}
{
}

camera_api::camera_api(QUdpSocket *udpSocket, QString addr, quint8 command)
{
    mUdpSock = udpSocket;
    mCamera = new CameraInterface(mUdpSock);
    mCommand = command;
    mAddr = addr;
}

void camera_api::slotReadyRead()
{

    /* Emit the signal taht the ack has been received for a pending req_id. */
    emit signalAckReceived();

}

void camera_api::run()
{
    QDomDocument temp;
    /* This is the function that will execute desired commands. */
    switch (mCommand) {
    case CAMERA_API_COMMAND_READXML:
        cameraXmlReadXmlFileFromDevice(temp);
        cameraXmlReadCameraAttribute("Hello", temp);

        break;
    }
}

quint32 camera_api::byteArrayToUint32(const QByteArray &bytes)
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

bool camera_api::cameraXmlReadXmlFileFromDevice(QDomDocument& xmlFile)
{
    bool error = false;
    QList<QByteArray> url;
    QRandomGenerator rand;
    QHostAddress addr;
    QNetworkDatagram datagram;
    QByteArray first_url;
    QByteArray xml_data;
    QScopedPointer<QEventLoop> loop(new QEventLoop);

    if (!mUdpSock) {
        error = true;
        qDebug() << "Invalid UDP socket pointer";
    }

    if (!error) {

        /* Bound to the maximum range of UDP port. */
        rand.bounded(100, 65530);

        /* Set address */
        addr.setAddress(mAddr);

        /* Make relevant connections. */
        connect(mUdpSock, &QUdpSocket::readyRead, this, &camera_api::slotReadyRead, Qt::DirectConnection);

        /* Bind UDP socket to an address. */
        if (!(mUdpSock->bind(addr, rand.generate()))) {

            qDebug() << "Could not bind socket to an address/port.";
            error = true;
        }
    }

    /* Set addr for fetching the file. */
    addr.setAddress("172.19.17.21");

    if (!error) {

        /* Fetch first URL. */
        error = cameraFetchFirstUrl(addr, first_url);
    }

    if (!error) {

        /* Identify the location in which XML file is placed. */
        if (first_url.contains("Local:")) {

            /* Split the path, address and size. */
            url = first_url.mid(0, first_url.indexOf('\0')).split(';');
            error = cameraFetchXmlFromDevice(url[0], url[1], url[2], addr, xml_data);

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

bool camera_api::cameraXmlReadCameraAttribute(const QString feature, const QDomDocument& xmlFile)
{
    bool error = false;
    QDomElement root;

    if (!error) {
        root = xmlFile.firstChildElement();

        QDomNodeList items = root.elementsByTagName("Address");
        for (int i = 0; i < items.count(); i++) {
            QDomNode itemnode = items.at(i);
            qDebug() << itemnode.parentNode().attributes().item(0).nodeValue() << itemnode.childNodes().item(0).nodeValue();
        }
    }

    return error;
}

bool camera_api::cameraFetchAck(strNonStdGvcpAckHdr& ackHdr, const quint16 reqId)
{
    bool error = false;

    /* Receive the next pending ack. */
    error = mCamera->camReceiveAck(nullptr, ackHdr);

    /* If correct ack is received */
    if ((ackHdr.genericAckHdr.ackId == reqId) && !error) {

        /* Remove the pending request ID stored in vector. */
        mVectorPendingReq.removeOne(ackHdr.genericAckHdr.ackId);
        mVectorPendingReq.squeeze();

    } else {

        /* Wrong ACK packet is received. */
        error = true;

        if (ackHdr.cmdSpecificAckHdr) {

            /* Free memory space acquired by command specific ack header for packed with undesired req_id. */
            free(ackHdr.cmdSpecificAckHdr);
        }
    }

    return error;
}

bool camera_api::cameraReadMemoryBlock(const quint32 address, const quint16 size,
                                          const QHostAddress addr, QByteArray& returnedData)
{
    bool error = false;
    QByteArray cmdSpecificData;
    strGvcpCmdReadMemHdr hdr;
    strNonStdGvcpAckHdr ackHdr;
    strGvcpAckMemReadHdr *readMemAckHdr;
    QScopedPointer<QEventLoop> loop(new QEventLoop);
    quint16 reqId = 2;
    quint16 bytesRead = 0;

    /* General precautions. */
    returnedData.clear();

    /* Make connection to escape from main event loop. */
    connect(this, &camera_api::signalAckReceived, loop.data(), &QEventLoop::quit, Qt::DirectConnection);

    /* Set the address for first iteration. */
    hdr.address = address;

    /* Set bytes to read in first iteration. */
    hdr.count = (GVCP_MAX_PAYLOAD_LENGTH < size ? GVCP_MAX_PAYLOAD_LENGTH : size);

    while ((hdr.address < (address + size)) && !error) {

        /* Append data set for URL header in generic command specidifc data array. */
        cmdSpecificData.append((const char *)&hdr, sizeof(hdr));

        /* Send command to the specified address. */
        mVectorPendingReq.push_front(reqId);
        mCamera->camSendCmd(GVCP_READMEM_CMD, &cmdSpecificData, &addr, GVCP_DEFAULT_UDP_PORT, reqId);

        /* Receive data in the array. */
        for (int retry_count = 0, error = true;
            (retry_count < MAX_ACK_FETCH_RETRY_COUNT) && error;
            (retry_count++)) {

            /* Wait for the signal, for reception of first URL. */
            loop->exec();

            /* Receive the next pending ack. */
            error = cameraFetchAck(ackHdr, reqId);
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

bool camera_api::cameraFetchFirstUrl(QHostAddress addr, QByteArray& byteArray)
{
    bool error = false;
    QByteArray cmdSpecificData;
    strGvcpCmdReadMemHdr hdr;
    strNonStdGvcpAckHdr ackHdr;
    strGvcpAckMemReadHdr *readMemAckHdr;
    QNetworkDatagram datagram;
    QScopedPointer<QEventLoop> loop(new QEventLoop);
    quint8 retryCount = 0;
    quint16 reqId = 1;

    /* Clear buffers for command processing. */
    cmdSpecificData.clear();
    memset(&hdr, 0x00, sizeof(hdr));

    /* Make connection to escape from main event loop. */
    connect(this, &camera_api::signalAckReceived, loop.data(), &QEventLoop::quit, Qt::DirectConnection);

    /* Set properties of header to fetch URL. */
    hdr.address = DEVICE_FIRST_URL_ADDRESS;
    hdr.count = DEVICE_URL_ADDRESS_REG_LENGTH;

    /* Append data set for URL header in generic command specidifc data array. */
    cmdSpecificData.append((const char *)&hdr, sizeof(hdr));

    /* Send command to the specified address. */
    mVectorPendingReq.push_front(reqId);
    mCamera->camSendCmd(GVCP_READMEM_CMD, &cmdSpecificData, &addr, GVCP_DEFAULT_UDP_PORT, reqId);

    /* Set to true and turn it to false when finding the right packet. */
    error = true;

    /* Receive data in the array. */
    for (retryCount = 0; retryCount < MAX_ACK_FETCH_RETRY_COUNT; retryCount++) {

        /* Wait for the signal, for reception of first URL. */
        loop->exec();

        /* Receive the next pending ack. */
        error = cameraFetchAck(ackHdr, reqId);

        /* Correct acknowledgement packet has been found. */
        if (!error)
            break;
    }

    if (!error) {

        /* Now we have first URL. */
        readMemAckHdr = (strGvcpAckMemReadHdr *)ackHdr.cmdSpecificAckHdr;

        /* Append data into the byte array, and return it. */
        byteArray.append(readMemAckHdr->data);
    }

    if (ackHdr.cmdSpecificAckHdr) {

        /* Free memory allocated by ack header. */
        delete((strGvcpAckMemReadHdr *)ackHdr.cmdSpecificAckHdr);
    }

    return error;
}

bool camera_api::cameraFetchXmlFromDevice(QByteArray fileName, QByteArray startAddress,
                                              QByteArray size, QHostAddress addr,
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
    error = cameraReadMemoryBlock(startAddressReadFromUrl, sizeReadFromUrl, addr, retrievedXml);
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
