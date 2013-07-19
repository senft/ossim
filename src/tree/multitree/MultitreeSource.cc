#include "MultitreeSource.h"

#include <algorithm> 

Define_Module(MultitreeSource)

MultitreeSource::MultitreeSource(){}
MultitreeSource::~MultitreeSource(){}

void MultitreeSource::initialize(int stage)
{
	MultitreeBase::initialize(stage);

	if(stage != 3)
		return;
	if(stage == 2)
	{
		bindToGlobalModule();
		bindToTreeModule();
		bindToStatisticModule();
	}

	if(stage == 3)
	{
		for (int i = 0; i < numStripes; i++)
		{
			m_state[i] = TREE_JOIN_STATE_ACTIVE;
		}

		m_apTable->addAddress(getNodeAddress());

		// -------------------------------------------------------------------------
		// -------------------------------- Timers ---------------------------------
		// -------------------------------------------------------------------------
		// -- One-time timers


		// -- Repeated timers

		if(bwCapacity < 1)
			throw cException("The source has a bandwidth capacity < 1. This won't work.");

	}
}

void MultitreeSource::finish(void)
{
	MultitreeBase::finish();

	cancelAndDeleteTimer();
}

void MultitreeSource::processPacket(cPacket *pkt)
{
	PeerStreamingPacket *appMsg = check_and_cast<PeerStreamingPacket *>(pkt);
    if (appMsg->getPacketGroup() != PACKET_GROUP_TREE_OVERLAY)
    {
        throw cException("MultitreeSource::processPacket: received a wrong packet. Wrong packet type!");
    }

    TreePeerStreamingPacket *treeMsg = check_and_cast<TreePeerStreamingPacket *>(appMsg);
    switch (treeMsg->getPacketType())
    {
	case TREE_CONNECT_REQUEST:
    {
		processConnectRequest(treeMsg);
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
        throw cException("MultitreeSource::processPacket: Unrecognized packet types! %d", treeMsg->getPacketType());
        break;
    }
    }

    delete pkt;
}

void MultitreeSource::processDisconnectRequest(cPacket* pkt)
{
	TreeDisconnectRequestPacket *treePkt = check_and_cast<TreeDisconnectRequestPacket *>(pkt);

	IPvXAddress address;
	getSender(pkt, address);

	std::vector<DisconnectRequest> requests = treePkt->getRequests();

	for(std::vector<DisconnectRequest>::iterator it = requests.begin(); it != requests.end(); ++it)
	{
		int stripe = ((DisconnectRequest)*it).stripe;
		removeChild(stripe, address);
	}
}
 

void MultitreeSource::handleTimerMessage(cMessage *msg)
{
	if (msg == timer_optimization)
	{
        handleTimerOptimization();
	}
}

void MultitreeSource::bindToGlobalModule(void)
{
	MultitreeBase::bindToGlobalModule();
}

void MultitreeSource::bindToTreeModule(void)
{
	MultitreeBase::bindToTreeModule();
}

void MultitreeSource::cancelAndDeleteTimer(void)
{
}

void MultitreeSource::cancelAllTimer()
{
}


void MultitreeSource::scheduleSuccessorInfo(int stripe)
{ 
	// Do nothing because a source has no parents...
}

bool MultitreeSource::isPreferredStripe(int stripe)
{
	return true;
}

void MultitreeSource::onNewChunk(int sequenceNumber)
{
	Enter_Method("onNewChunk");

	VideoChunkPacket *chunkPkt = m_videoBuffer->getChunk(sequenceNumber);
	VideoStripePacket *stripePkt = check_and_cast<VideoStripePacket *>(chunkPkt);

	int stripe = stripePkt->getStripe();
	int hopcount = stripePkt->getHopCount();
	lastSeqNumber[stripe] = stripePkt->getSeqNumber();

	stripePkt->setHopCount(++hopcount);

	std::set<IPvXAddress> &children = m_partnerList->getChildren(stripe);

	for(std::set<IPvXAddress>::iterator it = children.begin(); it != children.end(); ++it)
	{
		sendToDispatcher(stripePkt->dup(), m_localPort, (IPvXAddress)*it, m_destPort);
	}
}

