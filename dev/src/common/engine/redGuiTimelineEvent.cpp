/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiTimelineEvent.h"

namespace RedGui
{

	CRedGuiTimelineEvent::CRedGuiTimelineEvent( const String& name, Float startTime, const Color& color )
		: m_name( name )
		, m_startTime( startTime )
		, m_color( color )
		, m_duration( -1.0f )
	{
		/* intentionally empty */
	}

	CRedGuiTimelineEvent::~CRedGuiTimelineEvent()
	{
		/* intentionally empty */
	}

	void CRedGuiTimelineEvent::SetStopTime( Float stopTime )
	{
		m_duration = stopTime - m_startTime;
	}

	Float CRedGuiTimelineEvent::GetStartTime() const
	{
		return m_startTime;
	}

	Float CRedGuiTimelineEvent::GetDuration() const
	{
		return m_duration;
	}

	String CRedGuiTimelineEvent::GetName() const
	{
		return m_name;
	}

	Color CRedGuiTimelineEvent::GetColor() const
	{
		return m_color;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
