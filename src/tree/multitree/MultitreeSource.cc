#include "MultitreeSource.h"

Define_Module(MultitreeSource)

MultitreeSource::MultitreeSource(){}
MultitreeSource::~MultitreeSource()
{
	finish();
}

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
	{m_state = TREE_JOIN_STATE_ACTIVE;
	m_apTable->addAddress(getNodeAddress());

	// -------------------------------------------------------------------------
    // -------------------------------- Timers ---------------------------------
    // -------------------------------------------------------------------------
    // -- One-time timers


    // -- Repeated timers


	}
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

	int stripe = treePkt->getStripe();

	EV << "Removing " << address.str() << " (stripe " << stripe << ")" << endl;
	disconnectFromChild(stripe, address);
}
 

void MultitreeSource::finish(void)
{
	cancelAndDeleteTimer();
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

int MultitreeSource::getMaxOutConnections()
{
	return numStripes * bwCapacity;
}
