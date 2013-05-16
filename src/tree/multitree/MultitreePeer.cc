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

		param_intervalReconnect = par("intervalReconnect");

		// -------------------------------------------------------------------------
		// -------------------------------- Timers ---------------------------------
		// -------------------------------------------------------------------------
		// -- One-time timers
		timer_getJoinTime       = new cMessage("TREE_NODE_TIMER_GET_JOIN_TIME");
		timer_join              = new cMessage("TREE_NODE_TIMER_JOIN");
		timer_leave             = new cMessage("TREE_NODE_TIMER_LEAVE");

		// -- Repeated timers
		// e.g. optimization

		m_state = TREE_JOIN_STATE_IDLE;

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
	EV << m_state << endl;
	if(m_state != TREE_JOIN_STATE_IDLE)
		return;

	IPvXAddress addrPeer = m_apTable->getARandPeer(getNodeAddress());
	connectVia(addrPeer, -1);
}

void MultitreePeer::handleTimerLeave()
{
	if(m_state != TREE_JOIN_STATE_ACTIVE)
		return;

	m_state = TREE_JOIN_STATE_IDLE;

	EV << "LEAVING" << endl;
	m_partnerList->printPartnerList();

	// Remove myself from ActivePeerTable
	m_apTable->removeAddress(getNodeAddress());

	TreeDisconnectRequestPacket *reqPkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");

	// TODO would be cool to bundle all DRQ to a nodes I am connected to multiple times in 1 DRQ

	// Disconnect from parents
	// TODO: which stripes?
	for (int i = 0; i < numStripes; i++)
	{
		IPvXAddress address = m_partnerList->getParent(i);
		reqPkt->setStripe(i);
		sendToDispatcher(reqPkt->dup(), m_localPort, address, m_destPort);
	}

	// Disconnect from children
	for (int i = 0; i < numStripes; i++)
	{
		std::vector<MultitreeChildInfo> curChildren = m_partnerList->getChildren(i);
		if(!curChildren.empty())
		{
			IPvXAddress alternativParent = m_partnerList->getParent(i);
			reqPkt->setAlternativeNode(alternativParent);

			for(std::vector<MultitreeChildInfo>::iterator it = curChildren.begin(); it != curChildren.end(); ++it) {
				IPvXAddress address = ((MultitreeChildInfo)*it).getAddress();
				sendToDispatcher(reqPkt->dup(), m_localPort, address, m_destPort);
			}
		}
	}

	m_partnerList->clear();
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

void MultitreePeer::connectVia(IPvXAddress address, int stripe)
{
	TreeConnectRequestPacket *reqPkt = new TreeConnectRequestPacket("TREE_CONNECT_REQUEST");

	reqPkt->setStripe(stripe);

	// Include my numbers of successors
	reqPkt->setNumSuccessorArraySize(numStripes);
	for (int i = 0; i < numStripes; i++)
	{
		reqPkt->setNumSuccessor(i, m_partnerList->getNumOutgoingConnections(i));
	}

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

	if(m_state == TREE_JOIN_STATE_IDLE_WAITING)
	{
		// A node rejected my ConnectRequest
		IPvXAddress alternativeNode = treePkt->getAlternativeNode();

		if(alternativeNode.isUnspecified())
		{
			// Wait and try to connect later
			EV << "Node refused to let me join. No alternative node given. Retrying in " << param_intervalReconnect << "s";
			m_state = TREE_JOIN_STATE_IDLE;

			scheduleAt(simTime() + param_intervalReconnect, timer_getJoinTime);
		}
		else
		{
			connectVia(alternativeNode, -1);
		}
	}
	else if(m_state == TREE_JOIN_STATE_ACTIVE)
	{
		// A node wants to disconnect from me

		IPvXAddress senderAddress;
		getSender(pkt, senderAddress);

		int stripe = treePkt->getStripe();

		EV << senderAddress.str() << " wants to disconnect from me (stripe " << stripe << ")" << endl;

		if( m_partnerList->hasChild(stripe, senderAddress) )
		{
			disconnectFromChild(stripe, senderAddress);
		}
		else if( m_partnerList->hasParent(stripe, senderAddress) )
		{
			disconnectFromParent(stripe, treePkt->getAlternativeNode());
		}

	}
}

void MultitreePeer::disconnectFromParent(int stripe, IPvXAddress alternativeParent)
{
	m_partnerList->removeParent(stripe);
	connectVia(alternativeParent, stripe);
}

//void MultitreePeer::disconnectFromParent(IPvXAddress address, IPvXAddress alternativParent)
//{
//	std::vector<int> stripes = m_partnerList->removeParent(address);
//	connectVia(alternativParent, stripes);
//}

int MultitreePeer::getMaxOutConnections()
{
	return numStripes * (bwCapacity - 1);
}
