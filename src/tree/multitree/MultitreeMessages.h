#ifndef MULTITREEMESSAGE
#define MULTITREEMESSAGE true

struct ConnectRequest
{
	int stripe;
	int numSuccessors;
	long lastReceivedChunk;
	IPvXAddress currentParent;
	std::set<IPvXAddress> lastRequests;
};

struct ConnectConfirm
{
	int stripe;
	IPvXAddress alternativeParent;	
};

struct DisconnectRequest
{
	int stripe;
	IPvXAddress alternativeParent;
};

struct PassNodeRequest
{
	int stripe;
	int remainingBW;
	float threshold;
	float dependencyFactor;
};

struct SuccessorInfo
{
	int stripe;
	int numSuccessors;
};

#endif
