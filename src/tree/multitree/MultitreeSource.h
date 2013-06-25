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
	virtual IPvXAddress getAlternativeNode(int stripe, IPvXAddress forNode, IPvXAddress currentParent);

    virtual void processPacket(cPacket *pkt);
    void handleTimerMessage(cMessage *msg);

    void onNewChunk(int sequenceNumber);

	void processDisconnectRequest(cPacket *pkt);

	virtual void scheduleSuccessorInfo(int stripe);

	virtual bool isPreferredStripe(int stripe);
	
    void bindToGlobalModule(void);
    void bindToTreeModule(void);

	void cancelAllTimer(void);
	void cancelAndDeleteTimer(void);
};

#endif
