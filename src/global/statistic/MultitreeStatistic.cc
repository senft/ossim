#include "MultitreeStatistic.h"

Define_Module(MultitreeStatistic)

MultitreeStatistic::MultitreeStatistic(){}
MultitreeStatistic::~MultitreeStatistic(){}

void MultitreeStatistic::finish()
{
	delete[] oVNumTrees;
	delete[] oVHopcount;
}

void MultitreeStatistic::initialize(int stage)
{
    if (stage == 0)
    {
        sig_chunkArrival		= registerSignal("Signal_Chunk_Arrival");
        sig_packetLoss   		= registerSignal("Signal_Packet_Loss");
        sig_numTrees            = registerSignal("Signal_Mean_Num_Trees");
        sig_BWUtil              = registerSignal("Signal_BW_Utilization");
        sig_connTime            = registerSignal("Signal_Connection_Time");
        sig_retrys              = registerSignal("Signal_Retrys");
	}

    if (stage != 3)
        return;

	// -- Binding to Active Peer Table
    cModule *temp = simulation.getModuleByPath("activePeerTable");
    m_apTable = check_and_cast<ActivePeerTable *>(temp);
    EV << "Binding to activePeerTable is completed successfully" << endl;

    temp = simulation.getModuleByPath("appSetting");
    m_appSetting = check_and_cast<AppSettingMultitree *>(temp);
	numStripes = m_appSetting->getNumStripes();

	timer_reportGlobal = new cMessage("GLOBAL_STATISTIC_REPORT_GLOBAL");

	param_interval_reportGlobal = par("interval_reportGlobal");

	awakeNodes = 0;

	m_count_chunkHit = 0;
	m_count_allChunk = 0;
	m_count_chunkMiss = 0;

	meanBWUtil = 0;
	meanConnectionTime = 0;
	meanNumTrees = 0;
	meanRetrys = 0;
	maxRetrys = 0;

	oVNumTrees = new cOutVector[numStripes + 1];
	oVHopcount = new cOutVector[numStripes + 1];

	for (int i = 0; i < numStripes + 1; i++)
	{
		numTrees.push_back(0);

		char name[24];
		sprintf(name, "nodesActiveIn%dstripes", i);
		oVNumTrees[i].setName(name);
	}

	for (int i = 0; i < numStripes; i++)
	{
		char name[24];
		sprintf(name, "hopcountTree%d", i);
		oVHopcount[i].setName(name);
	}

	WATCH(m_count_chunkHit);
	WATCH(m_count_chunkMiss);
	WATCH(m_count_allChunk);

	WATCH(meanBWUtil);
	WATCH(meanConnectionTime);
	WATCH(meanNumTrees);
	WATCH(meanRetrys);
	WATCH(maxRetrys);

	WATCH(awakeNodes);

	WATCH_VECTOR(numTrees);

	scheduleAt(simTime() + param_interval_reportGlobal, timer_reportGlobal);
}

void MultitreeStatistic::gatherPreferredStripe(const IPvXAddress node, int stripe)
{
	preferredStripes[node] = stripe;
}

void MultitreeStatistic::gatherRetrys(int numRetrys)
{
	retrys.push_back(numRetrys);
}
void MultitreeStatistic::reportRetrys()
{
	int sum = 0;
	for(std::vector<int>::iterator it = retrys.begin(); it != retrys.end(); ++it)
	{
		int numRetrys = (int)*it;
		sum += numRetrys;

		if(numRetrys > maxRetrys)
			maxRetrys = numRetrys;
	}
	meanRetrys = (double)sum / (double)retrys.size();
	emit(sig_retrys, meanRetrys);
}

void MultitreeStatistic::reportAwakeNode(void)
{
	awakeNodes++;
}

void MultitreeStatistic::reportNodeLeft(void)
{
	awakeNodes--;
}

void MultitreeStatistic::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        handleTimerMessage(msg);
    }
    else
    {
        throw cException("MultitreeStatistic doesn't process messages!");
    }
}

