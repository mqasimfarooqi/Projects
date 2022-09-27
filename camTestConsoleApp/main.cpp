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
    cameraApi *cheetah;
    QUdpSocket m_sock;
    QHostAddress hostAddr, destAddr;
    quint16 hostPort;
    QList<QString> featureList;
    strGvcpAckDiscoveryHdr discHdr;
    QDomDocument XmlDoc;
    QList<strGvcpCmdWriteRegHdr> writeUnit;
    bool error = false;
    char option;
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

    featureList.append("GevCCPReg"); //gigeCCPReg
    featureList.append("GevHeartbeatTimeoutReg"); //gigeHeartBeatReg
    featureList.append("GevSCPHostPortReg"); //gigeStreamChannelPortReg
    featureList.append("GevSCDAReg"); //gigeStreamChannelDestinationReg
    featureList.append("GevSCSPReg"); //gigeStreamChannelSourcePortReg
    featureList.append("GevSCPSPacketSizeReg"); //gigeStreamChannelPacketSizeReg
    featureList.append("GevSCPDReg"); //gigeStreamChannelPacketDelayReg
    featureList.append("AcquisitionModeReg"); //gigeAquisitionModeReg
    featureList.append("AcquisitionStartReg"); //gigeAquisitionStartReg
    featureList.append("AcquisitionStopReg"); //gigeAquisitionStopReg
    featureList.append("OffsetXReg"); //gigeSensorOffSetXReg
    featureList.append("OffsetYReg"); //gigeSensorOffSetYReg
    featureList.append("HeightReg"); //gigeSensorHeightSelectionRegDL
    featureList.append("WidthReg"); //gigeSensorWidthSelectionRegDL

    readReg.registerAddress = 0xa00;
    regAddresses.append(readReg);

    readReg.registerAddress = 0x960;
    regAddresses.append(readReg);

    readReg.registerAddress = 0x0A8;
    regAddresses.append(readReg);

    readReg.registerAddress = 0x934;
    regAddresses.append(readReg);

    writeReg.registerAddress = 0xa00;
    writeReg.registerData = 0x2;
    writeUnit.append(writeReg);

    writeReg.registerAddress = 0xb00;
    writeReg.registerData = 0xa9da;
    writeUnit.append(writeReg);

    writeReg.registerAddress = 0xb00;
    writeReg.registerData = 0xa9da;
    writeUnit.append(writeReg);

    if (!error) {

        qInfo() << "-----------------------------";
        qInfo() << "0 -> Test discover device";
        qInfo() << "1 -> Test read attribute command";
        qInfo() << "2 -> Test write register command";
        qInfo() << "3 -> Test read reg command";
        qInfo() << "4 -> Test start stream command";
        qInfo() << "-----------------------------";

    } else {

        qInfo() << "Unable to initialize camera.";
    }

    hostAddr.setAddress("192.168.10.3");
    destAddr.setAddress("192.168.10.8");
    hostPort = 55967;
    cheetah = new cameraApi(hostAddr, destAddr, hostPort);

    destAddr.setAddress("255.255.255.255");
    cheetah->cameraInitializeDevice();
    cheetah->cameraStartStream(47345);
    a.exec();

    while(!error) {

        option = std::getchar();

        switch (option) {
        case '0':
            error = cheetah->cameraDiscoverDevice(destAddr, discHdr);
            if (error) {
                qInfo() << "Command failed";
            } else {
                qInfo() << "currentIp = "  << discHdr.currentIp;
                qInfo() << "defaultGateway = "  << discHdr.defaultGateway;
                qInfo() << "deviceMacAddrHigh = "  << discHdr.deviceMacAddrHigh;
                qInfo() << "deviceMacAddrLow = "  << discHdr.deviceMacAddrLow;
                qInfo() << "deviceMode = "  << discHdr.deviceMode;
                qInfo() << "specVersionMajor = "  << discHdr.specVersionMajor;
                qInfo() << "specVersionMinor = "  << discHdr.specVersionMinor;
                qInfo() << "ipConfigCurrent = "  << discHdr.ipConfigCurrent;
                qInfo() << "ipConfigOptions = "  << discHdr.ipConfigOptions;
                qInfo() << "userDefinedName = "  << QByteArray::fromRawData((char *)&discHdr.userDefinedName, 16);
                qInfo() << "serialNumber = "  << QByteArray::fromRawData((char *)&discHdr.serialNumber, 16);
                qInfo() << "userDefinedName = "  << QByteArray::fromRawData((char *)&discHdr.userDefinedName, 16);
                qInfo() << "deviceVersion = "  << QByteArray::fromRawData((char *)&discHdr.deviceVersion, 32);
                qInfo() << "manufacturerName = "  << QByteArray::fromRawData((char *)&discHdr.manufacturerName, 32);
                qInfo() << "modelName = "  << QByteArray::fromRawData((char *)&discHdr.modelName, 32);
                qInfo() << "manufacturerSpecificInfo" << QByteArray::fromRawData((char *)&discHdr.manufacturerSpecificInfo, 48);
            }
            break;

        case '1':
            error = cheetah->cameraReadCameraAttribute(featureList, camAttributes);
            if (error) {
                qInfo() << "Command failed";
            } else {
                for (int i = 0; i < camAttributes.count(); i++) {
                    if (!camAttributes.at(i).isEmpty()) {
                        qInfo() << featureList.at(i) << " = " << camAttributes.at(i);
                    }
                }
            }
            camAttributes.clear();
            break;

        case '2':
            error = cheetah->cameraWriteRegisterValue(writeUnit);
            if (error) {
                qInfo() << "Command failed";
            } else {
                qInfo() << "Registers written successfully.";
            }
            break;

        case '3':
            error = cheetah->cameraReadRegisterValue(regAddresses, regValues);
            for (int i = 0; i < regAddresses.count(); i++) {
                qInfo() << regAddresses.at(i).registerAddress << " = " << regValues.at(i);
            }
            regValues.clear();

            break;

        case '4':
            error = cheetah->cameraStartStream(59965);
            a.exec();

            break;

        }
    }

    qInfo() << "Error occured. Exiting program.";
    delete(cheetah);

    return a.exec();
}
