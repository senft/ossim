/*
 * NeighborInfo.cc
 *
 *  Created on: May 3, 2012
 *      Author: giang
 */
#include "NeighborInfo.h"

using namespace std;

NeighborInfo::NeighborInfo(int bmSize)
: m_lastRecvBmTime(-1.0)
, m_bufferSize(bmSize) /* m_lastKeepAliveTime(-1.0) */
, m_upBw(-1.0)
, m_seqNum_recvBmStart(-1L), m_seqNum_recvBmEnd(-1L), m_seqNum_recvBmHead(-1L)
, m_seqNum_sendBmStart(-1L), m_seqNum_sendBmEnd(-1L), m_seqNum_sendBmHead(-1L)
, m_sendBmModified(false)
//, m_timeBudget(0.0)
{
    if (bmSize < 0) throw cException("bmSize = %d is invalid", bmSize);
    // m_bufferSize = bmSize;
    // m_reqMap = new BufferMap(bmSize);
    // m_recvMap = new BufferMap(bmSize);

    // New implementation
    // m_bufferSize = bmSize;
    m_recvBm.resize(bmSize);
    m_sendBm.resize(bmSize);

    // -- for Donet Scheduling
    m_availTime.resize(bmSize);
}

NeighborInfo::~NeighborInfo()
{
}

bool NeighborInfo::isInRecvBufferMap(SEQUENCE_NUMBER_T seq_num)
{
    //Enter_Method("isInRecvBufferMap");

    EV << "Checking whether a chunk is in the RecvBufferMap" << endl;
    EV << " -- seq_num = " << seq_num << endl;
    EV << " -- start = " << m_seqNum_recvBmStart << endl;
    EV << " -- end = " << m_seqNum_recvBmEnd << endl;

    long offset = seq_num - m_seqNum_recvBmStart;

    // -- Debug
    // printRecvBm();

    if (seq_num > m_seqNum_recvBmEnd) return false;

    if (seq_num < m_seqNum_recvBmStart) return false;

    return (m_recvBm[offset]);
}

void NeighborInfo::setElementSendBm(SEQUENCE_NUMBER_T seq_num, bool val)
{
    //Enter_Method("setElementSendBm");

    EV << "Set an element of the SendBufferMap" << endl;

    int offset = seq_num - m_seqNum_sendBmStart;
    EV << " -- NeighborInfo::setElementSendBm:: offset: " << offset << endl;

    if (offset < 0) throw cException("offset %d is invalid", offset);
    m_sendBm[offset] = val;

    m_sendBmModified = true;

    // -- Debug
    // printSendBm();
}

void NeighborInfo::copyFrom(BufferMap *bm)
{
    m_seqNum_recvBmStart    = bm->getBmStartSeqNum();
    m_seqNum_recvBmEnd      = bm->getBmEndSeqNum();
    m_seqNum_recvBmHead     = bm->getHeadSeqNum();

    for (int i = 0; i < m_bufferSize; ++i)
    {
        m_recvBm[i] = bm->getElementByOffset(i);
    }
}

void NeighborInfo::copyFrom(MeshBufferMapPacket *bmPkt)
{
    // -- Get the value of id_start or id_end of the BufferMap Packet
    m_seqNum_recvBmStart    = bmPkt->getBmStartSeqNum();
    m_seqNum_recvBmEnd      = bmPkt->getBmEndSeqNum();
    m_seqNum_recvBmHead     = bmPkt->getHeadSeqNum();

    // -- Browse through all element of the array in the BufferMap packet
    // -- update the BufferMap elements respectively
    for (int i=0; i < m_bufferSize; ++i)
    {
        m_recvBm[i] = bmPkt->getBufferMap(i);
    }
}

void NeighborInfo::copyTo(BufferMap *bm)
{
    bm->setBmStartSeqNum(m_seqNum_sendBmStart);
    bm->setBmEndSeqNum(m_seqNum_sendBmEnd);
    bm->setHeadSeqNum(m_seqNum_sendBmHead);

    for (int i = 0; i < m_bufferSize; ++i)
    {
        bm->setElementByOffset(i, m_sendBm[i]);
    }

}

void NeighborInfo::copyTo(MeshChunkRequestPacket *reqPkt)
{
    reqPkt->setRequestMapArraySize(m_bufferSize);

    // -- Get the value of id_start or id_end of the BufferMap Packet
    reqPkt->setSeqNumMapStart(m_seqNum_sendBmStart);
    reqPkt->setSeqNumMapEnd(m_seqNum_sendBmEnd);
    reqPkt->setSeqNumMapHead(m_seqNum_sendBmHead);

    // -- Browse through all element of the array in the BufferMap packet
    // -- update the BufferMap elements respectively
    for (int i=0; i < m_bufferSize; i++)
    {
        reqPkt->setRequestMap(i, m_sendBm[i]);
    }
}

void NeighborInfo::printRecvBm(void)
{
    EV << "Print Received Buffer Map:" << endl;

    EV << "--- Start: " << m_seqNum_recvBmStart << endl
       << "--- End: " << m_seqNum_recvBmEnd << endl
       << "--- Head " << m_seqNum_recvBmHead
       << endl;

    for (int i = 0; i < m_bufferSize; ++i)
    {
        EV << m_recvBm[i] << " ";
    }

    EV << endl;
}

void NeighborInfo::printSendBm(void)
{
    EV << "sendBm:: << endl"
            << " -- m_sendBmModified: " << m_sendBmModified << endl
            << " -- Start: " << m_seqNum_sendBmStart << endl
            << " -- End: " << m_seqNum_sendBmEnd << endl
            << " -- Head: " << m_seqNum_sendBmHead
            << endl;

    for (int i = 0; i < m_bufferSize; ++i)
    {
        EV << m_sendBm[i] << " ";
    }
    EV << endl;
}

void NeighborInfo::clearSendBm(void)
{
    for (int i = 0; i < m_bufferSize; ++i)
    {
        m_sendBm[i] = false;
    }
    m_sendBmModified = false;
}

bool NeighborInfo::isSendBmModified(void)
{
    return m_sendBmModified;
}

void NeighborInfo::resetVectorAvailableTime(SEQUENCE_NUMBER_T vb_start, SEQUENCE_NUMBER_T win_start, double chunkInterval)
{
    // -- Update the starting point of the list
    m_winStart = win_start;

    int offset = win_start - vb_start;
    if (offset < 0) throw cException("offset %d is invalid", offset);

    for (int i = 0; i < m_bufferSize; ++i)
    {
        m_availTime[i] = (i + offset) * chunkInterval;
    }
}

void NeighborInfo::updateChunkAvailTime(SEQUENCE_NUMBER_T seq_num, double txTime)
{
    int offset = seq_num - m_winStart;
    if (offset < 0) throw cException("offset %d is invalid", offset);

    m_availTime[offset] = m_availTime[offset] - txTime;
}

double NeighborInfo::getChunkAvailTime(SEQUENCE_NUMBER_T seq_num)
{
    int offset = seq_num - m_winStart;
    if (offset < 0) throw cException("offset %d is invalid", offset);

    return m_availTime[offset];
}
