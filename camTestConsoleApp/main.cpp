#include <QCoreApplication>
#include "cameraapi.h"

#ifdef GIT_TRACKED
const QString gitCommitHash = GIT_COMMIT_HASH;
const QString gitCommitDate = GIT_COMMIT_DATE;
const QString gitBranch = GIT_BRANCH;
const QString buildDate = __DATE__;
const QString buildTime = __TIME__;
#endif

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    cameraApi *cam_api;
    QUdpSocket m_sock;
    QHostAddress bindAddr, destAddr;
    QList<QString> featureList;
    strGvcpAckDiscoveryHdr discHdr;
    QDomDocument XmlDoc;
    QList<strGvcpCmdWriteRegHdr> writeUnit;
    bool error = false, xmlRead = false;
    char option;
    QVector<quint8> pendingReqVec;
    QList<QByteArray> camAttributes;
    QList<quint32> regValues;
    QList<strGvcpCmdReadRegHdr> regAddresses;
    strGvcpCmdWriteRegHdr writeReg;
    strGvcpCmdReadRegHdr readReg;

#ifdef GIT_TRACKED
    qDebug() << "Git Commit Hash = " << gitCommitHash;
    qDebug() << "Git Commit Date = " << gitCommitDate;
    qDebug() << "Git Branch = " << gitBranch;
    qDebug() << "Build Date = " << buildDate;
    qDebug() << "Build Time = " << buildTime;
    qDebug() << "";
#else
    qDebug() << "This project is not tracked.";
#endif

    cam_api = new cameraApi(&m_sock, &pendingReqVec);

    featureList.append("DeviceModelName");
    featureList.append("DeviceManufacturerInfo");
    featureList.append("DeviceVersion");
    featureList.append("DeviceSerialNumber");
    featureList.append("CameraAttributesReg");
    featureList.append("TriggerOverlapReg");
    featureList.append("TriggerTypeReg");
    featureList.append("DeviceUserID");
    featureList.append("BinningVerticalReg");
    featureList.append("DecimationHorizontalReg");
    featureList.append("FixedFramePeriodValueReg");
    featureList.append("MinFrameTimeReg");
    featureList.append("CurrentFrameTimeReg");
    featureList.append("CurrentFrameReadOutTimeLinesReg");
    featureList.append("CurrentLineTimePClocksReg");
    featureList.append("CurrentLineTimeSecondsReg");
    featureList.append("TriggerModeReg");
    featureList.append("TriggerSoftwareReg");

    readReg.registerAddress = 0xa00;
    regAddresses.append(readReg);

    readReg.registerAddress = 0x960;
    regAddresses.append(readReg);

    readReg.registerAddress = 0x0A8;
    regAddresses.append(readReg);

    readReg.registerAddress = 0x934;
    regAddresses.append(readReg);

    /* Bind UDP socket to an address. */
    bindAddr.setAddress("192.168.10.3");
    if (!(m_sock.bind(bindAddr, 55455))) {

        qDebug() << "Could not bind socket to an address/port.";
        error = true;
    }

    writeReg.registerAddress = 0xa00;
    writeReg.registerData = 0x2;
    writeUnit.append(writeReg);

    writeReg.registerAddress = 0xb00;
    writeReg.registerData = 0xa9da;
    writeUnit.append(writeReg);

    writeReg.registerAddress = 0xb00;
    writeReg.registerData = 0xa9da;
    writeUnit.append(writeReg);

    /* Set address where the packet should be sent. */
    destAddr.setAddress("192.168.10.18");

    qDebug() << "0 -> Test discover device";
    qDebug() << "1 -> Test read attribute command";
    qDebug() << "2 -> Test write register command";
    qDebug() << "3 -> Test read xml file";
    qDebug() << "4 -> Test read register command";
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
                qDebug() << "currentIp = "  << discHdr.currentIp;
                qDebug() << "defaultGateway = "  << discHdr.defaultGateway;
                qDebug() << "deviceMacAddrHigh = "  << discHdr.deviceMacAddrHigh;
                qDebug() << "deviceMacAddrLow = "  << discHdr.deviceMacAddrLow;
                qDebug() << "deviceMode = "  << discHdr.deviceMode;
                qDebug() << "specVersionMajor = "  << discHdr.specVersionMajor;
                qDebug() << "specVersionMinor = "  << discHdr.specVersionMinor;
                qDebug() << "ipConfigCurrent = "  << discHdr.ipConfigCurrent;
                qDebug() << "ipConfigOptions = "  << discHdr.ipConfigOptions;
                qDebug() << "userDefinedName = "  << QByteArray::fromRawData((char *)&discHdr.userDefinedName, 16);
                qDebug() << "serialNumber = "  << QByteArray::fromRawData((char *)&discHdr.serialNumber, 16);
                qDebug() << "userDefinedName = "  << QByteArray::fromRawData((char *)&discHdr.userDefinedName, 16);
                qDebug() << "deviceVersion = "  << QByteArray::fromRawData((char *)&discHdr.deviceVersion, 32);
                qDebug() << "manufacturerName = "  << QByteArray::fromRawData((char *)&discHdr.manufacturerName, 32);
                qDebug() << "modelName = "  << QByteArray::fromRawData((char *)&discHdr.modelName, 32);
                qDebug() << "manufacturerSpecificInfo" << QByteArray::fromRawData((char *)&discHdr.manufacturerSpecificInfo, 48);
            }
            break;

        case '1':
            if (xmlRead) {
                error = cam_api->cameraReadCameraAttribute(featureList, XmlDoc, destAddr, camAttributes);
                if (error) {
                    qDebug() << "Command failed";
                } else {
                    for (int i = 0; i < camAttributes.count(); i++) {
                        if (!camAttributes.at(i).isEmpty()) {
                            qDebug() << featureList.at(i) << " = " << camAttributes.at(i);
                        }
                    }
                }
                camAttributes.clear();
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
            error = cam_api->cameraReadRegisterValue(destAddr, regAddresses, regValues);
            for (int i = 0; i < regAddresses.count(); i++) {
                qDebug() << regAddresses.at(i).registerAddress << " = " << regValues.at(i);
            }
            regValues.clear();

            break;

        }
    }

    qDebug() << "Error occured. Exiting program.";
    delete(cam_api);
    return a.exec();
}
