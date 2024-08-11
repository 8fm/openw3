/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../core/memoryMacros.h"
#include "timerManager.h"
#include "../core/gameSave.h"
#include "baseEngine.h"
#include "gameTimeManager.h"
#include "gameSaveManager.h"

RED_DEFINE_STATIC_NAME( timers );
RED_DEFINE_STATIC_NAME( group );
RED_DEFINE_STATIC_NAME( timeToNextEvent );
RED_DEFINE_STATIC_NAME( timeSinceLastEvent );
RED_DEFINE_STATIC_NAME( repeatTime );
RED_DEFINE_STATIC_NAME( isRealTime );
RED_DEFINE_STATIC_NAME( timerIdGenerator );

CTimerManager::CTimerManager()
	: m_time( 0.0f )
{}

void CTimerManager::AdvanceTime( Float timeDelta )
{
	ASSERT( timeDelta >= 0.0f );
	m_time += timeDelta;
}

Bool CTimerManager::ValidateTimerFunctionExists( CEntity* entity, CName name, Bool isRealTime )
{
	IScriptable* context = entity;

	const CFunction* function = NULL;
	if ( !FindFunction( context, name, function ) )
	{
		RED_LOG( CTimerManager, TXT("Warning: attempt to add timer '%ls' for '%ls' but there's no matching script function."), name.AsChar(), entity->GetFriendlyName().AsChar() );
		return true;
	}

	if ( !CheckFunction( context, function, 2 ) )
	{
		RED_LOG( CTimerManager, TXT("Failed to add timer '%ls' for '%ls', reason: function doesn't take 2 parameters."), name.AsChar(), entity->GetFriendlyName().AsChar() );
		return false;
	}

	if ( isRealTime )
	{
		if ( !CheckFunctionParameter( function, 0, ::GetTypeObject< Float >() ) )
		{
			RED_LOG( CTimerManager, TXT("Failed to add real-time timer '%ls' for '%ls', reason: first argument must be float."), name.AsChar(), entity->GetFriendlyName().AsChar() );
			return false;
		}
	}
	else
	{
		if ( !CheckFunctionParameter( function, 0, ::GetTypeObject< GameTime >() ) )
		{
			RED_LOG( CTimerManager, TXT("Failed to add game-time timer '%ls' for '%ls', reason: first argument must be GameTime."), name.AsChar(), entity->GetFriendlyName().AsChar() );
			return false;
		}
	}
	if ( !CheckFunctionParameter( function, 1, ::GetTypeObject< Int32 >() ) )
	{
		RED_LOG( CTimerManager, TXT("Failed to add timer '%ls' for '%ls', reason: second argument must be int."), name.AsChar(), entity->GetFriendlyName().AsChar() );
		return false;
	}

	return true;
}

Uint32 CTimerManager::AddTimer( CEntity* entity, ETickGroup group, const CName& name, Float length, Bool repeats, Bool scatter, Bool savable, Bool overrideExisting )
{
	ASSERT( name );
	ASSERT( entity );
	ASSERT( group < TICK_Max );
	ASSERT( length >= 0.0f, TXT("Attempted to add timer '%s' with length of %f. Length must be positive!"), name.AsChar(), length );
	ASSERT( SIsMainThread() );

	if ( !ValidateTimerFunctionExists( entity, name, true ) )
	{
		return 0xFFFFFFFF;
	}

	const Float startDelay = scatter ? GEngine->GetRandomNumberGenerator().Get< Float >( length ) : length;
	const Float repeatTime = repeats ? length : -1.0f;

	// Check if timer of that name doesn't exist already

	Timer*& list = m_timers[ group ].m_timers.GetRef( entity, nullptr );

	if ( overrideExisting )
	{
		RealTimer* current = static_cast< RealTimer* >( list );
		while ( current )
		{
			if ( current->m_name == name )
			{
				RED_ASSERT( current->m_isRealTime );
				current->m_lastEventTime = m_time;
				current->m_nextEventTime = m_time + startDelay;
				current->m_repeatTime = repeatTime;
				m_timers[ group ].m_realTimersQueue.Update( current );
				return current->m_id;
			}

			current = static_cast< RealTimer* >( current->m_next );
		}
	}

	// Add new timer

	RealTimer* newTimer = new RealTimer( entity, name, GenerateUniqueTimerId(), savable, m_time, m_time + startDelay, repeatTime );
	ASSERT( newTimer );
	m_timers[ group ].m_realTimersQueue.Push( newTimer );

	newTimer->m_next = list;
	list = newTimer;

	return newTimer->m_id;
}

