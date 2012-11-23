//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef DONETBASE_H_
#define DONETBASE_H_

#include "CommBase.h"

#include "PartnerList.h"
#include "VideoBuffer.h"
#include "Forwarder.h"
#include "ActivePeerTable.h"
#include "AppSettingDonet.h"
#include "IChurnGenerator.h"
#include "MeshOverlayObserver.h"
#include "Logger.h"

#include "IPvXAddress.h"
#include <fstream>

//class Logger;

//#define MESH_STATE_JOIN_IDLE                    0
//#define MESH_STATE_JOIN_WAITING_ACCEPT          1
//#define MESH_STATE_JOIN_ALL_ACCEPT_RECEIVED     2
//#define MESH_STATE_JOIN_WAITING_ACCEPT_ACK      3

enum Mesh_Join_State {
    MESH_JOIN_STATE_IDLE            = 0,
    MESH_JOIN_STATE_IDLE_WAITING    = 1,
    MESH_JOIN_STATE_ACTIVE          = 2,
    MESH_JOIN_STATE_ACTIVE_WAITING  = 3
};

class DonetBase : public CommBase
{
public:
    DonetBase();
    virtual ~DonetBase();

    virtual void handleMessage(cMessage *msg);
    virtual void processPacket(cPacket *pkt) = 0;
    virtual void handleTimerMessage(cMessage *msg) = 0;

protected:
    // -- utility functions
    void getAppSetting(void);
    void readChannelRate(void);
    double getDownloadBw();
    double getUploadBw();

    // -- -- Get the IP address and/or port number
    const IPvXAddress& getSender(const cPacket *pkt) const;
    void getSender(cPacket *pkt, IPvXAddress &senderAddress);
    void getSender(cPacket *pkt, IPvXAddress &senderAddress, int &senderPort);

    // -- Overloading functions
    void bindToGlobalModule(void);
    void bindToMeshModule(void);

    // *************************************************************************
    // *************************************************************************
    // -- Partner Management
    void processPartnershipRequest(cPacket *pkt);
    void considerAcceptPartner(PendingPartnershipRequest requester);
    bool canAcceptMorePartner(void);

    // Timer
    void handleTimerTimeoutWaitingAccept();
    void handleTimerTimeoutWaitingAck();
    void handleTimerReport(void);

    // -- BufferMap
    //    MeshBufferMapPacket *generateBufferMapPacket();
    void sendBufferMap(void);
    inline int getBufferMapSize(void) { return m_bufferMapSize_chunk; }

    void processChunkRequest(cPacket *pkt);
    void printChunkRequestPacket(MeshChunkRequestPacket *reqPkt);

    MeshPartnershipAcceptPacket *generatePartnershipRequestAcceptPacket();
    MeshPartnershipRejectPacket *generatePartnershipRequestRejectPacket();

    // -- Debug
    void reportStatus();

protected:
    int m_localPort, m_destPort;

    // -- Parameters
    // -- Partnership size
    int param_minNOP;
    int param_maxNOP;
    int param_offsetNOP;
    double param_upBw;
    double param_downBw; // Download bandwidth is not neccessary!

    int param_bufferMapSize_second;
    int param_videoStreamBitRate;
    int param_chunkSize;

    double param_interval_bufferMap;

    // -- Variables for settings
    int m_videoStreamChunkRate;     /* in [chunks/second] */
    int m_bufferMapSize_chunk;      /* in [chunks] */
    int m_BufferMapPacketSize_bit;  /* in [bits]   */

    // -- Timer messages
    cMessage *timer_sendBufferMap;
    cMessage *timer_sendReport;

    // -- Pointers to /global/ modules
    AppSettingDonet         *m_appSetting;
    IChurnGenerator         *m_churn;
    MeshOverlayObserver     *m_meshOverlayObserver;
    Logger                  *m_logger;

    // -- Pointers to /local/ modules
    PartnerList *m_partnerList;
    VideoBuffer *m_videoBuffer;
    Forwarder   *m_forwarder;

    // --- Additional variables for debugging purposes
    double m_arrivalTime;
    double m_joinTime;
    double m_video_startTime;
    SEQUENCE_NUMBER_T m_head_videoStart;
    SEQUENCE_NUMBER_T m_begin_videoStart;
    int m_threshold_videoStart;
    long m_nChunkRequestReceived;
    long m_nChunkSent;
    long m_nBufferMapRecv;

    // -- Join process
    IPvXAddress m_address_candidate;
    double      m_upBw_candidate;

    // -- For join request & response
    // IP addresses of nodes to send JOIN_REQ to will be stored here
    //vector<IPvXAddress> m_list_joinRequestedNode;
    // IP addresses of nodes to send JOIN_ACCEPT to will be stored here
    //vector<IPvXAddress> m_list_joinAcceptedNode;
    // IP addresses of nodes which sent JOIN_REQ to this node
    vector<PendingPartnershipRequest> m_list_partnershipRequestingNode;
    PendingPartnershipRequest m_candidate;

    bool m_state_joined;
    short m_joinState;

    Mesh_Join_State m_state;

    simsignal_t sig_pRejectSent;
    simsignal_t sig_pRequestRecv;
    simsignal_t sig_pRequestRecv_whileWaiting;

};

#endif /* DONETBASE_H_ */
