#ifndef GVSPHEADERS_H
#define GVSPHEADERS_H

#include <QObject>

enum {
    strGvspDataBlockHdrSTATUS = 0,
    strGvspDataBlockHdrBLOCKID$FLAG = 2,
    strGvspDataBlockHdrEXTIDRESPKTFMTPKTIDRES = 4,
};

enum {
    strGvspDataBlockExtensionHdrEXTBLOCKID = 0,
    strGvspDataBlockExtensionHdrEXTPACKETID = 8
};

enum {
    strGvspGenericDataLeaderHdrPAYLOADTYPESPECIFIC = 0,
    strGvspGenericDataLeaderHdrPAYLOADTYPE = 2
};

enum {
    strGvspGenericDataTrailerHdrRESERVED = 0,
    strGvspGenericDataTrailerHdrPAYLOADTYPE = 2
};

enum {
    strGvspImageDataLeaderHdrFIELDID$FIELDCOUNT = 0,
    strGvspImageDataLeaderHdrRESERVED = 1,
    strGvspImageDataLeaderHdrPAYLOADTYPE = 2,
    strGvspImageDataLeaderHdrTIMESTAMPHIGH = 4,
    strGvspImageDataLeaderHdrTIMESTAMPLOW = 8,
    strGvspImageDataLeaderHdrPIXELFORMAT = 12,
    strGvspImageDataLeaderHdrSIZEX = 16,
    strGvspImageDataLeaderHdrSIZEY = 20,
    strGvspImageDataLeaderHdrOFFSETX = 24,
    strGvspImageDataLeaderHdrOFFSETY = 28,
    strGvspImageDataLeaderHdrPADDINGX = 32,
    strGvspImageDataLeaderHdrPADDINGY = 36
};

enum {
    strGvspImageDataTrailerHdrRESERVED = 0,
    strGvspImageDataTrailerHdrPAYLOADTYPE = 2,
    strGvspImageDataTrailerHdrSIZEY = 4
};

/* Generic GVSP header. */
; // To remove compiler warning (Bug with clang)
#pragma pack(push, 1)
typedef struct {
    quint16 status;
    quint16 blockId$flag;
    quint32 extId_res_pktFmt_pktID$res;

} strGvspDataBlockHdr;
#pragma pack(pop)

/* Extension of GVSP header. */
#pragma pack(push, 1)
typedef struct {
    quint64 extBlockId;
    quint32 extPacketId;

} strGvspDataBlockExtensionHdr;
#pragma pack(pop)

/* Generic data leader header. */
#pragma pack(push, 1)
typedef struct {
    quint16 payloadTypeSpecific;
    quint16 payloadType;

} strGvspGenericDataLeaderHdr;
#pragma pack(pop)

/* Generic data trailer header. */
#pragma pack(push, 1)
typedef struct {
    quint16 reserved;
    quint16 payloadType;

} strGvspGenericDataTrailerHdr;
#pragma pack(pop)

/* Image data leader header. */
typedef struct {
    quint8 fieldId$fieldCount;
    quint8 reserved;
    quint16 payloadType;
    quint32 timestampHigh;
    quint32 timestampLow;
    quint32 pixelFormat;
    quint32 sizeX;
    quint32 sizeY;
    quint32 offsetX;
    quint32 offsetY;
    quint16 paddingX;
    quint16 paddingY;

} strGvspImageDataLeaderHdr;

/* Image data trailer header. */
typedef struct {
    quint16 reserved;
    quint16 payloadType;
    quint32 sizeY;

} strGvspImageDataTrailerHdr;

#endif // GVSPHEADERS_H
