#include "camera_api.h"

camera_api::camera_api(QObject *parent)
    : QObject{parent}
{
}

camera_api::camera_api(QUdpSocket *udp_socket, QString addr, quint8 command)
{
    m_udp_sock = udp_socket;
    m_camera = new CameraInterface(m_udp_sock);
    m_command = command;
    m_addr = addr;
}

void camera_api::slot_ready_read()
{

    /* Emit the signal taht the ack has been received for a pending req_id. */
    emit signal_ack_received();

}

void camera_api::run()
{
    QDomDocument temp;
    /* This is the function that will execute desired commands. */
    switch (m_command) {
    case CAMERA_API_COMMAND_READXML:
        camera_xml_read_xml_file_from_device(temp);
        camera_xml_read_camera_attribute("Hello", temp);

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

bool camera_api::camera_xml_read_xml_file_from_device(QDomDocument& xml_file)
{
    bool error = false;
    QList<QByteArray> url;
    QRandomGenerator rand;
    QHostAddress addr;
    QNetworkDatagram datagram;
    QByteArray first_url;
    QByteArray xml_data;
    QScopedPointer<QEventLoop> loop(new QEventLoop);

    if (!m_udp_sock) {
        error = true;
        qDebug() << "Invalid UDP socket pointer";
    }

    if (!error) {

        /* Bound to the maximum range of UDP port. */
        rand.bounded(100, 65530);

        /* Set address */
        addr.setAddress(m_addr);

        /* Make relevant connections. */
        connect(m_udp_sock, &QUdpSocket::readyRead, this, &camera_api::slot_ready_read, Qt::DirectConnection);

        /* Bind UDP socket to an address. */
        if (!(m_udp_sock->bind(addr, rand.generate()))) {

            qDebug() << "Could not bind socket to an address/port.";
            error = true;
        }
    }

    /* Set addr for fetching the file. */
    addr.setAddress("172.19.17.21");

    if (!error) {

        /* Fetch first URL. */
        error = camera_fetch_first_url(addr, first_url);
    }

    if (!error) {

        /* Identify the location in which XML file is placed. */
        if (first_url.contains("Local:")) {

            /* Split the path, address and size. */
            url = first_url.mid(0, first_url.indexOf('\0')).split(';');
            error = camera_fetch_xml_from_device(url[0], url[1], url[2], addr, xml_data);

        } else if (first_url.contains("http:")) {
            qDebug() << "Unsupported location.";
        } else {
            qDebug() << "Unsupported location.";
        }
    }

    if (!error) {
        if(!xml_file.setContent(xml_data)) {
            error = true;
        }
    }

    return (error);
}

bool camera_api::camera_xml_read_camera_attribute(const QString feature, const QDomDocument& xml_file)
{
    bool error = false;
    QDomElement root;

    if (!error) {
        root = xml_file.firstChildElement();

        QDomNodeList items = root.elementsByTagName("Address");
        for (int i = 0; i < items.count(); i++) {
            QDomNode itemnode = items.at(i);
            qDebug() << itemnode.parentNode().attributes().item(0).nodeValue() << itemnode.childNodes().item(0).nodeValue();
        }
    }

    return error;
}

bool camera_api::camera_fetch_ack(str_non_std_gvcp_ack_hdr& ack_hdr, const quint16 req_id)
{
    bool error = false;

    /* Receive the next pending ack. */
    error = m_camera->cam_receive_ack(nullptr, ack_hdr);

    /* If correct ack is received */
    if ((ack_hdr.generic_ack_hdr.be_ack_id == req_id) && !error) {

        /* Remove the pending request ID stored in vector. */
        m_vector_pending_req.removeOne(ack_hdr.generic_ack_hdr.be_ack_id);
        m_vector_pending_req.squeeze();

    } else {

        /* Wrong ACK packet is received. */
        error = true;

        if (ack_hdr.cmd_specific_ack_hdr) {

            /* Free memory space acquired by command specific ack header for packed with undesired req_id. */
            free(ack_hdr.cmd_specific_ack_hdr);
        }
    }

    return error;
}

bool camera_api::camera_read_memory_block(const quint32 address, const quint16 size,
                                          const QHostAddress addr, QByteArray& returned_data)
{
    bool error = false;
    QByteArray cmd_specific_data;
    str_gvcp_cmd_read_mem_hdr hdr;
    str_non_std_gvcp_ack_hdr ack_hdr;
    str_gvcp_ack_mem_read_hdr *read_mem_ack_hdr;
    QScopedPointer<QEventLoop> loop(new QEventLoop);
    quint16 req_id = 2;
    quint16 bytes_read = 0;

    /* General precautions. */
    returned_data.clear();

    /* Make connection to escape from main event loop. */
    connect(this, &camera_api::signal_ack_received, loop.data(), &QEventLoop::quit, Qt::DirectConnection);

    /* Set the address for first iteration. */
    hdr.address = address;

    /* Set bytes to read in first iteration. */
    hdr.count = (GVCP_MAX_PAYLOAD_LENGTH < size ? GVCP_MAX_PAYLOAD_LENGTH : size);

    while ((hdr.address < (address + size)) && !error) {

        /* Append data set for URL header in generic command specidifc data array. */
        cmd_specific_data.append((const char *)&hdr, sizeof(hdr));

        /* Send command to the specified address. */
        m_vector_pending_req.push_front(req_id);
        m_camera->cam_send_cmd(GVCP_READMEM_CMD, &cmd_specific_data, &addr, GVCP_DEFAULT_UDP_PORT, req_id);

        /* Receive data in the array. */
        for (int retry_count = 0, error = true;
            (retry_count < MAX_ACK_FETCH_RETRY_COUNT) && error;
            (retry_count++)) {

            /* Wait for the signal, for reception of first URL. */
            loop->exec();

            /* Receive the next pending ack. */
            error = camera_fetch_ack(ack_hdr, req_id);
        }

        if (!error) {
            /* Increase the address to point to new memory location. */
            hdr.address += hdr.count;

            /* Increase bytes read counter. */
            bytes_read += hdr.count;

            /* Set the next number of bytes to be read. */
            hdr.count = (((hdr.address + GVCP_MAX_PAYLOAD_LENGTH) < (address + size)) ?
                             GVCP_MAX_PAYLOAD_LENGTH : size - bytes_read);

            /* Append data to the byte_array to be returned. */
            read_mem_ack_hdr = (str_gvcp_ack_mem_read_hdr *)ack_hdr.cmd_specific_ack_hdr;
            returned_data.append(read_mem_ack_hdr->data);

            /* Free up memory spaceacquired by command specific header. */
            if (ack_hdr.cmd_specific_ack_hdr) {

                /* Free memory allocated by ack header. */
                delete((str_gvcp_ack_mem_read_hdr *)ack_hdr.cmd_specific_ack_hdr);
            }

            /* Increase the request ID for next transation. */
            req_id++;

            /* Clear buffers for next command processing. */
            cmd_specific_data.clear();
        }
    }

    return error;
}

bool camera_api::camera_fetch_first_url(QHostAddress addr, QByteArray& byte_array)
{
    bool error = false;
    QByteArray cmd_specific_data;
    str_gvcp_cmd_read_mem_hdr hdr;
    str_non_std_gvcp_ack_hdr ack_hdr;
    str_gvcp_ack_mem_read_hdr *read_mem_ack_hdr;
    QNetworkDatagram datagram;
    QScopedPointer<QEventLoop> loop(new QEventLoop);
    quint8 retry_count = 0;
    quint16 req_id = 1;

    /* Clear buffers for command processing. */
    cmd_specific_data.clear();
    memset(&hdr, 0x00, sizeof(hdr));

    /* Make connection to escape from main event loop. */
    connect(this, &camera_api::signal_ack_received, loop.data(), &QEventLoop::quit, Qt::DirectConnection);

    /* Set properties of header to fetch URL. */
    hdr.address = DEVICE_FIRST_URL_ADDRESS;
    hdr.count = DEVICE_URL_ADDRESS_REG_LENGTH;

    /* Append data set for URL header in generic command specidifc data array. */
    cmd_specific_data.append((const char *)&hdr, sizeof(hdr));

    /* Send command to the specified address. */
    m_vector_pending_req.push_front(req_id);
    m_camera->cam_send_cmd(GVCP_READMEM_CMD, &cmd_specific_data, &addr, GVCP_DEFAULT_UDP_PORT, req_id);

    /* Set to true and turn it to false when finding the right packet. */
    error = true;

    /* Receive data in the array. */
    for (retry_count = 0; retry_count < MAX_ACK_FETCH_RETRY_COUNT; retry_count++) {

        /* Wait for the signal, for reception of first URL. */
        loop->exec();

        /* Receive the next pending ack. */
        error = camera_fetch_ack(ack_hdr, req_id);

        /* Correct acknowledgement packet has been found. */
        if (!error)
            break;
    }

    if (!error) {

        /* Now we have first URL. */
        read_mem_ack_hdr = (str_gvcp_ack_mem_read_hdr *)ack_hdr.cmd_specific_ack_hdr;

        /* Append data into the byte array, and return it. */
        byte_array.append(read_mem_ack_hdr->data);
    }

    if (ack_hdr.cmd_specific_ack_hdr) {

        /* Free memory allocated by ack header. */
        delete((str_gvcp_ack_mem_read_hdr *)ack_hdr.cmd_specific_ack_hdr);
    }

    return error;
}

bool camera_api::camera_fetch_xml_from_device(QByteArray file_name, QByteArray start_address,
                                              QByteArray size, QHostAddress addr,
                                              QByteArray& xml_data)
{
    bool error = false;
    QByteArray retrieved_xml;
    QNetworkDatagram datagram;
    QStringList list;
    QFile file(file_name), xml_file;
    QDataStream stream(&file);

    const quint32 start_address_read_from_url = byteArrayToUint32(QByteArray::fromHex(start_address));
    const quint16 size_read_from_url = byteArrayToUint32(QByteArray::fromHex(size));

    /* Retrieve data from the memory blockes where XML file is stored in the device. */
    error = camera_read_memory_block(start_address_read_from_url, size_read_from_url, addr, retrieved_xml);
    if (!error) {
        if (file.open(QIODevice::ReadWrite)) {
            stream.setByteOrder(QDataStream::LittleEndian);
            stream << retrieved_xml;
        }

        list = JlCompress::extractDir(file_name, file_name + "_extracted");
        if (list.isEmpty()) {
            error = true;
        } else {

            xml_file.setFileName(list.first());
            if (xml_file.open(QIODevice::ReadWrite)) {
                xml_data = xml_file.readAll();
            }
        }
    }

    return error;
}
