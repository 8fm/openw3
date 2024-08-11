#pragma once

#include "swarmLairEntity.h"

///////////////////////////////////////////////////////////////
// Async update job
class CSwarmUpdateJob : public CTask
{
protected:
	volatile CSwarmLairEntity*					m_lair;
	const TSwarmStatesList						m_currentState;
	TSwarmStatesList							m_targetState;
	CSwarmAlgorithmData*						m_swarmAlgorithmData;
	Bool										m_boidListUpdated;

	virtual void		UpdateSwarm() = 0;
public:
	CSwarmUpdateJob( volatile CSwarmLairEntity* entity, const TSwarmStatesList currentState, TSwarmStatesList targetState, volatile CSwarmAlgorithmData* data );

	void				Run() override;
#ifndef NO_DEBUG_PAGES
	const Char*			GetDebugName() const override;
	Uint32				GetDebugColor() const override;
#endif
};