IPvXAddress MultitreeSource::getAlternativeNode(int stripe, IPvXAddress forNode, IPvXAddress currentParent, std::vector<IPvXAddress> lastRequests)
{
	//EV << "Searching alternative parent for " << forNode << ", currentParent="
	//	<< currentParent << ", alread tried to connect to " << lastRequests.size()
	//	<< " nodes." << endl;

	std::set<IPvXAddress> skipNodes;
	skipNodes.insert(forNode);
	skipNodes.insert(currentParent);
	for(std::vector<IPvXAddress>::iterator it = lastRequests.begin(); it != lastRequests.end(); ++it)
	{
		skipNodes.insert( (IPvXAddress)*it );
	}

	//IPvXAddress address = m_partnerList->getChildWithMostChildren(stripe, skipNodes);
	IPvXAddress address = m_partnerList->getBestLazyChild(stripe, skipNodes);
	//IPvXAddress address = m_partnerList->getChildWithLeastChildren(stripe, skipNodes);
	
	while(m_partnerList->nodeHasMoreChildrenInOtherStripe(stripe, address) && !address.isUnspecified())
	{
		skipNodes.insert(address);
		address = m_partnerList->getBestLazyChild(stripe, skipNodes);
	}

	if(address.isUnspecified())
	{
		skipNodes.clear();
		skipNodes.insert(forNode);
		skipNodes.insert(currentParent);
		if(!lastRequests.empty())
			skipNodes.insert( lastRequests.back() );

		//address = m_partnerList->getRandomChild(stripe, skipNodes);
		//address = m_partnerList->getChildWithMostChildren(stripe, skipNodes);
		address = m_partnerList->getBestLazyChild(stripe, skipNodes);
		//address = m_partnerList->getChildWithLeastChildren(stripe, skipNodes);
	}

	if(address.isUnspecified())
	{
		if(!lastRequests.empty())
			skipNodes.erase(skipNodes.find( lastRequests.back() ));

		address = m_partnerList->getRandomChild(stripe, skipNodes);
	}

	if(address.isUnspecified())
	{
		address = getNodeAddress();
	}

	return address;
}

