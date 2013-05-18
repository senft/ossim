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
		bool allActive = true;
		for (int i = 0; i < numStripes; i++)
		{
			if(m_state[i] != TREE_JOIN_STATE_ACTIVE)
			{
				allActive = false;
				break;
			}
		}
		if(allActive)
		{
			if(hasBWLeft(numRequestedStripes))
			{
				EV << "Received TREE_CONECT_REQUEST (" << numRequestedStripes << " stripe). Accepting..." << endl;
				acceptConnectRequest(treePkt);
			}
			else
			{
				 // No bandwith left
				EV << "Received TREE_CONECT_REQUEST (" << numRequestedStripes << " stripe). No Bandwidth left.  Rejecting..." 
					<< endl;
				rejectConnectRequest(treePkt);
			}

		}
		else
		{
			// TODO: Queue the request or something (really neccessary?)
		}
	}
	else // Requested one stripe
	{
		switch(m_state[stripe])
		{ 
		case TREE_JOIN_STATE_ACTIVE:
		{
			m_state[stripe] = TREE_JOIN_STATE_ACTIVE_WAITING;

			if(hasBWLeft(1))
			{
				EV << "Received TREE_CONECT_REQUEST (1 stripe). Accepting..." << endl;
				acceptConnectRequest(treePkt);
			}
			else
			{
				 // No bandwith left
				EV << "Received TREE_CONECT_REQUEST (1 stripe). No Bandwidth left.  Rejecting..." << endl;
				rejectConnectRequest(treePkt);
			}

			m_state[stripe] = TREE_JOIN_STATE_ACTIVE;
			break;
		}
		case TREE_JOIN_STATE_ACTIVE_WAITING:
		{
			// TODO: Queue the request or something (really neccessary?)
			break;
		}
		default:
		{
			throw cException("sup?");
			break;
		}
		}
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
	int numSuccArraySize = pkt->getNumSuccessorArraySize();

	if( numSuccArraySize != numStripes )
		throw cException("Received invalid ConnectRequest. Contains %d numbers of successors. Should be %d.",
				numSuccArraySize, numStripes);

	IPvXAddress senderAddress;
	getSender(pkt, senderAddress);
	int requestedStripe = pkt->getStripe();

	TreeConnectConfirmPacket *acpPkt = new TreeConnectConfirmPacket("TREE_CONECT_CONFIRM");
	acpPkt->setStripe(requestedStripe);
	// TODO: this should also contain an alternative peer
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

	scheduleInformParents();
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

	scheduleInformParents();
}

void MultitreeBase::disconnectFromChild(IPvXAddress address)
{
	m_partnerList->removeChild(address);
	scheduleInformParents();
}

void MultitreeBase::disconnectFromChild(int stripe, IPvXAddress address)
{
	m_partnerList->removeChild(stripe, address);
	scheduleInformParents();
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
