/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "globalEventsManager.h"
#include "game.h"

IMPLEMENT_RTTI_ENUM( EGlobalEventCategory );
IMPLEMENT_RTTI_ENUM( EGlobalEventType );

//////////////////////////////////////////////////////////////////////////

CGlobalEventsManager::CListeners::CListeners()
	: m_areListening( false )
{
}

CGlobalEventsManager::CListeners::~CListeners()
{
	Clear();
}

Bool CGlobalEventsManager::CListeners::Add( IGlobalEventsListener* listener )
{
	// if not listening let's to it easy way
	if ( !m_areListening )
	{
		return m_listeners.PushBackUnique( listener );
	}

	m_listenersToRemove.Remove( listener );
	return m_listenersToAdd.PushBackUnique( listener );
}

Bool CGlobalEventsManager::CListeners::Remove( IGlobalEventsListener* listener )
{
	// if not listening let's to it easy way
	if ( !m_areListening )
	{
		return m_listeners.Remove( listener );
	}

	m_listenersToAdd.Remove( listener );
	return m_listenersToRemove.PushBackUnique( listener );
}

Bool CGlobalEventsManager::CListeners::ProcessAddAndRemove()
{
	RED_ASSERT( !m_areListening );
	if ( m_listenersToAdd.Size() == 0 && m_listenersToRemove.Size() == 0 )
	{
		return false;
	}
	m_listeners.PushBack( m_listenersToAdd );
	for ( IGlobalEventsListener* listener : m_listenersToRemove )
	{
		m_listeners.Remove( listener );
	}
	m_listenersToAdd.ClearFast();
	m_listenersToRemove.ClearFast();
	return true;
}

void CGlobalEventsManager::CListeners::SetAreListening( Bool areListening )
{
	m_areListening = areListening;
}

void CGlobalEventsManager::CListeners::Clear()
{
	m_listeners.ClearFast();
	m_listenersToAdd.ClearFast();
	m_listenersToRemove.ClearFast();
}

Uint32 CGlobalEventsManager::CListeners::Size( Bool all ) const
{
	if ( !all )
	{
		return m_listeners.Size();
	}
	RED_ASSERT( m_listeners.Size() + m_listenersToAdd.Size() >= m_listenersToRemove.Size() );
	return m_listeners.Size() + m_listenersToAdd.Size() - m_listenersToRemove.Size();
}

Bool CGlobalEventsManager::CListeners::Exist( IGlobalEventsListener* listener, Bool all ) const
{
	if ( !all )
	{
		return m_listeners.Exist( listener );
	}
	return ( ( m_listeners.Exist( listener ) || m_listenersToAdd.Exist( listener ) ) && !m_listenersToRemove.Exist( listener ) );
}

//////////////////////////////////////////////////////////////////////////

CGlobalEventsManager::CGlobalEventsManager()
{
	m_listenersByCategory.Resize( static_cast< size_t >( GEC_Last ) );
	m_filteredListenersByCategory.Resize( static_cast< size_t >( GEC_Last ) );
}

CGlobalEventsManager::~CGlobalEventsManager()
{
	m_filterSelector.CallAll< void, CallClear >();
	for ( Uint32 i = 0; i < m_listenersByCategory.Size(); i++)
	{
		m_listenersByCategory[i].Clear();
	}
	m_listenersByCategory.ClearFast();
	for ( Uint32 i = 0; i < m_filteredListenersByCategory.Size(); i++)
	{
		m_filteredListenersByCategory[i].Clear();
	}
	m_filteredListenersByCategory.ClearFast();
	m_reporters.ClearFast();
}

Bool CGlobalEventsManager::AddListener( EGlobalEventCategory eventCategory, IGlobalEventsListener* listener )
{
	Int32 index = static_cast< Int32 >( eventCategory );
	if ( m_listenersByCategory[ index ].Add( listener ) )
	{
		// if we're adding the first listener -> update reporters state for this category
		if ( m_listenersByCategory[ index ].Size( true ) == 1 && m_filteredListenersByCategory[ index ].Size( true ) == 0 )
		{
			ReportCategoryActivated( eventCategory );
		}
		return true;
	}
	return false;
}

Bool CGlobalEventsManager::RemoveListener( EGlobalEventCategory eventCategory, IGlobalEventsListener* listener )
{
	Int32 index = static_cast< Int32 >( eventCategory );
	if ( m_listenersByCategory[ index ].Remove( listener ) )
	{
		// if we're removing the last listener -> update reporters state for this category
		if ( m_listenersByCategory[ index ].Size( true ) == 0 && m_filteredListenersByCategory[ index ].Size( true ) == 0 )
		{
			ReportCategoryDeactivated( eventCategory );
		}
		return true;
	}
	return false;
}

Bool CGlobalEventsManager::AddReporter( CGlobalEventsReporter* reporter )
{
	Bool res = m_reporters.PushBackUnique( reporter );
	EGlobalEventCategory category = reporter->m_category;
	if ( m_listenersByCategory[ category ].Size( true ) > 0 || m_filteredListenersByCategory[ category ].Size( true ) > 0 )
	{
		reporter->OnCategoryActivated( category );
	}
	return res;
}

Bool CGlobalEventsManager::RemoveReporter( CGlobalEventsReporter* reporter )
{
	return m_reporters.Remove( reporter );
}

void CGlobalEventsManager::ReportCategoryActivated( EGlobalEventCategory eventCategory )
{
	TReporters::iterator itEnd = m_reporters.End();
	for ( TReporters::iterator it = m_reporters.Begin(); it != itEnd; ++it )
	{
		(*it)->OnCategoryActivated( eventCategory );
	}
}

void CGlobalEventsManager::ReportCategoryDeactivated( EGlobalEventCategory eventCategory )
{
	TReporters::iterator itEnd = m_reporters.End();
	for ( TReporters::iterator it = m_reporters.Begin(); it != itEnd; ++it )
	{
		(*it)->OnCategoryDeactivated( eventCategory );
	}
}

//////////////////////////////////////////////////////////////////////////

CGlobalEventsReporter::CGlobalEventsReporter( EGlobalEventCategory eventCategory )
	: m_category( eventCategory )
	, m_registered( false )
	, m_active( false )
{
	Register();
}

CGlobalEventsReporter::~CGlobalEventsReporter()
{
	if ( m_registered && GGame != nullptr && GGame->GetGlobalEventsManager() != nullptr )
	{
		GGame->GetGlobalEventsManager()->RemoveReporter( this );
	}
}

void CGlobalEventsReporter::Register()
{
	if ( GGame != nullptr && GGame->GetGlobalEventsManager() != nullptr )
	{
		GGame->GetGlobalEventsManager()->AddReporter( this );
		m_registered = true;
	}
}

void CGlobalEventsReporter::OnCategoryActivated( EGlobalEventCategory eventCategory )
{
	if ( eventCategory == m_category )
	{
		m_active = true;
	}
}

void CGlobalEventsReporter::OnCategoryDeactivated( EGlobalEventCategory eventCategory )
{
	if ( eventCategory == m_category )
	{
		m_active = false;
	}
}

CGlobalEventsManager* CGlobalEventsReporter::GetGlobalEventsManager() const
{
	if ( GGame != nullptr )
	{
		return GGame->GetGlobalEventsManager();
	}
	return nullptr;
}