#include "AppSettingMultitree.h"

Define_Module(AppSettingMultitree)

AppSettingMultitree::AppSettingMultitree(){}
AppSettingMultitree::~AppSettingMultitree(){}
void AppSettingMultitree::finish(){}

void AppSettingMultitree::handleMessage(cMessage* msg)
{
    throw cException("AppSettingMultitree does not process messages!");
}

void AppSettingMultitree::initialize()
{
	param_numStripes = par("numStripes");
	param_stripeSize = par("stripeSize").longValue();
}
