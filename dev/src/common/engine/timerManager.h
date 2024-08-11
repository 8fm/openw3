/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/intrusivePriorityQueue.h"
#include "gameTime.h"
#include "tickStats.h"

#if !defined( RED_FINAL_BUILD ) || defined( RED_PROFILE_BUILD ) 
	// Enables collection of all timers fired per frame
	#define TIMER_MANAGER_DETAILED_TIMER_STATS
#endif

/**
 *	Gameplay timer manager.
 */
class CTimerManager
{
	friend class CTickManager;
public:

#ifdef TIMER_MANAGER_DETAILED_TIMER_STATS
	// Fired timer stats
	struct FiredTimerStats
	{
		Float m_timeMS;
		CName m_name;
		Uint32 m_count;

		struct CmpByTime
		{
			Bool operator () ( const FiredTimerStats& a, const FiredTimerStats& b ) const
			{
				return b.m_timeMS < a.m_timeMS;
			}
		};

		struct CmpByName
		{
			Bool operator () ( const FiredTimerStats& a, const FiredTimerStats& b ) const
			{
				return a.m_name < b.m_name;
			}
		};
	};
#endif

private:
	// Entity event timer
	struct Timer : TIntrusivePriorityQueueElement
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Gameplay );
	public:
		CEntity*		m_entity;			// Owner entity
		Timer*			m_next;				// Pointer to the next entity timer (for the same entity)
		CName			m_name;				// Timer name
		Uint32			m_id;				// Unique timer id
		Bool			m_isRealTime;		// Is this timer using real time? Otherwise it's using game time
		Bool			m_savable;			// Timer gets saved/restored in savegames

		RED_INLINE Timer( CEntity* entity, CName name, Uint32 id, Bool savable, Bool isRealTime )
			: m_entity( entity )
			, m_name ( name )
			, m_id( id )
			, m_isRealTime( isRealTime )
			, m_savable( savable )
			, m_next( nullptr )
		{}
	};

	// Timer using real (float based) time
	struct RealTimer : Timer
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Gameplay );
	public:

		Float			m_nextEventTime;	// Time of next event
		Float			m_repeatTime;		// Repeat time, 0.0f if not repeating
		Float			m_lastEventTime;	// Time of last event
		Bool IsRepeating() const { return m_repeatTime >= 0.0f; }

		RED_INLINE RealTimer( CEntity* entity, CName name, Uint32 id, Bool savable, Float lastEvent, Float nextEvent, Float repeatTime )
			: Timer( entity, name, id, savable, true )
			, m_nextEventTime( nextEvent )
			, m_repeatTime( repeatTime )
			, m_lastEventTime( lastEvent )
		{}

		struct Compare
		{
			static Bool Less( const RealTimer* a, const RealTimer* b ) { return a->m_nextEventTime < b->m_nextEventTime; }
		};
	};

	// Timer using game time (in seconds)
	struct GameTimer : Timer
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Gameplay );
	public:
		GameTime		m_nextEventTime;	// Time of next event
		GameTime		m_repeatTime;		// Repeat time, 0 if not repeating
		GameTime		m_lastEventTime;	// Time of last event
		Bool IsRepeating() const { return m_repeatTime.m_seconds != 0; }

		RED_INLINE GameTimer( CEntity* entity, CName name, Uint32 id, Bool savable, GameTime lastEvent, GameTime nextEvent, GameTime repeatTime )
			: Timer( entity, name, id, savable, false )
			, m_nextEventTime( nextEvent )
			, m_repeatTime( repeatTime )
			, m_lastEventTime( lastEvent )
		{}

		struct Compare
		{
			static Bool Less( const GameTimer* a, const GameTimer* b ) { return a->m_nextEventTime < b->m_nextEventTime; }
		};
	};

	typedef TIntrusivePriorityQueue< RealTimer, RealTimer::Compare > RealTimeTimersQueue;
	typedef TIntrusivePriorityQueue< GameTimer, GameTimer::Compare > GameTimeTimersQueue;

	// Set of timers for a particular tick group
	struct TimerSet
	{
		RealTimeTimersQueue				m_realTimersQueue;
		GameTimeTimersQueue				m_gameTimersQueue;
		THashMap< CEntity*, Timer* >	m_timers;			// Active timers for each entity; each value is actually a list of timers
		STickGenericStats				m_stats;

		void RemoveFromQueue( Timer* timer ) { timer->m_isRealTime ? m_realTimersQueue.Remove( static_cast< RealTimer* >( timer ) ) : m_gameTimersQueue.Remove( static_cast< GameTimer* >( timer ) ); }
	};

	TimerSet							m_timers[ TICK_Max ];				// Timers per tick group
	Uint32								m_timerIdGenerator;					// Generator of unique timer ids
	Float								m_time;								// Current time
