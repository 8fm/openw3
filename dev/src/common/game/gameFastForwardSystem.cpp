/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gameFastForwardSystem.h"

#include "../engine/worldIterators.h"

#include "communitySystem.h"
#include "encounter.h"
#include "entityPool.h"
#include "gameWorld.h"

IMPLEMENT_ENGINE_CLASS( CGameFastForwardSystem )

const String CGameFastForwardSystem::BLACKSCREEN_REASON( TXT("FastForward") );

///////////////////////////////////////////////////////////////////////////////
// SFastForwardExecutionParameters
///////////////////////////////////////////////////////////////////////////////
SFastForwardExecutionParameters::SFastForwardExecutionParameters( const Vector& referencePos )
	: m_isExternallyTicked( false )
	, m_dontSpawnHostilesClose( true )
	, m_despawnExistingGuyz( true )
	, m_handleBlackScreen( false )
	, m_fastForwardSpawnTrees( true )
	, m_referencePosition( referencePos )
{

}
///////////////////////////////////////////////////////////////////////////////
// CGameFastForwardSystem
///////////////////////////////////////////////////////////////////////////////
CGameFastForwardSystem::CGameFastForwardSystem()
	: m_isOn( false )
	, m_isWaiting( false )
	, m_beginRequestIsPending( false )
{
	m_executionContext.m_isCompleted = true;
}
CGameFastForwardSystem::~CGameFastForwardSystem()
{

}
void CGameFastForwardSystem::ProcessPendingBeginRequest()
{
	if ( m_beginRequestIsPending )
	{
		m_beginRequestIsPending = false;
		BeginFastForward( Move( m_requestedExecutionParameters ) );
	}
}
void CGameFastForwardSystem::TickInternal( Float timeDelta )
{
	if ( m_isWaiting )
	{
		m_waitingTimer -= timeDelta;
		if ( m_waitingTimer > 0.f )
		{
			return;
		}
		m_isWaiting = false;
	}
	m_executionContext.m_isCompleted = true;

	if ( m_executionContext.m_parameters.m_fastForwardSpawnTrees && !m_simulatedEncounters.Empty() )
	{
		Float encounterCompletenessPossible = 1.f / Float( m_simulatedEncounters.Size() );
		for ( Int32 i = m_simulatedEncounters.Size() - 1; i >= 0; --i )
		{
			CEncounter* enc = m_simulatedEncounters[ i ].m_encounter.Get();
			if ( !enc )
			{
				m_simulatedEncounters.RemoveAtFast( i );
				continue;
			}

			CEncounter::FastForwardUpdateContext& context = m_simulatedEncounters[ i ].m_context;
			context.PreUpdate();

			enc->FastForwardTick( timeDelta, context );

			if ( context.m_isLoadingTemplates || context.m_runningSpawnJobsCount > 0 )
			{
				m_executionContext.m_isCompleted = false;
			}
		}
	}

	CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	communitySystem->GetAgentsWorld().ForceFullLodUpdate();
	communitySystem->Tick( timeDelta );
	if ( !communitySystem->IsSpawningStabilized() )
	{
		m_executionContext.m_isCompleted = false;
	}
	
	if ( m_executionContext.m_allowSelfCompletion && m_executionContext.m_isCompleted )
	{
		EndFastForward();
	}
}

Bool CGameFastForwardSystem::ExternalTick( Float timeDelta )
{
	ProcessPendingBeginRequest();

	ASSERT( m_isOn ); 
	TickInternal( timeDelta );

	return !m_executionContext.m_isCompleted;
}

void CGameFastForwardSystem::BeginFastForward( SFastForwardExecutionParameters&& parameters )
{
	m_isOn = true;
	m_isWaiting = false;
	m_waitingTimer = 0.f;

	m_executionContext.m_parameters = Move( parameters );

	m_executionContext.m_allowSelfCompletion = false;
	m_executionContext.m_isShutdownRequested = false;
	m_executionContext.m_isCompleted = false;
	m_executionContext.m_referenceTransform.SetIdentity();
	m_executionContext.m_referenceTransform.SetTranslation( m_executionContext.m_parameters.m_referencePosition );

	// turn on entity pool greedy mode
	if ( CEntityPool* entityPool = GCommonGame->GetEntityPool() )
	{
		entityPool->StartFastForward();
	}
	
	if ( m_executionContext.m_parameters.m_fastForwardSpawnTrees )
	{
		// Find every encounter in the game
		CWorld* world = GGame->GetActiveWorld();
		WorldAttachedEntitiesIterator it( world );
		// collect fast forward simulated encounters
		while ( it )
		{
			CEncounter* encounter = Cast< CEncounter >( *it );

			if ( encounter )
			{
				CEncounter::FastForwardUpdateContext context( &m_executionContext );
				if ( encounter->BeginFastForward( m_executionContext.m_parameters.m_referencePosition, context ) )
				{
					SimulatedEncounter s;
					s.m_encounter = encounter;
					s.m_context = context;
					m_simulatedEncounters.PushBack( s );
				}
			}

			++it;
		}
	}


	if ( m_executionContext.m_parameters.m_handleBlackScreen )
	{
		m_waitingTimer = 0.25f;
		m_isWaiting = true;
		GCommonGame->StartFade( false, BLACKSCREEN_REASON, 0.25f );
	}
}

