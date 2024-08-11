/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "extAnimEvent.h"

class CExtAnimRaiseEventEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimRaiseEventEvent )

public:
	CExtAnimRaiseEventEvent();

	CExtAnimRaiseEventEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName,
		const CName& eventToBeRaisedName = CName::NONE, Bool forceEvent = false);

	virtual ~CExtAnimRaiseEventEvent();
	
	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

protected:
	CName		m_eventToBeRaisedName;	//!< event name to be raised
	Bool		m_forceRaiseEvent;		//!< raise as force event
};

BEGIN_CLASS_RTTI( CExtAnimRaiseEventEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_EDIT( m_eventToBeRaisedName, TXT( "Event to be raised" ) );
	PROPERTY_EDIT( m_forceRaiseEvent, TXT( "Raise as force event" ) );
END_CLASS_RTTI();
