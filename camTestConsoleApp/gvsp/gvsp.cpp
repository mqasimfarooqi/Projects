#include "gvsp.h"
#include "caminterface.h"

static QByteArray imageData;

strGvspDataBlockHdr gvspPopulateGenericDataHdr(const quint8 *dataPtr) {
    strGvspDataBlockHdr streamHdr;

    streamHdr.status = qFromBigEndian(*(quint16 *)(dataPtr + strGvspDataBlockHdrSTATUS));
    streamHdr.blockId$flag = qFromBigEndian(*(quint16 *)(dataPtr + strGvspDataBlockHdrBLOCKID$FLAG));
    streamHdr.extId_res_pktFmt_pktID$res = qFromBigEndian(*(quint32 *)(dataPtr + strGvspDataBlockHdrEXTIDRESPKTFMTPKTIDRES));

    return streamHdr;
}

strGvspDataBlockExtensionHdr gvspPopulateGenericDataExtensionHdr(const quint8 *dataPtr) {
    strGvspDataBlockExtensionHdr extHdr;

    extHdr.extBlockId = qFromBigEndian(*(quint64 *)(dataPtr + strGvspDataBlockExtensionHdrEXTBLOCKID));
    extHdr.extPacketId = qFromBigEndian(*(quint32 *)(dataPtr + strGvspDataBlockExtensionHdrEXTPACKETID));

    return extHdr;
}

bool gvspFetchPacket(QUdpSocket& gvspSocket) {
    bool error = false;
    QNetworkDatagram gvspPkt;
    strGvspDataBlockHdr streamHdr;
    strGvspGenericDataLeaderHdr leader;
    strGvspGenericDataTrailerHdr trailer;
    strGvspDataBlockExtensionHdr extHdr;
    strGvspImageDataLeaderHdr imgLeaderHdr;
    strGvspImageDataTrailerHdr imgTrailerHdr;
    QByteArray dataArr;
    quint8 *dataPtr;

    /* Receive the datagram. */
    gvspPkt = gvspSocket.receiveDatagram();
    dataArr = gvspPkt.data();
    dataPtr = (quint8 *)dataArr.data();

    /* Populate generic stream header. */
    streamHdr = gvspPopulateGenericDataHdr(dataPtr);
    dataPtr += sizeof(strGvspDataBlockHdr);

    /* Parse extended generic header if it is preasent. */
    if (streamHdr.extId_res_pktFmt_pktID$res & GVSP_DATA_BLOCK_HDR_EI_RES_PKTFMT_PKTID_EI) {

        extHdr = gvspPopulateGenericDataExtensionHdr(dataPtr);
        dataPtr += sizeof(strGvspDataBlockExtensionHdr);
    }

    /* Switch on the bases of packet format. */
    switch ((streamHdr.extId_res_pktFmt_pktID$res & GVSP_DATA_BLOCK_HDR_EI_RES_PKTFMT_PKTID_PKTFMT)
             >> GVSP_DATA_BLOCK_HDR_EI_RES_PKTFMT_PKTID_PKTFMT_SHIFT) {
    case GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_LEADER_FORMAT:
        leader.payloadTypeSpecific = qFromBigEndian(*(quint16 *)(dataPtr + strGvspGenericDataLeaderHdrPAYLOADTYPESPECIFIC));
        leader.payloadType = qFromBigEndian(*(quint16 *)(dataPtr + strGvspGenericDataLeaderHdrPAYLOADTYPE));

        switch (leader.payloadType) {
        case GVSP_DATA_BLOCK_HDR_PAYLOAD_TYPE_IMAGE:
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
            dataPtr += sizeof(strGvspImageDataLeaderHdr);
            break;
        }
        break;

    case GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_TRAILER_FORMAT:
        trailer.reserved = qFromBigEndian(*(quint16 *)(dataPtr + strGvspGenericDataTrailerHdrRESERVED));
        trailer.payloadType = qFromBigEndian(*(quint16 *)(dataPtr + strGvspGenericDataTrailerHdrPAYLOADTYPE));

        switch (trailer.payloadType) {
        case GVSP_DATA_BLOCK_HDR_PAYLOAD_TYPE_IMAGE:
            imgTrailerHdr.reserved = qFromBigEndian(*(quint16 *)(dataPtr + strGvspImageDataTrailerHdrRESERVED));
            imgTrailerHdr.payloadType = qFromBigEndian(*(quint16 *)(dataPtr + strGvspImageDataTrailerHdrPAYLOADTYPE));
            imgTrailerHdr.sizeY = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataTrailerHdrSIZEY));
            dataPtr += sizeof(strGvspImageDataTrailerHdr);
            imageData.clear();
            break;
        }
        break;

    case GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_PAYLOAD_FORMAT_GENERIC:
        imageData.append((char *)dataPtr + sizeof(strGvspDataBlockHdr), dataArr.count() - sizeof(strGvspDataBlockHdr));
        break;
    }

    return error;
}
