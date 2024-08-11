/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "../../common/game/gameWorld.h"
#include "crowdManager.h"
#include "../../common/core/parallelFor.h"
#include "../../common/engine/pathlibWorld.h"

void CCrowdManager::UpdateZPositions()
{
	// this is temporary
	// target solution is: update positions by LOD
	// so the LOD0 gets all the Z in every frame
	// each frame few agents of LOD1 also gets it updated
	// and LOD2 doesn't get those updates at all

	if ( GetNumAgents() < 1 )
	{
		return;
	}

	class CPositionProcessor
	{		
	public:
		const CPathLibWorld* m_pathLibWorld;

		void Compute( const SCrowdAgent& agent, Float& output )
		{
			Vector3 v( agent.m_pos.X, agent.m_pos.Y, output );
			m_pathLibWorld->ComputeHeight( v, output );
		}		
	};

	CPositionProcessor processor;
	processor.m_pathLibWorld = GCommonGame->GetActiveWorld()->GetPathLibWorld();
	ASSERT( processor.m_pathLibWorld );

	auto params = CParallelForTaskDoubleArray< SCrowdAgent, Float, CPositionProcessor >::SParams::Create();
	{
		params->m_inputArray		= m_agents;
		params->m_outputArray		= m_zPositions;
		params->m_numElements		= GetNumAgents();
		params->m_processFunc		= &CPositionProcessor::Compute;
		params->m_processFuncOwner	= &processor;
		params->SetDebugName		( TXT("CrowdZPos") );
	}

	{
		CROWD_PROFILE_SCOPE( crowdPositionComputer )
		params->ProcessNow();
		params->Release();
	}
}

