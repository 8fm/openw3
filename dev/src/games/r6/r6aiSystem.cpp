/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "r6aiSystem.h"
#include "aiAction.h"
#include "aiTreeComponent.h"
#include "../../common/engine/renderFrame.h"

IMPLEMENT_RTTI_ENUM( EAITickPriorityGroup )
IMPLEMENT_ENGINE_CLASS( CR6AISystem )

Uint32 CR6AISystem::TICK_GROUP_BUDGET[] = { Uint32( -1 ), 200, 100, 50 };

CR6AISystem::CR6AISystem()
{
}

void CR6AISystem::OnWorldStart( const CGameInfo& gameInfo )
{
	// reset tick indices
	for ( Uint32 i = 0; i < TICK_MAX; ++i )
	{
		m_tickIndices[ i ] = 0;
	}
}

void CR6AISystem::OnWorldEnd( const CGameInfo& gameInfo )
{
	// stop all running actions
	for ( Int32 i = m_runningActions.SizeInt() - 1; i >= 0; --i )
	{
		m_runningActions[ i ]->Stop( ACTION_Failed );
	}

	// free the memory
	for ( Uint32 i = 0; i < TICK_MAX; ++i )
	{
		for ( Uint32 k = 0; k < m_tickGroups[ i ].Size(); ++k )
		{
			m_tickGroups[ i ][ k ].m_component->OnWorldEnd();
		}
		m_tickGroups[ i ].Clear();
		m_tickIndices[ i ] = 0;
	}

	m_runningActions.Clear();
	m_tickableActions.Clear();
}

void CR6AISystem::Tick( Float timeDelta )
{
	for ( Uint32 i = 0; i < TICK_MAX; ++i )
	{
		// accumulate time
		for ( Uint32 k = 0; k < m_tickGroups[ i ].Size(); ++k )
		{
			m_tickGroups[ i ][ k ].m_timeAccum += timeDelta;
		}

		// update the components
		const Uint32 numComponentsToTick = min( m_tickGroups[ i ].Size(), TICK_GROUP_BUDGET[ i ] );
		Uint32 currentTickIndex = m_tickIndices[ i ];
		for ( Uint32 k = 0; k < numComponentsToTick; ++k, ++currentTickIndex )
		{
			currentTickIndex = currentTickIndex < m_tickGroups[ i ].Size() ? currentTickIndex : 0; // wrap around
			STickEntry& entry = m_tickGroups[ i ][ currentTickIndex ];

			entry.m_component->Update( entry.m_timeAccum, timeDelta );
			entry.m_timeAccum = 0.f;
		}
		m_tickIndices[ i ] = currentTickIndex;
	}

	// tick all actions
	for ( Uint32 i = 0; i < m_tickableActions.Size(); ++i )
	{
		CAIAction* action = m_tickableActions[ i ];
		R6_ASSERT( action->GetStatus() == ACTION_InProgress );

		const EAIActionStatus newStatus = action->Tick( timeDelta );
		if ( newStatus != ACTION_InProgress )
		{
			--i; // action stopped and removed itself from the list
		}
	}
}

void CR6AISystem::RegisterComponent( CAITreeComponent* component )
{
	EAITickPriorityGroup priorityGroup = component->GetCurrentPriorityGroup();
	R6_ASSERT( priorityGroup >= TICK_EveryFrame && priorityGroup < TICK_MAX );
	R6_ASSERT( m_tickGroups[ priorityGroup ].End() == FindTickEntryByComponent( priorityGroup, component ) );

	m_tickGroups[ priorityGroup ].PushBack( STickEntry( component ) ); 
}

void CR6AISystem::UnregisterComponent( CAITreeComponent* component )
{
	EAITickPriorityGroup priorityGroup = component->GetCurrentPriorityGroup();
	R6_ASSERT( priorityGroup >= TICK_EveryFrame && priorityGroup < TICK_MAX );

	TDynArray< CR6AISystem::STickEntry >::iterator it = FindTickEntryByComponent( priorityGroup, component );
	R6_ASSERT( it != m_tickGroups[ priorityGroup ].End() );

	m_tickGroups[ priorityGroup ].EraseFast( it );
}

TDynArray< CR6AISystem::STickEntry >::iterator CR6AISystem::FindTickEntryByComponent( EAITickPriorityGroup priorityGroup, const CAITreeComponent* component ) 
{
	for ( TDynArray< STickEntry >::iterator it = m_tickGroups[ priorityGroup ].Begin(); it != m_tickGroups[ priorityGroup ].End(); ++it )
	{
		if ( component == it->m_component )
		{
			return it;
		}
	}

	return m_tickGroups[ priorityGroup ].End();
}

void CR6AISystem::OnPerformAIAction( CAIAction* runningAction )
{
	R6_ASSERT( false == m_runningActions.Exist( runningAction ) );
	m_runningActions.PushBack( runningAction );

	if ( runningAction->ShouldBeTicked() )
	{
		R6_ASSERT( false == m_tickableActions.Exist( runningAction ) );
		m_tickableActions.PushBack( runningAction );
	}
}

void CR6AISystem::OnStopAIAction( CAIAction* stoppedAction )
{
	R6_ASSERT( true == m_runningActions.Exist( stoppedAction ) );
	m_runningActions.RemoveFast( stoppedAction );

	if ( stoppedAction->ShouldBeTicked() )
	{
		R6_ASSERT( true == m_tickableActions.Exist( stoppedAction ) );
		m_tickableActions.RemoveFast( stoppedAction );
	}
}

void CR6AISystem::OnGenerateDebugFragments( CRenderFrame* frame )
{
	// pass to actions
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_AIActions ) )
	{
		for ( Uint32 i = 0; i < m_runningActions.Size(); ++i )
		{
			m_runningActions[ i ]->OnGenerateDebugFragments( frame );
		}
	}

	TBaseClass::OnGenerateDebugFragments( frame );
}

