#include "gvsp.h"
#include "caminterface.h"

strGvspDataBlockHdr gvspPopulateGenericDataHdrFromNetwork(const quint8 *dataPtr) {
    strGvspDataBlockHdr streamHdr;

    streamHdr.status = qFromBigEndian(*(quint16 *)(dataPtr + strGvspDataBlockHdrSTATUS));
    streamHdr.blockId$flag = qFromBigEndian(*(quint16 *)(dataPtr + strGvspDataBlockHdrBLOCKID$FLAG));
    streamHdr.extId_res_pktFmt_pktID$res = qFromBigEndian(*(quint32 *)(dataPtr + strGvspDataBlockHdrEXTIDRESPKTFMTPKTIDRES));

    return streamHdr;
}

strGvspDataBlockExtensionHdr gvspPopulateGenericDataExtensionHdrFromNetwork(const quint8 *dataPtr) {
    strGvspDataBlockExtensionHdr extHdr;

    extHdr.extBlockId = qFromBigEndian(*(quint64 *)(dataPtr + strGvspDataBlockExtensionHdrEXTBLOCKID));
    extHdr.extPacketId = qFromBigEndian(*(quint32 *)(dataPtr + strGvspDataBlockExtensionHdrEXTPACKETID));

    return extHdr;
}

strGvspImageDataLeaderHdr gvspPopulateImageLeaderHdrFromNetwork(const quint8 *dataPtr) {
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

strGvspImageDataTrailerHdr gvspPopulateImageTrailerHdrFromNetwork(const quint8 *dataPtr) {
    strGvspImageDataTrailerHdr imgTrailerHdr;

    imgTrailerHdr.reserved = qFromBigEndian(*(quint16 *)(dataPtr + strGvspImageDataTrailerHdrRESERVED));
    imgTrailerHdr.payloadType = qFromBigEndian(*(quint16 *)(dataPtr + strGvspImageDataTrailerHdrPAYLOADTYPE));
    imgTrailerHdr.sizeY = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataTrailerHdrSIZEY));

    return imgTrailerHdr;
}
