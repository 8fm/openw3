
#include "build.h"
#include "extAnimGameplayMimicEvent.h"

#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/core/gatheredResource.h"

#ifndef NO_EDITOR

CGatheredResource resGameplayMimicAnims( TXT("animations\\mimics\\gameplay_man_mimic_animation.w2anims"), 0 );

namespace
{
	Float GetDurationFromAnim( const CName& anim )
	{
		CSkeletalAnimationSet* set = resGameplayMimicAnims.LoadAndGet< CSkeletalAnimationSet >();
		if ( set )
		{
			CSkeletalAnimationSetEntry* entry = set->FindAnimation( anim );
			if ( entry )
			{
				return entry->GetDuration();
			}
		}
		else
		{
			ASSERT( set );
		}

		return 0.5f;
	}
}

#endif

IMPLEMENT_ENGINE_CLASS( CExtAnimGameplayMimicEvent );

CExtAnimGameplayMimicEvent::CExtAnimGameplayMimicEvent()
	: CExtAnimDurationEvent()
{
	m_reportToScript = false;
	m_alwaysFiresEnd = true;
}


CExtAnimGameplayMimicEvent::CExtAnimGameplayMimicEvent( const CName& eventName, 
	const CName& animationName, 
	Float startTime, 
	Float duration, 
	const String& trackName )
	: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
{
	m_reportToScript = false;
	m_alwaysFiresEnd = true;
}

#ifndef NO_EDITOR

void CExtAnimGameplayMimicEvent::OnPropertyPostChanged( const CName& propertyName )
{
	CExtAnimDurationEvent::OnPropertyPostChanged( propertyName );

	if ( propertyName == TXT("animation") )
	{
		m_duration = GetDurationFromAnim( m_animation );	
	}
}

const CSkeletalAnimationSet* CExtAnimGameplayMimicEvent::GetAnimationSet()
{
	return resGameplayMimicAnims.LoadAndGet< CSkeletalAnimationSet >();
}

#endif

void CExtAnimGameplayMimicEvent::Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CEntity* ent = component->GetEntity();
	CActor* actor = Cast< CActor >( ent );
	if ( actor )
	{
		actor->PlayMimicAnimation( m_animation, CNAME( MIMIC_GMPL_GESTURE_SLOT ) );
	}
}

void CExtAnimGameplayMimicEvent::Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CEntity* ent = component->GetEntity();
	CActor* actor = Cast< CActor >( ent );
	if ( actor )
	{
		CAnimatedComponent* root = actor->GetRootAnimatedComponent();
		if ( root )
		{
			CBehaviorGraphStack* stack = root->GetBehaviorStack();
			if ( stack )
			{
				actor->StopMimicAnimation( CNAME( MIMIC_GMPL_GESTURE_SLOT ) );
			}
		}
	}
}
