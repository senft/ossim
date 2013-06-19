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
	if(m_partnerList->hasChildren())
	{
		printStatus();
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
	}

	EV << "Leaving the system now." << endl;
	printStatus();

	// Remove myself from ActivePeerTable
	m_apTable->removeAddress(getNodeAddress());

	if(!m_partnerList->hasChildren())
	{
		EV << "I am leaving and have no children -> Just disconnect from parents." << endl;
		// Disconnect from parents
		for (int i = 0; i < numStripes; i++)
		{
			IPvXAddress address = m_partnerList->getParent(i);
			if(!address.isUnspecified())
			{
				// No need to give an alternative when disconnecting from a parent
				dropChild(i, address, IPvXAddress());
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

			std::vector<IPvXAddress> children = m_partnerList->getChildren(i);

			for (std::vector<IPvXAddress>::iterator it = children.begin() ; it != children.end(); ++it)
			{
				int stripe = i;
				IPvXAddress addr = (IPvXAddress)*it;
				dropChild(stripe, addr, getAlternativeNode(stripe, addr));
			}
		}
	}
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

	for (int i = 0; i < numStripes; i++)
	{
		if(numSuccChanged[i])
		{
			int numSucc = m_partnerList->getNumSuccessors(i);
			pkt->getStripes().insert( std::pair<int, int>( i, numSucc ) );
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
	pkt->setLastReceivedChunk(lastSeqNumber[stripes[0]]);

	EV << "Sending ConnectRequest for stripe(s) ";

	for (int i = 0; i < numReqStripes; i++)
	{
		int stripe = stripes[i];
		int numSucc = m_partnerList->getNumSuccessors(stripe);

		pkt->getStripes().insert( std::pair<int, int>(stripe, numSucc) );

		requestedChildship[stripe] = address;
		m_state[stripe] = TREE_JOIN_STATE_IDLE_WAITING;

		EV << stripe << ", ";
	}

	EV << "to " << address << " " << endl;

	sendToDispatcher(pkt, m_localPort, address, m_destPort);
}

void MultitreePeer::processConnectConfirm(cPacket* pkt)
{
	// TODO this contains an alternative "parent". save this in case your parent leaves

	TreeConnectConfirmPacket *treePkt = check_and_cast<TreeConnectConfirmPacket *>(pkt);
	std::map<int, IPvXAddress> stripes = treePkt->getStripes();

	IPvXAddress address;
	getSender(pkt, address);

	int nextSeq = treePkt->getNextSequenceNumber();

	for (std::map<int, IPvXAddress>::iterator it = stripes.begin() ; it != stripes.end(); ++it)
	{
		int stripe = it->first;

		if(m_state[stripe] != TREE_JOIN_STATE_IDLE_WAITING)
		{
			const char *sAddr = address.str().c_str();
			throw cException("Received a ConnectConfirm (%s) although I am already connected (or haven't even requested) (stripe %d) (state %d).",
					sAddr, stripe, m_state[stripe]);
		}

	}

	for (std::map<int, IPvXAddress>::iterator it = stripes.begin() ; it != stripes.end(); ++it)
	{
		int stripe = it->first;
		IPvXAddress alternativeParent = it->second;

		IPvXAddress oldParent = m_partnerList->getParent(stripe);
		if(!oldParent.isUnspecified())
		{
			// There already is another parent for this stripe (I disconnected from it, though).
			// So now I should tell him that it can stop forwarding packets to me
			EV << "Switching parent in stripe: " << stripe << " old: " << m_partnerList->getParent(stripe)
				<< " new: " << address << endl;

			if(!address.equals(oldParent))
				// No need to give an alternative when disconnecting from a parent
				dropChild(stripe, m_partnerList->getParent(stripe), IPvXAddress());
		}
		else
		{
			EV << "New parent in stripe: " << stripe << endl;
		}

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

	std::map<int, IPvXAddress> stripes = treePkt->getStripes();

	IPvXAddress senderAddress;
	getSender(pkt, senderAddress);

	for (std::map<int, IPvXAddress>::iterator it = stripes.begin() ; it != stripes.end(); ++it)
	{
		int stripe = it->first;
		IPvXAddress alternativeParent = it->second;

		if( m_partnerList->hasChild(stripe, senderAddress) )
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

				if(alternativeParent.isUnspecified())
				{
					const char *sAddr = senderAddress.str().c_str();
					throw cException("Received DisconnectRequest from %s (stripe %d) without alternative parent.",
							sAddr, stripe);
				}
				else if(m_partnerList->hasChild(stripe, alternativeParent)) // To avoid connecting to a child
				{
					EV << "Node suggested my child as an alternative node." << endl;
					std::vector<int> connect;
					connect.push_back(stripe);
					connectVia(senderAddress, connect);
				}
				else
				{
					std::vector<int> connect;
					connect.push_back(stripe);
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
				// I have sent a DR to a child node and that node finally disconnects

				EV << "Child " << senderAddress << " connected to new parent in stripe " << stripe << endl;
				m_partnerList->removeChild(stripe, senderAddress);

				if(m_partnerList->getChildren(stripe).empty())
				{
					EV << "No more children in stripe: " << stripe << "... Disconnecting from parent." << endl;
					// No more children for this stripe -> disconnect from my parent
					IPvXAddress parent = m_partnerList->getParent(stripe);

					// No need to give an alternative when disconnecting from a parent
					dropChild(stripe, parent, IPvXAddress());

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

	// TODO pick a node the parent "can handle"
	if(m_partnerList->getChildren(stripe).size() > 0)
	{
		IPvXAddress child = m_partnerList->getBusiestChild(stripe);
		double k3 = (dependencyFactor - m_partnerList->getNumChildsSuccessors(stripe, child)) / dependencyFactor;
		int k2 = m_partnerList->getNumChildsSuccessors(stripe, child) > 0 ? 0 : 1;
		double gain = k3 - (double)k2;

		EV << "k3: " << k3 << " k2: " << k2 << " gain: " << gain << endl;

		if(gain < threshold)
			dropChild(stripe, child, senderAddress);
		else
			EV << "NOT GIVING CHILD: " << child << endl;
	}
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
	EV << "Parent (stripe " << stripe << ") wants me to leave." << endl;
	m_state[stripe] = TREE_JOIN_STATE_IDLE;

	IPvXAddress candidate = alternativeParent;
	// Make sure I am not connecting to a child of mine
	if(m_partnerList->hasChild(stripe, candidate))
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
			throw cException("WHAT?!.");
		}
	}

	std::vector<int> connect;
	connect.push_back(stripe);
	connectVia(alternativeParent, connect);
}

int MultitreePeer::getMaxOutConnections()
{
	return numStripes * (bwCapacity - 1);
}


bool MultitreePeer::isPreferredStripe(int stripe)
{
	return stripe == getPreferredStripe();
}

void MultitreePeer::onNewChunk(int sequenceNumber)
{
	Enter_Method("onNewChunk");

	VideoChunkPacket *chunkPkt = m_videoBuffer->getChunk(sequenceNumber);
	VideoStripePacket *stripePkt = check_and_cast<VideoStripePacket *>(chunkPkt);

	int stripe = stripePkt->getStripe();
	int hopcount = stripePkt->getHopCount();

	if(firstSequenceNumber == -1)
	{
		EV << "First chunk received. Init buffer at: " << sequenceNumber << endl;
		m_videoBuffer->initializeRangeVideoBuffer(sequenceNumber);
		firstSequenceNumber = sequenceNumber;

		for (int i = 0; i < numStripes; i++)
		{
			lastSeqNumber[i] = sequenceNumber;
		}
	}
	else
	{
		if(sequenceNumber > lastSeqNumber[stripe])
			lastSeqNumber[stripe] = sequenceNumber;
	}

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
		// Doing so prevents packet loss in the case that I am accepted in one
		// stripe but still looking for parents in all other stripes
		for (int i = 0; i < numStripes; ++i)
		{
			if(m_partnerList->getParent(i).isUnspecified())
				return;

			if(!m_videoBuffer->inBuffer(firstSequenceNumber + i))
				// Do not start until we have <numStripes> consecutive chunks
				return;

		}

		m_player->activate();
	}
}

IPvXAddress MultitreePeer::getAlternativeNode(int stripe, IPvXAddress forNode)
{

	IPvXAddress node = m_partnerList->getRandomNodeFor(stripe, forNode);
	if(node.isUnspecified())
		return getNodeAddress();
	else
		return node;
}
