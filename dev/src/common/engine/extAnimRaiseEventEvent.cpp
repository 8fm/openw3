/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "extAnimRaiseEventEvent.h"
#include "behaviorGraphStack.h"
#include "animatedComponent.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimRaiseEventEvent );

CExtAnimRaiseEventEvent::CExtAnimRaiseEventEvent()
	: CExtAnimEvent()
	, m_eventToBeRaisedName( CName::NONE )
	, m_forceRaiseEvent( false )
{
	m_reportToScript = false;
}

CExtAnimRaiseEventEvent::CExtAnimRaiseEventEvent( const CName& eventName,
												  const CName& animationName, Float startTime, const String& trackName,
												  const CName& eventToBeRaisedName, Bool forceRaiseEvent )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_eventToBeRaisedName( eventToBeRaisedName )
	, m_forceRaiseEvent( forceRaiseEvent )
{
	m_reportToScript = false;
}

CExtAnimRaiseEventEvent::~CExtAnimRaiseEventEvent()
{
}

void CExtAnimRaiseEventEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	if ( CBehaviorGraphStack* behaviorGraphStack = component->GetBehaviorStack() )
	{
		if ( m_forceRaiseEvent )
		{
			behaviorGraphStack->GenerateBehaviorForceEvent( m_eventToBeRaisedName );
		}
		else
		{
			behaviorGraphStack->GenerateBehaviorEvent( m_eventToBeRaisedName );
		}
	}
}


