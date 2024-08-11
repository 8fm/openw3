
#pragma once

#include "moveLocomotionSegment.h"
#include "animationSlider.h"
#include "actorActionMatchTo.h"
#include "../../common/engine/behaviorGraphAnimationManualSlot.h"
#include "../../common/engine/behaviorGraphAnimationSlotNode.h"
#include "../../common/engine/animationTrajectorySync.h"

class CMoveLSMatchTo : public IMoveLocomotionSegment
{
	SActionMatchToTarget			m_target;
	SActionMatchToSettings			m_settings;

	SAnimSliderTarget				m_sliderTarget;
	SAnimatedSlideSettings			m_sliderSettings;
	AnimSlider						m_slider;

	SAnimationState					m_animationState;
	Float							m_duration;
	CBehaviorManualSlotInterface	m_slot;

	THandle< CActionMoveAnimationProxy > m_proxy;

public:
	CMoveLSMatchTo( const SActionMatchToSettings& settings, const SActionMatchToTarget& target );
	CMoveLSMatchTo( const SActionMatchToSettings& settings, const SActionMatchToTarget& target, THandle< CActionMoveAnimationProxy >& proxy );

	// -------------------------------------------------------------------------
	// IMoveLocomotionSegment implementation
	// -------------------------------------------------------------------------
	virtual Bool Activate( CMovingAgentComponent& agent );
	virtual ELSStatus Tick( Float timeDelta, CMovingAgentComponent& agent );
	virtual void Deactivate( CMovingAgentComponent& agent );
	virtual Bool CanBeCanceled() const { return true; }
	virtual void GenerateDebugFragments( CMovingAgentComponent& agent, CRenderFrame* frame );
	virtual void GenerateDebugPage( TDynArray< String >& debugLines ) const;
};
