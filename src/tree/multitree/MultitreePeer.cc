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

		param_retryLeave = par("retryLeave");

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

		stat_retrys = new int[numStripes];
		numSuccChanged = new bool[numStripes];
		fallbackParent = new IPvXAddress[numStripes];

		for (int i = 0; i < numStripes; i++)
		{
			m_state[i] = TREE_JOIN_STATE_IDLE;
			stat_retrys[i] = 0;
			numSuccChanged[i] = false;
		}

		firstSequenceNumber = -1L;

		m_count_prev_chunkMiss = 0L;
		m_count_prev_chunkHit = 0L;

		WATCH(m_localAddress);
		WATCH(m_localPort);
		WATCH(m_destPort);
		WATCH(param_delaySuccessorInfo);
		WATCH(param_intervalReconnect);
		for (int i = 0; i < numStripes; i++)
		{
			WATCH(lastSeqNumber[i]);
		}
	}
}

void MultitreePeer::finish(void)
{
	MultitreeBase::finish();

	cancelAndDeleteTimer();

	delete[] stat_retrys;
	delete[] numSuccChanged;
	delete[] fallbackParent;
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
	else if (msg == timer_optimization)
	{
        handleTimerOptimization();
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
			throw cException("");
	}

	std::vector<int> stripes;
	for (int i = 0; i < numStripes; ++i)
	{
		stripes.push_back(i);
	}

	IPvXAddress addrPeer = m_apTable->getARandPeer(getNodeAddress());
	connectVia(addrPeer, stripes);
}

void MultitreePeer::handleTimerLeave()
{
	// It might happen, that I have sent a ConnectionRequest somewhere and
	// before the ConnectConfirm arrived, my leave timer kicks in. Thats why
	// the states have to be checked here.
	for (int i = 0; i < numStripes; i++)
	{
		if(m_state[i] != TREE_JOIN_STATE_ACTIVE)
		{
			EV << "Leave scheduled for now, but I am inactive in at least stripe " << i << ". Rescheduling..." << endl;
			if(timer_leave->isScheduled())
				cancelEvent(timer_leave);
			scheduleAt(simTime() + param_retryLeave, timer_leave);

			return;
		}
	}

	EV << "Leaving the system now." << endl;
	printStatus();

	// Remove myself from ActivePeerTable
	m_apTable->removeAddress(getNodeAddress());

	for (int i = 0; i < numStripes; i++)
	{
		if(!m_partnerList->hasChildren(i))
		{
			EV << "No children in stripe " << i << ". Just disconnect from parent." << endl;

			IPvXAddress address = m_partnerList->getParent(i);
			if(!address.isUnspecified())
			{
				// No need to give an alternative when disconnecting from a parent
				dropNode(i, address, IPvXAddress());
			}

			m_state[i] = TREE_JOIN_STATE_LEAVING;

			m_player->scheduleStopPlayer();

		}
		else
		{
			EV << "Children in stripe " << i << ". Disconnecting..." << endl;

			m_state[i] = TREE_JOIN_STATE_LEAVING;

			std::vector<IPvXAddress> children = m_partnerList->getChildren(i);

			std::set<IPvXAddress> skipNodes;
			IPvXAddress laziestChild = m_partnerList->getChildWithLeastSuccessors(i, skipNodes);
			// Drop the child with the least successors to my parent...
			dropNode(i, laziestChild, m_partnerList->getParent(i));

			// ... and all other children to that 'lazy' node
			for (std::vector<IPvXAddress>::iterator it = children.begin() ; it != children.end(); ++it)
			{
				IPvXAddress addr = (IPvXAddress)*it;
				if(!addr.equals(laziestChild))
					dropNode(i, addr, laziestChild);
			}

		}
	}

	if(!m_partnerList->hasChildren())
	{
		m_partnerList->clear();
		leave();
	}

}

