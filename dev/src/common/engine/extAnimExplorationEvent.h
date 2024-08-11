/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "extAnimDurationEvent.h"

class CExtAnimExplorationEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimExplorationEvent )

public:
	CExtAnimExplorationEvent();

	CExtAnimExplorationEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName );
	virtual ~CExtAnimExplorationEvent();
};

BEGIN_CLASS_RTTI( CExtAnimExplorationEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
END_CLASS_RTTI();
