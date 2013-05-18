#include "MultitreePeer.h"

Define_Module(MultitreePeer)

MultitreePeer::MultitreePeer(){}

MultitreePeer::~MultitreePeer(){}

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
		param_waitUntilInform = m_appSetting->getWaitUntilInform();

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

		for (int i = 0; i < numStripes; i++)
		{
			m_state[i] = TREE_JOIN_STATE_IDLE;
		}
	}
}

void MultitreePeer::finish(void)
{
	MultitreeBase::finish();

	cancelAndDeleteTimer();
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
	for (int i = 0; i < numStripes; i++)
	{
		if(m_state[i] != TREE_JOIN_STATE_IDLE)
			// Something can't be right here
			return;
	}

	IPvXAddress addrPeer = m_apTable->getARandPeer(getNodeAddress());
	connectVia(addrPeer, -1);
}

void MultitreePeer::handleTimerLeave()
{
	for (int i = 0; i < numStripes; i++)
	{
		if(m_state[i] != TREE_JOIN_STATE_ACTIVE)
			// TODO: reschedule timerLeave
			return;
	}

	for (int i = 0; i < numStripes; i++)
		m_state[i] = TREE_JOIN_STATE_IDLE;

	//EV << "LEAVING" << endl;
	//m_partnerList->printPartnerList();

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
		reqPkt->setStripe(i);
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
	case TREE_SUCCESSOR_INFO:
	{
		processSuccessorUpdate(treeMsg);
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
	if(stripe == -1)
	{
		for (int i = 0; i < numStripes; i++)
		{
			if(m_state[i] != TREE_JOIN_STATE_IDLE)
				throw cException("Trying to connect in an invalid state (all stripes).");
		}
		
		TreeConnectRequestPacket *reqPkt = new TreeConnectRequestPacket("TREE_CONNECT_REQUEST");
		reqPkt->setStripe(stripe);

		// Include my numbers of successors
		reqPkt->setNumSuccessorArraySize(numStripes);
		for (int i = 0; i < numStripes; i++)
		{
			reqPkt->setNumSuccessor(i, m_partnerList->getNumOutgoingConnections(i));
		}

		EV << "SENDING CRQ to " << address << " stripe: " << stripe << endl;
		sendToDispatcher(reqPkt, m_localPort, address, m_destPort);

		for (int i = 0; i < numStripes; i++)
		{
			m_state[i] = TREE_JOIN_STATE_IDLE_WAITING;
		}
	}
	else
	{
		if(m_state[stripe] != TREE_JOIN_STATE_IDLE)
			throw cException("Trying to connect in an invalid state (stripe %d).", stripe);

		TreeConnectRequestPacket *reqPkt = new TreeConnectRequestPacket("TREE_CONNECT_REQUEST");
		reqPkt->setStripe(stripe);

		// Include my numbers of successors
		reqPkt->setNumSuccessorArraySize(numStripes);
		for (int i = 0; i < numStripes; i++)
		{
			reqPkt->setNumSuccessor(i, m_partnerList->getNumOutgoingConnections(i));
		}

		sendToDispatcher(reqPkt, m_localPort, address, m_destPort);
		m_state[stripe] = TREE_JOIN_STATE_IDLE_WAITING;
	}
}

void MultitreePeer::processConnectConfirm(cPacket* pkt)
{
	TreeConnectConfirmPacket *treePkt = check_and_cast<TreeConnectConfirmPacket *>(pkt);
	int stripe = treePkt->getStripe();

	EV << "CONECT CONFIRM " << stripe << endl;

	if(stripe == -1)
	{
		for (int i = 0; i < numStripes; i++)
		{
			if(m_state[i] != TREE_JOIN_STATE_IDLE_WAITING)
				throw cException("Received a ConnectConfirm although I am already connected (stripe %d).", i);
		}
	}
	else
	{
		if(m_state[stripe] != TREE_JOIN_STATE_IDLE_WAITING)
			throw cException("Received a ConnectConfirm although I am already connected (stripe %d).", stripe);
	}

	IPvXAddress address;
	getSender(pkt, address);

	m_partnerList->addParent(address);

	if(stripe == -1)
	{
		for (int i = 0; i < numStripes; i++)
		{
			m_state[i] = TREE_JOIN_STATE_ACTIVE;
		}
	}
	else
	{
		EV << "Setting state for stripe " << stripe << "to TREE_JOIN_STATE_ACTIVE" << endl;
		m_state[stripe] = TREE_JOIN_STATE_ACTIVE;
	}

	// Add myself to ActivePeerList so other peers can find me (to connect to me)
	m_apTable->addAddress(getNodeAddress());
}

void MultitreePeer::processDisconnectRequest(cPacket* pkt)
{
	TreeDisconnectRequestPacket *treePkt = check_and_cast<TreeDisconnectRequestPacket *>(pkt);
	int stripe = treePkt->getStripe();

	EV << "RECEIVED DRQ stripe: " << stripe << endl;

	bool allIdleWaiting = true;
	if(stripe == -1)
	{
		for (int i = 0; i < numStripes; i++)
		{
			if(m_state[i] != TREE_JOIN_STATE_IDLE_WAITING)
			{
				//EV << "m_state " << i << " is not TREE_JOIN_STATE_q
				allIdleWaiting = false;
				break;
			}
		}
		//EV << "AllIdleWaiting " <<  allIdleWaiting << endl;
		if(allIdleWaiting)
		{
			// A node rejected my ConnectRequest
			IPvXAddress alternativeNode = treePkt->getAlternativeNode();

			for (int i = 0; i < numStripes; i++)
			{
				m_state[i] = TREE_JOIN_STATE_IDLE;
			}

			if(alternativeNode.isUnspecified())
			{
				// Wait and try to connect later
				EV << "Node refused to let me join. No alternative node given. Retrying in " << param_intervalReconnect << "s";

				scheduleAt(simTime() + param_intervalReconnect, timer_getJoinTime);
			}
			else
			{
				connectVia(alternativeNode, -1);
			}
		}
	}
	else if(m_state[stripe] == TREE_JOIN_STATE_ACTIVE) // in this case stripe should always be >= 0
	{
		// A node wants to disconnect from me
		IPvXAddress senderAddress;
		getSender(pkt, senderAddress);

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
	m_partnerList->printPartnerList();
}

void MultitreePeer::disconnectFromParent(int stripe, IPvXAddress alternativeParent)
{
	EV << "DISCONNECT FROM PARENT stripe " << stripe << " alt parent=" << alternativeParent.str() << endl;
	m_state[stripe] = TREE_JOIN_STATE_IDLE;
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
