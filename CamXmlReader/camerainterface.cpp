#include "camerainterface.h"
#include <QSequentialIterable>

CameraInterface::CameraInterface(QObject *parent)
    : QObject{parent} {
}

CameraInterface::CameraInterface(QUdpSocket *socket) {
    m_udp_sock = socket;
}

unsigned char CameraInterface::reverse_bits(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

bool CameraInterface::cam_setup() {
    bool error = false;

    /* Check to see if socket pointer is valid. */
    if (!m_udp_sock) {

        /* Set error to true to avoid any further processing in this func. */
        error = true;
        qDebug() << "m_udp_sock pointer is invalid.";
    }

    if (!error) {
        /* Continue here if udp socket pointer is valid. */
    }

    return error;
}

/* This function expects header and data to be in big endian format. */
bool CameraInterface::cam_send_cmd(quint32 command_type, QByteArray *cmd_specific_data,
                                   const QHostAddress *addr, quint16 port,
                                   quint16 req_id) {
    bool                error = false;
    QByteArray          datagram = { 0 };
    str_gvcp_cmd_hdr    generic_hdr;

    /* Initialize memory to 0. */
    memset(&generic_hdr, 0x00, sizeof(generic_hdr));

    /* Check to see if the pointer is not null. */
    if (!m_udp_sock) {
        error = true;
        qDebug() << "m_udp_sock pointer is invalid.";
    }

    if (!error) {

        /* Construct generic header for a specific command type. */
        switch (command_type) {

        case GVCP_DISCOVERY_CMD:
            /* Construct generic header for command [GVCP_DISCOVERY_CMD]. */
            generic_hdr.key_code = GVCP_CMD_HARDCODED_KEYCODE;
            generic_hdr.be_command = command_type;
            generic_hdr.flag = reverse_bits(GVCP_CMD_FLAG_ACKNOWLEDGE | GVCP_CMD_FLAG_DISCOVERY_BROADCAST_ACK);
            if (cmd_specific_data != nullptr)
                generic_hdr.be_length = cmd_specific_data->length();
            generic_hdr.be_req_id = req_id;

            /* Append generic header to the byte array. */
            datagram.append((char *)&generic_hdr, sizeof(generic_hdr));

            break;

        case GVCP_READMEM_CMD:
            /* Construct generic header for command [GVCP_READMEM_CMD]. */
            generic_hdr.key_code = GVCP_CMD_HARDCODED_KEYCODE;
            generic_hdr.be_command = command_type;
            generic_hdr.flag = reverse_bits(GVCP_CMD_FLAG_ACKNOWLEDGE);
            if (cmd_specific_data != nullptr)
                generic_hdr.be_length = cmd_specific_data->length();
            generic_hdr.be_req_id = req_id;

            /* Append generic header to the byte array. */
            datagram.append((char *)&generic_hdr, sizeof(generic_hdr));

            break;

        case GVCP_WRITEREG_CMD:
            /* Construct generic header for command [GVCP_WRITEREG_CMD]. */
            generic_hdr.key_code = GVCP_CMD_HARDCODED_KEYCODE;
            generic_hdr.be_command = command_type;
            generic_hdr.flag = reverse_bits(GVCP_CMD_FLAG_ACKNOWLEDGE);
            if (cmd_specific_data != nullptr)
                generic_hdr.be_length = cmd_specific_data->length();
            generic_hdr.be_req_id = req_id;

            /* Append generic header to the byte array. */
            datagram.append((char *)&generic_hdr, sizeof(generic_hdr));

            break;

        case GVCP_READREG_CMD:
            /* Construct generic header for command [GVCP_READREG_CMD]. */
            generic_hdr.key_code = GVCP_CMD_HARDCODED_KEYCODE;
            generic_hdr.be_command = command_type;
            generic_hdr.flag = reverse_bits(GVCP_CMD_FLAG_ACKNOWLEDGE);
            if (cmd_specific_data != nullptr)
                generic_hdr.be_length = cmd_specific_data->length();
            generic_hdr.be_req_id = req_id;

            /* Append generic header to the byte array. */
            datagram.append((char *)&generic_hdr, sizeof(generic_hdr));

            break;
        }

        /* Guarding checks to see if we have valid data in the buffer. */
        if ((cmd_specific_data != nullptr) && !error) {

            /* Append only if data length is greater than 0. */
            if (cmd_specific_data->length() > 0) {

                /* Append command specific data to the byte array. */
                datagram.append(*cmd_specific_data);
            }
        }

        /* Write datagram to the udp socket. */
        if(!(m_udp_sock->writeDatagram(datagram, *addr, port) > 0)) {
            /* Enter this when bytes written are <= 0. */

            qDebug() << "Error sending datagram.";
            error = true;
        }
    }

    return error;
}

bool CameraInterface::cam_receive_ack(QByteArray *raw_socket_data, str_non_std_gvcp_ack_hdr& ack_header) {
    bool                    error = false;
    char*                   data_ptr = nullptr;
    QByteArray              temp_array = { 0 };
    QNetworkDatagram        datagram = { 0 };

    /* Check to see if the pointer is not null. */
    if (!m_udp_sock) {
        error = true;
        qDebug() << "m_udp_sock pointer is invalid.";
    }

    if (!error) {
        /* Enter only if m_udp_sock is a valid pointer. */

        if (!(m_udp_sock->hasPendingDatagrams())) {

            /* No datagrams are waiting to be read. */
            error = true;

        } else {

            /* Valid datagrams are waiting to be read. */
            datagram = m_udp_sock->receiveDatagram();
        }
    }

    if (!(datagram.isValid())) {

        /* Set error to true because datagram is invalid. */
        error = true;
    }

    if (!error && raw_socket_data) {
        /* Enter here only if the received datagram is valid. */

        raw_socket_data->append(datagram.data());
    }

    if (!error) {

        /* Just create a temp array for storing payload. */
        temp_array.append(datagram.data());

        /* A pointer pointing to the data of byte array. */
        data_ptr = temp_array.data();

        memset(&ack_header, 0x00, sizeof(ack_header));
        memcpy(&ack_header.generic_ack_hdr, data_ptr, sizeof(ack_header.generic_ack_hdr));
    }

    if ((!error) && (ack_header.generic_ack_hdr.be_length > 0)) {

        switch (ack_header.generic_ack_hdr.be_acknowledge) {

        case GVCP_DISCOVERY_ACK:
            ack_header.ack_hdr_type = GVCP_DISCOVERY_ACK;
            ack_header.cmd_specific_ack_hdr = malloc(sizeof(str_gvcp_ack_discovery_hdr));
            memset(ack_header.cmd_specific_ack_hdr, 0x00, sizeof(str_gvcp_ack_discovery_hdr));
            memcpy(ack_header.cmd_specific_ack_hdr, data_ptr + sizeof(ack_header.generic_ack_hdr),
                   sizeof(str_gvcp_ack_discovery_hdr));
            break;

        case GVCP_READMEM_ACK:
            ack_header.ack_hdr_type = GVCP_READMEM_ACK;
            ack_header.cmd_specific_ack_hdr = malloc(sizeof(str_gvcp_ack_mem_read_hdr));
            memset(ack_header.cmd_specific_ack_hdr, 0x00, sizeof(str_gvcp_ack_mem_read_hdr));
            str_gvcp_ack_mem_read_hdr *read_mem_ack = (str_gvcp_ack_mem_read_hdr *)ack_header.cmd_specific_ack_hdr;

            /* Copy address. */
            memcpy(&read_mem_ack->address, data_ptr + sizeof(ack_header.generic_ack_hdr), sizeof(read_mem_ack->address));

            /* Copy data. */
            read_mem_ack->data.append(data_ptr + sizeof(ack_header.generic_ack_hdr) + sizeof(read_mem_ack->address),
                                      ack_header.generic_ack_hdr.be_length - sizeof(read_mem_ack->address));

            break;

        } /* Switch case end. */
    }

    if (error) {

        /* Was memory allocated for a command specific header? */
        if (ack_header.cmd_specific_ack_hdr) {

            /* Free the allocated memory. */
            free(ack_header.cmd_specific_ack_hdr);
        }
    }

    return (error);
}

