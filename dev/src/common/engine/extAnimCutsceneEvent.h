#pragma once

#include "extAnimEvent.h"
#include "extAnimDurationEvent.h"

class CExtAnimCutsceneEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneEvent )
public:
	CExtAnimCutsceneEvent() {}
	CExtAnimCutsceneEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName )
		: CExtAnimEvent( eventName, animationName, startTime, trackName ) {}

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component, CCutsceneInstance* cs ) const {};
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneEvent )
	PARENT_CLASS( CExtAnimEvent )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CExtAnimCutsceneDurationEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneDurationEvent )

public:
	CExtAnimCutsceneDurationEvent() 
	{ 
		m_reportToScript = false; 
	}

	CExtAnimCutsceneDurationEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName )
		: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName ) 
	{ 
		m_reportToScript = false; 
	}

	virtual void StartEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const {};
	virtual void StopEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const {};
	virtual void ProcessEx( const CAnimationEventFired& info, CCutsceneInstance* cs ) const {};

	virtual void OnCutsceneFinish( CCutsceneInstance* cs ) const {}
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneDurationEvent )
	PARENT_CLASS( CExtAnimDurationEvent )
END_CLASS_RTTI()
