#ifndef GVCPHEADERS_H
#define GVCPHEADERS_H

#include <QObject>
#include <QtEndian>
#include "gvcpAckHeaders.h"
#include "gvcpCmdHeaders.h"

/* Device XML related defines. */
#define DEVICE_MANIFEST_TABLE_ADDRESS   (0x0934)
#define DEVICE_FIRST_URL_ADDRESS        (0X0200)
#define DEVICE_SECOND_URL_ADDRESS       (0x0400)
#define DEVICE_URL_ADDRESS_REG_LENGTH   (512)

/* Default GVP Port. */
#define GVCP_DEFAULT_UDP_PORT (3956)

/* Hardcoded keycode for message validation. */
#define GVCP_CMD_HARDCODED_KEYCODE (0x42)

/* Command and acknowledge IDs */
#define GVCP_DISCOVERY_CMD (0x0002)
#define GVCP_DISCOVERY_ACK (0x0003)
#define GVCP_FORCEIP_CMD (0x0004)
#define GVCP_FORCEIP_ACK (0x0005)
#define GVCP_PACKETRESEND_CMD (0x0040)
#define GVCP_PACKETRESEND_ACK (0x0041)
#define GVCP_READREG_CMD (0x0080)
#define GVCP_READREG_ACK (0x0081)
#define GVCP_WRITEREG_CMD (0x0082)
#define GVCP_WRITEREG_ACK (0x0083)
#define GVCP_READMEM_CMD (0x0084)
#define GVCP_READMEM_ACK (0x0085)
#define GVCP_WRITEMEM_CMD (0x0086)
#define GVCP_WRITEMEM_ACK (0x0087)
#define GVCP_PENDING_ACK (0x0089)
#define GVCP_EVENT_CMD (0x00C0)
#define GVCP_EVENT_ACK (0x00C1)
#define GVCP_EVENTDATA_CMD (0x00C2)
#define GVCP_EVENTDATA_ACK (0x00C3)
#define GVCP_ACTION_CMD (0x0100)
#define GVCP_ACTION_ACK (0x0101)

/* General program defines. */
#define BIT(x) (1 << x)
#define MAX_ACK_FETCH_RETRY_COUNT (3)
#define GVCP_MAX_PAYLOAD_LENGTH (0x218)

/* Command header flag bit fields. */
#define GVCP_CMD_FLAG_ACKNOWLEDGE BIT(7)
#define GVCP_CMD_FLAG_DISCOVERY_BROADCAST_ACK BIT(3)

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
