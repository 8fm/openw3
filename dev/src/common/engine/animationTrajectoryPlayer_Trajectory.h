
#pragma once

#include "animationTrajectoryPlayer_State.h"
#include "animationTrajectory.h"
#include "behaviorGraphAnimationManualSlot.h"
#include "animationPlayerTimeController.h"

class AnimationTrajectoryPlayer_Trajectory : public AnimationTrajectoryPlayer_State
{
	const AnimationTrajectoryData*		m_trajectorySrc;
	AnimationTrajectoryData				m_trajectoryMod;

	Vector								m_pointWS;
	Vector								m_requestedPointWS;
	Bool								m_pointWasChanged;

	const CSkeletalAnimationSetEntry*	m_animation;
	SAnimationState						m_animationState;

	Float								m_blendIn;
	Float								m_blendOut;
	Float								m_weight;

	AnimationPlayerTimers::TimeController_AfterHitSlowMotionEvt	m_timeController;

public:
	AnimationTrajectoryPlayer_Trajectory( const SAnimationTrajectoryPlayerToken& token, const CAnimatedComponent* component );
	~AnimationTrajectoryPlayer_Trajectory();

	virtual Bool UpdatePoint( const Matrix& l2w, const Vector& pointWS );

	virtual Bool Update( Float& dt );
	virtual Bool PlayAnimationOnSlot( CBehaviorManualSlotInterface& slot );

	virtual Float GetTime() const;
	virtual Bool IsBeforeSyncTime() const;

	virtual void GenerateFragments( CRenderFrame* frame ) const;
};

