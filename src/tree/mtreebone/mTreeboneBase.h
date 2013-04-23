#ifndef MTREEBONE_BASE_H_
#define MTREEBONE_BASE_H_ true

#include "CommBase.h"

#include "IPv4InterfaceData.h"
#include "InterfaceTableAccess.h"
#include "MembershipBase.h"
#include "AppCommon.h"
#include "Contact.h"
#include "MessageLogger.h"
#include "Dispatcher.h"
#include "GossipMembershipPacket_m.h"
#include "ScampStatistic.h"


class mTreeboneBase : public CommBase
{
public:
	mTreeboneBase();
	virtual ~mTreeboneBase();

private:
	/* data */
};

#endif
