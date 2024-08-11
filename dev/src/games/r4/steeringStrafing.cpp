#include "build.h"
#include "steeringStrafing.h"

#include "../../common/core/instanceDataLayoutCompiler.h"
#include "../../common/core/mathUtils.h"

#include "../../common/engine/renderFrame.h"

#include "../../common/game/movementCommandBuffer.h"
#include "../../common/game/movementGoal.h"

#include "combatDataComponent.h"


IMPLEMENT_ENGINE_CLASS( IMoveSTBaseStrafeTarget );
IMPLEMENT_ENGINE_CLASS( CMoveSTStrafeSurroundTarget );
IMPLEMENT_ENGINE_CLASS( CMoveSTStrafeTargetRandomly );
IMPLEMENT_ENGINE_CLASS( CMoveSTStrafeTargetOneWay );


///////////////////////////////////////////////////////////////////////////////
// CMoveSTStrafeSurroundTarget
///////////////////////////////////////////////////////////////////////////////
Float IMoveSTBaseStrafeTarget::CalculateDesiredOutput( IMovementCommandBuffer& comm, InstanceBuffer& data, const Vector& targetPos, Float timeDelta ) const
{
	return 1.f;
}
void IMoveSTBaseStrafeTarget::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	Vector targetPos;
	if ( !GetTargetPosition( goal, targetPos ) )
	{
		return;
	}
	CMovingAgentComponent& mac = comm.GetAgent();
	
	const Vector& myPos = mac.GetWorldPositionRef();

	// compute desired output
	const Float desiredOutput = CalculateDesiredOutput( comm, data, targetPos, timeDelta );

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

	// calculate distances
	

	Vector2 toTarget = targetPos.AsVector2() - myPos.AsVector2();

	Vector2 steerVec = MathUtils::GeometryUtils::PerpendicularL( toTarget.Normalized() );
	steerVec *= currentOutput;

	Float importanceMult = Abs( currentOutput );
	if ( importanceMult > 0.f )
	{
		// setup steering
		comm.AddHeading( steerVec, m_importance * importanceMult );
		comm.AddSpeed( m_moveSpeed, m_importance * importanceMult );
	}
}

String IMoveSTBaseStrafeTarget::GetTaskName() const 
{
	static const String TASKNAME( TXT( "ABSTRACT[ Strafe ]" ) );
	return TASKNAME;
}

void IMoveSTBaseStrafeTarget::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_lastOutput;
}
void IMoveSTBaseStrafeTarget::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	data[ i_lastOutput ] = 0.f;
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSTStrafeSurroundTarget
///////////////////////////////////////////////////////////////////////////////

