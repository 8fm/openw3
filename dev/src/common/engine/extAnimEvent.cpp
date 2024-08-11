/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../core/fileSkipableBlock.h"
#include "extAnimEvent.h"
#include "animatedComponent.h"
#include "animationEvent.h"
#include "extAnimDurationEvent.h"
#include "extAnimBehaviorEvents.h"
#include "entity.h"
#include "../core/listener.h"

CExtAnimEvent::StatsCollector CExtAnimEvent::StatsCollector::s_statsCollector;

CExtAnimEvent::StatsCollector& CExtAnimEvent::StatsCollector::GetInstance()
{
	return s_statsCollector;
}

void CExtAnimEvent::StatsCollector::Reset()
{
	m_totalEventTimeMS = 0.0f;
	m_stats.ClearFast();
}

void CExtAnimEvent::StatsCollector::OnEvent( CName name, Float timeMS )
{
	EventStats& stats = m_stats.GetRef( name, EventStats( name ) );
	++stats.m_count;
	stats.m_timeMS += timeMS;

	m_totalEventTimeMS += timeMS;
}

IMPLEMENT_ENGINE_CLASS( CExtAnimEvent );

CExtAnimEvent::CExtAnimEvent()
	: m_eventName( CName::NONE )
	, m_startTime( 0.0f )
	, m_reportToScript( true )
	, m_reportToScriptMinWeight( 0.0f )
#ifdef USE_EXT_ANIM_EVENTS
	, m_animationName( CName::NONE )
#endif
#ifndef NO_EDITOR
	, m_trackName()
#endif
{
	
}

CExtAnimEvent::CExtAnimEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName )
	: m_eventName( eventName )
	, m_startTime( startTime )
	, m_reportToScript( true )
	, m_reportToScriptMinWeight( 0.0f )
#ifdef USE_EXT_ANIM_EVENTS
	, m_animationName( animationName )
#endif
#ifndef NO_EDITOR
	, m_trackName( trackName )
#endif
{
	
}

CExtAnimEvent::~CExtAnimEvent()
{

}

void CExtAnimEvent::SetEventName( const CName& name )
{ 
	m_eventName = name;
	CheckReportToScriptFlag();
}

void CExtAnimEvent::SetStartTime( Float startTime )
{
	m_startTime = startTime;
	// TODO:
	//MarkModified();
}

#ifdef USE_EXT_ANIM_EVENTS

void CExtAnimEvent::SetAnimationName( const CName& animationName )
{
	m_animationName = animationName;
	// TODO:
	//MarkModified();
}

#endif

 void CExtAnimEvent::Process( const CAnimationEventFired& currEvent, CAnimatedComponent* component ) const
 {
 	CEventNotifier< CAnimationEventFired >* eventNotifier = component->GetAnimationEventNotifier( m_eventName );
 	if ( eventNotifier )
 	{
 		eventNotifier->BroadcastEvent( currEvent );
 	}
 }

void CExtAnimEvent::Serialize( TDynArray< CExtAnimEvent* >& m_events, IFile& file )
{
	// GC
	if ( file.IsGarbageCollector() )
	{
		/*
		for( TDynArray< CExtAnimEvent* >::iterator eventIter = m_events.Begin(); eventIter != m_events.End(); ++eventIter )
		{
			CExtAnimEvent* event = *eventIter;
			event->GetClass()->Serialize( file, event );
		}
		*/
		return;
	}

	// Write to file
	if( file.IsWriter() )
	{
		// Write number of events
		Uint32 numOfEvents = m_events.Size();
		file << numOfEvents;

		// Serialize events
		for( TDynArray< CExtAnimEvent* >::iterator eventIter = m_events.Begin();
			eventIter != m_events.End(); ++eventIter )
		{
			CExtAnimEvent* event = *eventIter;

			CFileSkipableBlock block( file );

			// Serialize type
			CName type = event->GetClass()->GetName();
			file << type;

			// Serialize object
			event->GetClass()->Serialize( file, event );
		}
	}
	// Read from file
	else if ( file.IsReader() )
	{
		// Read number of events
		Uint32 serializedEvents;
		file << serializedEvents;
		m_events.Reserve( serializedEvents );

		// Read events
		for( Uint32 i = 0; i < serializedEvents; ++i )
		{
			CFileSkipableBlock block( file );

			// Read type
			CName eventType;
			file << eventType;

			// Find type in RTTI
			CClass* theClass = SRTTI::GetInstance().FindClass( eventType );
			if( theClass == NULL )
			{
				//WARN_ENGINE( TXT( "Unregistered RTTI type '%ls' failed" ), eventType.AsChar() );
				block.Skip();
				continue;
			}

			// Create object
			RED_DISABLE_WARNING_MSC( 4344 )
			CExtAnimEvent* event = theClass->CreateObject< CExtAnimEvent >();
			ASSERT( event != NULL );

			// Serialize object
			theClass->Serialize( file, event );

			// Add to the list
			m_events.PushBack( event );
		}
	}
}

void CExtAnimEvent::CheckReportToScriptFlag()
{
	const CClass* c = GetClass();

	if ( c == CExtAnimEvent::GetStaticClass() || c == CExtAnimDurationEvent::GetStaticClass() )
	{
		m_reportToScript = !( m_eventName == CNAME( FootLeft ) || m_eventName == CNAME( FootRight ) || m_eventName == CNAME( LSync ) );
	}
	else if ( c == CExtAnimHitEvent::GetStaticClass() )
	{
		m_reportToScript = true;
	}
	else
	{
		m_reportToScript = false;
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CAnimEventSerializer );

CAnimEventSerializer::CAnimEventSerializer()
{

}

CAnimEventSerializer::~CAnimEventSerializer()
{
	/*
	// Delete events
	for( TSortedArray< CExtAnimEvent*, CExtAnimEvent::Sorter >::iterator eventIter = m_events.Begin();
		eventIter != m_events.End(); ++eventIter )
	{
		delete *eventIter;
	}
	*/
}

void CAnimEventSerializer::OnSerialize( IFile& file )
{
	CExtAnimEvent::Serialize( m_events, file );
}
