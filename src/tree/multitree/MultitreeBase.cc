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

		m_videoBuffer->addListener(this);

		// -------------------------------------------------------------------------
		// -------------------------------- Timers ---------------------------------
		// -------------------------------------------------------------------------
		// -- One-time timers
		
		// -- Repeated timers
		timer_optimization = new cMessage("TIMER_OPTIMIZATION");

		m_state = new TreeJoinState[numStripes];

		preferredStripe = -1;

		requestedChildship = new IPvXAddress[numStripes];
		lastSeqNumber = new long[numStripes];
		for (int i = 0; i < numStripes; i++)
		{
			lastSeqNumber[i] = -1L;
		}

		param_delayOptimization = par("delayOptimization");
		param_optimize = par("optimize");

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
	std::vector<ConnectRequest> requests = treePkt->getRequests();

	std::vector<ConnectRequest> accept;
	std::vector<ConnectRequest> reject;

	bool doOptimize = false;

	bool onlyPreferredStripes = true;
	// 2 runs: 1 for the preferred stripes, 1 for the remaining
	for (int i = 0; i < 2; i++)
	{
		for(std::vector<ConnectRequest>::iterator it = requests.begin(); it != requests.end(); ++it)
		{
			const ConnectRequest request = (ConnectRequest)*it;
			int stripe = request.stripe;
			
			if( (onlyPreferredStripes && !isPreferredStripe(stripe))
					|| (!onlyPreferredStripes && isPreferredStripe(stripe)) )
				continue;

			if(m_state[stripe] == TREE_JOIN_STATE_LEAVING)
			{
				EV << "Received ConnectRequest (stripe " << stripe << ") while leaving. Rejecting..." << endl;
				reject.push_back(request);
			}
			else if( m_partnerList->hasParent(stripe, senderAddress) )
			{
				EV << "Received ConnectRequest from parent " << senderAddress << " for stripe " << stripe 
					<< ". Rejecting..." << endl;
				reject.push_back(request);
			}
			else if( requestedChildship[stripe].equals(senderAddress) )
			{
				// TODO: would be better to just queue this.. maybe the node rejects me
				EV << "Received ConnectRequest from a node (" << senderAddress << ") that I requested childship from for stripe "
					<< stripe << ". Rejecting..." << endl;
				reject.push_back(request);
			}
			else if(hasBWLeft(accept.size() + 1))
			{
				accept.push_back(request);
				if(!isPreferredStripe(stripe))
					doOptimize  = true;
			}
			else
			{
				EV << "Received ConnectRequest from " << senderAddress << " (stripe " << stripe
					<< ") but have no bandwidth left. Rejecting..." << endl;
				reject.push_back(request);
				doOptimize  = true;
			}
		}

		onlyPreferredStripes = false;
	}

	if(!accept.empty())
		acceptConnectRequests(accept, senderAddress);
	if(!reject.empty())
		rejectConnectRequests(reject, senderAddress);

	if(doOptimize)
		scheduleOptimization();
}

void MultitreeBase::rejectConnectRequests(const std::vector<ConnectRequest> &requests, IPvXAddress address)
{
	int numReqStripes = requests.size();

	TreeDisconnectRequestPacket *pkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");

	for (int i = 0; i < numReqStripes; i++)
	{
		ConnectRequest cRequest = requests[i];
		int stripe = cRequest.stripe;
		IPvXAddress currentParent = cRequest.currentParent;

		IPvXAddress alternativeParent = getAlternativeNode(stripe, address, currentParent);

		DisconnectRequest dRequest;
		dRequest.stripe = stripe;
		dRequest.alternativeParent = alternativeParent;

		EV << "Alternative parent for " << address << " (stripe " << stripe << ") is " << alternativeParent << endl;

		pkt->getRequests().push_back(dRequest);
	}

    sendToDispatcher(pkt, m_localPort, address, m_destPort);
}

void MultitreeBase::acceptConnectRequests(const std::vector<ConnectRequest> &requests, IPvXAddress address)
{
	int numReqStripes = requests.size();

	TreeConnectConfirmPacket *pkt = new TreeConnectConfirmPacket("TREE_CONECT_CONFIRM");

	EV << "Accepting ConnectRequest for stripe(s) ";
	for (int i = 0; i < numReqStripes; i++)
	{
		ConnectRequest request = requests[i];
		int stripe = request.stripe;
		int numSucc = request.numSuccessors;
		IPvXAddress alternativeParent = getAlternativeNode(stripe, address, IPvXAddress());

		ConnectConfirm confirm;
		confirm.stripe = stripe;
		confirm.alternativeParent = alternativeParent;

		pkt->getConfirms().push_back(confirm);

		m_partnerList->addChild(stripe, address, numSucc);

		EV << stripe << ", ";
	}
	EV << " of node " << address << endl;

    sendToDispatcher(pkt, m_localPort, address, m_destPort);

	for (int i = 0; i < numReqStripes; i++)
	{
		int stripe = requests[i].stripe;
		scheduleSuccessorInfo(stripe);
	}

	printStatus();
}

