#ifndef GVCPACKHEADERS_H
#define GVCPACKHEADERS_H

#include <QObject>
#include <QtEndian>

/* Generic acknowledgement header. */
typedef struct __attribute__ ((__packed__))
{
    quint16_be status;
    quint16_be acknowledge;
    quint16_be length;
    quint16_be ackId;

} strGvcpAckHdr;

/* Discovery acknowledgement header. */
typedef struct __attribute__ ((__packed__))
{
    quint16_be specVersionMajor;
    quint16_be specVersionMinor;
    quint32_be deviceMode;
    quint16_be reserved0;
    quint16_be deviceMacAddrHigh;
    quint32_be deviceMacAddrLow;
    quint32_be ipConfigOptions;
    quint32_be ipConfigCurrent;
    quint32_be reserved1[3];
    quint32_be currentIp;
    quint32_be reserved2[3];
    quint32_be currentSubnetMask;
    quint32_be reserved3[3];
    quint32_be defaultGateway;
    quint32_be manufacturerName[8];
    quint32_be modelName[8];
    quint32_be deviceVersion[8];
    quint32_be manufacturerSpecificInfo[12];
    quint32_be serialNumber[4];
    quint32_be userDefinedName[4];

} strGvcpAckDiscoveryHdr;

/* Discovery acknowledgement header. */
typedef struct __attribute__ ((__packed__))
{
    quint32_be address;
    QByteArray data;

} strGvcpAckMemReadHdr;

#endif // GVCPACKHEADERS_H
