#ifndef GVCPACKHEADERS_H
#define GVCPACKHEADERS_H

#include <QObject>

enum {

    strGvcpAckHdrSTATUS = 0,
    strGvcpAckHdrACKNOWLEDGE = 2,
    strGvcpAckHdrLENGTH = 4,
    strGvcpAckHdrACKID = 6

};

enum {
    strGvcpAckDiscoveryHdrSPECVERSIONMAJOR = 0,
    strGvcpAckDiscoveryHdrSPECVERSIONMINOR = 2,
    strGvcpAckDiscoveryHdrDEVICEMODE = 4,
    strGvcpAckDiscoveryHdrRESERVED0 = 8,
    strGvcpAckDiscoveryHdrDEVICEMACADDRHIGH = 10,
    strGvcpAckDiscoveryHdrDEVICEMACADDRLOW = 12,
    strGvcpAckDiscoveryHdrIPCONFIGOPTIONS = 16,
    strGvcpAckDiscoveryHdrIPCONFIGCURRENT = 20,
    strGvcpAckDiscoveryHdrRESERVED1 = 24,
    strGvcpAckDiscoveryHdrCURRENTIP = 36,
    strGvcpAckDiscoveryHdrRESERVED2 = 40,
    strGvcpAckDiscoveryHdrCURRENTSUBNETMASK = 52,
    strGvcpAckDiscoveryHdrRESERVED3 = 56,
    strGvcpAckDiscoveryHdrDEFAULTGATEWAY = 68,
    strGvcpAckDiscoveryHdrMANUFACTURERNAME = 72,
    strGvcpAckDiscoveryHdrMODELNAME = 104,
    strGvcpAckDiscoveryHdrDEVICEVERSION = 136,
    strGvcpAckDiscoveryHdrMANUFACTURERSPECIFICINFO = 168,
    strGvcpAckDiscoveryHdrSERIALNUMBER = 216,
    strGvcpAckDiscoveryHdrUSERDEFINEDNAME = 232
};

enum {
    strGvcpAckMemReadHdrADDRESS = 0,
    strGvcpAckMemReadHdrDATA = 4
};

enum {
    strGvcpAckRegReadHdrREGISTERDATA = 0
};

enum {
    strGvcpAckRegWriteHdrRESERVED = 0,
    strGvcpAckRegWriteHdrINDEX = 2
};

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
    quint32 reserved1[4];
    quint32 currentIp;
    quint32 reserved2[4];
    quint32 currentSubnetMask;
    quint32 reserved3[4];
    quint32 defaultGateway;
    quint8 manufacturerName[32];
    quint8 modelName[32];
    quint8 deviceVersion[32];
    quint8 manufacturerSpecificInfo[48];
    quint8 serialNumber[16];
    quint8 userDefinedName[16];

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

    quint32 *registerData;

} strGvcpAckRegReadHdr;
#pragma pack(pop)

/* Write register acknowledgement header. */
#pragma pack(push, 1)
typedef struct {

    quint16 reserved;
    quint16 index;

} strGvcpAckRegWriteHdr;
#pragma pack(pop)

#endif // GVCPACKHEADERS_H
