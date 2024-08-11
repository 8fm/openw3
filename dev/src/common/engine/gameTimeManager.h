/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../engine/game.h"
#include "gameTime.h"

/// Event in game time
class ITimeEvent
{
protected:
	Bool					m_ownerIsNull;		//!< Is owner of this event null?
	THandle<IScriptable>	m_owner;			//!< Owner of this event
	GameTime				m_period;			//!< Period of event
	GameTime				m_date;				//!< Event time
	Int32					m_limit;			//!< Repeat count
	Bool					m_ignoreDays;		//!< Event repeated every day

public:
	//! Is it time to play the event ?
	RED_INLINE Bool IsTime( const GameTime &time ) const { return m_date < time; }

	//! Is this event periodic ?
	RED_INLINE Bool IsPeriodic() const { return m_period.GetSeconds() > 0; }

	//! Get owner of this event
	RED_INLINE const THandle<IScriptable>& GetOwner() { return m_owner; }

	//! Is the timer owner valid ?
	RED_INLINE Bool IsOwnerValid() const { return m_ownerIsNull || m_owner.IsValid(); }

public:
	ITimeEvent( const THandle<IScriptable>& owner, GameTime date, GameTime period, Int32 limit, Bool relative );
	virtual ~ITimeEvent();

	//! Reschedule event
	void Repeat();

	//! Fire event
	virtual void OnEvent( CNode* caller, const CName& eventName, void* params ) = 0;

public:
	struct CompareFunc
	{
		static RED_INLINE Bool Less( ITimeEvent *event1, ITimeEvent* event2 )
		{
			return event1->m_date < event2->m_date;
		}	
	};
};

typedef TSortedArray< ITimeEvent *, ITimeEvent::CompareFunc > TTimeEventList;

/// Time manager
class CTimeManager : public IGameSaveSection
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

private:
	GameTime		m_time;			//!< Current time
	Float			m_deltaAccum;	//!< Accumulated time delta
	Bool			m_isPaused;		//!< Game time is paused
	TTimeEventList	m_events;		//!< Sorted list of time events to fire

	Float			m_hoursPerMinute; 
	Float			m_lastTick;

	Uint32			m_timeScalePriorityIndexGenerator;

public:
	//! Get current time
	RED_INLINE GameTime GetTime() const { return m_time; }

	//! Is game time paused
	Bool IsPaused() const;

	//! Last time delta
	RED_INLINE Float GetLastTickTime() const { return m_lastTick; }

	//! Get the time multiplier
	RED_INLINE Float GetHoursPerMinute() const { return m_hoursPerMinute; }

	Uint32 GenerateNewTimeScalePriorityIndex() { return m_timeScalePriorityIndexGenerator++; }

public:
	CTimeManager();
	~CTimeManager();

	void SetTime( GameTime time, Bool callEvents );
	void AddEvent( ITimeEvent *timeEvent );
	void RemoveEvent( ITimeEvent *timeEvent );
	void RemoveEvents( const THandle<IScriptable>& object );
	void Tick( Float timeDelta );

	void Pause() { m_isPaused = true; }
	void Resume() { m_isPaused = false; }

	void SetHoursPerMinute( Float f ) { m_hoursPerMinute = f; }

public:
	void Reset();

	//! Resets the facts at the game start
	virtual void OnGameStart( const CGameInfo& info );

	//! Cleanup ( to save memory )
	virtual void OnGameEnd( const CGameInfo& gameInfo );

	//! Save to game save
	virtual bool OnSaveGame( IGameSaver* saver );

private:
	void LoadGame( IGameLoader* loader );
};
