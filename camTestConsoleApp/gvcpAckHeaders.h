#ifndef GVCPACKHEADERS_H
#define GVCPACKHEADERS_H

#include <QObject>
#include <QtEndian>

/* Generic acknowledgement header. */
; //Just to remove the warning (Bug with clang)
#pragma pack(push, 1)
typedef struct {

    quint16 status;
    quint16 acknowledge;
    quint16 length;
    quint16 ackId;

} strGvcpAckHdr;
#pragma pack(pop)

/* Discovery acknowledgement header. */
#pragma pack(push, 1)
typedef struct {

    quint16 specVersionMajor;
    quint16 specVersionMinor;
    quint32 deviceMode;
    quint16 reserved0;
    quint16 deviceMacAddrHigh;
    quint32 deviceMacAddrLow;
    quint32 ipConfigOptions;
    quint32 ipConfigCurrent;
    quint32 reserved1[3];
    quint32 currentIp;
    quint32 reserved2[3];
    quint32 currentSubnetMask;
    quint32 reserved3[3];
    quint32 defaultGateway;
    quint32 manufacturerName[8];
    quint32 modelName[8];
    quint32 deviceVersion[8];
    quint32 manufacturerSpecificInfo[12];
    quint32 serialNumber[4];
    quint32 userDefinedName[4];

} strGvcpAckDiscoveryHdr;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {

    quint32 address;
    QByteArray data;

} strGvcpAckMemReadHdr;
#pragma pack(pop)

/* Read register acknowledgement header. */
#pragma pack(push, 1)
typedef struct {

    QByteArray registerData;

} strGvcpAckRegReadHdr;
#pragma pack(pop)

#endif // GVCPACKHEADERS_H
