
#include "build.h"
#include "moveMatchToSegment.h"
#include "../../common/engine/behaviorGraphInstance.h"
#include "../../common/engine/behaviorGraphStack.h"
#include "../engine/skeletalAnimationContainer.h"

CMoveLSMatchTo::CMoveLSMatchTo( const SActionMatchToSettings& settings, const SActionMatchToTarget& target, THandle< CActionMoveAnimationProxy >& proxy )
	: m_duration( 1.f )
	, m_target( target )
	, m_settings( settings )
	, m_proxy( proxy )
{
	m_animationState.m_animation = settings.m_animation;

	m_sliderSettings.m_animation = settings.m_animation;
	m_sliderSettings.m_blendIn = settings.m_blendIn;
	m_sliderSettings.m_blendOut = settings.m_blendOut;
	m_sliderSettings.m_slotName = settings.m_slotName;
	m_sliderSettings.m_useGameTimeScale = settings.m_useGameTimeScale;
	m_sliderSettings.m_useRotationDeltaPolicy = settings.m_useRotationDeltaPolicy;

	m_sliderTarget.m_isTypeStatic = target.m_isTypeStatic;
	m_sliderTarget.m_node = target.m_node;
	m_sliderTarget.m_useRot = target.m_useRot;
	m_sliderTarget.m_useTrans = target.m_useTrans;
	m_sliderTarget.m_vec = target.m_vec;
}

CMoveLSMatchTo::CMoveLSMatchTo( const SActionMatchToSettings& settings, const SActionMatchToTarget& target )
	: m_duration( 1.f )
	, m_target( target )
	, m_settings( settings )
	, m_proxy( NULL )
{
	m_animationState.m_animation = settings.m_animation;

	if ( m_settings.m_matchBoneName != CName::NONE )
	{
		//...
	}

	m_sliderSettings.m_animation = settings.m_animation;
	m_sliderSettings.m_blendIn = settings.m_blendIn;
	m_sliderSettings.m_blendOut = settings.m_blendOut;
	m_sliderSettings.m_slotName = settings.m_slotName;
	m_sliderSettings.m_useGameTimeScale = settings.m_useGameTimeScale;
	m_sliderSettings.m_useRotationDeltaPolicy = settings.m_useRotationDeltaPolicy;

	m_sliderTarget.m_isTypeStatic = target.m_isTypeStatic;
	m_sliderTarget.m_node = target.m_node;
	m_sliderTarget.m_useRot = target.m_useRot;
	m_sliderTarget.m_useTrans = target.m_useTrans;
	m_sliderTarget.m_vec = target.m_vec;
}

Bool CMoveLSMatchTo::Activate( CMovingAgentComponent& agent )
{
	CActionMoveAnimationProxy* proxy = m_proxy.Get();
	if ( proxy )
	{
		proxy->m_isInitialized = true;
	}

	if ( m_settings.m_matchBoneName != CName::NONE )
	{
		const CSkeletalAnimationSetEntry* animationEntry = agent.GetAnimationContainer()->FindAnimation( m_settings.m_animation );
		if ( animationEntry && animationEntry->GetAnimation() )
		{
			//const CSkeletalAnimation* animation = animationEntry->GetAnimation();

			//const Float matchTime = animationEntry->GetDuration();

#ifdef USE_HAVOK_ANIMATION
			hkQsTransform motion = animationEntry->GetAnimation()->GetMovementBetweenTime( 0.f, matchTime, 0 );
			hkQsTransform offset;

			SkeletonBonesUtils::CalcBoneOffset( &agent, animation, m_settings.m_matchBoneName, matchTime, offset );
			
			//...
#endif
		}
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

void CMoveLSMatchTo::Deactivate( CMovingAgentComponent& agent )
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

ELSStatus CMoveLSMatchTo::Tick( Float timeDelta, CMovingAgentComponent& agent )
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
#ifdef USE_HAVOK_ANIMATION
	m_slot.SetMotion( hkQsTransform::getIdentity() );
#else
	m_slot.SetMotion( RedQsTransform::IDENTITY );
#endif
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

		//AI_LOG( TXT("%u, CMoveLSAnimationSlide::Tick - Yaw delta: %1.4f"), (Uint32)GEngine->GetCurrentEngineTick(), angles.Yaw );
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

void CMoveLSMatchTo::GenerateDebugFragments( CMovingAgentComponent& agent, CRenderFrame* frame )
{
	m_slider.GenerateDebugFragments( frame );
}

void CMoveLSMatchTo::GenerateDebugPage( TDynArray< String >& debugLines ) const
{
	debugLines.PushBack( TXT( "CMoveLSMatchTo" ) );
}
