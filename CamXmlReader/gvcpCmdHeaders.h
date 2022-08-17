#ifndef GVCPCMDHEADERS_H
#define GVCPCMDHEADERS_H

#include <QObject>
#include <QtEndian>

/* Generic command header. */
typedef struct __attribute__ ((__packed__))
{
    quint8 keyCode;
    quint8 flag;
    quint16_be command;
    quint16_be length;
    quint16_be reqId;

} strGvcpCmdHdr;

/* Read memory command header. */
typedef struct __attribute__ ((__packed__))
{
    quint32_be address;
    quint16_be reserved;
    quint16_be count;

} strGvcpCmdReadMemHdr;

/* Write register command header. */
typedef struct __attribute__ ((__packed__))
{
    quint32_be registerAddress;
    quint32_be registerData;

} strGvcpCmdWriteRegHdr;

/* Read register command header. */
typedef struct __attribute__ ((__packed__))
{
    quint32_be  registerAddress;

} strGvcpCmdReadRegHdr;

#endif // GVCPCMDHEADERS_H
