#include "packethandler.h"
#include "cameraapi.h"
#include "gvsp/gvsp.h"

PacketHandler::PacketHandler(QHash<quint16, QHash<quint32, QByteArray>> *streamHT, quint16 streamPktSize,
                             QMutex *mutex, QQueue<quint16> *blockIDQueue, QQueue<QNetworkDatagram> *datagramQueue,
                             QObject *parent) : QObject{ parent } {
    mStreamHashTablePtr = streamHT;
    mQueueFrameResendBlockID = blockIDQueue;
    mPktSize = streamPktSize;
    mQueueDatgrams = datagramQueue;
    mMutexPtr = mutex;
}

void PacketHandler::slotServicePendingPackets() {
    quint32 error = CAMERA_API_STATUS_SUCCESS;
    QNetworkDatagram gvspPkt;
    strGvspDataBlockHdr streamHdr;
    strGvspGenericDataLeaderHdr leader;
    strGvspGenericDataTrailerHdr trailer;
    strGvspDataBlockExtensionHdr extHdr;
    strGvspImageDataLeaderHdr imgLeaderHdr;
    strGvspImageDataTrailerHdr imgTrailerHdr;
    QByteArray dataArr;
    quint8 *dataPtr;
    quint32 expectedNoOfPackets;
    quint16 leastBlockID;
    QList<quint16> keys;
    QHash<quint32, QByteArray> frameHT;

    /* Receive the datagram. */
    mMutexPtr->lock();
    if (!mQueueDatgrams->isEmpty()) {
        //qDebug() << "Packet serviced by " << QThread::currentThread();
        gvspPkt = mQueueDatgrams->dequeue();
    }
    mMutexPtr->unlock();

    if (!gvspPkt.isNull()) {

        dataArr = gvspPkt.data();
        dataPtr = (quint8 *)dataArr.data();

        /* Remove the first entry if max enteries become greater than CAMERA_FRAME_BUFFER_MAXSIZE. */
        mMutexPtr->lock();
        if (mStreamHashTablePtr->count() > CAMERA_MAX_FRAME_BUFFER_SIZE) {
            keys = mStreamHashTablePtr->keys();
            leastBlockID = keys.constFirst();
            for (int counter = 0; counter < keys.size(); counter++) {
                if (keys.at(counter) < leastBlockID) {
                    leastBlockID = keys.at(counter);
                }
            }
            //qDebug() << "Removing hashtable entry with block ID = " << leastBlockID;
            mStreamHashTablePtr->remove(leastBlockID);
        }
        mMutexPtr->unlock();

        /* Populate generic stream header. */
        error = gvspPopulateGenericDataHdrFromBigEndian(dataPtr, streamHdr);
        if (!error) {
            dataPtr += sizeof(strGvspDataBlockHdr);
        } else {
            qDebug() << "Invalid generic data header received.";
        }

        /* Parse extended generic header if it is preasent. */
        if (streamHdr.extId_res_pktFmt_pktID$res & GVSP_DATA_BLOCK_HDR_EI_RES_PKTFMT_PKTID_EI) {

            error = gvspPopulateGenericDataExtensionHdrFromBigEndian(dataPtr, extHdr);
            if (!error) {
                dataPtr += sizeof(strGvspDataBlockExtensionHdr);
            } else {
                qDebug() << "Invalid generic data extension header received.";
            }
        }

        /* Switch on the bases of packet format. */
        switch ((streamHdr.extId_res_pktFmt_pktID$res & GVSP_DATA_BLOCK_HDR_EI_RES_PKTFMT_PKTID_PKTFMT)
                >> GVSP_DATA_BLOCK_HDR_EI_RES_PKTFMT_PKTID_PKTFMT_SHIFT) {
        case GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_LEADER_FORMAT:
            leader.payloadTypeSpecific = qFromBigEndian(*(quint16 *)(dataPtr + strGvspGenericDataLeaderHdrPAYLOADTYPESPECIFIC));
            leader.payloadType = qFromBigEndian(*(quint16 *)(dataPtr + strGvspGenericDataLeaderHdrPAYLOADTYPE));

            switch (leader.payloadType) {
            case GVSP_DATA_BLOCK_HDR_PAYLOAD_TYPE_IMAGE:
                error = gvspPopulateImageLeaderHdrFromBigEndian(dataPtr, imgLeaderHdr);
                if (!error) {
                    expectedNoOfPackets = (((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) /
                                            (mPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)) + 1);
                    ((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) % (mPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)) ?
                                expectedNoOfPackets++ : expectedNoOfPackets;

                    frameHT.insert(streamHdr.extId_res_pktFmt_pktID$res & 0x00FFFFFF,
                                   QByteArray((char *)&imgLeaderHdr, sizeof(strGvspImageDataLeaderHdr)));
                    frameHT.reserve(expectedNoOfPackets);
                    mMutexPtr->lock();
                    mStreamHashTablePtr->insert(streamHdr.blockId$flag, frameHT);
                    mMutexPtr->unlock();
                } else {
                    qDebug() << "Invalid image leader header received.";
                }
                break;
            }
            break;
        case GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_TRAILER_FORMAT:
            trailer.reserved = qFromBigEndian(*(quint16 *)(dataPtr + strGvspGenericDataTrailerHdrRESERVED));
            trailer.payloadType = qFromBigEndian(*(quint16 *)(dataPtr + strGvspGenericDataTrailerHdrPAYLOADTYPE));

            switch (trailer.payloadType) {
            case GVSP_DATA_BLOCK_HDR_PAYLOAD_TYPE_IMAGE:
                error = gvspPopulateImageTrailerHdrFromBigEndian(dataPtr, imgTrailerHdr);
                if (!error) {
                    mMutexPtr->lock();
                    if (mStreamHashTablePtr->contains(streamHdr.blockId$flag)) {
                        mStreamHashTablePtr->find(streamHdr.blockId$flag)->insert(streamHdr.extId_res_pktFmt_pktID$res & 0x00FFFFFF, dataArr);

                        memcpy(&imgLeaderHdr, mStreamHashTablePtr->find(streamHdr.blockId$flag)->value(0).data(), sizeof(strGvspImageDataLeaderHdr));
                        expectedNoOfPackets = (((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) /
                                                (mPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)) + 1);
                        ((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) % (mPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)) ?
                                    expectedNoOfPackets++ : expectedNoOfPackets;

                        if (mStreamHashTablePtr->find(streamHdr.blockId$flag)->count() < (int)expectedNoOfPackets) {
                            //qDebug() << "Complete packet not found";
                            mQueueFrameResendBlockID->enqueue(streamHdr.blockId$flag);
                            emit signalRequestResend();
                        } else {
                            qDebug() << "mQueueDatgrams->count() = " << mQueueDatgrams->count();
                            qDebug() << "Packet received completely with block ID = " << streamHdr.blockId$flag;
                            mStreamHashTablePtr->remove(streamHdr.blockId$flag);
                        }
                    }
                    mMutexPtr->unlock();
                } else {
                    qDebug() << "Invalid image trailer header received.";
                }
                break;
            }
            break;
        case GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_PAYLOAD_FORMAT_GENERIC:
            mMutexPtr->lock();
            if (mStreamHashTablePtr->contains(streamHdr.blockId$flag) && (dataArr.size() > (int)sizeof(strGvspDataBlockHdr))) {
                mStreamHashTablePtr->find(streamHdr.blockId$flag)->insert(streamHdr.extId_res_pktFmt_pktID$res & 0x00FFFFFF,
                                                                          QByteArray((char *)dataPtr + sizeof(strGvspDataBlockHdr),
                                                                                     (mPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)));
            }
            mMutexPtr->unlock();
            break;
        }
    }
}