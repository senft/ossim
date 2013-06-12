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

		requestedChildship = new IPvXAddress[numStripes];

		m_videoBuffer->addListener(this);

		// -------------------------------------------------------------------------
		// -------------------------------- Timers ---------------------------------
		// -------------------------------------------------------------------------
		// -- One-time timers
		
		// -- Repeated timers

		m_state = new TreeJoinState[numStripes];
		lastSeqNumber = -1L;
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

void MultitreeBase::bindToStatisticModule(void)
{
    cModule *temp = simulation.getModuleByPath("globalStatistic");
	m_gstat = check_and_cast<MultitreeStatistic *>(temp);
}

void MultitreeBase::processConnectRequest(cPacket *pkt)
{
	// TODO: Refactor?!
	IPvXAddress senderAddress;
    getSender(pkt, senderAddress);

	TreeConnectRequestPacket *treePkt = check_and_cast<TreeConnectRequestPacket *>(pkt);
	int numReqStripes = treePkt->getStripesArraySize();

	printStatus();

	if(hasBWLeft(numReqStripes))
	{
		// See if I could accept all requests
		bool canAccept = true;
		for (int i = 0; i < numReqStripes; ++i)
		{
			int stripe = treePkt->getStripes(i);
			if(m_state[stripe] != TREE_JOIN_STATE_ACTIVE
					|| m_partnerList->hasParent(stripe, senderAddress) 
					|| m_partnerList->hasChild(stripe, senderAddress) 
					|| requestedChildship[stripe].equals(senderAddress))
			{
				canAccept = false;
				break;
			}
		}

		if(canAccept)
		{
			// TODO: refactor
			TreeConnectConfirmPacket *acpPkt = new TreeConnectConfirmPacket("TREE_CONECT_CONFIRM");
			acpPkt->setStripesArraySize(numReqStripes);
			for (int i = 0; i < numReqStripes; ++i)
			{
				int stripe = treePkt->getStripes(i);
				int numSuccessors = treePkt->getNumSuccessor(i);
				acpPkt->setStripes(i, stripe);

				m_partnerList->addChild(stripe, senderAddress, numSuccessors);
			}
			acpPkt->setNextSequenceNumber(lastSeqNumber + 1);
			acpPkt->setAlternativeNode(getAlternativeNode(0, senderAddress));

			EV << "Accepting ConnectRequest for stripe(s) ";
			for (int i = 0; i < numReqStripes; ++i)
				EV << treePkt->getStripes(i) << " ";
			EV << "of " << senderAddress << endl;

			sendToDispatcher(acpPkt, m_localPort, senderAddress, m_destPort);

			int lastChunk = treePkt->getLastReceivedChunk();
			for (int i = 0; i < numReqStripes; ++i)
				sendChunksToNewChild(treePkt->getStripes(i), senderAddress, lastChunk);

			scheduleSuccessorInfo();
			return;
		}
	}

	// Cannot accept all requests, so process them one-by-one:
	// First process the preferred stripes
	for (int i = 0; i < numReqStripes; ++i)
	{
		int stripe = treePkt->getStripes(i);

		if(!isPreferredStripe(stripe))
			continue;

		if(m_state[stripe] != TREE_JOIN_STATE_ACTIVE)
		{
			EV << "Received ConnectRequest in for unconnected (not yet connected or leaving) stripe " << stripe << " Rejecting..." << endl;
			rejectConnectRequest(stripe, senderAddress);
		}
		else if( m_partnerList->hasParent(stripe, senderAddress) )
		{
			EV << "Received ConnectRequest from parent " << senderAddress << " for stripe " << stripe << ". Rejecting..." << endl;
			rejectConnectRequest(stripe, senderAddress);
		}
		else if( m_partnerList->hasChild(stripe, senderAddress) )
		{
			EV << "Received ConnectRequest from child " << senderAddress << " for stripe " << stripe << ". Ignoring..." << endl;
		}
		else if( requestedChildship[stripe].equals(senderAddress) )
		{
			// TODO: would be better to just queue this.. maybe the node rejects me
			EV << "Received ConnectRequest from a node (" << senderAddress << ") that I requested childship from for stripe " << stripe << ". Rejecting..." << endl;
			rejectConnectRequest(stripe, senderAddress);
		}
		else if(hasBWLeft(1))
		{
			acceptConnectRequest(stripe, senderAddress, treePkt->getNumSuccessor(i), treePkt->getLastReceivedChunk());
		}
		else
		{
			EV << "Received ConnectRequest but no bandwidth left. Rejecting..." << endl;
			rejectConnectRequest(stripe, senderAddress);
			//optimize();
		}
	}

	// Then process the un-preferred stripes
	for (int i = 0; i < numReqStripes; ++i)
	{
		int stripe = treePkt->getStripes(i);

		if(isPreferredStripe(stripe))
			continue;

		if(m_state[stripe] != TREE_JOIN_STATE_ACTIVE)
		{
			EV << "Received ConnectRequest in for unconnected (not yet connected or leaving) stripe " << stripe << " Rejecting..." << endl;
			rejectConnectRequest(stripe, senderAddress);
		}
		else if( m_partnerList->hasParent(stripe, senderAddress) )
		{
			EV << "Received ConnectRequest from parent " << senderAddress << " for stripe " << stripe << ". Rejecting..." << endl;
			rejectConnectRequest(stripe, senderAddress);
		}
		else if( m_partnerList->hasChild(stripe, senderAddress) )
		{
			EV << "Received ConnectRequest from child " << senderAddress << " for stripe " << stripe << ". Ignoring..." << endl;
		}
		else if(hasBWLeft(1))
		{
			acceptConnectRequest(stripe, senderAddress, treePkt->getNumSuccessor(i), treePkt->getLastReceivedChunk());
			//optimize();
		}
		else
		{
			EV << "Received ConnectRequest but no bandwidth left. Rejecting..." << endl;
			rejectConnectRequest(stripe, senderAddress);
			//optimize();
		}
	}
}

