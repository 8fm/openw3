#pragma once

#include "../../common/game/swarmUpdateJob.h"

class CFlyingCrittersUpdateJob : public CSwarmUpdateJob
{
	typedef CSwarmUpdateJob Super;
protected:
	void UpdateSwarm() override;
public:
	CFlyingCrittersUpdateJob( volatile CSwarmLairEntity* entity, const TSwarmStatesList currentState, TSwarmStatesList targetState, volatile CSwarmAlgorithmData* data )
		: CSwarmUpdateJob( entity, currentState, targetState, data ) {}
};