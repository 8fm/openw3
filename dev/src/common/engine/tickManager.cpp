/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "tickManager.h"
#include "../core/memoryMacros.h"
#include "fxState.h"
#include "immediateJobsCollector.h"
#include "propertyAnimationSet.h"
#include "component.h"
#include "../core/taskBatch.h"
#include "../core/gameSave.h"
#include "baseEngine.h"

STickManagerContext::STickManagerContext( CTaskBatch& recycledTaskSync, ETickGroup group, Float timeDelta, Bool asyncTick )
	: m_group( group )
	, m_timeDelta( timeDelta )
	, m_taskSync( recycledTaskSync )
	, m_shouldWait( false )
	, m_asyncTick( asyncTick )
{

}

STickManagerContext::~STickManagerContext()
{
	m_taskSync.FinishTasksAndWait();
}

void STickManagerContext::Issue( ETaskSchedulePriority pri )
{
	if ( m_asyncTick && m_shouldWait )
	{
		m_taskSync.Issue( pri );
	}
}

void STickManagerContext::Wait()
{
	if ( m_asyncTick && m_shouldWait )
	{
		m_taskSync.GetSyncToken().Wait();
	}
}

// CTickManager

CTickManager::CTickManager( CWorld* world )
	:	m_world( world ),
		m_taskSync( 256 )
{
	const SGameplayLODConfig& lodConfig = GGame->GetGameplayConfig().m_LOD;

	m_effects.SetMaxBudgetedProcessingTime( lodConfig.m_effectsTickTime );
}

CTickManager::~CTickManager()
{
}

Bool CTickManager::ValidateMatchingWorld( CNode* node )
{
	if ( CLayer* layer = node->GetLayer() )
	{
		if ( CWorld* world = layer->GetWorld() )
		{
			return world == m_world;
		}
	}
	return true; // Node might not be attached in which case don't fail the test
}

void CTickManager::SetReferencePosition( const Vector& position )
{
	m_entityTickManager.SetReferencePosition( position );
}

void CTickManager::BeginFrame()
{
	m_statEffects.Reset();
	m_timerManager.ResetStats();
	m_componentTickManager.ResetStats();
	m_entityTickManager.ResetStats();
}

void CTickManager::ProcessPendingCommands()
{
	ASSERT( SIsMainThread() );

	if ( !m_pendingCommands.Empty() )
	{
		PC_SCOPE_PIX( ProcessPendingTickCommands );

		// We can safely iterate over the command list
		for ( Uint32 i=0; i<m_pendingCommands.Size(); ++i )
		{
			TickCommand* command = &m_pendingCommands[i];

			// Process command type
			if ( command->m_action == ACTION_Add )
			{
				if ( CComponent* component = command->m_component.Get() )
				{
					AddToGroup( component, command->m_group );
				}
				else if ( CEntity* entity = command->m_entity.Get() )
				{
					m_entityTickManager.Add( entity );
				}
				else if ( command->m_fx )
				{
					m_effects.Add( command->m_fx, command->m_budgeted );
				}
			}
			else if ( command->m_action == ACTION_Remove )
			{
				if ( CComponent* component = command->m_component.Get() )
				{
					if ( command->m_group == TICK_Max )
					{
						Remove( component );
					}
					else
					{
						RemoveFromGroup( component, command->m_group );
					}
				}
				else if ( CEntity* entity = command->m_entity.Get() )
				{
					m_entityTickManager.Remove( entity );
				}
				else if ( command->m_fx )
				{
					m_effects.Remove( command->m_fx, command->m_budgeted );
				}
			}
		}

		// Reset command list
		m_pendingCommands.ClearFast();
	}
}

void CTickManager::AddToGroupDelayed( CComponent* component, ETickGroup group )
{
	new ( m_pendingCommands ) TickCommand( ACTION_Add, group, component );
}

void CTickManager::RemoveFromGroupDelayed( CComponent* component, ETickGroup group )
{
	new( m_pendingCommands ) TickCommand( ACTION_Remove, group, component );
}

void CTickManager::AddEntity( CEntity* entity )
{
	ASSERT( ValidateMatchingWorld( entity ) ); 
	m_entityTickManager.Add( entity );
}

