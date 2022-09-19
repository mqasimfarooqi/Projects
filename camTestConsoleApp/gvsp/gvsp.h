#ifndef GVSP_H
#define GVSP_H

#include <QUdpSocket>
#include "gvsp/gvspHeaders.h"

#define GVSP_PAYLOAD_IMAGE                  (0x0001)
#define GVSP_PAYLOAD_RAWDATA                (0x0002)
#define GVSP_PAYLOAD_FILE                   (0x0003)
#define GVSP_PAYLOAD_CHUNKDATA              (0x0004)
#define GVSP_PAYLOAD_EXTENDEDCHUNKDATA      (0x0005)
#define GVSP_PAYLOAD_JPEG                   (0x0006)
#define GVSP_PAYLOAD_JPEG2000               (0x0007)
#define GVSP_PAYLOAD_H264                   (0x0008)
#define GVSP_PAYLOAD_MULTIZONEIMAGE         (0x0009)
#define GVSP_PAYLOAD_MULTIPART              (0x000A)
#define GVSP_PAYLOAD_GENDC                  (0x000B)
#define GVSP_PAYLOAD_DEVICE_SPECIFIC_START  (0x8000)

#define GVSP_DATA_BLOCK_HDR_EI_RES_PKTFMT_PKTID_EI                  (0x80000000)
#define GVSP_DATA_BLOCK_HDR_EI_RES_PKTFMT_PKTID_PKTFMT              (0x0F000000)

#define GVSP_DATA_BLOCK_HDR_EI_RES_PKTFMT_PKTID_PKTFMT_SHIFT        (24)
#define GVSP_DATA_BLOCK_HDR_EI_RES_PKTFMT_PKTID_EI_SHIFT            (31)

#define GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_LEADER_FORMAT              (1)
#define GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_TRAILER_FORMAT             (2)
#define GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_PAYLOAD_FORMAT_GENERIC     (3)
#define GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_PAYLOAD_FORMAT_ALLINONE    (4)
#define GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_PAYLOAD_FORMAT_H264        (5)
#define GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_PAYLOAD_FORMAT_MULTIZONE   (6)

#define GVSP_DATA_BLOCK_HDR_PAYLOAD_TYPE_IMAGE                      (0x1)
#define GVSP_DATA_BLOCK_HDR_PAYLOAD_TYPE_RAW_DATA                   (0x2)

bool gvspFetchPacket(QUdpSocket& gvspSocket);

/* Helper functions. */
strGvspDataBlockHdr gvspPopulateGenericDataHdr(const quint8 *dataPointer);

typedef struct {
    quint32 packetId;
    QByteArray packetData;

} strFrameDataUnit;

typedef struct {
    quint16 blockId;
    bool packetsMissing;
    QVector<strFrameDataUnit> frameData;

} strFrame;

#endif // GVSP_H