void MultitreeSource::optimize(void)
{
	// TODO maybe start with the tree I have the most children in

	printStatus();

	int remainingBW = getMaxOutConnections() - m_partnerList->getNumOutgoingConnections();

	std::vector<std::map<IPvXAddress, std::vector<int> > > children;

	for (int stripe = 0; stripe < numStripes; stripe++)
	{
        children.push_back(m_partnerList->getChildrenWithCount(stripe));
	}

	int stripe;
	std::set<int> noGainIn;
	while(noGainIn.size() < numStripes)
	{
		// Get stripe with most children
		int maxChildren = INT_MIN;
		for (int i = 0; i < numStripes; i++)
		{
			if(noGainIn.find(i) != noGainIn.end())
				continue;

			int currentNumChildren = children[i].size();
			if(currentNumChildren > maxChildren || (currentNumChildren == maxChildren && intrand(2) == 0))
			{
				maxChildren = currentNumChildren;
				stripe = i;
			}
		}

		EV << "---------------------------------------------- OPTIMIZE, STRIPE: " << stripe << endl;

		bool gain = false;
		if(children[stripe].size() > 1)
		{
			IPvXAddress linkToDrop;	
			getCostliestChild(children[stripe], stripe, linkToDrop);

			IPvXAddress alternativeParent;	
			getCheapestChild(children[stripe], stripe, alternativeParent, linkToDrop);

			EV << "COSTLIEST CHILD: " << linkToDrop << endl;
			EV << "CHEAPEST CHILD: " << alternativeParent << endl;

			if(!linkToDrop.isUnspecified() && !alternativeParent.isUnspecified())
			{
				//double gainIf = getGain(children[stripe], stripe, alternativeParent);
				double gainIf = getGain(children[stripe], stripe, linkToDrop);
			
				EV << "GAIN: " << gainIf << endl;
				EV << "THRESHOLD: " << gainThreshold << endl;

				if(gainIf >= gainThreshold)
				{
					// Drop costliest to cheapest
					dropNode(stripe, linkToDrop, alternativeParent);

					int succParent = m_partnerList->getNumChildsSuccessors(stripe, alternativeParent);
					int succDrop = m_partnerList->getNumChildsSuccessors(stripe, linkToDrop);
					m_partnerList->updateNumChildsSuccessors(stripe, alternativeParent, succParent + 1 + succDrop);

					children[stripe][alternativeParent][stripe] += 1 + children[stripe][linkToDrop][stripe];
					children[stripe].erase(children[stripe].find(linkToDrop));
					gain = true;
				}
			}
		}

		if(!gain)
			noGainIn.insert(stripe);

	}			

	EV << "Currently have " << m_partnerList->getNumOutgoingConnections() <<
		" outgoing connections. Max: " << getMaxOutConnections() << " remaining: " << remainingBW << endl;

	// <node, <stripe, remainingBW> >
	std::map<IPvXAddress, std::map<int ,int> > requestNodes;
	std::set<int> treesWithNoMoreChildren;
	while(remainingBW > 0 && treesWithNoMoreChildren.size() < numStripes)
	{

		// Get stripe with least children...
		int minIndex = 0;
		int minChildren = INT_MAX;
		for (int i = 0; i < numStripes; i++)
		{
			if(treesWithNoMoreChildren.find(i) != treesWithNoMoreChildren.end())
				continue;

			int currentNumChildren = children[i].size();
			if(currentNumChildren < minChildren
					|| (currentNumChildren == minChildren && intrand(2) == 0)
					)
			{
				minChildren = currentNumChildren;
				minIndex = i;
			}
		}
		stripe = minIndex;

		EV << "Try to request in stripe " << stripe << endl;

		int maxSucc = 0;
		int maxActiveTrees = 0;
		IPvXAddress child;

		// ... and the node that should be requested from...
		for (std::map<IPvXAddress, std::vector<int> >::iterator it = children[stripe].begin() ; it != children[stripe].end(); ++it)
		{
			if( disconnectingChildren[stripe].find(it->first) == disconnectingChildren[stripe].end() 
					&& ( (m_partnerList->getNumActiveTrees(it->first) > maxActiveTrees && m_partnerList->nodeHasMoreChildrenInOtherStripe(stripe, it->first))
					&& ((it->second[stripe] > maxSucc && m_partnerList->getNumActiveTrees(it->first) >= maxActiveTrees )
					|| (it->second[stripe] == maxSucc && m_partnerList->getNumActiveTrees(it->first) == maxActiveTrees && intrand(2) == 0))
					)
					)
			{
				maxSucc = it->second[stripe];
				child = it->first;
				maxActiveTrees = m_partnerList->getNumActiveTrees(it->first);
			}
		}

		if(child.isUnspecified() || maxSucc <= 0)
		{
			for (std::map<IPvXAddress, std::vector<int> >::iterator it = children[stripe].begin() ; it != children[stripe].end(); ++it)
			{
				if( disconnectingChildren[stripe].find(it->first) == disconnectingChildren[stripe].end() 
						&& (it->second[stripe] > maxSucc && m_partnerList->getNumActiveTrees(it->first) >= maxActiveTrees )
						|| (it->second[stripe] == maxSucc && m_partnerList->getNumActiveTrees(it->first) == maxActiveTrees && intrand(2) == 0)
						)
				{
					maxSucc = it->second[stripe];
					child = it->first;
					maxActiveTrees = m_partnerList->getNumActiveTrees(it->first);
				}
			}
		}

		if(child.isUnspecified() || maxSucc <= 0)
		{
			treesWithNoMoreChildren.insert(stripe);
			continue;
		}

		// ... and request a node from that child

		remainingBW--;
		requestNodes[child][stripe]++;
		children[stripe][child][stripe]--;
	}

	for (std::map<IPvXAddress, std::map<int, int> >::iterator it = requestNodes.begin() ; it != requestNodes.end(); ++it)
	{
		TreePassNodeRequestPacket *reqPkt = new TreePassNodeRequestPacket("TREE_PASS_NODE_REQUEST");

		for (std::map<int, int>::iterator stripes = it->second.begin() ; stripes != it->second.end(); ++stripes)
		{
			double depFactor = (double)(m_partnerList->getNumSuccessors(stripes->first) / 
								((double)m_partnerList->getNumOutgoingConnections(stripes->first) )) - 1;

			PassNodeRequest request;
			request.stripe = stripes->first;
			request.remainingBW = stripes->second;
			request.threshold = gainThreshold;
			request.dependencyFactor = depFactor;

			reqPkt->getRequests().push_back(request);

			EV << "Request " << request.remainingBW << " from " << it->first << ", stripe " << stripes->first << endl;
		}

		m_gstat->reportMessagePNR();
		numPNR++;
		sendToDispatcher(reqPkt, m_localPort, it->first, m_localPort);
	}
}
