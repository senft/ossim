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
	bool allIdle = true;
	for (int i = 0; i < numStripes; i++)
	{
		if(m_state[i] != TREE_JOIN_STATE_IDLE)
		{
			allIdle = false;
			break;
		}
	}
	if(allIdle)
		return;

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

			if(m_state[stripe] == TREE_JOIN_STATE_LEAVING || m_state[stripe] == TREE_JOIN_STATE_IDLE)
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

void MultitreeBase::getCheapestChild(successorList childList, int stripe, IPvXAddress &address, IPvXAddress skipAddress)
{
    IPvXAddress curMaxAddress;
	double curMaxGain = INT_MIN;

	for(std::map<IPvXAddress, int>::iterator it = childList.begin() ; it != childList.end(); ++it)
	{
		IPvXAddress curAddress = it->first;

		if(curAddress.equals(skipAddress))
			continue;

		double curGain = getGain(childList, stripe, curAddress, IPvXAddress());

		//EV << "checking: " << curAddress << ", gain: " << curGain << endl;

		if(curMaxGain < curGain)
		{
			curMaxGain = curGain;
			curMaxAddress = curAddress;
		}
	}
	address = curMaxAddress;
}

double MultitreeBase::getGain(successorList childList, int stripe, IPvXAddress child, IPvXAddress childToDrop)
{
	//EV << "********* GAIN WITH DROPCHILD ***********" << endl;
	//EV << "Stripe: " << stripe << " Child: " << child << endl;
	//EV << "K3: " << getBalanceCosts(childList, stripe, child, childToDrop) << endl;
	//EV << "K2: " << getForwardingCosts(childList, stripe, child) << endl;
	//EV << "Total: " << getBalanceCosts(childList, stripe, child, childToDrop) -
	//	getForwardingCosts(childList, stripe, child) << endl;
	//EV << "****************************************" << endl;

	// K_3 - K_2
	return getBalanceCosts(childList, stripe, child, childToDrop) - getForwardingCosts(childList, stripe, child);
}

void MultitreeBase::getCostliestChild(successorList childList, int stripe, IPvXAddress &address)
{
    IPvXAddress curMaxAddress;
	double curMaxCosts = INT_MIN;

	for(std::map<IPvXAddress, int>::iterator it = childList.begin() ; it != childList.end(); ++it)
	{
		IPvXAddress curAddress = it->first;
		double curCosts = getCosts(childList, stripe, curAddress);

		//EV << "checking: " << curAddress << ", costs: " << curCosts << endl;

		if(curMaxCosts < curCosts)
		{
			curMaxCosts = curCosts;
			curMaxAddress = curAddress;
		}
	}
	address = curMaxAddress;
}

double MultitreeBase::getCosts(successorList childList, int stripe, IPvXAddress child)
{
	//EV << "****************************************" << endl;
	//EV << child << endl;
	//EV << "K1: " << getStripeDensityCosts(childList, stripe);
	//EV << " K2: " << getForwardingCosts(childList, stripe, child);
	//EV << " K3: " << getBalanceCosts(childList, stripe, child, IPvXAddress());
	//EV << " K4: " << getDepencyCosts(child);
	//EV << "Total: " << getStripeDensityCosts(childList, stripe)
	//	+ 2 * getForwardingCosts(childList, stripe, child)
	//	+ 3 * getBalanceCosts(childList, stripe, child, IPvXAddress())
	//	+ 4 * getDepencyCosts(child) << endl;
	//EV << "****************************************" << endl;

	// K_1 + 2 * K_2 + 3 * K_3 + 4 * K_4
	return getStripeDensityCosts(childList, stripe)
		+ 2 * getForwardingCosts(childList, stripe, child)
		+ 3 * getBalanceCosts(childList, stripe, child, IPvXAddress())
		+ 4 * getDepencyCosts(child);
}

double MultitreeBase::getStripeDensityCosts(successorList childList, int stripe) // K_sel ,K_1
{
	int fanout = childList.size();
	float outCapacity = bwCapacity;
	//EV << "K1: " << fanout << " " << outCapacity << endl;
	return 1 - (fanout / outCapacity);
}

int MultitreeBase::getForwardingCosts(successorList childList, int stripe, IPvXAddress child) // K_forw, K_2
{
    return (childList[child] == 0);
}

double MultitreeBase::getBalanceCosts(successorList childList, int stripe, IPvXAddress child, IPvXAddress childToDrop) // K_bal, K_3
{
    double myChildren = childList.size();
	double mySuccessors = myChildren;
	for (successorList::iterator it = childList.begin() ; it != childList.end(); ++it)
	{
		mySuccessors += it->second;
	}

    int childsSuccessors = childList[child];

	//EV << "fanout: " << myChildren << " mySucc: " << mySuccessors << " childsSucc: " << childsSuccessors << endl; 

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
	// This does not need an up-to-date successorList, because this only gets called during an
	// optimization process. During that process only one stripe changes, the others stay the same.
	// And we can be sure, that this node is conencted with <child> else this would not get called.

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

	EV << "Sending DisconnectRequests to " << address << " (stripe: " << stripe << "), alternativeParent=" << alternativeParent << endl;

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
	EV << getNodeAddress() << endl;
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
