#include <omnetpp.h>

#include <algorithm>

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

bool MultitreePartnerList::hasChildren(void)
{
	for (int i = 0; i < numStripes; i++)
		if(children[i].size() > 0)
			return true;

	return false;
}

bool MultitreePartnerList::hasChild(int stripe, IPvXAddress address)
{
	if(stripe == -1)
		return hasChild(address);

	std::map<IPvXAddress, int> currentChildren = children[stripe];
	std::map<IPvXAddress, int>::const_iterator it = currentChildren.find(address);
	return it != currentChildren.end();
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

void MultitreePartnerList::addChild(int stripe, IPvXAddress address, int successors)
{
	children[stripe].insert(
			std::pair<IPvXAddress, int>(address, successors));
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
	// TODO: maybe a check would be better?!
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
	int sum = 0, i;
	for (i = 0; i < numStripes; i++)
	{
        sum = sum + getNumChildren(i);
	}
	return sum;
}

int MultitreePartnerList::getNumChildren(int stripe)
{
	return children[stripe].size();
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
    return children[stripe][address];
}

void MultitreePartnerList::updateNumChildsSuccessors(int stripe, IPvXAddress address, int numSuccessors)
{
	children[stripe][address] =  numSuccessors;
}

IPvXAddress MultitreePartnerList::getRandomNodeFor(int stripe, IPvXAddress forNode)
{
	std::set<IPvXAddress> candidates;
	if(!parents[stripe].isUnspecified() && !parents[stripe].equals(forNode))
		candidates.insert(parents[stripe]);

	for (int i = 0; i < numStripes; ++i)
	{
		std::map<IPvXAddress, int> curChildren = children[i];
		for (std::map<IPvXAddress, int>::iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
		{
			// Add all children that have at least 1 successor in this stripe
			// to the list of potential candidates
			if(it->second > 0)
				candidates.insert(it->first);
		}
	}

	// Add some more if there are too little children
	if(candidates.size() < 3)
	{
		for (int i = 0; i < numStripes; ++i)
		{
			std::map<IPvXAddress, int> curChildren = children[i];
			for (std::map<IPvXAddress, int>::iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
			{
				candidates.insert(it->first);
			}
		}
	}

	if(candidates.size() == 0)
	{
		return IPvXAddress();
	}
	else if(candidates.size() == 1)
	{
		if(candidates.begin()->equals(forNode))
			return IPvXAddress();
		else
			return (IPvXAddress)*candidates.begin();
	}
	else
	{
		std::set<IPvXAddress>::const_iterator it(candidates.begin());
		advance(it, intrand(candidates.size() - 1));
		IPvXAddress child = (IPvXAddress)*it;

		if(child.equals(forNode))
		{
			if(it == candidates.end())
				it--;
			else
				it++;
			child = (IPvXAddress)*it;
		}
		return child;
	}
}

void MultitreePartnerList::printPartnerList(void)
{
	EV << "*********** Parents **********" << endl;
	for (int i = 0; i < numStripes; i++)
	{
		EV << "Stripe " << i << ": " << parents[i] << endl;
	}

	EV << "********** Children **********" << endl;
	for (int i = 0; i < numStripes; i++)
	{
		EV << "Stripe " << i << ": ";

		std::map<IPvXAddress, int> curChildren = children[i];

		for (std::map<IPvXAddress, int>::iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
		{
			EV << it->first.str() << " (" << it->second << " successors), ";
		}
		EV << endl;
	}

	EV << "******************************" << endl;
}
