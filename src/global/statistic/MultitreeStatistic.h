#include "IChurnGenerator.h"
#include "NotificationBoard.h"
#include "AppCommon.h"
#include "ActivePeerTable.h"
#include "AppSettingMultitree.h"
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

	void reportAwakeNode(void);
	void reportNodeLeft(void);

	void reportChunkArrival(int stripe, int hopcount);

	void gatherPreferredStripe(const IPvXAddress node, int stripe);
	void gatherBWUtilization(const IPvXAddress node, int curNumConn, int maxNumConn);
	void gatherNumTreesForwarding(const IPvXAddress node, int numTrees);
	void gatherOutDegree(const IPvXAddress node, int stripe, int degree);
	void gatherConnectionTime(int stripe, double time);
	void gatherRetrys(int retrys);

private:
	cMessage *timer_reportGlobal;
	double param_interval_reportGlobal;

	void handleTimerMessage(cMessage *msg);

	void reportOutDegree();
	void reportBWUtilization();
	void reportPacketLoss();
	void reportNumTreesForwarding();
	void reportConnectionTime();
	void reportRetrys();

	ActivePeerTable			*m_apTable;
    AppSettingMultitree   	*m_appSetting;

	simsignal_t sig_BWUtil;
	simsignal_t sig_packetLoss;
	simsignal_t sig_chunkArrival;
	simsignal_t sig_numTrees;
	simsignal_t sig_connTime;
	simsignal_t sig_retrys;
	simsignal_t sig_meanOutDegree;

	std::vector<std::map<IPvXAddress, int> > outDegreeSamples;
	std::vector<double> meanOutDegree;

	std::map<IPvXAddress, int> preferredStripes;
	std::map<IPvXAddress, int> currentBWUtilization;
	std::map<IPvXAddress, int> maxBWUtilization;
	std::map<IPvXAddress, int> totalNumTreesForwarding;
	std::vector<double> connectionTimes;
	std::vector<int> retrys;
	std::vector<int> numTrees;
	std::vector<int> hopcounts;

	cOutVector *oVNumTrees;
	cOutVector *oVHopcount;

	int numStripes;

	int awakeNodes;

	long m_count_chunkHit;
	long m_count_chunkMiss;
	long m_count_allChunk;

	int maxRetrys;
	double overallOutDegree;
	double meanRetrys;
	double meanBWUtil;
	double meanConnectionTime;
	double meanNumTrees;
	double meanHopcount;
};

#endif /* MULTITREE_STATISTIC_H_ */
