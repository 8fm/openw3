/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "componentTickManager.h"
#include "../core/memoryMacros.h"
#include "immediateJobsCollector.h"
#include "component.h"
#include "../core/taskBatch.h"
#include "../core/gameSave.h"
#include "baseEngine.h"
#include "tickManager.h"

// CComponentTickManager::TickGroupStats

void CComponentTickManager::TickGroupStats::Reset()
{
	m_statsCount = 0;
	m_statsTime = 0;
	m_numGroups = 0;
}

void CComponentTickManager::TickGroupStats::AccumulateStats( CClass* componentClass, Uint64 timeTook )
{
	// Update global stats
	m_statsCount += 1;
	m_statsTime += timeTook;

	// Update per class data
	for ( Uint32 i=0; i<m_numGroups; ++i )
	{
		if ( m_statGroups[i].m_class == componentClass )
		{
			m_statGroups[i].m_count += 1;
			m_statGroups[i].m_time += timeTook;
			return;
		}
	}

	// Define group
	if ( m_numGroups < ARRAY_COUNT( m_statGroups ) )
	{
		TickGroupClassStats& group = m_statGroups[ m_numGroups++ ];
		group.m_class = componentClass;
		group.m_count = 1;
		group.m_time = timeTook;
	}
}

// CComponentTickManager::TickGroup

void CComponentTickManager::NonBudgetedComponents::Add( CComponentTickProxy* proxy )
{
	m_components.Insert( proxy );
}

void CComponentTickManager::NonBudgetedComponents::Remove( CComponentTickProxy* proxy )
{
	m_components.Erase( proxy );
}

////////////////////////////////////////////////////////////////////////////////////////////

CComponentTickManager::BudgetedComponents::BudgetedComponents()
{
	m_safeIterator.Init( &m_components );
}

void CComponentTickManager::BudgetedComponents::Add( CComponentTickProxy* proxy )
{
	if ( proxy->m_totalRecentTickTime == 0.0f )
	{
		const Float initialGuestimatedTickTime = 0.0001f;
		proxy->m_totalRecentTickTime = initialGuestimatedTickTime;
	}
	proxy->m_accumulatedDeltaTime = 0.0f;
	m_components.PushBack( proxy );
}

void CComponentTickManager::BudgetedComponents::Remove( CComponentTickProxy* proxy )
{
	// Removing component does not break order (assures honest tick time distribution)

	m_components.Remove( proxy );
	m_tickedThisFrame.Erase( proxy );
}

////////////////////////////////////////////////////////////////////////////////////////////

CComponentTickManager::CComponentTickManager()
{
	const SGameplayLODConfig& lodConfig = GGame->GetGameplayConfig().m_LOD;

	m_maxBudgetedTickTime = lodConfig.m_maxBudgetedComponentsTickTime;
	m_minBudgetedTickPercentage = lodConfig.m_minBudgetedComponentsTickPercentage;
	ASSERT( 1 < m_minBudgetedTickPercentage && m_minBudgetedTickPercentage <= 100 );

	m_expectedNonBudgetedTickTime = lodConfig.m_expectedNonBudgetedComponentsTickTime;

	m_budgeted.m_usedBudget = 0.0f;
	m_totalRecentNonBudgetedTickTime = 0.0f;
}

void CComponentTickManager::ResetStats()
{
	for ( Uint32 i = 0; i < TICK_Max; ++i )
	{
		m_stats[ i ].Reset();
	}
}

void CComponentTickManager::TickComponent( CComponent* component, Float deltaTime, ETickGroup tickGroup )
{
	ASSERT( !component->GetTickProxy()->IsTickSuppressed() );

	// Tick the component

#ifndef NO_DEBUG_PAGES
	Uint64 startTime = 0;
	Red::System::Clock::GetInstance().GetTimer().GetTicks( startTime );
#endif

	switch ( tickGroup )
	{
	case TICK_PrePhysics:
		{
			component->OnTickPrePhysics( deltaTime );
			break;
		}

	case TICK_PrePhysicsPost:
		{
			component->OnTickPrePhysicsPost( deltaTime );
			break;
		}

	case TICK_Main:
		{
			component->OnTick( deltaTime );
			break;
		}

	case TICK_PostPhysics:
		{
			component->OnTickPostPhysics( deltaTime );
			break;
		}

	case TICK_PostPhysicsPost:
		{
			component->OnTickPostPhysicsPost( deltaTime );
			break;
		}

	case TICK_PostUpdateTransform:
		{
			component->OnTickPostUpdateTransform( deltaTime );
			break;
		}

	default:
		{
			ASSERT( !"Unsupported tick group" );
		}
	}

#ifndef NO_DEBUG_PAGES
	Uint64 endTime = 0;
	Red::System::Clock::GetInstance().GetTimer().GetTicks( endTime );

	const Uint64 timePassed = endTime - startTime;
	m_stats[ tickGroup ].AccumulateStats( component->GetClass(), timePassed );
#endif
}