void MultitreeStatistic::handleTimerMessage(cMessage *msg)
{
	if(msg == timer_reportGlobal)
	{
		reportBWUtilization();
		reportPacketLoss();
		reportNumTreesForwarding();
		reportConnectionTime();
		reportRetrys();

		std::map<int, int> counts;
		for (std::map<IPvXAddress, int>::const_iterator it = preferredStripes.begin() ; it != preferredStripes.end(); ++it)
		{
			counts[it->second]++;
		}

		scheduleAt(simTime() + param_interval_reportGlobal, timer_reportGlobal);
	}
}

void MultitreeStatistic::reportBWUtilization()
{
	if(currentBWUtilization.size() > 0)
	{
		int totalCurNumCon = 0;
		int totalMaxNumCon = 0;

		for (std::map<IPvXAddress, int>::iterator it = currentBWUtilization.begin() ; it != currentBWUtilization.end(); ++it)
		{
			totalCurNumCon += it->second;
			totalMaxNumCon += maxBWUtilization[it->first];
		}

		meanBWUtil = (double)totalCurNumCon / (double)totalMaxNumCon;
		emit(sig_BWUtil, meanBWUtil);
	}
}

void MultitreeStatistic::gatherConnectionTime(int stripe, double time)
{
	connectionTimes.push_back(time);
}

void MultitreeStatistic::reportConnectionTime()
{
	if(connectionTimes.size() > 0)
	{
		double sum = 0;
		for(std::vector<double>::iterator it = connectionTimes.begin(); it != connectionTimes.end(); ++it)
		{
			sum += (double)*it;
		}
		meanConnectionTime = sum / (double)connectionTimes.size();
		emit(sig_connTime, meanConnectionTime);
	}
}

void MultitreeStatistic::gatherBWUtilization(const IPvXAddress node, int curNumConn, int maxNumConn)
{
	currentBWUtilization[node] = curNumConn;
	maxBWUtilization[node] = maxNumConn;
}

void MultitreeStatistic::reportNumTreesForwarding()
{
	//EV << totalNumTreesForwarding.size() << " nodes have reported their number of active trees." << endl;
	int num[numStripes + 1];
	for (int i = 0; i < numStripes + 1; i++)
	{
		num[i] = 0;
	}

	if(totalNumTreesForwarding.size() > 0)
	{
		int totalTrees = 0;

		for (std::map<IPvXAddress, int>::iterator it = totalNumTreesForwarding.begin() ; it != totalNumTreesForwarding.end(); ++it)
		{
			totalTrees += it->second;
			num[it->second]++;
		}

		meanNumTrees = (double)totalTrees / (double)totalNumTreesForwarding.size();

		for (int i = 0; i < numStripes + 1; i++)
		{
			numTrees[i] = num[i];
			oVNumTrees[i].record(num[i]);
		}

		emit(sig_numTrees, meanNumTrees);
	}

}

void MultitreeStatistic::gatherNumTreesForwarding(const IPvXAddress node, int numTrees)
{
	totalNumTreesForwarding[node] = numTrees;
}

void MultitreeStatistic::reportPacketLoss()
{
	emit(sig_packetLoss, (double)m_count_chunkMiss / (double)m_count_allChunk);
}

void MultitreeStatistic::reportChunkArrival(int stripe, int hopcount)
{
    emit(sig_chunkArrival, hopcount);
	oVHopcount[stripe].record(hopcount);
}

void MultitreeStatistic::increaseChunkHit(const int &delta)
{  
	m_count_chunkHit += delta;
	m_count_allChunk += delta;
}

void MultitreeStatistic::increaseChunkMiss(const int &delta)
{
	m_count_chunkMiss += delta;
	m_count_allChunk += delta;
}

void MultitreeStatistic::receiveChangeNotification(int category, const cPolymorphic *details)
{
	return;
}
