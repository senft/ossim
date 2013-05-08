#include "MultitreeSource.h"

Define_Module(MultitreeSource)

MultitreeSource::MultitreeSource(){}
MultitreeSource::~MultitreeSource(){}

void MultitreeSource::initialize(int stage){
	MultitreeBase::initialize(stage);

	m_state = TREE_JOIN_STATE_ACTIVE;

    bindToGlobalModule();
    bindToStatisticModule();
}

void MultitreeSource::finish(void){}

void MultitreeSource::handleTimerMessage(cMessage *msg){}
