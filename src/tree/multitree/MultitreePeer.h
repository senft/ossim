#ifndef MULTITREEPEER_H_
#define MULTITREEPEER_H_ value

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
    void handleTimerMessage(cMessage *msg);

    void bindToGlobalModule(void);

	void handleTimerJoin(void);
	void handleTimerLeave(void);

	void cancelAllTimer(void);
	void cancelAndDeleteTimer(void);

	cMessage *timer_getJoinTime;
	cMessage *timer_join;
	cMessage *timer_leave;
};
#endif