Uint32 CTimerManager::AddTimer( CEntity* entity, ETickGroup group, const CName& name, GameTime period, Bool repeats, Bool scatter, Bool savable, Bool overrideExisting )
{
	ASSERT( name );
	ASSERT( entity );
	ASSERT( group < TICK_Max );
	ASSERT( period.m_seconds > 0, TXT("Attempted to add timer '%s' with length of %u seconds. Length must be positive!"), name.AsChar(), ( Uint32 ) period.m_seconds );
	ASSERT( SIsMainThread() );

	if ( !ValidateTimerFunctionExists( entity, name, false ) )
	{
		return 0xFFFFFFFF;
	}

	const GameTime currentGameTime = GGame->GetTimeManager()->GetTime();
	const GameTime repeatTime = repeats ? period : GameTime( 0 );

	// Check if timer of that name doesn't exist already

	Timer*& list = m_timers[ group ].m_timers.GetRef( entity, nullptr );

	if ( overrideExisting )
	{
		GameTimer* current = static_cast< GameTimer* >( list );
		while ( current )
		{
			if ( current->m_name == name )
			{
				RED_ASSERT( !current->m_isRealTime );
				current->m_lastEventTime = currentGameTime;
				current->m_nextEventTime = currentGameTime + period;
				current->m_repeatTime = repeatTime;
				m_timers[ group ].m_gameTimersQueue.Update( current );
				return current->m_id;
			}

			current = static_cast< GameTimer* >( current->m_next );
		}
	}

	// Add new timer

	GameTimer* newTimer = new GameTimer( entity, name, GenerateUniqueTimerId(), savable, currentGameTime, currentGameTime + period, repeatTime );
	ASSERT( newTimer );
	m_timers[ group ].m_gameTimersQueue.Push( newTimer );

	newTimer->m_next = list;
	list = newTimer;

	return newTimer->m_id;
}

void CTimerManager::RemoveTimer( CEntity* entity, ETickGroup group, const CName& name )
{
	ASSERT( entity );
	ASSERT( group < TICK_Max );
	ASSERT( SIsMainThread() );

	auto it = m_timers[ group ].m_timers.Find( entity );
	if ( it != m_timers[ group ].m_timers.End() )
	{
		Timer** timer = &it->m_second;
		while ( *timer )
		{
			if ( ( *timer )->m_name == name )
			{
				// Delete the timer and fix up the list

				Timer* timerToDelete = *timer;

				*timer = (*timer)->m_next;
				m_timers[ group ].RemoveFromQueue( timerToDelete );
				delete timerToDelete;

				// Remove entity entry if it has no more timers now

				if ( !it->m_second )
				{
					m_timers[ group ].m_timers.Erase( it );
					return;
				}

				continue; // Search for other timers with matching name
			}
			timer = &( *timer )->m_next;
		}
	}
}

void CTimerManager::RemoveTimer( CEntity* entity, ETickGroup group, Uint32 id )
{
	ASSERT( entity );
	ASSERT( group < TICK_Max );
	ASSERT( SIsMainThread() );

	auto it = m_timers[ group ].m_timers.Find( entity );
	if ( it != m_timers[ group ].m_timers.End() )
	{
		Timer** timer = &it->m_second;
		while ( *timer )
		{
			if ( ( *timer )->m_id == id )
			{
				// Delete the timer and fix up the list

				Timer* timerToDelete = *timer;

				*timer = (*timer)->m_next;
				m_timers[ group ].RemoveFromQueue( timerToDelete );
				delete timerToDelete;

				// Remove entity entry if it has no more timers now

				if ( !it->m_second )
				{
					m_timers[ group ].m_timers.Erase( it );
				}

				return;
			}
			timer = &( *timer )->m_next;
		}
	}
}

void CTimerManager::RemoveTimers( CEntity* entity )
{
	ASSERT( entity );
	ASSERT( SIsMainThread() );

	for ( Uint32 group = 0; group < TICK_Max; group++ )
	{
		auto it = m_timers[ group ].m_timers.Find( entity );
		if ( it != m_timers[ group ].m_timers.End() )
		{
			Timer* timer = it->m_second;
			while ( timer )
			{
				Timer* next = timer->m_next;
				m_timers[ group ].RemoveFromQueue( timer );
				delete timer;
				timer = next;
			}
		
			m_timers[ group ].m_timers.Erase( it );
		}
	}
}

