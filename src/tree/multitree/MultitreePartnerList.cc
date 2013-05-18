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
			std::vector<MultitreeChildInfo> v;
			children.push_back(v);
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
	// TODO: implement
}

bool MultitreePartnerList::hasChild(int stripe, IPvXAddress address)
{
	std::vector<MultitreeChildInfo> currentChildren = children[stripe];
	for(std::vector<MultitreeChildInfo>::iterator it = currentChildren.begin(); it != currentChildren.end(); ++it) {
		if( ((MultitreeChildInfo)*it).getAddress().equals(address) )
			return true;
	}
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

void MultitreePartnerList::addChild(int stripe, MultitreeChildInfo child){
	// TODO check if child is not already added
	children[stripe].push_back(child);
}

void MultitreePartnerList::addChild(MultitreeChildInfo child){
	for (int i = 0; i < numStripes; i++)
	{
		addChild(i, child);
	}
}

std::vector<MultitreeChildInfo> MultitreePartnerList::getChildren(int stripe)
{
	return children.at(stripe);
}

void MultitreePartnerList::removeChild(int stripe, IPvXAddress address)
{
	std::vector<MultitreeChildInfo>::iterator it = children[stripe].begin();
	while (it != children[stripe].end()) {
		if ( address.equals(it->getAddress()) ) {
			it = children[stripe].erase(it);
		}
		else {
			++it;
		}
	}
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

//std::set<IPvXAddress> MultitreePartnerList::getAllConnections()
//{
//	std::set<IPvXAddress> result;
//
//	for (int i = 0; i < numStripes; i++)
//	{
//		result.insert(parents[i]);
//
//		std::vector<MultitreeChildInfo> curChildren = children[i];
//		for(std::vector<MultitreeChildInfo>::iterator it = curChildren.begin(); it != curChildren.end(); ++it) {
//			result.insert( ((MultitreeChildInfo)*it).getAddress() );
//		}
//	}
//
//	return result;
//}

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
	std::vector<MultitreeChildInfo> curChildren = children[stripe];

	for (std::vector<MultitreeChildInfo>::iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
	{
		MultitreeChildInfo current = (MultitreeChildInfo)*it;
		sum = sum + 1 + current.getNumSuccessors(stripe);
	}
	return sum;
}

void MultitreePartnerList::updateNumSuccessor(int stripe, IPvXAddress address, int numSuccessors)
{
	std::vector<MultitreeChildInfo> curChildren = children[stripe];

	//EV << "UPDATING PartnerList for stripe: " << stripe << " LIST " << this << endl;

	for (std::vector<MultitreeChildInfo>::iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
	{
		MultitreeChildInfo current = (MultitreeChildInfo)*it;
		if(current.getAddress().equals(address))
		{
			EV << &current << " FOUND PEER: " << address << ". Update NUMSUC to " << numSuccessors << endl;
			current.setNumSuccessors(stripe, numSuccessors);
		}
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

		std::vector<MultitreeChildInfo> curChildren = children[i];

		for (std::vector<MultitreeChildInfo>::iterator it = curChildren.begin() ; it != curChildren.end(); ++it)
		{
		    MultitreeChildInfo current = (MultitreeChildInfo)*it;
			EV << &current << " " << current.getAddress().str() << " (" << current.getNumSuccessors(i) << " successors), ";
		}
		EV << endl;
	}

	EV << "******************************" << endl;
}
