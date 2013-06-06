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

	size_t numReqStripes = treePkt->getStripesArraySize();

	for (size_t i = 0; i < numReqStripes; ++i)
	{
		int stripe = treePkt->getStripes(i);
		EV << "Removing " << address.str() << " (stripe " << stripe << ")" << endl;
		disconnectFromChild(stripe, address);
	}

}
 

void MultitreeSource::handleTimerMessage(cMessage *msg)
{
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


void MultitreeSource::scheduleSuccessorInfo(void)
{ 
	// Do nothing because a source has no parents...
	EV << getNodeAddress();
	m_partnerList->printPartnerList();
}

int MultitreeSource::getMaxOutConnections()
{
	return numStripes * bwCapacity;
}

bool MultitreeSource::isPreferredStripe(int stripe)
{
	return true;
}

IPvXAddress MultitreeSource::getAlternativeNode(int stripe, IPvXAddress forNode)
{
	if(m_partnerList->getNumChildren() == 0)
		return getNodeAddress();

	if(stripe == -1)
	{
		// TODO Pick a child that is forwarding something..
		std::vector<IPvXAddress> children = m_partnerList->getChildren(intrand(numStripes));
		if(children.size() == 1)
		{
			if(children[0].equals(forNode))
				return getNodeAddress();
			else
				return children[0];
		}
		else if(children.size() > 1)
		{
			IPvXAddress candidate = children[intrand(children.size())];
			while(candidate.equals(forNode))
				candidate = children[intrand(children.size())];
			return candidate;
		}
	}
	else
	{
		// TODO Pick a child that is forwarding that stripe..

		std::vector<IPvXAddress> children = m_partnerList->getChildren(stripe);
		if(children.size() == 1)
		{
			if(children[0].equals(forNode))
				return getNodeAddress();
			else
				return children[0];
		}
		else if(children.size() > 1)
		{
			IPvXAddress candidate = children[intrand(children.size())];
			while(candidate.equals(forNode))
				// TODO: this hangs
				candidate = children[intrand(children.size())];
			return candidate;
		}
	}
}
