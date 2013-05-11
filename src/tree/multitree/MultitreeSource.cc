#include "MultitreeSource.h"

Define_Module(MultitreeSource)

MultitreeSource::MultitreeSource(){}
MultitreeSource::~MultitreeSource(){}

void MultitreeSource::initialize(int stage){
	MultitreeBase::initialize(stage);

	m_state = TREE_JOIN_STATE_ACTIVE;

    bindToGlobalModule();
    bindToTreeModule();
    bindToStatisticModule();
}

void MultitreeSource::finish(void){}

void MultitreeSource::handleTimerMessage(cMessage *msg){}

void MultitreeSource::bindToGlobalModule(void)
{
	MultitreeBase::bindToGlobalModule();

    // -- Churn
    cModule *temp = simulation.getModuleByPath("churnModerator");
    m_churn = check_and_cast<IChurnGenerator *>(temp);
    EV << "Binding to churnModerator is completed successfully" << endl;
}

void MultitreeSource::bindToTreeModule(void)
{
	MultitreeBase::bindToTreeModule();
}

void MultitreeSource::cancelAndDeleteTimer(void)
{
}
