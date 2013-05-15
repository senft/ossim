#ifndef CHILDINFO_H_
#define CHILDINFO_H_ true

#include "IPvXAddress.h"
#include "AppSettingMultitree.h"

class MultitreeChildInfo
{
public:
	MultitreeChildInfo();
	virtual ~MultitreeChildInfo();

	inline void setAddress(IPvXAddress newAddress){ address = newAddress; }
	inline IPvXAddress getAddress(){ return address; }

	inline void setPort(int newPort){ port = newPort; }
	inline int getPort(){ return port; }

	void setNumSuccessors(int stripe, int successors);
	int getNumSuccessors(int stripe);
	int getNumSuccessors();

	inline void setForwardingStripes(int* newStripes){ stripes = newStripes; }
	inline int* getForwardingStripes(){ return stripes; }

protected:
	AppSettingMultitree *m_appSetting;
	int numStripes;

	IPvXAddress address;
	int port;
	int* numSuccessors;
	int* stripes;

private:
};

#endif
