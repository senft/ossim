#include "MultitreeSource.h"

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

	std::vector<IPvXAddress> children = m_partnerList->getChildren(stripe);

	for(std::vector<IPvXAddress>::iterator it = children.begin(); it != children.end(); ++it)
	{
		sendToDispatcher(stripePkt->dup(), m_localPort, (IPvXAddress)*it, m_destPort);
	}
}

IPvXAddress MultitreeSource::getAlternativeNode(int stripe, IPvXAddress forNode, IPvXAddress currentParent, IPvXAddress lastRequest)
{
	std::set<IPvXAddress> skipNodes;
	skipNodes.insert(forNode);
	skipNodes.insert(currentParent);
	skipNodes.insert(lastRequest);

	IPvXAddress address = m_partnerList->getChildWithLeastSuccessors(stripe, skipNodes);

	if(address.isUnspecified())
		address = getNodeAddress();
	return address;
}

void MultitreeSource::optimize(void)
{
	//int stripe = getPreferredStripe();

	printStatus();

	int remainingBW = getMaxOutConnections() - m_partnerList->getNumOutgoingConnections();

	std::vector<std::map<IPvXAddress, int> > children;
	for (int stripe = 0; stripe < numStripes; stripe++)
	{

		EV << "---------------------------------------------- OPTIMIZE, STRIPE: " << stripe << endl;

		// Get children of current stripe
        children.push_back(m_partnerList->getChildrenWithCount(stripe));

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

			//double gainIf = getGain(children[stripe], stripe, alternativeParent, linkToDrop);
			double gainIf = getGain(children[stripe], stripe, alternativeParent, IPvXAddress());

			EV << "GAIN: " << gainIf << endl;
			EV << "THRESHOLD: " << getGainThreshold() << endl;

			if(gainIf >= getGainThreshold() && !linkToDrop.isUnspecified() && !alternativeParent.isUnspecified())
			{
				// Drop costliest to cheapest
				dropNode(stripe, linkToDrop, alternativeParent);

				children[stripe][alternativeParent] += 1 + children[stripe][linkToDrop];
				children[stripe].erase(children[stripe].find(linkToDrop));
				gain = true;
			}
		}

	}			

	EV << "Currently have " << m_partnerList->getNumOutgoingConnections() <<
		" outgoing connections. Max: " << getMaxOutConnections() << " remaining: " << remainingBW << endl;

	int stripe = 0;

	// <node, <stripe, remainingBW> >
	std::map<IPvXAddress, std::map<int ,int> > requestNodes;
	while(remainingBW > 0)
	{
		int maxSucc = -1;
		IPvXAddress busiestChild;
		for (std::map<IPvXAddress, int>::iterator it = children[stripe].begin() ; it != children[stripe].end(); ++it)
		{
			if( it->second > maxSucc && disconnectingChildren[stripe].find(it->first) == disconnectingChildren[stripe].end() )
			{
				// More children and I didnt already send a DisconenctRequest there (the latter is
				// needed to not send multiple DisconnectRequests to a child)
				maxSucc = it->second;
				busiestChild = it->first;
			}
		}

		if(busiestChild.isUnspecified() || maxSucc <= 0 || remainingBW <= 0)
			break;

		remainingBW--;
		requestNodes[busiestChild][stripe]++;
		children[stripe][busiestChild]--;

		stripe = ++stripe % numStripes;
	}

	double threshold = getGainThreshold();
	double depFactor = (double)(m_partnerList->getNumSuccessors(stripe) / 
						(double)m_partnerList->getNumOutgoingConnections(stripe)) - 1;

	for (std::map<IPvXAddress, std::map<int, int> >::iterator it = requestNodes.begin() ; it != requestNodes.end(); ++it)
	{
		TreePassNodeRequestPacket *reqPkt = new TreePassNodeRequestPacket("TREE_PASS_NODE_REQUEST");

		for (std::map<int, int>::iterator stripes = it->second.begin() ; stripes != it->second.end(); ++stripes)
		{
			PassNodeRequest request;
			request.stripe = stripes->first;
			request.remainingBW = stripes->second;
			request.threshold = threshold;
			request.dependencyFactor = depFactor;

			reqPkt->getRequests().push_back(request);

			EV << "Request " << stripes->second << " from " << it->first << endl;
		}

		sendToDispatcher(reqPkt, m_localPort, it->first, m_localPort);
	}
}
