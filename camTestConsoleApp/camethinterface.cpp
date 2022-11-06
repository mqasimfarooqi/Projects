#include "camethinterface.h"

/* This function populates the relevant headers sends the packet. */
bool CamEthernetInterface::camEthernetSendCmd(QUdpSocket& udpSock, QNetworkDatagram& datagram) {

    quint32 error = CAMERA_ETHERNET_INTERFACE_STATUS_SUCCESS;

    /* Check to see if the pointer is not null. */
    if (!udpSock.isValid()) {
        error = CAMERA_ETHERNET_INTERFACE_STATUS_FAILED;
        qDebug() << "Udp socket pointer is invalid.";

    } else if (!(datagram.data().length() > 0)) {
        error = CAMERA_ETHERNET_INTERFACE_STATUS_FAILED;
        qDebug() << "No valid data present in the datagram.";

    } else if (datagram.destinationAddress().isNull()) {
        error = CAMERA_ETHERNET_INTERFACE_STATUS_FAILED;
        qDebug() << "Incorrect destination address for the datagram.";

    } else if (!(datagram.destinationPort() > 0)) {
        error = CAMERA_ETHERNET_INTERFACE_STATUS_FAILED;
        qDebug() << "Incorrect destination port for the datagram.";
    }

    if (error == CAMERA_ETHERNET_INTERFACE_STATUS_SUCCESS) {

        /* Write datagram to the udp socket. */
        if(!(udpSock.writeDatagram(datagram) > 0)) {

            qDebug() << "Error sending datagram.";
            error = CAMERA_ETHERNET_INTERFACE_STATUS_FAILED;
        }
    }

    return error;
}

bool CamEthernetInterface::camEthernetReceiveAck(QUdpSocket& udpSock, QNetworkDatagram& datagram)
{
    quint32 error = CAMERA_ETHERNET_INTERFACE_STATUS_SUCCESS;

    /* Check to see if the pointer is not null. */
    if (!udpSock.isValid() || !(udpSock.hasPendingDatagrams())) {

        error = CAMERA_ETHERNET_INTERFACE_STATUS_FAILED;
        qDebug() << "Cannot receive UDP packet either due to "
                    "invalid pointer to udp socket or there are no pending datagrams.";

    } else {

        /* Valid datagrams are waiting to be read. */
        datagram = udpSock.receiveDatagram();
    }

    return error;
}
