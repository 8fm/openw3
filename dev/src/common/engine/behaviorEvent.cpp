/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorEvent.h"
#include "behaviorGraph.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorEventDescription );

CBehaviorEventDescription::CBehaviorEventDescription()
	: m_isModifiableByEffect( false )
{
}

//////////////////////////////////////////////////////////////////////////

CBehaviorEventsList::CBehaviorEventsList()
	: m_behaviorGraph( NULL )
{
}

Uint32 CBehaviorEventsList::AddEvent( const CName& name )
{
	ASSERT( m_behaviorGraph );

	TContainerType::iterator it = FindEventByName( name );

	if ( it == m_events.End() )
	{
		CBehaviorEventDescription* newEventDesc = CreateObject< CBehaviorEventDescription >( m_behaviorGraph );
		newEventDesc->m_eventName = name;

		m_events.PushBack( newEventDesc );		

		return m_events.Size() - 1;
	}
	else
	{
		return PtrDiffToUint32( (void*)( it - m_events.Begin() ) );
	}
}

CBehaviorEventDescription* CBehaviorEventsList::GetEvent( const CName& name ) const
{
	TContainerType::const_iterator it = FindEventByName( name );
	if ( it == m_events.End() )
	{
		return NULL;
	}

	return (*it);
}


Uint32 CBehaviorEventsList::GetEventId( const CName& name ) const
{
	TContainerType::const_iterator it = FindEventByName( name );
	if ( it == m_events.End() )
	{
		return NO_EVENT;
	}

	return PtrDiffToUint32( (void*)( it - m_events.Begin() ) );
}

void CBehaviorEventsList::RemoveEvent( const CName& name )
{
	TContainerType::iterator it = FindEventByName( name );
	if ( it != m_events.End() )
	{
		m_events.Erase( it );
	}
}

Uint32 CBehaviorEventsList::GetNumEvents() const
{
	return m_events.Size();
}

void CBehaviorEventsList::GetEvents( TDynArray< CBehaviorEventDescription* >& events ) const
{
	for ( TContainerType::const_iterator it = m_events.Begin(); it != m_events.End(); ++it )
	{
		events.PushBack( *it );
	}
}

const CName& CBehaviorEventsList::GetEventName( Uint32 index ) const
{
	TContainerType::const_iterator it = m_events.Begin();
	it += index;
	return (*it)->GetEventName();
}

const CName& CBehaviorEventsList::FindEventNameById( Uint32 id ) const
{
	if ( m_events.Size() > id )
	{
		return m_events[ id ]->GetEventName();
	}
	else
	{
		ASSERT( m_events.Size() > id );

		return CName::NONE;
	}
}

CBehaviorEventDescription* CBehaviorEventsList::GetEvent( Uint32 index ) const
{
	TContainerType::const_iterator it = m_events.Begin();
	it += index;
	return (*it);
}

CBehaviorEventsList::TContainerType::const_iterator CBehaviorEventsList::FindEventByName( const CName& name ) const
{
	TContainerType::const_iterator it = m_events.Begin();

	while( it != m_events.End() )
	{
		if ( (*it)->GetEventName() == name )
			break;

		++it;
	}

	return it;
}

CBehaviorEventsList::TContainerType::iterator CBehaviorEventsList::FindEventByName( const CName& name )
{
	TContainerType::iterator it = m_events.Begin();

	while( it != m_events.End() )
	{
		if ( (*it)->GetEventName() == name )
			break;

		++it;
	}

	return it;
}

Uint32 CBehaviorEventsList::GetSize() const
{
	return static_cast< Uint32 >( sizeof( CBehaviorEventsList ) + m_events.DataSize() );
}

Bool CBehaviorEventsList::DuplicationTestName() const
{
	struct Local
	{
		static Bool TestFun( const CBehaviorEventDescription* e1, const CBehaviorEventDescription* e2 )
		{
			return ( e1->GetEventName() == e2->GetEventName() );
		}

		static Bool MessageFun( const CBehaviorEventDescription* e1, const CBehaviorEventDescription* e2 )
		{
			ERR_ENGINE( TXT("WARNING: Behavior events with duplicated names found: %s and %s"), e1->GetEventName().AsString().AsChar(), e2->GetEventName().AsString().AsChar() );
			return true;
		}
	};

	return DuplicationTest( &Local::TestFun, &Local::MessageFun );
}

Bool CBehaviorEventsList::DuplicationTest( FUNCTION_EVENT_EVENT testFun, FUNCTION_EVENT_EVENT messageFun ) const
{
	Bool duplicates = false;
	TContainerType::const_iterator it1;
	for( it1 = m_events.Begin(); it1 !=m_events.End(); ++it1 )
	{
		TContainerType::const_iterator it2;
		// Before it1
		for( it2 = m_events.Begin(); it2!=it1; ++it2 )
		{

			if( testFun( (*it1), (*it2) ) )
			{
				messageFun( (*it1), (*it2) );	
				duplicates = true;
			}
		}

		// After it1
		for( it2 = it1+1; it2!=m_events.End(); ++it2 )
		{
			if( testFun( (*it1), (*it2) ) )
			{
				messageFun( (*it1), (*it2) );
				duplicates = true;
			}
		}
	}

	return duplicates;
}

CBehaviorEventsList& CBehaviorEventsList::operator=( const CBehaviorEventsList& rhs )
{
	ASSERT( m_behaviorGraph );
	if ( !m_behaviorGraph )
	{
		return *this;
	}

	Uint32 rhsSize = rhs.m_events.Size();
	Uint32 size = m_events.Size();

	if ( size < rhsSize )
	{
		Uint32 count = rhsSize - size;
		for ( Uint32 i=0; i<count; ++i )
		{
			CBehaviorEventDescription* newEvent = CreateObject< CBehaviorEventDescription >( m_behaviorGraph );
			m_events.PushBack( newEvent );
		}
	}
	else if ( rhsSize < size )
	{
		Uint32 count = size - rhsSize;
		m_events.Resize( count );
	}

	size = m_events.Size();
	ASSERT( size == rhsSize );

	for ( Uint32 i=0; i<size; ++i )
	{
		CBehaviorEventDescription* rhsEvent = rhs.m_events[i];
		CBehaviorEventDescription* event = m_events[i];

		ASSERT( rhsEvent );
		ASSERT( event );

		event->Set( rhsEvent );
	}

	return *this;
}

IFile& operator<<( IFile &file, CBehaviorEventsList &list )
{
	file << list.m_behaviorGraph;
	file << list.m_events;

	return file;
}
