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

		requestedChildship = new IPvXAddress[numStripes];

		m_videoBuffer->addListener(this);

		// -------------------------------------------------------------------------
		// -------------------------------- Timers ---------------------------------
		// -------------------------------------------------------------------------
		// -- One-time timers
		
		// -- Repeated timers
		timer_optimization = new cMessage("TIMER_OPTIMIZATION");

		m_state = new TreeJoinState[numStripes];

		lastSeqNumber = new long[numStripes];
		for (int i = 0; i < numStripes; i++)
		{
			lastSeqNumber[i] = -1L;
		}

		param_delayOptimization = par("delayOptimization");

		bwCapacity = getBWCapacity();
	}
}

void MultitreeBase::finish(void)
{
	cancelAndDeleteTimer();

	m_partnerList->clear();
	delete[] m_state;
	delete[] requestedChildship;
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

void MultitreeBase::handleTimerOptimization(void)
{
	optimize();
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
	IPvXAddress senderAddress;
    getSender(pkt, senderAddress);

	TreeConnectRequestPacket *treePkt = check_and_cast<TreeConnectRequestPacket *>(pkt);
	std::map<int, int> stripes = treePkt->getStripes();

	std::map<int, int> accept;
	std::vector<int> reject;

	bool doOptimize = false;

	bool onlyPreferredStripes = true;
	// 2 runs: 1 for the preferred stripes, 1 for the remaining
	for (int i = 0; i < 2; i++)
	{
		for (std::map<int, int>::iterator it = stripes.begin(); it != stripes.end(); ++it)
		{
			int stripe = it->first;
			int numSucc = it->second;

			if( (onlyPreferredStripes && !isPreferredStripe(stripe))
					|| (!onlyPreferredStripes && isPreferredStripe(stripe)) )
				continue;

			if(m_state[stripe] != TREE_JOIN_STATE_ACTIVE)
			{
				if(m_state[stripe] == TREE_JOIN_STATE_LEAVING)
					EV << "Received ConnectRequest (stripe " << stripe << ") while leaving. Rejecting..." << endl;
				else 
					EV << "Received ConnectRequest for currently unconnected stripe " << stripe << " Rejecting..." << endl;
				reject.push_back(stripe);
			}
			else if( m_partnerList->hasParent(stripe, senderAddress) )
			{
				EV << "Received ConnectRequest from parent " << senderAddress << " for stripe " << stripe 
					<< ". Rejecting..." << endl;
				reject.push_back(stripe);
			}
			//else if( m_partnerList->hasChild(stripe, senderAddress) )
			//{
			//	EV << "Received ConnectRequest from child " << senderAddress << " for stripe " << stripe 
			//		<< ". Ignoring..." << endl;
			//}
			else if( requestedChildship[stripe].equals(senderAddress) )
			{
				// TODO: would be better to just queue this.. maybe the node rejects me
				EV << "Received ConnectRequest from a node (" << senderAddress << ") that I requested childship from for stripe "
					<< stripe << ". Rejecting..." << endl;
				reject.push_back(stripe);
			}
			else if(hasBWLeft(accept.size() + 1))
			{
				accept.insert( std::pair<int,int>(stripe, numSucc) );
				if(!isPreferredStripe(stripe))
					doOptimize  = true;
			}
			else
			{
				EV << "Received ConnectRequest (stripe " << stripe << ") but have no bandwidth left. Rejecting..." << endl;
				reject.push_back(stripe);
				doOptimize  = true;
			}
		}

		onlyPreferredStripes = false;
	}

	if(!accept.empty())
		acceptConnectRequests(accept, senderAddress, treePkt->getLastReceivedChunk());
	if(!reject.empty())
		rejectConnectRequests(reject, senderAddress);

	if(doOptimize)
		scheduleOptimization();
}

void MultitreeBase::rejectConnectRequests(const std::vector<int> &stripes, IPvXAddress address)
{
	int numStripes = stripes.size();

	TreeDisconnectRequestPacket *pkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");

	for (int i = 0; i < numStripes; i++)
	{
		int stripe = stripes[i];
		pkt->getStripes().insert( std::pair<int, IPvXAddress>(stripe, getAlternativeNode(stripe, address)) );
	}

    sendToDispatcher(pkt, m_localPort, address, m_destPort);
}

void MultitreeBase::acceptConnectRequests(const std::map<int, int> &stripes, IPvXAddress address, int lastChunk)
{
	TreeConnectConfirmPacket *pkt = new TreeConnectConfirmPacket("TREE_CONECT_CONFIRM");

	EV << "Accepting ConnectRequest for stripe(s) ";
	for (std::map<int, int>::const_iterator it = stripes.begin(); it != stripes.end(); ++it)
	{
		int stripe = it->first;
		int numSucc = it->second;
		IPvXAddress alternativeParent = getAlternativeNode(stripe, address);

		pkt->getStripes().insert( std::pair<int, IPvXAddress>(stripe, alternativeParent) );

		m_partnerList->addChild(stripe, address, numSucc);

		EV << stripe << ", ";
	}
	EV << " of node " << address << endl;

    sendToDispatcher(pkt, m_localPort, address, m_destPort);

	for (std::map<int, int>::const_iterator it = stripes.begin() ; it != stripes.end(); ++it)
	{
		int stripe = it->first;
		scheduleSuccessorInfo(stripe);
		// TODO check if the first maybe can be skipped
		sendChunksToNewChild(stripe, address, lastChunk);
	}

	printStatus();
}

void MultitreeBase::processSuccessorUpdate(cPacket *pkt)
{
    TreeSuccessorInfoPacket *treePkt = check_and_cast<TreeSuccessorInfoPacket *>(pkt);

	std::map<int, int> stripes = treePkt->getStripes();

	IPvXAddress address;
	getSender(pkt, address);

	bool changes = false;

	for (std::map<int, int>::iterator it = stripes.begin() ; it != stripes.end(); ++it)
	{
		int stripe = it->first;
		if(m_partnerList->hasChild(stripe, address))
		{
			int numSucc = it->second;
			int oldSucc = m_partnerList->getNumChildsSuccessors(stripe, address);

			if(numSucc != oldSucc)
			{
				changes = true;
				scheduleSuccessorInfo(stripe);
				m_partnerList->updateNumChildsSuccessors(stripe, address, numSucc);
			}
		}
	}

	if(changes)
	{
		printStatus();

		// Optimize when a node detects "major changes" in the topology below
		scheduleOptimization();
	}
}

void MultitreeBase::removeChild(int stripe, IPvXAddress address)
{
	EV << "Removing child: " << address << " (stripe: " << stripe << ")" << endl;
	m_partnerList->removeChild(stripe, address);
    scheduleSuccessorInfo(stripe);
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
	//EV << "BW: current: " << outConnections << ", req: " << additionalConnections << ", max: " << getMaxOutConnections() << endl;
	return (outConnections + additionalConnections) <= getMaxOutConnections();
}

void MultitreeBase::scheduleOptimization(void)
{
	if(timer_optimization->isScheduled())
		return;

	EV << "SCHEDULING OPTIMIZATION to: " << simTime() + param_delayOptimization << endl;
    scheduleAt(simTime() + param_delayOptimization, timer_optimization);
}

void MultitreeBase::optimize(void)
{
	for (int i = 0; i < numStripes; i++)
		if(m_state[i] != TREE_JOIN_STATE_ACTIVE)
			return;

	if(!m_partnerList->hasChildren() || m_partnerList->getNumSuccessors() < 2)
		return;

	int stripe;

	// TODO maybe start with a random stripe here, so not every node picks stripe 0 as its preferred
	for (stripe = 0; stripe < numStripes; stripe++)
	{
		// This loop runs only 1 time in normal nodes, but multiple times in the source
		if(!isPreferredStripe(stripe))
			continue;

		bool gain = true;

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

			EV << "GAIN: " << getGain(stripe, alternativeParent, linkToDrop) << endl;
			EV << "THRESHOLD: " << getGainThreshold() << endl;

			double gain = getGain(stripe, alternativeParent, linkToDrop);

			if(gain >= getGainThreshold())
			{
				// Drop costliest to cheapest
				EV << "DROP " << linkToDrop << " (stripe: " << stripe << ") to " << alternativeParent << endl;
				dropChild(stripe, linkToDrop, alternativeParent);
				gain = true;
			}
		}

		int remainingBW = getMaxOutConnections() - m_partnerList->getNumOutgoingConnections();
		EV << "Currently have " << m_partnerList->getNumOutgoingConnections() <<
			" outgoing connections. Max: " << getMaxOutConnections() << endl;


		while(remainingBW > 0)
		{
			IPvXAddress busiestChild = m_partnerList->getBusiestChild(stripe);

			// TODO this should request nodes from all children
			int childsSuccessors = m_partnerList->getNumChildsSuccessors(stripe, busiestChild);
			if(childsSuccessors > 0)
			{
				// Only request nodes if the child HAS successors
				TreePassNodeRequestPacket *reqPkt = new TreePassNodeRequestPacket("TREE_PASS_NODE_REQUEST");

				if(childsSuccessors <= remainingBW)
				{
					reqPkt->setRemainingBW(childsSuccessors);
					remainingBW = remainingBW - childsSuccessors;
					remainingBW = 0;
				}
				else
				{
					reqPkt->setRemainingBW(remainingBW);
					remainingBW = 0;
				}

				reqPkt->setStripe(stripe);
				reqPkt->setThreshold(getGainThreshold());
				reqPkt->setDependencyFactor( (m_partnerList->getNumSuccessors(stripe) / 
							m_partnerList->getNumOutgoingConnections(stripe)) - 1 );

				sendToDispatcher(reqPkt, m_localPort, busiestChild, m_localPort);
			}	
			else
			{
				break;
			}
		}
	}
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

		double curGain = getGain(fromStripe, curAddress, IPvXAddress());

		//EV << "checking: " << curAddress << ", gain: " << curGain << endl;

		if(curMaxGain < curGain)
		{
			curMaxGain = curGain;
			curMaxAddress = curAddress;
		}
	}
	address = curMaxAddress;
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
		double curCosts = getCosts(fromStripe, curAddress);

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
	//EV << "K1: " << getStripeDensityCosts(stripe);
	//EV << " K2: " << getForwardingCosts(stripe, child);
	//EV << " K3: " << getBalanceCosts(stripe, child, IPvXAddress());
	//EV << " K4: " << getDepencyCosts(child);
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
	float outCapacity = bwCapacity - 1;
	//EV << "K1: " << fanout << " " << outCapacity << endl;
	return 1 - (fanout / outCapacity);
}

