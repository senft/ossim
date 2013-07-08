#include "MultitreeStatistic.h"

Define_Module(MultitreeStatistic)

MultitreeStatistic::MultitreeStatistic(){}
MultitreeStatistic::~MultitreeStatistic(){}

void MultitreeStatistic::finish(){}

void MultitreeStatistic::initialize(int stage)
{
    if (stage == 0)
    {
        sig_chunkArrival		= registerSignal("Signal_Chunk_Arrival");
        sig_packetLoss   		= registerSignal("Signal_Packet_Loss");
        sig_numTrees            = registerSignal("Signal_Num_Trees");
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

	WATCH(m_count_chunkHit);
	WATCH(m_count_chunkMiss);
	WATCH(m_count_allChunk);

	WATCH(meanBWUtil);
	WATCH(meanConnectionTime);
	WATCH(meanNumTrees);
	WATCH(meanRetrys);
	WATCH(maxRetrys);

	WATCH(awakeNodes);

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

		// TODO refactor
		std::map<int, int> counts;
		for (std::map<IPvXAddress, int>::const_iterator it = preferredStripes.begin() ; it != preferredStripes.end(); ++it)
		{
			counts[it->second]++;
		}

		EV << m_apTable->getNumActivePeer() << " nodes in total." << endl;
		for (std::map<int, int>::const_iterator it = counts.begin() ; it != counts.end(); ++it)
		{
			EV << "stripe " << it->first << ": " << it->second << " nodes" << endl;
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
	if(numTreesForwarding.size() > 0)
	{
		int totalTrees = 0;

		for (std::map<IPvXAddress, int>::iterator it = numTreesForwarding.begin() ; it != numTreesForwarding.end(); ++it)
		{
			totalTrees += it->second;
		}

		meanNumTrees = (double)totalTrees / (double)numTreesForwarding.size();
		emit(sig_numTrees, meanNumTrees);
	}
}

void MultitreeStatistic::gatherNumTreesForwarding(const IPvXAddress node, int numTrees)
{
	numTreesForwarding[node] = numTrees;
}

void MultitreeStatistic::reportPacketLoss()
{
	emit(sig_packetLoss, (double)m_count_chunkMiss / (double)m_count_allChunk);
}

void MultitreeStatistic::reportChunkArrival(const int hopcount)
{
    emit(sig_chunkArrival, hopcount);
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
