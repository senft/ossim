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
    TREE_JOIN_STATE_ACTIVE          = 2, // Joined/Active
    TREE_JOIN_STATE_LEAVING         = 3  // Leaving
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

	virtual int numInitStages() const { return 4; }

protected:
    MultitreePartnerList    *m_partnerList;
    Forwarder				*m_forwarder;
    AppSettingMultitree   	*m_appSetting;
    VideoBuffer				*m_videoBuffer;
    MultitreeStatistic 		*m_gstat;

	IPvXAddress *requestedChildship;

    TreeJoinState     		*m_state;

    int m_localPort, m_destPort;

	int numStripes;

	/*
	 * The bandwidth capacity of this node. A bandwidth capacity of 1 means,
	 * this node is capable of delivering or (!) receiving the whole stream
	 * once at a time.
	 */
	double bwCapacity;

	long lastSeqNumber;

	bool hasBWLeft(int additionalConnections);

    void bindToGlobalModule(void);
    void bindToTreeModule(void);
    void bindToStatisticModule(void);

	void getSender(cPacket *pkt, IPvXAddress &senderAddress, int &senderPort);
	void getSender(cPacket *pkt, IPvXAddress &senderAddress);
	const IPvXAddress& getSender(const cPacket *pkt) const;

    virtual void onNewChunk(int sequenceNumber) = 0;

	void processSuccessorUpdate(cPacket *pkt);
	void processConnectRequest(cPacket *pkt);
	void removeChild(int stripe, IPvXAddress address); 

	void optimize(void);

	void printStatus(void);

	void dropChild(int stripe, IPvXAddress address, IPvXAddress alternativeParent); 
	int getPreferredStripe();

	IPvXAddress getAlternativeNode(int stripe, IPvXAddress forNode);

private:
	double getBWCapacity(void);

	void getAppSetting(void);
	void acceptConnectRequests(const std::map<int, int> &stripes, IPvXAddress address, int lastChunk);
	void rejectConnectRequests(const std::vector<int> &stripes, IPvXAddress address);

    virtual void scheduleSuccessorInfo(int stripe) = 0;
    virtual int getMaxOutConnections(void) = 0;
	int getConnections(void);

	void sendChunksToNewChild(int stripe, IPvXAddress address, int lastChunk);

	virtual bool isPreferredStripe(int stripe) = 0;

	// Optimization functions
	double getCosts(int stripe, IPvXAddress child);
	double getGain(int stripe, IPvXAddress child, IPvXAddress childToDrop);
	double getGainThreshold(void);

	void getCostliestChild(int fromStripe, IPvXAddress &address);
	void getCheapestChild(int fromStripe, IPvXAddress &address, IPvXAddress skipAddress);

	double getStripeDensityCosts(int stripe); // K_sel, K_1
    int getForwardingCosts(int stripe, IPvXAddress child); // K_forw, K_2
    double getBalanceCosts(int stripe, IPvXAddress child, IPvXAddress childToDrop); //K_bal, K_3
    double getDepencyCosts(IPvXAddress child); //K_4
};

#endif
