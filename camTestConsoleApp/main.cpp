#include <QCoreApplication>
#include "cameraapi.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    cameraApi *cam_api;
    QUdpSocket m_sock;
    QHostAddress addr;
    QByteArray regValues;
    QList<QString> featureList;
    strNonStdGvcpAckHdr discHdr;
    QDomDocument XmlDoc;
    strGvcpCmdWriteRegHdr writeUnit[3];
    bool error = false, xmlRead = false;
    char option;

    cam_api = new cameraApi(&m_sock, "172.19.17.20", CAMERA_API_COMMAND_READXML);

    featureList.append("DeviceVendorName");
    featureList.append("DeviceUserID");
    featureList.append("GevMCPHostPortReg");

    /* Set address to listen for acks */
    addr.setAddress("172.19.17.20");

    /* Bind UDP socket to an address. */
    if (!(m_sock.bind(addr, 55455))) {

        qDebug() << "Could not bind socket to an address/port.";
        error = true;
    }

    writeUnit[0].registerAddress = 0xa00;
    writeUnit[0].registerData = 0x2;

    writeUnit[1].registerAddress = 0xb00;
    writeUnit[1].registerData = 0xa9da;

    writeUnit[2].registerAddress = 0xb00;
    writeUnit[2].registerData = 0xa9da;

    /* Set address where the packet should be sent. */
    addr.setAddress("172.19.17.21");

    qDebug() << "0 -> Test discover device";
    qDebug() << "1 -> Test read register command";
    qDebug() << "2 -> Test write register command";
    qDebug() << "3 -> Test read xml file";
    qDebug() << "-----------------------------";
    qDebug() << "Watch output on wireshark";

    while(!error) {

        option = std::getchar();

        switch (option) {
        case '0':
            error = cam_api->cameraDiscoverDevice(addr, discHdr);
            if (error)
                qDebug() << "Command failed";
            break;

        case '1':
            if (xmlRead) {
                error = cam_api->cameraXmlReadCameraAttribute(featureList, XmlDoc, addr, regValues);
                if (error)
                    qDebug() << "Command failed";
            }
            else
                qDebug() << "Fetch xml file from device first";
            break;

        case '2':
            error = cam_api->cameraWriteRegisterValue(addr, writeUnit, sizeof(writeUnit)/sizeof(writeUnit[0]));
            if (error)
                qDebug() << "Command failed";
            break;

        case '3':
            error = cam_api->cameraXmlReadXmlFileFromDevice(XmlDoc, addr);
            if (error)
                qDebug() << "Command failed";
            else
                xmlRead = true;
            break;

        }
    }

    qDebug() << "Error occured. Exiting program.";
    delete (cam_api);
    return a.exec();
}
