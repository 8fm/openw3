#pragma once

#include "../../common/game/swarmUpdateJob.h"

class CHumbleCrittersUpdateJob : public CSwarmUpdateJob
{
	typedef CSwarmUpdateJob Super;
protected:
	void UpdateSwarm() override;
public:
	CHumbleCrittersUpdateJob( volatile CSwarmLairEntity* entity, const TSwarmStatesList currentState, TSwarmStatesList targetState, volatile CSwarmAlgorithmData* data )
		: CSwarmUpdateJob( entity, currentState, targetState, data ) {}
};