#ifndef CHILDINFO_H_
#define CHILDINFO_H_ true

#include "IPvXAddress.h"

class MultitreeChildInfo
{
public:
	MultitreeChildInfo();
	virtual ~MultitreeChildInfo();

	inline void setAddress(IPvXAddress newAddress){ address = newAddress; }
	inline IPvXAddress getAddress(){ return address; }

	inline void setPort(int newPort){ port = newPort; }
	inline int getPort(){ return port; }

	inline void setNumSuccessors(int successors){ numSuccessors = successors; }
	inline int getNumSuccessors(){ return numSuccessors; }

	inline void setForwardingStripes(int* newStripes){ stripes = newStripes; }
	inline int* getForwardingStripes(){ return stripes; }

protected:
	IPvXAddress address;
	int port;
	int numSuccessors;
	int* stripes;

private:
};

#endif