void CMoveSTStrafeSurroundTarget::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	struct Local
	{
		static Float HalfAngle( Float leftAngle, Float rightAngle )
		{
			Float angle = ( leftAngle + rightAngle ) / 2.f;
			return leftAngle >= rightAngle ? angle : MathUtils::GeometryUtils::ClampDegrees( angle + 180.f );
		}
		static Float AngleDiff( Float leftAngle, Float rightAngle )
		{
			Float angle = leftAngle - rightAngle;
			return angle > 0 ? angle : 360.f + angle;
		}
	};

	SMoveLocomotionGoal& goal = comm.GetGoal();
	CNode* targetNode = GetTarget( goal );
	if ( !targetNode )
	{
		return;
	}
	CMovingAgentComponent& mac = comm.GetAgent();
	CActor* actor = Cast< CActor >( mac.GetEntity() );
	//////////////////
	// TODO: OPTIMIZE!
	CActor* targetActor = Cast< CActor >( targetNode );
	if ( !targetActor )
	{
		return;
	}
	CCombatDataPtr targetDataPtr( targetActor );
	CCombatDataComponent* targetData = targetDataPtr.Get();
	if ( !targetData )
	{
		return;
	}
	//////////////////
	targetData->ComputeYawDistances();
	Int32 attackerIndex = targetData->GetAttackerIndex( actor );
	if ( !CCombatDataComponent::IsAttackerIndexValid( attackerIndex ) )
	{
		ASSERT( false, TXT("Strafing guy is not on attackers list!") );
		return;
	}
	targetData->SetAttackerActionRing( attackerIndex, Int16(m_strafingRing) );
	targetData->SetAttackerDesiredSeparation( attackerIndex, m_desiredSeparationAngle );
	const CCombatDataComponent::CAttacker& attackerData = targetData->GetAttackerData( attackerIndex );
	Float desiredYaw = attackerData.m_worldSpaceYaw;

	// side strafing update
	{
		Int32 leftAttackerIndex = targetData->LeftAttackerIndex( attackerIndex );
		if ( leftAttackerIndex != attackerIndex )
		{
			Int32 rightAttackerIndex = targetData->RightAttackerIndex( attackerIndex );
			const CCombatDataComponent::CAttacker& leftAttackerData = targetData->GetAttackerData( leftAttackerIndex );
			const CCombatDataComponent::CAttacker& rightAttackerData = targetData->GetAttackerData( rightAttackerIndex );

			Float desiredSeparationLeft = Max( m_desiredSeparationAngle, leftAttackerData.m_desiredSeparation );
			Float desiredSeparationRight = Max( m_desiredSeparationAngle, rightAttackerData.m_desiredSeparation );

			Float anglesDiff = Local::AngleDiff( leftAttackerData.m_worldSpaceYaw, rightAttackerData.m_worldSpaceYaw );


			if ( desiredSeparationLeft + desiredSeparationRight > anglesDiff )
			{
				// both repulsion zones are overlapping
				Float halfAngleDiff = anglesDiff / 2.f;
				Bool repulseLeft = halfAngleDiff <= desiredSeparationLeft;
				Bool repulseRight = halfAngleDiff <= desiredSeparationRight;
				if ( repulseLeft && repulseRight )
				{
					// both repulsion zones are overlapping point in a middle
					desiredYaw = Local::HalfAngle( leftAttackerData.m_worldSpaceYaw, rightAttackerData.m_worldSpaceYaw );
				}
				else if ( repulseLeft )
				{
					desiredYaw = MathUtils::GeometryUtils::ClampDegrees( rightAttackerData.m_worldSpaceYaw + desiredSeparationRight );
				}
				else
				{
					ASSERT( repulseRight );
					desiredYaw = MathUtils::GeometryUtils::ClampDegrees( leftAttackerData.m_worldSpaceYaw - desiredSeparationLeft );
				}
			}
			else
			{
				// repulsion zones are not overlapping
				// so we need to determine which repulsion zone npc is and base desired yaw on that fact
				Float diffLeft = Local::AngleDiff( leftAttackerData.m_worldSpaceYaw, attackerData.m_worldSpaceYaw );
				Float diffRight = Local::AngleDiff( attackerData.m_worldSpaceYaw, rightAttackerData.m_worldSpaceYaw );

				if ( diffLeft < desiredSeparationLeft )
				{
					desiredYaw = MathUtils::GeometryUtils::ClampDegrees( leftAttackerData.m_worldSpaceYaw - desiredSeparationLeft );
				}
				else if ( diffRight < desiredSeparationRight )
				{
					desiredYaw = MathUtils::GeometryUtils::ClampDegrees( rightAttackerData.m_worldSpaceYaw + desiredSeparationRight );
				}
				else if ( m_gravityToSeparationAngle )
				{
					if ( diffLeft - desiredSeparationLeft < diffRight - desiredSeparationRight )
					{
						desiredYaw = MathUtils::GeometryUtils::ClampDegrees( leftAttackerData.m_worldSpaceYaw - desiredSeparationLeft );
					}
					else
					{
						desiredYaw = MathUtils::GeometryUtils::ClampDegrees( rightAttackerData.m_worldSpaceYaw + desiredSeparationRight );
					}
				}
				// else no strafing
			}
		}
	}

	// based on desired yaw determine side strafing steering component
	Float yawDiff = MathUtils::GeometryUtils::ClampDegrees( desiredYaw - attackerData.m_worldSpaceYaw );
	Float absYawDiff = Abs( yawDiff );

	goal.SetFlag( CNAME( DesiredTargetYawDifference ), yawDiff );

	Float desiredStrafing = 0.f;
	if ( absYawDiff > m_smoothAngle )
	{
		desiredStrafing = yawDiff >= 0.f ? 1.f : -1.f;
	}
	else if ( absYawDiff > m_toleranceAngle )
	{
		desiredStrafing = (absYawDiff - m_toleranceAngle) / (m_smoothAngle - m_toleranceAngle);
		if ( yawDiff < 0.f )
		{
			desiredStrafing = -desiredStrafing;
		}
	}
	// compute final output
	Float orientation = attackerData.m_worldSpaceYaw + 180.f;

	// blend output
	const Float desiredOutput = desiredStrafing;
	const Float lastOutput = data[ i_lastOutput ];
	const Float outputDiff = desiredOutput - lastOutput;
	const Float outputDiffAbs = Abs( outputDiff );

	Float maxChange = timeDelta * m_acceleration;

	Float currentStrafing;

	if( outputDiffAbs <= maxChange )
	{
		currentStrafing = desiredOutput;
	}
	else
	{
		currentStrafing = lastOutput + (outputDiff >= 0.f ? maxChange : -maxChange);
	}

	data[ i_lastOutput ] = currentStrafing;

	Float importanceMult = Abs( currentStrafing );

	if ( importanceMult > NumericLimits< Float >::Epsilon() )
	{
		Vector2 heading = MathUtils::GeometryUtils::Rotate2D( Vector2( currentStrafing, 0.f ), DEG2RAD( orientation ) );

		// setup steering
		comm.AddHeading( heading, m_importance * importanceMult );
		comm.AddSpeed( m_moveSpeed, m_importance * importanceMult );
	}

}

