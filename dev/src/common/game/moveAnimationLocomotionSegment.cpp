#include "build.h"
#include "moveAnimationLocomotionSegment.h"

#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphStack.h"
#include "../engine/skeletalAnimationContainer.h"

#include "aiLog.h"
#include "movementTargeter.h"
#include "moveGlobalPathPlanner.h"


///////////////////////////////////////////////////////////////////////////////

CMoveLSAnimationSlide::CMoveLSAnimationSlide( const SAnimatedSlideSettings& settings, const SAnimSliderTarget& target, THandle< CActionMoveAnimationProxy >& proxy )
	: m_duration( 1.f )
	, m_target( target )
	, m_settings( settings )
	, m_proxy( proxy )
{
	m_animationState.m_animation = settings.m_animation;
}

CMoveLSAnimationSlide::CMoveLSAnimationSlide( const SAnimatedSlideSettings& settings, const SAnimSliderTarget& target )
	: m_duration( 1.f )
	, m_target( target )
	, m_settings( settings )
	, m_proxy( NULL )
{
	m_animationState.m_animation = settings.m_animation;
}

Bool CMoveLSAnimationSlide::Activate( CMovingAgentComponent& agent )
{
	CActionMoveAnimationProxy* proxy = m_proxy.Get();
	if ( proxy )
	{
		proxy->m_isInitialized = true;
	}

	SAnimSliderSettings settings;
	settings.m_slotName = m_settings.m_slotName;
	settings.m_rotationPolicy = m_settings.m_useRotationDeltaPolicy ? SAnimSliderSettings::RP_Delta : SAnimSliderSettings::RP_Speed;

	if ( !agent.GetBehaviorStack() )
	{
		WARN_GAME( TXT("Play action animation for %s: no behavior stack"), agent.GetEntity()->GetFriendlyName().AsChar() );
		return false;
	}
	else if ( !agent.GetAnimationContainer() )
	{
		WARN_GAME( TXT("Play action animation for %s: no animation container"), agent.GetEntity()->GetFriendlyName().AsChar() );
		return false;
	}
	
	const CSkeletalAnimationSetEntry* animation = agent.GetAnimationContainer()->FindAnimation( m_animationState.m_animation );
	if ( !animation )
	{
		return false;
	}

	m_duration = animation->GetDuration();

	if ( !agent.GetBehaviorStack()->GetSlot( settings.m_slotName, m_slot ) )
	{
		return false;
	}

	ASSERT( m_slot.IsValid() );

	if ( !m_slider.Init( animation, settings ) )
	{
		return false;
	}

	const Matrix& l2w = agent.GetLocalToWorld();

	m_slider.SetCurrentPosition( l2w );

	Matrix dest;
	Bool ret = m_target.Get( l2w, dest );
	if ( ret )
	{
		m_slider.SetTarget( dest );
	}

	if ( proxy )
	{
		proxy->m_isValid = true;
		proxy->m_duration = m_duration;
		proxy->m_prevTime = 0.f;
		proxy->m_currTime = 0.f;
		proxy->m_finished = false;
	}

	return true;
}

void CMoveLSAnimationSlide::Deactivate( CMovingAgentComponent& agent )
{
	if ( m_slot.IsValid() )
	{
		m_slot.ResetMotion();
		m_slot.Stop();
	}

	CActionMoveAnimationProxy* proxy = m_proxy.Get();
	if ( proxy )
	{
		proxy->m_isValid = false;
	}
}

ELSStatus CMoveLSAnimationSlide::Tick( Float timeDelta, CMovingAgentComponent& agent )
{
	ASSERT( m_slot.IsValid() );

	if ( !m_settings.m_useGameTimeScale )
	{
		timeDelta = GEngine->GetLastTimeDelta();
	}

	const Matrix& l2w = agent.GetLocalToWorld();
	
	m_animationState.m_prevTime = m_animationState.m_currTime;
	m_animationState.m_currTime += timeDelta;

	m_animationState.m_currTime = Min( m_animationState.m_currTime, m_duration );

	// Calculate the blend weight based on blend time
	Float weight = 1.f;
	if( m_animationState.m_currTime < m_settings.m_blendIn )
	{
		weight = m_animationState.m_currTime / m_settings.m_blendIn;
	}
	else if( m_settings.m_blendOut > 0.f )
	{
		const Float blendOutStart = m_duration - m_settings.m_blendOut;
		if( m_animationState.m_currTime > blendOutStart )
		{
			weight = 1.f - ( (m_animationState.m_currTime - blendOutStart) / m_settings.m_blendOut );
		}
	}

	const Bool animUpdate = m_slot.PlayAnimation( m_animationState, weight );
	m_slot.SetMotion( AnimQsTransform::IDENTITY );
	Matrix dest;
	Bool ret = m_target.Get( l2w, dest );

	Bool sliderInProgress = false;

	if ( ret )
	{
		Vector deltaTransWS;
		Float deltaRotWS;

		if ( m_target.IsRotationSet() && m_target.IsTranslationSet() )
		{
			m_slider.SetTarget( dest );
		}
		else if ( m_target.IsRotationSet() )
		{
			m_slider.SetTargetRotation( dest.GetYaw() );
		}
		else
		{
			m_slider.SetTargetPosition( dest.GetTranslationRef() );
		}

		sliderInProgress = m_slider.UpdateWS( l2w, timeDelta, deltaTransWS, deltaRotWS );

		EulerAngles angles( 0.f, 0.f, deltaRotWS );

		agent.AddDeltaMovement( deltaTransWS, angles );

		AI_LOG( TXT("%u, CMoveLSAnimationSlide::Tick - Yaw delta: %1.4f"), (Uint32)GEngine->GetCurrentEngineTick(), angles.Yaw );
	}

	CActionMoveAnimationProxy* proxy = m_proxy.Get();
	if ( proxy )
	{
		proxy->m_prevTime = m_animationState.m_prevTime;
		proxy->m_currTime = m_animationState.m_currTime;
	}

	if ( !animUpdate )
	{
		if ( proxy )
		{
			proxy->m_finished = true;
		}

		return LS_Failed;
	}
	else if ( m_animationState.m_currTime < m_duration || sliderInProgress )
	{
		return LS_InProgress;
	}
	else
	{
		if ( proxy )
		{
			proxy->m_finished = true;
		}

		return LS_Completed;
	}
}

void CMoveLSAnimationSlide::GenerateDebugFragments( CMovingAgentComponent& agent, CRenderFrame* frame )
{
	m_slider.GenerateDebugFragments( frame );
}

void CMoveLSAnimationSlide::GenerateDebugPage( TDynArray< String >& debugLines ) const
{
	debugLines.PushBack( TXT( "CMoveLSAnimationSlide" ) );
}
