#include "limits.h"
#include "MultitreeBase.h"
#include "DpControlInfo_m.h"

#include <algorithm> 

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

		lastSeqNumber = new long[numStripes];
		for (int i = 0; i < numStripes; i++)
		{
			lastSeqNumber[i] = -1L;
		}

		//bwCapacity = getBWCapacity();
		bwCapacity = par("bwCapacity");
		param_delayOptimization = par("delayOptimization");
		param_optimize = par("optimize");

		numCR = 0;
		numDR = 0;
		numCC = 0;
		numPNR = 0;
		numSI = 0;
		WATCH(numCR);
		WATCH(numDR);
		WATCH(numCC);
		WATCH(numPNR);
		WATCH(numSI);
	}
}

void MultitreeBase::finish(void)
{
	cancelAndDeleteTimer();

	m_partnerList->clear();
	delete[] m_state;
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
	std::random_shuffle(requests.begin(), requests.end(), intrand);

	std::vector<ConnectRequest> accept;
	std::vector<ConnectRequest> reject;

	bool doOptimize = false;

	bool onlyPreferredStripes = true;
	// 2 runs: 1 for the preferred stripes, 1 for the remaining
	
	printStatus();
	for (int i = 0; i < 2; i++)
	{
		for(std::vector<ConnectRequest>::const_iterator it = requests.begin(); it != requests.end(); ++it)
		{
			const ConnectRequest request = (ConnectRequest)*it;
			int stripe = request.stripe;

			
			if( (onlyPreferredStripes && !isPreferredStripe(stripe))
					|| (!onlyPreferredStripes && isPreferredStripe(stripe)) )
				continue;

			int numSucc = request.numSuccessors;

			//EV << "Processing stripe: " << stripe << endl;

			// TODO check if node is a child
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
			else if( m_partnerList->hasChild(stripe, senderAddress) )
			{
				EV << "Received ConnectRequest from child (" << senderAddress << ", stripe " << stripe
					<< "). Tell him he can stay..." << endl;
				accept.push_back(request);

				disconnectingChildren[stripe].erase(disconnectingChildren[stripe].find(senderAddress));

			}
			//else if( requestedChildship[stripe].find(senderAddress) != requestedChildship[stripe].end() )
			else if( !requestedChildship.empty() && senderAddress.equals(requestedChildship[stripe].back()) )
			{
				// TODO: maybe skip this even when the node is in requestedChildship (not just last
				// element)
				// TODO: would be better to just queue this.. maybe the node rejects me
				EV << "Received ConnectRequest from a node (" << senderAddress << ") that I requested childship from for stripe "
					<< stripe << ". Rejecting..." << endl;
				reject.push_back(request);
			}
			else if(hasBWLeft(accept.size() + 1))
			{
				if(preferredStripe == -1)
					preferredStripe = stripe;

				if(isPreferredStripe(stripe))
				{
					if(m_partnerList->getParent(stripe).isUnspecified())
					{

						// source
						bool moreSuccInOther = true;
						bool lessChildrenInOther = false;

						int myNumChildren = m_partnerList->getNumOutgoingConnections(stripe);
						int myNumSucc = m_partnerList->getNumSuccessors(stripe);

						for (int i = 0; i < numStripes; i++)
						{
							if(i == stripe)
								continue;

							if(myNumSucc > m_partnerList->getNumSuccessors(i))
							{
								//EV << "Succ: " << myNumSucc << " < " << m_partnerList->getNumSuccessors(i) << endl;
								moreSuccInOther = false;
							}

							if(myNumChildren > m_partnerList->getNumOutgoingConnections(i))
							{
								//EV << "Children: " << myNumChildren << " > " << m_partnerList->getNumOutgoingConnections(i) << endl;
								lessChildrenInOther = true;
							}
						}

						//if( (moreSuccInOther && moreChildrenInOther) || (moreSuccInOther ^ moreChildrenInOther))
						if( (lessChildrenInOther))
						{
							if(moreSuccInOther)
							{
								accept.push_back(request);
								m_partnerList->addChild(stripe, senderAddress, numSucc);
							}
							else
							{
								EV << "Received ConnectRequest from " << senderAddress << " (stripe " << stripe
									<< ") but already have most sucessors/children in that stripe. Rejecting..." << endl;
								reject.push_back(request);
							}
						}
						else
						{
							accept.push_back(request);
							m_partnerList->addChild(stripe, senderAddress, numSucc);
						}

					}
					else
					{
						// peer
						accept.push_back(request);
						m_partnerList->addChild(stripe, senderAddress, numSucc);

						//if(!isPreferredStripe(stripe))
						//	doOptimize  = true;
					}
				}
				else
				{
					if(m_partnerList->getNumActiveTrees() <= 1)
					{
						accept.push_back(request);
						m_partnerList->addChild(stripe, senderAddress, numSucc);
					}
					else
					{
						EV << "Received ConnectRequest from " << senderAddress << " (stripe " << stripe
							<< ") but not my preferred stripe. Rejecting..." << endl;
						reject.push_back(request);
					}
				}
			}
			else
			{
				//if(onlyPreferredStripes && stripe == getPreferredStripe() 
				//		&& !m_partnerList->hasChild(stripe, senderAddress)
				//		)
				//{
				//	// A node wants to connect to my preferred stripe, but there is no no spare
				//	// bandwidth. First try to drop a node in an "un-preferred" to a node
				//	// in the same stripe (hence this only works when there are at least 2 children
				//	// in an unpreferred stripe). Then try to drop a node in the preferred stripe to
				//	// another node in the same stripe.

				//	printStatus();

				//	bool droppedNode = false;
				//	if(request.currentParent.isUnspecified()) // when it is the nodes initial connect
				//	{

				//		if(!droppedNode)
				//		{
				//			if( m_partnerList->getNumOutgoingConnections(stripe) > 1 ) // At least 2 children...
				//			{
				//				std::set<IPvXAddress> skipNodes;
				//				for(std::set<IPvXAddress>::iterator it = disconnectingChildren[stripe].begin(); it != disconnectingChildren[stripe].end(); ++it)
				//				{
				//					skipNodes.insert((IPvXAddress)*it);
				//				}

				//				IPvXAddress drop = m_partnerList->getChildWithLeastChildren( stripe, skipNodes );
				//				skipNodes.insert(drop);
				//				//IPvXAddress alternativeParent = m_partnerList->getChildWithMostChildren(stripe, skipNodes );
				//				IPvXAddress alternativeParent = senderAddress;

				//				if( !drop.isUnspecified() && ! alternativeParent.isUnspecified()
				//						&& !alternativeParent.equals(m_partnerList->getParent(stripe)))
				//				{
				//					EV << "Dropping " << drop << " to make room for " << senderAddress << endl;

				//					droppedNode = true;
				//					dropNode(stripe, drop, alternativeParent);
				//					accept.push_back(request);
				//				}
				//			}
				//		}

				//	}
				//	//else
				//	//{

				//	//	if(!droppedNode)
				//	//	{
				//	//		for (int j = 0; j < numStripes; j++)
				//	//		{
				//	//			if(j == stripe || droppedNode == true)
				//	//				continue;

				//	//			//if( m_partnerList->getNumOutgoingConnections(j) > 1 ) // At least 2 children...
				//	//			if( m_partnerList->hasChildren(j) )
				//	//			{

				//	//				std::vector<IPvXAddress> vSkipNodes;
				//	//				std::set<IPvXAddress> skipNodes;
				//	//				for(std::set<IPvXAddress>::iterator it = disconnectingChildren[j].begin(); it != disconnectingChildren[j].end(); ++it)
				//	//				{
				//	//					skipNodes.insert((IPvXAddress)*it);
				//	//					vSkipNodes.push_back((IPvXAddress)*it);
				//	//				}

				//	//				IPvXAddress drop = m_partnerList->getChildWithLeastChildren( j, skipNodes );
				//	//				skipNodes.insert(drop);
				//	//				vSkipNodes.push_back(drop);
				//	//				//IPvXAddress alternativeParent = m_partnerList->getChildWithMostChildren(j, skipNodes);
				//	//				IPvXAddress alternativeParent = getAlternativeNode(j, drop, IPvXAddress(), vSkipNodes);

				//	//				if( !drop.isUnspecified() && !alternativeParent.isUnspecified()
				//	//						&& m_partnerList->getNumChildsSuccessors(j, drop) == 0
				//	//						//&& !alternativeParent.equals(m_partnerList->getParent(j))
				//	//					)
				//	//				{
				//	//					EV << "Dropping " << drop << " to make room for " << senderAddress << endl;

				//	//					droppedNode = true;
				//	//					dropNode(j, drop, alternativeParent);
				//	//					accept.push_back(request);
				//	//				}
				//	//			}
				//	//		}
				//	//	}

				//	//}
				//		
				//	if(!droppedNode)
				//	{
				//		EV << "Received ConnectRequest from " << senderAddress << " (stripe " << stripe
				//			<< ") but have no bandwidth left. Rejecting..." << endl;
				//		reject.push_back(request);
				//		doOptimize  = true;
				//	}

				//}
				//else
				//{
				  // This is no preferred stripe -> just reject
				  //EV << "Received ConnectRequest from " << senderAddress << " (stripe " << stripe
				  //	<< ") but have no bandwidth left. Rejecting..." << endl;

				  EV << "Received ConnectRequest from " << senderAddress << " (stripe " << stripe
				  	<< ") but have no bandwidth left. Rejecting..." << endl;

				  reject.push_back(request);
				  //doOptimize  = true;
				//}
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
		std::vector<IPvXAddress> lastRequests = cRequest.lastRequests;

		IPvXAddress alternativeParent = getAlternativeNode(stripe, address, currentParent, lastRequests);

		DisconnectRequest dRequest;
		dRequest.stripe = stripe;
		dRequest.alternativeParent = alternativeParent;

		EV << "Alternative parent for " << address << " (stripe " << stripe << ") is " << alternativeParent << endl;

		pkt->getRequests().push_back(dRequest);
	}

	numDR++;
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
		IPvXAddress currentParent = request.currentParent;
		std::vector<IPvXAddress> lastRequests = request.lastRequests;
		IPvXAddress alternativeParent = getAlternativeNode(stripe, address, currentParent, lastRequests);

		ConnectConfirm confirm;
		confirm.stripe = stripe;
		confirm.alternativeParent = alternativeParent;

		pkt->getConfirms().push_back(confirm);

		EV << stripe << ", ";
	}
	EV << " of node " << address << endl;

	numCC++;
    sendToDispatcher(pkt, m_localPort, address, m_destPort);

	for (int i = 0; i < numReqStripes; i++)
	{
		ConnectRequest request = requests[i];

		int stripe = request.stripe;

		if(param_sendMissingChunks)
		{
			int lastChunk = request.lastReceivedChunk;
			sendChunksToNewChild(stripe, address, lastChunk);
		}

		scheduleSuccessorInfo(stripe);
	}

	//printStatus();
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

	std::set<IPvXAddress> &curDisconnectingChildren = disconnectingChildren[stripe];
	if(curDisconnectingChildren.find(address) != curDisconnectingChildren.end())
		curDisconnectingChildren.erase(curDisconnectingChildren.find(address));
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
	double outCapacity = (bwCapacity * numStripes);
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
	{
		myChildren--;
		childsSuccessors++;
	}

	if(myChildren == 0) // TODO is this ok?
		return 0;

    double x = (mySuccessors / myChildren) - 1.0;

	if(x == 0) // TODO is this ok?
		return 0;

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
		std::set<IPvXAddress> curDisconnectingChildren = disconnectingChildren[i];
       	std::vector<IPvXAddress> curChildren = m_partnerList->getChildren(i);
       	for(std::vector<IPvXAddress>::iterator it = curChildren.begin(); it != curChildren.end(); ++it)
       	{
			IPvXAddress child = (IPvXAddress)*it;
			if ( child.equals(child) && curDisconnectingChildren.find(child) == curDisconnectingChildren.end() )
			{
				numConnections++;
				break;
			}
		}
	}

    return (double)numConnections / (double)numStripes;
}

