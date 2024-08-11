/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "extAnimCutsceneDialogEvent.h"
#include "actorInterface.h"
#include "cutsceneDebug.h"
#include "animatedComponent.h"
#include "animationEvent.h"
#include "skeletalAnimationSet.h"
#include "entityTemplate.h"
#include "entity.h"

#ifdef DEBUG_CUTSCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneDialogEvent );

CExtAnimCutsceneDialogEvent::CExtAnimCutsceneDialogEvent()
	: CExtAnimEvent()
{
	m_reportToScript = false;
}

CExtAnimCutsceneDialogEvent::CExtAnimCutsceneDialogEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
{
	m_reportToScript = false;
}

CExtAnimCutsceneDialogEvent::~CExtAnimCutsceneDialogEvent()
{

}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimDisableDialogLookatEvent );

CExtAnimDisableDialogLookatEvent::CExtAnimDisableDialogLookatEvent()
	: CExtAnimDurationEvent() , m_speed( 1.f )
{
	m_alwaysFiresEnd = true;
	m_reportToScript = false;
}

CExtAnimDisableDialogLookatEvent::CExtAnimDisableDialogLookatEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName )
	: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName ) , m_speed( 1.f )
{
	m_alwaysFiresEnd = true;
	m_reportToScript = false;
}

CExtAnimDisableDialogLookatEvent::~CExtAnimDisableDialogLookatEvent()
{

}

void CExtAnimDisableDialogLookatEvent::Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( component );
	if( component )
	{
		CEntity* ent = component->GetEntity();
		IActorInterface* act = ent->QueryActorInterface();
		if( act )
		{
			CName animName = info.m_animInfo.m_animation->GetName();
			act->ActivateLookatFilter( animName, true );
		}
	}
}

void CExtAnimDisableDialogLookatEvent::Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( component );
	if ( component )
	{
		CEntity* ent = component->GetEntity();
		IActorInterface* act = ent->QueryActorInterface();
		if( act )
		{
			CName animName = info.m_animInfo.m_animation->GetName();
			act->ActivateLookatFilter( animName, false );
		}
	}
}


//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneBreakEvent );

CExtAnimCutsceneBreakEvent::CExtAnimCutsceneBreakEvent()
	: CExtAnimEvent()
	, m_iAmHackDoNotUseMeInGame( true )
{
	m_reportToScript = false;
}

CExtAnimCutsceneBreakEvent::CExtAnimCutsceneBreakEvent( const CName& eventName,
	const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_iAmHackDoNotUseMeInGame( true )
{
	m_reportToScript = false;
}

CExtAnimCutsceneBreakEvent::~CExtAnimCutsceneBreakEvent()
{

}

#ifdef DEBUG_CUTSCENES
#pragma optimize("",on)
#endif
