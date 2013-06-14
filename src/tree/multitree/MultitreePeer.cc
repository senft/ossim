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

		param_intervalReconnect =  m_appSetting->getIntervalReconnect();
		param_delaySuccessorInfo = m_appSetting->getDelaySuccessorInfo();

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

		// TODO: maybe schedule this first when connected to the system
		scheduleAt(simTime() + 2, timer_reportStatistic);

		for (int i = 0; i < numStripes; i++)
		{
			m_state[i] = TREE_JOIN_STATE_IDLE;
		}

		m_count_prev_chunkMiss = 0L;
		m_count_prev_chunkHit = 0L;

		WATCH(m_localAddress);
		WATCH(m_localPort);
		WATCH(m_destPort);
		WATCH(param_delaySuccessorInfo);
		WATCH(param_intervalReconnect);
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

	int reqStripes[numStripes];
	for (int i = 0; i < numStripes; ++i)
	{
		reqStripes[i] = i;
	}

	IPvXAddress addrPeer = m_apTable->getARandPeer(getNodeAddress());
	connectVia(addrPeer, numStripes, reqStripes);
}

void MultitreePeer::handleTimerLeave()
{
	if(m_partnerList->hasChildren())
	{
		for (int i = 0; i < numStripes; i++)
		{
			if(m_state[i] != TREE_JOIN_STATE_ACTIVE)
			{
				EV << "Leave scheduled for now, but I am inactive in one stripe. Rescheduling..." << endl;
				// TODO make this a parameter
				if(timer_leave->isScheduled())
					cancelEvent(timer_leave);
				scheduleAt(simTime() + 2, timer_leave);

				return;
			}
		}
	}

	EV << "Leaving the system now." << endl;
	printStatus();

	// Remove myself from ActivePeerTable
	m_apTable->removeAddress(getNodeAddress());

	TreeDisconnectRequestPacket *reqPkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");
	reqPkt->setStripesArraySize(1);

	if(!m_partnerList->hasChildren())
	{
		EV << "I am leaving and have no children -> Just disconnect from parents." << endl;
		// Disconnect from parents
		for (int i = 0; i < numStripes; i++)
		{
			IPvXAddress address = m_partnerList->getParent(i);
			if(!address.isUnspecified())
			{
				reqPkt->setStripes(0, i);
				sendToDispatcher(reqPkt->dup(), m_localPort, address, m_destPort);
			}
		}

		m_player->scheduleStopPlayer();
		m_partnerList->clear();
	}
	else
	{
		// If I have children I have to tell them I will be leaving but still
		// forward packets to them until they are connected to the new parent

		// Disconnect from children
		for (int i = 0; i < numStripes; i++)
		{
			m_state[i] = TREE_JOIN_STATE_LEAVING;

			reqPkt->setStripes(0, i);
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
	}
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
	printStatus();

    if(timer_successorInfo->isScheduled())
		return;

	// TODO this only happens in forwarding nodes
	EV << "SCHEDULING SUCC_INFO to: " << simTime() + param_delaySuccessorInfo << endl;
    scheduleAt(simTime() + param_delaySuccessorInfo, timer_successorInfo);
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
	
	if(timer_reportStatistic != NULL)
	{
		delete cancelEvent(timer_reportStatistic);
		timer_reportStatistic = NULL;
	}
}

void MultitreePeer::cancelAllTimer(void)
{
	cancelEvent(timer_getJoinTime);
	cancelEvent(timer_join);
	cancelEvent(timer_leave);
    cancelEvent(timer_successorInfo);
    cancelEvent(timer_reportStatistic);

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
	case TREE_PASS_NODE_REQUEST:
	{
		processPassNodeRequest(treeMsg);
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
	int reqStripes[1];
	reqStripes[0] = stripe;
	connectVia(address, 1, reqStripes);
}

void MultitreePeer::connectVia(IPvXAddress address, int numReqStripes, int stripes[])
{
	for (int i = 0; i < numReqStripes; i++)
	{
		if(m_state[stripes[i]] != TREE_JOIN_STATE_IDLE)
		{
			const char *sAddr = address.str().c_str();
			throw cException("Trying to connect to %s in an invalid state (%d) in stripe %d.", sAddr, m_state[stripes[i]], stripes[i]);
		}
	}

	TreeConnectRequestPacket *reqPkt = new TreeConnectRequestPacket("TREE_CONNECT_REQUEST");

	// Set requested stripes
	reqPkt->setStripesArraySize(numReqStripes);
	for (int i = 0; i < numReqStripes; i++)
	{
		int stripe = stripes[i];

		requestedChildship[stripe] = address;
		reqPkt->setStripes(i, stripe);
	}

	// Include my numbers of successors
	reqPkt->setNumSuccessorArraySize(numStripes);
	for (int i = 0; i < numStripes; i++)
	{
		int numSucc = m_partnerList->getNumSuccessors(i);
		reqPkt->setNumSuccessor(i, numSucc);
	}
	
	reqPkt->setLastReceivedChunk(lastSeqNumber);

	EV << "Sending ConnectRequest for stripe(s) ";
	for (int i = 0; i < numReqStripes; i++)
		EV << stripes[i] << " ";
	EV << "to " << address << " " << endl;

	sendToDispatcher(reqPkt, m_localPort, address, m_destPort);

	for (int i = 0; i < numReqStripes; i++)
	{
		m_state[stripes[i]] = TREE_JOIN_STATE_IDLE_WAITING;
	}
}

void MultitreePeer::processConnectConfirm(cPacket* pkt)
{
	// TODO this contains an alternative "parent". save this in case your parent leaves

	TreeConnectConfirmPacket *treePkt = check_and_cast<TreeConnectConfirmPacket *>(pkt);
	int numReqStripes = treePkt->getStripesArraySize();
	int nextSeq = treePkt->getNextSequenceNumber();

	IPvXAddress address;
	getSender(pkt, address);
	for (int i = 0; i < numReqStripes; ++i)
	{
		int stripe = treePkt->getStripes(i);

		if(m_state[stripe] != TREE_JOIN_STATE_IDLE_WAITING)
		{
			const char *sAddr = address.str().c_str();
			throw cException("Received a ConnectConfirm (%s) although I am already connected (or haven't even requested) (stripe %d) (state %d).",
					sAddr, stripe, m_state[stripe]);
		}

	}

	for (int i = 0; i < numReqStripes; ++i)
	{
		int stripe = treePkt->getStripes(i);

		if(!m_partnerList->getParent(stripe).isUnspecified())
		{
			// There already is another parent for this stripe (I disconnected from it, though).
			// So now I should tell him that it can stop forwarding packets to me
			EV << "Switching parent in stripe: " << stripe << " old: " << m_partnerList->getParent(stripe) << " new: " << address << endl;
			TreeDisconnectRequestPacket *rejPkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");
			rejPkt->setStripesArraySize(1);
			rejPkt->setStripes(0, stripe);
			sendToDispatcher(rejPkt, m_localPort, m_partnerList->getParent(stripe), m_destPort);
		}
		else
		{
			EV << "New parent in stripe : " << stripe << " starts forwarding with chunk #" << nextSeq << endl;
		}

		requestedChildship[stripe] = IPvXAddress();
		m_state[stripe] = TREE_JOIN_STATE_ACTIVE;
		m_partnerList->addParent(stripe, address);
	}

	printStatus();

	for (int i = 0; i < numStripes; i++)
	{
		if(m_partnerList->getParent(i).isUnspecified())
			return;

		// Add myself to ActivePeerList when I have <numStripes> parents, so other peers can find me (to connect to me)
		m_apTable->addAddress(getNodeAddress());
	}

}

void MultitreePeer::processDisconnectRequest(cPacket* pkt)
{
	TreeDisconnectRequestPacket *treePkt = check_and_cast<TreeDisconnectRequestPacket *>(pkt);
	int numReqStripes = treePkt->getStripesArraySize();

	IPvXAddress senderAddress;
	getSender(pkt, senderAddress);

	for (int i = 0; i < numReqStripes; ++i)
	{
		int stripe = treePkt->getStripes(i);

		if( m_partnerList->hasChild(stripe, senderAddress) )
		{
			// If the DisconnectRequest comes from a child, just drop it.. regardless of state
			disconnectFromChild(stripe, senderAddress);
			return;
		}

		switch (m_state[stripe])
		{
			case TREE_JOIN_STATE_IDLE_WAITING:
			{
				// A node rejected my ConnectRequest

				m_state[stripe] = TREE_JOIN_STATE_IDLE;
				requestedChildship[stripe] = IPvXAddress();

				IPvXAddress alternativeNode = treePkt->getAlternativeNode();

				EV << "Node " << senderAddress << " refused to let me join (stripe " << stripe << ")." << endl;

				if(alternativeNode.isUnspecified())
				{
					// TODO how is this happening?!
					connectVia(m_apTable->getARandPeer(getNodeAddress()), stripe);
				}
				else if(m_partnerList->hasChild(stripe, alternativeNode)) // To avoid connecting to a child
				{
					EV << "Node suggested an my child as an alternative node." << endl;
					// TODO: what's best here? Try to connect to child and let
					// him give me another node, reconnect to the node that
					// just suggested connecting to my child?
					connectVia(senderAddress, stripe);
				}
				else
				{
					connectVia(alternativeNode, stripe);
				}

				break;
			}
			case TREE_JOIN_STATE_ACTIVE:
			{
				// A node wants to disconnect from me

				if( m_partnerList->hasParent(stripe, senderAddress) )
				{
					disconnectFromParent(stripe, treePkt->getAlternativeNode());
				}
				else
				{
					EV << "Received a DisconnectRequest (stripe " << stripe << ") from a node (" 
						<< senderAddress << " that is neither child nor parent. "
					   	<< "Probably a PassNodeRequest arriving too late." << endl;
					
					//const char *sAddr = senderAddress.str().c_str();
					//throw cException("Received a DisconnectRequest from a node (%s) (stripe %d) that is neither child nor parent.",
						   	//sAddr, stripe);
				}
				break;
			}
			case TREE_JOIN_STATE_LEAVING:
			{
				// I have sent a DR to a child node and that node finally disconnects

				EV << "Child " << senderAddress << " connected to new parent in stripe " << stripe << endl;
				m_partnerList->removeChild(stripe, senderAddress);

				if(m_partnerList->getChildren(stripe).empty())
				{
					EV << "No more children in stripe: " << stripe << "... Disconnecting from parent." << endl;
					// No more children for this stripe -> disconnect from my parent
					IPvXAddress parent = m_partnerList->getParent(stripe);

					TreeDisconnectRequestPacket *reqPkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");
					reqPkt->setStripesArraySize(1);
					reqPkt->setStripes(0, stripe);
					sendToDispatcher(reqPkt, m_localPort, parent, m_destPort);

					// Don't remove the parent here. It is still needed to forward other nodes to my
					// parent when they want to connect

					// We already have to stop the player as soon as I disconnect from
					// one parent, because I will miss all packets from that parents
					// (and that would be counted as packet loss).
					if(m_player->getState() == PLAYER_STATE_PLAYING)
						m_player->scheduleStopPlayer();
				}

				printStatus();

				if(!m_partnerList->hasChildren())
				{
					leave();
				}

				break;
			}
			default:
			{
				throw cException("This happens!");
				break;
			}
		}
	}
}

void MultitreePeer::processPassNodeRequest(cPacket* pkt)
{
	IPvXAddress senderAddress;
	getSender(pkt, senderAddress);

	TreePassNodeRequestPacket *treePkt = check_and_cast<TreePassNodeRequestPacket *>(pkt);

	int stripe = treePkt->getStripe();
	int remainingBW = treePkt->getRemainingBW();
	float threshold = treePkt->getThreshold();
	float dependencyFactor = treePkt->getDependencyFactor();

	EV << "PassNodeRequest from parent " << senderAddress << " (stripe: " << stripe << ") (remainingBW: "
		<< remainingBW <<", threshold: " << threshold << ", depFactor: " <<
		dependencyFactor << endl;

	m_partnerList->printPartnerList();

	// TODO pick a node the parent "can handle"
	if(m_partnerList->getChildren(stripe).size() > 0)
		dropChild(stripe, m_partnerList->getBusiestChild(stripe), senderAddress);
}

void MultitreePeer::leave(void)
{
	EV << "No more parents, nor children. I am outta here!" << endl;

	// Player should already be stopped here...
	//if(m_player->getState() == PLAYER_STATE_PLAYING)
	//	m_player->scheduleStopPlayer();

	cancelAllTimer();
}

void MultitreePeer::disconnectFromParent(int stripe, IPvXAddress alternativeParent)
{
	// TODO: refactor
	EV << "Parent (stripe " << stripe << ") wants me to leave." << endl;
	m_state[stripe] = TREE_JOIN_STATE_IDLE;

	IPvXAddress candidate;
	// Make sure I am not connecting to a child of mine
	if(m_partnerList->hasChild(stripe, alternativeParent))
	{
		EV << "Old parent wants me to connect to a child of mine." << endl;
		for (int i = 0; i < numStripes; ++i)
		{
			candidate = m_partnerList->getParent(i);
			if(!m_partnerList->hasChild(stripe, candidate))
				break;
		}

		if(m_partnerList->hasChild(stripe, candidate))
		{
			EV << "Cannot connect to any of my other parents." << endl;
			throw cException("FUCK THIS SHIT.");
		}
		else
		{
			connectVia(candidate, stripe);
		}

	}
	else
	{
		connectVia(alternativeParent, stripe);
	}
}

int MultitreePeer::getMaxOutConnections()
{
	return numStripes * (bwCapacity - 1);
}


bool MultitreePeer::isPreferredStripe(int stripe)
{
	return stripe == getPreferredStripe();
	//int numChildren = m_partnerList->getNumOutgoingConnections(stripe);
	//for (int i = 0; i < numStripes; i++)
	//{
	//	if(i != stripe && numChildren < m_partnerList->getNumOutgoingConnections(i))
	//		return false;
	//}
	//return true;
}

void MultitreePeer::onNewChunk(int sequenceNumber)
{
	Enter_Method("onNewChunk");

	VideoChunkPacket *chunkPkt = m_videoBuffer->getChunk(sequenceNumber);
	VideoStripePacket *stripePkt = check_and_cast<VideoStripePacket *>(chunkPkt);

	int stripe = stripePkt->getStripe();
	int hopcount = stripePkt->getHopCount();
	lastSeqNumber = sequenceNumber;

	m_gstat->reportChunkArrival(hopcount);

	stripePkt->setHopCount(++hopcount);

	std::vector<IPvXAddress> children = m_partnerList->getChildren(stripe);
	for(std::vector<IPvXAddress>::iterator it = children.begin(); it != children.end(); ++it)
	{
		sendToDispatcher(stripePkt->dup(), m_localPort, (IPvXAddress)*it, m_destPort);
	}

	// If node is "fully connected" (and not in the process of leaving),
	// start playback (starting the chunk received just now)
	for (int i = 0; i < numStripes; i++)
	{
		// If the node is leaving the player is already stopped and would
		// be restarted if we don't return here
		if(m_state[i] == TREE_JOIN_STATE_LEAVING)
			return;
	}
	if(m_player->getState() == PLAYER_STATE_IDLE)
	{
		// TODO: Include this loop to not start the player before I am connected in all stripes
		// Doing so prevents packet loss in the case that I am accepted in one
		// stripe but still looking for parents in all other stripes
		for (int i = 0; i < numStripes; ++i)
		{
			if(m_partnerList->getParent(i).isUnspecified())
				return;
		}

		m_videoBuffer->initializeRangeVideoBuffer(sequenceNumber);
		m_player->activate();
	}
}


