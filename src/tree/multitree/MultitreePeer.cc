#include "MultitreePeer.h"

Define_Module(MultitreePeer)

MultitreePeer::MultitreePeer(){}

MultitreePeer::~MultitreePeer()
{
	cancelAndDeleteTimer();
}

void MultitreePeer::initialize(int stage)
{
	MultitreeBase::initialize(stage);

    bindToGlobalModule();
    bindToStatisticModule();

    m_state = TREE_JOIN_STATE_IDLE;

	// -------------------------------------------------------------------------
    // -------------------------------- Timers ---------------------------------
    // -------------------------------------------------------------------------
    // -- One-time timers
    timer_getJoinTime       = new cMessage("TREE_NODE_TIMER_GET_JOIN_TIME");
    timer_join              = new cMessage("TREE_NODE_TIMER_JOIN");
    timer_leave             = new cMessage("TREE_NODE_TIMER_LEAVE");

    // -- Repeated timers
	// e.g. optimization


    scheduleAt(simTime() + par("startTime").doubleValue(), timer_getJoinTime);

}

void MultitreePeer::handleTimerMessage(cMessage *msg)
{
    Enter_Method("handleTimerMessage()");

    if (msg == timer_getJoinTime)
    {
        scheduleAt(simTime() + 5, timer_join);
        scheduleAt(simTime() + 10, timer_leave);
    }
    else if (msg == timer_join)
    {
		handleTimerJoin();
    }
    else if (msg == timer_leave)
    {
		handleTimerLeave();
    }
} 

void MultitreePeer::handleTimerJoin()
{
	if(m_state != TREE_JOIN_STATE_IDLE)
		return;

	// TODO: pick random peer
	TreeConnectRequestPacket *reqPkt = new TreeConnectRequestPacket("TREE_CONNECT_REQUEST");
	//reqPkt->setStripesArraySize(3);
	//reqPkt->setStripes(0, 1);
	//reqPkt->setStripes(1, 3);
	//reqPkt->setStripes(2, 6);
	IPvXAddress addrPeer = IPvXAddress("192.168.0.1");
    sendToDispatcher(reqPkt, m_localPort, addrPeer, m_destPort);
	
	m_state = TREE_JOIN_STATE_IDLE_WAITING;
}

void MultitreePeer::handleTimerLeave()
{
	if(m_state != TREE_JOIN_STATE_ACTIVE)
		return;

	// TODO: send leave to all parents
	TreeDisconnectRequestPacket *reqPkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");
	IPvXAddress addrPeer = IPvXAddress("192.168.0.1");
    sendToDispatcher(reqPkt, m_localPort, addrPeer, m_destPort);

	m_state = TREE_JOIN_STATE_IDLE;
}

void MultitreePeer::finish(void)
{
	cancelAndDeleteTimer();
}

void MultitreePeer::cancelAndDeleteTimer(void)
{
	if(timer_getJoinTime != NULL)
	{
		cancelEvent(timer_getJoinTime);
		timer_getJoinTime = NULL;
	}

	if(timer_join != NULL)
	{
		cancelEvent(timer_join);
		timer_join = NULL;
	}
	
	if(timer_leave != NULL)
	{
		cancelEvent(timer_leave);
		timer_leave = NULL;
	}
}

void MultitreePeer::bindToGlobalModule(void)
{
	MultitreeBase::bindToGlobalModule();

    // -- Churn
    //cModule *temp = simulation.getModuleByPath("churnModerator");
    //m_churn = check_and_cast<IChurnGenerator *>(temp);
    //EV << "Binding to churnModerator is completed successfully" << endl;
}
