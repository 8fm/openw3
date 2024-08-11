/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "extAnimDurationEvent.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimDurationEvent );

CExtAnimDurationEvent::CExtAnimDurationEvent()
	 : CExtAnimEvent()
	 , m_duration( 0.0f )
	 , m_alwaysFiresEnd( false )
{
}

CExtAnimDurationEvent::CExtAnimDurationEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_duration( duration )
	, m_alwaysFiresEnd( false )
{
}

CExtAnimDurationEvent::~CExtAnimDurationEvent()
{

}

void CExtAnimDurationEvent::SetDuration( Float duration )
{
	m_duration = duration;
}