void MultitreePeer::handleTimerReportStatistic()
{
	// Report bandwidth usage
	m_gstat->gatherBWUtilization(getNodeAddress(), m_partnerList->getNumOutgoingConnections(), getMaxOutConnections());

	// Report hit/missing chunks
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

	for (int i = 0; i < numStripes; i++)
	{
		if(numSuccChanged[i])
		{
			int numSucc = m_partnerList->getNumSuccessors(i);

			SuccessorInfo info;
			info.stripe = i;
			info.numSuccessors = numSucc;

			pkt->getUpdates().push_back(info);
		}
	}

	set<IPvXAddress> sentTo;
	for (int i = 0; i < numStripes; i++)
	{
		if(numSuccChanged[i])
		{
			IPvXAddress parent = m_partnerList->getParent(i);
        	if( !parent.isUnspecified() && sentTo.find(parent) == sentTo.end() )
			{
				sentTo.insert(parent);
				sendToDispatcher(pkt->dup(), m_localPort, parent, m_destPort);
        	}
			numSuccChanged[i] = false;
		}
	}
	delete pkt;
}

void MultitreePeer::scheduleSuccessorInfo(int stripe)
{
	numSuccChanged[stripe] = true;

    if(timer_successorInfo->isScheduled())
		return;

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

	if(timer_optimization != NULL)
	{
		delete cancelEvent(timer_optimization);
		timer_optimization = NULL;
	}
}

