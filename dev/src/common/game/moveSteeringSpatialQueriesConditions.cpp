/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "moveSteeringSpatialQueriesConditions.h"

#include "../engine/renderFrame.h"

#include "movableRepresentationPathAgent.h"
#include "movementCommandBuffer.h"

IMPLEMENT_ENGINE_CLASS( CMoveSCNavigationClearLine )


///////////////////////////////////////////////////////////////////////////////
// CMoveSCNavigationClearLine
///////////////////////////////////////////////////////////////////////////////

Bool CMoveSCNavigationClearLine::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	Vector2 dir( m_destinationLeft, m_destinationForward );
	CPathAgent* pathAgent = comm.GetAgent().GetPathAgent();

	if ( m_useCharacterOrientation )
	{
		dir = MathUtils::GeometryUtils::Rotate2D( dir, DEG2RAD( pathAgent->GetOrientation().Yaw ) );
	}
	else if ( m_useSteeringOutput )
	{
		const Vector2& heading = comm.GetHeading();
		if ( heading.IsAlmostZero() )
		{
			dir.Set( 0.f, 0.f );
		}
		else
		{
			Float yaw = EulerAngles::YawFromXY( heading.X, heading.Y );
			dir = MathUtils::GeometryUtils::Rotate2D( dir, DEG2RAD( yaw ) );
		}
	}
	else if ( m_useGoalDirection )
	{
		const SMoveLocomotionGoal& goal = comm.GetGoal();
		if ( !goal.IsHeadingGoalSet() )
		{
			return false;
		}
		const Vector2& heading = goal.GetHeadingToGoal();
		Float yaw = EulerAngles::YawFromXY( heading.X, heading.Y );
		dir = MathUtils::GeometryUtils::Rotate2D( dir, DEG2RAD( yaw ) );
	}

	
	Float testRadius = m_testRadius;
	if ( testRadius < 0.f )
	{
		testRadius = pathAgent->GetPersonalSpace();
	}

	const Vector3& position = pathAgent->GetPosition();
	Vector3 destination = position;
	destination.AsVector2() += dir;

	return pathAgent->TestLine( position, destination, testRadius );
}

void CMoveSCNavigationClearLine::GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame, InstanceBuffer& data ) const
{
	// Ugly copy+paste of stuff above
	Vector2 dir( m_destinationLeft, m_destinationForward );
	CPathAgent* pathAgent = agent.GetPathAgent();

	if ( m_useCharacterOrientation )
	{
		dir = MathUtils::GeometryUtils::Rotate2D( dir, DEG2RAD( pathAgent->GetOrientation().Yaw ) );
	}
	else if ( m_useSteeringOutput )
	{
		// not supported for debug
		return;
	}
	else if ( m_useGoalDirection )
	{
		// not supported for debug
		return;
	}

	Float testRadius = m_testRadius;
	if ( testRadius < 0.f )
	{
		testRadius = pathAgent->GetPersonalSpace();
	}

	const Vector3& position = pathAgent->GetPosition();
	Vector3 destination = position;
	destination.AsVector2() += dir;

	Color color = Color::WHITE;
	if ( !pathAgent->TestLine( position, destination, testRadius ) )
	{
		color = Color::RED;
	}
	
	if ( testRadius > 0.f )
	{
		frame->AddDebugCircle( position, testRadius, Matrix::IDENTITY, color, 12, true );
		frame->AddDebugCircle( destination, testRadius, Matrix::IDENTITY, color, 12, true );
	}

	Vector2 diff = (destination.AsVector2() - position.AsVector2()).Normalized() * testRadius;
	Vector2 diffL = MathUtils::GeometryUtils::PerpendicularL( diff );
	Vector2 diffR = MathUtils::GeometryUtils::PerpendicularR( diff );

	Vector pL0 = position; 
	pL0.AsVector2() += diffL;
	Vector pL1 = destination;
	pL1.AsVector2() += diffL;
	Vector pR0 = position;
	pR0.AsVector2() += diffR;
	Vector pR1 = destination;
	pR1.AsVector2() += diffR;

	frame->AddDebugLine( pL0, pL1, color, true );
	frame->AddDebugLine( pR0, pR1, color, true );
}