#include <omnetpp.h>

#include <algorithm>
#include <limits>

#include "MultitreePartnerList.h"
#include "AppSettingMultitree.h"

Define_Module(MultitreePartnerList)

MultitreePartnerList::MultitreePartnerList(){}
MultitreePartnerList::~MultitreePartnerList(){}

void MultitreePartnerList::initialize(int stage)
{
    if (stage == 3)
    {
		cModule *temp = simulation.getModuleByPath("appSetting");
		AppSettingMultitree *m_appSetting = check_and_cast<AppSettingMultitree *>(temp);

		numStripes = m_appSetting->getNumStripes();

		parents = new IPvXAddress[numStripes];

		for (int i = 0; i < numStripes; i++)
		{
			mChildren.push_back(std::map<IPvXAddress, std::vector<int> >());
			vChildren.push_back(std::set<IPvXAddress>());
		}
    }
}

void MultitreePartnerList::finish()
{
	delete[] parents;
}

void MultitreePartnerList::handleMessage(cMessage *)
{
    throw cException("PartnerList does not process messages!");
}

void MultitreePartnerList::clear(void)
{
	delete[] parents;
	parents = new IPvXAddress[numStripes];

	mChildren.clear();
	vChildren.clear();
	for (int i = 0; i < numStripes; i++)
	{
		mChildren.push_back(std::map<IPvXAddress, std::vector<int> >());
		vChildren.push_back(std::set<IPvXAddress>());
	}
}

bool MultitreePartnerList::hasChildren(int stripe)
{
	return mChildren[stripe].size() > 0;
}

bool MultitreePartnerList::hasChildren(void)
{
	for (int i = 0; i < numStripes; i++)
		if(hasChildren(i))
			return true;

	return false;
}

bool MultitreePartnerList::hasChild(IPvXAddress address)
{
	for (int i = 0; i < numStripes; i++)
	{
		if(hasChild(i, address))
			return true;
	}
	return false;
}

bool MultitreePartnerList::hasChild(int stripe, IPvXAddress address)
{
	return vChildren[stripe].find(address) != vChildren[stripe].end();
}

void MultitreePartnerList::addChild(int stripe, IPvXAddress address, int successors)
{
	mChildren[stripe].insert( std::pair<IPvXAddress, std::vector<int> >(address, std::vector<int>()) );

	for (int i = 0; i < numStripes; i++)
	{
		mChildren[stripe][address].push_back(0);
	}

	mChildren[stripe][address][stripe] = successors;

	vChildren[stripe].insert(address);
	numOutgoingChanged = true;
}

