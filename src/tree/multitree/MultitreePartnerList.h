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

	int getNumChildren(void);
	int getNumChildren(int stripe);

	int getNumSuccessors(int stripe);
    int getNumChildsSuccessors(int stripe, IPvXAddress address);

    void updateNumChildsSuccessors(int stripe, IPvXAddress address, int numSuccessors);

	bool hasParent(IPvXAddress address);
	bool hasParent(int stripe, IPvXAddress address);
	void addParent(IPvXAddress address);
	void addParent(int stripe, IPvXAddress address);
	std::vector<int> removeParent(IPvXAddress address);
	void removeParent(int stripe);
	IPvXAddress getParent(int stripe);

	bool hasChildren(void);
	bool hasChild(IPvXAddress address);
	bool hasChild(int stripe, IPvXAddress address);
	void addChild(int stripe, IPvXAddress address, int successors);
	void removeChild(IPvXAddress address);
	void removeChild(int stripe, IPvXAddress address);
	std::vector<IPvXAddress> getChildren(int stripe);

	IPvXAddress getRandomNodeFor(int stripe, IPvXAddress forNode);

	void printPartnerList(void);
protected:
	int numStripes;

	IPvXAddress* parents;
	std::vector<std::map<IPvXAddress, int> > children;

private:

};

#endif