String CMoveSTStrafeSurroundTarget::GetTaskName() const
{
	static const String TASKNAME( TXT( "Strafe surround" ) );
	return TASKNAME;
}
void CMoveSTStrafeSurroundTarget::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_lastOutput;
}
void CMoveSTStrafeSurroundTarget::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	TBaseClass::OnInitData( agent, data );

	data[ i_lastOutput ] = 0.f;
}
///////////////////////////////////////////////////////////////////////////////
// CMoveSTStrafeTargetRandomly
///////////////////////////////////////////////////////////////////////////////
Float CMoveSTStrafeTargetRandomly::CalculateDesiredOutput( IMovementCommandBuffer& comm, InstanceBuffer& data, const Vector& targetPos, Float timeDelta ) const
{
	// first update decision process
	Float& currentOutput = data[ i_desiredOutput ];

	EngineTime gameTime = GGame->GetEngineTime();
	if ( gameTime >= data[ i_decisionTimeout ] )
	{
		data[ i_decisionTimeout ] = gameTime + m_randomizationFrequency * ( 0.5f + GEngine->GetRandomNumberGenerator().Get< Float >() );

		Float output = 1.f;
		if ( m_outputRandomizationPower )
		{
			output = ::powf( GEngine->GetRandomNumberGenerator().Get< Float >(), m_outputRandomizationPower );
		}
		if ( GEngine->GetRandomNumberGenerator().Get< Uint16 >() & 1 )
		{
			output = -output;
		}

		currentOutput = output;
	}
	else if ( m_changeDirectionOnBlockDelay >= 0.f && gameTime >= data[ i_blockTestLockTimeout ] && fabs( currentOutput ) >= 0.5f )
	{
		// check if we are blocked
		Bool& wasBlocked = data[ i_isBlocked ];
		Bool isBlockedNow = false;
		
		CMovingAgentComponent& mac = comm.GetAgent();
		const Vector& actorPos = mac.GetWorldPositionRef();
		
		Vector2 actorMovement = mac.GetSteeringVelocity();
		if ( actorMovement.SquareMag() < 0.1f*0.1f )
		{
			isBlockedNow = true;
		}
		else
		{
			Vector2 fromTargetDiff = actorPos.AsVector2() - targetPos.AsVector2();
			Vector2 desiredWalkDir = MathUtils::GeometryUtils::PerpendicularR( fromTargetDiff );

			// we dont care about desiredWalkDir length - only heading
			desiredWalkDir *= data[ i_desiredOutput ];

			// check if we are moving in opposite direction to desired
			if ( desiredWalkDir.Dot( actorMovement ) < 0.f )
			{
				isBlockedNow = true;
			}
		}

		if ( isBlockedNow )
		{
			if ( wasBlocked )
			{
				if ( gameTime >= data[ i_blockTestTimeout ]  )
				{
					data[ i_blockTestLockTimeout ] = gameTime + m_changeDirectionOnBlockDelay * GEngine->GetRandomNumberGenerator().Get< Float >( 0.75f, 1.25f );
					data[ i_decisionTimeout ] = gameTime + m_randomizationFrequency * ( 0.5f + GEngine->GetRandomNumberGenerator().Get< Float >() );
					wasBlocked = false;

					currentOutput = -currentOutput;
				}
			}
			else
			{
				wasBlocked = true;
				data[ i_blockTestTimeout ] = gameTime + 0.5f;
			}
		}
		else
		{
			wasBlocked = false;
		}

	}

	// return current decision
	return currentOutput;
}

