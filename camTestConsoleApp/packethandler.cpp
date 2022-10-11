#include "packethandler.h"
#include "cameraapi.h"
#include "gvsp/gvsp.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc/types_c.h>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;

PacketHandler::PacketHandler(QHash<quint16, QHash<quint32, QByteArray>> *streamHT, quint16 streamPktSize,
                             QReadWriteLock *hashLocker, QReadWriteLock *queueLocker, QQueue<quint16> *blockIDQueue,
                             QQueue<QNetworkDatagram> *datagramQueue,
                             QObject *parent) : QObject{ parent } {
    mStreamHashTablePtr = streamHT;
    mQueueFrameResendBlockID = blockIDQueue;
    mPktSize = streamPktSize;
    mQueueDatgrams = datagramQueue;
    mHashLockerPtr = hashLocker;
    mQueueLockerPtr = queueLocker;
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
    QByteArray imgRawBytes;
    quint8 *dataPtr;
    quint32 expectedNoOfPackets;
    quint32 packetCounter;
    quint32 tempValue;
    bool tempBool;
    quint16 leastBlockID;
    QList<quint16> keys;
    QHash<quint32, QByteArray> frameHT;
    QHash<quint32, QByteArray> tempFrame;

    /* Receive the datagram. */
    mQueueLockerPtr->lockForWrite();
    if (!mQueueDatgrams->isEmpty()) {
        gvspPkt = mQueueDatgrams->dequeue();
    }
    mQueueLockerPtr->unlock();

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
                    expectedNoOfPackets = ((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) /
                                            (mPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE));
                    ((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) % (mPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)) ?
                                expectedNoOfPackets++ : expectedNoOfPackets;

                    /* Adding 2 for header and trailer. */
                    expectedNoOfPackets += 2;

                    frameHT.insert(streamHdr.extId_res_pktFmt_pktID$res & 0x00FFFFFF,
                                   QByteArray((char *)&imgLeaderHdr, sizeof(strGvspImageDataLeaderHdr)));
                    frameHT.reserve(expectedNoOfPackets);

                    mHashLockerPtr->lockForWrite();
                    mStreamHashTablePtr->insert(streamHdr.blockId$flag, frameHT);
                    mHashLockerPtr->unlock();
                } else {
                    qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Invalid image leader header received.";
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
                    mHashLockerPtr->lockForRead();
                    tempBool = mStreamHashTablePtr->contains(streamHdr.blockId$flag);
                    mHashLockerPtr->unlock();

                    if (tempBool) {
                        mHashLockerPtr->lockForWrite();
                        mStreamHashTablePtr->find(streamHdr.blockId$flag)->insert(streamHdr.extId_res_pktFmt_pktID$res & 0x00FFFFFF, dataArr);
                        mHashLockerPtr->unlock();

                        memcpy(&imgLeaderHdr, mStreamHashTablePtr->find(streamHdr.blockId$flag)->value(0).data(), sizeof(strGvspImageDataLeaderHdr));
                        expectedNoOfPackets = ((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) /
                                                (mPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE));
                        ((imgLeaderHdr.sizeX * imgLeaderHdr.sizeY) % (mPktSize - IP_HEADER_SIZE - UDP_HEADER_SIZE - GVSP_HEADER_SIZE)) ?
                                    expectedNoOfPackets++ : expectedNoOfPackets;

                        /* Adding 2 for header and trailer. */
                        expectedNoOfPackets += 2;

                        {
                            /* NOTE: The logic in this block due to a corner case
                             * i.e (If another thread has a data packet for the same block ID).
                             * When we again try to acquire the lock for read, it will not allow
                             * until other threads waiting for write do not complete their transaction.
                             * But somehow this still does not work. Though still causes no packet loss,
                             * but still wastes the bandwidth. */
                            mHashLockerPtr->lockForRead();
                            tempValue = ((quint32)mStreamHashTablePtr->find(streamHdr.blockId$flag)->count());
                            mHashLockerPtr->unlock();
#if 0
                            if (tempValue < expectedNoOfPackets) {
                                QThread::currentThread()->msleep(1); /* NOTE: VERY POOR FIX, NEED TO CHANGE IT. */
                                mHashLockerPtr->lockForRead();
                                tempValue = ((quint32)mStreamHashTablePtr->find(streamHdr.blockId$flag)->count());
                                mHashLockerPtr->unlock();
                            }
#endif
                        }

                        if (tempValue < expectedNoOfPackets) {

                            mQueueLockerPtr->lockForWrite();
                            mQueueFrameResendBlockID->enqueue(streamHdr.blockId$flag);
                            mQueueLockerPtr->unlock();
                            emit signalRequestResend();
                        } else {
                            qInfo() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Packet received completely with block ID = " << streamHdr.blockId$flag;
                            qInfo() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Unserviced packets left in queue = " << mQueueDatgrams->count();

                            mHashLockerPtr->lockForRead();
                            tempFrame = mStreamHashTablePtr->find(streamHdr.blockId$flag).value();
                            mHashLockerPtr->unlock();

                            for (packetCounter = 1;
                                 packetCounter < (quint32)tempFrame.count() - 1;
                                 packetCounter++) {

                                imgRawBytes.append(tempFrame[packetCounter]);
                            }

                            /* TEST */
                            Mat image;
                            Mat data(imgLeaderHdr.sizeY, imgLeaderHdr.sizeX, CV_8UC1, imgRawBytes.data());
                            cvtColor(data, image, CV_BayerBG2BGR);

//                            if (!image.empty()) {
//                                imshow("image", image);
//                                waitKey(0);
//                            } else {
//                                qDebug() << "Image data is Null";
//                            }
                            /* TEST */

                            qInfo() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Size of image = " << imgRawBytes.count();
                            qInfo() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Number of packets in this frame = " << tempFrame.count();

                            mHashLockerPtr->lockForWrite();
                            mStreamHashTablePtr->remove(streamHdr.blockId$flag);
                            mHashLockerPtr->unlock();
                        }
                    }
                } else {
                    qDebug() << "(" << __FILENAME__ << ":" << __LINE__ << ")" << "Invalid image trailer header received.";
                }
                break;
            }
            break;
        case GVSP_DATA_BLOCK_HDR_PKT_FMT_DATA_PAYLOAD_FORMAT_GENERIC:
            mHashLockerPtr->lockForWrite();
            if (mStreamHashTablePtr->contains(streamHdr.blockId$flag) && (dataArr.size() > (int)sizeof(strGvspDataBlockHdr))) {
                mStreamHashTablePtr->find(streamHdr.blockId$flag)->insert(streamHdr.extId_res_pktFmt_pktID$res & 0x00FFFFFF,
                                                                          QByteArray((char *)dataPtr, gvspPkt.data().count() - GVSP_HEADER_SIZE));
            }
            mHashLockerPtr->unlock();
            break;
        }

        /* Following block removes the first entry if max enteries become greater than CAMERA_FRAME_BUFFER_MAXSIZE. */
        {
            mHashLockerPtr->lockForRead();
            tempBool = mStreamHashTablePtr->count() > CAMERA_MAX_FRAME_BUFFER_SIZE;
            if (tempBool == true) {
                keys = mStreamHashTablePtr->keys();
            }
            mHashLockerPtr->unlock();

            if (tempBool == true) {
                leastBlockID = keys.constFirst();
                for (int counter = 0; counter < keys.size(); counter++) {
                    if (keys.at(counter) < leastBlockID) {
                        leastBlockID = keys.at(counter);
                    }
                }

                mHashLockerPtr->lockForWrite();
                mStreamHashTablePtr->remove(leastBlockID);
                mHashLockerPtr->unlock();
            }
        }
    }
}

