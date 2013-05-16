#ifndef MULTITREEPEER_H_
#define MULTITREEPEER_H_ true

#include "MultitreeBase.h"

class MultitreePeer : public MultitreeBase
{
public:
	MultitreePeer();
	virtual ~MultitreePeer();

protected:
    virtual void initialize(int stage);
    virtual void finish(void);

private:
	double param_intervalReconnect;

    virtual void processPacket(cPacket *pkt);
    void handleTimerMessage(cMessage *msg);

	void processConnectConfirm(cPacket* pkt);
	void processDisconnectRequest(cPacket *pkt);

	void disconnectFromParent(int stripe, IPvXAddress alternativeParent);
	void disconnectFromParent(IPvXAddress address, IPvXAddress alternativeParent);

	virtual int getMaxOutConnections(void);
	void connectVia(IPvXAddress address, std::vector<int> stripes);

    void bindToGlobalModule(void);
    void bindToTreeModule(void);

	void handleTimerJoin(void);
	void handleTimerLeave(void);

	void cancelAllTimer(void);
	void cancelAndDeleteTimer(void);

	cMessage *timer_getJoinTime;
	cMessage *timer_join;
	cMessage *timer_leave;
};
#endif
