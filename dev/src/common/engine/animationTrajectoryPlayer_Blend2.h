
#pragma once

#include "animationTrajectoryPlayer_State.h"
#include "animationTrajectory.h"
#include "behaviorGraphAnimationManualSlot.h"
#include "animationPlayerTimeController.h"
#include "animationTrajectorySync.h"

class AnimationTrajectoryPlayer_Blend2 : public AnimationTrajectoryPlayer_State
{
	const AnimationTrajectoryData*		m_trajectorySrcA;
	const AnimationTrajectoryData*		m_trajectorySrcB;

	Vector								m_trajectorySyncPointA;
	Vector								m_trajectorySyncPointB;

	TDynArray< Vector >					m_trajectoryLS;
	TDynArray< Vector >					m_trajectoryMS;
	TDynArray< Vector >					m_trajectoryWS;

	Vector								m_pointWS;
	Matrix								m_localToWorld;
	Vector								m_requestedPointWS;
	Bool								m_pointWasChanged;

	SAnimationState						m_animationStateA;
	SAnimationState						m_animationStateB;
	Float								m_animationWeight;

	Float								m_blendIn;
	Float								m_blendOut;
	Float								m_duration;
	Float								m_weight;

	THandle< CActionMoveAnimationProxy > m_proxy;
	EActionMoveAnimationSyncType		m_proxySyncType;

	AnimationPlayerTimers::TimeController_AfterHitSlowMotionEvt	m_timeController;

	Int32									hack_lastIndex;
	Vector								hack_lastPoint;

public:
	AnimationTrajectoryPlayer_Blend2( const SAnimationTrajectoryPlayerToken& token, const CAnimatedComponent* component );
	~AnimationTrajectoryPlayer_Blend2();

	virtual Bool UpdatePoint( const Matrix& l2w, const Vector& pointWS );

	virtual Bool Update( Float& dt );
	virtual Bool PlayAnimationOnSlot( CBehaviorManualSlotInterface& slot );

	virtual Float GetTime() const;
	virtual Bool IsBeforeSyncTime() const;

	virtual void GenerateFragments( CRenderFrame* frame ) const;

private:
	void RefreshTrajectoryDatas();

	void HACK_SendTrajectoryToPlayer();
};
