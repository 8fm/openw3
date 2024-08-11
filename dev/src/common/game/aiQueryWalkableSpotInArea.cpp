/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "aiQueryWalkableSpotInArea.h"

#include "../core/mathUtils.h"

#include "../engine/areaComponent.h"

#include "movableRepresentationPathAgent.h"


///////////////////////////////////////////////////////////////////////////////
// CQueryReacheableSpotInAreaRequest
///////////////////////////////////////////////////////////////////////////////
CQueryReacheableSpotInAreaRequest::CQueryReacheableSpotInAreaRequest()
{

}

Bool CQueryReacheableSpotInAreaRequest::RefreshSetup( CWorld* world, CActor* actor, const CAreaComponent* area, const Vector& referencePos, Float maxDist, Float tolerance )
{
	// check if we have our request already setup properly
	if ( GetQueryState() == STATE_SETUP && area == m_areaHandle.Get() )
	{
		return true;
	}

	m_areaHandle = area;
	m_areaShape = area->GetCompiledShapePtr();
	m_areaShapeTransform = area->GetWorldToLocal();

	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return false;
	}
	CPathAgent* pathAgent = mac->GetPathAgent();
	const Box& testBox = area->GetBoundingBox();

	m_tolerance = tolerance;
	m_useTolerance = tolerance > 1.f;

	Super::Setup( *world->GetPathLibWorld(), testBox, referencePos.AsVector3(), pathAgent->GetPosition(), pathAgent->GetPersonalSpace(), maxDist, FLAG_REQUIRE_REACHABLE_FROM_SOURCE | FLAG_BAIL_OUT_ON_SUCCESS, pathAgent->GetCollisionFlags(), pathAgent->GetForbiddenPathfindFlags(), 13 );
	return true;
}

Bool CQueryReacheableSpotInAreaRequest::AcceptPosition( const Vector3& nodePos )
{
	const Bool roughTestSuccessful = m_useTolerance ? m_testBox.IntersectSphere( Sphere( nodePos, m_tolerance ) ) : m_testBox.Contains( nodePos );
	
	if ( roughTestSuccessful )
	{
		// sometimes it's possible that area shape is not built due to some convex triangulation failures
		// we don't need a crash here, but in general performing of only rough bounding box intersection test may lead to incorrect behavior (like selecting wander points outside the guard areas)
		if ( !m_areaShape )
		{
			return true;
		}
		
		if ( m_useTolerance )
		{
			const Box toleranceBox( nodePos, m_tolerance );
			return m_areaShape->BoxOverlap( m_areaShapeTransform.TransformPoint( toleranceBox.CalcCenter() ), toleranceBox.CalcExtents(), m_areaShapeTransform.TransformVector( Vector::EX ), m_areaShapeTransform.TransformVector( Vector::EY ), m_areaShapeTransform.TransformVector( Vector::EZ ) );
		}

		return m_areaShape->PointOverlap( m_areaShapeTransform.TransformPoint( nodePos ) );
	}
	
	return false;
}

void CQueryReacheableSpotInAreaRequest::CompletionCallback()
{
	m_areaShape.Clear();
	Super::CompletionCallback();
}