void CComponentTickManager::TickComponentsSingleThreaded( Float timeDelta, ETickGroup tickGroup )
{
	// Note: This function is only here to make profiling more readable

	switch ( tickGroup )
	{
		case TICK_PrePhysics:
		{
			PC_SCOPE_PIX( TickComponents_Pre1PxFrame );
			TickComponentsSingleThreadedImpl( timeDelta, tickGroup );
			break;
		}

		case TICK_PrePhysicsPost:
		{
			PC_SCOPE_PIX( TickComponents_Pre2PxFrame );
			TickComponentsSingleThreadedImpl( timeDelta, tickGroup );
			break;
		}

		case TICK_Main:
		{
			PC_SCOPE_PIX( TickComponents_Main );
			TickComponentsSingleThreadedImpl( timeDelta, tickGroup );
			break;
		}

		case TICK_PostPhysics:
		{
			PC_SCOPE_PIX( TickComponents_Post1PxFrame );
			TickComponentsSingleThreadedImpl( timeDelta, tickGroup );
			break;
		}

		case TICK_PostPhysicsPost:
		{
			PC_SCOPE_PIX( TickComponents_Post2PxFrame );
			TickComponentsSingleThreadedImpl( timeDelta, tickGroup );
			break;
		}

		case TICK_PostUpdateTransform:
		{
			PC_SCOPE_PIX( TickComponents_PostUpdateTransform );
			TickComponentsSingleThreadedImpl( timeDelta, tickGroup );
			break;
		}

		default:
		{
			ASSERT( !"Unsupported tick group" );
		}
	}
}

void CComponentTickManager::TickComponentsSingleThreadedImpl( Float timeDelta, ETickGroup tickGroup )
{
	ASSERT( SIsMainThread() );

	// Ticking first tick-group? - perform per-frame setup

	if ( tickGroup == 0 )
	{
		PC_SCOPE_PIX( TickSetup );

		// Reset total tick time for non-budgeted components to be ticked this frame

		const Float totalRecentNonBudgetedTickTime = m_totalRecentNonBudgetedTickTime;
		m_totalRecentNonBudgetedTickTime = 0.0f;

		for ( Uint32 i = 0; i < TICK_Max; ++i )
		{
			for ( CComponentTickProxy* proxy : m_nonBudgeted[ i ].m_components )
			{
				proxy->m_totalRecentTickTime = 0.0f;
			}
		}

		// Reset accumulated delta time for budgeted components that have been ticked last frame

		for ( CComponentTickProxy* proxy : m_budgeted.m_tickedThisFrame )
		{
			proxy->m_accumulatedDeltaTime = 0.0f;
		}

		// Generate set of budgeted components to tick

		m_budgeted.m_tickedThisFrame.ClearFast();
		m_budgeted.m_usedBudget = 0.0f;

		if ( m_budgeted.m_components.Length() > 0 )
		{
			const Uint32 minComponentsToInclude = Max( ( Uint32 ) 1, m_budgeted.m_components.Length() * m_minBudgetedTickPercentage / 100 );
			ASSERT( minComponentsToInclude <= m_budgeted.m_components.Length() );

			const Float nonBudgetedTickTimeLeftovers = Max( 0.0f, m_expectedNonBudgetedTickTime - totalRecentNonBudgetedTickTime );
			const Float dynamicBudgetedTickTime = m_maxBudgetedTickTime + nonBudgetedTickTimeLeftovers;

			m_budgeted.m_safeIterator.Begin();
			while ( !m_budgeted.m_safeIterator.IsEnd() &&																// Checked all elements?
				  ( m_budgeted.m_usedBudget + m_budgeted.m_safeIterator->m_totalRecentTickTime <= dynamicBudgetedTickTime ||	// Is there enough budget available?
				    m_budgeted.m_tickedThisFrame.Size() < minComponentsToInclude ) )									// Did we include X % of components?
			{
				CComponentTickProxy* nextToInclude = *m_budgeted.m_safeIterator;
				++m_budgeted.m_safeIterator;

				// Include next component in current frame's budgeted tick

				m_budgeted.m_tickedThisFrame.Insert( nextToInclude );

				// Update used time budget estimate

				m_budgeted.m_usedBudget += nextToInclude->m_totalRecentTickTime;
			}
		}

		// Reset total tick time for budgeted components to be ticked this frame

		for ( CComponentTickProxy* proxy : m_budgeted.m_tickedThisFrame )
		{
			proxy->m_totalRecentTickTime = 0.0f;
		}

		// Update delta time accumulator for all budgeted components

		for ( CComponentTickProxy* proxy : m_budgeted.m_components )
		{
			proxy->m_accumulatedDeltaTime += timeDelta;
		}
	}

	// Tick all non-budgeted (do temporary copy to allow for removal during iteration)
	{
		PC_SCOPE_PIX( TickNonBudgeted );

		m_tickedComponentsCopy.ClearFast();
		m_tickedComponentsCopy.Reserve( m_nonBudgeted[ tickGroup ].m_components.Size() );
		for ( CComponentTickProxy* proxy : m_nonBudgeted[ tickGroup ].m_components )
		{
			m_tickedComponentsCopy.PushBack( proxy->m_component );
		}

		for ( CComponent* component : m_tickedComponentsCopy )
		{
			if ( CComponentTickProxy* proxy = component->GetTickProxy() ) // If we have a proxy it means the component is attached to world
			{
				CTimeCounter timer;

				TickComponent( component, timeDelta, tickGroup );

				const Float tickTime = timer.GetTimePeriod();
				m_totalRecentNonBudgetedTickTime += tickTime;
				proxy->m_totalRecentTickTime += tickTime;
			}
		}
	}

	// Tick budgeted (do temporary copy to allow for removal during iteration)
	{
		PC_SCOPE_PIX( TickBudgeted );

		const Uint32 tickGroupMask = 1 << tickGroup;

		m_tickedComponentsCopy.ClearFast();
		m_tickedComponentsCopy.Reserve( m_budgeted.m_tickedThisFrame.Size() );
		for ( CComponentTickProxy* proxy : m_budgeted.m_tickedThisFrame )
		{
			if ( proxy->m_tickMask & tickGroupMask )
			{
				m_tickedComponentsCopy.PushBack( proxy->m_component );
			}
		}

		for ( CComponent* component : m_tickedComponentsCopy )
		{
			if ( CComponentTickProxy* proxy = component->GetTickProxy() ) // If we have a proxy it means the component is attached to world
			{
				CTimeCounter timer;
				TickComponent( component, proxy->m_accumulatedDeltaTime, tickGroup );
				proxy->m_totalRecentTickTime += timer.GetTimePeriod();
			}
		}
	}
}