void MultitreeBase::rejectConnectRequest(int stripe, IPvXAddress address)
{
	TreeDisconnectRequestPacket *rejPkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");
    rejPkt->setStripesArraySize(1);
    rejPkt->setStripes(0, stripe);
	rejPkt->setAlternativeNode(getAlternativeNode(stripe, address));
    sendToDispatcher(rejPkt, m_localPort, address, m_destPort);
}

void MultitreeBase::acceptConnectRequest(int stripe, IPvXAddress address, int numSuccessors, int lastChunk)
{
	TreeConnectConfirmPacket *acpPkt = new TreeConnectConfirmPacket("TREE_CONECT_CONFIRM");
	acpPkt->setStripesArraySize(1);
	acpPkt->setStripes(0, stripe);
	acpPkt->setNextSequenceNumber(lastSeqNumber + 1);
    acpPkt->setAlternativeNode(getAlternativeNode(stripe, address));

	EV << "Accepting ConnectRequest for stripe " << stripe << " of " << address << endl;

    sendToDispatcher(acpPkt, m_localPort, address, m_destPort);

	sendChunksToNewChild(stripe, address, lastChunk);

	m_partnerList->addChild(stripe, address, numSuccessors);

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
        m_partnerList->updateNumChildsSuccessors(i, address, treePkt->getNumSuccessor(i));
	}

    scheduleSuccessorInfo();

	// Optimize when a node detects "major changes" in the topology below
	//optimize();
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

void MultitreeBase::optimize(void)
{

	bool gain = true;
	int stripe = getPreferredStripe();

	EV << "---------------------------------------------- OPTIMIZE, STRIPE: " << stripe << endl;

	while(gain && m_partnerList->getChildren(stripe).size() > 1)
	{
		gain = false;

		IPvXAddress linkToDrop;	
		getCostliestChild(stripe, linkToDrop);

		IPvXAddress alternativeParent;	
		getCheapestChild(stripe, alternativeParent, linkToDrop);

		EV << "COSTLIEST CHILD: " << linkToDrop << endl;
		EV << "CHEAPEST CHILD: " << alternativeParent << endl;

		EV << "GAIN: " << getGain(stripe, alternativeParent) << " drop " << linkToDrop << " to " << alternativeParent << endl;
		EV << "THRESHOLD: " << getGainThreshold() << endl;

		double gain = getGain(stripe, alternativeParent);

		if( gain >= getGainThreshold() )
		{
			// Drop costliest to cheapest
			EV << "DROP" << endl;
			dropChild(stripe, linkToDrop, alternativeParent);
			gain = true;
		}
	}
	// TODO while hasBWLeft() -> requestNode
}

void MultitreeBase::getCheapestChild(int fromStripe, IPvXAddress &address, IPvXAddress skipAddress)
{
    IPvXAddress curMaxAddress;
	double curMaxGain = INT_MIN;

	std::vector<IPvXAddress> children = m_partnerList->getChildren(fromStripe);

	for(std::vector<IPvXAddress>::iterator it = children.begin(); it != children.end(); ++it)
	{
		IPvXAddress curAddress = (IPvXAddress)*it;

		if(curAddress.equals(skipAddress))
			continue;

		int curGain = getGain(fromStripe, curAddress);

		if(curMaxGain < curGain)
		{
			curMaxGain = curGain;
			curMaxAddress = curAddress;
		}
	}
	address = curMaxAddress;
}


