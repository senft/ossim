#include "MultitreeBase.h"
#include "DpControlInfo_m.h"

MultitreeBase::MultitreeBase(){

}

MultitreeBase::~MultitreeBase(){
}

void MultitreeBase::initialize(int stage)
{
	if(stage == 3)
	{
		m_localPort = getLocalPort();
		m_destPort = getDestPort();

		findNodeAddress();
		// TODO: This should happen not until the node is connected to the tree (else we get un-connected trees)
		m_apTable->addAddress(getNodeAddress());
	}
}



void MultitreeBase::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        handleTimerMessage(msg);
    }
    else
    {
        processPacket(PK(msg));
    }
}

void MultitreeBase::processPacket(cPacket *pkt)
{
	PeerStreamingPacket *appMsg = check_and_cast<PeerStreamingPacket *>(pkt);
    if (appMsg->getPacketGroup() != PACKET_GROUP_TREE_OVERLAY)
    {
        throw cException("MultitreBase::processPacket received a wrong packet. Wrong packet type!");
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
    default:
    {
        throw cException("Unrecognized packet types! %d", treeMsg->getPacketType());
        break;
    }
    } // switch

    delete pkt;

}

void MultitreeBase::bindToGlobalModule(void)
{
    CommBase::bindToGlobalModule();

    cModule *temp = simulation.getModuleByPath("appSetting");
    m_appSetting = check_and_cast<AppSettingDonet *>(temp);

    temp = simulation.getModuleByPath("activePeerTable");
    m_apTable = check_and_cast<ActivePeerTable *>(temp);
}


void MultitreeBase::bindToStatisticModule(void){}

void MultitreeBase::processConnectRequest(cPacket *pkt)
{
	switch(m_state)
	{
	case TREE_JOIN_STATE_ACTIVE:
	{
		TreeConnectRequestPacket *treePkt = check_and_cast<TreeConnectRequestPacket *>(pkt);

		IPvXAddress senderAddress;
		int senderPort;
		getSender(pkt, senderAddress, senderPort);

		
		int numRequestedStripes = treePkt->getStripesArraySize();

		if(numRequestedStripes == 0)
		{
			// Requested all stripes
			// EV << "requested all stripes";
		}
		else
		{
			int stripes[numRequestedStripes];
			int i;
			for (i = 0; i < numRequestedStripes; i++)
			{
				stripes[i] = treePkt->getStripes(i);
			}

			// EV << "requested " << numRequestedStripes << " stripes";
		}

		// TODO: Add real alternative node

		if(hasBWLeft())
		{
			EV << "Received TREE_CONECT_REQUEST FROM " << senderAddress << ". Accepting..." << endl;
			TreeConnectConfirmPacket *acpPkt = new TreeConnectConfirmPacket("TREE_CONECT_CONFIRM");
			acpPkt->setAltNode("hallo");
			sendToDispatcher(acpPkt, m_localPort, senderAddress, senderPort);
		}
		else
		{
			EV << "Received TREE_CONECT_REQUEST FROM " << senderAddress << ". No Bandwidth left. Rejecting..." << endl;
			TreeDisconnectRequestPacket *rejPkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");
			rejPkt->setAltNode("hallo");
			sendToDispatcher(rejPkt, m_localPort, senderAddress, senderPort);
		}
		break;
	}
	case TREE_JOIN_STATE_ACTIVE_WAITING:
	{
		// TODO: queue incoming request
		break;
	}
	case TREE_JOIN_STATE_IDLE:
	{
		// TODO: implement
		break;
	}
	case TREE_JOIN_STATE_IDLE_WAITING:
	{
		// TODO: implement
		break;
	}
	default:
    {
		// Cannot process ConnectRequest
        throw cException("Uncovered state, check assignment of state variable!");
        break;
    }
	}
}
 
void MultitreeBase::processConnectConfirm(cPacket* pkt)
{
	if(m_state != TREE_JOIN_STATE_IDLE_WAITING)
		return;
}

void MultitreeBase::processDisconnectRequest(cPacket* pkt)
{}

void MultitreeBase::getSender(cPacket *pkt, IPvXAddress &senderAddress, int &senderPort)
{
    DpControlInfo *controlInfo = check_and_cast<DpControlInfo *>(pkt->getControlInfo());
	senderAddress   = controlInfo->getSrcAddr();
	senderPort      = controlInfo->getSrcPort();
}

void MultitreeBase::getSender(cPacket *pkt, IPvXAddress &senderAddress)
{
    DpControlInfo *controlInfo = check_and_cast<DpControlInfo *>(pkt->getControlInfo());
        senderAddress   = controlInfo->getSrcAddr();
}

const IPvXAddress& MultitreeBase::getSender(const cPacket *pkt) const
{
    DpControlInfo *controlInfo = check_and_cast<DpControlInfo *>(pkt->getControlInfo());
        return controlInfo->getSrcAddr();
}

bool MultitreeBase::hasBWLeft(void)
{
	// TODO: implement
	return true;
}
