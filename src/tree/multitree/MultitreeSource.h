#ifndef MULTITREESOURCE_H_
#define MULTITREESOURCE_H_ value

#include "MultitreeBase.h"

class MultitreeSource : public MultitreeBase
{
public:
	MultitreeSource();
	virtual ~MultitreeSource();

protected:
    virtual void initialize(int stage);
    virtual void finish();

private:
    //void processPacket(cPacket *pkt);
    void handleTimerMessage(cMessage *msg);
	void cancelAndDeleteTimer(void);
};

#endif