void CTickManager::RemoveEntity( CEntity* entity )
{
	ASSERT( ValidateMatchingWorld( entity ) ); 
	m_entityTickManager.Remove( entity );
}

// Add actor so it will be on tick list
void CTickManager::AddEntityDelayed( CEntity* entity )
{
	new ( m_pendingCommands ) TickCommand( ACTION_Add, entity );
}

// Remove actor form tick list
void CTickManager::RemoveEntityDelayed( CEntity* entity )
{
	new ( m_pendingCommands ) TickCommand( ACTION_Remove, entity );
}

void CTickManager::AddPropertyAnimationSet( CPropertyAnimationSet* set )
{
	m_propertyAnimationSets.Insert( set );
}

void CTickManager::RemovePropertyAnimationSet( CPropertyAnimationSet* set )
{
	m_propertyAnimationSets.Erase( set );
}

void CTickManager::AddEffect( CFXState* effect, Bool budgeted )
{
	ASSERT( !effect->GetEntity() || ValidateMatchingWorld( effect->GetEntity() ) ); 
	m_effects.Add( effect, budgeted );
}

void CTickManager::RemoveEffect( CFXState* effect, Bool budgeted )
{
	ASSERT( !effect->GetEntity() || ValidateMatchingWorld( effect->GetEntity() ) ); 
	m_effects.Remove( effect, budgeted );
}

void CTickManager::SetBudgeted( CFXState* effect, Bool budgeted )
{
	ASSERT( !effect->GetEntity() || ValidateMatchingWorld( effect->GetEntity() ) ); 
	m_effects.ChangeBudgeted( effect, budgeted );
}

void CTickManager::Tick( STickManagerContext& context )
{
	const ETickGroup group = context.m_group;
	const Float timeDelta = context.m_timeDelta;

	// most of the engine code does not support time delta == 0.0f
	if ( timeDelta > 0.0f )
	{
		// Tick the timers fired before component tick
		const Bool preTimers = ( group == TICK_PrePhysics ) || ( group == TICK_Main );
		if ( preTimers )
		{
			m_timerManager.FireTimers( group );
		}

		// Immediate jobs
		{
			PC_SCOPE_PIX( TickManagerTick_CollectImmediateJobs );

			// Collect jobs
			m_componentTickManager.CollectImmediateJobs( context, m_taskSync );
		}

		if ( group != TICK_PrePhysics )
		{
			PC_SCOPE_PIX( ImmediateJobs );
			m_taskSync.IssueAndWait( TSP_VeryHigh );
			context.Issue( TSP_VeryHigh );
		}

		// Update components
		m_componentTickManager.TickComponentsSingleThreaded( timeDelta, group );

		// Tick the timers fired after component tick
		if ( !preTimers )
		{
			m_timerManager.FireTimers( group );
		}

		// Tick entities on main tick
		if ( group == TICK_Main )
		{
			m_entityTickManager.Tick( timeDelta );
		}

		// This will be flushed anyways so it's better to do here for the profiling
		if ( group == TICK_PrePhysics )
		{
			PC_SCOPE_PIX( ImmediateJobs );
			m_taskSync.IssueAndWait( TSP_VeryHigh );
			context.Issue( TSP_VeryHigh );
		}
	}
}

#ifdef USE_ANSEL
void CTickManager::TickNPCLODs()
{
	m_entityTickManager.TickNPCLODs();
}
#endif // USE_ANSEL

void CTickManager::TickImmediateJobs( STickManagerContext& context, CTaskBatch& taskSync )
{
	const ETickGroup group = context.m_group;
	const Float timeDelta = context.m_timeDelta;

	// most of the engine code does not support time delta == 0.0f
	if ( timeDelta <= 0.0f ) return;
	// Tick the timers fired before component tick
	const Bool preTimers = ( group == TICK_PrePhysics ) || ( group == TICK_Main );
	if ( preTimers )
	{
		m_timerManager.FireTimers( group );
	}

	// Immediate jobs
	{
		PC_SCOPE_PIX( TickManagerTickImmediateJobs_CollectImmediateJobs );

		// Collect jobs
		m_componentTickManager.CollectImmediateJobs( context, taskSync );
	}
}

