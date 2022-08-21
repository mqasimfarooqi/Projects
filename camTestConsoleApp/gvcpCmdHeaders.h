#ifndef GVCPCMDHEADERS_H
#define GVCPCMDHEADERS_H

#include <QObject>
#include <QtEndian>

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

#pragma pack(push, 1)
/* Read memory command header. */
typedef struct {
    quint32 address;
    quint16 reserved;
    quint16 count;

} strGvcpCmdReadMemHdr;
#pragma pack(pop)

#pragma pack(push, 1)
/* Write register command header. */
typedef struct {
    quint32 registerAddress;
    quint32 registerData;

} strGvcpCmdWriteRegHdr;
#pragma pack(pop)

/* Read register command header. */
#pragma pack(push, 1)
typedef struct {
    quint32 *registerAddress;

} strGvcpCmdReadRegHdr;
#pragma pack(pop)

#endif // GVCPCMDHEADERS_H
