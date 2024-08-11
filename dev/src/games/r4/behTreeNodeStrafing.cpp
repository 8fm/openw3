/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodeStrafing.h"

#include "behTreeStrafingAlgorithm.h"
#include "../../common/game/behTreeInstance.h"
#include "../../common/game/movementGoal.h"
#include "../../common/core/mathUtils.h"

//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeStrafingDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ ) const 
{
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeStrafingInstance::CBehTreeNodeStrafingInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeCustomSteeringInstance( def, owner, context, parent )
	, m_updateFrequency( def.m_updateFrequency.GetVal( context ) )
	, m_steeringSpeed( def.m_steeringSpeed.GetVal( context ) )
	, m_steeringImportance( def.m_steeringImportance.GetVal( context ) )
	, m_accelerationRate( def.m_accelerationRate.GetVal( context ) )
	, m_strafingWeight( def.m_strafingWeight.GetVal( context ) )
	, m_keepDistanceWeight( def.m_keepDistanceWeight.GetVal( context ) )
	, m_randomStrafeWeight( def.m_randomStrafeWeight.GetVal( context ) )
	, m_randomizationFrequency( def.m_randomizationFrequency.GetVal( context ) )
	, m_minRange( def.m_minRange.GetVal( context ) )
	, m_desiredSeparationAngle( def.m_desiredSeparationAngle.GetVal( context ) )
	, m_currentWorldHeading( 0.f, 0.f )
	, m_updateTimeout( 0.f )
	, m_randomizer( 0.f, 0.f )
	, m_randomizerTimeout( 0.f )
	, m_strafingRing( Int16(def.m_strafingRing.GetVal( context )) )
	, m_gravityToSeparationAngle( def.m_gravityToSeparationAngle.GetVal( context ) )
	, m_lockOrientationByDefault( def.m_lockOrientation.GetVal( context ) )
	, m_customAlgorithm( NULL )
{
	m_rangeSpan = def.m_maxRange.GetVal( context ) - m_minRange;
	if ( def.m_customAlgorithm )
	{
		m_customAlgorithm = def.m_customAlgorithm->SpawnInstance( this, context );
	}
}

Bool CBehTreeNodeStrafingInstance::Activate()
{
	CActor* target = m_owner->GetCombatTarget().Get();

	if ( !target )
	{
		DebugNotifyActivationFail();
		return false;
	}

	CCombatDataComponent* targetData = (m_targetData = target);
	if ( !targetData )
	{
		DebugNotifyActivationFail();
		return false;
	}

	Int32 attackerIndex = targetData->GetAttackerIndex( m_owner->GetActor() );
	if ( !CCombatDataComponent::IsAttackerIndexValid( attackerIndex ) )
	{
		ASSERT( false );
		DebugNotifyActivationFail();
		return false;
	}

	targetData->SetAttackerActionRing( attackerIndex, m_strafingRing );
	targetData->SetAttackerDesiredSeparation( attackerIndex, m_desiredSeparationAngle );

	// reset/initialize runtime data
	m_currentWorldHeading.Set( 0.f, 0.f );
	m_updateTimeout = 0.f;
	m_randomizer.Set( 0.f, 0.f );
	m_randomizerTimeout = 0.f;
	m_desiredAngleDistance = 0.f;
	m_currentTargetDistance = 0.f;
	m_customSpeed = 0.f;
	m_lockOrientation = m_lockOrientationByDefault;
	m_useCustomSpeed = false;
	m_currDesiredDistance = m_minRange + m_rangeSpan * 0.5f;
	m_desiredRangeTimeout = 0.f;

	if ( m_customAlgorithm )
	{
		m_customAlgorithm->Activate();
	}

	return Super::Activate();
}

void CBehTreeNodeStrafingInstance::Deactivate()
{
	CCombatDataComponent* targetData = m_targetData;
	if ( targetData )
	{
		Int32 attackerIndex = targetData->GetAttackerIndex( m_owner->GetActor() );
		if ( CCombatDataComponent::IsAttackerIndexValid( attackerIndex ) )
		{
			targetData->ClearAttackerActionRing( attackerIndex );
			// TODO: Ugly but disscusable
			// we can keep our separation angle 'for others'
			//targetData->ClearAttackerDesiredSeparation( attackerIndex );
		}
	}
	
	m_targetData.Clear();

	if ( m_customAlgorithm )
	{
		m_customAlgorithm->Deactivate();
	}

	Super::Deactivate();
}

void CBehTreeNodeStrafingInstance::Update()
{
}


Bool CBehTreeNodeStrafingInstance::IsFinished() const
{
	return false;
}

void CBehTreeNodeStrafingInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
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

	goal.SetFulfilled( false );

	CCombatDataComponent* targetData = m_targetData;

	if ( !targetData )
	{
		return;
	}

	CActor* actor = m_owner->GetActor();
	CActor* target = m_owner->GetCombatTarget().Get();
	if ( !target )
	{
		return;
	}

	targetData->ComputeYawDistances();

	Int32 attackerIndex = targetData->GetAttackerIndex( actor );
	if ( !CCombatDataComponent::IsAttackerIndexValid( attackerIndex ) )
	{
		ASSERT( false, TXT("Strafing guy is not on attackers list!") );
		return;
	}
	const CCombatDataComponent::CAttacker& attackerData = targetData->GetAttackerData( attackerIndex );

	// side strafing update
	Float desiredYaw = attackerData.m_worldSpaceYaw;
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

	// based on desired yaw determine side strafing steering component
	Float sideStrafing = 0.f;

	const Float SMOOTHING_ANGLE = 30.f;
	Float yawDiff = MathUtils::GeometryUtils::ClampDegrees( desiredYaw - attackerData.m_worldSpaceYaw );
	m_desiredAngleDistance = yawDiff;
	Float absYawDiff = Abs( yawDiff );

	if ( absYawDiff > SMOOTHING_ANGLE )
	{
		sideStrafing = yawDiff > 0.f ? 1.f : -1.f;
	}
	else if ( absYawDiff > 1.f )
	{
		sideStrafing = yawDiff / SMOOTHING_ANGLE;
	}

	sideStrafing *= m_strafingWeight;

	// determine keep distance steering component
	Float keepDistance = 0.f;
	Float currentTime = m_owner->GetLocalTime();

	const Float SMOOTHING_DISTANCE = 2.f;
	if ( m_desiredRangeTimeout < currentTime )
	{
		m_currDesiredDistance = m_minRange;
		if ( m_rangeSpan )
		{
			m_currDesiredDistance += GEngine->GetRandomNumberGenerator().Get< Float >( m_rangeSpan );
		}
		m_desiredRangeTimeout = currentTime + (GEngine->GetRandomNumberGenerator().Get< Float >() + 0.5f) * m_randomizationFrequency;
	}

	
	m_currentTargetDistance = sqrt( attackerData.m_distance2DSq );
	if ( m_currentTargetDistance > m_currDesiredDistance + 0.25f )
	{
		Float diff = m_currentTargetDistance - m_currDesiredDistance;
		if ( diff > SMOOTHING_DISTANCE )
		{
			keepDistance = 1.f;
		}
		else
		{
			keepDistance = diff / SMOOTHING_DISTANCE;
		}
	}
	else if ( m_currentTargetDistance < m_currDesiredDistance - 0.25f )
	{
		Float diff = m_currDesiredDistance - m_currentTargetDistance;
		if ( diff > SMOOTHING_DISTANCE )
		{
			keepDistance = -1.f;
		}
		else
		{
			keepDistance = -diff / SMOOTHING_DISTANCE;
		}
	}

	keepDistance *= m_keepDistanceWeight;

	// add randomization
	if ( m_randomizerTimeout < currentTime )
	{
		m_randomizer.Set( GEngine->GetRandomNumberGenerator().Get< Float >( -1.f , 1.f ), 0.f );
		m_randomizer *= m_randomStrafeWeight;
		m_randomizerTimeout = currentTime + (GEngine->GetRandomNumberGenerator().Get< Float >() + 0.5f) * m_randomizationFrequency;
	}

	// compute final output
	Float orientation = attackerData.m_worldSpaceYaw + 180.f;

	Vector2 heading( sideStrafing, keepDistance );
	heading += m_randomizer;

	// normalize output
	Float outputLenSq = heading.SquareMag();
	if ( outputLenSq < 0.05f )
	{
		heading.Set( 0.f, 0.f );
	}
	else if ( outputLenSq > 1.f )
	{
		heading *= 1.f / sqrt( outputLenSq );
	}

	if ( m_customAlgorithm )
	{
		m_customAlgorithm->UpdateHeading( heading );
	}

	// look orientation
	Float lockOrientation;
	if ( m_lockOrientation )
	{
		lockOrientation = orientation;
	}
	else
	{
		lockOrientation = EulerAngles::YawFromXY( heading.X, heading.Y );
	}

	// rotate output to world coordinates
	heading = MathUtils::GeometryUtils::Rotate2D( heading, DEG2RAD( orientation ) );

	// interpolate output with acceleration
	{
		Float outputChange = timeDelta * m_accelerationRate;
		Vector2 headingChange = heading - m_currentWorldHeading;
		Float diffSq = headingChange.SquareMag();
		if ( diffSq <= (outputChange*outputChange))
		{
			m_currentWorldHeading = heading;
		}
		else
		{
			m_currentWorldHeading = m_currentWorldHeading + headingChange * ( outputChange / sqrt( diffSq ) );
		}
	}

	Float headingLen = m_currentWorldHeading.Mag();
	Float speedGoal = headingLen;

	// use speed multiplier
	speedGoal *= m_useCustomSpeed ? m_customSpeed : m_steeringSpeed;

	goal.SetOrientationGoal( agent, orientation );
	if ( headingLen > 0.05f )
	{
		// update movement goal
		goal.SetSpeedGoal( speedGoal );
		goal.SetHeadingGoal( agent, m_currentWorldHeading * (1.f/headingLen), false );
		goal.SetDistanceToGoal( headingLen * 5.f );
		goal.MatchOrientationWithHeading( !m_lockOrientation );
		goal.SetHeadingImportanceMultiplier( m_steeringImportance );
	}
	else
	{
		goal.SetSpeedGoal( 0.f );
		goal.SetHeadingGoal( agent, Vector2( 0,0 ), false );
		goal.SetDistanceToGoal( 0.f );
		goal.MatchOrientationWithHeading( !m_lockOrientation );
		goal.SetHeadingImportanceMultiplier( m_steeringImportance );
	}

	goal.SetGoalTargetNode( target );
}


Bool CBehTreeNodeStrafingInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventType == BTET_GameplayEvent )
	{
		if ( e.m_eventName == CNAME( AI_Strafing_GetAngleDiff ) )
		{
			if ( e.m_gameplayEventData.m_customData && e.m_gameplayEventData.m_customDataType->GetType() == RT_Class )
			{
				CClass* classId = (CClass*)e.m_gameplayEventData.m_customDataType;
				SGameplayEventParamFloat* data = Cast < SGameplayEventParamFloat > ( classId, const_cast< CBehTreeEvent& >( e ).m_gameplayEventData.m_customData );
				if ( data )
				{
					data->m_value = Abs( m_desiredAngleDistance );
				}
			}
			return false;
		}

		if ( e.m_eventName == CNAME( AI_Strafing_LockOrientation ) )
		{
			if ( e.m_gameplayEventData.m_customData && e.m_gameplayEventData.m_customDataType->GetType() == RT_Class )
			{
				CClass* classId = (CClass*)e.m_gameplayEventData.m_customDataType;
				SGameplayEventParamInt* data = Cast < SGameplayEventParamInt > ( classId, const_cast< CBehTreeEvent& >( e ).m_gameplayEventData.m_customData );
				if ( data )
				{
					m_lockOrientation = data->m_value != 0;
				}
			}
			return false;
		}
		
		if ( e.m_eventName == CNAME( AI_Strafing_SetMovementSpeed ) )
		{
			if ( e.m_gameplayEventData.m_customData && e.m_gameplayEventData.m_customDataType->GetType() == RT_Class )
			{
				CClass* classId = (CClass*)e.m_gameplayEventData.m_customDataType;
				SGameplayEventParamInt* data = Cast < SGameplayEventParamInt > ( classId, const_cast< CBehTreeEvent& >( e ).m_gameplayEventData.m_customData );
				if ( data )
				{
					CActor* actor = m_owner->GetActor();
					if( actor )
					{
						CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
						if( mac )
						{
							EMoveType moveType = (EMoveType)data->m_value;
							m_useCustomSpeed = true;
							m_customSpeed = mac->GetSpeedForMoveType( moveType, 0.0f );
							mac->SetMoveType( moveType );
							ActorActionCustomSteer* customAction = actor->GetActionCustomSteer();
							customAction->SetMoveType( moveType );
						}
					}
				}
			}
			return false;
		}
	}


	return Super::OnEvent( e );
}