CTimerManager::Timer* CTimerManager::FindTimer( CEntity* entity, ETickGroup group, Uint32 id )
{
	auto it = m_timers[ group ].m_timers.Find( entity );
	if ( it != m_timers[ group ].m_timers.End() )
	{
		Timer* current = it->m_second;
		while ( current )
		{
			if ( current->m_id == id )
			{
				return current;
			}
			current = current->m_next;
		}
	}
	return nullptr;
}

void CTimerManager::SaveTimer( const Timer* timer, ETickGroup group, IGameSaver* saver )
{
	saver->WriteValue< Uint8 >( CNAME( group ), ( Uint8 ) group );
	saver->WriteValue( CNAME( name ), timer->m_name );
	saver->WriteValue( CNAME( isRealTime ), timer->m_isRealTime );
	saver->WriteValue( CNAME( id ), timer->m_id );
	if ( timer->m_isRealTime )
	{
		const RealTimer* realTimer = static_cast< const RealTimer* >( timer );
		saver->WriteValue( CNAME( timeToNextEvent ), realTimer->m_nextEventTime - m_time );
		saver->WriteValue( CNAME( timeSinceLastEvent ), m_time - realTimer->m_lastEventTime );
		saver->WriteValue( CNAME( repeatTime ), realTimer->m_repeatTime );
	}
	else
	{
		const GameTime currentGameTime = GGame->GetTimeManager()->GetTime();
		const GameTimer* gameTimer = static_cast< const GameTimer* >( timer );

		saver->WriteValue( CNAME( timeToNextEvent ), gameTimer->m_nextEventTime - currentGameTime );
		saver->WriteValue( CNAME( timeSinceLastEvent ), currentGameTime - gameTimer->m_lastEventTime );
		saver->WriteValue( CNAME( repeatTime ), gameTimer->m_repeatTime );
	}
}

void CTimerManager::LoadTimer( CEntity* entity, IGameLoader* loader )
{
	ETickGroup group;
	CName name;
	Bool isRealTime;
	Uint32 id;
	Float timeToNextEvent, timeSinceLastEvent, repeatTime;
	GameTime timeToNextEventGT, timeSinceLastEventGT, repeatTimeGT;

	// Load timer data

	Uint8 groupAsUint8;
	loader->ReadValue< Uint8 >( CNAME( group ), groupAsUint8 );
	group = ( ETickGroup ) groupAsUint8;
	loader->ReadValue( CNAME( name ), name );
	if ( loader->GetSaveVersion() >= SAVE_VERSION_TIMERS_WITH_IDS )
	{
		loader->ReadValue( CNAME( isRealTime ), isRealTime );
		loader->ReadValue( CNAME( id ), id );
		if ( isRealTime )
		{
			loader->ReadValue( CNAME( timeToNextEvent ), timeToNextEvent );
			loader->ReadValue( CNAME( timeSinceLastEvent ), timeSinceLastEvent );
			loader->ReadValue( CNAME( repeatTime ), repeatTime );
		}
		else
		{
			loader->ReadValue( CNAME( timeToNextEvent ), timeToNextEventGT );
			loader->ReadValue( CNAME( timeSinceLastEvent ), timeSinceLastEventGT );
			loader->ReadValue( CNAME( repeatTime ), repeatTimeGT );
		}
	}
	else
	{
		isRealTime = true;
		id = GenerateUniqueTimerId();
		loader->ReadValue( CNAME( timeToNextEvent ), timeToNextEvent );
		loader->ReadValue( CNAME( timeSinceLastEvent ), timeSinceLastEvent );
		loader->ReadValue( CNAME( repeatTime ), repeatTime );
	}

	// Create timer

	Timer* timer = nullptr;
	if ( isRealTime )
	{
		RealTimer* realTimer = new RealTimer( entity, name, id, true, m_time - timeSinceLastEvent, m_time + timeToNextEvent, repeatTime );
		m_timers[ group ].m_realTimersQueue.Push( realTimer );
		timer = realTimer;
	}
	else
	{
		const GameTime currentGameTime = GGame->GetTimeManager()->GetTime();
		GameTimer* gameTimer = new GameTimer( entity, name, id, true, currentGameTime - timeSinceLastEventGT, currentGameTime + timeToNextEventGT, repeatTimeGT );
		m_timers[ group ].m_gameTimersQueue.Push( gameTimer );
		timer = gameTimer;
	}

	// Add timer to the list of timers

	Timer*& timers = m_timers[ group ].m_timers.GetRef( entity, nullptr );
	timer->m_next = timers;
	timers = timer;
}