int MultitreeBase::getForwardingCosts(int stripe, IPvXAddress child) // K_forw, K_2
{
    return (m_partnerList->getNumChildsSuccessors(stripe, child) == 0);
}

double MultitreeBase::getBalanceCosts(int stripe, IPvXAddress child, IPvXAddress childToDrop) // K_bal, K_3
{
    double mySuccessors = m_partnerList->getNumSuccessors(stripe);
    double myChildren = m_partnerList->getNumOutgoingConnections(stripe);

    int childsSuccessors = m_partnerList->getNumChildsSuccessors(stripe, child);

	// EV << "fanout: " << myChildren << " mySucc: " << mySuccessors << " childsSucc: " << childsSuccessors << endl; 

	if(!childToDrop.isUnspecified())
		myChildren--;

	if(myChildren == 0) // TODO is this ok?
		return 0;

    double x = (mySuccessors / myChildren) - 1.0;

	if(x == 0) // TODO is this ok?
		return 0;

	if(!childToDrop.isUnspecified())
		childsSuccessors++;

    return  (x - childsSuccessors) / x;
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
		if( m_partnerList->getNumOutgoingConnections(max) < m_partnerList->getNumOutgoingConnections(i) )
			max = i;
	}
	return max;
}

double MultitreeBase::getGainThreshold(void)
{
	float t = 0.2;
	float b = getConnections() / (bwCapacity * numStripes);

	//EV << "deg: " << getConnections() << " max: " << (bwCapacity * numStripes) << " b: " << b << endl;

	if(b == 1)
		return INT_MIN;

	return (1 - pow(b, pow(2 * t, 3)) * (1 - pow(t, 3))) + pow(t, 3);
}

