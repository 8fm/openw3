#pragma once

#include "moveLocomotionSegment.h"


///////////////////////////////////////////////////////////////////////////////

class CMoveLSCurve : public IMoveLocomotionSegment
{
private:
	Vector			m_target;
	Vector			m_shiftPos;
	Float			m_shiftRot;
	Float			m_timer;
	Float			m_duration;
	Float			m_heading;
	Bool			m_finished;

	Vector			m_controlPoint0;
	Vector			m_controlPoint1;
	Vector			m_startPosition;
	Float			m_initialTargetDistance;
	Float			m_curveProgress;
	Float			m_velocity;
	Bool			m_rightShiftCurve;

	Bool			m_disableCollisions;

public:
	CMoveLSCurve( const Vector& target, Float duration, Bool rightShift, const Float* heading = NULL );

	RED_INLINE CMoveLSCurve* DisableCollisions() { m_disableCollisions = true; return this; }

	// -------------------------------------------------------------------------
	// IMoveLocomotionSegment implementation
	// -------------------------------------------------------------------------
	virtual Bool Activate( CMovingAgentComponent& agent );
	virtual ELSStatus Tick( Float timeDelta, CMovingAgentComponent& agent );
	virtual void Deactivate( CMovingAgentComponent& agent );
	virtual Bool CanBeCanceled() const { return false; }
	Vector CalculateBezier( Float t, const Vector& p0, const Vector& p1, const Vector& p2, const Vector& p3 );
	void UpdateControlPoints( const Vector& targetPos );
	virtual void GenerateDebugFragments( CMovingAgentComponent& agent, CRenderFrame* frame );
	virtual void GenerateDebugPage( TDynArray< String >& debugLines ) const;
};

///////////////////////////////////////////////////////////////////////////////
