#ifndef CAMERAINTERFACE_H
#define CAMERAINTERFACE_H

#include <QObject>
#include <QThreadPool>
#include <QRunnable>
#include <QUdpSocket>
#include <QDebug>
#include <QNetworkInterface>
#include <QtEndian>
#include <QRandomGenerator>
#include <QtNetwork>
#include "gigevheaders.h"

class CameraInterface : public QObject
{
    Q_OBJECT
public:
    explicit CameraInterface(QObject *parent = nullptr);
    explicit CameraInterface(QUdpSocket *socket);

    /* Setup camera on the network. */
    bool cam_setup();

    /* Send a command to specified address and port. */
    bool cam_send_cmd(quint32 command_type, QByteArray *cmd_specific_data,
                      const QHostAddress *addr, quint16 port,
                      quint16 req_id);

    /* Receive a packet from UDP socket. */
    /* NOTE: This function allocates memory for command specific acknowledge. */
    bool cam_receive_ack(QByteArray *raw_socket_data, str_non_std_gvcp_ack_hdr& ack_header);

signals:

private:
    QUdpSocket *m_udp_sock;

    /* Just some helper functions. */
    unsigned char reverse_bits(unsigned char b);
};

#endif // CAMERAINTERFACE_H
