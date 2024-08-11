/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "build.h"

#include "crowdAgent.h"
#include "crowdSpaceImplementation.h"

class CSimpleSolver
{
private:
	const CCrowdSpaceImpl* m_crowsSpace;
	Float m_deltaTime;

public:
	FORCE_INLINE void SetCrowdManager( CCrowdManager* mgr ) {}
	FORCE_INLINE void SetCrowdSpaceImpl( const CCrowdSpaceImpl* crowsSpace ){ m_crowsSpace = crowsSpace; }
	FORCE_INLINE void SetDeltaTime( Float deltaTime ){ m_deltaTime = deltaTime; }

	void UpdateAgent( const SCrowdAgent& input, SCrowdAgent& output );
};