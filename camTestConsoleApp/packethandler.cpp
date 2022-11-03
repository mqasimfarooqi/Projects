#include "packethandler.h"
#include "camapi.h"
#include "gvsp/gvsp.h"

PacketHandler::PacketHandler(QHash<quint16, QHash<quint32, quint8*>> *streamHT, quint16 streamPktSize,
                             QReadWriteLock *lock, quint32 expectedImageSize, QList<quint8*> rawDataBuffer,
                             QObject *parent) : QObject{ parent } {
    mStreamHashTablePtr = streamHT;
    mPktSize = streamPktSize;
    mLockPtr = lock;
    mExpectedImageSize = expectedImageSize;
    mRawDataBuffer = rawDataBuffer;
}

void PacketHandler::slotServicePendingPackets(QNetworkDatagram gvspPkt) {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
    strGvspDataBlockHdr streamHdr;
    strGvspGenericDataLeaderHdr leader;
    strGvspGenericDataTrailerHdr trailer;
    strGvspDataBlockExtensionHdr extHdr;
    strGvspImageDataLeaderHdr imgLeaderHdr;
    strGvspImageDataTrailerHdr imgTrailerHdr;
    QByteArray dataArr;
    quint8 *dataPtr, *bufferPtr;
    quint32 expectedNoOfPackets;
    quint32 tempValue;
    bool tempBool;
    QHash<quint32, quint8*> frameHT;

    if (!gvspPkt.isNull()) {
        dataArr = gvspPkt.data();
        dataPtr = (quint8 *)dataArr.data();

        /* Populate generic stream header. */
        error = gvspPopulateGenericDataHdrFromBigEndian(dataPtr, streamHdr);
        if (!error) {
            dataPtr += sizeof(strGvspDataBlockHdr);
        } else {
            qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Invalid generic data header received.";
        }

        /* Parse extended generic header if it is preasent. */
        if (streamHdr.extId_res_pktFmt_pktID$res & GVSP_DATA_BLOCK_HDR_EI_RES_PKTFMT_PKTID_EI) {

            error = gvspPopulateGenericDataExtensionHdrFromBigEndian(dataPtr, extHdr);
            if (!error) {
                dataPtr += sizeof(strGvspDataBlockExtensionHdr);
            } else {
                qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Invalid generic data extension header received.";
            }
        }

        /* Switch on the bases of packet format field. */
        switch ((streamHdr.extId_res_pktFmt_pktID$res & GVSP_DATA_BLOCK_HDR_EI_RES_PKTFMT_PKTID_PKTFMT)
                >> GVSP_DATA_BLOCK_HDR_EI_RES_PKTFMT_PKTID_PKTFMT_SHIFT) {

        case GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_LEADER_FORMAT:

            /* Parse the leader. */
            leader.payloadTypeSpecific = qFromBigEndian(*(quint16 *)(dataPtr + strGvspGenericDataLeaderHdrPAYLOADTYPESPECIFIC));
            leader.payloadType = qFromBigEndian(*(quint16 *)(dataPtr + strGvspGenericDataLeaderHdrPAYLOADTYPE));

            /* Switch on leader payload type. */
            switch (leader.payloadType) {
            case GVSP_DATA_BLOCK_HDR_PAYLOAD_TYPE_IMAGE:

                /* Parse image leader header. */
                error = gvspPopulateImageLeaderHdrFromBigEndian(dataPtr, imgLeaderHdr);
                if (!error) {

                    /* Calculate the expected number of packets. */
                    expectedNoOfPackets = ((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) /
                                            (mPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE));
                    ((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) % (mPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)) ?
                                expectedNoOfPackets++ : expectedNoOfPackets;

                    /* Adding 2 for header and trailer. */
                    expectedNoOfPackets += 2;

                    /* Copy data into the buffer. */
                    mLockPtr->lockForWrite();
                    bufferPtr = mRawDataBuffer[streamHdr.blockId$flag % CAMERA_MAX_FRAME_BUFFER_SIZE];
                    memcpy(bufferPtr, &imgLeaderHdr, sizeof(strGvspImageDataLeaderHdr));

                    /* Make a new frame entry for the hash table. */
                    frameHT.insert(streamHdr.extId_res_pktFmt_pktID$res & 0x00FFFFFF, bufferPtr);
                    frameHT.reserve(expectedNoOfPackets);

                    /* Insert newly made entry into the hash table. */
                    mStreamHashTablePtr->insert(streamHdr.blockId$flag, frameHT);
                    mLockPtr->unlock();

                    /* Clean up entries from the hash table. */
                    HashTableCleanup(CAMERA_MAX_FRAME_BUFFER_SIZE);

                } else {
                    qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Invalid image leader header received.";
                }
                break;
            }
            break;
        case GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_TRAILER_FORMAT:

            /* Parse trailer. */
            trailer.reserved = qFromBigEndian(*(quint16 *)(dataPtr + strGvspGenericDataTrailerHdrRESERVED));
            trailer.payloadType = qFromBigEndian(*(quint16 *)(dataPtr + strGvspGenericDataTrailerHdrPAYLOADTYPE));

            switch (trailer.payloadType) {
            case GVSP_DATA_BLOCK_HDR_PAYLOAD_TYPE_IMAGE:

                /* Parse image trailer. */
                error = gvspPopulateImageTrailerHdrFromBigEndian(dataPtr, imgTrailerHdr);
                if (!error) {

                    /* Check to see if an entry already exists in the hash table. */
                    mLockPtr->lockForRead();
                    tempBool = mStreamHashTablePtr->contains(streamHdr.blockId$flag);
                    mLockPtr->unlock();

                    if (tempBool) {

                        /* Copy data into the buffer. */
                        mLockPtr->lockForWrite();
                        bufferPtr = mRawDataBuffer[streamHdr.blockId$flag % CAMERA_MAX_FRAME_BUFFER_SIZE];
                        mStreamHashTablePtr->find(streamHdr.blockId$flag)->insert(streamHdr.extId_res_pktFmt_pktID$res & 0x00FFFFFF, bufferPtr + mExpectedImageSize);
                        mLockPtr->unlock();

                        /* Extract image leader from the frame retrieved from hash table. */
                        memcpy(&imgLeaderHdr, mStreamHashTablePtr->find(streamHdr.blockId$flag)->value(0), sizeof(strGvspImageDataLeaderHdr));

                        /* Calculate the expected number of packets. */
                        expectedNoOfPackets = ((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) /
                                                (mPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE));
                        ((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) % (mPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)) ?
                                    expectedNoOfPackets++ : expectedNoOfPackets;

                        /* Adding 2 for header and trailer. */
                        expectedNoOfPackets += 2;

                        mLockPtr->lockForRead();
                        tempValue = ((quint32)mStreamHashTablePtr->find(streamHdr.blockId$flag)->count());
                        mLockPtr->unlock();

                        if (tempValue < expectedNoOfPackets) {
#if (CAMERA_API_ENABLE_RESEND == 1)
                            emit signalRequestResend(streamHdr.blockId$flag);
#endif
                        } else {
                            qInfo() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Packet received completely with block ID = " << streamHdr.blockId$flag <<
                                       " by " << QThread::currentThread();

                            mLockPtr->lockForRead();
                            frameHT = mStreamHashTablePtr->find(streamHdr.blockId$flag).value();
                            mLockPtr->unlock();

                            /* Emit signal with pointer to first packet data. */
                            emit signalImageAcquisitionComplete(frameHT[1]);

                            mLockPtr->lockForWrite();
                            mStreamHashTablePtr->remove(streamHdr.blockId$flag);
                            mLockPtr->unlock();
                        }
                    }
                } else {
                    qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Invalid image trailer header received.";
                }
                break;
            }
            break;
        case GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_PAYLOAD_FORMAT_GENERIC:
            bufferPtr = mRawDataBuffer[streamHdr.blockId$flag % CAMERA_MAX_FRAME_BUFFER_SIZE];

            mLockPtr->lockForWrite();
            if (mStreamHashTablePtr->contains(streamHdr.blockId$flag) && (dataArr.size() > (int)sizeof(strGvspDataBlockHdr))) {
                /*((streamHdr.extId_res_pktFmt_pktID$res & 0x00FFFFFF) - 1) => -1 is is because actual image data starts from first index. */
                memcpy(bufferPtr +
                       sizeof(strGvspImageDataLeaderHdr) +
                       (((streamHdr.extId_res_pktFmt_pktID$res & 0x00FFFFFF) - 1) * (mPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)),
                       dataPtr, dataArr.count() - GVSP_HEADER_SIZE);
                mStreamHashTablePtr->find(streamHdr.blockId$flag)->insert(streamHdr.extId_res_pktFmt_pktID$res & 0x00FFFFFF, bufferPtr);
            }
            mLockPtr->unlock();
            break;
        }
    }
}

void PacketHandler::HashTableCleanup(quint32 entries) {
    bool tempBool;
    quint16 leastBlockID;
    QList<quint16> keys;

    /* Following block removes the first entry if max enteries become greater than CAMERA_FRAME_BUFFER_MAXSIZE. */
    mLockPtr->lockForRead();
    tempBool = (quint32)mStreamHashTablePtr->count() > entries;
    if (tempBool == true) {
        keys = mStreamHashTablePtr->keys();
    }
    mLockPtr->unlock();

    if (tempBool == true) {
        leastBlockID = keys.constFirst();
        for (int counter = 0; counter < keys.size(); counter++) {
            if (keys.at(counter) < leastBlockID) {
                leastBlockID = keys.at(counter);
            }
        }

        mLockPtr->lockForWrite();
        mStreamHashTablePtr->remove(leastBlockID);
        mLockPtr->unlock();
    }
}
