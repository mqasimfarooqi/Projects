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
    QHostAddress hostAddr;
    QList<QString> featureList;
    QDomDocument XmlDoc;
    QList<strGvcpCmdWriteRegHdr> writeUnit;
    bool error = false;
    char option;
    QList<QByteArray> camAttributes;
    QList<quint32> regValues;
    QList<strGvcpCmdReadRegHdr> regAddresses;
    QList<strGvcpAckDiscoveryHdr> discHeaders;
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

    hostAddr = QHostAddress("192.168.10.3");
    cheetah = new cameraApi(hostAddr);

    cheetah->cameraDiscoverDevice(QHostAddress("255.255.255.255"), discHeaders);
    cheetah->cameraInitializeDevice(QHostAddress("192.168.10.8"));
    cheetah->cameraStartStream();
    a.exec();

    while(!error) {

        option = std::getchar();

        switch (option) {
        case '0':
            discHeaders.clear();
            error = cheetah->cameraDiscoverDevice(QHostAddress("255.255.255.255"), discHeaders);
            if (error) {
                qInfo() << "Command failed";
            } else {
                for (int counter = 0; counter < discHeaders.count(); counter++) {
                    qInfo() << "currentIp = "  << discHeaders.at(counter).currentIp;
                    qInfo() << "defaultGateway = "  << discHeaders.at(counter).defaultGateway;
                    qInfo() << "deviceMacAddrHigh = "  << discHeaders.at(counter).deviceMacAddrHigh;
                    qInfo() << "deviceMacAddrLow = "  << discHeaders.at(counter).deviceMacAddrLow;
                    qInfo() << "deviceMode = "  << discHeaders.at(counter).deviceMode;
                    qInfo() << "specVersionMajor = "  << discHeaders.at(counter).specVersionMajor;
                    qInfo() << "specVersionMinor = "  << discHeaders.at(counter).specVersionMinor;
                    qInfo() << "ipConfigCurrent = "  << discHeaders.at(counter).ipConfigCurrent;
                    qInfo() << "ipConfigOptions = "  << discHeaders.at(counter).ipConfigOptions;
                    qInfo() << "userDefinedName = "  << QByteArray::fromRawData((char *)&discHeaders.at(counter).userDefinedName, 16);
                    qInfo() << "serialNumber = "  << QByteArray::fromRawData((char *)&discHeaders.at(counter).serialNumber, 16);
                    qInfo() << "userDefinedName = "  << QByteArray::fromRawData((char *)&discHeaders.at(counter).userDefinedName, 16);
                    qInfo() << "deviceVersion = "  << QByteArray::fromRawData((char *)&discHeaders.at(counter).deviceVersion, 32);
                    qInfo() << "manufacturerName = "  << QByteArray::fromRawData((char *)&discHeaders.at(counter).manufacturerName, 32);
                    qInfo() << "modelName = "  << QByteArray::fromRawData((char *)&discHeaders.at(counter).modelName, 32);
                    qInfo() << "manufacturerSpecificInfo" << QByteArray::fromRawData((char *)&discHeaders.at(counter).manufacturerSpecificInfo, 48);
                }
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
            error = cheetah->cameraStartStream();
            a.exec();

            break;

        }
    }

    qInfo() << "Error occured. Exiting program.";
    delete(cheetah);

    return a.exec();
}
