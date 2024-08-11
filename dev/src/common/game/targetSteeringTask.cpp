/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "targetSteeringTask.h"

#include "../core/instanceDataLayoutCompiler.h"
#include "../core/mathUtils.h"


#include "movementCommandBuffer.h"
#include "movementGoal.h"
#include "behTreeMachine.h"


IMPLEMENT_ENGINE_CLASS( IMoveTargetSteeringTask )
IMPLEMENT_ENGINE_CLASS( IMoveTargetPositionSteeringTask )
IMPLEMENT_ENGINE_CLASS( CMoveSTKeepDistanceToTarget )
IMPLEMENT_ENGINE_CLASS( CMoveSTNeverBackDown )
IMPLEMENT_ENGINE_CLASS( CMoveSTFaceTarget )


///////////////////////////////////////////////////////////////////////////////
// IMoveTargetSteeringTask
///////////////////////////////////////////////////////////////////////////////
String IMoveTargetSteeringTask::GetTaskName() const
{
	static const String TASKNAME( TXT( "ABSTRACT TargetSteering" ) );
	return TASKNAME;
}

CNode* IMoveTargetSteeringTask::GetTarget( const SMoveLocomotionGoal& goal ) const
{
	if ( m_namedTarget.Empty() )
	{
		return goal.GetGoalTargetNode();
	}
	CNode* targetNode = nullptr;
	goal.TGetFlag( m_namedTarget, targetNode );
	return targetNode;
}

///////////////////////////////////////////////////////////////////////////////
// IMoveTargetPositionSteeringTask
///////////////////////////////////////////////////////////////////////////////
String IMoveTargetPositionSteeringTask::GetTaskName() const
{
	return TXT("TargetPositionSteering");
}
Bool IMoveTargetPositionSteeringTask::GetTargetPosition( const SMoveLocomotionGoal& goal, Vector& outTargetPosition ) const
{
	if ( !m_customPosition.Empty()  )
	{
		return goal.GetFlag( m_customPosition, outTargetPosition );
	}
	CNode* target = GetTarget( goal );
	if ( target )
	{
		outTargetPosition = target->GetWorldPositionRef();
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSTKeepDistanceToTarget
///////////////////////////////////////////////////////////////////////////////

void CMoveSTKeepDistanceToTarget::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	Vector targetPos;
	if ( !GetTargetPosition( goal, targetPos ) )
	{
		return;
	}
	CMovingAgentComponent& mac = comm.GetAgent();
	const Vector& myPos = mac.GetWorldPositionRef();

	// first update decision process
	EngineTime gameTime = GGame->GetEngineTime();
	if ( gameTime >= data[ i_decisionTimeout ] )
	{
		data[ i_decisionTimeout ] = gameTime + m_randomizationFrequency * ( 0.5f + GEngine->GetRandomNumberGenerator().Get< Float >() );
		data[ i_desiredDistance ] = GEngine->GetRandomNumberGenerator().Get< Float >( m_minRange , m_maxRange );
	}

	Float desiredDist = data[ i_desiredDistance ];


	// calculate distances
	Vector2 toTarget = targetPos.AsVector2() - myPos.AsVector2();
	const Float dist = toTarget.Mag();

	// calculate desired output
	const Float distDiff = desiredDist - dist;
	const Float distDiffAbs = Abs( distDiff );

	if ( distDiffAbs < NumericLimits< Float >::Epsilon() )
	{
		return;
	}

	Float desiredOutput;

	if ( distDiffAbs <= m_tolerance )
	{
		desiredOutput = 0.f;
	}
	else if ( distDiffAbs >= m_brakeDistance )
	{
		desiredOutput = distDiff >= 0.f ? -1.f : 1.f;
	}
	else
	{
		desiredOutput = 1.f - (m_brakeDistance - distDiffAbs) / (m_brakeDistance - m_tolerance);
		if ( distDiff >= 0.f )
		{
			desiredOutput = -desiredOutput;
		}
	}

	// blend output
	const Float lastOutput = data[ i_lastOutput ];
	const Float outputDiff = desiredOutput - lastOutput;
	const Float outputDiffAbs = Abs( outputDiff );

	Float maxChange = timeDelta * m_acceleration;

	Float currentOutput;

	if( outputDiffAbs <= maxChange )
	{
		currentOutput = desiredOutput;
	}
	else
	{
		currentOutput = lastOutput + (outputDiff >= 0.f ? maxChange : -maxChange);
	}

	data[ i_lastOutput ] = currentOutput;

	Float importanceRatio = Abs( currentOutput );

	if ( importanceRatio > 0.f )
	{
		Vector2 heading = toTarget * (currentOutput / dist);

		// setup steering
		comm.AddHeading( heading, m_importance * importanceRatio );
		comm.AddSpeed( m_moveSpeed, m_importance * importanceRatio );
	}
}

String CMoveSTKeepDistanceToTarget::GetTaskName() const
{
	static const String TASKNAME( TXT( "Keep distance to target" ) );
	return TASKNAME;
}
void CMoveSTKeepDistanceToTarget::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_decisionTimeout;
	compiler << i_desiredDistance;
	compiler << i_lastOutput;
}
void CMoveSTKeepDistanceToTarget::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	TBaseClass::OnInitData( agent, data );

	data[ i_decisionTimeout ] = EngineTime::ZERO;
	data[ i_desiredDistance ] = m_maxRange;
	data[ i_lastOutput ] = 0.f;
}


