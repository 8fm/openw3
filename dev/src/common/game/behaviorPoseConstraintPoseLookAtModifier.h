
/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behaviorPoseConstraintPoseLookAtSegment.h"

class IBehaviorPoseConstraintPoseLookAtModifier : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBehaviorPoseConstraintPoseLookAtModifier, CObject );

	IBehaviorPoseConstraintPoseLookAtModifier()
		: m_enabled( true )
	{
	}

protected:
	Bool	m_enabled;

public:
	virtual void PreModify( Float& hAngle, Float& vAngle, Bool isTargetChanged, SPoseLookAtSegment& segment ) const = 0;
	virtual void PostModify( Float hAngle, Float vAngle, Bool isTargetChanged, SPoseLookAtSegment& segment ) const = 0;
	virtual void Reload() {}
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehaviorPoseConstraintPoseLookAtModifier );
	PROPERTY_EDIT( m_enabled, TXT("") );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

class CBPCPoseLookAtCurveTrajModifier : public IBehaviorPoseConstraintPoseLookAtModifier
{
	DECLARE_ENGINE_CLASS( CBPCPoseLookAtCurveTrajModifier, IBehaviorPoseConstraintPoseLookAtModifier, 0 );

protected:
	EAxis		m_axis;
	CCurve*		m_curve;
	Float		m_maxAngle;
	Float		m_maxValue;

public:
	CBPCPoseLookAtCurveTrajModifier()
		: m_axis( A_Z )
		, m_curve( nullptr )
		, m_maxAngle( 45.f )
		, m_maxValue( -10.f )
	{

	}

	virtual void PreModify( Float& hAngle, Float& vAngle, Bool isTargetChanged, SPoseLookAtSegment& segment ) const;
	virtual void PostModify( Float hAngle, Float vAngle, Bool isTargetChanged, SPoseLookAtSegment& segment ) const;
	virtual void Reload();
};

BEGIN_CLASS_RTTI( CBPCPoseLookAtCurveTrajModifier )
	PARENT_CLASS( IBehaviorPoseConstraintPoseLookAtModifier );
	PROPERTY_EDIT( m_axis, TXT("") );
	PROPERTY_CUSTOM_EDIT( m_curve, TXT("Curve"), TXT("CurveSelection") );
	PROPERTY_EDIT( m_maxAngle, TXT("") )
	PROPERTY_EDIT( m_maxValue, TXT("") )
END_CLASS_RTTI();
