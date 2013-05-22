#ifndef MULTITREEBASE_H_
#define MULTITREEBASE_H_ true

#include "CommBase.h"
#include "AppCommon.h"
#include "AppSettingMultitree.h"
#include "ActivePeerTable.h"
#include "DonetStatistic.h"
#include "Forwarder.h"
#include "MultitreePartnerList.h"
#include "MultitreeStatistic.h"
#include "VideoBuffer.h"
#include "VideoBufferListener.h"

#include "TreePeerStreamingPacket_m.h"

enum TreeJoinState
{
    TREE_JOIN_STATE_IDLE            = 0, // Not joined
    TREE_JOIN_STATE_IDLE_WAITING    = 1, // Not joined but requested
    TREE_JOIN_STATE_ACTIVE          = 2 // Active node
};


class MultitreeBase : public CommBase, public VideoBufferListener
{
public:
	MultitreeBase();
	virtual ~MultitreeBase();

	virtual void initialize(int stage);
	virtual void finish(void);

    virtual void handleMessage(cMessage *msg);
    virtual void processPacket(cPacket *pkt) = 0;
    virtual void handleTimerMessage(cMessage *msg) = 0;

	bool hasBWLeft(int additionalConnections);

	virtual int numInitStages() const { return 4; }
protected:
    MultitreePartnerList    *m_partnerList;
    //DonetStatistic          *m_gstat;
    Forwarder				*m_forwarder;
    AppSettingMultitree   	*m_appSetting;
    VideoBuffer				*m_videoBuffer;
    MultitreeStatistic 		*m_gstat;

    TreeJoinState     		*m_state;

    int m_localPort, m_destPort;

	int numStripes;

	/*
	 * The bandwidth capacity of this node. A bandwidth capacity of 1 means,
	 * this node is capable of delivering or (!) receiving the whole stream
	 * once at a time.
	 */
	double bwCapacity;

    void bindToGlobalModule(void);
    void bindToTreeModule(void);
    void bindToStatisticModule(void);

	void getSender(cPacket *pkt, IPvXAddress &senderAddress, int &senderPort);
	void getSender(cPacket *pkt, IPvXAddress &senderAddress);
	const IPvXAddress& getSender(const cPacket *pkt) const;

    virtual void onNewChunk(int sequenceNumber);

	void processSuccessorUpdate(cPacket *pkt);
	void processConnectRequest(cPacket *pkt);
	void disconnectFromChild(int stripe, IPvXAddress address); 
	void disconnectFromChild(IPvXAddress address);

	void optimize(void);

private:
	void getAppSetting(void);
	void acceptConnectRequest(TreeConnectRequestPacket *pkt);
	void rejectConnectRequest(TreeConnectRequestPacket *pkt);

    virtual void scheduleSuccessorInfo(void) = 0;
    virtual int getMaxOutConnections(void) = 0;
	virtual bool isPreferredStripe(int stripe) = 0;

	// Optimization functions
	double getCosts(int stripe, IPvXAddress child);
	double getGain(int stripe, IPvXAddress child);
	double getGainThreshold(void);

	void getCostliestChild(int &stripe, IPvXAddress &address);
	void getCheapestChild(int &stripe, IPvXAddress &address, int skipStripe, IPvXAddress skipAddress);

	double getStripeDensityCosts(int stripe); // K_sel, K_1
    int getForwardingCosts(int stripe, IPvXAddress child); // K_forw, K_2
    double getBalanceCosts(int stripe, IPvXAddress child); //K_bal, K_3
    double getDepencyCosts(IPvXAddress child); //K_4
};

#endif
