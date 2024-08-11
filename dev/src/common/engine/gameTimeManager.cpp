/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gameTimeManager.h"
#include "../core/gameSave.h"

RED_DEFINE_STATIC_NAME( timeScalePriorityIndexGenerator )

ITimeEvent::ITimeEvent( const THandle<IScriptable>& owner, GameTime date, GameTime period, Int32 limit, Bool relative )
	: m_limit( limit )
	, m_ownerIsNull( owner.Get() == NULL ? true : false )
	, m_owner( owner )
	, m_period( period )
{

	// Calculate invoke time
	if ( relative )
	{
		// Relative values - modify with current time
		m_date =  date + GGame->GetTimeManager()->GetTime();
	}
	else
	{
		// Absolute date
		m_date = date;
	}
}

ITimeEvent::~ITimeEvent()
{
}

void ITimeEvent::Repeat()
{
	ASSERT( m_period > 0 );

	m_date += m_period;
	if ( m_limit > 0 )
	{
		m_limit--;
		if ( !m_limit )
		{
			m_period = 0;
		}
	}
}

CTimeManager::CTimeManager()
	: m_timeScalePriorityIndexGenerator( 0 )
{
	Reset();
}

CTimeManager::~CTimeManager()
{
	m_events.ClearPtr();
}

void CTimeManager::Tick( Float timeDelta )
{
	if ( !IsPaused() )
	{
		m_lastTick = timeDelta;

		m_deltaAccum += timeDelta * m_hoursPerMinute * 60.f;

		m_time += (Uint32)MFloor( m_deltaAccum );
		m_deltaAccum = MFract( m_deltaAccum );

		// Advance events
		while ( !m_events.Empty() && m_events[0]->IsTime( m_time ) )
		{
			if ( ! m_events[0]->IsOwnerValid() )
			{
				delete m_events[0];
				m_events.Erase( m_events.Begin() );
				continue;
			}

			m_events[0]->OnEvent( NULL, CNAME( TimeEvent ), (void *) &m_time );
			if ( m_events[0]->IsPeriodic() )
			{
				m_events[0]->Repeat();
				m_events.Insert( m_events[0] );
			}
			else
			{
				delete m_events[0];
			}
			m_events.Erase( m_events.Begin() );
		}
	}
}

void CTimeManager::SetTime( GameTime time, Bool callEvents )
{
	m_time = time;
	m_deltaAccum = 0.f;

	if ( callEvents )
	{
		Tick( 0 );
	}
	else
	{
		while ( !m_events.Empty() && m_events[0]->IsTime( m_time ) )
		{
			// moving all events in the event queue without calling them
			// TODO: optimize; it will be a bit more technically sophisticated
			if ( m_events[0]->IsPeriodic() )
			{
				// Repeat event
				m_events[0]->Repeat();
				m_events.Insert( m_events[0] );
			}
			else
			{
				// Delete event for good
				delete m_events[0];
			}

			// Remove the head...
			m_events.Erase( m_events.Begin() );
		}
	}

	GGame->OnGameTimeChanged();
}

void CTimeManager::AddEvent( ITimeEvent *timeEvent )
{
	ASSERT( ::SIsMainThread() );

	ASSERT( timeEvent );
	m_events.Insert( timeEvent );
}

void CTimeManager::RemoveEvent( ITimeEvent *timeEvent )
{
	ASSERT( ::SIsMainThread() );

	m_events.RemoveFast( timeEvent );
}

void CTimeManager::RemoveEvents( const THandle<IScriptable>& owner )
{
	ASSERT( ::SIsMainThread() );

	// Grab events to remove
	TDynArray< ITimeEvent * > toDelete;
	for ( Uint32 i = 0; i < m_events.Size(); i++ )
	{
		if ( m_events[i]->GetOwner() == owner )
		{
			toDelete.PushBack(m_events[i]);
		}
	}

	// Remove events from time table
	for ( Uint32 i = 0; i < toDelete.Size(); i++ )
	{
		m_events.Remove( toDelete[i] );
	}

	// Remove objects
	toDelete.ClearPtr();
}

void CTimeManager::Reset()
{
	m_isPaused = false;
	m_time = GameTime( 0,0,0,0 );
	m_hoursPerMinute = 0.25f;

	m_deltaAccum = 0.f;
	m_lastTick = 0.f;

	m_timeScalePriorityIndexGenerator = 0;
}

void CTimeManager::OnGameStart( const CGameInfo& info )
{
	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );

	if ( info.IsNewGame() )
	{
		Reset();
	}

	if ( info.m_gameLoadStream )
	{
		LoadGame( info.m_gameLoadStream );
	}
	if ( info.m_isChangingWorldsInGame )
	{
		m_time += GameTime(2, 0, 0, 0);
	}
}

void CTimeManager::OnGameEnd( const CGameInfo& gameInfo )
{
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );
}

Bool CTimeManager::OnSaveGame( IGameSaver* saver )
{
	TIMER_BLOCK( time )

	CGameSaverBlock block( saver, CNAME(timeManger) );

	saver->WriteValue( CNAME(time), m_time );
	saver->WriteValue( CNAME(isPaused), m_isPaused );
	saver->WriteValue( CNAME(timeScalePriorityIndexGenerator), m_timeScalePriorityIndexGenerator );

	END_TIMER_BLOCK( time )

	// Done
	return true;
}

void CTimeManager::LoadGame( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME(timeManger) );

	loader->ReadValue( CNAME(time), m_time );
	loader->ReadValue( CNAME(isPaused), m_isPaused );
	loader->ReadValue( CNAME(timeScalePriorityIndexGenerator), m_timeScalePriorityIndexGenerator );
}

Bool CTimeManager::IsPaused() const
{ 
	return m_isPaused || GGame->IsPaused() || GGame->IsActivelyPaused(); 
}
