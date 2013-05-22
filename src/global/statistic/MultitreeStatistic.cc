#include "MultitreeStatistic.h"

Define_Module(MultitreeStatistic)

MultitreeStatistic::MultitreeStatistic(){}
MultitreeStatistic::~MultitreeStatistic(){}

void MultitreeStatistic::finish()
{
    // -- Close the log file
    m_outFile.close();

}

void MultitreeStatistic::initialize(int stage)
{
    if (stage == 0)
    {
        sig_chunkArrival    = registerSignal("Signal_Chunk_Arrival");

	}

    if (stage != 3)
        return;

    // get a pointer to the NotificationBoard module and IInterfaceTable
    nb = NotificationBoardAccess().get();

    nb->subscribe(this, NF_INTERFACE_CREATED);
    nb->subscribe(this, NF_INTERFACE_DELETED);
    nb->subscribe(this, NF_INTERFACE_STATE_CHANGED);
    nb->subscribe(this, NF_INTERFACE_CONFIG_CHANGED);
    nb->subscribe(this, NF_INTERFACE_IPv4CONFIG_CHANGED);

    // -- Binding to Active Peer Table
    cModule *temp = simulation.getModuleByPath("activePeerTable");
    m_apTable = check_and_cast<ActivePeerTable *>(temp);
    EV << "Binding to activePeerTable is completed successfully" << endl;

    m_outFile.open(par("gstatLog").stringValue(), fstream::out);
    
	WATCH(m_outFile);
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


void MultitreeStatistic::writeActivePeerTable2File(vector<IPvXAddress> activePeerList)
{
    m_outFile << "List of active peers" << endl;

    for (vector<IPvXAddress>::iterator iter = activePeerList.begin();
         iter != activePeerList.end(); ++iter)
    {
        m_outFile << *iter << endl;
    }
}

void MultitreeStatistic::reportChunkArrival(const int hopcount)
{
    emit(sig_chunkArrival, hopcount);
}

void MultitreeStatistic::reportChunkHit(const SEQUENCE_NUMBER_T &seq_num)
{}

void MultitreeStatistic::reportChunkMiss(const SEQUENCE_NUMBER_T &seq_num)
{}

void MultitreeStatistic::increaseChunkHit(const int &delta)
{}

void MultitreeStatistic::increaseChunkMiss(const int &delta)
{}

void MultitreeStatistic::receiveChangeNotification(int category, const cPolymorphic *details)
{}
