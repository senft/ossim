#include "limits.h"
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


		m_videoBuffer->addListener(this);

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

    temp = getParentModule()->getModuleByRelativePath("videoBuffer");
    m_videoBuffer = check_and_cast<VideoBuffer *>(temp);

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

		optimize();
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

	bool doOptimize = false;
	if(requestedStripe == -1)
	{
		// Requested all stripes
		for (int i = 0; i < numStripes; i++)
		{
			m_partnerList->addChild(i, senderAddress, pkt->getNumSuccessor(i));
			if(!doOptimize && !isPreferredStripe(i))
				doOptimize = true;
		}
	}
	else
	{
		m_partnerList->addChild(requestedStripe, senderAddress, pkt->getNumSuccessor(requestedStripe));
		if(!isPreferredStripe(requestedStripe))
			doOptimize = true;
	}

    scheduleSuccessorInfo();

	if(doOptimize)
		optimize();
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
        m_partnerList->updateNumChildsSuccessors(i, address, treePkt->getNumSuccessor(i));
	}

    scheduleSuccessorInfo();

	// TODO: Maybe count the overall number of changes received here and only optimize if > X
	// paper says "only on major changes"
	optimize();
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
    int outConnections = m_partnerList->getNumChildren();
	return (outConnections + additionalConnections) <= getMaxOutConnections();
}

void MultitreeBase::optimize(void)
{
	EV << "----------------------------------------------------- OPTIMIZE" << endl;

	bool gain = true;

	while(gain)
	{
		gain = false;

		int costliestStripe;
		IPvXAddress costliestAddress;	
		getCostliestChild(costliestStripe, costliestAddress);

		int cheapestStripe;
		IPvXAddress cheapestAddress;	
		getCheapestChild(cheapestStripe, cheapestAddress, costliestStripe, costliestAddress);

		EV << "COSTLIEST CHILD: " << costliestAddress << ", stripe: " << costliestStripe << endl;
		EV << "CHEAPEST CHILD: " << cheapestAddress << ", stripe: " << cheapestStripe << endl;

		//if( 1 >= getGainThreshold() )
		//{
		//	// Drop costliest to cheapest
		//	gain = true;
		//}

	}

	// TODO while hasBWLeft() -> requestNode
}

void MultitreeBase::getCheapestChild(int &stripe, IPvXAddress &address, int skipStripe, IPvXAddress skipAddress)
{
	double curMaxGain = INT_MIN;
	int curMaxStripe = 0;
    IPvXAddress curMaxAddress;

	for (int i = 0; i < numStripes; i++)
	{
		std::vector<IPvXAddress> children = m_partnerList->getChildren(i);

		for(std::vector<IPvXAddress>::iterator it = children.begin(); it != children.end(); ++it)
		{
			IPvXAddress curAddress = (IPvXAddress)*it;

			if(i == skipStripe && curAddress.equals(skipAddress))
				continue;

				int curGain = getGain(i, curAddress);

				if(curMaxGain < curGain)
				{
					curMaxGain = curGain;
					curMaxStripe = i;
					curMaxAddress = curAddress;
				}
		}
	}
	stripe = curMaxStripe;
	address = curMaxAddress;
}


double MultitreeBase::getGain(int stripe, IPvXAddress child)
{
	// K_3 - K_2
	return getBalanceCosts(stripe, child) - getForwardingCosts(stripe, child);
}

void MultitreeBase::getCostliestChild(int &stripe, IPvXAddress &address)
{
	double curMaxCosts = INT_MIN;
	int curMaxStripe = 0;
    IPvXAddress curMaxAddress;

	for (int i = 0; i < numStripes; i++)
	{
		std::vector<IPvXAddress> children = m_partnerList->getChildren(i);

		for(std::vector<IPvXAddress>::iterator it = children.begin(); it != children.end(); ++it) {
			IPvXAddress curAddress = (IPvXAddress)*it;
			int curCosts = getCosts(i, curAddress);
			if(curMaxCosts < curCosts)
			{
				curMaxCosts = curCosts;
				curMaxStripe = i;
				curMaxAddress = curAddress;
			}
		}
	}
	stripe = curMaxStripe;
	address = curMaxAddress;
}

double MultitreeBase::getCosts(int stripe, IPvXAddress child)
{
	//EV << "****************************************" << endl;
	//EV << "Stripe: " << stripe << " Child: " << child << endl;
	//EV << "K1: " << getStripeDensityCosts(stripe) << endl;
	//EV << "K2: " << getForwardingCosts(stripe, child) << endl;
	//EV << "K3: " << getBalanceCosts(stripe, child) << endl;
	//EV << "K4: " << getDepencyCosts(child) << endl;
	//EV << "****************************************" << endl;

	// K_1 + 2 * K_2 + 3 * K_3 + 4 * K_4
	return getStripeDensityCosts(stripe)
		+ 2 * getForwardingCosts(stripe, child)
		+ 3 * getBalanceCosts(stripe, child)
		+ 4 * getDepencyCosts(child);
}

double MultitreeBase::getStripeDensityCosts(int stripe) // K_sel ,K_1
{
	return m_partnerList->getNumSuccessors(stripe) / (getMaxOutConnections() / numStripes);
}

int MultitreeBase::getForwardingCosts(int stripe, IPvXAddress child) // K_forw, K_2
{
    return (m_partnerList->getNumChildsSuccessors(stripe, child) == 0);
}

double MultitreeBase::getBalanceCosts(int stripe, IPvXAddress child) // K_bal, K_3
{
    int mySuccessors = m_partnerList->getNumSuccessors(stripe);
    int myChildren = m_partnerList->getNumChildren(stripe);
    double x = (mySuccessors / myChildren) - 1.0;

	if(x == 0) // TODO is this ok?
		return 0;

    int childsSuccessors = m_partnerList->getNumSuccessors(stripe, child);

    return   (x - childsSuccessors) / x;
}

double MultitreeBase::getDepencyCosts(IPvXAddress child) // K_4
{
    return 1.0 / numStripes;
}

void MultitreeBase::onNewChunk(int sequenceNumber)
{
	Enter_Method("onNewChunk");

	VideoChunkPacket *chunkPkt = m_videoBuffer->getChunk(sequenceNumber);
	VideoStripePacket *stripePkt = check_and_cast<VideoStripePacket *>(chunkPkt);

	int stripe = stripePkt->getStripe();
	int hopcount = stripePkt->getHopCount();


	stripePkt->setHopCount(++hopcount);

	std::vector<IPvXAddress> children = m_partnerList->getChildren(stripe);
	for(std::vector<IPvXAddress>::iterator it = children.begin(); it != children.end(); ++it)
	{
		sendToDispatcher(stripePkt->dup(), m_localPort, (IPvXAddress)*it, m_destPort);
	}
}
