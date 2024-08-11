/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "moveSteeringSideBySide.h"

#include "movableRepresentationPathAgent.h"
#include "movementCommandBuffer.h"
#include "movingAgentComponent.h"
#include "../core/mathUtils.h"

IMPLEMENT_ENGINE_CLASS( CMoveSTWalkSideBySide )
IMPLEMENT_ENGINE_CLASS( CMoveSCWalkSideBySide )
IMPLEMENT_ENGINE_CLASS( CMoveSTFaceTargetFacing )


///////////////////////////////////////////////////////////////////////////////
// CMoveSTWalkSideBySide - TASK
///////////////////////////////////////////////////////////////////////////////
void CMoveSTWalkSideBySide::ComputeSideBySidePosition( const Vector& myPos, const Vector& targetPos, const Vector2& targetHeading, Float sideBySideDistance, Vector& outSideBySidePos )
{
	Vector2 myDiff = myPos.AsVector2() - targetPos.AsVector2();

	outSideBySidePos = targetPos;
	outSideBySidePos.AsVector2() +=	// MathUtils::GeometryUtils::PerpendicularL( targetHeading ) * sideBySideDistance;
		(targetHeading.CrossZ( myDiff ) >= 0.f
		?  MathUtils::GeometryUtils::PerpendicularL( targetHeading )
		: MathUtils::GeometryUtils::PerpendicularR( targetHeading )) * sideBySideDistance;

		/*(targetHeading.CrossZ( myDiff ) < 0.f
		? MathUtils::GeometryUtils::PerpendicularL( targetHeading )
		: MathUtils::GeometryUtils::PerpendicularR( targetHeading ))*/
}

void CMoveSTWalkSideBySide::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	const CMovingAgentComponent& mac = comm.GetAgent();
	CPathAgent* pathAgent = mac.GetPathAgent();

	CNode* node = comm.GetGoal().GetGoalTargetNode();
	CActor* actor = Cast< CActor >( node );
	CMovingAgentComponent* targetMac = actor ? actor->GetMovingAgentComponent() : NULL;
	if ( !targetMac )
	{
		return;
	}

	const Vector& myPos = mac.GetWorldPositionRef();
	const Vector& targetPos = node->GetWorldPositionRef();

	Vector2 targetHeading = node->GetLocalToWorld().GetAxisY().AsVector2().Normalized();

	Vector sideBySidePos;

	CMoveSTWalkSideBySide::ComputeSideBySidePosition( myPos, targetPos, targetHeading, m_sideBySideDistance, sideBySidePos );

	Vector2 vecToSideBySidePos		= sideBySidePos - myPos;

	Float distToSideBySidePos		= vecToSideBySidePos.Mag();
	const Float relativeMaxSpeed	= mac.ConvertSpeedAbsToRel( mac.GetMaxSpeed() );
	Float targetSpeedLocal			= Min( mac.ConvertSpeedAbsToRel( targetMac->GetAbsoluteMoveSpeed() ) / relativeMaxSpeed, 1.f );

	Vector2 headingOutput;
	Float speedOutput;

	Float dot = targetHeading.Dot( vecToSideBySidePos );

	if ( distToSideBySidePos <= m_minApproachDistance )
	{
		// mimic target ONLY
		speedOutput = targetSpeedLocal;
		headingOutput = targetHeading;
	}
	else if ( dot >= 0.f )
	{
		if ( distToSideBySidePos >= m_maxApproachDistance )
		{
			// Go to follow position ONLY
			const Float walkSpeed = ( 1.0f / relativeMaxSpeed );
			
			if ( targetSpeedLocal != 0.0f )
			{
				
				speedOutput = Min( targetSpeedLocal * m_catchupSpeedMultiplier, walkSpeed );
			}
			else
			{
				speedOutput = walkSpeed;
			}
			
			headingOutput = vecToSideBySidePos * (1.f / distToSideBySidePos);									// normalize target pos


		}
		else
		{
			// Blend between mimic target and follow position
			Float ratio = Max( distToSideBySidePos - m_minApproachDistance, 0.0f ) / (m_maxApproachDistance - m_minApproachDistance);
			// ratio == 1 - cachup only, ratio == 0 mimin only

			const Float walkSpeed = ( 1.0f / relativeMaxSpeed );
			// speed
			if ( targetSpeedLocal != 0.0f )
			{
				Float speedMimic	= targetSpeedLocal;
				Float speedCachup	= Min( targetSpeedLocal * m_catchupSpeedMultiplier, walkSpeed);
				speedOutput			= speedMimic + (speedCachup - speedMimic) * ratio;
			}
			else
			{
				speedOutput = walkSpeed;
			}

			// heading
			Vector2 headingMimic	= targetHeading;
			Vector2 headingCachup	= vecToSideBySidePos * (1.f / distToSideBySidePos);

			headingOutput = headingMimic + (headingCachup - headingMimic) * ratio;
		}
	}
	else
	{
		// we are ahead of target
		if ( distToSideBySidePos >= m_maxApproachDistance )
		{
			headingOutput.Set( 0.f, 0.f );
			speedOutput = 0.f;
		}
		else
		{
			Float ratio	= (distToSideBySidePos - m_minApproachDistance) / (m_maxApproachDistance - m_minApproachDistance);

			Float speedMimic		= targetSpeedLocal;
			const Float walkSpeed	= ( 1.0f / relativeMaxSpeed );
			Float speedCatchup		= Min( targetSpeedLocal * m_slowDownSpeedMultiplier, walkSpeed );
			speedOutput				= speedMimic + (speedCatchup - speedMimic) * ratio;
			headingOutput			= targetHeading;
		}
	}

	

	comm.AddHeading( headingOutput, m_headingImportance );
	comm.AddSpeed( speedOutput, m_speedImportance );
}

