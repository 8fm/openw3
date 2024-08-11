/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "extAnimEvent.h"

class CExtAnimCutsceneFadeEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneFadeEvent )

public:
	CExtAnimCutsceneFadeEvent();

	CExtAnimCutsceneFadeEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName );
	virtual ~CExtAnimCutsceneFadeEvent();

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

private:
	Bool	m_in;
	Float	m_duration;
	Color	m_color;
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneFadeEvent )
	PARENT_CLASS( CExtAnimEvent )
	PROPERTY_EDIT( m_in, TXT( "Fade in or out" ) )
	PROPERTY_EDIT( m_duration, TXT( "Duration of fade" ) )
	PROPERTY_INLINED( m_color, TXT( "Color" ) )
END_CLASS_RTTI()
