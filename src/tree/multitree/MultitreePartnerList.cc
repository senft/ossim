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
			children.push_back(std::map<IPvXAddress, int>());
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

	children.clear();
	for (int i = 0; i < numStripes; i++)
	{
		children.push_back(std::map<IPvXAddress, int>());
	}
}

bool MultitreePartnerList::hasChildren(int stripe)
{
	return children[stripe].size() > 0;
}

bool MultitreePartnerList::hasChildren(void)
{
	for (int i = 0; i < numStripes; i++)
		if(hasChildren(i))
			return true;

	return false;
}

bool MultitreePartnerList::hasChild(int stripe, IPvXAddress address)
{
	std::map<IPvXAddress, int> currentChildren = children[stripe];
	std::map<IPvXAddress, int>::const_iterator it = currentChildren.find(address);
	return it != currentChildren.end();
}

void MultitreePartnerList::addChild(int stripe, IPvXAddress address, int successors)
{
	children[stripe].insert( std::pair<IPvXAddress, int>(address, successors) );
}

IPvXAddress MultitreePartnerList::getChildWithLeastChildren(int stripe, const std::set<IPvXAddress> &skipNodes)
{
	IPvXAddress child;
	const std::map<IPvXAddress, int> curChildren = children[stripe];

	if(curChildren.size() > 0)
	{
		int minSucc = INT_MAX;

		for (std::map<IPvXAddress, int>::const_iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
		{
			bool skipNode = skipNodes.find(it->first) != skipNodes.end();
			if(!skipNode)
			{
				if(it->second < minSucc)
				{
					minSucc = it->second;
					child = it->first;
				}
			}
			//else
			//{
			//	EV << "skip: " << it->first << endl;
			//}
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
	const std::map<IPvXAddress, int> curChildren = children[stripe];

	if(curChildren.size() > 0)
	{
		int minSucc = INT_MAX;

		for (std::map<IPvXAddress, int>::const_iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
		{
			bool skipNode = skipNodes.find(it->first) != skipNodes.end();
			if(!skipNode)
			{
				if(it->second > 0 && it->second < minSucc)
				{
					minSucc = it->second;
					child = it->first;
				}
			}
		}
	}
	return child;
}

IPvXAddress MultitreePartnerList::getRandomChild(int stripe, const std::set<IPvXAddress> &skipNodes)
{
	IPvXAddress child;
	const std::map<IPvXAddress, int> curChildren = children[stripe];

	if(curChildren.size() > 0)
	{
		bool allInSkipNodes = true;
		for (std::map<IPvXAddress, int>::const_iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
		{
			if(skipNodes.find(it->first) == skipNodes.end())
			{
				allInSkipNodes = false;
				return it->first;
			}
		}

		if(!allInSkipNodes)
		{
			do {
				std::map<IPvXAddress, int>::const_iterator it = curChildren.begin();
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
	std::map<IPvXAddress, int> curChildren = children[stripe];

	for (std::map<IPvXAddress, int>::iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
	{
		bool skipNode = skipNodes.find(it->first) != skipNodes.end();
		if(!skipNode)
		{
			if(it->second > maxSucc)
			{
				maxSucc = it->second;
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
	child = getLaziestForwardingChild(stripe, skipNodes);

	if(child.isUnspecified())
		child = getChildWithLeastChildren(stripe, skipNodes);

	//if(child.isUnspecified())
	//	child = getRandomChild(stripe, skipNodes);

	return child;
}

std::set<IPvXAddress> MultitreePartnerList::getChildren()
{
	std::set<IPvXAddress> distinctChildren;

	for (int i = 0; i < numStripes; ++i)
	{
		std::map<IPvXAddress, int> curChildren = children[i];
		for (std::map<IPvXAddress, int>::iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
		{
			distinctChildren.insert(it->first);
		}
	}
	return distinctChildren;

}

std::vector<IPvXAddress> MultitreePartnerList::getChildren(int stripe)
{
	std::vector<IPvXAddress> result;
	std::map<IPvXAddress, int> curChildren = children[stripe];

	for (std::map<IPvXAddress, int>::iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
	{
		result.push_back(it->first);
	}

	return result;
}

/** 
 * Return all children (incl. number of successors) of the given stripe
*/
std::map<IPvXAddress, int> MultitreePartnerList::getChildrenWithCount(int stripe)
{
	return children[stripe];
}

void MultitreePartnerList::removeChild(int stripe, IPvXAddress address)
{
	children[stripe].erase(address);
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

int MultitreePartnerList::getNumChildren(void)
{
	return getChildren().size();
}

int MultitreePartnerList::getNumOutgoingConnections(void)
{
	int sum = 0, i;
	for (i = 0; i < numStripes; i++)
	{
        sum = sum + getNumOutgoingConnections(i);
	}
	return sum;
}

int MultitreePartnerList::getNumOutgoingConnections(int stripe)
{
	return children[stripe].size();
}

int MultitreePartnerList::getNumSuccessors()
{
	int sum = 0;
	std::set<IPvXAddress> distinctChildren = getChildren();
	for (std::set<IPvXAddress>::iterator child = distinctChildren.begin() ; child != distinctChildren.end(); ++child)
	{
		int max = 0;

		for (int i = 0; i < numStripes; i++)
		{

			std::map<IPvXAddress, int> curChildren = children[i];
			for (std::map<IPvXAddress, int>::iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
			{
				if(it->first.equals((IPvXAddress)*child) && max < it->second)
					max = it->second;
			}

		}
		sum = sum + max;

	}
	sum = sum + getNumChildren();
	return sum;
}

int MultitreePartnerList::getNumSuccessors(int stripe)
{
	int sum = 0;
	std::map<IPvXAddress, int> curChildren = children[stripe];
	for (std::map<IPvXAddress, int>::iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
	{
		sum = sum + 1 + it->second;
	}
	return sum;
}

int MultitreePartnerList::getNumChildsSuccessors(int stripe, IPvXAddress address)
{
	if(hasChild(stripe, address))
		return children[stripe][address];
	else
		return -1;
}

void MultitreePartnerList::updateNumChildsSuccessors(int stripe, IPvXAddress address, int numSuccessors)
{
	if(hasChild(stripe, address))
		children[stripe][address] =  numSuccessors;
}

void MultitreePartnerList::printPartnerList(void)
{
	for (int i = 0; i < numStripes; i++)
	{
		EV << "*********** Stripe " << i << " **********" << endl;
		EV << "Parent  : " << parents[i] << " (" << getNumOutgoingConnections(i) << " children, " << getNumSuccessors(i) << " successors)" << endl;
		EV << "Children: ";

		std::map<IPvXAddress, int> curChildren = children[i];
		for (std::map<IPvXAddress, int>::iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
		{
			EV << it->first.str() << " (" << it->second << " successors), ";
		}
		EV << endl;
	}
}