#ifdef TIMER_MANAGER_DETAILED_TIMER_STATS
	mutable TDynArray< FiredTimerStats >m_firedTimersStats;					// Stats: set of timers fired last frame
#endif

public:
	// Add real-time based entity timer; returns timer id (new or existing depending on 'overrideExisting' parameter)
	Uint32 AddTimer( CEntity* entity, ETickGroup group, const CName& name, Float length, Bool repeats, Bool scatter, Bool savable, Bool overrideExisting );
	// Add game-time based entity timer; returns timer id (new or existing depending on 'overrideExisting' parameter)
	Uint32 AddTimer( CEntity* entity, ETickGroup group, const CName& name, GameTime period, Bool repeats, Bool scatter, Bool savable, Bool overrideExisting );
	// Remove all entity timers matching given name in given tick group
	void RemoveTimer( CEntity* entity, ETickGroup group, const CName& name );
	// Remove entity timer matching id in given tick group
	void RemoveTimer( CEntity* entity, ETickGroup group, Uint32 id );
	// Remove all entity timers
	void RemoveTimers( CEntity* entity );

	// Saves all savable timers for an entity
	void SaveTimers( CEntity* entity, IGameSaver* saver );
	// Loads (savable) timers for an entity
	void LoadTimers( CEntity* entity, IGameLoader* loader );

#ifdef TIMER_MANAGER_DETAILED_TIMER_STATS
	const TDynArray< FiredTimerStats >& SortAndGetFiredTimerStats() const;
#endif

private:
	CTimerManager();
	RED_INLINE const STickGenericStats& GetStats( ETickGroup group ) const { return m_timers[ group ].m_stats; }
	void AdvanceTime( Float time );
	void FireTimers( ETickGroup group );
	void ResetStats();

	void OnSaveGameplayState( IGameSaver* saver );
	void OnLoadGameplayState( IGameLoader* loader );

	void SaveTimer( const Timer* timer, ETickGroup group, IGameSaver* saver );
	void LoadTimer( CEntity* entity, IGameLoader* loader );
	RED_FORCE_INLINE Uint32 GenerateUniqueTimerId() { return ++m_timerIdGenerator; }

	Timer* FindTimer( CEntity* entity, ETickGroup group, Uint32 id );

	template < typename TIMER_TYPE, typename TIMER_COMPARE_TYPE, typename TIME_TYPE >
	Uint32 FireTimers( ETickGroup group, TIntrusivePriorityQueue< TIMER_TYPE, TIMER_COMPARE_TYPE >& queue, TIME_TYPE currentTime )
	{
		Uint32 numTimersFired = 0;

		TIMER_TYPE* timer = nullptr;
		while ( ( timer = queue.Front() ) && timer->m_nextEventTime < currentTime )
		{
			const TIME_TYPE elapsed = currentTime - timer->m_lastEventTime;

#ifdef TIMER_MANAGER_DETAILED_TIMER_STATS
			FiredTimerStats timerStats;
			timerStats.m_name = timer->m_name;
			CTimeCounter executionTimer;
#endif
			// Is this repeating timer?

			if ( timer->IsRepeating() )
			{
				// Update next event time

				timer->m_lastEventTime = currentTime;
				timer->m_nextEventTime = currentTime + timer->m_repeatTime;
				queue.Update( timer );

				// Fire it

				timer->m_entity->OnTimer( timer->m_name, timer->m_id, elapsed );
			}
			else // Timer expired
			{
				CEntity* entity = timer->m_entity;
				const CName timerName = timer->m_name;
				const Uint32 timerId = timer->m_id;

				// Delete it

				RemoveTimer( timer->m_entity, group, timer->m_id );

				// Fire it

				entity->OnTimer( timerName, timerId, elapsed );
			}

			++numTimersFired;
#ifdef TIMER_MANAGER_DETAILED_TIMER_STATS
			timerStats.m_timeMS = ( Float ) executionTimer.GetTimePeriodMS();
			m_firedTimersStats.PushBack( timerStats );
#endif
		}

		return numTimersFired;
	}

	Bool ValidateTimerFunctionExists( CEntity* entity, CName name, Bool isRealTime );
};