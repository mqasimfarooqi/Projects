#include "caminterface.h"

/* This function populates the relevant headers sends the packet. */
bool caminterface::camGigeVSendCmd(QUdpSocket& udpSock, QNetworkDatagram& datagram) {

    bool error = false;

    /* Check to see if the pointer is not null. */
    if (!udpSock.isValid()) {
        error = true;
        qDebug() << "Error: Udp socket pointer is invalid.";

    } else if (!(datagram.data().length() > 0)) {
        error = true;
        qDebug() << "Error: No valid data present in the datagram.";

    } else if (datagram.destinationAddress().isNull()) {
        error = true;
        qDebug() << "Error: Incorrect destination address for the datagram.";

    } else if (!(datagram.destinationPort() > 0)) {
        error = true;
        qDebug() << "Error: Incorrect destination port for the datagram.";
    }

    if (!error) {

        /* Write datagram to the udp socket. */
        if(!(udpSock.writeDatagram(datagram) > 0)) {

            qDebug() << "Error: Error sending datagram.";
            error = true;
        }
    }

    return error;
}

bool caminterface::camGigeVReceiveAck(QUdpSocket& udpSock, QNetworkDatagram& datagram)
{
    bool error = false;

    /* Check to see if the pointer is not null. */
    if (!udpSock.isValid() || !(udpSock.hasPendingDatagrams())) {

        error = true;
        qDebug() << "Error: Cannot receive UDP packet either due to "
                    "invalid pointer to udp socket or there are no pending datagrams.";

    } else {

        /* Valid datagrams are waiting to be read. */
        datagram = udpSock.receiveDatagram();
    }

    return error;
}