String CMoveSTWalkSideBySide::GetTaskName() const
{
	return TXT("Move side by side");
}

void CMoveSTWalkSideBySide::GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const
{

};


///////////////////////////////////////////////////////////////////////////////
// CMoveSCWalkSideBySide - CONDITION
///////////////////////////////////////////////////////////////////////////////
Bool CMoveSCWalkSideBySide::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	CNode* node = comm.GetGoal().GetGoalTargetNode();
	if ( !node )
	{
		return false;
	}

	const CMovingAgentComponent& mac = comm.GetAgent();
	CPathAgent* pathAgent = mac.GetPathAgent();
		
	const Vector& myPos = mac.GetWorldPositionRef();
	const Vector& targetPos = node->GetWorldPositionRef();

	// max distance test
	Float distSq = (targetPos - myPos).SquareMag3();
	if ( distSq > m_distanceLimit*m_distanceLimit )
	{
		return false;
	}

	// line to target test - obsolate probably
	//if( !pathAgent->TestLine( targetPos ) )
	//{
	//	return false;
	//}

	Vector2 targetHeading = node->GetLocalToWorld().GetAxisY().AsVector2().Normalized();

	Vector sideBySidePos;

	CMoveSTWalkSideBySide::ComputeSideBySidePosition( myPos, targetPos, targetHeading, m_sideBySideDistance, sideBySidePos );

	// line to side-by-side follow pos test
	if ( !pathAgent->TestLine( sideBySidePos ) )
	{
		return false;
	}

	return true;
}
String CMoveSCWalkSideBySide::GetConditionName() const
{
	return TXT("Can move side by side");
}


///////////////////////////////////////////////////////////////////////////////
// CMoveSTFaceTargetFacing
///////////////////////////////////////////////////////////////////////////////
void CMoveSTFaceTargetFacing::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	CNode* node = GetTarget( comm.GetGoal() );
	if ( !node )
	{
		return;
	}

	CMovingAgentComponent& moveAgent	= comm.GetAgent();
	Float desiredYaw					= node->GetWorldYaw();
	Float currYaw						= moveAgent.GetWorldYaw();
	Float yawDiff						= EulerAngles::AngleDistance( currYaw, desiredYaw );
	comm.LockRotation( yawDiff, true );
}

String CMoveSTFaceTargetFacing::GetTaskName() const
{
	return TXT("Face target facing");
}