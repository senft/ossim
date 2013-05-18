#include <omnetpp.h>

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

bool MultitreePartnerList::hasChild(int stripe, IPvXAddress address)
{
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

void MultitreePartnerList::updateNumSuccessor(int stripe, IPvXAddress address, int numSuccessors)
{
	children[stripe][address] =  numSuccessors;
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
