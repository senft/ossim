#include "MultitreeSource.h"

Define_Module(MultitreeSource)

MultitreeSource::MultitreeSource(){}
MultitreeSource::~MultitreeSource()
{
	finish();
}

void MultitreeSource::initialize(int stage){
	MultitreeBase::initialize(stage);

	m_state = TREE_JOIN_STATE_ACTIVE;

    bindToGlobalModule();
    bindToTreeModule();
    bindToStatisticModule();

	// -------------------------------------------------------------------------
    // -------------------------------- Timers ---------------------------------
    // -------------------------------------------------------------------------
    // -- One-time timers


    // -- Repeated timers

}

void MultitreeSource::finish(void)
{
	cancelAndDeleteTimer();
}

void MultitreeSource::handleTimerMessage(cMessage *msg)
{
}

void MultitreeSource::bindToGlobalModule(void)
{
	MultitreeBase::bindToGlobalModule();
}

void MultitreeSource::bindToTreeModule(void)
{
	MultitreeBase::bindToTreeModule();
}

void MultitreeSource::cancelAndDeleteTimer(void)
{
}

void MultitreeSource::cancelAllTimer()
{
}