///////////////////////////////////////////////////////////////////////////////
// CMoveSTNeverBackDown
///////////////////////////////////////////////////////////////////////////////
void CMoveSTNeverBackDown::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	Vector targetPos;
	if ( !GetTargetPosition( goal, targetPos ) )
	{
		return;
	}
	Vector2 currHeading = comm.GetHeading();
	if ( currHeading.IsAlmostZero() )
	{
		return;
	}

	CMovingAgentComponent& mac = comm.GetAgent();
	Vector2 ownerPos = mac.GetWorldPositionRef();
	Vector2 toTarget = (targetPos.AsVector2() - ownerPos).Normalized();
	Float headingLen = currHeading.Mag();
	Vector2 normalizedHeadig = currHeading * (1.f / headingLen);
	Float cosAngle = data[ i_cosAngle ];

	Float cosGamma = toTarget.Dot( normalizedHeadig );
	if ( cosGamma < cosAngle )
	{
		Float sinAngle = data[ i_sinAngle ];

		Float sinGamma = sqrt( 1.f - cosGamma );
		Float r = (headingLen * sinGamma / data[ i_sinAngle ]);

		if ( toTarget.CrossZ( normalizedHeadig ) < 0.f )
		{
			sinAngle = -sinAngle;
		}

		Vector2 modifedHeading = MathUtils::GeometryUtils::Rotate2D( toTarget, sinAngle, cosAngle );
		modifedHeading *= r;

		comm.ModifyHeading( modifedHeading );
	}
}

String CMoveSTNeverBackDown::GetTaskName() const
{
	static const String TASKNAME( TXT( "Never back down" ) );
	return TASKNAME;
}

void CMoveSTNeverBackDown::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_cosAngle;
	compiler << i_sinAngle;
}
void CMoveSTNeverBackDown::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	TBaseClass::OnInitData( agent, data );

	data[ i_cosAngle ] = cosf( DEG2RAD( m_maxAngleFromTarget ) );
	data[ i_sinAngle ] = sinf( DEG2RAD( m_maxAngleFromTarget ) );
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSTFaceTarget
///////////////////////////////////////////////////////////////////////////////
void CMoveSTFaceTarget::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	Vector targetPos;
	if ( !GetTargetPosition( goal, targetPos ) )
	{
		return;
	}
	CMovingAgentComponent& mac = comm.GetAgent();
	const Vector& myPos = mac.GetWorldPositionRef();

	// calculate distances
	Vector2 toTarget = targetPos.AsVector2() - myPos.AsVector2();

	Float desiredYaw					= EulerAngles::YawFromXY( toTarget.X, toTarget.Y );
	Float currYaw						= mac.GetWorldYaw();
	Float yawDiff						= EulerAngles::AngleDistance( currYaw, desiredYaw );
	comm.LockRotation( yawDiff );
}

String CMoveSTFaceTarget::GetTaskName() const
{
	static const String TASKNAME( TXT( "Face target" ) );
	return TASKNAME;
}