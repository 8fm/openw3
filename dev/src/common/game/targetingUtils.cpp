/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "targetingUtils.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( STargetSelectionWeights );
IMPLEMENT_ENGINE_CLASS( STargetSelectionData );

//////////////////////////////////////////////////////////////////////////

Float CTargetingUtils::CalcSelectionPriority( CNode* node, const STargetSelectionWeights& selectionWeights, const STargetSelectionData& selectionData )
{
	// PC_SCOPE_PIX( CalcSelectionPriority );

	Vector sourceToTarget = node->GetWorldPosition() - selectionData.m_sourcePosition;
	sourceToTarget.Z = 0.0f;
	const Float sourceToTargetDist = sourceToTarget.Normalize2();
	const Float sourceToTargetAngleDiff = MAbs( RAD2DEG( acosf( Vector::Dot2( sourceToTarget, selectionData.m_headingVector ) ) ) );

	Float res = ( selectionWeights.m_angleWeight * ( ( 180 - sourceToTargetAngleDiff ) / 180 ) );
	res += selectionWeights.m_distanceWeight * ( ( selectionData.m_softLockDistance - sourceToTargetDist ) / selectionData.m_softLockDistance );	

	if ( sourceToTargetDist > 0.0f  && sourceToTargetDist <= selectionData.m_closeDistance )
	{
		res += selectionWeights.m_distanceRingWeight * 1.0f;	
	}
	else if ( sourceToTargetDist > selectionData.m_closeDistance && sourceToTargetDist <= selectionData.m_softLockDistance )
	{
		res += selectionWeights.m_distanceRingWeight * 0.4f;
	}	

	return res;
}