double MultitreeBase::getGainThreshold(void)
{
	double t = 0.2;
	//double b = getConnections() / ((bwCapacity + 1) * numStripes);

    int outDegree = 0;
    for (int i = 0; i < numStripes; i++)
    {
		int out = m_partnerList->getNumOutgoingConnections(i);
		int disconnecting = disconnectingChildren[i].size();

		if(out < disconnecting)
		{
			throw cException("Stripe %d: More disconnecting children (%d) than normal children (%d).",
					i, disconnecting, out );
		}

		outDegree += out;
		outDegree -= disconnecting;
    }
    double b = (double)outDegree / (double)(bwCapacity * numStripes);

	//EV << "deg: " << outDegree << " max: " << (bwCapacity * numStripes) << " b: " << b << endl;
	//EV << "t: " <<  (1 - pow(b, pow(2 * t, 3)) * (1 - pow(t, 3))) + pow(t, 3) << endl;

	if(b == 1)
		return INT_MIN;

	return (1 - pow(b, pow(2 * t, 3)) * (1 - pow(t, 3))) + pow(t, 3);
}

void MultitreeBase::dropNode(int stripe, IPvXAddress address, IPvXAddress alternativeParent)
{
	if(disconnectingChildren[stripe].find(address) != disconnectingChildren[stripe].end())
		return;


	// TODO make stripe a vector
	TreeDisconnectRequestPacket *pkt = new TreeDisconnectRequestPacket("TREE_DISCONNECT_REQUEST");

	EV << "Sending DisconnectRequests to " << address << " (stripe: " << stripe 
		<< "), alternativeParent=" << alternativeParent << endl;

	DisconnectRequest request;
	request.stripe = stripe;
	request.alternativeParent = alternativeParent;

	pkt->getRequests().push_back(request);

	if(!m_partnerList->hasParent(stripe, address))
		// If that node is no parent, mark it as disconnecting
		disconnectingChildren[stripe].insert(address);

	numDR++;
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
	EV << "*******************************" << endl;
	EV << getNodeAddress() << endl;
	m_partnerList->printPartnerList();
	EV << "*******************************" << endl;
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

