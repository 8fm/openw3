/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiTimeline.h"

namespace RedGui
{

	CRedGuiTimeline::CRedGuiTimeline( const String& name ) 
		: m_name( name )
	{
		/* intentionally empty */
	}

	CRedGuiTimeline::~CRedGuiTimeline()
	{
		/* intentionally empty */
	}

	void CRedGuiTimeline::SetName( const String& name )
	{
		m_name = name;
	}

	String CRedGuiTimeline::GetName() const
	{
		return m_name;
	}

	void CRedGuiTimeline::StartEvent( const String& eventName, Float startTime )
	{
		m_events.PushBack( new CRedGuiTimelineEvent( eventName, startTime, Color::LIGHT_BLUE ) );
	}

	void CRedGuiTimeline::StopEvent( const String& eventName, Float stopTime )
	{
		IRedGuiTimelineEvent* timelineEvent = FindEvent( eventName );
		if( timelineEvent != nullptr )
		{
			timelineEvent->SetStopTime( stopTime );
		}
	}

	void CRedGuiTimeline::AddEvent( const String& eventName, Float startTime, Float stopTime, Color color )
	{
		CRedGuiTimelineEvent * newEvent(  new CRedGuiTimelineEvent( eventName, startTime, color ) );
		newEvent->SetStopTime( stopTime );
		m_events.PushBack( newEvent );
	}

	void CRedGuiTimeline::GetEvents( Float time, TDynArray< const IRedGuiTimelineEvent* >& events ) const
	{
		for( Uint32 i=0; i<m_events.Size(); ++i )
		{
			if( m_events[i]->GetStartTime() > time )
			{
				events.PushBack( m_events[i] );
			}
		}
	}

	Uint32 CRedGuiTimeline::GetEventCount() const
	{
		return m_events.Size();
	}

	IRedGuiTimelineEvent* CRedGuiTimeline::GetEvent( Uint32 index ) const
	{
		return m_events[ index ];
	}

	void CRedGuiTimeline::ClearData()
	{
		m_events.ClearPtr();
	}

	void CRedGuiTimeline::ClearData( Uint32 amountOfFirstElements )
	{
		auto i = m_events.Begin();
		amountOfFirstElements = Min( amountOfFirstElements, m_events.Size() );

		i += amountOfFirstElements;

		for( auto iter = m_events.Begin(); iter != i; ++iter )
		{
			delete (*iter);
		}

		m_events.Erase( m_events.Begin(), i );
	}

	IRedGuiTimelineEvent* CRedGuiTimeline::FindEvent( const String& name )
	{
		Uint32 eventCount = m_events.Size();
		for( Uint32 i=0; i<eventCount; ++i )
		{
			if( m_events[i]->GetName() == name )
			{
				return m_events[i];
			}
		}
		return nullptr;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
