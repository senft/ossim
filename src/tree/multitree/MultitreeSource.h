#ifndef MULTITREESOURCE_H_
#define MULTITREESOURCE_H_ true

#include "MultitreeBase.h"

class MultitreeSource : public MultitreeBase
{
public:
	MultitreeSource();
	virtual ~MultitreeSource();

protected:
    virtual void initialize(int stage);
    virtual void finish(void);

private:
    virtual void processPacket(cPacket *pkt);
    void handleTimerMessage(cMessage *msg);

	void processDisconnectRequest(cPacket *pkt);

	virtual void scheduleSuccessorInfo(void);

	virtual int getMaxOutConnections(void);
	
    void bindToGlobalModule(void);
    void bindToTreeModule(void);

	void cancelAllTimer(void);
	void cancelAndDeleteTimer(void);
};

#endif
