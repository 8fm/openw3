/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entitiesDetector.h"
#include "../../common/game/gameplayStorage.h"

//////////////////////////////////////////////////////////////////////////

namespace
{

struct SListenersFilter
{
	SListenersFilter( CEntitiesDetector::TListeners& listeners )
		: m_listener( nullptr )
	{
		m_itBegin = listeners.Begin();
		m_itEnd = listeners.End();
		CEntitiesDetector::TListeners::iterator it = m_itBegin;
		for ( ; it != m_itEnd; ++it )
		{
			(*it)->m_listener->BeginEntitiesDetection();
		}
	}

	SListenersFilter( IEntitiesDetectorListener* listener )
		: m_listener( listener )
	{
		if ( m_listener != nullptr )
		{
			m_listener->BeginEntitiesDetection();
		}
	}

	~SListenersFilter()
	{
		CEntitiesDetector::TListeners::iterator it = m_itBegin;
		for ( ; it != m_itEnd; ++it )
		{
			(*it)->m_listener->EndEntitiesDetection();
		}
		if ( m_listener != nullptr )
		{
			m_listener->EndEntitiesDetection();
		}
	}

	RED_INLINE Bool operator()( const TPointerWrapper< CGameplayEntity >& ptr )
	{
		CEntitiesDetector::TListeners::iterator it = m_itBegin;
		for ( ; it != m_itEnd; ++it )
		{
			(*it)->m_listener->ProcessDetectedEntity( ptr.Get() );
		}
		if ( m_listener != nullptr )
		{
			m_listener->ProcessDetectedEntity( ptr.Get() );
		}
		return true;
	}

	SListenersFilter& operator=( const SListenersFilter& filter )
	{
		m_itBegin = filter.m_itBegin;
		m_itEnd = filter.m_itEnd;
		m_listener = filter.m_listener;
		return *this;
	}

	enum { SORT_OUTPUT = false };

private:

	CEntitiesDetector::TListeners::iterator	m_itBegin;
	CEntitiesDetector::TListeners::iterator	m_itEnd;
	IEntitiesDetectorListener*				m_listener;
};

}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CEntitiesDetector );

const Float CEntitiesDetector::UPDATE_RANGE = 30.0f;

CEntitiesDetector::CEntitiesDetector()
	: m_nextUpdate( EngineTime::ZERO )
	, m_minFrequency( 0.0f )
	, m_distanceChecker( UPDATE_RANGE )
{
}

CEntitiesDetector::~CEntitiesDetector()
{
	m_listeners.ClearPtr();
	m_newListeners.Clear();
}

void CEntitiesDetector::OnGameStart( const CGameInfo& gameInfo )
{
	m_nextUpdate = EngineTime::ZERO;
	m_minFrequency = 0.0f;
	m_distanceChecker.Reset();
}

void CEntitiesDetector::OnGameEnd( const CGameInfo& gameInfo )
{
	m_listeners.ClearPtr();
	m_newListeners.Clear();
}

void CEntitiesDetector::OnWorldStart( const CGameInfo& gameInfo )
{

}

void CEntitiesDetector::OnWorldEnd( const CGameInfo& gameInfo )
{

}

void CEntitiesDetector::Tick( Float timeDelta )
{
	PC_SCOPE_PIX( EntitiesDetector_Tick );

	if ( GCommonGame->GetPlayer() != nullptr )
	{
		Update();
	}
}

Bool CEntitiesDetector::Register( IEntitiesDetectorListener* listener, Float frequency, Float range, Bool explicitRange /* = false */ )
{
	RED_ASSERT( listener != nullptr );
	SListenerEntry* entry = FindListenerEntry( listener );
	m_minFrequency = m_minFrequency == 0.0f ? frequency : Min( m_minFrequency, frequency );
	if ( entry != nullptr )
	{
		entry->m_frequency = frequency;
		entry->m_nextUpdate = m_nextUpdate;
		entry->m_range = range;
		entry->m_explicitRange = explicitRange;
		return false;
	}
	SListenerEntry* newListener = new SListenerEntry( listener, frequency, m_nextUpdate, range, explicitRange );
	m_listeners.PushBack( newListener );
	m_newListeners.PushBack( newListener ); // for instant detection
	return true;
}

Bool CEntitiesDetector::Unregister( IEntitiesDetectorListener* listener )
{
	SListenerEntry* entry = FindListenerEntry( listener );
	if ( entry != nullptr )
	{
		m_listeners.Remove( entry );
		m_newListeners.Remove( entry );
		delete entry;
		UpdateMinFrequency();
		return true;
	}
	return false;
}

