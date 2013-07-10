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

	printStatus();

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
	//printStatus();

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

	IPvXAddress address = m_partnerList->getChildWithMostChildren(stripe, skipNodes);
	//IPvXAddress address = m_partnerList->getBestLazyChild(stripe, skipNodes);
	//IPvXAddress address = m_partnerList->getChildWithLeastChildren(stripe, skipNodes);

	if(address.isUnspecified())
	{
		skipNodes.clear();
		skipNodes.insert(forNode);
		skipNodes.insert(currentParent);
		if(!lastRequests.empty())
			skipNodes.insert( lastRequests.back() );

		address = m_partnerList->getRandomChild(stripe, skipNodes);

		//address = m_partnerList->getChildWithMostChildren(stripe, skipNodes);
		//address = m_partnerList->getBestLazyChild(stripe, skipNodes);
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

	std::vector<std::map<IPvXAddress, int> > children;

	int maxIndex = 0;
	int maxChildren = m_partnerList->getNumOutgoingConnections(maxIndex);
	for (int stripe = 0; stripe < numStripes; stripe++)
	{
		// Get children of current stripe
        children.push_back(m_partnerList->getChildrenWithCount(stripe));

		int currentNumChildren = children[stripe].size();
		if(currentNumChildren > maxChildren)
		{
			maxChildren = currentNumChildren;
			maxIndex = stripe;
		}
	}

	int stripe = maxIndex;
	int steps = 0;
	while(steps++ < numStripes)
	{
		EV << "---------------------------------------------- OPTIMIZE, STRIPE: " << stripe << endl;


		bool gain = true;
		while(gain && children[stripe].size() > 1)
		{
			gain = false;

			IPvXAddress linkToDrop;	
			getCostliestChild(children[stripe], stripe, linkToDrop);

			IPvXAddress alternativeParent;	
			getCheapestChild(children[stripe], stripe, alternativeParent, linkToDrop);

			EV << "COSTLIEST CHILD: " << linkToDrop << endl;
			EV << "CHEAPEST CHILD: " << alternativeParent << endl;

			double gainIf = getGain(children[stripe], stripe, alternativeParent);

			EV << "GAIN: " << gainIf << endl;
			EV << "THRESHOLD: " << gainThreshold << endl;

			if(gainIf >= gainThreshold && !linkToDrop.isUnspecified() && !alternativeParent.isUnspecified())
			{
				// Drop costliest to cheapest
				dropNode(stripe, linkToDrop, alternativeParent);

				children[stripe][alternativeParent] += 1 + children[stripe][linkToDrop];
				children[stripe].erase(children[stripe].find(linkToDrop));
				gain = true;
			}
		}

		stripe = ++stripe % numStripes;

	}			

	EV << "Currently have " << m_partnerList->getNumOutgoingConnections() <<
		" outgoing connections. Max: " << getMaxOutConnections() << " remaining: " << remainingBW << endl;

	// TODO Start with tree with most children
	stripe = 0;

	// <node, <stripe, remainingBW> >
	std::map<IPvXAddress, std::map<int ,int> > requestNodes;
	int treesWithNoMoreChildren = 0;
	while(remainingBW > 0 && treesWithNoMoreChildren < numStripes)
	{

		int maxSucc = 0;
		IPvXAddress child;

		for (std::map<IPvXAddress, int>::iterator it = children[stripe].begin() ; it != children[stripe].end(); ++it)
		{
			if( it->second > maxSucc 
					&& disconnectingChildren[stripe].find(it->first) == disconnectingChildren[stripe].end() )
			{
				// Make sure I didnt already send a DisconenctRequest to the child, to not send
				// multiple DisconnectRequests
				maxSucc = it->second;
				child = it->first;
			}
		}

		if(child.isUnspecified() || maxSucc <= 0)
		{
			treesWithNoMoreChildren++;
			stripe = (stripe + 1) % numStripes;
			continue;
		}

		remainingBW--;
		requestNodes[child][stripe]++;
		children[stripe][child]--;

		stripe = (stripe + 1) % numStripes;
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

		numPNR++;
		sendToDispatcher(reqPkt, m_localPort, it->first, m_localPort);
	}
}
