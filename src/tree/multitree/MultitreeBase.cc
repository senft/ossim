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

		// -------------------------------------------------------------------------
		// -------------------------------- Timers ---------------------------------
		// -------------------------------------------------------------------------
		// -- One-time timers
		
		// -- Repeated timers


		m_state = new TreeJoinState[numStripes];
	}
}

void MultitreeBase::finish(void)
{
	m_partnerList->clear();
	delete [] m_state;
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
	TreeConnectRequestPacket *treePkt = check_and_cast<TreeConnectRequestPacket *>(pkt);
	int stripe = treePkt->getStripe();
	int numRequestedStripes = (stripe == -1) ? numStripes : 1;

	if(stripe == -1) // Requested all stripes
	{
        for (int i = 0; i < numStripes; i++)
        {
            if(m_state[i] != TREE_JOIN_STATE_ACTIVE)
            {
                EV << "CR in for unconnected stripe " << stripe << " Rejecting..." << endl;
                rejectConnectRequest(treePkt);
                return;
            }
        }
    }
    else // Requested one stripes
    {
        if (m_state[stripe] != TREE_JOIN_STATE_ACTIVE)
        {
            EV << "CR in for unconnected stripe " << stripe << " Rejecting..." << endl;
            rejectConnectRequest(treePkt);
            return;
        }
    }

    if(hasBWLeft(numRequestedStripes))
    {
        EV << "Received CR (" << numRequestedStripes << " stripe). Accepting..." << endl;
        acceptConnectRequest(treePkt);
    }
    else
    {
         // No bandwith left
        EV << "Received CR (" << numRequestedStripes << " stripe). No Bandwidth left.  Rejecting..."
            << endl;
        rejectConnectRequest(treePkt);
    }
}

void MultitreeBase::rejectConnectRequest(TreeConnectRequestPacket *pkt)
{
	IPvXAddress senderAddress;
    getSender(pkt, senderAddress);

	TreeDisconnectRequestPacket *rejPkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");
	// TODO: choose a better alternative peer
    rejPkt->setStripe(pkt->getStripe());
	rejPkt->setAlternativeNode(m_apTable->getARandPeer(getNodeAddress()));
    sendToDispatcher(rejPkt, m_localPort, senderAddress, m_destPort);
}

void MultitreeBase::acceptConnectRequest(TreeConnectRequestPacket *pkt)
{
	int numSuccArraySize = pkt->getNumSuccessorArraySize();

	if( numSuccArraySize != numStripes )
        throw cException("Received invalid CR. Contains %d numbers of successors. Should be %d.",
				numSuccArraySize, numStripes);

	IPvXAddress senderAddress;
	getSender(pkt, senderAddress);
	int requestedStripe = pkt->getStripe();

	TreeConnectConfirmPacket *acpPkt = new TreeConnectConfirmPacket("TREE_CONECT_CONFIRM");
	acpPkt->setStripe(requestedStripe);
    // TODO: choose a better alternative peer
    acpPkt->setAlternativeNode(m_apTable->getARandPeer(getNodeAddress()));
    sendToDispatcher(acpPkt, m_localPort, senderAddress, m_destPort);

	if(requestedStripe == -1)
	{
		// Requested all stripes
		for (int i = 0; i < numStripes; i++)
		{
			m_partnerList->addChild(i, senderAddress, pkt->getNumSuccessor(i));
		}
	}
	else
	{
		m_partnerList->addChild(requestedStripe, senderAddress, pkt->getNumSuccessor(requestedStripe));
	}

    scheduleSuccessorInfo();
}

void MultitreeBase::processSuccessorUpdate(cPacket *pkt)
{
    TreeSuccessorInfoPacket *treePkt = check_and_cast<TreeSuccessorInfoPacket *>(pkt);
	int arraySize = treePkt->getNumSuccessorArraySize();
	if(arraySize != numStripes)
		throw cException("Received invalid SuccessorInfo. Contains %d numbers of successors. Should be %d.",
				arraySize, numStripes);

	IPvXAddress address;
	getSender(pkt, address);

	for (int i = 0; i < numStripes; i++)
	{
		m_partnerList->updateNumSuccessor(i, address, treePkt->getNumSuccessor(i));
	}

    scheduleSuccessorInfo();
}

void MultitreeBase::disconnectFromChild(IPvXAddress address)
{
	m_partnerList->removeChild(address);
    scheduleSuccessorInfo();
}

void MultitreeBase::disconnectFromChild(int stripe, IPvXAddress address)
{
	m_partnerList->removeChild(stripe, address);
    scheduleSuccessorInfo();
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
	return (outConnections + additionalConnections) <= getMaxOutConnections();
}

double MultitreeBase::getStripeDensityCosts(int stripe)
{
	return m_partnerList->getNumSuccessors(stripe) / (bwCapacity - 1);
}
