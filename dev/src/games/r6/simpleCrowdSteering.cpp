/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "simpleCrowdSteering.h"
#include "crowdAgent.h"
#include "crowdManager.h"
#include "crowdArea.h"
#include "crowdEntryPoint.h"
#include "../../common/core/parallelFor.h"


void CSimpleCrowdSteering::UpdateLOD1( SCrowdAgent* input, Int16 amountOfAgents, CCrowdManager* mgr, const  Float timeDelta  )
{
	m_solver.SetCrowdManager( mgr );
	m_solver.SetCrowdSpaceImpl( &mgr->GetCrowdSpace() );
	m_solver.SetDeltaTime( timeDelta );	

	/*for( TAgentIndex i =0; i<amountOfAgents; ++i )
	{
		m_solver.UpdateAgent( input[i], output[i] );
	}*/
	
	auto params = CParallelForTaskSingleArray< SCrowdAgent, CSolverImpl >::SParams::Create();
	{
		params->m_array				= input;
		params->m_numElements		= amountOfAgents;
		params->m_processFunc		= &CSolverImpl::UpdateAgent;
		params->m_processFuncOwner	= &m_solver;
		params->SetDebugName		( TXT("CrowdSteering") );
	}

	params->ProcessNow();
	params->Release();
}
