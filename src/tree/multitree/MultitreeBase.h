#ifndef MULTITREEBASE_H_
#define MULTITREEBASE_H_ true

#include "CommBase.h"
#include "AppCommon.h"
#include "AppSettingDonet.h"
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
    virtual void handleTimerMessage(cMessage *msg) = 0;

    void processPacket(cPacket *pkt);

	bool hasBWLeft(void);

	virtual int numInitStages() const { return 4; }
protected:
    MultitreePartnerList    *m_partnerList;
    DonetStatistic          *m_gstat;
    Forwarder				*m_forwarder;
	AppSettingDonet 		*m_appSetting;
	TreeJoinState m_state;

    int m_localPort, m_destPort;

    void bindToGlobalModule(void);
    void bindToTreeModule(void);
    void bindToStatisticModule(void);

	void getSender(cPacket *pkt, IPvXAddress &senderAddress, int &senderPort);
	void getSender(cPacket *pkt, IPvXAddress &senderAddress);
	const IPvXAddress& getSender(const cPacket *pkt) const;

private:
	void processConnectRequest(cPacket *pkt);
	void processConnectConfirm(cPacket *pkt);
	void processDisconnectRequest(cPacket *pkt);

};

#endif
