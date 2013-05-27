#include "MultitreeStatistic.h"

Define_Module(MultitreeStatistic)

MultitreeStatistic::MultitreeStatistic(){}
MultitreeStatistic::~MultitreeStatistic(){}

void MultitreeStatistic::finish(){}

void MultitreeStatistic::initialize(int stage)
{
    if (stage == 0)
    {
        sig_chunkArrival    = registerSignal("Signal_Chunk_Arrival");
	}

    if (stage != 3)
        return;

	// -- Binding to Active Peer Table
    cModule *temp = simulation.getModuleByPath("activePeerTable");
    m_apTable = check_and_cast<ActivePeerTable *>(temp);
    EV << "Binding to activePeerTable is completed successfully" << endl;

	WATCH(m_count_chunkHit);
	WATCH(m_count_chunkMiss);
	WATCH(m_count_allChunk);
}

void MultitreeStatistic::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        handleTimerMessage(msg);
    }
    else
    {
        throw cException("ActivePeerTable doesn't process messages!");
    }
}

void MultitreeStatistic::handleTimerMessage(cMessage *msg)
{
    
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
