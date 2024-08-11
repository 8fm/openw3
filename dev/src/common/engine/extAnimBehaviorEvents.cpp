/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "extAnimBehaviorEvents.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimComboEvent );

CExtAnimComboEvent::CExtAnimComboEvent()
	: CExtAnimDurationEvent()
{

}

CExtAnimComboEvent::CExtAnimComboEvent( const CName& eventName,
									    const CName& animationName, 
										Float startTime, 
										Float duration, 
										const String& trackName )
	: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
{

}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimHitEvent );

CExtAnimHitEvent::CExtAnimHitEvent()
	: CExtAnimEvent()
	, m_hitLevel( 0 )
{

}

CExtAnimHitEvent::CExtAnimHitEvent( const CName& eventName,
									const CName& animationName, 
									Float startTime, 
									const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_hitLevel( 0 )
{

}

Uint32 CExtAnimHitEvent::GetHitLevel() const
{
	return m_hitLevel;
}

void CExtAnimHitEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	/*CAnimatedComponent* ac = info.m_animatedComponent;
	if ( ac )
	{
		CEntity* entity = ac->GetEntity();

		// Call hit event
		// entity->CallEvent()
	}*/
}
