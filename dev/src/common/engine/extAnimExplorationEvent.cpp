/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "extAnimExplorationEvent.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimExplorationEvent );

CExtAnimExplorationEvent::CExtAnimExplorationEvent()
	: CExtAnimDurationEvent()
{
}

CExtAnimExplorationEvent::CExtAnimExplorationEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName )
	: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
{
}

CExtAnimExplorationEvent::~CExtAnimExplorationEvent()
{

}