String CMoveSTStrafeTargetRandomly::GetTaskName() const
{
	static const String TASKNAME( TXT( "Strafe randomly" ) );
	return TASKNAME;
}

void CMoveSTStrafeTargetRandomly::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_desiredOutput;
	compiler << i_decisionTimeout;
	compiler << i_blockTestTimeout;
	compiler << i_blockTestLockTimeout;
	compiler << i_isBlocked;

	TBaseClass::OnBuildDataLayout( compiler );
}
void CMoveSTStrafeTargetRandomly::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	data[ i_desiredOutput ] = 0.f;
	data[ i_decisionTimeout ] = EngineTime::ZERO;
	data[ i_blockTestTimeout ] = EngineTime::ZERO;
	data[ i_blockTestLockTimeout ] = EngineTime::ZERO;
	data[ i_isBlocked ] = false;

	TBaseClass::OnInitData( agent, data );
}

void CMoveSTStrafeTargetRandomly::OnGraphDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const
{
	data[ i_desiredOutput ] = 0.f;
	data[ i_decisionTimeout ] = EngineTime::ZERO;
	data[ i_blockTestTimeout ] = EngineTime::ZERO;
	data[ i_blockTestLockTimeout ] = EngineTime::ZERO;
	data[ i_isBlocked ] = false;

	TBaseClass::OnGraphDeactivation( agent, data );
}
void CMoveSTStrafeTargetRandomly::GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const
{
	InstanceBuffer* data = agent.GetCurrentSteeringRuntimeData();
	if ( data == nullptr )
	{
		return;
	}

	if ( (*data)[ i_isBlocked ] )
	{
		Float timeToSwitchDir = Max( 0.f, Float( (*data)[ i_blockTestTimeout ] - GGame->GetEngineTime() ) );
		String desc = String::Printf( TXT( "Blocked %0.1f" ), timeToSwitchDir );
		frame->AddDebugText( agent.GetWorldPositionRef(), desc, -25, -3, true, Color::LIGHT_YELLOW );
	}

	TBaseClass::GenerateDebugFragments( agent, frame );
}
///////////////////////////////////////////////////////////////////////////////
// CMoveSTStrafeTargetOneWay
///////////////////////////////////////////////////////////////////////////////
Float CMoveSTStrafeTargetOneWay::CalculateDesiredOutput( IMovementCommandBuffer& comm, InstanceBuffer& data, const Vector& targetPos, Float timeDelta ) const 
{
	return m_left ? -1.f : 1.f;
}

String CMoveSTStrafeTargetOneWay::GetTaskName() const
{
	static const String TASKNAME( TXT( "Strafe one way" ) );
	return TASKNAME;
}