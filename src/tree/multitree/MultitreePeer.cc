#include <algorithm>
#include "MultitreePeer.h"

Define_Module(MultitreePeer)

MultitreePeer::MultitreePeer(){}

MultitreePeer::~MultitreePeer(){}

void MultitreePeer::initialize(int stage)
{
	MultitreeBase::initialize(stage);

	if(stage == 0)
	{
        sig_numTrees            = registerSignal("Signal_Num_Trees");
	}

	if(stage == 2)
	{
		bindToGlobalModule();
		bindToTreeModule();
		bindToStatisticModule();
	}

	if(stage == 3)
	{

		param_retryLeave = par("retryLeave");
		param_intervalReportStats =  par("intervalReportStats");
		param_delaySuccessorInfo =  par("delaySuccessorInfo");

		param_delaySuccessorInfo  =  par("delaySuccessorInfo");
		param_delayRetryConnect   =  par("delayRetryConnect");

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
		timer_connect			= new cMessage("TREE_NODE_TIMER_CONNECT");

		// -------------------------------------------------------------------------

		scheduleAt(simTime() + par("startTime").doubleValue(), timer_getJoinTime);

		for (int i = 0; i < numStripes; i++)
		{
			beginConnecting.push_back(-1);
		}

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

		WATCH(param_delayRetryConnect);
		WATCH(param_delaySuccessorInfo);
		for (int i = 0; i < numStripes; i++)
		{
			WATCH(lastSeqNumber[i]);
			WATCH(beginConnecting[i]);
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
       scheduleAt(simTime() + param_intervalReportStats, timer_reportStatistic);
    }
	else if (msg == timer_connect)
	{
        handleTimerConnect();
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

	m_gstat->reportAwakeNode();

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

			std::set<IPvXAddress> &children = m_partnerList->getChildren(i);

			// TODO try busiest child here
			std::set<IPvXAddress> skipNodes;
			IPvXAddress laziestChild = m_partnerList->getBestLazyChild(i, skipNodes);
			// Drop the child with the least successors to my parent...
			dropNode(i, laziestChild, m_partnerList->getParent(i));

			// ... and all other children to that 'lazy' node
			for (std::set<IPvXAddress>::iterator it = children.begin() ; it != children.end(); ++it)
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
	m_gstat->gatherBWUtilization(getNodeAddress(),
			m_partnerList->getNumOutgoingConnections(), getMaxOutConnections());

	// Report number of trees I am forwarding in
	int numActiveTrees = m_partnerList->getNumActiveTrees();
	m_gstat->gatherNumTreesForwarding(getNodeAddress(), numActiveTrees);
	emit(sig_numTrees, numActiveTrees);

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
	EV << "Sending SuccessorInfo." << endl;

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
				numSI++;
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

	EV << "Scheduling SUCC_INFO to: " << simTime() + param_delaySuccessorInfo << endl;
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

	if(timer_connect != NULL)
	{
		delete cancelEvent(timer_connect);
		timer_connect = NULL;
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
	cancelEvent(timer_connect);

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

	TreeConnectRequestPacket *pkt = new TreeConnectRequestPacket("TREE_CONNECT_REQUEST");

	EV << "Sending ConnectRequest for stripe(s) ";
	for (int i = 0; i < numReqStripes; i++)
	{
		int stripe = stripes[i];
		if(m_state[stripe] != TREE_JOIN_STATE_IDLE)
		{
			const char *sAddr = address.str().c_str();
			throw cException("Trying to connect to %s in an invalid state (%d) in stripe %d.",
					sAddr, m_state[stripe], stripe);
		}


		int numSucc = m_partnerList->getNumSuccessors(stripe);
		IPvXAddress currentParent = m_partnerList->getParent(stripe);
		long lastReceivedChunk = lastSeqNumber[stripe];

		ConnectRequest request;
		request.stripe = stripe;
		request.numSuccessors = numSucc;
		request.lastReceivedChunk = lastReceivedChunk;
		request.currentParent = currentParent;
		request.lastRequests = requestedChildship[stripe];

		pkt->getRequests().push_back(request);

		if(beginConnecting[stripe] == -1)
			beginConnecting[stripe] = simTime();
		requestedChildship[stripe].push_back(address);
		m_state[stripe] = TREE_JOIN_STATE_IDLE_WAITING;

		EV << stripe << ", ";
	}
	EV << "to " << address << " " << endl;

	numCR++;
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

		double time = (simTime() - beginConnecting[stripe]).dbl();

		IPvXAddress oldParent = m_partnerList->getParent(stripe);
		if(!oldParent.isUnspecified())
		{
			// There already is another parent for this stripe (I disconnected from it, though).
			// So now I should tell him that it can stop forwarding packets to me
			EV << "Switching parent in stripe: " << stripe << " old: " << m_partnerList->getParent(stripe)
				<< " new: " << address << ". ";

			if(!address.equals(oldParent))
				// No need to give an alternative when disconnecting from a parent
				dropNode(stripe, m_partnerList->getParent(stripe), IPvXAddress());
		}
		else
		{
			EV << "New parent in stripe: " << stripe << " (fallback: " << alternativeParent 
				<< "). ";
		}

		EV << "It took me: "  << time << " seconds and " << requestedChildship[stripe].size()
			    << " requests." << endl;

		m_gstat->gatherConnectionTime(stripe, time);
		m_gstat->gatherRetrys(requestedChildship[stripe].size());

		fallbackParent[stripe] = alternativeParent;
		requestedChildship[stripe].clear();
		m_state[stripe] = TREE_JOIN_STATE_ACTIVE;
		m_partnerList->addParent(stripe, address);
		beginConnecting[stripe] = -1;

		//m_gstat->reportConnectionRetry(stat_retrys[stripe]);
		//stat_retrys[stripe] = 0;
	}

	printStatus();

	for (int i = 0; i < numStripes; i++)
	{
		// TODO Make sure this only happens when switching from no parent at all to a parent
		if(m_partnerList->getParent(i).isUnspecified())
		{
			return;
		}
	}

	// Add myself to ActivePeerList when I have <numStripes> parents, so other peers can find me
	// (to connect to me)
	EV << "Adding myself to ActivePeerTable" << endl;
	m_apTable->addAddress(getNodeAddress());

	// Start collecting statistics
	if(!timer_reportStatistic->isScheduled())
		scheduleAt(simTime() + 2, timer_reportStatistic);
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

			// TODO if the node knew, that the DRQ came from a child I dropped because of a PNR I
			// could omit the update
			scheduleSuccessorInfo(stripe);
			continue;
		}

		switch (m_state[stripe])
		{
			case TREE_JOIN_STATE_IDLE_WAITING:
			{
				if( !requestedChildship.empty() && senderAddress.equals(requestedChildship[stripe].back()) )
				{
					//if(requestedChildship[stripe].size() > 100)
					//	throw cException("staaaawp");
					// The last node I sent a ConnectRequest to rejected my request
					
					stat_retrys[stripe]++;

					m_state[stripe] = TREE_JOIN_STATE_IDLE;

					EV << "Node " << senderAddress << " refused to let me join (stripe " << stripe << ")(request #" <<
					   requestedChildship[stripe].size() << ")." << endl;

					std::vector<int> connect;
					connect.push_back(stripe);

					if( alternativeParent.isUnspecified()
							|| m_partnerList->hasChild(stripe, alternativeParent)
							//|| m_partnerList->hasParent(stripe, alternativeParent) 
						)
					{
						EV << "Node gave an invalid alternative parent (" << alternativeParent 
							<< ")(unspecified, child or parent). Reconnecting to sender..." << endl;
						connectTo[senderAddress].push_back(stripe);
					}
					else
					{
						// Connect to the given alternative parent
						connectTo[alternativeParent].push_back(stripe);
					}
				}
				else if(m_partnerList->hasParent(stripe, senderAddress))
				{
					EV << "Received another DisconnectRequest (stripe " << stripe << ") from parent ("
						<< senderAddress << "). Ignoring..." << endl;
					continue;
				}
				else
				{
					const char *sAddr = senderAddress.str().c_str();
					throw cException("Received DisconnectRequets (stripe %d) from %s, which is neither child nor parent.", stripe, sAddr);
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
					// Since I am already in leaving state, the DisconnectRequest already has be
					// on the way to my parent (it just wants me to leave and doesn't know that
					// I already am...), so I can safely ignore this
					EV << "Parent sent me another DisconnectRequest, while I am leaving. Ignoring..." << endl;
				}
				else if(m_partnerList->hasChild(stripe, senderAddress))
				{
					// I have sent a DisconnectRequest to a child node and that node finally
					// disconnects

					EV << "Child " << senderAddress << " connected to new parent in stripe " << stripe << endl;
					removeChild(stripe, senderAddress);
					//m_partnerList->removeChild(stripe, senderAddress);

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
						m_partnerList->clear();
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

	if(!timer_connect->isScheduled() && connectTo.size() > 0)
	{
		simtime_t nextConnect = simTime() + param_delayRetryConnect;
		EV << "Scheduling reconnect to " << nextConnect<< endl;
		scheduleAt(nextConnect, timer_connect);
	}

}

void MultitreePeer::handleTimerConnect(void)
{
	for (std::map<IPvXAddress, std::vector<int> >::iterator it = connectTo.begin() ; it != connectTo.end(); ++it)
	{
		connectVia(it->first, it->second);
	}

	connectTo.clear();
}


void MultitreePeer::processPassNodeRequest(cPacket* pkt)
{
	TreePassNodeRequestPacket *treePkt = check_and_cast<TreePassNodeRequestPacket *>(pkt);

	std::vector<PassNodeRequest> requests = treePkt->getRequests();

	for(std::vector<PassNodeRequest>::iterator it = requests.begin(); it != requests.end(); ++it)
	{

		PassNodeRequest request = (PassNodeRequest)*it;
		int stripe = request.stripe;

		if(m_state[stripe] != TREE_JOIN_STATE_ACTIVE)
			continue;

		int remainingBW = request.remainingBW;
		double threshold = request.threshold;
		double dependencyFactor = request.dependencyFactor;

		IPvXAddress senderAddress;
		getSender(pkt, senderAddress);

		EV << "PassNodeRequest from parent " << senderAddress << " (stripe: " << stripe << ") (remainingBW: "
			<< remainingBW <<", threshold: " << threshold << ", depFactor: " <<
			dependencyFactor << ")" << endl;

		//successorList children = m_partnerList->getChildrenWithCount(stripe);

		std::set<IPvXAddress> &curDisconnectingChildren = disconnectingChildren[stripe];

		std::set<IPvXAddress> skipNodes;
		for(std::set<IPvXAddress>::iterator it = curDisconnectingChildren.begin(); it != curDisconnectingChildren.end(); ++it)
		{
			skipNodes.insert( (IPvXAddress)*it );
		}
		while(remainingBW > 0)
		{
			IPvXAddress child = m_partnerList->getChildWithMostChildren(stripe, skipNodes);

			if(child.isUnspecified())
				break;

			double k3;
			if(dependencyFactor != 0)
				k3 = (dependencyFactor - (double)m_partnerList->getNumChildsSuccessors(stripe, child)) / dependencyFactor;
			else
				k3 = 0;
			int k2 = (m_partnerList->getNumChildsSuccessors(stripe, child) > 0) ? 0 : 1;
			double gain = param_weightK3 * k3 - param_weightK2 * (double)k2;

			EV << "k3: " << k3 << " k2: " << k2 << " gain: " << gain << endl;

			if(gain < threshold)// || m_partnerList->getNumChildsSuccessors(stripe, child) > dependencyFactor)
			{
				dropNode(stripe, child, senderAddress);
				remainingBW--;
			}
			else
			{
				EV << "Not giving child: " << child << endl;
			}
			skipNodes.insert(child);
		}

	}
}

void MultitreePeer::leave(void)
{
	EV << "No more parents, nor children. I am outta here!" << endl;
	cancelAllTimer();
	m_gstat->reportNodeLeft();
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
		//throw cException("this happens.");
		EV << "Old parent wants me to connect to a child of mine. (" << alternativeParent << ")" << endl;
		candidate = m_partnerList->getParent(stripe);
	}

	std::vector<int> connect;
	connect.push_back(stripe);
	connectVia(candidate, connect);
}

void MultitreePeer::onNewChunk(int sequenceNumber)
{
	Enter_Method("onNewChunk");

	VideoChunkPacket *chunkPkt = m_videoBuffer->getChunk(sequenceNumber);
	VideoStripePacket *stripePkt = check_and_cast<VideoStripePacket *>(chunkPkt);

	int stripe = stripePkt->getStripe();

	if(m_state[stripe] == TREE_JOIN_STATE_IDLE && !m_partnerList->hasChildren(stripe))
		return;

	int hopcount = stripePkt->getHopCount();

	m_gstat->reportChunkArrival(hopcount);

	stripePkt->setHopCount(++hopcount);

	// Forward to children
	std::set<IPvXAddress> &children = m_partnerList->getChildren(stripe);
	for(std::set<IPvXAddress>::iterator it = children.begin(); it != children.end(); ++it)
	{
		sendToDispatcher(stripePkt->dup(), m_localPort, (IPvXAddress)*it, m_destPort);
	}

	lastSeqNumber[stripe] = sequenceNumber;

	if(m_player->getState() == PLAYER_STATE_IDLE)
	{
		if(firstSequenceNumber == -1 || sequenceNumber < firstSequenceNumber)
		{
			//EV << "FIRST CHUNK: " << sequenceNumber << endl;
			firstSequenceNumber = sequenceNumber;
		}

		startPlayer();
	}
}

/*
 * Sees if there are <numStripes> consecutive chunks, if so, start the player
 */
void MultitreePeer::startPlayer(void)
{
	for (int i = 0; i < numStripes; i++)
	{
		if(m_state[i] == TREE_JOIN_STATE_LEAVING)
			return;
	}

	int max = getGreatestReceivedSeqNumber();
	//EV << "MAX SEQ: " << max << endl;

	int current = firstSequenceNumber;
	int currentStart = firstSequenceNumber;
	int streak = 0;

	while(m_player->getState() == PLAYER_STATE_IDLE && current <= max)
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

IPvXAddress MultitreePeer::getAlternativeNode(int stripe, IPvXAddress forNode, IPvXAddress currentParent, const std::vector<IPvXAddress> lastRequests)
{
	std::set<IPvXAddress> skipNodes;
	skipNodes.insert(forNode);
	skipNodes.insert(currentParent);
	for(std::vector<IPvXAddress>::const_iterator it = lastRequests.begin(); it != lastRequests.end(); ++it)
	{
		skipNodes.insert( (IPvXAddress)*it );
	}

	std::set<IPvXAddress> curDisconnectingChildren = disconnectingChildren[stripe];
	for(std::set<IPvXAddress>::iterator it = curDisconnectingChildren.begin(); it != curDisconnectingChildren.end(); ++it)
	{
		skipNodes.insert( (IPvXAddress)*it );
	}

	IPvXAddress address = m_partnerList->getBestLazyChild(stripe, skipNodes);

	//IPvXAddress address = m_partnerList->getChildWithMostChildren(stripe, skipNodes);
	//IPvXAddress address = m_partnerList->getChildWithLeastChildren(stripe, skipNodes);

	//while(m_partnerList->nodeForwardingInOtherStripe(stripe, address) && !address.isUnspecified())
	while(m_partnerList->nodeHasMoreChildrenInOtherStripe(stripe, address) && !address.isUnspecified())
	{
		skipNodes.insert(address);
		address = m_partnerList->getBestLazyChild(stripe, skipNodes);
	}

	if( address.isUnspecified() ||
		!m_partnerList->hasChildren(stripe) ||
		(m_partnerList->getNumOutgoingConnections(stripe) == 1 && m_partnerList->hasChild(stripe, forNode)) )
	{
		if(std::find(lastRequests.begin(), lastRequests.end(), m_partnerList->getParent(stripe)) == lastRequests.end())
		{
			address = m_partnerList->getParent(stripe);
		}
		else
		{
			// The node already tried connecting to my parent and all my children, so actually I
			// cannot make a reasonable choice.. just pick a random child, or the parent (in hope
			// the situation at that nodes changed)

			skipNodes.clear();
			skipNodes.insert(forNode);
			skipNodes.insert(currentParent);
			if(!lastRequests.empty())
				skipNodes.insert( lastRequests.back() );

			address = m_partnerList->getRandomChild(stripe, skipNodes);

			if( address.isUnspecified() || intrand(m_partnerList->getNumOutgoingConnections(stripe) + 1) == 0 )
					address = m_partnerList->getParent(stripe);
		}
	}

	//EV << "Giving alternative node: " << address << endl;
	return address;
}

void MultitreePeer::optimize(void)
{
	for (int i = 0; i < numStripes; i++)
		if(m_state[i] == TREE_JOIN_STATE_IDLE || m_state[i] == TREE_JOIN_STATE_LEAVING)
			return;

	int stripe = getStripeToOptimize();

	if(!m_partnerList->hasChildren(stripe))
		return;

	printStatus();

	std::map<IPvXAddress, int> children = m_partnerList->getChildrenWithCount(stripe);

	EV << "---------------------------------------------- OPTIMIZE, STRIPE: " << stripe << endl;

	bool gain = true;
	while(gain && children.size() > 1)
	{
		gain = false;

		IPvXAddress linkToDrop;	
		getCostliestChild(children, stripe, linkToDrop);

		IPvXAddress alternativeParent;	
		getCheapestChild(children, stripe, alternativeParent, linkToDrop);

		EV << "COSTLIEST CHILD: " << linkToDrop << endl;
		EV << "CHEAPEST CHILD: " << alternativeParent << endl;

		double gainIf = getGain(children, stripe, alternativeParent);
		EV << "GAIN: " << gainIf << endl;
		EV << "THRESHOLD: " << gainThreshold << endl;

		if(gainIf >= gainThreshold && !linkToDrop.isUnspecified() && !alternativeParent.isUnspecified())
		{
			// Drop costliest to cheapest
			dropNode(stripe, linkToDrop, alternativeParent);

			//int succParent = m_partnerList->getNumChildsSuccessors(stripe, alternativeParent);
			//int succDrop = m_partnerList->getNumChildsSuccessors(stripe, linkToDrop);
			//m_partnerList->updateNumChildsSuccessors(stripe, alternativeParent, succParent + 1 + succDrop);

			children[alternativeParent] += 1 + children[linkToDrop];
			children.erase(children.find(linkToDrop));
			gain = true;
			gainThreshold = getGainThreshold();
		}
	}

	int remainingBW = getMaxOutConnections() - m_partnerList->getNumOutgoingConnections();
		
	EV << "Currently have " << m_partnerList->getNumOutgoingConnections() <<
		" outgoing connections. Max: " << getMaxOutConnections() << " remaining: " << remainingBW << endl;

	std::set<IPvXAddress> curDisconnectingChildren = disconnectingChildren[stripe];

	// <node, remainingBW>
	std::map<IPvXAddress, int> requestNodes;
	while(remainingBW > 0)
	{
		int maxSucc = 0;
		IPvXAddress child;
		for (std::map<IPvXAddress, int>::iterator it = children.begin() ; it != children.end(); ++it)
		{
			if( it->second > maxSucc 
					&& curDisconnectingChildren.find(it->first) == curDisconnectingChildren.end() )
			{
				// Make sure I didnt already send a DisconenctRequest to the child, to not send
				// multiple DisconnectRequests
				maxSucc = it->second;
				child = it->first;
			}
		}

		if(child.isUnspecified() || maxSucc <= 0)
			break;

		remainingBW--;
		requestNodes[child]++;
		children[child]--;
	}

	EV << "threshold: " << gainThreshold << endl;
	
	for (std::map<IPvXAddress, int>::iterator it = requestNodes.begin() ; it != requestNodes.end(); ++it)
	{
		TreePassNodeRequestPacket *reqPkt = new TreePassNodeRequestPacket("TREE_PASS_NODE_REQUEST");

		PassNodeRequest request;
		request.stripe = stripe;
		request.threshold = gainThreshold;
		request.dependencyFactor = (double)(m_partnerList->getNumSuccessors(stripe) / 
					((double)m_partnerList->getNumOutgoingConnections(stripe) )) - 1;

		EV << "depFactor: " << request.dependencyFactor << endl;
		request.remainingBW = it->second;

		reqPkt->getRequests().push_back(request);

		EV << "Request " << request.remainingBW << " from " << it->first << ", stripe " << stripe << endl;
		numPNR++;
		sendToDispatcher(reqPkt, m_localPort, it->first, m_localPort);
	}
}

int MultitreePeer::getStripeToOptimize(void)
{
	int maxIndex = intrand(numStripes);
	int maxChildren = m_partnerList->getNumOutgoingConnections(maxIndex);
	int maxSucc = m_partnerList->getNumSuccessors(maxIndex);

	int startWith = maxIndex;
	for(int i = 0; i < numStripes; ++i)
	{
		int check = (startWith + i) % numStripes;
		int currentChildren = m_partnerList->getNumOutgoingConnections(check);
		int currentSucc = m_partnerList->getNumSuccessors(check);

		if( maxChildren < currentChildren || (maxChildren == currentChildren && maxSucc < currentSucc) )
		{
			maxIndex = check;
			maxChildren = currentChildren;
			maxSucc = currentSucc;
		}
	}

	return maxIndex;
}

bool MultitreePeer::isPreferredStripe(int stripe)
{
	int numChildren = m_partnerList->getNumOutgoingConnections(stripe);

	for (int i = 0; i < numStripes; i++)
	{
		if(i != stripe && m_partnerList->getNumOutgoingConnections(i) > numChildren)
			return false;
	}
	return true;
}