void MultitreeBase::processSuccessorUpdate(cPacket *pkt)
{
    TreeSuccessorInfoPacket *treePkt = check_and_cast<TreeSuccessorInfoPacket *>(pkt);

	std::vector<SuccessorInfo> updates = treePkt->getUpdates();

	IPvXAddress address;
	getSender(pkt, address);

	bool changes = false;

	for(std::vector<SuccessorInfo>::iterator it = updates.begin(); it != updates.end(); ++it)
	{
		SuccessorInfo update = (SuccessorInfo)*it;
		int stripe = update.stripe;

		if(m_partnerList->hasChild(stripe, address))
		{
			int numSucc = update.numSuccessors;
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
	if(timer_optimization->isScheduled() || !param_optimize)
		return;

	EV << "Scheduling optimization to: " << simTime() + param_delayOptimization << endl;
    scheduleAt(simTime() + param_delayOptimization, timer_optimization);
}

void MultitreeBase::optimize(void)
{
	for (int i = 0; i < numStripes; i++)
		if(m_state[i] != TREE_JOIN_STATE_ACTIVE)
			return;

	if(!m_partnerList->hasChildren() || m_partnerList->getNumSuccessors() < 2)
		return;

	printStatus();

	int stripe;

	int remainingBW = getMaxOutConnections() - m_partnerList->getNumOutgoingConnections();
		
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
			// TODO I probably need to keep track of how many nodes I already dropped, in order to
			// calculate the costs correctly after dropping at least 1 node
			gain = false;

			IPvXAddress linkToDrop;	
			getCostliestChild(stripe, linkToDrop);

			IPvXAddress alternativeParent;	
			getCheapestChild(stripe, alternativeParent, linkToDrop);

			EV << "COSTLIEST CHILD: " << linkToDrop << endl;
			EV << "CHEAPEST CHILD: " << alternativeParent << endl;

			double gain = getGain(stripe, alternativeParent, linkToDrop);

			//EV << "GAIN: " << gain << endl;
			//EV << "THRESHOLD: " << getGainThreshold() << endl;

			if(gain >= getGainThreshold())
			{
				// Make sure the source doesnt drop it's last child of a stripe (the parent [in an
				// optimizing node] should only be unspec in the source)
				if(!(m_partnerList->getParent(stripe).isUnspecified() && m_partnerList->getNumOutgoingConnections(stripe)))
				{

					// Drop costliest to cheapest
					EV << "DROP " << linkToDrop << " (stripe: " << stripe << ") to " << alternativeParent << endl;
					dropNode(stripe, linkToDrop, alternativeParent);
					gain = true;
				}
			}
		}

		EV << "Currently have " << m_partnerList->getNumOutgoingConnections() <<
			" outgoing connections. Max: " << getMaxOutConnections() << " remaining: " << remainingBW << endl;

		while(remainingBW > 0)
		{
			IPvXAddress busiestChild = m_partnerList->getBusiestChild(stripe);

			// TODO this should request nodes from all children (then also delete the remainingBW =
			// 0 in the first if
			int childsSuccessors = m_partnerList->getNumChildsSuccessors(stripe, busiestChild);

			// Only request nodes if the child HAS successors
			if(childsSuccessors > 0)
			{
				TreePassNodeRequestPacket *reqPkt = new TreePassNodeRequestPacket("TREE_PASS_NODE_REQUEST");

				if(childsSuccessors <= remainingBW)
				{
					reqPkt->setRemainingBW(childsSuccessors);
					remainingBW = remainingBW - childsSuccessors;
					remainingBW = 0; // TODO delete this when requesting nodes from more than 1 child
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
	float outCapacity = bwCapacity;
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

    return (double)numConnections / (double)numStripes;
}

int MultitreeBase::getPreferredStripe()
{
	if(preferredStripe != -1 && isPreferredStripe(preferredStripe))
		return preferredStripe;

	int max = 0;
	for (int i = 1; i < numStripes; i++)
	{
		// Introduce randomness so not every node selects the same stripe as preferred
		if( m_partnerList->getNumOutgoingConnections(max) < m_partnerList->getNumOutgoingConnections(i) && (intrand(2) % 2 == 0) )
			max = i;
	}
	preferredStripe = max;
	return max;
}

double MultitreeBase::getGainThreshold(void)
{
	float t = 0.2;
	float b = getConnections() / ((bwCapacity + 1) * numStripes);

	//EV << "deg: " << getConnections() << " max: " << ((bwCapacity + 1) * numStripes) << " b: " << b << endl;

	if(b == 1)
		return INT_MIN;

	return (1 - pow(b, pow(2 * t, 3)) * (1 - pow(t, 3))) + pow(t, 3);
}

void MultitreeBase::dropNode(int stripe, IPvXAddress address, IPvXAddress alternativeParent)
{
	// TODO make stripe a vector
	TreeDisconnectRequestPacket *pkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");

	EV << "DROP " << address << " to " << alternativeParent << endl;

	DisconnectRequest request;
	request.stripe = stripe;
	request.alternativeParent = alternativeParent;

	pkt->getRequests().push_back(request);

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
            rate = 0.9 * (check_and_cast<cDatarateChannel *>(currentGate->getChannel())->getDatarate());
        }
    }

	double capacity = (rate / m_appSetting->getVideoStreamBitRate());
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

int MultitreeBase::getMaxOutConnections()
{
	return numStripes * (bwCapacity);
}