void MultitreePeer::cancelAllTimer(void)
{
	cancelEvent(timer_getJoinTime);
	cancelEvent(timer_join);
	cancelEvent(timer_leave);
    cancelEvent(timer_successorInfo);
    cancelEvent(timer_reportStatistic);
	cancelEvent(timer_optimization);

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

void MultitreePeer::connectVia(IPvXAddress address, const std::vector<int> &stripes)
{
	int numReqStripes = stripes.size();

	for (int i = 0; i < numReqStripes; i++)
	{
		int stripe = stripes[i];
		if(m_state[stripe] != TREE_JOIN_STATE_IDLE)
		{
			const char *sAddr = address.str().c_str();
			throw cException("Trying to connect to %s in an invalid state (%d) in stripe %d.",
					sAddr, m_state[stripe], stripe);
		}
	}

	TreeConnectRequestPacket *pkt = new TreeConnectRequestPacket("TREE_CONNECT_REQUEST");

	EV << "Sending ConnectRequest for stripe(s) ";

	for (int i = 0; i < numReqStripes; i++)
	{
		int stripe = stripes[i];
		int numSucc = m_partnerList->getNumSuccessors(stripe);
		IPvXAddress currentParent = m_partnerList->getParent(stripe);
		long lastReceivedChunk = lastSeqNumber[stripe];

		ConnectRequest request;
		request.stripe = stripe;
		request.numSuccessors = numSucc;
		request.lastReceivedChunk = lastReceivedChunk;
		request.currentParent = currentParent;

		pkt->getRequests().push_back(request);

		requestedChildship[stripe] = address;
		m_state[stripe] = TREE_JOIN_STATE_IDLE_WAITING;

		EV << stripe << ", ";
	}

	EV << "to " << address << " " << endl;

	sendToDispatcher(pkt, m_localPort, address, m_destPort);
}

void MultitreePeer::processConnectConfirm(cPacket* pkt)
{
	TreeConnectConfirmPacket *treePkt = check_and_cast<TreeConnectConfirmPacket *>(pkt);
	std::vector<ConnectConfirm> confirms = treePkt->getConfirms();

	IPvXAddress address;
	getSender(pkt, address);

	for(std::vector<ConnectConfirm>::iterator it = confirms.begin(); it != confirms.end(); ++it)
	{
		int stripe = ((ConnectConfirm)*it).stripe;

		if(m_state[stripe] != TREE_JOIN_STATE_IDLE_WAITING)
		{
			const char *sAddr = address.str().c_str();
			throw cException("Received a ConnectConfirm (%s) although I am already connected (or haven't even requested) (stripe %d) (state %d).",
					sAddr, stripe, m_state[stripe]);
		}
	}

	for(std::vector<ConnectConfirm>::iterator it = confirms.begin(); it != confirms.end(); ++it)
	{
		ConnectConfirm confirm = (ConnectConfirm)*it;
		int stripe = confirm.stripe;
		IPvXAddress alternativeParent = confirm.alternativeParent;

		IPvXAddress oldParent = m_partnerList->getParent(stripe);
		if(!oldParent.isUnspecified())
		{
			// There already is another parent for this stripe (I disconnected from it, though).
			// So now I should tell him that it can stop forwarding packets to me
			EV << "Switching parent in stripe: " << stripe << " old: " << m_partnerList->getParent(stripe)
				<< " new: " << address << endl;

			if(!address.equals(oldParent))
				// No need to give an alternative when disconnecting from a parent
				dropNode(stripe, m_partnerList->getParent(stripe), IPvXAddress());
		}
		else
		{
			EV << "New parent in stripe: " << stripe << " (fallback: " << alternativeParent << ")" << endl;
		}

		fallbackParent[stripe] = alternativeParent;
		requestedChildship[stripe] = IPvXAddress();
		m_state[stripe] = TREE_JOIN_STATE_ACTIVE;
		m_partnerList->addParent(stripe, address);

		m_gstat->reportConnectionRetry(stat_retrys[stripe]);
		stat_retrys[stripe] = 0;
	}

	printStatus();

	for (int i = 0; i < numStripes; i++)
	{
		if(m_partnerList->getParent(i).isUnspecified())
			return;

		// Add myself to ActivePeerList when I have <numStripes> parents, so other peers can find me (to connect to me)
		m_apTable->addAddress(getNodeAddress());

		// Start collecting statistics
		if(!timer_reportStatistic->isScheduled())
			scheduleAt(simTime() + 2, timer_reportStatistic);
	}
}

void MultitreePeer::processDisconnectRequest(cPacket* pkt)
{
	TreeDisconnectRequestPacket *treePkt = check_and_cast<TreeDisconnectRequestPacket *>(pkt);

	std::vector<DisconnectRequest> requests = treePkt->getRequests();

	IPvXAddress senderAddress;
	getSender(pkt, senderAddress);

	for(std::vector<DisconnectRequest>::iterator it = requests.begin(); it != requests.end(); ++it)
	{
		DisconnectRequest request = (DisconnectRequest)*it;
		int stripe = request.stripe;
		IPvXAddress alternativeParent = request.alternativeParent;

		if( m_partnerList->hasChild(stripe, senderAddress) && m_state[stripe] != TREE_JOIN_STATE_LEAVING )
		{
			// If the DisconnectRequest comes from a child, just remove it from
			// my PartnerList it.. regardless of state
			removeChild(stripe, senderAddress);
			return;
		}

		switch (m_state[stripe])
		{
			case TREE_JOIN_STATE_IDLE_WAITING:
			{
				if(m_partnerList->hasParent(stripe, senderAddress))
				{
					EV << "Received another DisconnectRequest (stripe " << stripe << ") from parent ("
						<< senderAddress << "). Ignoring..." << endl;
					return;
				}

				// A node rejected my ConnectRequest
				
				stat_retrys[stripe]++;

				m_state[stripe] = TREE_JOIN_STATE_IDLE;
				requestedChildship[stripe] = IPvXAddress();

				EV << "Node " << senderAddress << " refused to let me join (stripe " << stripe << ")." << endl;

				std::vector<int> connect;
				connect.push_back(stripe);

				if( alternativeParent.isUnspecified()
						|| m_partnerList->hasChild(stripe, alternativeParent)
						|| m_partnerList->hasParent(stripe, alternativeParent) )
				{
					EV << "Node gave an invalid alternative parent (" << alternativeParent 
						<< ")(unspecified, child or parent). Reconnecting to sender..." << endl;
						connectVia(senderAddress, connect);
				}
				else
				{
					// Connect to the given alternative parent
					connectVia(alternativeParent, connect);
				}

				break;
			}
			case TREE_JOIN_STATE_ACTIVE:
			{
				// A node wants to disconnect from me
				if( m_partnerList->hasParent(stripe, senderAddress) )
				{
					disconnectFromParent(stripe, alternativeParent);
				}
				else
				{
					EV << "Received a DisconnectRequest (stripe " << stripe << ") from a node (" 
						<< senderAddress << " that is neither child nor parent. "
					   	<< "Probably a PassNodeRequest arriving too late." << endl;
				}
				break;
			}
			case TREE_JOIN_STATE_LEAVING:
			{

				if( m_partnerList->hasParent(stripe, senderAddress) )
				{
					if(m_partnerList->hasChildren(stripe))
					{
						EV << "MEH" << endl;	
					}
					else
					{
						// Since I am already in leaving state, the DisconnectRequest already has be
						// on the way to my parent (it just wants me to leave and doesn't know that
						// I already am...), so I can safely ignore this
					}
				}
				else if(m_partnerList->hasChild(stripe, senderAddress))
				{
					// I have sent a DisconnectRequest to a child node and that node finally
					// disconnects

					EV << "Child " << senderAddress << " connected to new parent in stripe " << stripe << endl;
					m_partnerList->removeChild(stripe, senderAddress);

					if(m_partnerList->getChildren(stripe).empty())
					{
						EV << "No more children in stripe: " << stripe << "... Disconnecting from parent." << endl;
						// No more children for this stripe -> disconnect from my parent
						IPvXAddress parent = m_partnerList->getParent(stripe);

						// No need to give an alternative when disconnecting from a parent
						dropNode(stripe, parent, IPvXAddress());

						m_state[stripe] = TREE_JOIN_STATE_IDLE;

						// Don't remove the parent here. It is still needed to forward other nodes to my
						// parent when they want to connect (alternative parent)

						// We already have to stop the player as soon as I disconnect from
						// one parent, because I will miss all packets from that parents
						// (and that would be counted as packet loss).
						m_player->scheduleStopPlayer();
					}

					printStatus();

					if(!m_partnerList->hasChildren())
					{
						leave();
					}
				}
				else
				{
					// This happens when:
					//   - This node leaves and disconnected from all children
					//   - Because all child nodes left I disconnect from my parents and clear my
					//     partner list
					//   - While my DisconnectRequest is on the way to my parent, my parent wants to
					//     disconnect from me
					//   - Since my DisconnectRequest is already on the way I can safely ignore this
				}

				break;
			}
			default:
			{
				EV << "Received DisconnectRequest from " << senderAddress << " (stripe " << stripe 
					<< ") although I already left that tree." << endl;
			}
		}
	}
}

void MultitreePeer::processPassNodeRequest(cPacket* pkt)
{
	TreePassNodeRequestPacket *treePkt = check_and_cast<TreePassNodeRequestPacket *>(pkt);

	int stripe = treePkt->getStripe();

	if(m_state[stripe] != TREE_JOIN_STATE_ACTIVE)
		return;

	int remainingBW = treePkt->getRemainingBW();
	float threshold = treePkt->getThreshold();
	float dependencyFactor = treePkt->getDependencyFactor();

	IPvXAddress senderAddress;
	getSender(pkt, senderAddress);

	EV << "PassNodeRequest from parent " << senderAddress << " (stripe: " << stripe << ") (remainingBW: "
		<< remainingBW <<", threshold: " << threshold << ", depFactor: " <<
		dependencyFactor << ")" << endl;

	if(m_partnerList->getChildren(stripe).size() > 0)
	{
		IPvXAddress child = m_partnerList->getBusiestChild(stripe);
		double k3 = (dependencyFactor - m_partnerList->getNumChildsSuccessors(stripe, child)) / dependencyFactor;
		int k2 = m_partnerList->getNumChildsSuccessors(stripe, child) > 0 ? 0 : 1;
		double gain = k3 - (double)k2;

		// TODO k2 and k3 are often -nan
		EV << "k3: " << k3 << " k2: " << k2 << " gain: " << gain << endl;

		if(gain < threshold)
			dropNode(stripe, child, senderAddress);
		else
			EV << "NOT GIVING CHILD: " << child << endl;
	}
}

void MultitreePeer::leave(void)
{
	EV << "No more parents, nor children. I am outta here!" << endl;
	cancelAllTimer();
}

void MultitreePeer::disconnectFromParent(int stripe, IPvXAddress alternativeParent)
{
	EV << "Parent " << m_partnerList->getParent(stripe) << " (stripe " << stripe
		<< ") wants me to leave." << endl;

	m_state[stripe] = TREE_JOIN_STATE_IDLE;

	IPvXAddress candidate = alternativeParent;
	// Make sure I am not connecting to a child of mine
	if(m_partnerList->hasChild(stripe, candidate))
	{
		throw cException("this happens.");
		EV << "Old parent wants me to connect to a child of mine." << endl;

		for (int i = 0; i < numStripes; ++i)
		{
			candidate = m_partnerList->getParent(i);
			if(!candidate.equals(alternativeParent) && !m_partnerList->hasChild(stripe, candidate))
			{
				EV << candidate << " is a good candidate." << endl;
				break;
			}
		}

		if(m_partnerList->hasChild(stripe, candidate))
		{
			EV << "Cannot connect to any of my other parents." << endl;
			throw cException("WHAT?!.");
		}
	}

	std::vector<int> connect;
	connect.push_back(stripe);
	connectVia(candidate, connect);
}

bool MultitreePeer::isPreferredStripe(int stripe)
{
	for (int i = 1; i < numStripes; i++)
	{
		if(i == stripe)
			continue;

		if( m_partnerList->getNumOutgoingConnections(stripe) < m_partnerList->getNumOutgoingConnections(i) )
			return false;
	}
	return true;
}

void MultitreePeer::onNewChunk(int sequenceNumber)
{
	Enter_Method("onNewChunk");

	VideoChunkPacket *chunkPkt = m_videoBuffer->getChunk(sequenceNumber);
	VideoStripePacket *stripePkt = check_and_cast<VideoStripePacket *>(chunkPkt);

	int stripe = stripePkt->getStripe();

	if(m_state[stripe] == TREE_JOIN_STATE_IDLE)
		return;

	int hopcount = stripePkt->getHopCount();

	m_gstat->reportChunkArrival(hopcount);

	stripePkt->setHopCount(++hopcount);

	// Forward to children
	std::vector<IPvXAddress> children = m_partnerList->getChildren(stripe);
	for(std::vector<IPvXAddress>::iterator it = children.begin(); it != children.end(); ++it)
	{
		sendToDispatcher(stripePkt->dup(), m_localPort, (IPvXAddress)*it, m_destPort);
	}

	lastSeqNumber[stripe] = sequenceNumber;

	if(firstSequenceNumber == -1 || sequenceNumber < firstSequenceNumber)
	{
		//EV << "FIRST CHUNK: " << sequenceNumber << endl;
		firstSequenceNumber = sequenceNumber;
	}

	int max = getGreatestReceivedSeqNumber();

	//EV << "MAX SEQ: " << max << endl;

	int current = firstSequenceNumber;
	int currentStart = firstSequenceNumber;
	int streak = 0;

	while(m_state[stripe] != TREE_JOIN_STATE_LEAVING && m_player->getState() == PLAYER_STATE_IDLE && current <= max)
	{
		//EV << "Checking: " << current << endl;
		if(m_videoBuffer->isInBuffer(current++))
		{
			//EV << "streak: " << streak << endl;
			streak++;
		}
		else
		{
			//EV << "COMBOBREAKER" << endl;
			streak = 0;
			currentStart = current;
		}

		if(streak >= numStripes)
		{
			//EV << "ACTIVATE AT: " << currentStart << endl;
			m_videoBuffer->initializeRangeVideoBuffer(currentStart);
			m_player->activate();
		}
		
	}
}

int MultitreePeer::getGreatestReceivedSeqNumber(void)
{
	int max = lastSeqNumber[0];
	for (int i = 1; i < numStripes; i++)
	{
		if(lastSeqNumber[i] > max)
			max = lastSeqNumber[i];
	}
	return max;
}

IPvXAddress MultitreePeer::getAlternativeNode(int stripe, IPvXAddress forNode, IPvXAddress currentParent)
{
	std::set<IPvXAddress> skipNodes;
	skipNodes.insert(forNode);
	skipNodes.insert(currentParent);

	// Chose the node with the least successors (however getChildWithLeastSuccessors tries to make
	// sure that the node has at least one successors, meaning the node is forwarding in the given
	// stripe)
	IPvXAddress address = m_partnerList->getChildWithLeastSuccessors(stripe, skipNodes);

	if( //m_state[stripe] == TREE_JOIN_STATE_LEAVING ||
			address.isUnspecified() ||
			!m_partnerList->hasChildren(stripe) ||
			(m_partnerList->getNumOutgoingConnections(stripe) == 1 && m_partnerList->hasChild(stripe, forNode)) 
		)
	{
		address = m_partnerList->getParent(stripe);
	}

	//EV << "Giving alternative node: " << address << endl;
	return address;
}