void CTickManager::TickSingleThreaded( STickManagerContext& context, CTaskBatch& taskSync )
{
	const ETickGroup group = context.m_group;
	const Float timeDelta = context.m_timeDelta;

	// most of the engine code does not support time delta == 0.0f
	if ( timeDelta <= 0.0f ) return;

	// Update components
	m_componentTickManager.TickComponentsSingleThreaded( timeDelta, group );

	// Tick the timers fired after component tick
	const Bool preTimers = ( group == TICK_PrePhysics ) || ( group == TICK_Main );
	if ( !preTimers )
	{
		m_timerManager.FireTimers( group );
	}

	// Tick entities on main tick
	if ( group == TICK_Main )
	{
		m_entityTickManager.Tick( timeDelta );
	}

	// This will be flushed anyways so it's better to do here for the profiling
	if ( group == TICK_PrePhysics )
	{
		PC_SCOPE_PIX( ImmediateJobs );
		taskSync.IssueAndWait( TSP_VeryHigh );
		context.Issue( TSP_VeryHigh );
	}
}

void CTickManager::OnSaveGameplayState( IGameSaver* saver )
{
	m_timerManager.OnSaveGameplayState( saver );
}

void CTickManager::OnLoadGameplayState( IGameLoader* loader )
{
	m_timerManager.OnLoadGameplayState( loader );
}

void CTickManager::AdvanceTime( Float timeDelta )
{
	m_timerManager.AdvanceTime( timeDelta );
}

void CTickManager::TickEffects( Float timeDelta )
{
	// Get start time
	Uint64 timeStart = 0;
	Red::System::Clock::GetInstance().GetTimer().GetTicks( timeStart );

	// Update effect list
	ProcessPendingCommands();

	// Tick effects and delete dead ones

	struct FXStateTicker
	{
		CEffectManager& m_effectManager;

		FXStateTicker( CEffectManager& effectManager )
			: m_effectManager( effectManager )
		{}

		RED_FORCE_INLINE void Tick( CFXState* effect, Float deltaTime )
		{
			effect->Tick( deltaTime );
		}
	} effectTicker( m_world->GetEffectManager() );

	m_effects.Tick( effectTicker, timeDelta );
	
	ProcessPendingCommands();

	// Get end time
	Uint64 timeEnd = 0;
	Red::System::Clock::GetInstance().GetTimer().GetTicks( timeEnd );

	// Update stats
	m_statEffects.m_statsCount += m_effects.GetNumRecentlyProcessedBudgeted();
	m_statEffects.m_statsTime += ( timeEnd - timeStart );
}


void CTickManager::TickAllEffects( Float timeDelta )
{
	// Get start time
	Uint64 timeStart = 0;
	Red::System::Clock::GetInstance().GetTimer().GetTicks( timeStart );

	// Update effect list
	ProcessPendingCommands();

	// Tick effects and delete dead ones

	struct FXStateTicker
	{
		CEffectManager& m_effectManager;

		FXStateTicker( CEffectManager& effectManager )
			: m_effectManager( effectManager )
		{}

		RED_FORCE_INLINE void Tick( CFXState* effect, Float deltaTime )
		{
			effect->Tick( deltaTime );
		}
	} effectTicker( m_world->GetEffectManager() );

	m_effects.TickAll( effectTicker, timeDelta );

	ProcessPendingCommands();

	// Get end time
	Uint64 timeEnd = 0;
	Red::System::Clock::GetInstance().GetTimer().GetTicks( timeEnd );

	// Update stats
	m_statEffects.m_statsCount += m_effects.GetNumRecentlyProcessedBudgeted();
	m_statEffects.m_statsTime += ( timeEnd - timeStart );
}


void CTickManager::TickPropertyAnimations( Float deltaTime )
{
	// Make a copy (animation sets can be removed during iteration)

	static TDynArray< CPropertyAnimationSet* > copy;

	copy.ClearFast();
	copy.Reserve( m_propertyAnimationSets.Size() );
	for ( CPropertyAnimationSet* set : m_propertyAnimationSets )
	{
		copy.PushBack( set );
	}

	// Tick animation sets

	for ( CPropertyAnimationSet* set : copy )
	{
		set->Update( deltaTime );
	}
}
