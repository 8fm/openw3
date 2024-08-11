/**
* Copyright © 2010 CDProjekt Red, Inc. All Rights Reserved.
*/
#pragma once

#include "../engine/behaviorGraphAnimationManualSlot.h"
#include "../engine/behaviorGraphAnimationSlotNode.h"
#include "../engine/animationTrajectorySync.h"

#include "moveLocomotionSegment.h"
#include "animationSlider.h"

class CMoveLSAnimationSlide : public IMoveLocomotionSegment
{
	SAnimSliderTarget				m_target;
	SAnimatedSlideSettings			m_settings;
	AnimSlider						m_slider;

	SAnimationState					m_animationState;
	Float							m_duration;
	CBehaviorManualSlotInterface	m_slot;

	THandle< CActionMoveAnimationProxy > m_proxy;

public:
	CMoveLSAnimationSlide( const SAnimatedSlideSettings& settings, const SAnimSliderTarget& target );
	CMoveLSAnimationSlide( const SAnimatedSlideSettings& settings, const SAnimSliderTarget& target, THandle< CActionMoveAnimationProxy >& proxy );

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

//////////////////////////////////////////////////////////////////////////