void CComponentTickManager::CollectImmediateJobs( STickManagerContext& context, CTaskBatch& taskBatch )
{
	const ETickGroup group = context.m_group;
	CTaskBatch& taskBatch_Main( context.m_asyncTick ? context.m_taskSync : taskBatch );

	if ( group == TICK_PrePhysicsPost || group == TICK_PostPhysicsPost )
	{
		// Collect non-budgeted
		for ( CComponentTickProxy* proxy : m_nonBudgeted[ group ].m_components )
		{
			if ( proxy->UseImmediateJobs_ActorPass() )
			{
				proxy->m_component->CollectImmediateJobs( context, taskBatch );
			}
			else if ( proxy->UseImmediateJobs_MainPass() )
			{
				proxy->m_component->CollectImmediateJobs( context, taskBatch_Main );
			}
		}

		// Collect budgeted
		const Uint32 tickMask = 1 << group;
		const Float oldDeltaTime = context.m_timeDelta;
		for ( CComponentTickProxy* proxy : m_budgeted.m_tickedThisFrame )
		{
			context.m_timeDelta = proxy->m_accumulatedDeltaTime;
			if ( proxy->UseImmediateJobs_ActorPass() && proxy->m_tickMask & tickMask )
			{
				proxy->m_component->CollectImmediateJobs( context, taskBatch );
			}
			else if ( proxy->UseImmediateJobs_MainPass() && proxy->m_tickMask & tickMask )
			{
				proxy->m_component->CollectImmediateJobs( context, taskBatch_Main );
			}
		}
		context.m_timeDelta = oldDeltaTime;
	}
}

void CComponentTickManager::AddToGroup( CComponent* component, ETickGroup group )
{
	ASSERT( group < TICK_Max );

	CComponentTickProxy* proxy = component->GetOrCreateTickProxy();

	const Uint32 groupMask = ( 1 << group );
	const Uint32 oldTickMask = proxy->m_tickMask;
	proxy->m_tickMask |= groupMask;

	if ( !proxy->IsTickSuppressed() )
	{
		if ( proxy->IsTickBudgeted() )
		{
			if ( !oldTickMask )
			{
				m_budgeted.Add( proxy );
			}
		}
		else if ( oldTickMask != proxy->m_tickMask )
		{
			m_nonBudgeted[ group ].Add( proxy );
		}
	}
}

