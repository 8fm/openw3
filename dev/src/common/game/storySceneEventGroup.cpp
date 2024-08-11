/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "storySceneEventGroup.h"
#include "../core/fileSkipableBlock.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( SStorySceneEventGroupEntry );
IMPLEMENT_ENGINE_CLASS( CStorySceneEventGroup );

CStorySceneEventGroup::CStorySceneEventGroup()
{

}

CStorySceneEventGroup::CStorySceneEventGroup( const String& eventName, CStorySceneElement* sceneElement, const String& trackName, Float startTime, Float duration /*= 2.0f */ )
	: CStorySceneEventDuration( eventName, sceneElement, startTime, duration, trackName )
{

}

/*
Cctor.
*/
CStorySceneEventGroup::CStorySceneEventGroup( const CStorySceneEventGroup& other )
{
	RED_FATAL( "CStorySceneEventGroup: cctor called - we have no intention of supporting CStorySceneEventGroup anymore." );
}

CStorySceneEventGroup* CStorySceneEventGroup::Clone() const 
{
	RED_FATAL( "CStorySceneEventGroup: Clone() called - we have no intention of supporting CStorySceneEventGroup anymore." );
	return nullptr;
}

void CStorySceneEventGroup::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	const Uint32 size = m_events.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		SStorySceneEventGroupEntry& entry = m_events[ i ];
		if ( entry.m_event )
		{
			entry.m_event->OnBuildDataLayout( compiler );
		}
	}
}

void CStorySceneEventGroup::OnInitInstance( InstanceBuffer& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	const Uint32 size = m_events.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const SStorySceneEventGroupEntry& entry = m_events[ i ];
		if ( entry.m_event )
		{
			entry.m_event->OnInitInstance( instance );
		}
	}
}

void CStorySceneEventGroup::OnReleaseInstance( InstanceBuffer& instance ) const
{
	TBaseClass::OnReleaseInstance( instance );

	const Uint32 size = m_events.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const SStorySceneEventGroupEntry& entry = m_events[ i ];
		if ( entry.m_event )
		{
			entry.m_event->OnReleaseInstance( instance );
		}
	}
}

void CStorySceneEventGroup::OnInit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const
{
	TBaseClass::OnInit( data, scenePlayer );

	const Uint32 size = m_events.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const SStorySceneEventGroupEntry& entry = m_events[ i ];
		if ( entry.m_event )
		{
			entry.m_event->OnInit( data, scenePlayer );
		}
	}
}

void CStorySceneEventGroup::OnDeinit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const
{
	TBaseClass::OnDeinit( data, scenePlayer );

	const Uint32 size = m_events.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const SStorySceneEventGroupEntry& entry = m_events[ i ];
		if ( entry.m_event )
		{
			entry.m_event->OnDeinit( data, scenePlayer );
		}
	}
}

void CStorySceneEventGroup::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );

	const Float eventTime = timeInfo.m_timeLocal;
	const Float timeDelta = timeInfo.m_timeDelta;

	// DIALOG_TOMSIN_TODO - to do wywalenia i przerobienia na parentowanie eventow miedzy soba
	for( TDynArray< SStorySceneEventGroupEntry >::const_iterator eventIter = m_events.Begin(); eventIter != m_events.End(); ++eventIter )
	{
		CStorySceneEvent* embeddedEvent = eventIter->m_event;
		Float	embeddedEventStartTime = eventIter->m_time;
		Float	embeddedEventEndTime = eventIter->m_time + embeddedEvent->GetInstanceDuration( data );

		if ( eventTime - timeDelta <= embeddedEventStartTime && eventTime >= embeddedEventStartTime )
		{
			SStorySceneEventTimeInfo info;
			info.m_timeLocal = eventTime - embeddedEventStartTime;
			info.m_timeAbs = eventTime;
			info.m_progress = 0.f;
			info.m_timeDelta = timeDelta;

			embeddedEvent->OnStart( data, scenePlayer, collector, info );
		}
		else if ( eventTime > embeddedEventEndTime && eventTime - timeDelta <= embeddedEventEndTime )
		{
			SStorySceneEventTimeInfo info;
			info.m_timeLocal = eventTime - embeddedEventStartTime;
			info.m_timeAbs = eventTime;
			info.m_progress = 0.f;
			info.m_timeDelta = timeDelta;

			embeddedEvent->OnEnd( data, scenePlayer, collector, info );
			continue;
		}
		
		if ( eventTime >= embeddedEventStartTime && eventTime < embeddedEventEndTime )
		{
			SStorySceneEventTimeInfo info;
			info.m_timeLocal = eventTime - embeddedEventStartTime;
			info.m_timeAbs = eventTime;
			info.m_progress = 0.f;
			info.m_timeDelta = timeDelta;

			embeddedEvent->OnProcess( data, scenePlayer, collector, info );
		}
	}
}

void CStorySceneEventGroup::AddEvent( CStorySceneEvent* event, Float time )
{
	m_events.PushBack( SStorySceneEventGroupEntry( time, event ) );
}

void CStorySceneEventGroup::Serialize( IFile& file )
{
	TBaseClass::Serialize( file );

	// Basicaly a copy of event serialization from StorySceneSection with time offsets added

	// Write to file
	if( file.IsWriter() )
	{
		// Write number of events
		Uint32 numberOfEvents = m_events.Size();
		file << numberOfEvents;

		// Serialize events
		for( TDynArray< SStorySceneEventGroupEntry >::iterator eventIter = m_events.Begin();
			eventIter != m_events.End(); ++eventIter )
		{
			CStorySceneEvent* event = eventIter->m_event;
			Float eventTime = eventIter->m_time;
			CName eventType = event->GetClass()->GetName();

			CFileSkipableBlock block( file );

			file << eventTime;

			// Serialize type
			file << eventType;

			// Serialize object
			event->Serialize( file );
		}
	}
	// Read from file
	else if ( file.IsReader() )
	{
		// Read number of events
		Uint32 numberOfEvents;
		file << numberOfEvents;

		// Read events
		for( Uint32 i = 0; i < numberOfEvents; ++i )
		{
			CFileSkipableBlock block( file );

			Float eventTime;
			file << eventTime;

			// Read type
			CName eventType;
			file << eventType;

			// Find type in RTTI
			CClass* theClass = SRTTI::GetInstance().FindClass( eventType );
			if( theClass == NULL )
			{
				WARN_GAME( TXT( "Loading of embedded dialog event '%ls' failed" ), eventType.AsString().AsChar() );
				block.Skip();
				continue;
			}

			// Create object
			RED_DISABLE_WARNING_MSC( 4344 ) // use of explicit template arguments results in call...
			CStorySceneEvent* event = theClass->CreateObject< CStorySceneEvent >();
			ASSERT( event != NULL );

			// Serialize object
			event->Serialize( file );

			// Add to the list
			m_events.PushBack( SStorySceneEventGroupEntry( eventTime, event ) );
		}
	}
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