double MultitreeBase::getGain(int stripe, IPvXAddress child)
{
	//EV << "*************** GAIN ******************" << endl;
	//EV << "Stripe: " << stripe << " Child: " << child << endl;
	//EV << "K3: " << getBalanceCosts(stripe, child, IPvXAddress()) << endl;
	//EV << "K2: " << getForwardingCosts(stripe, child) << endl;
	//EV << "Total: " << getBalanceCosts(stripe, child, IPvXAddress()) - getForwardingCosts(stripe, child) << endl;
	//EV << "****************************************" << endl;

	// K_3 - K_2
	return getBalanceCosts(stripe, child, IPvXAddress()) - getForwardingCosts(stripe, child);
}

double MultitreeBase::getGain(int stripe, IPvXAddress child, IPvXAddress childToDrop)
{
	//EV << "********* GAIN WITH DROPCHILD ***********" << endl;
	//EV << "Stripe: " << stripe << " Child: " << child << endl;
	//EV << "K3: " << getBalanceCosts(stripe, child, childToDrop) << endl;
	//EV << "K2: " << getForwardingCosts(stripe, child) << endl;
	//EV << "Total: " << getBalanceCosts(stripe, child, childToDrop) - getForwardingCosts(stripe, child) << endl;
	//EV << "****************************************" << endl;

	// K_3 - K_2
	return getBalanceCosts(stripe, child, childToDrop) - getForwardingCosts(stripe, child);
}

void MultitreeBase::getCostliestChild(int fromStripe, IPvXAddress &address)
{
    IPvXAddress curMaxAddress;
	double curMaxCosts = INT_MIN;
	std::vector<IPvXAddress> children = m_partnerList->getChildren(fromStripe);

	for(std::vector<IPvXAddress>::iterator it = children.begin(); it != children.end(); ++it) {
		IPvXAddress curAddress = (IPvXAddress)*it;
		int curCosts = getCosts(fromStripe, curAddress);

		//EV << "checking: " << curAddress << ", costs: " << curCosts << endl;

		if(curMaxCosts < curCosts)
		{
			curMaxCosts = curCosts;
			curMaxAddress = curAddress;
		}
	}
	address = curMaxAddress;
}

double MultitreeBase::getCosts(int stripe, IPvXAddress child)
{
	//EV << "****************************************" << endl;
	//EV << "Stripe: " << stripe << " Child: " << child << endl;
	//EV << "K1: " << getStripeDensityCosts(stripe) << endl;
	//EV << "K2: " << getForwardingCosts(stripe, child) << endl;
	//EV << "K3: " << getBalanceCosts(stripe, child, IPvXAddress()) << endl;
	//EV << "K4: " << getDepencyCosts(child) << endl;
	//EV << "Total: " << getStripeDensityCosts(stripe)
	//	+ 2 * getForwardingCosts(stripe, child)
	//	+ 3 * getBalanceCosts(stripe, child, IPvXAddress())
	//	+ 4 * getDepencyCosts(child) << endl;
	//EV << "****************************************" << endl;

	// K_1 + 2 * K_2 + 3 * K_3 + 4 * K_4
	return getStripeDensityCosts(stripe)
		+ 2 * getForwardingCosts(stripe, child)
		+ 3 * getBalanceCosts(stripe, child, IPvXAddress())
		+ 4 * getDepencyCosts(child);
}

double MultitreeBase::getStripeDensityCosts(int stripe) // K_sel ,K_1
{
	int fanout = m_partnerList->getNumOutgoingConnections(stripe);
	int outCapacity = getMaxOutConnections() / numStripes;
	//EV << fanout << " " << outCapacity << endl;
	return 1 - (fanout / outCapacity);
}

int MultitreeBase::getForwardingCosts(int stripe, IPvXAddress child) // K_forw, K_2
{
    return (m_partnerList->getNumChildsSuccessors(stripe, child) == 0);
}

