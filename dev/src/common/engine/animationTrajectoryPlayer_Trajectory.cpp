
#include "build.h"
#include "animationTrajectoryPlayer_Trajectory.h"
#include "animationTrajectoryPlayer.h"

AnimationTrajectoryPlayer_Trajectory::AnimationTrajectoryPlayer_Trajectory( const SAnimationTrajectoryPlayerToken& token, const CAnimatedComponent* component )
	: m_pointWasChanged( false )
	, m_weight( 1.f )
{
	ASSERT( token.m_isValid );
	ASSERT( token.m_animationA );
	ASSERT( token.m_trajectoryParamA );

	m_animation = token.m_animationA;

	m_animationState.m_animation = m_animation->GetName();
	m_animationState.m_currTime = 0.f;
	m_animationState.m_prevTime = 0.f;

	m_trajectorySrc = &(token.m_trajectoryParamA->GetDataR());
	//m_trajectoryMod = 

	m_timeController.Set( m_trajectorySrc->GetSyncPointTime(), token.m_timeFactor, token.m_syncPointDuration, token.m_duration, token.m_trajectoryParamA );

	m_blendIn = token.m_blendIn;
	m_blendOut = token.m_blendOut;

	ASSERT( m_weight == 1.f );
}

AnimationTrajectoryPlayer_Trajectory::~AnimationTrajectoryPlayer_Trajectory()
{

}

Bool AnimationTrajectoryPlayer_Trajectory::UpdatePoint( const Matrix& l2w, const Vector& pointWS )
{
	m_requestedPointWS = pointWS;
	m_pointWasChanged = true;

	if ( m_pointWasChanged )
	{
		m_pointWS = m_requestedPointWS;
		m_pointWasChanged = false;

		// TODO
	}

	return true;
}

Bool AnimationTrajectoryPlayer_Trajectory::Update( Float& dt )
{
	Float time = m_timeController.Update( dt );

	m_animationState.m_prevTime = m_animationState.m_currTime;
	m_animationState.m_currTime = time;

	m_weight = 1.f;

	if ( m_blendIn > 0.f && m_animationState.m_currTime < m_blendIn )
	{
		const Float w = m_animationState.m_currTime / m_blendIn;
		ASSERT( w >= 0.f && w <= 1.f );

		m_weight = w;
	}
	else if ( m_blendOut > 0.f )
	{
		const Float duration = m_animation->GetDuration();
		const Float blendOutStart = Max( 0.f, duration - m_blendOut );

		if ( m_animationState.m_currTime > blendOutStart )
		{
			const Float w = 1.f - ( m_animationState.m_currTime - blendOutStart ) / m_blendOut;
			ASSERT( w >= 0.f && w <= 1.f );

			m_weight = w;
		}
	}

	return m_animationState.m_currTime >= m_animation->GetDuration();
}

Bool AnimationTrajectoryPlayer_Trajectory::PlayAnimationOnSlot( CBehaviorManualSlotInterface& slot )
{
	return slot.IsValid() ? slot.PlayAnimation( m_animationState, m_weight ) : true;
}

Float AnimationTrajectoryPlayer_Trajectory::GetTime() const
{
	return m_timeController.GetTime();
}

Bool AnimationTrajectoryPlayer_Trajectory::IsBeforeSyncTime() const
{
	return m_timeController.IsBeforeSyncTime();
}

void AnimationTrajectoryPlayer_Trajectory::GenerateFragments( CRenderFrame* frame ) const
{
	//m_currAnimation.m_trajectoryL->GenerateFragments( frame, m_currAnimation.m_animationState.m_currTime, m_component->GetLocalToWorld(), Color( 0, 0, 255 ), true );
	//m_currAnimation.m_trajectoryR->GenerateFragments( frame, m_currAnimation.m_animationState.m_currTime, m_component->GetLocalToWorld(), Color( 255, 0, 255 ), false );

	//frame->AddDebugSphere( m_currAnimation.m_pointWS, 0.1f, Matrix::IDENTITY, Color( 255, 0, 0 ) );
}