// TODO rename to dropNode and make stripe a vector
void MultitreeBase::dropChild(int stripe, IPvXAddress address, IPvXAddress alternativeParent)
{
	TreeDisconnectRequestPacket *pkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");
	pkt->getStripes().insert( std::pair<int, IPvXAddress>( stripe, alternativeParent ) );
	sendToDispatcher(pkt, m_localPort, address, m_destPort);
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
	//EV << "Childs last chunk was: " <<  lastChunk << ", my last forwarded chunk was: " << lastSeqNumber << endl;
	if(lastChunk != -1)
	{
		for (int i = lastChunk; i <= lastSeqNumber[stripe]; i++)
		{
			if(m_videoBuffer->isInBuffer(i))
			{
				VideoChunkPacket *chunkPkt = m_videoBuffer->getChunk(i);
				VideoStripePacket *stripePkt = check_and_cast<VideoStripePacket *>(chunkPkt);

				if(stripePkt->getStripe() == stripe)
				{
					//EV << "Sending chunk: " << i << endl;
					sendToDispatcher(stripePkt->dup(), m_localPort, address, m_destPort);
				}
			}
		}
	}

}

double MultitreeBase::getBWCapacity(void)
{
	double rate;
    cModule* nodeModule = getParentModule();

    int gateSize = nodeModule->gateSize("pppg$o");

    for (int i = 0; i < gateSize; i++)
    {
        cGate* currentGate = nodeModule->gate("pppg$o", i);
        if(currentGate->isConnected())
        {
            rate = check_and_cast<cDatarateChannel *>(currentGate->getChannel())->getDatarate();
        }
    }

	double capacity = (rate / m_appSetting->getVideoStreamBitRate()) - 1;// + 1;
	EV << "Detected bandwidth capacity of " << capacity << endl;
	return capacity;
}

void MultitreeBase::cancelAndDeleteTimer(void)
{
	if(timer_optimization != NULL)
	{
		delete cancelEvent(timer_optimization);
		timer_optimization = NULL;
	}
}
