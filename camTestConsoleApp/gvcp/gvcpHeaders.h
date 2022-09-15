#ifndef GVCPHEADERS_H
#define GVCPHEADERS_H

#include <QObject>
#include "gvcpAckHeaders.h"
#include "gvcpCmdHeaders.h"

/* Non standard acknowledgement header. */
typedef struct NonStdGvcpAckHdr{
    strGvcpAckHdr genericAckHdr;
    quint32 ackHdrType;
    void *cmdSpecificAckHdr;

} strNonStdGvcpAckHdr;

/* Non standard command header. */
typedef struct {
    strGvcpCmdHdr genericCmdHdr;
    quint32 cmdSpecificDataLength;
    void *cmdSpecificData;

} strNonStdGvcpCmdHdr;

#endif // GVCPHEADERS_H
