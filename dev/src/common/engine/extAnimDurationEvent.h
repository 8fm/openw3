/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "extAnimEvent.h"

class CExtAnimDurationEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimDurationEvent )

public:
	CExtAnimDurationEvent();

	CExtAnimDurationEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName );
	virtual ~CExtAnimDurationEvent();

	virtual Float GetEndTimeWithoutClamp() const
	{ return m_startTime + GetDuration(); }

	RED_INLINE Float GetDuration() const
	{ return m_duration; }

	virtual Float VGetDuration() const override
	{ return GetDuration(); }

	RED_INLINE Bool AlwaysFiresEnd() const
	{ return m_alwaysFiresEnd; }

	void SetDuration( Float duration );

	virtual void Start( const CAnimationEventFired& /*info*/, CAnimatedComponent* /*component*/ ) const {}
	virtual void Stop( const CAnimationEventFired& /*info*/, CAnimatedComponent* /*component*/ ) const {}
	
protected:
	Float		m_duration;
	Bool		m_alwaysFiresEnd;
};

BEGIN_CLASS_RTTI( CExtAnimDurationEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_EDIT( m_duration, TXT( "Duration" ) );
	PROPERTY_EDIT( m_alwaysFiresEnd, TXT( "Duration end event is fired always, even if animation is interrupted" ) );
END_CLASS_RTTI();
