#ifndef MULTITREEBASE_H_
#define MULTITREEBASE_H_ value

#include "CommBase.h"
#include "AppCommon.h"
#include "AppSettingDonet.h"
#include "IPvXAddress.h"
#include "ActivePeerTable.h"
#include "DonetStatistic.h"

#include "TreePeerStreamingPacket_m.h" // really neccessary? ... Not in Donet simulation

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

	void initialize(int stage);

    virtual void handleMessage(cMessage *msg);
    void processPacket(cPacket *pkt);
    virtual void handleTimerMessage(cMessage *msg) = 0;

	bool hasBWLeft(void);

	virtual int numInitStages() const { return 4; }
protected:
    DonetStatistic          *m_gstat;
	ActivePeerTable			*m_apTable;
	AppSettingDonet 		*m_appSetting;
	TreeJoinState m_state;

    int m_localPort, m_destPort;

    void bindToGlobalModule(void);
    void bindToStatisticModule(void);

private:

	void processConnectRequest(cPacket *pkt);
	void processConnectConfirm(cPacket *pkt);
	void processDisconnectRequest(cPacket *pkt);

	void getSender(cPacket *pkt, IPvXAddress &senderAddress, int &senderPort);
	void getSender(cPacket *pkt, IPvXAddress &senderAddress);
	const IPvXAddress& getSender(const cPacket *pkt) const;
};


#endif
