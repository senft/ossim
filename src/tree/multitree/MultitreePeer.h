#ifndef MULTITREEPEER_H_
#define MULTITREEPEER_H_ true

#include "MultitreeBase.h"
#include "PlayerBase.h"

class MultitreePeer : public MultitreeBase
{
public:
	MultitreePeer();
	virtual ~MultitreePeer();

protected:
    virtual void initialize(int stage);
    virtual void finish(void);

private:
	int *stat_retrys;

	long firstSequenceNumber;

	bool *numSuccChanged;

	double param_retryLeave;

	double param_intervalReconnect;
	double param_delaySuccessorInfo;

	virtual IPvXAddress getAlternativeNode(int stripe, IPvXAddress forNode);

    virtual void processPacket(cPacket *pkt);
    void handleTimerMessage(cMessage *msg);

    void onNewChunk(int sequenceNumber);

	void processConnectConfirm(cPacket* pkt);
	void processDisconnectRequest(cPacket *pkt);
	void processPassNodeRequest(cPacket *pkt);

	void leave(void);

	void disconnectFromParent(int stripe, IPvXAddress alternativeParent);

	virtual int getMaxOutConnections(void);
	virtual bool isPreferredStripe(int stripe);

	void connectVia(IPvXAddress address, const std::vector<int> &stripes);

    void bindToGlobalModule(void);
    void bindToTreeModule(void);

	virtual void scheduleSuccessorInfo(int);

	void handleTimerJoin(void);
	void handleTimerLeave(void);
	void handleTimerSuccessorInfo(void);
	void handleTimerReportStatistic(void);

	void cancelAllTimer(void);
	void cancelAndDeleteTimer(void);

	int getSmallestReceivedSeqNumber(void);

    PlayerBase *m_player;

	cMessage *timer_getJoinTime;
	cMessage *timer_join;
	cMessage *timer_leave;
	cMessage *timer_successorInfo;
	cMessage *timer_reportStatistic;

	long m_count_prev_chunkMiss;
	long m_count_prev_chunkHit;
};
#endif
