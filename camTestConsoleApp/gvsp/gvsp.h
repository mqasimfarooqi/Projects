#ifndef GVSP_H
#define GVSP_H

#include <QUdpSocket>
#include <QtEndian>
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

#define IP_HEADER_SIZE                                              (20)
#define UDP_HEADER_SIZE                                             (8)
#define GVSP_HEADER_SIZE                                            (8)

bool gvspFetchPacket(QUdpSocket& gvspSocket);

/* Helper functions. */
inline strGvspDataBlockHdr gvspPopulateGenericDataHdrFromNetwork(const quint8 *dataPtr) {
    strGvspDataBlockHdr streamHdr;
    streamHdr.status = qFromBigEndian(*(quint16 *)(dataPtr + strGvspDataBlockHdrSTATUS));
    streamHdr.blockId$flag = qFromBigEndian(*(quint16 *)(dataPtr + strGvspDataBlockHdrBLOCKID$FLAG));
    streamHdr.extId_res_pktFmt_pktID$res = qFromBigEndian(*(quint32 *)(dataPtr + strGvspDataBlockHdrEXTIDRESPKTFMTPKTIDRES));
    return streamHdr;
}
inline strGvspDataBlockExtensionHdr gvspPopulateGenericDataExtensionHdrFromNetwork(const quint8 *dataPtr) {
    strGvspDataBlockExtensionHdr extHdr;
    extHdr.extBlockId = qFromBigEndian(*(quint64 *)(dataPtr + strGvspDataBlockExtensionHdrEXTBLOCKID));
    extHdr.extPacketId = qFromBigEndian(*(quint32 *)(dataPtr + strGvspDataBlockExtensionHdrEXTPACKETID));
    return extHdr;
}
inline strGvspImageDataLeaderHdr gvspPopulateImageLeaderHdrFromNetwork(const quint8 *dataPtr) {
    strGvspImageDataLeaderHdr imgLeaderHdr;
    imgLeaderHdr.fieldId$fieldCount = *(quint8 *)(dataPtr + strGvspImageDataLeaderHdrFIELDID$FIELDCOUNT);
    imgLeaderHdr.reserved = *(quint8 *)(dataPtr + strGvspImageDataLeaderHdrRESERVED);
    imgLeaderHdr.payloadType = qFromBigEndian(*(quint16 *)(dataPtr + strGvspImageDataLeaderHdrPAYLOADTYPE));
    imgLeaderHdr.timestampHigh = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataLeaderHdrTIMESTAMPHIGH));
    imgLeaderHdr.timestampLow = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataLeaderHdrTIMESTAMPLOW));
    imgLeaderHdr.pixelFormat = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataLeaderHdrPIXELFORMAT));
    imgLeaderHdr.sizeX = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataLeaderHdrSIZEX));
    imgLeaderHdr.sizeY = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataLeaderHdrSIZEY));
    imgLeaderHdr.offsetX = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataLeaderHdrOFFSETX));
    imgLeaderHdr.offsetY = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataLeaderHdrOFFSETY));
    imgLeaderHdr.paddingX = qFromBigEndian(*(quint16 *)(dataPtr + strGvspImageDataLeaderHdrPADDINGX));
    imgLeaderHdr.paddingY = qFromBigEndian(*(quint16 *)(dataPtr + strGvspImageDataLeaderHdrPADDINGY));
    return imgLeaderHdr;
}
inline strGvspImageDataTrailerHdr gvspPopulateImageTrailerHdrFromNetwork(const quint8 *dataPtr) {
    strGvspImageDataTrailerHdr imgTrailerHdr;
    imgTrailerHdr.reserved = qFromBigEndian(*(quint16 *)(dataPtr + strGvspImageDataTrailerHdrRESERVED));
    imgTrailerHdr.payloadType = qFromBigEndian(*(quint16 *)(dataPtr + strGvspImageDataTrailerHdrPAYLOADTYPE));
    imgTrailerHdr.sizeY = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataTrailerHdrSIZEY));
    return imgTrailerHdr;
}

#endif // GVSP_H
