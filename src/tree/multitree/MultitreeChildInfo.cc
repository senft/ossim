#include "MultitreeChildInfo.h"

MultitreeChildInfo::MultitreeChildInfo()
{
    cModule *temp = simulation.getModuleByPath("appSetting");
    m_appSetting = check_and_cast<AppSettingMultitree *>(temp);

	numStripes = m_appSetting->getNumStripes();

	numSuccessors = new int[numStripes];
}

MultitreeChildInfo::~MultitreeChildInfo(){}

void MultitreeChildInfo::setNumSuccessors(int stripe, int num)
{
	numSuccessors[stripe] = num;
}

int MultitreeChildInfo::getNumSuccessors(int stripe)
{
	return numSuccessors[stripe];
}

int MultitreeChildInfo::getNumSuccessors()
{
	int sum = 0, i;
	for (i = 0; i < numStripes; i++)
	{
		sum = sum + getNumSuccessors(i);
	}
	return sum;
}
