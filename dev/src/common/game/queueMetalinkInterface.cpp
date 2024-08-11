/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "queueMetalinkInterface.h"

const EngineTime IAIQueueMetalinkInterface::RECALCULATION_MIN_DELAY( 0.33f );

////////////////////////////////////////////////////////////////////////////
// IAIQueueMetalinkInterface
////////////////////////////////////////////////////////////////////////////
IAIQueueMetalinkInterface::IAIQueueMetalinkInterface()
	: m_nextPriorityCalculation( EngineTime::ZERO )
	, m_locked( false )
{}

IAIQueueMetalinkInterface::~IAIQueueMetalinkInterface()
{}
////////////////////////////////////////////////////////////////////////////

void IAIQueueMetalinkInterface::LazyCalculatePriorities()
{
	const EngineTime& curTime = GGame->GetEngineTime();
	if ( m_nextPriorityCalculation < curTime )
	{
		m_nextPriorityCalculation = curTime + RECALCULATION_MIN_DELAY;

		for ( auto it = m_registered.Begin(); it != m_registered.End(); ) // NOTICE: we can erase elements during iteration - so no precomputed end and only manual iterator incrementation
		{
			Member& m = *it;
			CActor* guy = m.m_guy.Get();
			if ( !guy )
			{
				// shouldn't really happen but lets shield
				m_registered.Erase( it );
				continue;
			}

			// recalculate priority
			m.m_priority = guy->SignalGameplayEventReturnFloat( AIPriorityEventName(), -1.f );

			++it;
		}
	}
}

void IAIQueueMetalinkInterface::Register( CActor* actor, Float priority )
{
	Member m;
	m.m_guy = actor;
	m.m_priority = priority;
	m_registered.Insert( Move( m ) );
}
void IAIQueueMetalinkInterface::Unregister( CActor* actor )
{
	for ( auto it = m_registered.Begin(), end = m_registered.End(); it != end; ++it )
	{
		if ( it->m_guy.Get() == actor )
		{
			m_registered.Erase( it );
			break;
		}
	}
}

void IAIQueueMetalinkInterface::Lock( Bool b )
{
	m_locked = b;
}

Bool IAIQueueMetalinkInterface::CanIGo( CActor* actor )
{
	return !m_registered.Empty() && m_registered[ 0 ].m_guy.Get() == actor;
}


CName IAIQueueMetalinkInterface::AIPriorityEventName()
{
	return CNAME( AI_ExplorationQueueCalculatePriority );
}

CName IAIQueueMetalinkInterface::AIReleaseLockEventName()
{
	return CNAME( AI_ExplorationQueueReleaseLock );
}