/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/globalEventsManager.h"
#include "../engine/tagManager.h"

enum EEntityWatcherStateChange
{
	EWST_NothingChanged,
	EWST_EntityFound,
	EWST_EntityLost,
};

template < typename T >
class TEntityWatcher : public IGlobalEventsListener
{
public:

	TEntityWatcher( const CName& tag );
	~TEntityWatcher();

	EEntityWatcherStateChange Enable( Bool enable );
	EEntityWatcherStateChange Update();

	const CName& GetTag()
	{
		return m_tag;
	}

	T* Get()
	{
		return m_handle.Get();
	}

	EEntityWatcherStateChange GetState()
	{
		return m_state;
	}

	// IGlobalEventsListener
	void OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param ) override;

	// IterateTaggedNodes interface
	RED_INLINE Bool EarlyTest( CNode* node )
	{
		return ( m_handle == nullptr && node != nullptr );
	}

	RED_INLINE void Process( CNode* node, Bool isGuaranteedUnique )
	{
		m_handle = Cast< T >( node );
	}

private:

	CName						m_tag;
	TagList						m_tagList;
	THandle< T >				m_handle;
	EEntityWatcherStateChange	m_state;
	Bool						m_entityFound;
	Bool						m_wasRegistered;

	void UpdateEntity();
	Bool RegisterCallback( Bool reg );
};

///////////////////////////////////////////////////////////////

template < typename T >
TEntityWatcher< T >::TEntityWatcher( const CName& tag )
	: m_tag( tag )
	, m_tagList( tag )
	, m_handle( nullptr )
	, m_state( EWST_NothingChanged )
	, m_entityFound( false )
	, m_wasRegistered( false )
{}

template < typename T >
TEntityWatcher< T >::~TEntityWatcher()
{
	RegisterCallback( false );
};

template < typename T >
EEntityWatcherStateChange TEntityWatcher< T >::Enable( Bool enable )
{
	m_entityFound = false;
	if ( enable )
	{
		UpdateEntity();
		Update();
	}
	else
	{
		RegisterCallback( false );
		m_handle = nullptr;
		if ( m_entityFound )
		{
			m_entityFound = false;
			m_state = EWST_EntityLost;			
		}
		else
		{
			m_state = EWST_NothingChanged;
		}
	}
	return m_state;
}

template < typename T >
EEntityWatcherStateChange TEntityWatcher< T >::Update()
{
	if ( m_handle.Get() != nullptr )
	{
		if ( m_entityFound )
		{
			m_state = EWST_NothingChanged;
		}
		else
		{
			m_entityFound = true;
			m_state = EWST_EntityFound;
		}
		if ( m_wasRegistered )
		{
			RegisterCallback( false );
		}
	}
	else
	{
		if ( m_entityFound )
		{
			m_entityFound = false;
			m_state = EWST_EntityLost;
		}
		else
		{
			m_state = EWST_NothingChanged;
		}
		if ( !m_wasRegistered )
		{
			RegisterCallback( true );
		}
	}
	return m_state;
}

template < typename T >
void TEntityWatcher< T >::OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param )
{
	if ( eventCategory == GEC_Tag && param.Get< CName >() == m_tag )
	{
		UpdateEntity();
	}
}

template < typename T >
void TEntityWatcher< T >::UpdateEntity()
{
	m_handle = nullptr;

	if ( m_tag == CName::NONE || GGame->GetActiveWorld() == nullptr )
	{
		return;
	}

	GGame->GetActiveWorld()->GetTagManager()->IterateTaggedNodes( m_tagList, *this );
}

template < typename T >
Bool TEntityWatcher< T >::RegisterCallback( Bool reg )
{
	if ( reg != m_wasRegistered && GGame != nullptr && GGame->GetGlobalEventsManager() != nullptr )
	{
		if ( reg )
		{
			UpdateEntity();
			GGame->GetGlobalEventsManager()->AddFilteredListener( GEC_Tag, this, m_tag );
		}
		else
		{
			GGame->GetGlobalEventsManager()->RemoveFilteredListener( GEC_Tag, this, m_tag );
		}
		m_wasRegistered = reg;
		return true;
	}
	return false;
}
