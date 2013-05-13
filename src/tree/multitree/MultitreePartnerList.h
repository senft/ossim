#ifndef MULTITREEPARTNERLIST_H_
#define MULTITREEPARTNERLIST_H_ true

#include <vector>
#include "IPvXAddress.h"

class MultitreePartnerList : public cSimpleModule
{
public:
	MultitreePartnerList();
	virtual ~MultitreePartnerList();

	virtual int numInitStages() const { return 4; }
	virtual void initialize(int stage);
	virtual void finish();

	virtual void handleMessage(cMessage *);

	//bool hasParent(IPvXAddress address);
	//bool hasParent(int stripe, IPvXAddress address);
	void addParent(IPvXAddress address);
	void addParent(int stripe, IPvXAddress address);
	void removeParent(IPvXAddress address);
	void removeParent(int stripe, IPvXAddress address);
	IPvXAddress getParent(int stripe);

	//bool hasChild(IPvXAddress address);
	//bool hasChild(int stripe, IPvXAddress address);
	void addChild(IPvXAddress address);
	void addChild(int stripe, IPvXAddress address);
	void removeChild(IPvXAddress address);
	void removeChild(int stripe, IPvXAddress address);
	std::vector<IPvXAddress> getChildren(int stripe);

protected:
	int numStripes;

	IPvXAddress* parents;
	std::vector<std::vector<IPvXAddress> > children;
};

#endif
