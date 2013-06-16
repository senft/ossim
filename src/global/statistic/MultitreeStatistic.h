#include "IChurnGenerator.h"
#include "NotificationBoard.h"
#include "AppCommon.h"
#include "ActivePeerTable.h"
#include "IPvXAddress.h"
#include <fstream>
#include "StatisticBase.h"
#include "StreamingStatistic.h"

#ifndef MULTITREE_STATISTIC_H_
#define MULTITREE_STATISTIC_H_

class ActivePeerTable;

class MultitreeStatistic : public StreamingStatistic
{
public:
    MultitreeStatistic();
    virtual ~MultitreeStatistic();

    virtual int numInitStages() const  {return 4;}
    virtual void initialize(int stage);
    virtual void finish();

    virtual void handleMessage(cMessage *msg);

	void increaseChunkHit(const int &delta);
	void increaseChunkMiss(const int &delta);

	virtual void receiveChangeNotification(int category, const cPolymorphic *details);

	void reportChunkArrival(const int hopcount);
	void reportConnectionRetry(const int count);

private:
	void handleTimerMessage(cMessage *msg);

	ActivePeerTable *m_apTable;

	simsignal_t sig_chunkArrival;
	simsignal_t sig_connectionRetry;

	long m_count_chunkHit;
	long m_count_chunkMiss;
	long m_count_allChunk;
};

#endif /* MULTITREE_STATISTIC_H_ */
