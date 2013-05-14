#include "MultitreeBase.h"
#include "ChildInfo.h"
#include "DpControlInfo_m.h"

MultitreeBase::MultitreeBase(){
}

MultitreeBase::~MultitreeBase(){
}

void MultitreeBase::initialize(int stage)
{
	if(stage == 3)
	{
		bindToGlobalModule();
		bindToTreeModule();
		bindToStatisticModule();

		getAppSetting();
		findNodeAddress();

		bwCapacity = par("bwCapacity");
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

void MultitreeBase::getAppSetting(void)
{
	m_localPort = getLocalPort();
	m_destPort = getDestPort();

	numStripes = m_appSetting->getNumStripes();
}

void MultitreeBase::bindToGlobalModule(void)
{
    CommBase::bindToGlobalModule();

    cModule *temp = simulation.getModuleByPath("appSetting");
    m_appSetting = check_and_cast<AppSettingMultitree *>(temp);

	// -- Churn
    temp = simulation.getModuleByPath("churnModerator");
    m_churn = check_and_cast<IChurnGenerator *>(temp);
    EV << "Binding to churnModerator is completed successfully" << endl;
}

void MultitreeBase::bindToTreeModule(void)
{
    cModule *temp = getParentModule()->getModuleByRelativePath("forwarder");
    m_forwarder = check_and_cast<Forwarder *>(temp);

    temp = getParentModule()->getModuleByRelativePath("partnerList");
    m_partnerList = check_and_cast<MultitreePartnerList *>(temp);
}

void MultitreeBase::bindToStatisticModule(void){
}

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

		// TODO: Add alternative nodes to Packets
		if(hasBWLeft())
		{
			TreeConnectConfirmPacket *acpPkt = new TreeConnectConfirmPacket("TREE_CONECT_CONFIRM");
			sendToDispatcher(acpPkt, m_localPort, senderAddress, senderPort);

			ChildInfo child;
			child.setAddress(senderAddress);

			if(numRequestedStripes == 0)
			{
				// Requested all stripes
				EV << "Received TREE_CONECT_REQUEST (all stripes) FROM " << senderAddress
					<< ". Accepting..." << endl;

				m_partnerList->addChild(child);
			}
			else
			{
				// Requested only some stripes
				EV << "Received TREE_CONECT_REQUEST (" << numRequestedStripes << " stripes) FROM "
					<< senderAddress << ". Accepting..." << endl;

				int i;
				for (i = 0; i < numRequestedStripes; i++)
				{
					m_partnerList->addChild(treePkt->getStripes(i), child);
				}
			}
		}
		else // No bandwith left
		{
			EV << "Received TREE_CONECT_REQUEST (" << numRequestedStripes << " stripes) FROM "
				<< senderAddress << ". No Bandwidth left.  Rejecting..." << endl;

			TreeDisconnectRequestPacket *rejPkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");
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

	m_partnerList->printPartnerList();
}



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
	int outConnections = m_partnerList->getNumOutgoingConnections();
	return outConnections < getMaxOutConnections();
}
