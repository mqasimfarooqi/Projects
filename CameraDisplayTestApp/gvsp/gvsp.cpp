#include "gvsp.h"
#include "camerainterface/caminterface.h"

/* Helper functions. */
qint32 gvspPopulateGenericDataHdrFromBigEndian(const quint8 *dataPtr, strGvspDataBlockHdr& hdr) {
    bool error = false;
    hdr.status = qFromBigEndian(*(quint16 *)(dataPtr + strGvspDataBlockHdrSTATUS));
    hdr.blockId$flag = qFromBigEndian(*(quint16 *)(dataPtr + strGvspDataBlockHdrBLOCKID$FLAG));
    hdr.extId_res_pktFmt_pktID$res = qFromBigEndian(*(quint32 *)(dataPtr + strGvspDataBlockHdrEXTIDRESPKTFMTPKTIDRES));
    return error;
}

qint32 gvspPopulateGenericDataExtensionHdrFromBigEndian(const quint8 *dataPtr, strGvspDataBlockExtensionHdr& hdr) {
    bool error = false;
    hdr.extBlockId = qFromBigEndian(*(quint64 *)(dataPtr + strGvspDataBlockExtensionHdrEXTBLOCKID));
    hdr.extPacketId = qFromBigEndian(*(quint32 *)(dataPtr + strGvspDataBlockExtensionHdrEXTPACKETID));
    return error;
}

qint32 gvspPopulateImageLeaderHdrFromBigEndian(const quint8 *dataPtr, strGvspImageDataLeaderHdr& hdr) {
    bool error = false;
    hdr.fieldId$fieldCount = *(quint8 *)(dataPtr + strGvspImageDataLeaderHdrFIELDID$FIELDCOUNT);
    hdr.reserved = *(quint8 *)(dataPtr + strGvspImageDataLeaderHdrRESERVED);
    hdr.payloadType = qFromBigEndian(*(quint16 *)(dataPtr + strGvspImageDataLeaderHdrPAYLOADTYPE));
    hdr.timestampHigh = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataLeaderHdrTIMESTAMPHIGH));
    hdr.timestampLow = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataLeaderHdrTIMESTAMPLOW));
    hdr.pixelFormat = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataLeaderHdrPIXELFORMAT));
    hdr.sizeX = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataLeaderHdrSIZEX));
    hdr.sizeY = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataLeaderHdrSIZEY));
    hdr.offsetX = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataLeaderHdrOFFSETX));
    hdr.offsetY = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataLeaderHdrOFFSETY));
    hdr.paddingX = qFromBigEndian(*(quint16 *)(dataPtr + strGvspImageDataLeaderHdrPADDINGX));
    hdr.paddingY = qFromBigEndian(*(quint16 *)(dataPtr + strGvspImageDataLeaderHdrPADDINGY));
    return error;
}

qint32 gvspPopulateImageTrailerHdrFromBigEndian(const quint8 *dataPtr, strGvspImageDataTrailerHdr& hdr) {
    bool error = false;
    hdr.reserved = qFromBigEndian(*(quint16 *)(dataPtr + strGvspImageDataTrailerHdrRESERVED));
    hdr.payloadType = qFromBigEndian(*(quint16 *)(dataPtr + strGvspImageDataTrailerHdrPAYLOADTYPE));
    hdr.sizeY = qFromBigEndian(*(quint32 *)(dataPtr + strGvspImageDataTrailerHdrSIZEY));
    return error;
}
