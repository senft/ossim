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
        timer_successorInfo     = new cMessage("TREE_NODE_TIMER_SUCCESSOR_UPDATE");
		timer_reportStatistic   = new cMessage("TREE_NODE_TIMER_REPORT_STATISTIC");

		// -------------------------------------------------------------------------

		scheduleAt(simTime() + par("startTime").doubleValue(), timer_getJoinTime);

		scheduleAt(simTime() + 2, timer_reportStatistic);

		for (int i = 0; i < numStripes; i++)
		{
			m_state[i] = TREE_JOIN_STATE_IDLE;
		}

		m_count_prev_chunkMiss = 0L;
		m_count_prev_chunkHit = 0L;
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
    else if (msg == timer_successorInfo)
	{
        handleTimerSuccessorInfo();
	}
	else if (msg == timer_reportStatistic)
    {
       handleTimerReportStatistic();
       scheduleAt(simTime() + 2, timer_reportStatistic);
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
			throw cException("This happens...probably should reschedule leave");
	}

	for (int i = 0; i < numStripes; i++)
		m_state[i] = TREE_JOIN_STATE_IDLE;

	// Remove myself from ActivePeerTable
	m_apTable->removeAddress(getNodeAddress());

	TreeDisconnectRequestPacket *reqPkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");

	// Disconnect from parents
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
		std::vector<IPvXAddress> curChildren = m_partnerList->getChildren(i);
		if(!curChildren.empty())
		{
			IPvXAddress alternativParent = m_partnerList->getParent(i);
			reqPkt->setAlternativeNode(alternativParent);

			for(std::vector<IPvXAddress>::iterator it = curChildren.begin(); it != curChildren.end(); ++it) {
				sendToDispatcher(reqPkt->dup(), m_localPort, ((IPvXAddress)*it), m_destPort);
			}
		}
	}

	m_player->scheduleStopPlayer();

	m_partnerList->clear();

	delete reqPkt;
}

void MultitreePeer::handleTimerReportStatistic()
{

   if (m_player->getState() == PLAYER_STATE_PLAYING)
   {
      long int delta = m_player->getCountChunkHit() - m_count_prev_chunkHit;

      m_count_prev_chunkHit = m_player->getCountChunkHit();
      m_gstat->increaseChunkHit((int)delta);

      EV << "Reporting " << delta << " found chunks" << endl;

      delta = m_player->getCountChunkMiss() - m_count_prev_chunkMiss;

      m_count_prev_chunkMiss = m_player->getCountChunkMiss();

      m_gstat->increaseChunkMiss((int)delta);

      EV << "Reporting " << delta << " missed chunks" << endl;
   }
   
}

void MultitreePeer::handleTimerSuccessorInfo(void)
{
    TreeSuccessorInfoPacket *pkt = new TreeSuccessorInfoPacket("TREE_SUCCESSOR_INFO");
	pkt->setNumSuccessorArraySize(numStripes);

	for (int i = 0; i < numStripes; i++)
	{
		int numSucc = m_partnerList->getNumSuccessors(i);
		pkt->setNumSuccessor(i, numSucc);
	}

	set<IPvXAddress> sentTo;
	for (int i = 0; i < numStripes; i++)
	{
		IPvXAddress address = m_partnerList->getParent(i);
        if( !address.isUnspecified() && sentTo.find(address) == sentTo.end() )
		{
			sentTo.insert(address);
			sendToDispatcher(pkt->dup(), m_localPort, address, m_destPort);
        }
	}

	delete pkt;
}

void MultitreePeer::scheduleSuccessorInfo(void)
{
	EV << getNodeAddress();
	m_partnerList->printPartnerList();

    if(timer_successorInfo->isScheduled())
		return;

	// TODO this only happens in forwarding nodes
    scheduleAt(simTime() + param_waitUntilInform, timer_successorInfo);
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

    if(timer_successorInfo != NULL)
	{
        delete cancelEvent(timer_successorInfo);
        timer_successorInfo = NULL;
	}
}

