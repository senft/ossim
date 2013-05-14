#include <omnetpp.h>

#include "MultitreePartnerList.h"

Define_Module(MultitreePartnerList)

MultitreePartnerList::MultitreePartnerList(){}
MultitreePartnerList::~MultitreePartnerList(){}

void MultitreePartnerList::initialize(int stage)
{
    if (stage == 3)
    {
		numStripes = 5; // TODO: make it a real parameter

		parents = new IPvXAddress[numStripes];

		int i;
		for (i = 0; i < numStripes; i++)
		{
			std::vector<ChildInfo> v;
			children.push_back(v);
		}
    }
}

void MultitreePartnerList::finish()
{

}

void MultitreePartnerList::handleMessage(cMessage *)
{
    throw cException("PartnerList does not process messages!");
}

void MultitreePartnerList::addChild(int stripe, ChildInfo child){
	children[stripe].push_back(child);
}

void MultitreePartnerList::addChild(ChildInfo child){
	int i;
	for (i = 0; i < numStripes; i++)
	{
		addChild(i, child);
	}
}

std::vector<ChildInfo> MultitreePartnerList::getChildren(int stripe)
{
	return children.at(stripe);
}

void MultitreePartnerList::addParent(int stripe, IPvXAddress address){
	parents[stripe] = address;
}

void MultitreePartnerList::addParent(IPvXAddress address){
	int i;
	for (i = 0; i < numStripes; i++)
	{
		addParent(i, address);
	}
}

IPvXAddress MultitreePartnerList::getParent(int stripe)
{
	return parents[stripe];
}
