/**
 * Copyright © 2010 CDProjekt Red, Inc. All Rights Reserved.
 */
#pragma once

#include "moveLocomotionSegment.h"


enum ESlideRotation
{
	SR_Nearest,
	SR_Right,
	SR_Left,
};

BEGIN_ENUM_RTTI( ESlideRotation );
ENUM_OPTION( SR_Nearest );
ENUM_OPTION( SR_Right );
ENUM_OPTION( SR_Left );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CMoveLSSlide : public IMoveLocomotionSegment
{
private:
	Vector			m_target;
	Vector			m_shiftPos;
	Float			m_shiftRot;
	Float			m_timer;
	Float			m_duration;
	Float			m_heading;
	ESlideRotation	m_rotation;
	Bool			m_finished;

	Bool			m_disableCollisions;

public:
	CMoveLSSlide( const Vector& target, Float duration, const Float* heading = NULL, ESlideRotation rotation = SR_Nearest );

	RED_INLINE CMoveLSSlide* DisableCollisions() { m_disableCollisions = true; return this; }

	// -------------------------------------------------------------------------
	// IMoveLocomotionSegment implementation
	// -------------------------------------------------------------------------
	virtual Bool Activate( CMovingAgentComponent& agent );
	virtual ELSStatus Tick( Float timeDelta, CMovingAgentComponent& agent );
	virtual void Deactivate( CMovingAgentComponent& agent );
	virtual Bool CanBeCanceled() const { return false; }
	virtual void GenerateDebugFragments( CMovingAgentComponent& agent, CRenderFrame* frame );
	virtual void GenerateDebugPage( TDynArray< String >& debugLines ) const;
};

///////////////////////////////////////////////////////////////////////////////
