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

	std::map<int, IPvXAddress> stripes = treePkt->getStripes();

	for (std::map<int, IPvXAddress>::iterator it = stripes.begin() ; it != stripes.end(); ++it)
	{
		int stripe = it->first;
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

int MultitreeSource::getMaxOutConnections()
{
	return numStripes * bwCapacity;
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

IPvXAddress MultitreeSource::getAlternativeNode(int stripe, IPvXAddress forNode)
{
	IPvXAddress node = m_partnerList->getRandomNodeFor(stripe, forNode);
	if(node.isUnspecified())
		return getNodeAddress();
	else
		return node;
}