void CEntitiesDetector::Detect( IEntitiesDetectorListener* listener, Float range )
{
	if ( GCommonGame->GetPlayer() == nullptr )
	{
		return;
	}
	Vector playerPos = GCommonGame->GetPlayer()->GetWorldPosition();
	SListenersFilter filter( listener );
	Box box( Vector::ZEROS, range );
	GCommonGame->GetGameplayStorage()->TQuery( playerPos, filter, box, true, nullptr, 0 );
}

CEntitiesDetector::SListenerEntry* CEntitiesDetector::FindListenerEntry( IEntitiesDetectorListener* listener )
{
	TListeners::iterator it = m_listeners.Begin();
	TListeners::iterator itEnd = m_listeners.End();
	for ( ; it != itEnd; ++it )
	{
		if ( (*it)->m_listener == listener )
		{
			return *it;
		}
	}
	return nullptr;
}

void CEntitiesDetector::UpdateMinFrequency()
{
	m_minFrequency = 0.0f;
	TListeners::iterator it = m_listeners.Begin();
	TListeners::iterator itEnd = m_listeners.End();
	for ( ; it != itEnd; ++it )
	{
		m_minFrequency = ( m_minFrequency == 0.0f ) ? (*it)->m_frequency : Min( m_minFrequency, (*it)->m_frequency );
	}
}

void CEntitiesDetector::Update()
{
	if ( m_newListeners.Size() > 0 )
	{
		Update( m_newListeners, true, false );
		m_newListeners.Clear();
	}

	if ( m_listeners.Size() == 0 )
	{
		return;
	}

	bool forceUpdate = ( m_distanceChecker.ShouldUpdate() || m_nextUpdate == EngineTime::ZERO );
	EngineTime now = GGame->GetEngineTime();
	if ( !forceUpdate && now < m_nextUpdate )
	{
		return;
	}

	Update( m_listeners, forceUpdate, true );

	// setup next update
	RED_ASSERT( m_minFrequency != 0.0f );
	m_nextUpdate = now + m_minFrequency;
}

void CEntitiesDetector::Update( TListeners& listeners, Bool forceUpdate, Bool setupNextUpdate )
{
	EngineTime now = GGame->GetEngineTime();
	// collect entries that need to be updated
	TListeners sharedRangeListeners;
	THashMap< Float, TListeners > explicitRangeListeners;
	TListeners::iterator it = listeners.Begin();
	TListeners::iterator itEnd = listeners.End();
	Float maxRange = 0.0f;
	for ( ; it != itEnd; ++it )
	{
		SListenerEntry* entry = *it;
		if ( forceUpdate || Float( entry->m_nextUpdate - m_nextUpdate ) < 0.001f )
		{
			if ( entry->m_explicitRange )
			{
				explicitRangeListeners.GetRef( entry->m_range ).PushBack( entry );
			}
			else
			{
				sharedRangeListeners.PushBack( entry );
				maxRange = Max( maxRange, entry->m_range );
			}
			if ( setupNextUpdate )
			{
				entry->m_nextUpdate = now + entry->m_frequency;
			}
		}
	}

	Vector playerPos = GCommonGame->GetPlayer()->GetWorldPosition();

	// process shared range listeners
	{
		SListenersFilter filter( sharedRangeListeners );
		Box box( Vector::ZEROS, maxRange );
		GCommonGame->GetGameplayStorage()->TQuery( playerPos, filter, box, true, nullptr, 0 );
	}

	// process explicit range listeners
	THashMap< Float, TListeners >::iterator mit = explicitRangeListeners.Begin();
	THashMap< Float, TListeners >::iterator mitEnd = explicitRangeListeners.End();
	for ( ; mit != mitEnd; ++mit )
	{
		SListenersFilter filter( mit->m_second );
		Box box( Vector::ZEROS, mit->m_first );
		GCommonGame->GetGameplayStorage()->TQuery( playerPos, filter, box, true, nullptr, 0 );
	}
}

void CEntitiesDetector::OnGenerateDebugFragments( CRenderFrame* frame )
{
	// does nothing (for now)
}

//////////////////////////////////////////////////////////////////////////

CEntitiesDetector::SListenerEntry::SListenerEntry()
	: m_listener( nullptr )
	, m_frequency( 0.0f )
	, m_nextUpdate( EngineTime::ZERO )
	, m_range( 0.0f )
	, m_explicitRange( false )
{}

CEntitiesDetector::SListenerEntry::SListenerEntry( IEntitiesDetectorListener* listener, Float frequency, EngineTime nextUpdate, Float range, Bool explicitRange )
	: m_listener( listener )
	, m_frequency( frequency )
	, m_nextUpdate( nextUpdate )
	, m_range( range )
	, m_explicitRange( explicitRange )
{}
