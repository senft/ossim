#include "MultitreePeer.h"

Define_Module(MultitreePeer)

MultitreePeer::MultitreePeer(){}

MultitreePeer::~MultitreePeer()
{
	finish();
}

void MultitreePeer::initialize(int stage)
{
	MultitreeBase::initialize(stage);

	if(stage == 2)
	{
		bindToGlobalModule();
		bindToTreeModule();
		bindToStatisticModule();
	}

	if(stage == 3)
	{
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
}

void MultitreePeer::handleTimerMessage(cMessage *msg)
{
    if (msg == timer_getJoinTime)
    {
		double arrivalTime = m_churn->getArrivalTime();

        EV << "Scheduled arrival time: " << simTime().dbl() + arrivalTime << endl;
        scheduleAt(simTime() + arrivalTime, timer_join);

        double departureTime = m_churn->getDepartureTime();
        if (departureTime > 0.0)
        {
           EV << "Scheduled departure time: " << simTime().dbl() + departureTime << endl;
           scheduleAt(simTime() + departureTime, timer_leave);
        }
        else
        {
           EV << "DepartureTime = " << departureTime << " --> peer won't leave" << endl;
        }
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

	IPvXAddress addrPeer = m_apTable->getARandPeer(getNodeAddress());
	connectVia(addrPeer);
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

	m_apTable->removeAddress(getNodeAddress());
}

void MultitreePeer::finish(void)
{
	cancelAndDeleteTimer();
}

void MultitreePeer::cancelAndDeleteTimer(void)
{
	if(timer_getJoinTime != NULL)
	{
		delete cancelEvent(timer_getJoinTime);
		timer_getJoinTime = NULL;
	}

	if(timer_join != NULL)
	{
		delete cancelEvent(timer_join);
		timer_join = NULL;
	}
	
	if(timer_leave != NULL)
	{
		delete cancelEvent(timer_leave);
		timer_leave = NULL;
	}
}

void MultitreePeer::cancelAllTimer(void)
{
	cancelEvent(timer_getJoinTime);
	cancelEvent(timer_join);
	cancelEvent(timer_leave);
}

void MultitreePeer::bindToGlobalModule(void)
{
	MultitreeBase::bindToGlobalModule();
}

void MultitreePeer::bindToTreeModule(void)
{
	MultitreeBase::bindToTreeModule();
}

void MultitreePeer::processPacket(cPacket *pkt)
{
	PeerStreamingPacket *appMsg = check_and_cast<PeerStreamingPacket *>(pkt);
    if (appMsg->getPacketGroup() != PACKET_GROUP_TREE_OVERLAY)
    {
        throw cException("MultitreePeer::processPacket: received a wrong packet. Wrong packet type!");
    }

    TreePeerStreamingPacket *treeMsg = check_and_cast<TreePeerStreamingPacket *>(appMsg);
    switch (treeMsg->getPacketType())
    {
	case TREE_CONNECT_REQUEST:
    {
		processConnectRequest(treeMsg);
		break;
    }
	case TREE_CONNECT_CONFIRM:
	{
		processConnectConfirm(treeMsg);
		break;
	}
	case TREE_DISCONNECT_REQUEST:
	{
		processDisconnectRequest(treeMsg);
		break;
	}
    default:
    {
        throw cException("MultitreePeer::processPacket: Unrecognized packet types! %d", treeMsg->getPacketType());
        break;
    }
    }

    delete pkt;
}

void MultitreePeer::connectVia(IPvXAddress address)
{
	TreeConnectRequestPacket *reqPkt = new TreeConnectRequestPacket("TREE_CONNECT_REQUEST");

    sendToDispatcher(reqPkt, m_localPort, address, m_destPort);
	m_state = TREE_JOIN_STATE_IDLE_WAITING;
}

void MultitreePeer::processConnectConfirm(cPacket* pkt)
{
	if(m_state != TREE_JOIN_STATE_IDLE_WAITING)
		return;

	// TODO: check if this is really the peer I wanted to connect to

	IPvXAddress address;
	getSender(pkt, address);

	m_partnerList->addParent(address);
	m_state = TREE_JOIN_STATE_ACTIVE;

	// Add myself to ActivePeerList so other peers can find me (to connect to me)
	m_apTable->addAddress(getNodeAddress());
}

void MultitreePeer::processDisconnectRequest(cPacket* pkt)
{
	TreeDisconnectRequestPacket *treePkt = check_and_cast<TreeDisconnectRequestPacket *>(pkt);

	IPvXAddress senderAddress;
	int senderPort;
	getSender(pkt, senderAddress, senderPort);

	if(m_state == TREE_JOIN_STATE_IDLE_WAITING)
	{
		IPvXAddress alternativeNode = treePkt->getAlternativeNode();
		if(alternativeNode.isUnspecified())
		{
			// Wait and try to connect later
			EV << "Node refused to let me join. No alternative node given. Waiting...";
			m_state = TREE_JOIN_STATE_IDLE;
		}
		else
		{
			connectVia(alternativeNode);
		}
	}
	else if(m_state == TREE_JOIN_STATE_ACTIVE)
	{
		// A node wants to disconnect from me
	}
}

int MultitreePeer::getMaxOutConnections()
{
	return numStripes * (bwCapacity - 1);
}
