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

    virtual void handleMessage(cMessage *msg);

    virtual void finish();

    void reportChunkHit(const SEQUENCE_NUMBER_T &seq_num);
    void reportChunkMiss(const SEQUENCE_NUMBER_T &seq_num);
    void increaseChunkHit(const int &delta);
    void increaseChunkMiss(const int &delta);

    virtual void receiveChangeNotification(int category, const cPolymorphic *details);

private:
    void handleTimerMessage(cMessage *msg);

public:
    void writeActivePeerTable2File(vector<IPvXAddress>);
    void writePartnerList2File(IPvXAddress node, vector<IPvXAddress> pList);
    void writePartnership2File(IPvXAddress local, IPvXAddress remote);

	void reportChunkArrival(const int hopcount);

private:

private:
    NotificationBoard *nb;
    ActivePeerTable *m_apTable;

    simsignal_t sig_chunkArrival;

    ofstream m_outFile;

};

#endif /* MULTITREE_STATISTIC_H_ */