double MultitreeBase::getBalanceCosts(int stripe, IPvXAddress child, IPvXAddress childToDrop) // K_bal, K_3
{
    int mySuccessors = m_partnerList->getNumSuccessors(stripe);
    int myChildren = m_partnerList->getNumOutgoingConnections(stripe);

	if(childToDrop.isUnspecified())
	{
		myChildren--;
	//	mySuccessors -= m_partnerList->getNumOutgoingConnections(stripe, childToDrop);
		//mySuccessors = mySuccessors - m_partnerList->getNumOutgoingConnections(stripe, childToDrop);
	}

	if(myChildren == 0) // TODO is this ok?
		return 0;

    double x = (mySuccessors / myChildren) - 1.0;

	//EV << "mySucc: " << mySuccessors << " " << "myChil: " << myChildren << " "
	//	<< "bruch: " << x  << " " << "childsSucc: "
	//	<< m_partnerList->getNumOutgoingConnections(stripe, child) << endl;

	if(x == 0) // TODO is this ok?
		return 0;

    int childsSuccessors = m_partnerList->getNumChildsSuccessors(stripe, child);
	if(childToDrop.isUnspecified())
		childsSuccessors++;

    return   (x - childsSuccessors) / x;
}

double MultitreeBase::getDepencyCosts(IPvXAddress child) // K_4
{
	int numConnections = 0;

	for (int i = 0; i < numStripes; i++)
	{
		std::vector<IPvXAddress> curChildren = m_partnerList->getChildren(i);
		for(std::vector<IPvXAddress>::iterator it = curChildren.begin(); it != curChildren.end(); ++it)
		{
			if ( ((IPvXAddress)*it).equals(child) )
			{
				numConnections++;
				break;
			}
		}
	}

    return (double)numConnections / numStripes;
}

int MultitreeBase::getPreferredStripe()
{
	int max = 0;
	for (int i = 1; i < numStripes; i++)
	{
		if( m_partnerList->getNumOutgoingConnections(max) < m_partnerList->getNumSuccessors(i) )
			max = i;
	}
	return max;
}

double MultitreeBase::getGainThreshold(void)
{
	double t = 0.2;
	int b = getConnections() / bwCapacity;
	if(b == 1)
		return INT_MIN;

	return (1 - pow(b, pow(2 * t, 3)) * (1 - pow(t, 3))) + pow(t, 3);
}

void MultitreeBase::dropChild(int stripe, IPvXAddress address, IPvXAddress alternativeParent)
{
	TreeDisconnectRequestPacket *reqPkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");
	reqPkt->setAlternativeNode(alternativeParent);
	reqPkt->setStripesArraySize(1);
	reqPkt->setStripes(0, stripe);
	sendToDispatcher(reqPkt, m_localPort, address, m_destPort);

	//m_partnerList->removeChild(stripe, address);
}

int MultitreeBase::getConnections(void)
{
	int result = 0;

	for (int i = 0; i < numStripes; i++)
	{
		if(!m_partnerList->getParent(i).isUnspecified())
			result++;

		result += m_partnerList->getChildren(i).size();
	}

	return result;
}

IPvXAddress MultitreeBase::getAlternativeNode(int stripe, IPvXAddress forNode)
{
	IPvXAddress parent = m_partnerList->getParent(stripe);
	if(!parent.isUnspecified())
	{
		// normal node
		return parent;
	}
	else
	{
		// source node
		IPvXAddress node = m_partnerList->getRandomNodeFor(stripe, forNode);
		if(node.isUnspecified())
			return getNodeAddress();
		else
			return node;
	}
}

void MultitreeBase::printStatus(void)
{
	EV << "******************************" << endl;
	EV << getNodeAddress() << " (" << m_partnerList->getNumChildren() << " children, " 
		<< m_partnerList->getNumSuccessors() << " successors, " 
		<<  m_apTable->getNumActivePeer() << " node(s) in the system)" << endl;
	m_partnerList->printPartnerList();
}


void MultitreeBase::sendChunksToNewChild(int stripe, IPvXAddress address, int lastChunk)
{
	EV << "Childs last chunk was: " <<  lastChunk << ", my last forwarded chunk was: " << lastSeqNumber << endl;
	if(lastChunk != -1)
	{
		for (int i = lastChunk; i <= lastSeqNumber; i++)
		{
			if(m_videoBuffer->isInBuffer(i))
			{
				VideoChunkPacket *chunkPkt = m_videoBuffer->getChunk(i);
				VideoStripePacket *stripePkt = check_and_cast<VideoStripePacket *>(chunkPkt);

				if(stripePkt->getStripe() == stripe)
				{
					EV << "Sending chunk: " << i << endl;
					sendToDispatcher(stripePkt->dup(), m_localPort, address, m_destPort);
				}
			}
		}
	}

}
