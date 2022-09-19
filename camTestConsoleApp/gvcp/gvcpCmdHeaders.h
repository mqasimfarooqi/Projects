#ifndef GVCPCMDHEADERS_H
#define GVCPCMDHEADERS_H

#include <QObject>

enum {
    strGvcpCmdReadMemHdrADDRESS = 0,
    strGvcpCmdReadMemHdrRESERVED = 4,
    strGvcpCmdReadMemHdrCOUNT = 6
};

enum {
    strGvcpCmdPktResendHdrSTREAMCHANNELIDX = 0,
    strGvcpCmdPktResendHdrBLOCKIDRES = 2,
    strGvcpCmdPktResendHdrFIRSTPKTID = 4,
    strGvcpCmdPktResendHdrLASTPKTID = 8
};

/* Generic command header. */
; // To remove compiler warning (Bug with clang)
#pragma pack(push, 1)
typedef struct {
    quint8 keyCode;
    quint8 flag;
    quint16 command;
    quint16 length;
    quint16 reqId;

} strGvcpCmdHdr;
#pragma pack(pop)

/* Read memory command header. */
#pragma pack(push, 1)
typedef struct {
    quint32 address;
    quint16 reserved;
    quint16 count;

} strGvcpCmdReadMemHdr;
#pragma pack(pop)

/* A wite transaction unit inside write register command. */
#pragma pack(push, 1)
typedef struct {
    quint32 registerAddress;
    quint32 registerData;

} strGvcpCmdWriteRegHdr;
#pragma pack(pop)

/* Read register command header. */
#pragma pack(push, 1)
typedef struct {
    quint32 registerAddress;

} strGvcpCmdReadRegHdr;
#pragma pack(pop)

/* Packet resend command header. */
#pragma pack(push, 1)
typedef struct {
    quint16 streamChannelIdx;
    quint16 blockIdRes;
    quint32 firstPktId;
    quint32 lastPktId;

} strGvcpCmdPktResendHdr;
#pragma pack(pop)

#endif // GVCPCMDHEADERS_H