IPvXAddress MultitreePartnerList::getChildWithLeastChildren(int stripe, const std::set<IPvXAddress> &skipNodes)
{
	IPvXAddress child;
	const std::map<IPvXAddress, std::vector<int> > curChildren = mChildren[stripe];
	int minSucc = INT_MAX;

	for (std::map<IPvXAddress, std::vector<int> >::const_iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
	{
		bool skipNode = skipNodes.find(it->first) != skipNodes.end();
		if(!skipNode)
		{
			int a = intrand(2);
			if(it->second[stripe] < minSucc || (it->second[stripe] == minSucc && a == 0))
			{
				//EV << it->second[stripe] << " vs. " << minSucc << " (" << a << ")" << endl;
				minSucc = it->second[stripe];
				child = it->first;
				//EV << "take: " << child << endl;
			}
		}
	}
	return child;
}

/**
 * Returns the child that has the least (but at least one) successor. It is also possible to specify
 * some nodes that should not be chosen here.
 */
IPvXAddress MultitreePartnerList::getLaziestForwardingChild(int stripe, const std::set<IPvXAddress> &skipNodes)
{
	IPvXAddress child;
	const std::map<IPvXAddress, std::vector<int> > curChildren = mChildren[stripe];
	int minSucc = INT_MAX;

	for (std::map<IPvXAddress, std::vector<int> >::const_iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
	{
		bool skipNode = skipNodes.find(it->first) != skipNodes.end();
		if(!skipNode)
		{
			if( //!nodeHasMoreChildrenInOtherStripe(stripe, it->first) && 
				((it->second[stripe] > 0 && it->second[stripe] < minSucc)
				|| (it->second[stripe] == minSucc && (int)intrand(2) == 0)))
			{
				minSucc = it->second[stripe];
				child = it->first;
			}
		}
	}
	return child;
}

IPvXAddress MultitreePartnerList::getRandomChild(int stripe, const std::set<IPvXAddress> &skipNodes)
{
	IPvXAddress child;
	const std::map<IPvXAddress, std::vector<int> > curChildren = mChildren[stripe];

	if(curChildren.size() > 0)
	{
		bool allInSkipNodes = true;
		for (std::map<IPvXAddress, std::vector<int> >::const_iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
		{
			if(skipNodes.find(it->first) == skipNodes.end())
			{
				allInSkipNodes = false;
				//return it->first;
			}
		}

		if(!allInSkipNodes)
		{
			do {
				std::map<IPvXAddress, std::vector<int> >::const_iterator it = curChildren.begin();
				std::advance(it, intrand(curChildren.size()));
				child = (IPvXAddress)it->first;
			} while(skipNodes.find(child) != skipNodes.end());
		}
	}
	return child;
}

IPvXAddress MultitreePartnerList::getChildWithMostChildren(int stripe, const std::set<IPvXAddress> &skipNodes)
{
	int maxSucc = -1;
	IPvXAddress busiestChild;
	std::map<IPvXAddress, std::vector<int> > curChildren = mChildren[stripe];

	for (std::map<IPvXAddress, std::vector<int> >::iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
	{
		bool skipNode = skipNodes.find(it->first) != skipNodes.end();
		if(!skipNode)
		{
			if(it->second[stripe] > maxSucc || (it->second[stripe] == maxSucc && intrand(2) == 0))
			//if(it->second > maxSucc)
			{
				maxSucc = it->second[stripe];
				busiestChild = it->first;
			}
		}
	}
	return busiestChild;
}

/*
 * Tries to find the "best" "lazy" child in 2 steps:
 *    1.) Find the node with least successors, that has at least 1 successor
 *    2.) If 1. didn't yield a result, find the child with least successors
 */
IPvXAddress MultitreePartnerList::getBestLazyChild(int stripe, const std::set<IPvXAddress> &skipNodes)
{
	IPvXAddress child = getLaziestForwardingChild(stripe, skipNodes);

	if(child.isUnspecified())
	{
		child = getChildWithLeastChildren(stripe, skipNodes);
	}

	return child;
}

std::set<IPvXAddress> &MultitreePartnerList::getChildren(int stripe)
{
	return vChildren[stripe];
}

/** 
 * Return all children (incl. number of successors) of the given stripe
*/
std::map<IPvXAddress, std::vector<int> > MultitreePartnerList::getChildrenWithCount(int stripe)
{
	return mChildren[stripe];
}

void MultitreePartnerList::removeChild(int stripe, IPvXAddress address)
{
	mChildren[stripe].erase(address);
	vChildren[stripe].erase(vChildren[stripe].find(address));
	numOutgoingChanged = true;
}

void MultitreePartnerList::removeChild(IPvXAddress address)
{
	for (int i = 0; i < numStripes; i++)
	{
		removeChild(i, address);
	}
}



bool MultitreePartnerList::hasParent(int stripe, IPvXAddress address)
{
	return parents[stripe].equals(address);
}

bool MultitreePartnerList::hasParent(IPvXAddress address)
{
	for (int i = 0; i < numStripes; i++)
	{
		if(hasParent(i, address))
			return true;
	}
	return false;
}

void MultitreePartnerList::addParent(int stripe, IPvXAddress address){
	parents[stripe] = address;
}

void MultitreePartnerList::addParent(IPvXAddress address){
	for (int i = 0; i < numStripes; i++)
	{
		addParent(i, address);
	}
}

IPvXAddress MultitreePartnerList::getParent(int stripe)
{
	return parents[stripe];
}

void MultitreePartnerList::removeParent(int stripe)
{
	IPvXAddress address;
	parents[stripe] = address;
}

std::vector<int> MultitreePartnerList::removeParent(IPvXAddress address)
{
	std::vector<int> affectedStripes;

	for (int i = 0; i < numStripes; i++)
	{
		if(address.equals(parents[i]))
		{
			IPvXAddress address;
			parents[i] = address;

			affectedStripes.push_back(i);
		}
	}
	return affectedStripes;
}

int MultitreePartnerList::getNumOutgoingConnections(void)
{
	if(numOutgoingChanged)
	{
		int sum = 0, i;
		for (i = 0; i < numStripes; i++)
		{
    	    sum = sum + getNumOutgoingConnections(i);
		}
		outgoingConnections = sum;
		numOutgoingChanged = false;
		return sum;
	}
	else
	{
		return outgoingConnections;
	}
}

int MultitreePartnerList::getNumOutgoingConnections(int stripe)
{
	return vChildren[stripe].size();
}

int MultitreePartnerList::getNumSuccessors(int stripe)
{
	int sum = 0;
	std::map<IPvXAddress, std::vector<int> > curChildren = mChildren[stripe];
	for (std::map<IPvXAddress, std::vector<int> >::iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
	{
		sum = sum + 1 + it->second[stripe];
	}
	return sum;
}

int MultitreePartnerList::getNumChildsSuccessors(int stripe, IPvXAddress address)
{
	for (int i = 0; i < numStripes; i++)
	{
		if(hasChild(i, address))
			return mChildren[i][address][stripe];
	}
	return -1;
}

bool MultitreePartnerList::updateNumChildsSuccessors(int stripe, IPvXAddress address, int numSuccessors)
{
	if(hasChild(address))
	{
		for (int i = 0; i < numStripes; i++)
		{
			if(hasChild(i, address))
			{
				mChildren[i][address][stripe] = numSuccessors;
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}

void MultitreePartnerList::printPartnerList(void)
{
	if(ev.isDisabled())
		return;

	for (int i = 0; i < numStripes; i++)
	{
		EV << "*********** Stripe " << i << " **********" << endl;
		EV << "Parent  : " << parents[i] << " (" << getNumOutgoingConnections(i) << " children, "
			<< getNumSuccessors(i) << " successors)" << endl;
		EV << "Children: ";

		std::map<IPvXAddress, std::vector<int> > curChildren = mChildren[i];
		for (std::map<IPvXAddress, std::vector<int> >::iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
		{
			EV << it->first.str() << " (" << it->second[i] << " successors [";

			for (int i = 0; i < numStripes; i++)
			{
				EV << it->second[i] << ",";
			}

			EV << "]), ";
		}
		EV << endl;
	}
}

int MultitreePartnerList::getNumActiveTrees(void)
{
	int numActiveTrees = 0;
	for (int i = 0; i < numStripes; i++)
	{
		if(hasChildren(i))
			numActiveTrees++;
	}
	return numActiveTrees;
}

bool MultitreePartnerList::nodeForwardingInOtherStripe(int stripe, IPvXAddress node)
{
	for (int i = 0; i < numStripes; i++)
	{
		if(i == stripe)
			continue;

		if(getNumChildsSuccessors(i, node) > 0)
			return true;
	}
	return false;
}

bool MultitreePartnerList::haveMoreChildrenInOtherStripe(int stripe)
{
	int numChildren = getNumOutgoingConnections(stripe);
	for (int i = 0; i < numStripes; i++)
	{
		if(i == stripe)
			continue;

		if(getNumOutgoingConnections(i) > numChildren)
			return true;
	}
	return false;
}

bool MultitreePartnerList::nodeHasMoreChildrenInOtherStripe(int stripe, IPvXAddress node)
{
	int numSucc = getNumChildsSuccessors(stripe, node);
	for (int i = 0; i < numStripes; i++)
	{
		if(i == stripe)
			continue;

		if(getNumChildsSuccessors(i, node) > numSucc)
			return true;
	}
	return false;
}

int MultitreePartnerList::getNumActiveTrees(IPvXAddress node)
{
	int activeTrees;
	for (int i = 0; i < numStripes; i++)
	{
		if(hasParent(i, node) || getNumChildsSuccessors(i, node) > 0)
		{
			activeTrees++;
		}
	}
	return activeTrees;
}