void MultitreePeer::cancelAllTimer(void)
{
	cancelEvent(timer_getJoinTime);
	cancelEvent(timer_join);
	cancelEvent(timer_leave);
    cancelEvent(timer_successorInfo);
}

void MultitreePeer::bindToGlobalModule(void)
{
	MultitreeBase::bindToGlobalModule();
}

void MultitreePeer::bindToTreeModule(void)
{
	MultitreeBase::bindToTreeModule();

    cModule *temp = getParentModule()->getModuleByRelativePath("player");
	m_player = check_and_cast<PlayerBase *>(temp);
    EV << "Binding to churnModerator is completed successfully" << endl;
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
			reqPkt->setNumSuccessor(i, m_partnerList->getNumChildren(i));
		}

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
			reqPkt->setNumSuccessor(i, m_partnerList->getNumChildren(i));
		}

		sendToDispatcher(reqPkt, m_localPort, address, m_destPort);
		m_state[stripe] = TREE_JOIN_STATE_IDLE_WAITING;
	}
}

void MultitreePeer::processConnectConfirm(cPacket* pkt)
{
	TreeConnectConfirmPacket *treePkt = check_and_cast<TreeConnectConfirmPacket *>(pkt);
	int stripe = treePkt->getStripe();

    if(stripe == -1)
	{
		for (int i = 0; i < numStripes; i++)
		{
			if(m_state[i] != TREE_JOIN_STATE_IDLE_WAITING)
                throw cException("Received a ConnectConfirm although I am already connected (or unconnected) (stripe %d).", i);
		}
	}
	else
	{
        if(m_state[stripe] != TREE_JOIN_STATE_IDLE_WAITING)
        {
            throw cException("Received a ConnectConfirm although I am already connected (or unconnected) (stripe %d), state is %d, should be %d.",
                             stripe, TREE_JOIN_STATE_IDLE_WAITING, m_state[stripe]);
        }
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

		int nextSeq = treePkt->getNextSequenceNumber();
		EV << "NEXT VIDEOPACKET I WILL RECEIVE IS: " << nextSeq << endl;
		m_videoBuffer->initializeRangeVideoBuffer(nextSeq);
		lastSeqNumber = nextSeq;
	}
	else
	{
		EV << "Setting state for stripe " << stripe << "to TREE_JOIN_STATE_ACTIVE" << endl;
		m_state[stripe] = TREE_JOIN_STATE_ACTIVE;
	}

	// Add myself to ActivePeerList so other peers can find me (to connect to me)
	m_apTable->addAddress(getNodeAddress());

	if(m_player->getState() == PLAYER_STATE_IDLE)
		m_player->activate();

    EV << getNodeAddress();
    m_partnerList->printPartnerList();
}

void MultitreePeer::processDisconnectRequest(cPacket* pkt)
{
	TreeDisconnectRequestPacket *treePkt = check_and_cast<TreeDisconnectRequestPacket *>(pkt);
	int stripe = treePkt->getStripe();

	bool allIdleWaiting = true;
	if(stripe == -1)
	{
		for (int i = 0; i < numStripes; i++)
		{
			if(m_state[i] != TREE_JOIN_STATE_IDLE_WAITING)
			{
				allIdleWaiting = false;
				break;
			}
		}

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
}

void MultitreePeer::disconnectFromParent(int stripe, IPvXAddress alternativeParent)
{
	EV << "DISCONNECT FROM PARENT stripe " << stripe << " alt parent=" << alternativeParent.str() << endl;
	m_state[stripe] = TREE_JOIN_STATE_IDLE;
	m_partnerList->removeParent(stripe);
	connectVia(alternativeParent, stripe);
}

int MultitreePeer::getMaxOutConnections()
{
	return numStripes * (bwCapacity - 1);
}


bool MultitreePeer::isPreferredStripe(int stripe)
{
	int numChildren = m_partnerList->getNumChildren(stripe);
	for (int i = 0; i < numStripes; i++)
	{
		if(i != stripe && numChildren < m_partnerList->getNumChildren(i))
			return false;
	}
	return true;
}
