/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "extAnimCutsceneActorEffect.h"
#include "extAnimCutsceneEffectEvent.h"
#include "cutsceneDebug.h"
#include "animatedComponent.h"
#include "cutscene.h"
#include "entity.h"
#include "fxDefinition.h"

#ifdef DEBUG_CUTSCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneActorEffect );

CExtAnimCutsceneActorEffect::CExtAnimCutsceneActorEffect()
	 : CExtAnimDurationEvent()
	 , m_effectName( CName::NONE )
{
	m_reportToScript = false;
}

CExtAnimCutsceneActorEffect::CExtAnimCutsceneActorEffect( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const CName& effectName,
		const String& trackName )
	: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
	, m_effectName( effectName )
{
	m_reportToScript = false;
}

CExtAnimCutsceneActorEffect::~CExtAnimCutsceneActorEffect()
{

}

void CExtAnimCutsceneActorEffect::Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CEntity* entity = component->GetEntity();

	if ( CCutsceneTemplate* templ = Cast< CCutsceneTemplate >( info.m_animInfo.m_animation->GetAnimSet() ) )
	{
		const TDynArray< CFXDefinition* >& csEffects = templ->GetEffects();

		auto efIt =	FindIf( csEffects.Begin(), csEffects.End(), 
			[ this ]( CFXDefinition* e ) { return e->GetName() == m_effectName; } 
			);

		if ( efIt != csEffects.End() )
		{
			// play the cutscene-embedded effect
			entity->PlayEffect( *efIt );
			return;
		}
	}

	// play the entity-embedded effect
	entity->PlayEffect( m_effectName );
}

void CExtAnimCutsceneActorEffect::Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CEntity* entity = component->GetEntity();
	entity->StopEffect( m_effectName );
}

#ifdef DEBUG_CUTSCENES
#pragma optimize("",on)
#endif