void CGameFastForwardSystem::EndFastForward()
{
	m_beginRequestIsPending = false;

	if ( !m_isOn )
	{
		return;
	}

	m_isOn = false;

	if ( CEntityPool* entityPool = GCommonGame->GetEntityPool() )
	{
		entityPool->StopFastForward();
	}

	if ( m_executionContext.m_parameters.m_fastForwardSpawnTrees )
	{
		// Notify every encounter in the world that fast forward process is finished
		CWorld* world = GGame->GetActiveWorld();
		WorldAttachedEntitiesIterator it( world );
		while ( it )
		{
			CEncounter* encounter = Cast< CEncounter >( *it );

			if ( encounter )
			{
				encounter->EndFastForward();
			}

			++it;
		}

		m_simulatedEncounters.ClearFast();
	}

	if ( m_executionContext.m_parameters.m_handleBlackScreen )
	{
		GCommonGame->StartFade( true, BLACKSCREEN_REASON, 0.5f );
	}
}

Bool CGameFastForwardSystem::AllowFastForwardSelfCompletion()
{
	if ( !m_isOn )
	{
		return true;
	}

	m_executionContext.m_allowSelfCompletion = true;

	if ( m_executionContext.m_isCompleted )
	{
		EndFastForward();
		return true;
	}
	return false;
}

Bool CGameFastForwardSystem::RequestFastForwardShutdown()
{
	m_executionContext.m_isShutdownRequested = true;

	return AllowFastForwardSelfCompletion();
}

void CGameFastForwardSystem::CoverWithBlackscreen()
{
	if ( m_isOn && !m_executionContext.m_parameters.m_handleBlackScreen )
	{
		m_executionContext.m_parameters.m_handleBlackScreen = true;

		GCommonGame->SetBlackscreen( true, BLACKSCREEN_REASON );
	}
}

void CGameFastForwardSystem::OnWorldStart( const CGameInfo& gameInfo )
{

}
void CGameFastForwardSystem::OnWorldEnd( const CGameInfo& gameInfo )
{
	if ( m_isOn )
	{
		m_isOn = false;
		if ( m_executionContext.m_parameters.m_handleBlackScreen )
		{
			GCommonGame->SetBlackscreen( false, BLACKSCREEN_REASON );
		}
	}
}

void CGameFastForwardSystem::Tick( Float timeDelta )
{
	ProcessPendingBeginRequest();

	if ( !m_isOn || m_executionContext.m_parameters.m_isExternallyTicked )
	{
		return;
	}

	TickInternal( timeDelta );
}

void CGameFastForwardSystem::PausedTick()
{
	ProcessPendingBeginRequest();

	if ( !m_isOn || m_executionContext.m_parameters.m_isExternallyTicked )
	{
		return;
	}

	TickInternal( 0.f );
}

void CGameFastForwardSystem::OnGenerateDebugFragments( CRenderFrame* frame )
{
	
}

void CGameFastForwardSystem::funcBeginFastForward( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, dontSpawnHostilesClose, false );
	GET_PARAMETER_OPT( Bool, coverWithBlackscreen, false );
	FINISH_PARAMETERS;

	SFastForwardExecutionParameters p( GCommonGame->GetPlayer() ? GCommonGame->GetPlayer()->GetWorldPositionRef() : GCommonGame->GetActiveWorld()->GetCameraPosition() );
	p.m_isExternallyTicked = false;
	p.m_handleBlackScreen = coverWithBlackscreen;
	p.m_dontSpawnHostilesClose = dontSpawnHostilesClose;

	m_requestedExecutionParameters = Move( p );
	m_beginRequestIsPending = true;
}

void CGameFastForwardSystem::funcEndFastForward( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	EndFastForward();
}

void CGameFastForwardSystem::funcAllowFastForwardSelfCompletion( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	AllowFastForwardSelfCompletion();
}

void CGameFastForwardSystem::funcRequestFastForwardShutdown( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, coverWithBlackscreen, false );
	FINISH_PARAMETERS;

	if ( !RequestFastForwardShutdown() )
	{
		if ( coverWithBlackscreen )
		{
			CoverWithBlackscreen();
		}
	}
}