void CComponentTickManager::RemoveFromGroup( CComponent* component, ETickGroup group )
{
	ASSERT( group < TICK_Max );

	CComponentTickProxy* proxy = component->GetTickProxy();
	if ( !proxy )
	{
		return;
	}

	const Uint32 groupMask = ( 1 << group );
	if ( !( proxy->m_tickMask & groupMask ) )
	{
		return;
	}

	const Uint32 oldTickMask = proxy->m_tickMask;
	proxy->m_tickMask &= ~groupMask;

	if ( !proxy->IsTickSuppressed() )
	{
		if ( proxy->IsTickBudgeted() )
		{
			if ( oldTickMask && !proxy->m_tickMask )
			{
				m_budgeted.Remove( proxy );
			}
		}
		else
		{
			m_nonBudgeted[ group ].Remove( proxy );
		}
	}
}

void CComponentTickManager::Remove( CComponent* component )
{
	CComponentTickProxy* proxy = component->GetTickProxy();
	if ( !proxy )
	{
		return;
	}

	if ( const Uint32 componentTickMask = proxy->m_tickMask )
	{
		if ( !proxy->IsTickSuppressed() )
		{
			if ( proxy->IsTickBudgeted() )
			{
				m_budgeted.Remove( proxy );
			}
			else
			{
				for ( Uint32 i = 0; i < TICK_Max; ++i )
				{
					const Uint32 tickGroupMask = ( 1 << i );
					if ( componentTickMask & tickGroupMask )
					{
						m_nonBudgeted[ i ].Remove( proxy );
					}
				}
			}
		}
		proxy->m_tickMask = 0;
	}
}

void CComponentTickManager::SetBudgeted( CComponent* component, Bool budgeted, CComponent::ETickBudgetingReason reason )
{
	CComponentTickProxy* proxy = component->GetOrCreateTickProxy();

	if ( proxy->IsTickBudgeted( reason ) != budgeted )
	{
		// Toggle tick-budgeted flag

		const Uint8 oldBudgetedMask = proxy->m_tickBudgetingMask;
		if ( budgeted )
		{
			proxy->m_tickBudgetingMask |= reason;
		}
		else
		{
			proxy->m_tickBudgetingMask &= ~reason;
		}

		// Add/remove to/from appropriate tick containers

		if ( !proxy->IsTickSuppressed() &&
			( oldBudgetedMask != 0 ) != ( proxy->m_tickBudgetingMask != 0 ) )
		{
			if ( const Uint32 componentTickMask = proxy->m_tickMask )
			{
				if ( budgeted )
				{
					m_budgeted.Add( proxy );
				}
				else
				{
					m_budgeted.Remove( proxy );
				}

				for ( Uint32 i = 0; i < TICK_Max; ++i )
				{
					const Uint32 tickGroupMask = ( 1 << i );
					if ( componentTickMask & tickGroupMask )
					{
						if ( budgeted )
						{
							m_nonBudgeted[ i ].Remove( proxy );
						}
						else
						{
							m_nonBudgeted[ i ].Add( proxy );
						}
					}
				}
			}
		}
	}
}

void CComponentTickManager::Suppress( CComponent* component, Bool suppress, CComponent::ETickSuppressReason reason )
{
	CComponentTickProxy* proxy = component->GetOrCreateTickProxy();

	const Uint8 oldSuppressionMask = proxy->m_tickSuppressionMask;
	if ( suppress )
	{
		proxy->m_tickSuppressionMask |= reason;
	}
	else
	{
		proxy->m_tickSuppressionMask &= ~reason;
	}

	const Bool wasSuppressed = oldSuppressionMask != 0;
	const Bool isSuppressed = proxy->m_tickSuppressionMask != 0;
	if ( wasSuppressed != isSuppressed )
	{
		// Remove from all groups but preserve component's tick mask

		if ( const Uint32 componentTickMask = proxy->m_tickMask )
		{
			if ( proxy->IsTickBudgeted() )
			{
				if ( suppress )
				{
					m_budgeted.Remove( proxy );
				}
				else
				{
					m_budgeted.Add( proxy );
				}
			}
			else
			{
				for ( Uint32 i = 0; i < TICK_Max; ++i )
				{
					const Uint32 tickGroupMask = ( 1 << i );
					if ( componentTickMask & tickGroupMask )
					{
						if ( suppress )
						{
							m_nonBudgeted[ i ].Remove( proxy );
						}
						else
						{
							m_nonBudgeted[ i ].Add( proxy );
						}
					}
				}
			}
		}
	}
}
