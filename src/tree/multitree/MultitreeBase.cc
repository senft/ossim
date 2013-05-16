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

	if(m_state == TREE_JOIN_STATE_ACTIVE)
		TreeConnectRequestPacket *treePkt = check_and_cast<TreeConnectRequestPacket *>(pkt);
		int numRequestedStripes = (treePkt->getStripe() == -1) ? numStripes : 1;

	{
		if(hasBWLeft(numRequestedStripes))
		{
			EV << "Received TREE_CONECT_REQUEST (" << numRequestedStripes << " stripes). Accepting..." << endl;
			acceptConnectRequest(treePkt);

			EV << getNodeAddress();
			m_partnerList->printPartnerList();
		}
		else
		{
			 // No bandwith left
			EV << "Received TREE_CONECT_REQUEST (" << numRequestedStripes << " stripes). No Bandwidth left.  Rejecting..." << endl;
			rejectConnectRequest(treePkt);
		}
	}
	else
    {
		// Cannot process ConnectRequest
        throw cException("Uncovered state, check assignment of state variable!");
    }
}

void MultitreeBase::rejectConnectRequest(TreeConnectRequestPacket *pkt)
{
	IPvXAddress senderAddress;
	int senderPort;
	getSender(pkt, senderAddress, senderPort);

	TreeDisconnectRequestPacket *rejPkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");
	// TODO: choose a better alternative peer
	rejPkt->setAlternativeNode(m_apTable->getARandPeer(getNodeAddress()));
	sendToDispatcher(rejPkt, m_localPort, senderAddress, senderPort);
}

void MultitreeBase::acceptConnectRequest(TreeConnectRequestPacket *pkt)
{
	IPvXAddress senderAddress;
	getSender(pkt, senderAddress);

	TreeConnectConfirmPacket *acpPkt = new TreeConnectConfirmPacket("TREE_CONECT_CONFIRM");
	// TODO: this should also contain an alternative peer
	sendToDispatcher(acpPkt, m_localPort, senderAddress, m_destPort);

	MultitreeChildInfo child;
	child.setAddress(senderAddress);

	for (unsigned int i = 0; i < pkt->getNumSuccessorArraySize(); i++)
	{
		child.setNumSuccessors(i, pkt->getNumSuccessor(i));
	}

	int requestedStripe = pkt->getStripe();

	if(requestedStripe == -1)
	{
		// Requested all stripes
		m_partnerList->addChild(child);
	}
	else
	{
		m_partnerList->addChild(requestedStripe, child);
	}
}

void MultitreeBase::disconnectFromChild(IPvXAddress address)
{
	m_partnerList->removeChild(address);
}

void MultitreeBase::disconnectFromChild(int stripe, IPvXAddress address)
{
	m_partnerList->removeChild(stripe, address);
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

bool MultitreeBase::hasBWLeft(int additionalConnections)
{
	int outConnections = m_partnerList->getNumOutgoingConnections();
	int maxOutCon = getMaxOutConnections();

	EV << "Currently have " << outConnections << " outgoing connections, " <<
		additionalConnections << " have been requested, max=" << maxOutCon <<
		endl;

	return (outConnections + additionalConnections) <= getMaxOutConnections();
}
