#include <QCoreApplication>
#include "cameraapi.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    cameraApi *cam_api;
    QUdpSocket m_sock;
    QHostAddress bindAddr, destAddr;
    QList<QString> featureList;
    strNonStdGvcpAckHdr discHdr;
    QDomDocument XmlDoc;
    QList<strGvcpCmdWriteRegHdr> writeUnit;
    bool error = false, xmlRead = false;
    char option;
    QVector<quint8> pendingReqVec;
    QList<quint32> regValues;
    strGvcpCmdWriteRegHdr temp;

    cam_api = new cameraApi(&m_sock, &pendingReqVec);

    featureList.append("DeviceVendorName");
    featureList.append("DeviceUserID");
    featureList.append("GevMCPHostPortReg");

    /* Bind UDP socket to an address. */
    bindAddr.setAddress("172.19.17.20");
    if (!(m_sock.bind(bindAddr, 55455))) {

        qDebug() << "Could not bind socket to an address/port.";
        error = true;
    }

    temp.registerAddress = 0xa00;
    temp.registerData = 0x2;
    writeUnit.append(temp);

    temp.registerAddress = 0xb00;
    temp.registerData = 0xa9da;
    writeUnit.append(temp);

    temp.registerAddress = 0xb00;
    temp.registerData = 0xa9da;
    writeUnit.append(temp);

    /* Set address where the packet should be sent. */
    destAddr.setAddress("172.19.17.21");

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
            error = cam_api->cameraDiscoverDevice(destAddr, discHdr);
            if (error) {
                qDebug() << "Command failed";
            } else {
                qDebug() << "manufacturerSpecificInfo" << QByteArray::fromRawData((char *)((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr)->manufacturerSpecificInfo, 48);
                qDebug() << "manufacturerName = "  << QByteArray::fromRawData((char *)((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr)->manufacturerName, 32);
                qDebug() << "modelName = "  << QByteArray::fromRawData((char *)((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr)->modelName, 32);
                qDebug() << "userDefinedName = "  << QByteArray::fromRawData((char *)((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr)->userDefinedName, 16);
                qDebug() << "currentIp = "  << ((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr)->currentIp;
                qDebug() << "defaultGateway = "  << ((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr)->defaultGateway;
                qDebug() << "deviceMacAddrHigh = "  << ((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr)->deviceMacAddrHigh;
                qDebug() << "deviceMacAddrLow = "  << ((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr)->deviceMacAddrLow;
                qDebug() << "deviceMode = "  << ((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr)->deviceMode;
                qDebug() << "deviceVersion = "  << ((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr)->deviceVersion;
                qDebug() << "specVersionMajor = "  << ((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr)->specVersionMajor;
                qDebug() << "specVersionMinor = "  << ((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr)->specVersionMinor;
                qDebug() << "ipConfigCurrent = "  << ((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr)->ipConfigCurrent;
                qDebug() << "ipConfigOptions = "  << ((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr)->ipConfigOptions;
                qDebug() << "serialNumber = "  << QByteArray::fromRawData((char *)((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr)->serialNumber, 16);
                qDebug() << "userDefinedName = "  << QByteArray::fromRawData((char *)((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr)->userDefinedName, 16);
            }
            delete((strGvcpAckDiscoveryHdr *)discHdr.cmdSpecificAckHdr);
            break;

        case '1':
            if (xmlRead) {
                error = cam_api->cameraReadCameraAttribute(featureList, XmlDoc, destAddr, regValues);
                if (error) {
                    qDebug() << "Command failed";
                } else {
                    for (int i = 0; i < featureList.count(); i++) {
                        if (!featureList.at(i).isNull()) {
                            qDebug() << featureList.at(i) << " = " << regValues.at(i);
                        }
                    }
                }
                regValues.clear();
            }
            else
                qDebug() << "Fetch xml file from device first";
            break;

        case '2':
            error = cam_api->cameraWriteRegisterValue(destAddr, writeUnit);
            if (error) {
                qDebug() << "Command failed";
            } else {
                qDebug() << "Registers written successfully.";
            }
            break;

        case '3':
            error = cam_api->cameraReadXmlFileFromDevice(XmlDoc, destAddr);
            if (error) {
                qDebug() << "Command failed";
            } else {
                xmlRead = true;
                qDebug() << "XML file read successfully.";
            }
            break;

        case '4':
            break;

        }
    }

    qDebug() << "Error occured. Exiting program.";
    delete(cam_api);
    return a.exec();
}
