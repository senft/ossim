#ifndef MULTITREEBASE_H_
#define MULTITREEBASE_H_ true

#include "CommBase.h"
#include "AppCommon.h"
#include "AppSettingMultitree.h"
#include "IPvXAddress.h"
#include "ActivePeerTable.h"
#include "DonetStatistic.h"
#include "Forwarder.h"
#include "ChildInfo.h"
#include "MultitreePartnerList.h"

#include "TreePeerStreamingPacket_m.h"

enum TreeJoinState
{
    TREE_JOIN_STATE_IDLE            = 0, // Not joined
    TREE_JOIN_STATE_IDLE_WAITING    = 1, // Not joined but requested
    TREE_JOIN_STATE_ACTIVE          = 2, // Joined, waiting for requests
    TREE_JOIN_STATE_ACTIVE_WAITING  = 3  // Joined, processing incoming request
};


class MultitreeBase : public CommBase
{
public:
	MultitreeBase();
	virtual ~MultitreeBase();

	virtual void initialize(int stage);

    virtual void handleMessage(cMessage *msg);
    virtual void processPacket(cPacket *pkt) = 0;
    virtual void handleTimerMessage(cMessage *msg) = 0;

	bool hasBWLeft(void);

	virtual int numInitStages() const { return 4; }
protected:
    MultitreePartnerList    *m_partnerList;
    DonetStatistic          *m_gstat;
    Forwarder				*m_forwarder;
	AppSettingMultitree 	*m_appSetting;
	TreeJoinState m_state;

    int m_localPort, m_destPort;

	int numStripes;

	/*
	 * TODO: correct?  The bandwidth capacity of this node. A bandwidth
	 * capacity of 1 means, this node is capable of delivering or (!) receiving
	 * the whole stream once at a time.
	 */
	double bwCapacity;

    void bindToGlobalModule(void);
    void bindToTreeModule(void);
    void bindToStatisticModule(void);

	void getSender(cPacket *pkt, IPvXAddress &senderAddress, int &senderPort);
	void getSender(cPacket *pkt, IPvXAddress &senderAddress);
	const IPvXAddress& getSender(const cPacket *pkt) const;

	void processConnectRequest(cPacket *pkt);
private:
	void getAppSetting(void);

	virtual int getMaxOutConnections(void) = 0;

	// Optimization functions
	void optimize(int stripe);
	int getPreferredStripe(void);

	int getCosts(ChildInfo child, int stripe);
	int getGain(ChildInfo child, int stripe);

	int getStripeDensityCosts(int stripe); // K_sel, K_1
	int getForwardingCosts(ChildInfo child); // K_forw, K_2
	int getBalanceCosts(ChildInfo child, int stripe); //K_bal, K_3
	int getNumConnectionsToChild(ChildInfo child); //K_4
};

#endif