void CTimerManager::SaveTimers( CEntity* entity, IGameSaver* saver )
{
	// Count savable timers

	Uint32 count = 0;
	Timer** timerLists[ TICK_Max ];
	for ( Uint32 group = 0; group < TICK_Max; group++ )
	{
		if ( ( timerLists[ group ] = m_timers[ group ].m_timers.FindPtr( entity ) ) )
		{
			const Timer* timer = *timerLists[ group ];
			while ( timer )
			{
				if ( timer->m_savable )
				{
					++count;
				}
				timer = timer->m_next;
			}
		}
	}

	// Write out savable timers
	{
		CGameSaverBlock block( saver, CNAME( timers ) );

		ASSERT( count < 256 );
		saver->WriteValue< Uint8 >( CNAME( count ), ( Uint8 ) count );

		if ( count > 0 )
		{
			for ( Uint32 group = 0; group < TICK_Max; group++ )
			{
				if ( timerLists[ group ] )
				{
					const Timer* timer = *timerLists[ group ];
					while ( timer )
					{
						if ( timer->m_savable )
						{
							SaveTimer( timer, ( ETickGroup ) group, saver );
						}
						timer = timer->m_next;
					}
				}
			}
		}
	}
}

void CTimerManager::LoadTimers( CEntity* entity, IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME( timers ) );

	Uint8 count = 0;
	loader->ReadValue< Uint8 >( CNAME( count ), count );

	for ( Uint32 i = 0; i < count; ++i )
	{
		LoadTimer( entity, loader );
	}
}

void CTimerManager::OnSaveGameplayState( IGameSaver* saver )
{
	CGameSaverBlock block( saver, CNAME( timers ) );
	saver->WriteValue( CNAME( timerIdGenerator ), m_timerIdGenerator );
}

void CTimerManager::OnLoadGameplayState( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME( timers ) );
	loader->ReadValue( CNAME( timerIdGenerator ), m_timerIdGenerator );
}

void CTimerManager::ResetStats()
{
	for ( Uint32 i = 0; i < TICK_Max; ++i )
	{
		m_timers[ i ].m_stats.Reset();
	}
#ifdef TIMER_MANAGER_DETAILED_TIMER_STATS
	m_firedTimersStats.ClearFast();
#endif
}

void CTimerManager::FireTimers( ETickGroup group )
{
	PC_SCOPE_PIX( FireTimers );
	ASSERT( SIsMainThread() );

#ifndef NO_DEBUG_PAGES
	// Init stats
	Uint64 timeStart = 0;
	Red::System::Clock::GetInstance().GetTimer().GetTicks( timeStart );
#endif

	// Fire real time timers

	const Uint32 numTimersFired =
		FireTimers< RealTimer, RealTimer::Compare, Float >( group, m_timers[ group ].m_realTimersQueue, m_time ) +
		FireTimers< GameTimer, GameTimer::Compare, GameTime >( group, m_timers[ group ].m_gameTimersQueue, GGame->GetTimeManager()->GetTime() );

#ifndef NO_DEBUG_PAGES
	// Update stats
	Uint64 timeEnd = 0;
	Red::System::Clock::GetInstance().GetTimer().GetTicks( timeEnd );

	m_timers[ group ].m_stats.m_statsCount += numTimersFired;
	m_timers[ group ].m_stats.m_statsTime += ( timeEnd - timeStart );
#endif
}

#ifdef TIMER_MANAGER_DETAILED_TIMER_STATS

const TDynArray< CTimerManager::FiredTimerStats >& CTimerManager::SortAndGetFiredTimerStats() const
{
	if ( !m_firedTimersStats.Empty() )
	{
		// Group timers by name

		Sort( m_firedTimersStats.Begin(), m_firedTimersStats.End(), FiredTimerStats::CmpByName() );

		Uint32 dst = 0;
		Uint32 src = 0;
		while ( src < m_firedTimersStats.Size() )
		{
			m_firedTimersStats[ dst ].m_name = m_firedTimersStats[ src ].m_name;
			Uint32 count = 0;
			Float totalTime = 0;
			while ( src < m_firedTimersStats.Size() && m_firedTimersStats[ dst ].m_name == m_firedTimersStats[ src ].m_name )
			{
				totalTime += m_firedTimersStats[ src ].m_timeMS;
				++count;
				++src;
			}
			m_firedTimersStats[ dst ].m_count = count;
			m_firedTimersStats[ dst ].m_timeMS = totalTime;
			++dst;
		}
		m_firedTimersStats.ResizeFast( dst );

		// Sort timers descending by time

		Sort( m_firedTimersStats.Begin(), m_firedTimersStats.End(), FiredTimerStats::CmpByTime() );
	}
	return m_firedTimersStats;
}

#endif
