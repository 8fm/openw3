/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorPoseConstraintPoseLookAtModifier.h"
#include "../engine/curve.h"

void CBPCPoseLookAtCurveTrajModifier::PreModify( Float& hAngle, Float& vAngle, Bool isTargetChanged, SPoseLookAtSegment& segment ) const
{
	if ( m_enabled )
	{
		Float p = Min( MAbs( hAngle ), m_maxAngle ) / m_maxAngle;
		ASSERT( p >= 0.f && p <= 1.f );

		Float val = m_curve->GetFloatValue( p );

		vAngle += m_maxValue * val;
	}
}

void CBPCPoseLookAtCurveTrajModifier::PostModify( Float hAngle, Float vAngle, Bool isTargetChanged, SPoseLookAtSegment& segment ) const
{

}

void CBPCPoseLookAtCurveTrajModifier::Reload()
{
	if ( !m_curve )
	{
		m_curve = CreateObject< CCurve >( this );
	}
}