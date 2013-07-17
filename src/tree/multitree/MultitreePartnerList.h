#ifndef MULTITREEPARTNERLIST_H_
#define MULTITREEPARTNERLIST_H_ true

#include <IPvXAddress.h>
#include <vector>

class MultitreePartnerList : public cSimpleModule
{
public:
	MultitreePartnerList();
	virtual ~MultitreePartnerList();

	virtual int numInitStages() const { return 4; }
	virtual void initialize(int stage);
	virtual void finish();
	virtual void handleMessage(cMessage *);

	void clear(void);

	int getNumOutgoingConnections(void);
	int getNumOutgoingConnections(int stripe);

	bool nodeForwardingInOtherStripe(int stripe, IPvXAddress node);
	bool nodeHasMoreChildrenInOtherStripe(int stripe, IPvXAddress node);

	int getNumChildren(void);
	int getNumSuccessors(int stripe);
    int getNumChildsSuccessors(int stripe, IPvXAddress address);

    bool updateNumChildsSuccessors(int stripe, IPvXAddress address, int numSuccessors);

	bool hasParent(IPvXAddress address);
	bool hasParent(int stripe, IPvXAddress address);
	void addParent(IPvXAddress address);
	void addParent(int stripe, IPvXAddress address);
	std::vector<int> removeParent(IPvXAddress address);
	void removeParent(int stripe);
	IPvXAddress getParent(int stripe);

	bool hasChildren(int stripe);
	bool hasChildren(void);
	bool hasChild(IPvXAddress address);
	bool hasChild(int stripe, IPvXAddress address);
	void addChild(int stripe, IPvXAddress address, int successors);
	void removeChild(IPvXAddress address);
	void removeChild(int stripe, IPvXAddress address);
	std::set<IPvXAddress> &getChildren(int stripe);
	std::map<IPvXAddress, std::vector<int> > getChildrenWithCount(int stripe);

	IPvXAddress getLaziestForwardingChild(int stripe, const std::set<IPvXAddress> &skipNodes);
	IPvXAddress getChildWithMostChildren(int stripe, const std::set<IPvXAddress> &skipNodes);
	IPvXAddress getChildWithLeastChildren(int stripe, const std::set<IPvXAddress> &skipNodes);
	IPvXAddress getRandomChild(int stripe, const std::set<IPvXAddress> &skipNodes);
	IPvXAddress getBestLazyChild(int stripe, const std::set<IPvXAddress> &skipNodes);

	int getNumActiveTrees(void);
	int getNumActiveTrees(IPvXAddress child);

	void printPartnerList(void);
protected:

private:
	bool numOutgoingChanged;
	int outgoingConnections;

	int numStripes;

	IPvXAddress* parents;
	std::vector<std::map<IPvXAddress, std::vector<int> > > mChildren;
	std::vector<std::set<IPvXAddress> > vChildren;


};

#endif
