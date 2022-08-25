#ifndef GVCP_H
#define GVCP_H

#include <QObject>
#include <QtEndian>
#include <QUdpSocket>
#include <QtNetwork>
#include "gvcp/gvcpHeaders.h"
#include "caminterface.h"

unsigned char gvcpHelperReverseBits(unsigned char b);
void gvcpHelperMakeCommandSpecificAckHeader(strNonStdGvcpAckHdr& ackHeader, QByteArray& dataArray);
void gvcpHelperMakeCmdSepcificCmdHeader(QByteArray& datagram, const quint16 cmdType, const QByteArray& cmdSpecificData);
void gvcpHelperMakeGenericCmdHeader(const quint16 cmdType, QByteArray& datagram, const QByteArray& cmdSpecificData, const quint16 reqId);
void gvcpHelperFreeAckMemory(strNonStdGvcpAckHdr& ackHeader);
bool gvcpReceiveAck(QUdpSocket *udpSock, strNonStdGvcpAckHdr& ackHeader);
bool gvcpSendCmd(QUdpSocket *udpSock, const quint16 cmdType, const QByteArray& cmdSpecificData, const QHostAddress& destAddr, const quint16 port, const quint16 reqId);

#endif // GVCP_H
