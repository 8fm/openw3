/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "generalMoveSteeringTask.h"

#include "../core/instanceDataLayoutCompiler.h"
#include "../core/mathUtils.h"

#include "../engine/behaviorGraphUtils.inl"
#include "../engine/behaviorGraphStack.h"
#include "../engine/pathlibCollectCollisionPointsSpatialQuery.h"
#include "../engine/pathlibWorld.h"
#include "../engine/physicsCharacterWrapper.h"


#include "actorsManager.h"
#include "gameplayStorage.h"
#include "movableRepresentationPathAgent.h"
#include "movementAdjustor.h"
#include "movementGoal.h"
#include "movingPhysicalAgentComponent.h"
#include "navigationCollectCollision.h"


IMPLEMENT_ENGINE_CLASS( CMoveSTResetSteering );
IMPLEMENT_ENGINE_CLASS( CMoveSTStop );
IMPLEMENT_ENGINE_CLASS( CMoveSTStopOnFreezing );
IMPLEMENT_ENGINE_CLASS( CMoveSTRotate );
IMPLEMENT_ENGINE_CLASS( CMoveSTSnapToMinimalVelocity );
IMPLEMENT_ENGINE_CLASS( CMoveSTMove );
IMPLEMENT_ENGINE_CLASS( CMoveSTMoveWithOffset );
IMPLEMENT_ENGINE_CLASS( CMoveSTFinalStep );
IMPLEMENT_ENGINE_CLASS( CMoveSTKeepNavdata );
IMPLEMENT_ENGINE_CLASS( CMoveSTMatchHeadingOrientation );
IMPLEMENT_ENGINE_CLASS( CMoveSTKeepAwayWalls );
IMPLEMENT_ENGINE_CLASS( CMoveSTKeepAwayWallsInPathfollow );
IMPLEMENT_ENGINE_CLASS( CMoveSTSeparateFromActors );
IMPLEMENT_ENGINE_CLASS( CMoveSTArrive );
IMPLEMENT_ENGINE_CLASS( CMoveSTApplySteering );
IMPLEMENT_ENGINE_CLASS( CMoveSTApplyAnimationSteering );
IMPLEMENT_ENGINE_CLASS( CMoveSTStep );
IMPLEMENT_ENGINE_CLASS( CMoveSTSlide );
IMPLEMENT_ENGINE_CLASS( CMoveSTSetBehaviorVariable );
IMPLEMENT_ENGINE_CLASS( CMoveSTChangeSpeed );
IMPLEMENT_ENGINE_CLASS( CMoveSTSetFlags );
IMPLEMENT_RTTI_ENUM( EBitOperation );
IMPLEMENT_ENGINE_CLASS( CMoveSTSetupRotationChange );
IMPLEMENT_ENGINE_CLASS( CMoveSTMapRotationChangeUsingCustomRotation );
IMPLEMENT_ENGINE_CLASS( CMoveSTSetMaxDirectionChange );
IMPLEMENT_ENGINE_CLASS( CMoveSTSetMaxRotationChange );
IMPLEMENT_ENGINE_CLASS( CMoveSTSetAcceleration );
IMPLEMENT_RTTI_ENUM( EAvoidObstacleStrategy );
IMPLEMENT_ENGINE_CLASS( CMoveSTAvoidObstacles );
IMPLEMENT_ENGINE_CLASS( CMoveSTCollisionResponse );
IMPLEMENT_ENGINE_CLASS( CMoveSTForwardCollisionResponse );
IMPLEMENT_RTTI_ENUM( EAdjustRotationChangesScenario );
IMPLEMENT_ENGINE_CLASS( CMoveSTAdjustRotationChanges );
IMPLEMENT_ENGINE_CLASS( CMoveSTKeepAwayTarget );
IMPLEMENT_ENGINE_CLASS( CMoveSTMoveTightening );
IMPLEMENT_ENGINE_CLASS( CMoveSTResolveStucking );

RED_DEFINE_STATIC_NAME( rotationBlend );
RED_DEFINE_STATIC_NAME( rotateAngle );
RED_DEFINE_STATIC_NAME( StepLength );
RED_DEFINE_STATIC_NAME( Rotate );
RED_DEFINE_STATIC_NAME( RotateEnd );
RED_DEFINE_STATIC_NAME( Step );
RED_DEFINE_STATIC_NAME( StepEnd );
RED_DEFINE_STATIC_NAME( ShouldBeUsed );
RED_DEFINE_STATIC_NAME( GetValueToMap );
RED_DEFINE_STATIC_NAME( GetTaskName );
RED_DEFINE_STATIC_NAME( MovementAdjustmentLocation );
RED_DEFINE_STATIC_NAME( FinalStepDistance );
RED_DEFINE_STATIC_NAME( FinalStep );
RED_DEFINE_STATIC_NAME( MovementAdjustmentActive );
RED_DEFINE_STATIC_NAME( KeepNavdata )


///////////////////////////////////////////////////////////////////////////////
// CMoveSTRotate
///////////////////////////////////////////////////////////////////////////////
void CMoveSTRotate::ProcessRotation( IMovementCommandBuffer& comm ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();

	if ( goal.IsOrientationGoalSet() )
	{
		comm.AddRotation( goal.GetOrientationDiff(), 1.f );
	}
}


void CMoveSTRotate::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	ProcessRotation( comm );
}

String CMoveSTRotate::GetTaskName() const
{
	return TXT("Rotate");
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSTSnapToMinimalVelocity
///////////////////////////////////////////////////////////////////////////////
void CMoveSTSnapToMinimalVelocity::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	if ( goal.ShouldSnapToMinimalSpeed() )
	{
		const Vector2& currHeading = comm.GetHeading();
		Float headingLenSq = currHeading.SquareMag();
		Float minSpeed = m_minVelocity+0.01f;
		if ( headingLenSq > (0.01f*0.01f) && headingLenSq < (minSpeed*minSpeed) )
		{
			Float headingLen = sqrt( headingLenSq );
			comm.ModifyHeading( currHeading * (minSpeed / headingLen) );
		}
	}
}

String CMoveSTSnapToMinimalVelocity::GetTaskName() const
{
	static const String STR( TXT( "Snap to min velocity" ) );
	return STR;
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSTMove
///////////////////////////////////////////////////////////////////////////////
CMoveSTMove::CMoveSTMove()
	: m_headingImportance( 1.f )
	, m_speedImportance( 1.f )
{}
void CMoveSTMove::ProcessMovement( IMovementCommandBuffer& comm ) const
{
	const CMovingAgentComponent& agent = comm.GetAgent();
	const SMoveLocomotionGoal& goal = comm.GetGoal();

	// velocity goal execution

	// canUseAnimationToRotate forces the agent to play a rotation animation instead of just sliding if the angle is greater then 30 degrees.
	// The reason is for the rotation to look more natural. Unfortunately the side effect is that first of all
	// we need to have the animations (which we don't at this point) and it creates an input lag if the agent is player.
	// Moreover there is a 1 second timeout for the notification of the rotation animation end in which we lose all the control
	// and the agent just stays there doing nothing and not rotating at all.

	if ( goal.IsHeadingGoalSet() )
	{
		Vector2 vel = goal.GetHeadingToGoal();
		comm.AddHeading( vel, m_headingImportance * goal.GetHeadingImportanceMultiplier() );

		if ( m_speedImportance > 0.f )
		{
			Float speed = goal.IsSpeedGoalSet() ? (goal.GetDesiredSpeed() / agent.GetMaxSpeed()) : 1.f;
			ASSERT( speed <= 1.0f, TXT("Speed should not be above 1 here !") );
			comm.AddSpeed( speed, m_speedImportance );
		}
	}

	// orientation goal execution
	ProcessRotation( comm );
}
void CMoveSTMove::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	ProcessMovement( comm );

	const SMoveLocomotionGoal& goal = comm.GetGoal();
	if ( goal.IsDistanceToDestinationSet() )
	{
		Float tolerance = goal.IsGoalToleranceSet() ? goal.GetGoalTolerance() : 0.5f;
		if ( goal.GetDistanceToDestination() <= tolerance )
		{
			comm.SetDestinationReached();
		}
	}
}

String CMoveSTMove::GetTaskName() const
{
	static const String STR( TXT( "Move" ) );
	return STR;
}

CMoveSTMoveWithOffset::CMoveSTMoveWithOffset()
	: m_headingImportance( 1.f )
	, m_speedImportance( 1.f )
	, m_offset( .5f )
{}

void CMoveSTMoveWithOffset::ProcessMovement( IMovementCommandBuffer& comm, const Vector& target ) const
{
	const CMovingAgentComponent& agent = comm.GetAgent();
	const SMoveLocomotionGoal& goal = comm.GetGoal();

	Vector2 vel = target - agent.GetWorldPositionRef();
	comm.AddHeading( vel.Normalized(), m_headingImportance * goal.GetHeadingImportanceMultiplier() );

	if ( m_speedImportance > 0.f )
	{
		Float speed = goal.IsSpeedGoalSet() ? (goal.GetDesiredSpeed() / agent.GetMaxSpeed()) : 1.f;
		ASSERT( speed <= 1.0f, TXT("Speed should not be above 1 here !") );
		comm.AddSpeed( speed, m_speedImportance );
	}

	// orientation goal execution
	ProcessRotation( comm );
}

void CMoveSTMoveWithOffset::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();

	if ( !goal.IsDestinationPositionSet() )
	{
		return;
	}

	const CMovingAgentComponent& mac = comm.GetAgent();
	CPathAgent* pathAgent = mac.GetPathAgent();
	if ( !pathAgent )
	{
		return;
	}

	Vector target = goal.GetDestinationPosition();

	const Vector directionToTarget = target - mac.GetWorldPositionRef();
	const Float sign = MSign( mac.GetVelocity().AsVector2().CrossZ( directionToTarget ) );

	const Vector perpendicularL = MathUtils::GeometryUtils::PerpendicularL( directionToTarget ).Normalized() * m_offset * sign;

	if ( pathAgent->TestLine( target - perpendicularL ) )
	{
		target -= perpendicularL;
	}
	else if ( pathAgent->TestLine( target + perpendicularL ) )
	{
		target += perpendicularL;
	}

	ProcessMovement( comm, target );

	if ( goal.IsDistanceToDestinationSet() )
	{
		Float tolerance = goal.IsGoalToleranceSet() ? goal.GetGoalTolerance() : 0.5f;
		if ( mac.GetWorldPositionRef().DistanceTo( target ) <= tolerance )
		{
			comm.SetDestinationReached();
		}
	}
}

String CMoveSTMoveWithOffset::GetTaskName() const
{
	static const String STR( TXT( "Move with offset" ) );
	return STR;
}


///////////////////////////////////////////////////////////////////////////////
// CMoveSTFinalStep
//	What is needed:
//	1. add to steering graph "Move (with FinalStep)"
//	2. add to behavior graph:
//		"FinalStep" event (to trigger final step)
//		optional "FinalStepDistance" (in case you want to choose between different animations depending on distance)
//		"MovementAdjustmentLocation" vector variable (to know where we should be)
//		"MovementAdjustmentActive" float variable (to know if we are doing final step)
//		Add transitions to state handling final step 
//		State handling final step should have:
//			"FinalStep" activation notification
//			"FinalStep" deactivation notification (not required but advised)
//	3. add to animations used for stopping (those animations should be inside
//	   "final step" state)
//		Motion/Location adjustment events
//		those events create movement adjustor requests that do actual adjustments
//	4. or you could add final step transition
//
//	When "FinalStep" has begun (we know that because of activation notification)
//	character will have steering values set to "stopped" to not force it to make
//	another move.
//
//	Character will move towards destination point but will try to stop
//	at the distance of goal tolerance radius. This happens if "ignore goal tolerance"
//	is set to false. If it is set to true, character will stop exactly at the
//	destination point.
//
///////////////////////////////////////////////////////////////////////////////
CMoveSTFinalStep::CMoveSTFinalStep()
	: m_ignoreGoalToleranceForFinalLocation( false )
	, m_finalStepPositionVar( CNAME( MovementAdjustmentLocation ) )
	, m_finalStepDistanceVar( CNAME( FinalStepDistance ) )
	, m_finalStepActiveVar( CNAME( MovementAdjustmentActive ) )
	, m_finalStepEvent( CNAME( FinalStep ) )
	, m_finalStepActivationNotification( CNAME( FinalStep ) )
	, m_finalStepDeactivationNotification( CNAME( FinalStep ) )
	, m_finalStepDeactivationNotificationTimeOut( 0.5f )
	, m_finalStepDistanceLimit( 1.5f )
{}

void CMoveSTFinalStep::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_finalStepDeactivationTimeout;
	compiler << i_finalStepInProgress;
}

void CMoveSTFinalStep::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	TBaseClass::OnInitData( agent, data );

	data[ i_finalStepDeactivationTimeout ] = EngineTime::ZERO;
	data[ i_finalStepInProgress ] = false;
}

void CMoveSTFinalStep::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();

	Bool completedFinalStep = false;
	Bool& finalStepInProgress = data[ i_finalStepInProgress ];

	// handle ongoing final step
	if ( finalStepInProgress )
	{
		EngineTime& timeout = data [ i_finalStepDeactivationTimeout ];

		if ( timeout < GGame->GetEngineTime() )
		{
			finalStepInProgress = false;
			completedFinalStep = true;
		}
		else
		{
			CMovingAgentComponent& moveAgent = comm.GetAgent();
			if ( CBehaviorGraphStack * stack = moveAgent.GetBehaviorStack() )
			{
				if ( stack->DeactivationNotificationReceived( m_finalStepDeactivationNotification ) )
				{
					finalStepInProgress = false;
					completedFinalStep = true;
				}
			}
		}
	}

	// handle final step starting condition
	if ( goal.IsDistanceToDestinationSet() && goal.GetDistanceToDestination() <= ( goal.IsGoalToleranceSet() ? goal.GetGoalTolerance() : 0.5f ) )
	{
		Bool doStop = false;
		if ( completedFinalStep )
		{
			comm.SetDestinationReached();

			doStop = true;
		}
		else if ( finalStepInProgress )
		{
			doStop = true;
		}
		else
		{
			Bool finalStepIsEnabled = false;
			goal.GetFlag( CNAME( PreciseArrival ), finalStepIsEnabled );

			if ( finalStepIsEnabled )
			{
				CMovingAgentComponent& moveAgent = comm.GetAgent();

				// spam with event as it might not be started immediately
				Vector destinationPosition = goal.GetDestinationPosition();

				Float distanceToDestination =
					goal.IsDistanceToDestinationSet() ?
					Max( 0.0f, goal.GetDistanceToDestination() ) : 
					( destinationPosition.AsVector2() - moveAgent.GetWorldPosition().AsVector2() ).Mag();

				if ( distanceToDestination > m_finalStepDistanceLimit )
				{
					Float ratio = m_finalStepDistanceLimit / distanceToDestination;

					const Vector& pos = moveAgent.GetWorldPositionRef();

					Vector delta = destinationPosition - pos;
					delta *= ratio;

					distanceToDestination = m_finalStepDistanceLimit;
					destinationPosition = pos + delta;
				}

				comm.SetVar( BVC_Extra, m_finalStepActiveVar, 1.0f );
				comm.ClearVarOnNextTick( m_finalStepActiveVar );
				comm.SetVectorVar( m_finalStepPositionVar, destinationPosition );
				comm.SetVar( BVC_Extra, m_finalStepDistanceVar, distanceToDestination );

				Bool eventProcessed = false;
				if ( CBehaviorGraphStack * stack = moveAgent.GetBehaviorStack() )
				{
					eventProcessed = stack->GenerateBehaviorEvent( m_finalStepEvent );

					if ( stack->ActivationNotificationReceived( m_finalStepActivationNotification ) )
					{
						finalStepInProgress= true;
						data[ i_finalStepDeactivationTimeout ] = GGame->GetEngineTime() + m_finalStepDeactivationNotificationTimeOut;
					}
				}
				// if we don't support final step - progress
				if ( eventProcessed || finalStepInProgress )
				{
					doStop = true;
				}
			}
		}
		// does we support final step by ai?
		if ( doStop )
		{
			comm.LockSpeed( 0.0f );
			comm.LockRotation( 0.0f );
			comm.LockHeading( comm.GetAgent().GetWorldForward() );
			return;
		}

		comm.SetDestinationReached();
	}

	TBaseClass::ProcessMovement( comm );
}

String CMoveSTFinalStep::GetTaskName() const
{
	static const String STR( TXT( "Move (with FinalStep)" ) );
	return STR;
}


///////////////////////////////////////////////////////////////////////////////
// CMoveSTKeepNavdata
///////////////////////////////////////////////////////////////////////////////
void CMoveSTKeepNavdata::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	Bool& isSliding = data[ i_isSliding ];
	CMovingAgentComponent& moveAgent = comm.GetAgent();

	if ( ( !m_applyStandardConditions || comm.GetGoal().IsSet() || moveAgent.IsPulledToNavigableSpace() ) && !moveAgent.IsTeleported() )					// should enable snap to navdata algorithm
	{
		CPathAgent* pathAgent = moveAgent.GetPathAgent();

		// test if we should begin sliding
		if ( !isSliding && pathAgent && !pathAgent->IsOnNavdata() && moveAgent.IsSnapedToNavigableSpace() && !moveAgent.IsEntityRepresentationForced() )
		{
			CPathLibWorld& pathlib = *pathAgent->GetPathLib();
			const Vector3& agentPos = pathAgent->GetPosition();
			Vector3 returnPos;

			const Vector3& lastNavigableSpot = moveAgent.GetLastNavigableSpot().AsVector3();
			Float minZ = agentPos.Z - 2.f;
			Float maxZ = agentPos.Z + 1.f;
			Bool foundSafeSpot;
			Vector3 lastNavigableSpotDiff = lastNavigableSpot - agentPos;
			Float lastNavigableSpotDist2DSq = lastNavigableSpotDiff.AsVector2().SquareMag();

			if ( lastNavigableSpotDist2DSq < m_maxTeleportationRange * m_maxTeleportationRange && Abs( lastNavigableSpotDiff.Z ) < 2.f && lastNavigableSpotDist2DSq > NumericLimits< Float >::Epsilon() )
			{
				// use lastNavigableSpot to compute safe spot
				Vector2 preferedDir = lastNavigableSpotDiff.AsVector2() * (1.f / MSqrt( lastNavigableSpotDist2DSq ));
				foundSafeSpot = pathlib.FindSafeSpot( pathAgent->CacheAreaId(), agentPos, m_maxTeleportationRange, pathAgent->GetPersonalSpace() + 0.01f, returnPos, &minZ, &maxZ, pathAgent->GetCollisionFlags() | PathLib::CT_IGNORE_METAOBSTACLE, &preferedDir );
			}
			else
			{
				foundSafeSpot = pathlib.FindSafeSpot( pathAgent->CacheAreaId(), agentPos, m_maxTeleportationRange, pathAgent->GetPersonalSpace() + 0.01f, returnPos, &minZ, &maxZ, pathAgent->GetCollisionFlags() | PathLib::CT_IGNORE_METAOBSTACLE );
			}

			
			if ( foundSafeSpot && !( returnPos.AsVector2() - agentPos.AsVector2() ).IsAlmostZero() ) // added sanity check
			{
				
				isSliding = true;
				data[ i_slidingTarget ] = returnPos;
				data[ i_slidingTimeout ] = 1.f;
				ASSERT( returnPos.Z >= minZ && returnPos.Z <= maxZ );

				CMovementAdjustor* adjustor = moveAgent.GetMovementAdjustor();
				adjustor->CreateNewRequest( CNAME( KeepNavdata ) )->AdjustLocationVertically( true )->SlideTo( returnPos )->AdjustmentDuration( 1.f );
			}
		}

		// sliding implementation
		if ( isSliding )
		{
			CMovementAdjustor* adjustor = moveAgent.GetMovementAdjustor();
			if ( moveAgent.IsEntityRepresentationForced( ~CMovingAgentComponent::LS_Steering ) )
			{
				isSliding = false;
				moveAgent.ForceEntityRepresentation( false, CMovingAgentComponent::LS_Steering );
				adjustor->CancelByName( CNAME( KeepNavdata ) );
				return;
			}

			Vector3 returnPos = data[ i_slidingTarget ];
			Float& timeout = data[ i_slidingTimeout ];

			Vector heading = returnPos - pathAgent->GetPosition();
			Float dist3DSq = heading.SquareMag3();

			CPathLibWorld* const pathlib = pathAgent->GetPathLib();

			if ( dist3DSq < NumericLimits< Float >::Epsilon() || pathAgent->TestLocation( pathlib, pathAgent->GetPosition(), pathAgent->GetPersonalSpace(), pathAgent->GetCollisionFlags() | PathLib::CT_FORCE_BASEPOS_ZTEST ) )
			{
				// quit sliding, we are done:)
				isSliding = false;
				moveAgent.ForceEntityRepresentation( false, CMovingAgentComponent::LS_Steering );
				adjustor->CancelByName( CNAME( KeepNavdata ) );
				return;
			}

			// teleport if too far from a target or action timeout passed
			if ( dist3DSq > m_maxTeleportationRange * m_maxTeleportationRange || timeout <= 0.f )												
			{
				SMovementAdjustmentRequest* request = adjustor->GetRequest( CNAME( KeepNavdata ) );
				if ( !request )
				{
					request = adjustor->CreateNewRequest( CNAME( KeepNavdata ) );
				}
				moveAgent.ForceEntityRepresentation( true, CMovingAgentComponent::LS_Steering );
				request->SlideTo( returnPos )->AdjustmentDuration( 0.f );
				return;
			}

			timeout -= timeDelta;

			Float forceEntityDist = pathAgent->GetPersonalSpace() * 0.5f;

			Bool forceEntityRepresentation = dist3DSq > forceEntityDist*forceEntityDist;									// at the end of the movement we turn on the physics
			moveAgent.ForceEntityRepresentation( forceEntityRepresentation, CMovingAgentComponent::LS_Steering );			// its very light function we can actually call it every frame
		}
	}
	else if ( isSliding )
	{
		isSliding = false;
		moveAgent.ForceEntityRepresentation( false, CMovingAgentComponent::LS_Steering );
	}

}
void CMoveSTKeepNavdata::Deactivate( CMovingAgentComponent& agent, InstanceBuffer& data ) const
{
	Bool& isSliding = data[ i_isSliding ];
	if ( isSliding )
	{
		isSliding = false;
		agent.ForceEntityRepresentation( false, CMovingAgentComponent::LS_Steering );
		CMovementAdjustor* adjustor = agent.GetMovementAdjustor();
		adjustor->CancelByName( CNAME( KeepNavdata ) );
	}
}
void CMoveSTKeepNavdata::OnGraphDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const
{
	Deactivate( agent, data );

	TBaseClass::OnGraphDeactivation( agent, data );
}

void CMoveSTKeepNavdata::OnBranchDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const
{
	Deactivate( agent, data );

	TBaseClass::OnBranchDeactivation( agent, data );
}

void CMoveSTKeepNavdata::GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const
{
	InstanceBuffer* dataPtr = agent.GetCurrentSteeringRuntimeData();

	if ( !dataPtr )
	{
		return;
	}
	InstanceBuffer& data = *dataPtr;

	if ( data[ i_isSliding ] )
	{
		const Vector& agentPos = agent.GetPathAgent()->GetPosition();
		String text = String::Printf( TXT("SlideToNavdata (dist %0.2f)"), ( data[ i_slidingTarget ] - agentPos.AsVector3() ).Mag() );

		frame->AddDebugText( agentPos, text, -50, -1, true, Color::BLUE, Color::BLACK );
	}
}

void CMoveSTKeepNavdata::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_isSliding;
	compiler << i_slidingTimeout;
	compiler << i_slidingTarget;

	TBaseClass::OnBuildDataLayout( compiler );
}
void CMoveSTKeepNavdata::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	data[ i_isSliding ] = false;

	TBaseClass::OnInitData( agent, data );
}


String CMoveSTKeepNavdata::GetTaskName() const
{
	static const String TASKNAME( TXT( "KeepNavdata" ) );
	return TASKNAME;
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSTMatchHeadingOrientation
///////////////////////////////////////////////////////////////////////////////
void CMoveSTMatchHeadingOrientation::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	const Vector2& heading = comm.GetHeading();
	if ( !heading.IsAlmostZero() )
	{
		CMovingAgentComponent& moveAgent	= comm.GetAgent();
		Float desiredYaw					= EulerAngles::YawFromXY( heading.X, heading.Y );
		Float currYaw						= moveAgent.GetWorldYaw();
		Float yawDiff						= EulerAngles::AngleDistance( currYaw, desiredYaw );
		comm.LockRotation( yawDiff );
		if ( m_limitSpeedOnTurns && Abs( yawDiff ) > m_maxAngleNotLimitingSpeed )
		{
			Float speed = comm.GetSpeed();
			if ( speed > m_speedLimitOnRotation )
			{
				comm.LockSpeed( m_speedLimitOnRotation );
			}
		}

		// TODO: Rotation on animation
	}
}

String CMoveSTMatchHeadingOrientation::GetTaskName() const
{
	static const String TASKNAME( TXT( "Face move direction" ) );
	return TASKNAME;
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSTKeepAwayWalls
///////////////////////////////////////////////////////////////////////////////
namespace
{
	static RED_INLINE Float DirectionalTest( CPathAgent* pathAgent, const Vector3& actorPos, const Vector2& diff, Float testRadius, Float testLen, Vector2& inOutAccumulator )
	{
		Vector3 desiredSpot;
		desiredSpot = actorPos;

		Vector2 desiredDiff = diff * testLen;
		desiredSpot.AsVector2() += desiredDiff;

		PathLib::AreaId& areaId = pathAgent->CacheAreaId();
		Vector3 safeSpot;
		switch( pathAgent->GetPathLib()->GetClearLineInDirection( areaId, actorPos, desiredSpot, testRadius, safeSpot, PathLib::CT_NO_ENDPOINT_TEST ) )
		{
		case PathLib::CLEARLINE_INVALID_START_POINT:
			break;
		case PathLib::CLEARLINE_SUCCESS:
			inOutAccumulator += diff;
			return 1.f;
		case PathLib::CLEARLINE_WAS_HIT:
			{
				Float ratio = (safeSpot - actorPos).Mag() / testLen;
				inOutAccumulator += diff * ratio;
				return ratio;
			}
		default:
			ASSUME( false );

		}
		return 0.f;
	}
}

CMoveSTKeepAwayWalls::CMoveSTKeepAwayWalls()
{

}

void CMoveSTKeepAwayWalls::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	CMovingAgentComponent& moveAgent = comm.GetAgent();
	CPathAgent* pathAgent = moveAgent.GetPathAgent();
	if ( !pathAgent )
	{
		return;
	}

	const Vector3& agentPos = pathAgent->GetPosition();
	Float personalSpace = pathAgent->GetPersonalSpace();
	Float testDistance = m_wallDetectionDistance + personalSpace;
	
	// IMPORTANT: static variable here! It actually makes this node processing (and so whole steering) limited to single thread.
	// Well. Currently it is indeed single threaded.
#ifdef KEEP_AWAY_WALLS_USE_STATIC_VAR
	static PathLib::CCollectCollisionPointsInCircleProxy testProxy;
#else
	PathLib::CCollectCollisionPointsInCircleProxy& testProxy = data[ i_queryProxy ].m_navProxy;
#endif

	CPathLibWorld* pathlib = pathAgent->GetPathLib();
	pathlib->CollectCollisionPoints( pathAgent->CacheAreaId(), agentPos, testDistance, testProxy, pathAgent->GetCollisionFlags() );

	if ( testProxy.NoHits() )
	{
		return;
	}

	Vector2 closestCollision;
	Float collisionDistance = testProxy.GetClosestCollision( closestCollision );
	Vector2 repultionDir;
	Float repultionRange;
	testProxy.GetRepulsionSpot( testDistance, repultionDir, repultionRange );

	Float baseRatio =
		collisionDistance < personalSpace
		? m_headingImportance
		: m_headingImportance * ( 1.f - ( ( collisionDistance - personalSpace ) / m_wallDetectionDistance ) );

	ASSERT( repultionRange >= collisionDistance );

	Float finalRatio =
		repultionRange >= testDistance
		? baseRatio
		: baseRatio * ( 1.f - ( repultionRange - testDistance )  / ( collisionDistance - testDistance ) );

	if ( finalRatio > NumericLimits< Float >::Epsilon() )
	{
		comm.AddHeading( repultionDir, finalRatio );
	}
}

String CMoveSTKeepAwayWalls::GetTaskName() const
{
	static const String TASKNAME( TXT( "Keep away walls" ) );
	return TASKNAME;
}
#ifndef KEEP_AWAY_WALLS_USE_STATIC_VAR
void CMoveSTKeepAwayWalls::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_queryProxy;

	TBaseClass::OnBuildDataLayout( compiler );
}
void CMoveSTKeepAwayWalls::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	TBaseClass::OnInitData( agent, data );
}

#ifndef NO_EDITOR_FRAGMENTS
void CMoveSTKeepAwayWalls::GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const
{
	TBaseClass::GenerateDebugFragments( agent, frame );

	InstanceBuffer* pData = agent.GetCurrentSteeringRuntimeData();

	if ( pData == nullptr )
	{
		return;
	}
	InstanceBuffer& data = *pData;

	// agent position may be one-frame away from test position. whateva.
	data [ i_queryProxy ].m_navProxy.DebugRender( frame, agent.GetWorldPositionRef(), agent.GetPathAgent()->GetPersonalSpace() + m_wallDetectionDistance );
}
#endif // NO_EDITOR_FRAGMENTS
#endif // KEEP_AWAY_WALLS_USE_STATIC_VAR
///////////////////////////////////////////////////////////////////////////////
// CMoveSTKeepAwayWallsInPathfollow
///////////////////////////////////////////////////////////////////////////////
void CMoveSTKeepAwayWallsInPathfollow::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	CMovingAgentComponent& moveAgent = comm.GetAgent();
	CPathAgent* pathAgent = moveAgent.GetPathAgent();
	if ( !pathAgent )
	{
		return;
	}

	const Vector3& agentPos = pathAgent->GetPosition();
	Float personalSpace = pathAgent->GetPersonalSpace();
	Float testDistance = m_wallDetectionDistance + personalSpace;
	Vector3 wallPos;
	Float wallDist = pathAgent->GetClosestObstacle( testDistance, wallPos );
	if ( wallDist >= testDistance || wallDist < NumericLimits< Float >::Epsilon() )
	{
		return;
	}

	Float importanceRatio =
		wallDist <= personalSpace
		? 1.f
		: 1.f - (wallDist - personalSpace) / (m_wallDetectionDistance);

	Float oppositeTestDist =
		( wallDist < testDistance - wallDist )
		? wallDist
		: testDistance - wallDist;
	Float oppositeTestRadius = wallDist - 0.01f;

	Vector2 diffVector = ( agentPos.AsVector2() - wallPos.AsVector2() ) / wallDist;

	Vector2 accumulator( 0.f, 0.f );

	Float oppositeWallRatio = ::DirectionalTest( pathAgent, agentPos, diffVector, oppositeTestRadius, oppositeTestDist, accumulator );
	Float finalRatio = oppositeWallRatio * importanceRatio * m_headingImportance;
	if ( finalRatio > NumericLimits< Float >::Epsilon() )
	{
		comm.AddHeading( accumulator, finalRatio );
	}
}

String CMoveSTKeepAwayWallsInPathfollow::GetTaskName() const
{
	return TXT("PathfollowAwayWalls");
}


///////////////////////////////////////////////////////////////////////////////
// CMoveSTSeparateFromActors
///////////////////////////////////////////////////////////////////////////////

void CMoveSTSeparateFromActors::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	struct Functor : public CActorsManager::DefaultFunctor
	{
		enum { SORT_OUTPUT = false };
		Functor( const Vector& pos, Float separationDistance, Float personalSpace, CObject* owner, CNode* targetNode )
			: m_pos( pos )
			, m_personalSpace( personalSpace )
			, m_separationDistance( separationDistance )
			, m_testLimitSq( (personalSpace + separationDistance) * (personalSpace + separationDistance) )
			, m_output( 0.f, 0.f )
			, m_owner( owner )
			, m_targetNode( targetNode ) {}

		Bool ActorHasCCCollisionsOn( CActor* actor )
		{
			if ( CMovingAgentComponent* actorMac = actor->GetMovingAgentComponent() )
			{
				return actorMac->IsCharacterCollisionsEnabled();
			}
			return false;
		}

		RED_INLINE Bool operator()( CActor* actor )
		{
			if ( actor == m_owner || actor == m_targetNode )
			{
				return true;
			}
			if ( !actor->IsAlive() && !ActorHasCCCollisionsOn( actor ) )
			{
				return true;
			}
			const Vector& actorPos = actor->GetWorldPositionRef();
			Vector2 diff = m_pos - actorPos.AsVector2();
			Float distSq = diff.SquareMag();
			if ( distSq > m_testLimitSq || distSq < NumericLimits< Float >::Epsilon() )
			{
				return true;
			}
			Float dist = sqrt( distSq );

			Float distRatio = dist <= m_personalSpace ? 1.f : 1.f - (dist - m_personalSpace) / m_separationDistance;
			Float actorMass = 1.f * distRatio;		// TODO: some kind of actors weight

			diff *= actorMass / dist;				// set length to actorMass

			m_output += diff;

			return true;
		}

		Vector			m_pos;
		Float			m_personalSpace;
		Float			m_separationDistance;
		Float			m_testLimitSq;
		Vector2			m_output;
		CObject*		m_owner;
		CNode*			m_targetNode;
	};

	Float lastUpdateTime = data[ i_lastUpdateTime ];
	Float currentTime = GGame->GetEngineTime();											// NOTICE indirect cast EngineTime->Float
	Float interpolationTime = lastUpdateTime + m_updateFrequency;

	if ( currentTime >= lastUpdateTime && currentTime <= interpolationTime )
	{
		// interpolate algorithm output
		Float ratio = (currentTime - lastUpdateTime) / m_updateFrequency;
		// clamp - float precision issues
		ratio = Clamp( ratio, 0.f, 1.f );

		const Vector2& lastOutput = data[ i_lastOutput ];
		Float lastImportance = data[ i_lastImportance ];
		const Vector2& currOutput = data[ i_currentOutput ];
		Float currImportance = data[ i_currentImportance ];

		Vector2 interpolatedOutput = lastOutput + (currOutput - lastOutput) * ratio;
		Float interpolatedImportance = lastImportance + (currImportance - lastImportance) * ratio;

		if ( interpolatedImportance > NumericLimits< Float >::Epsilon() )
		{
			comm.AddHeading( interpolatedOutput,interpolatedImportance * m_headingImportance );
		}
		return;
	}

	const SMoveLocomotionGoal& goal = comm.GetGoal();
	CMovingAgentComponent& agent = comm.GetAgent();
	CPathAgent* pathAgent = agent.GetPathAgent();

	if ( !pathAgent )
	{
		return;
	}

	data[ i_lastOutput ] = data[ i_currentOutput ];
	data[ i_lastImportance ] = data[ i_currentImportance ];
	data[ i_lastUpdateTime ] = currentTime;

	Functor functor( pathAgent->GetPosition(), m_separationDistance, pathAgent->GetPersonalSpace(), agent.GetParent(), goal.GetGoalTargetNode() );
	Float testRadius = m_separationDistance + pathAgent->GetPersonalSpace();
	Box testBBox( Vector( -testRadius, -testRadius, -1.f ), Vector( testRadius, testRadius, 2.f ) );

	GCommonGame->GetActorsManager()->TQuery( functor.m_pos, functor, testBBox, true, NULL, 0 );

	if ( !functor.m_output.IsAlmostZero() )
	{
		Float outputDist = functor.m_output.Mag();
		Float importanceRatio;
		if ( outputDist > 1.f )
		{
			importanceRatio = 1.f;
			functor.m_output *= 1.f / outputDist;
		}
		else
		{
			importanceRatio = outputDist;
		}
		comm.AddHeading( functor.m_output, importanceRatio * m_headingImportance );

		data[ i_currentOutput ] = functor.m_output;
		data[ i_currentImportance ] = importanceRatio;
	}
	else
	{
		data[ i_currentOutput ].Set( 0,0 );
		data[ i_currentImportance ] = 0.f;
	}
	
	
}

String CMoveSTSeparateFromActors::GetTaskName() const
{
	static const String TASKNAME( TXT( "Keep away actors" ) );
	return TASKNAME;
}

void CMoveSTSeparateFromActors::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_lastUpdateTime;
	compiler << i_lastOutput;
	compiler << i_lastImportance;
	compiler << i_currentOutput;
	compiler << i_currentImportance;
}
void CMoveSTSeparateFromActors::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	TBaseClass::OnInitData( agent, data );

	data[ i_lastUpdateTime ] = GGame->GetEngineTime();
	data[ i_lastOutput ].Set( 0, 0 );
	data[ i_lastImportance ] = 0.f;
	data[ i_currentOutput ].Set( 0, 0 );
	data[ i_currentImportance ] = 0.f;
}

///////////////////////////////////////////////////////////////////////////////

CMoveSTArrive::CMoveSTArrive()
	: m_rotationVar( CNAME( rotationBlend ) )
	, m_rotationEvent( CNAME( Rotate ) )
	, m_rotationNotification( CNAME( RotateEnd ) )
{}

void CMoveSTArrive::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	const CMovingAgentComponent& agent = comm.GetAgent();
	const SMoveLocomotionGoal& goal = comm.GetGoal();

	// velocity goal execution

	Vector velocity = Vector::ZERO_3D_POINT;
	if ( goal.IsHeadingGoalSet() )
	{
		velocity.AsVector2() = goal.GetHeadingToGoal();
	}

	// orientation goal execution
	if ( goal.IsOrientationGoalSet() )
	{
		Bool canRotate = true;
		if ( goal.IsHeadingGoalSet() )
		{
			// when using a rotation target, don't use less restrictive rotation rules
			if ( agent.IsRotationTargetEnabled() )
			{
				// check if we're about to make a sudden turn and slow down if so
				Float absRotationToGoal = MAbs( goal.GetRotationToGoal() );
				if ( absRotationToGoal > 40.0f )
				{
					// force the character to walk
					Float walkAbsSpeed = agent.ConvertSpeedRelToAbs( 1.0f );
					velocity = velocity.Normalized3() * walkAbsSpeed;

					// check if at this speed a rotation is possible
					canRotate = ( agent.GetLastMovementSpeed() <= 1.0f );
				}
			}
			else
			{
				// check if we're about to make a sudden turn and slow down if so
				Float absRotationToGoal = MAbs( goal.GetRotationToGoal() );
				if ( absRotationToGoal > 40.0f && absRotationToGoal < 120.0f )
				{
					// force the character to walk
					Float walkAbsSpeed = agent.ConvertSpeedRelToAbs( 1.0f );
					velocity = velocity.Normalized3() * walkAbsSpeed;

					// check if at this speed a rotation is possible
					canRotate = ( agent.GetLastMovementSpeed() <= 1.0f );
				}
				else if ( absRotationToGoal >= 120.f)
				{
					velocity = Vector::ZERO_3D_POINT;

					// check if at this speed a rotation is possible
					canRotate = ( agent.GetLastMovementSpeed() <= 0.0f );
				}
			}
		}

		if ( canRotate )
		{
			Float newSpeed = velocity.Mag2();
			Bool canUseAnimationToRotate = newSpeed < 1e-3;
			if ( MAbs( goal.GetOrientationDiff() ) < 30 || !canUseAnimationToRotate )
			{
				// don't even try rotating using animation when the angle is too small
				comm.LockRotation( goal.GetOrientationDiff(), goal.ClampOrientation() );
			}
			else
			{
				comm.SetVar( BVC_RotationAngle, m_rotationVar, goal.GetOrientationDiff() );
				if ( comm.RaiseEvent( m_rotationEvent, m_rotationNotification ) == false )
				{
					comm.LockRotation( goal.GetOrientationDiff(), goal.ClampOrientation() );
				}
				else
				{
					comm.LockRotation( 0, false );
				}
			}
		}
	}

	// set the proper velocity
	Float requestedSpeed = velocity.Mag2();
	if ( requestedSpeed > 0.0f )
	{
		comm.LockHeading( velocity );
	}
	comm.LockSpeed( requestedSpeed );
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSTApplySteering

void CMoveSTApplySteering::ApplyHeading( IMovementCommandBuffer& comm, const Vector2& heading ) const
{
	CMovingAgentComponent& agent = comm.GetAgent();

	Float inputMoveDirYaw = EulerAngles::YawFromXY( heading.X, heading.Y );
	Vector2 currMoveDir = agent.GetHeading().AsVector2();
	Float currMoveDirYaw = EulerAngles::YawFromXY( currMoveDir.X, currMoveDir.Y );
	Float moveDirChange = EulerAngles::AngleDistance( currMoveDirYaw, inputMoveDirYaw );
	//moveDirChange = ::Clamp< Float >( moveDirChange, -360.f, maxDirChange );			// clamp the direction change - not used
	Float clampedMoveDir = EulerAngles::NormalizeAngle( currMoveDirYaw + moveDirChange );

	agent.RequestMoveDirection( clampedMoveDir );				// WORLD SPACE COORDINATES
}
void CMoveSTApplySteering::ApplyRotation( IMovementCommandBuffer& comm, Float rotation ) const
{
	CMovingAgentComponent& agent = comm.GetAgent();
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	agent.RequestMoveRotation( rotation, goal.ClampOrientation() );
}
void CMoveSTApplySteering::ApplySpeed( IMovementCommandBuffer& comm, Float speed ) const
{
	CMovingAgentComponent& agent = comm.GetAgent();
	agent.RequestAbsoluteMoveSpeed( speed );
}

CMoveSTApplySteering::CMoveSTApplySteering()
	: m_minSpeed( 0.f )
{}

void CMoveSTApplySteering::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	const Float MIN_INPUT_VALUE = 0.05f;

	if ( comm.IsHeadingSet() )
	{
		const Vector2& heading = comm.GetHeading();
		// calculate the movement direction

		Float headingLenSq = heading.SquareMag();
		if ( headingLenSq > (MIN_INPUT_VALUE*MIN_INPUT_VALUE) )
		{
			Float speedMult = 1.f;
			if ( headingLenSq < 1.f )
			{
				speedMult = sqrt( headingLenSq );
			}

			ApplyHeading( comm, heading );

			// speed should be evaluated only if we actually wants to 'move'
			const SMoveLocomotionGoal& goal = comm.GetGoal();
			const CMovingAgentComponent& agent = comm.GetAgent();
			Float speed = comm.IsSpeedSet() ? comm.GetSpeed() * speedMult : speedMult;
			if ( goal.ShouldSnapToMinimalSpeed() && speed < m_minSpeed && speed > MIN_INPUT_VALUE )  // speed > MIN_INPUT_VALUE if the guy wants to stop let him stop :
			{
				speed = m_minSpeed;
			}

			// SteeringGraphSpeed [ 0,1 ] is maped on the relative speed SpeedRel[ 0,speedScaleRel ]
			// but speedScaleRel depends on parameters
			const Float speedScaleAbs	= goal.IsSpeedGoalSet() ? goal.GetDesiredSpeed() : agent.GetMaxSpeed();
			// SteeringGraphSpeed [ 0,1 ] -> SpeedRel[ 0,speedScaleRel ]
			const Float speedScaleRel	= agent.ConvertSpeedAbsToRel( speedScaleAbs );

			// Now that the mapping is ready, the absolute speed needs to be calculated from 'speed'
			const Float speedRel		= speed * speedScaleRel;
			const Float speedAbs		= agent.ConvertSpeedRelToAbs( speedRel );

			ApplySpeed( comm, speedAbs );
		}
	}
	
	if ( comm.IsRotationSet() )
	{
		ApplyRotation( comm, comm.GetRotation() );
	}
}

String CMoveSTApplySteering::GetTaskName() const
{
	static const String NAME( TXT( "ApplySteering" ) );
	return NAME;
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSTApplyAnimationSteering
CMoveSTApplyAnimationSteering::CMoveSTApplyAnimationSteering()
	: m_rotationVar( CNAME( rotationBlend ) )
{}

void CMoveSTApplyAnimationSteering::ApplyRotation( IMovementCommandBuffer& comm, Float rotation ) const
{
	CMoveSTApplySteering::ApplyRotation( comm, rotation );
	comm.SetVar( BVC_RotationAngle, m_rotationVar, rotation );
}

String CMoveSTApplyAnimationSteering::GetTaskName() const
{
	static const String NAME( TXT( "DEPRECATED convert to ApplySteering" ) );
	return NAME;
}

///////////////////////////////////////////////////////////////////////////////

void CMoveSTResetSteering::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	comm.ResetBehavior();
}

String CMoveSTResetSteering::GetTaskName() const
{
	return TXT( "ResetSteering" );
}

///////////////////////////////////////////////////////////////////////////////
void CMoveSTStop::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	comm.LockSpeed( 0.f );
	comm.LockHeading( Vector2( 0,0 ) );
}

String CMoveSTStop::GetTaskName() const
{
	return TXT( "Stop" );
}

///////////////////////////////////////////////////////////////////////////////

void CMoveSTStopOnFreezing::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	const CMovingAgentComponent& movingAgentComponent = comm.GetAgent();
	const CActor* const actor = Cast< CActor >( movingAgentComponent.GetEntity() );
	const Bool noPhysics = ( movingAgentComponent.GetPhysicalRepresentationDisableFlags() & CMovingAgentComponent::LS_Force ) != 0;
	const Bool cantMove = actor && actor->IsFreezedOnFailedPhysicalRequest() && noPhysics && !movingAgentComponent.GetPathAgent()->IsOnNavdata();
		
	if ( cantMove )
	{
		comm.LockSpeed( 0.f );
		comm.LockRotation( 0.f );
		comm.LockHeading( Vector2( 0, 0 ) );
	}
}

String CMoveSTStopOnFreezing::GetTaskName() const
{
	return TXT( "StopWhenFrozen" );
}

///////////////////////////////////////////////////////////////////////////////

CMoveSTStep::CMoveSTStep()
: m_stepDistanceVar( CNAME( StepLength ) )
, m_stepHeadingVar( CNAME( rotationBlend ) )
, m_stepEvent( CNAME( Step ) )
, m_stepNotification( CNAME( StepEnd ) )
{
}

void CMoveSTStep::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();

	if ( !goal.IsHeadingGoalSet() )
	{
		return;
	}

	Vector2 velocity = goal.GetVelocity();

	// make the max dist quadratic, depending on the step angle
	Float alpha = MAbs( goal.GetRotationToGoal() );
	alpha = alpha - ( (Int32)( alpha / 45.0f ) * 45.0f );

	// calculate the actual step length
	Double behStepDist = ( (Double)velocity.Mag() ) * MCos( DEG2RAD( alpha ) );	

	comm.SetVar( BVC_RotationAngle, m_stepHeadingVar, goal.GetRotationToGoal() );
	comm.SetVar( BVC_Extra, m_stepDistanceVar, (Float)behStepDist );
	comm.RaiseEvent( m_stepEvent, m_stepNotification );
}

///////////////////////////////////////////////////////////////////////////////

void CMoveSTSlide::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	CMovingAgentComponent& agent = comm.GetAgent();
	const SMoveLocomotionGoal& goal = comm.GetGoal();

	Vector dir( 0, 0, 0 );
	if ( goal.IsHeadingGoalSet() )
	{
		dir.AsVector2() = goal.GetVelocity();
	}

	EulerAngles rotation( 0, 0, 0 );
	if ( goal.IsOrientationGoalSet() )
	{
		rotation.Yaw = goal.GetOrientationDiff();
	}

	agent.Slide( dir, rotation );
}

///////////////////////////////////////////////////////////////////////////////

CMoveSTSetBehaviorVariable::CMoveSTSetBehaviorVariable()
	: m_variableContext( BVC_Extra )
	, m_value( 0.0f )
{
}

void CMoveSTSetBehaviorVariable::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	comm.SetVar( m_variableContext, m_variableName, m_value );
}

///////////////////////////////////////////////////////////////////////////////

CMoveSTChangeSpeed::CMoveSTChangeSpeed()
: m_speedType( MT_Walk )
, m_absSpeed( 0.0f )
{
}

void CMoveSTChangeSpeed::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	CMovingAgentComponent& agent = comm.GetAgent();

	agent.SetMoveType( m_speedType, m_absSpeed );
}

String CMoveSTChangeSpeed::GetTaskName() const
{
	return TXT( "ChangeSpeed" );
}


///////////////////////////////////////////////////////////////////////////////

CMoveSTSetFlags::CMoveSTSetFlags()
: m_movementFlags( 0 )
, m_bitOperation( BO_Or )
{
}

void CMoveSTSetFlags::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	Uint8 currMovementFlags = comm.GetAgent().GetMovementFlags();
	Uint8 newMovementFlags = currMovementFlags;
	switch( m_bitOperation )
	{
	case BO_And: newMovementFlags = m_movementFlags & currMovementFlags; break;
	case BO_Or: newMovementFlags = m_movementFlags | currMovementFlags; break;
	case BO_Xor: newMovementFlags = m_movementFlags ^ currMovementFlags; break;
	}

	comm.GetAgent().SetMovementFlags( newMovementFlags );
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSTSetupRotationChange
///////////////////////////////////////////////////////////////////////////////

CMoveSTSetupRotationChange::CMoveSTSetupRotationChange()
	: m_maxDirectionChange( 360.0f )
	, m_maxRotationChange( 180.f )
{
}

void CMoveSTSetupRotationChange::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	CMovingAgentComponent& agent = comm.GetAgent();
	agent.SetDirectionChangeRate( m_maxDirectionChange );
	agent.SetMaxRotation( m_maxRotationChange );
}

String CMoveSTSetupRotationChange::GetTaskName() const
{
	static const String NAME( TXT( "SetupRotation" ) );
	return NAME;
}

///////////////////////////////////////////////////////////////////////////////

CMoveSTMapRotationChangeUsingCustomRotation::CMoveSTMapRotationChangeUsingCustomRotation()
	: m_defaultMaxDirectionChange( 360.0f )
	, m_defaultMaxRotationChange( 180.0f )
{
}

void CMoveSTMapRotationChangeUsingCustomRotation::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	CMovingAgentComponent& agent = comm.GetAgent();
	agent.SetDirectionChangeRate( m_defaultMaxDirectionChange );
	agent.SetMaxRotation( m_defaultMaxRotationChange );
}

String CMoveSTMapRotationChangeUsingCustomRotation::GetTaskName() const 
{ 
	return TXT( "OBSOLATE: replace with SetupRotation" );
}

///////////////////////////////////////////////////////////////////////////////

CMoveSTSetMaxDirectionChange::CMoveSTSetMaxDirectionChange()
: m_angle( 180.0f )
{
}

void CMoveSTSetMaxDirectionChange::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	comm.GetAgent().SetDirectionChangeRate( m_angle );
}
String CMoveSTSetMaxDirectionChange::GetTaskName() const
{
	static const String NAME( TXT( "SetMaxDirectionChange" ) );
	return NAME;
}
///////////////////////////////////////////////////////////////////////////////

CMoveSTSetMaxRotationChange::CMoveSTSetMaxRotationChange()
	: m_angle( 180.0f )
{
}

void CMoveSTSetMaxRotationChange::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	comm.GetAgent().SetMaxRotation( m_angle );
}

String CMoveSTSetMaxRotationChange::GetTaskName() const
{
	static const String NAME( TXT( "SetMaxRotationChange" ) );
	return NAME;
}

///////////////////////////////////////////////////////////////////////////////

CMoveSTSetAcceleration::CMoveSTSetAcceleration()
	: m_acceleration( FLT_MAX )
	, m_deceleration( FLT_MAX )
{
}

void CMoveSTSetAcceleration::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	comm.GetAgent().SetMaxAcceleration( m_acceleration, m_deceleration );
}

String CMoveSTSetAcceleration::GetTaskName() const
{
	static const String NAME( TXT( "SetAcceleration" ) );
	return NAME;
}

///////////////////////////////////////////////////////////////////////////////

CMoveSTAvoidObstacles::CMoveSTAvoidObstacles()
	: m_avoidObstaclesImportance( 1.0f )
	, m_timeToChooseNextObstacle( 0.8f )
	, m_maxDistanceToObstacle( 5.0f )
	, m_furthestImpactTime( 4.0f )
	, m_directionChangeOverride( 220.0f )
	, m_minVelocityToOverrideDirectionChange( 0.3f )
	, m_overrideValues( true )
	, m_modifyRotation( false )
	, m_modifyHeading( true )
	, m_modifySpeed( true )
{
}

Float CMoveSTAvoidObstacles::CalcSqDist( Float t, const Vector& aLoc, const Vector& aVel, const Vector& bLoc, const Vector& bVel )
{
	return ( ( aLoc + aVel * t ) - ( bLoc + bVel * t ) ).SquareMag2();
}

void CMoveSTAvoidObstacles::CalcClosestTimeWorker( Bool& collided, Float& t, Float stepT, Float minSqDist, const Vector& aLoc, const Vector& aVel, const Vector& bLoc, const Vector& bVel )
{
	if ( stepT < 0.001f )
	{
		return;
	}
	else
	{
		Float atT = CalcSqDist( t, aLoc, aVel, bLoc, bVel );
		if ( atT < minSqDist )
		{
			// for sure we have collided, remember that and move closer
			collided = true;
			t -= stepT;
		}
		else if ( collided )
		{
			// we if haven't collided now but we know that we already collided, move further to get as close to impact point as possible
			t += stepT;
		}
		else 
		{
			// check if we might be closer to obstacle when we move
			Float atMinus = CalcSqDist( t - stepT, aLoc, aVel, bLoc, bVel );
			Float atPlus = CalcSqDist( t + stepT, aLoc, aVel, bLoc, bVel );
			if ( atT > atMinus || atT > atPlus )
			{
				// decide where we will be closer
				if ( atPlus < atMinus )
				{
					t += stepT;
				}
				else
				{
					t -= stepT;
				}
			}
		}
		t = Max( 0.0f, t );
		CalcClosestTimeWorker( collided, t, stepT * 0.5f, minSqDist, aLoc, aVel, bLoc, bVel );
	}
}

Bool CMoveSTAvoidObstacles::CalcClosestTime( Float& outT, Float minT, Float maxT, Float minDist, const Vector& aLoc, const Vector& aVel, const Vector& bLoc, const Vector& bVel )
{
	// start in the middle
	outT = ( minT + maxT ) * 0.5f;
	Bool collided = false;
	CalcClosestTimeWorker( collided, outT, ( maxT - minT ) * 0.25f, minDist * minDist, aLoc, aVel, bLoc, bVel );
	return collided;
}

void CMoveSTAvoidObstacles::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	Float& timeToChooseNextObstacle = data[ i_timeToChooseNextObstacle ];

	timeToChooseNextObstacle -= timeDelta;
	if ( timeToChooseNextObstacle <= 0.0f )
	{
		timeToChooseNextObstacle = GEngine->GetRandomNumberGenerator().Get< Float >( m_timeToChooseNextObstacle * 0.5f, m_timeToChooseNextObstacle  ); // just use new value, we don't need to be precisely switching every <time> defined
		FindObstacleToAvoid( comm, data, timeDelta );
	}

	ProcessAvoidance( comm, data, timeDelta );
}

void CMoveSTAvoidObstacles::FindObstacleToAvoid( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{ 
	THandle< CActor >& avoidObstacle = data[ i_avoidObstacle ];	
	THandle< CActor >& prevAvoidObstacleTH	= data[ i_prevAvoidObstacle ];	
	EAvoidObstacleStrategy& avoidObstacleStrategy = data[ i_avoidObstacleStrategy ];
	Float& avoidanceRadius = data[ i_avoidanceRadius ];

	CMovingAgentComponent& agent = comm.GetAgent();
	const SMoveLocomotionGoal& goal	= comm.GetGoal();

	CActor* prevObstacle = prevAvoidObstacleTH.Get();
	CActor* cachedAvoidObstacle = avoidObstacle.Get();
	EAvoidObstacleStrategy prevAOS = avoidObstacleStrategy;

	// take into account our goal
	const Float actualMaxDistanceToObstacle = goal.IsDistanceToGoalSet()? Min( m_maxDistanceToObstacle , goal.GetDistanceToGoal() ) : 0.0f;
	
	avoidObstacle = NULL;
	avoidObstacleStrategy = AOS_None;
	avoidanceRadius = 2.5f;

	// find obstacle on the way, find strategy
	const Vector& agentLoc = agent.GetWorldPositionRef();
	const EulerAngles agentRotDesired = agent.GetWorldRotation() + EulerAngles( 0.0f, 0.0f, comm.GetRotation() );
	const Vector agentVel = agent.GetVelocity();
	if ( agentVel.SquareMag2() > 0.0f && ! agent.GetEntity()->GetTransformParent() )
	{
		Vector agentMoveInDir;
		agentMoveInDir = EulerAngles::YawToVector( agentRotDesired.Yaw );
		Vector agentMoveInDirRight = Vector( agentMoveInDir.Y, -agentMoveInDir.X, 0.0f );
			
		Vector agentWorkingVel = agentMoveInDir * comm.GetSpeed();

		// find obstacle that we guess is the one that we are going to collide next
		Float closestImpactTime = m_furthestImpactTime;
		
		THandle< CActor > avoidObstacleSec	= NULL;
		Float closestImpactTimeSec			= m_furthestImpactTime;
		Float avoidanceRadiusSec;

		{
			struct Functor : public CActorsManager::DefaultFunctor
			{
				enum { SORT_OUTPUT = false };
				Functor( Float agentAvoidanceRadius, Float closestImpactTime, Float maxDistanceToObstacle, CEntity* agent, const Vector& agentLoc, const Vector& agentMovementDir, const Vector& agentWorkingVel, const CNode* goalTarget )
					: m_avoid( NULL )
					, m_agentAvoidanceRadius( agentAvoidanceRadius )
					, m_closestImpactTime( closestImpactTime )
					, m_maxDistanceToObstacle( maxDistanceToObstacle )
					, m_agent( agent )
					, m_agentLoc( agentLoc )
					, m_agentMovementDir( agentMovementDir )
					, m_agentWorkingVelocity( agentWorkingVel )
					, m_agentWorkingVelocitySize( agentWorkingVel.Mag2() )
					, m_goalTargetNode( goalTarget )
					, m_avoidSec( NULL )
					, m_closestImpactTimeSec( closestImpactTime )

				{}

				RED_INLINE Bool operator()( CActor* actor )
				{
					if ( actor == m_agent ||
						 actor == m_goalTargetNode )
					{
						return true;
					}
					CMovingAgentComponent* actorMac = actor->GetMovingAgentComponent();
					if ( !actorMac )
					{
						return true;
					}
					const Vector& actorLoc = actor->GetWorldPositionRef();
					Vector toActor = actorLoc - m_agentLoc;
					Vector toActorDir = toActor.Normalized2();
					Float toActorDot = Vector::Dot2( toActorDir, m_agentMovementDir );
					if ( toActorDot > 0.3f ) // in front only (we don't see anyone behind us)
					{
						const Vector actorVel = actorMac->GetVelocity();

						Float predictedImpactTime;
						Float collisionPredictionRadius = actorMac->GetAvoidanceRadius() + m_agentAvoidanceRadius;


						if(	CalcClosestTime( predictedImpactTime, 0.0f, m_closestImpactTime, collisionPredictionRadius, m_agentLoc, m_agentWorkingVelocity, actorLoc, actorVel ) && 
							predictedImpactTime >= -0.5f && predictedImpactTime < m_closestImpactTime )
						{
							if ( m_maxDistanceToObstacle <= 0.0f || m_agentWorkingVelocitySize * predictedImpactTime < m_maxDistanceToObstacle )
							{
								//remember second closest
								m_avoidSec				= m_avoid;
								m_closestImpactTimeSec	= m_closestImpactTime;
								m_avoidanceRadiusSec	= m_avoidanceRadius;									

								m_avoid = actor;
								m_closestImpactTime = predictedImpactTime;
								m_avoidanceRadius = collisionPredictionRadius + 1.0f; //+1m for safety
							}
						}
						else 
						{							
							Vector velocity = actorVel - m_agentWorkingVelocity;
							if( velocity.SquareMag2() > 0.1f )
							{
								Float distance = ( m_agentLoc - actorLoc ).Mag2();	
								predictedImpactTime = distance / velocity.Mag2();
								if( predictedImpactTime < m_closestImpactTimeSec )
								{
									//remember second closest
									m_avoidSec				= actor;
									m_closestImpactTimeSec	= predictedImpactTime;
									m_avoidanceRadiusSec	= collisionPredictionRadius + 1.0f;	//+1m for safety
								}
							}
						}
					
					}
					return true;
				}

				CActor* m_avoid;
				Float m_closestImpactTime;				
				Float m_avoidanceRadius;

				CActor* m_avoidSec;
				Float m_closestImpactTimeSec;
				Float m_avoidanceRadiusSec;

				const Float m_maxDistanceToObstacle;
				const CEntity* m_agent;
				const Vector m_agentLoc;
				const Vector m_agentMovementDir;
				const Vector m_agentWorkingVelocity;
				const Float m_agentWorkingVelocitySize;
				const CNode* m_goalTargetNode;
				const Float m_agentAvoidanceRadius;
			};

			const Vector findAroundLoc = agentLoc + agentWorkingVel * closestImpactTime * 0.5f; // look in front of yourself
			const Float range = m_maxDistanceToObstacle * Clamp( comm.GetSpeed() * 0.6f, 1.0f, 2.0f ); // magical numbers! right... this is done to extend radius of search when moving faster
			Vector minBound( -range, -range, -1.f );
			Vector maxBound( range, range, 1.f );

			const Float agentAvoidanceRadius = agent.GetEntity()->FindComponent< CMovingPhysicalAgentComponent >()->GetAvoidanceRadius();	

			// perform query and get results
			Functor functor( agentAvoidanceRadius, closestImpactTime, actualMaxDistanceToObstacle, agent.GetEntity(), agentLoc, agentMoveInDir, agentWorkingVel, comm.GetGoal().GetGoalTargetNode() );
			GCommonGame->GetActorsManager()->TQuery( findAroundLoc, functor, Box( minBound, maxBound ), true, NULL, 0 );
			avoidObstacle = functor.m_avoid;
			cachedAvoidObstacle = avoidObstacle.Get();
			closestImpactTime = functor.m_closestImpactTime;
			avoidanceRadius = functor.m_avoidanceRadius;

			avoidObstacleSec	= functor.m_avoidSec;
			closestImpactTimeSec= functor.m_closestImpactTimeSec;
			avoidanceRadiusSec	= functor.m_avoidanceRadiusSec;
		}

		// if we're trying to avoid same obstacle for too many times, stop
		Int32& avoidingSameObstacleCount		= data[ i_avoidingSameObstacleCount ];
		Int32& switchAvoidDirectionsCounter = data[ i_switchAvoidDirectionsCounter ];

		if ( cachedAvoidObstacle && cachedAvoidObstacle == prevObstacle )
		{			
			Float radius = avoidanceRadius * 1.5f;
			if ( ( cachedAvoidObstacle->GetWorldPosition() - agentLoc ).SquareMag2() < radius * radius || GEngine->GetRandomNumberGenerator().Get< Int32 >( 4 ) == 0 )
			{			
				++avoidingSameObstacleCount;
				if( switchAvoidDirectionsCounter > 0 )
				{
					avoidObstacleStrategy = AOS_Stop;	
				}
				else if ( avoidingSameObstacleCount > 3 && GEngine->GetRandomNumberGenerator().Get< Int32 >( 3 ) == 0 )
				{
					avoidingSameObstacleCount = 0;
					avoidObstacleStrategy = AOS_Stop;	
					++switchAvoidDirectionsCounter;
				}				
			}
		}
		else
		{
			avoidingSameObstacleCount		= 0;
			switchAvoidDirectionsCounter	= 0;
		}

		// decide about strategy (if we haven't already pick one)
		if ( cachedAvoidObstacle && avoidObstacleStrategy == AOS_None )
		{			
			prevAvoidObstacleTH = cachedAvoidObstacle;
			Float predictedImpactTime = closestImpactTime;
			data[ i_predictedImpactTime ] = predictedImpactTime;
			const Vector obstacleLoc = cachedAvoidObstacle->GetWorldPosition();
			const Vector obstacleVel = cachedAvoidObstacle->GetMovingAgentComponent()? cachedAvoidObstacle->GetMovingAgentComponent()->GetVelocity() : Vector::ZEROS;
			if ( obstacleVel.SquareMag2() > 0.1f )
			{
				const Float agentWorkingVelYaw = EulerAngles::YawFromXY( agentWorkingVel.X, agentWorkingVel.Y );
				if ( Abs( EulerAngles::AngleDistance( EulerAngles::YawFromXY( obstacleVel.X, obstacleVel.Y ), agentWorkingVelYaw ) ) < 45.0f )
				{
					avoidObstacleStrategy = AOS_Follow;
				}
				else
				{
					// estimated impact time, but use different value to predict which direction is best to be used when avoiding collision
					Float modifiedPredictedImpactTime = predictedImpactTime * 0.7f + 0.5f;
					data[ i_predictedImpactTime ] = modifiedPredictedImpactTime;
					// where we and obstacle are going to be?
					Vector obstacleColLoc = obstacleLoc + obstacleVel * modifiedPredictedImpactTime;
					Vector agentColLoc = agentLoc + agentWorkingVel * modifiedPredictedImpactTime;
					data[ i_obstacleColLoc ] = obstacleColLoc;
					data[ i_agentColLoc ] = agentColLoc;
					// check on which side it is easier to avoid
					// and by easier, we mean, we will have to rotate less, from our requested movement direction
					const Vector agentToObstacleCol = obstacleColLoc - agentLoc;
					const Vector agentToObstacleColDir = agentToObstacleCol.Normalized2();
					const Vector agentToObstacleColRight = Vector( agentToObstacleColDir.Y, -agentToObstacleColDir.X, 0.0f );
					const Vector avoidOnRight = obstacleColLoc + agentToObstacleColRight * (avoidanceRadius * 0.8f);
					const Vector avoidOnLeft = obstacleColLoc - agentToObstacleColRight * (avoidanceRadius * 0.8f);
					const Vector toAvoidOnRight = avoidOnRight - agentLoc;
					const Vector toAvoidOnLeft = avoidOnLeft - agentLoc;
					const Float diffOnRight = EulerAngles::AngleDistance( agentWorkingVelYaw, EulerAngles::YawFromXY( toAvoidOnRight.X, toAvoidOnRight.Y ) );
					const Float diffOnLeft = EulerAngles::AngleDistance( agentWorkingVelYaw, EulerAngles::YawFromXY( toAvoidOnLeft.X, toAvoidOnLeft.Y ) );
					// try to keep same as last time
					const Float preferAddition = prevAOS == AOS_Right? -30.0f :
						( prevAOS == AOS_Left? 30.0f : GEngine->GetRandomNumberGenerator().Get< Float >( -10.0f , 10.0f ) );
					avoidObstacleStrategy = Abs(diffOnRight) - Abs(diffOnLeft) + preferAddition < 0.0f? AOS_Right : AOS_Left;
				}
			}
			else
			{
				CActor* avoidObstacleActSec = avoidObstacleSec.Get();
				Bool obstacleCalculated = false;

				if( avoidObstacleSec )
				{									
					const Vector obstacleVelSecSec = avoidObstacleSec->GetMovingAgentComponent()? avoidObstacleSec->GetMovingAgentComponent()->GetVelocity() : Vector::ZEROS;
					const Vector obstacleLocSec = avoidObstacleSec->GetWorldPosition();

					if( ( obstacleLoc - obstacleLocSec ).SquareMag2() < 1.5f * 1.5f && obstacleVel.SquareMag2() < 0.1f )
					{									
						Vector obstacleMiddle = ( obstacleLoc + obstacleLocSec )/2;
						Vector toObstacle = obstacleMiddle - agentLoc;
						data[ i_obstacleColLoc ] = obstacleMiddle;
						data[ i_agentColLoc ] = agentLoc + agentWorkingVel * predictedImpactTime;
						avoidObstacleStrategy = Vector::Dot2( toObstacle, agentMoveInDirRight ) >= 0.0f? AOS_Left : AOS_Right;
						obstacleCalculated = true;
					}
				}

				if( !obstacleCalculated )
				{				
					// just check on which side is obstacle
					Vector toObstacle = obstacleLoc - agentLoc;
					data[ i_obstacleColLoc ] = obstacleLoc;
					data[ i_agentColLoc ] = agentLoc + agentWorkingVel * predictedImpactTime;
					avoidObstacleStrategy = Vector::Dot2( toObstacle, agentMoveInDirRight ) >= 0.0f? AOS_Left : AOS_Right;
				}
			}
			
		}
		else
		{
			if( avoidObstacleStrategy == AOS_Stop && switchAvoidDirectionsCounter > 0 )
			{
				const Vector obstacleVel = cachedAvoidObstacle->GetMovingAgentComponent()? cachedAvoidObstacle->GetMovingAgentComponent()->GetVelocity() : Vector::ZEROS;
				if( obstacleVel.SquareMag2() < 0.1f )
				{//if obstacle is not moving...

					Float predictedImpactTime = closestImpactTime;
					data[ i_predictedImpactTime ] = predictedImpactTime;
					const Vector obstacleLoc = cachedAvoidObstacle->GetWorldPosition();
				
					Vector toObstacle = obstacleLoc - agentLoc;
					data[ i_obstacleColLoc ] = obstacleLoc;
					data[ i_agentColLoc ] = agentLoc + agentWorkingVel * predictedImpactTime;
					avoidObstacleStrategy = Vector::Dot2( toObstacle, agentMoveInDirRight ) >= 0.0f? AOS_Right : AOS_Left; //...try to avoid from different direction...
					++switchAvoidDirectionsCounter;
					if( switchAvoidDirectionsCounter > 10 )
					{//..and reset if you try too many times
						switchAvoidDirectionsCounter = -1;
					}
				}
				else
				{
					switchAvoidDirectionsCounter = -1;
				}
			}
		}
	}
}

void CMoveSTAvoidObstacles::ProcessAvoidance( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	CActor* const avoidObstacle = data[ i_avoidObstacle ].Get();
	EAvoidObstacleStrategy& avoidObstacleStrategy = data[ i_avoidObstacleStrategy ];
	Bool& clearRotation = data[ i_clearRotation ];
	Float& requiredRotationChange = data[ i_requiredRotationChange ];
	Float& avoidanceTurnActive = data[ i_avoidanceTurnActive ];

	Bool allowToClearRotationThisFrame = true;

	if ( avoidObstacle )
	{
		CMovingAgentComponent& agent = comm.GetAgent();

		if ( avoidObstacleStrategy == AOS_Left ||
			 avoidObstacleStrategy == AOS_Right ||
			 avoidObstacleStrategy == AOS_Follow )
		{
			// modify heading to be on left or right side

			const Float curYaw = agent.GetWorldYaw();
			const Float agentRequestedMoveDirYaw = curYaw + comm.GetRotation();
			Float& predictedImpactTime = data[ i_predictedImpactTime ];

			Float desiredRequiredRotationChange = 0.0f;

			if ( avoidObstacleStrategy == AOS_Follow )
			{
				Vector obstacleVel = avoidObstacle->GetMovingAgentComponent()? avoidObstacle->GetMovingAgentComponent()->GetVelocity() : Vector::ZEROS;
				if ( obstacleVel.SquareMag2() < 0.1f )
				{
					avoidObstacleStrategy = AOS_Stop;
				}
				else
				{
					const Float obstacleMovDir = EulerAngles::YawFromXY( obstacleVel.X, obstacleVel.Y );
					desiredRequiredRotationChange = EulerAngles::AngleDistance( curYaw, obstacleMovDir );
				}
			}
			else
			{
				// use rotation to get requested movement direction
				//const Vector agentWorkingVel = agentRequestedMoveDir * comm.GetSpeed();
				Vector obstacleVel = avoidObstacle->GetMovingAgentComponent()? avoidObstacle->GetMovingAgentComponent()->GetVelocity() : Vector::ZEROS;
				Vector agentLocWS = agent.GetWorldPosition();
				Vector obstacleLocWS = avoidObstacle->GetWorldPosition();
				// check where obstacle will be using our predicted time
				obstacleLocWS += obstacleVel * predictedImpactTime;
				predictedImpactTime = Max( 0.0f, predictedImpactTime - timeDelta );
				// calculate avoidance
				const Vector toObstacleWS = obstacleLocWS - agentLocWS;
				// still going to hit each other
				const Vector toObstacleDirWS = toObstacleWS.Normalized3();
				const Vector toObstacleRightWS = Vector( toObstacleDirWS.Y, -toObstacleDirWS.X, 0.0f );
				// this is temp variable to be used with right vector and with calculation of rotation change (to keep values positive in given dir)
				const Float signAvoidance = avoidObstacleStrategy == AOS_Left? 1.0f : -1.0f;
				// where should we head to avoid collision (minus as left is +, right is -)
				const Vector toObstacleAvoidWS = toObstacleWS - toObstacleRightWS * signAvoidance * data[ i_avoidanceRadius ];
				const Float toObstacleAvoidYaw = EulerAngles::YawFromXY( toObstacleAvoidWS.X, toObstacleAvoidWS.Y );
				// variable names are self explanatory (and long)
				const Float requiredRotationToMoveInRequestedMoveDir = EulerAngles::AngleDistance( curYaw, agentRequestedMoveDirYaw );
				const Float requiredRotationToAvoidObstacle = EulerAngles::AngleDistance( curYaw, toObstacleAvoidYaw );
				// calculate how do we change rotation
				desiredRequiredRotationChange = Max( -5.0f, requiredRotationToMoveInRequestedMoveDir * signAvoidance, requiredRotationToAvoidObstacle * signAvoidance ) * signAvoidance;
			}

			// blend to allow smooth start
			avoidanceTurnActive = BlendToWithBlendTime( avoidanceTurnActive, Abs(desiredRequiredRotationChange) > 1.0f? 1.0f : 0.0f, 0.2f, timeDelta );
			// prevent oscillation and smooth
			if ( requiredRotationChange * desiredRequiredRotationChange <= 0.0f )
			{
				requiredRotationChange = BlendToWithBlendTime( requiredRotationChange, desiredRequiredRotationChange * avoidanceTurnActive, 0.1f, timeDelta );
			}
			else
			{
				requiredRotationChange = desiredRequiredRotationChange * avoidanceTurnActive;
			}
			// where exactly do we want to end up now?
			const Float weWantToMoveInDirYaw = curYaw + desiredRequiredRotationChange;
			const Float wellMovingInDirYaw = curYaw + requiredRotationChange;
			const Float requiredDirDiff = EulerAngles::AngleDistance( agentRequestedMoveDirYaw, weWantToMoveInDirYaw );
			// if it is too different from our requested movement direction or we're going to hit something, slow down...
			if ( (Abs( requiredDirDiff ) > 60.0f || predictedImpactTime < 1.0f) && m_modifySpeed )
			{
				ModifySpeed( comm, comm.GetSpeed() * 0.5f, m_avoidObstaclesImportance );
			}
			// ...or even stop
			if ( Abs( requiredDirDiff ) > 130.0f )
			{
				avoidObstacleStrategy = AOS_Stop;
			}

			if ( m_directionChangeOverride > 0.0f && agent.GetVelocity().SquareMag2() > m_minVelocityToOverrideDirectionChange * m_minVelocityToOverrideDirectionChange )
			{
				agent.SetDirectionChangeRate( m_directionChangeOverride );
			}

			// apply values
			if ( m_modifyRotation )
			{
				ModifyRotation( comm, requiredRotationChange, m_avoidObstaclesImportance );
				// to clear it afterwards
				clearRotation = true;
				allowToClearRotationThisFrame = false;
			}
			if ( m_modifyHeading )
			{
				Vector finalHeadingDirWS = EulerAngles::YawToVector( wellMovingInDirYaw );
				ModifyHeading( comm, finalHeadingDirWS, m_avoidObstaclesImportance );
			}
		}
		else
		{
			avoidanceTurnActive = BlendToWithBlendTime( avoidanceTurnActive, 0.0f, 0.2f, timeDelta );
		}

		if ( avoidObstacleStrategy == AOS_Stop )
		{
			comm.LockSpeed( 0.0f );
		}
		if ( avoidObstacleStrategy == AOS_SlowDown && m_modifySpeed )
		{
			ModifySpeed( comm, comm.GetSpeed() * 0.5f, m_avoidObstaclesImportance );
		}
		if ( avoidObstacleStrategy == AOS_SpeedUp && m_modifySpeed )
		{
			ModifySpeed( comm, comm.GetSpeed() * 1.5f, m_avoidObstaclesImportance );
		}
	}

	if ( allowToClearRotationThisFrame && clearRotation )
	{
		CMovingAgentComponent& agent = comm.GetAgent();
		clearRotation = false;
		requiredRotationChange = 0.0f;
		if ( m_modifyRotation )
		{
			ModifyRotation( comm, requiredRotationChange, m_avoidObstaclesImportance );
		}
		if ( m_modifyHeading )
		{
			const Vector currentForward = agent.GetWorldForward();
			const Vector2 currentHeadingWS = Vector2( currentForward.X, currentForward.Y );
			ModifyHeading( comm, currentHeadingWS, m_avoidObstaclesImportance );
		}
	}
}

void CMoveSTAvoidObstacles::ModifySpeed( IMovementCommandBuffer& comm, Float newSpeed, Float importance ) const
{
	if ( m_overrideValues )
	{
		comm.OverrideSpeed( newSpeed, importance );
	}
	else
	{
		comm.AddSpeed( newSpeed, importance );
	}
}

void CMoveSTAvoidObstacles::ModifyRotation( IMovementCommandBuffer& comm, Float rotationChange, Float importance ) const
{
	if ( m_overrideValues )
	{
		comm.OverrideRotation( rotationChange, importance );
	}
	else
	{
		comm.AddRotation( rotationChange, importance );
	}
}

void CMoveSTAvoidObstacles::ModifyHeading( IMovementCommandBuffer& comm, const Vector2& newHeading, Float importance ) const
{
	if ( m_overrideValues )
	{
		comm.OverrideHeading( newHeading, importance );
	}
	else
	{
		comm.AddHeading( newHeading, importance );
	}
}

String CMoveSTAvoidObstacles::GetTaskName() const
{
	static const String NAME( TXT( "AvoidObstacles" ) );
	return NAME;
}

void CMoveSTAvoidObstacles::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeToChooseNextObstacle;
	compiler << i_avoidingSameObstacleCount;
	compiler << i_switchAvoidDirectionsCounter;
	compiler << i_avoidObstacle;
	compiler << i_prevAvoidObstacle;
	compiler << i_avoidObstacleStrategy;
	compiler << i_clearRotation;
	compiler << i_requiredRotationChange;
	compiler << i_predictedImpactTime;
	compiler << i_obstacleColLoc;
	compiler << i_agentColLoc;
	compiler << i_avoidanceTurnActive;
	compiler << i_avoidanceRadius;
}

void CMoveSTAvoidObstacles::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	TBaseClass::OnInitData( agent, data );

	data[ i_timeToChooseNextObstacle ] = 0.0f;
	data[ i_avoidingSameObstacleCount ] = 0;
	data[ i_switchAvoidDirectionsCounter ] = 0;
	data[ i_avoidObstacle ]		= NULL;
	data[ i_prevAvoidObstacle ] = NULL;
	data[ i_avoidObstacleStrategy ] = AOS_None;
	data[ i_clearRotation ] = false;
	data[ i_requiredRotationChange ] = 0.0f;
	data[ i_predictedImpactTime ] = 0.0f;
	data[ i_obstacleColLoc ] = Vector::ZEROS;
	data[ i_agentColLoc ] = Vector::ZEROS;
	data[ i_avoidanceTurnActive ] = 0.0f;
	data[ i_avoidanceRadius ] = 2.5f;
}

void CMoveSTAvoidObstacles::OnDeinitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	TBaseClass::OnDeinitData( agent, data );

	data[ i_avoidObstacle ]		= NULL;
	data[ i_prevAvoidObstacle ] = NULL;
}

void CMoveSTAvoidObstacles::GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const
{
	TBaseClass::GenerateDebugFragments( agent, frame );

	InstanceBuffer* pData = agent.GetCurrentSteeringRuntimeData();

	if ( pData == nullptr )
	{
		return;
	}
	InstanceBuffer& data = *pData;
	//Float& timeToChooseNextObstacle = data[ i_timeToChooseNextObstacle ];
	CActor* const avoidObstacle = data[ i_avoidObstacle ].Get();
	EAvoidObstacleStrategy& avoidObstacleStrategy = data[ i_avoidObstacleStrategy ];

	if ( avoidObstacle )
	{
		Vector obstacleVel = avoidObstacle->GetMovingAgentComponent()? avoidObstacle->GetMovingAgentComponent()->GetVelocity() : Vector::ZEROS;
		Vector obstacleLocWS = avoidObstacle->GetWorldPosition();
		obstacleLocWS += obstacleVel * data[ i_predictedImpactTime ];
		frame->AddDebugLineWithArrow( agent.GetWorldPosition(), avoidObstacle->GetWorldPosition(), 1.0f, 0.5f, 1.0f, Color(255,100,100), true );
		frame->AddDebugLineWithArrow( avoidObstacle->GetWorldPosition(), obstacleLocWS, 1.0f, 0.2f, 0.2f, Color(255,150,100), true );
		if ( avoidObstacleStrategy == AOS_Follow )
		{
			frame->AddDebugLineWithArrow( agent.GetWorldPosition(), obstacleLocWS, 1.0f, 0.5f, 0.5f, Color(50,50,255), true );
		}
		if ( avoidObstacleStrategy == AOS_Left ||
			 avoidObstacleStrategy == AOS_Right )
		{
			frame->AddDebugLineWithArrow( agent.GetWorldPosition(), data[ i_agentColLoc ], 1.0f, 0.2f, 0.2f, Color(100,255,180), true );
			frame->AddDebugLineWithArrow( avoidObstacle->GetWorldPosition(), data[ i_obstacleColLoc ], 1.0f, 0.2f, 0.2f, Color(100,255,180), true );
			const Vector toObstacleWS = obstacleLocWS - agent.GetWorldPosition();
			const Vector toObstacleDirWS = toObstacleWS.Normalized3();
			const Vector toObstacleRightWS = Vector( toObstacleDirWS.Y, -toObstacleDirWS.X, 0.0f );
			Vector endAvoidArrow = obstacleLocWS + toObstacleRightWS * (avoidObstacleStrategy == AOS_Left? -1.0f : 1.0f);
			frame->AddDebugLineWithArrow( obstacleLocWS, endAvoidArrow, 1.0f, 0.5f, 0.5f, Color(100,255,100), true );
		}
	}
	if ( 0 )
	{
		const Vector textLoc = agent.GetWorldPosition();
		Uint32 line = 0;
		{
			frame->AddDebugText( textLoc, String::Printf( TXT("%s"), agent.GetEntity()->GetName().AsChar() ), 0, line * 2, true, Color(220,220,220), Color(0,0,0,128) );
			++ line;
			if ( avoidObstacle )
			{
				frame->AddDebugText( textLoc, String::Printf( TXT("%s %s"), avoidObstacle->GetName().AsChar(),
																			avoidObstacleStrategy == AOS_Left? TXT("left") :
																		  ( avoidObstacleStrategy == AOS_Right? TXT("right") :
																		  ( avoidObstacleStrategy == AOS_Stop? TXT("stop") : TXT("??") ) ) ),
									 0, line * 2, true, Color(220,120,120), Color(0,0,0,128) );
				++ line;
				frame->AddDebugText( textLoc, String::Printf( TXT("t:%.3f"), data[ i_predictedImpactTime ] ), 0, line * 2, true, Color(220,220,220), Color(0,0,0,128) );
				++ line;
			}
			frame->AddDebugText( textLoc, String::Printf( TXT("tl:%.3f"), data[ i_timeToChooseNextObstacle ] ), 0, line * 2, true, Color(220,220,220), Color(0,0,0,128) );
			++ line;
		}
		/*
		frame->AddDebugText( textLoc, String::Printf( TXT("r:%.2f'"), curRot.Yaw ), 0, line * 2, true, Color(220,220,220), Color(0,0,0,128) );
		++ line;
		frame->AddDebugText( textLoc, String::Printf( TXT("ar:%.2f'"), data[ i_requiredRotationChange ] ), 0, line * 2, true, Color(220,220,220), Color(0,0,0,128) );
		++ line;
		*/
	}
}

///////////////////////////////////////////////////////////////////////////////
void CMoveSTCollisionResponse::CollisionResponse( IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	struct Functor : public CActorsManager::DefaultFunctor
	{
		enum { SORT_OUTPUT = false };
		Functor( const Vector& pos, Float separationDistance, CActor* owner )
			: m_pos( pos )
			, m_separationDistance( separationDistance )
			, m_output( 0.f, 0.f )
			, m_effected( false )
			, m_owner( owner ) {}
		RED_INLINE Bool operator()( CActor* actor )
		{
			if ( actor == m_owner )
			{
				return true;
			}
			const Vector& actorPos = actor->GetWorldPositionRef();
			CMovingAgentComponent* actorMac = actor->GetMovingAgentComponent();
			if ( !actorMac )
			{
				return true;
			}
			Float actorRadius = actorMac->GetRadius();
			Float separationDistance = m_separationDistance + actorRadius;
			Vector2 diff = m_pos - actorPos.AsVector2();
			Float distSq = diff.SquareMag();
			if ( distSq < NumericLimits< Float >::Epsilon() )
			{
				// move forward
				Vector2 moveVec = actor->GetWorldForward().AsVector2();
				m_output += moveVec;
				m_effected = true;
				return true;
			}
			else if ( distSq > separationDistance*separationDistance )
			{
				return true;
			}
			Float dist = sqrt( distSq );

			Float distRatio = 1.f - (dist / separationDistance);

			diff *= distRatio / dist;				// set length to actorMass

			m_output += diff;

			m_effected = true;

			return true;
		}

		Vector			m_pos;
		Float			m_personalSpace;
		Float			m_separationDistance;
		Vector2			m_output;
		Bool			m_effected;
		CActor*			m_owner;
	};

	CMovingAgentComponent& mac = comm.GetAgent();
	Float radius = mac.GetRadius() * m_radiusMult;
	const Vector& pos = mac.GetWorldPositionRef();
	CActor* actor = static_cast< CActor* >( mac.GetEntity() );

	Functor functor( pos, radius, actor );
	Box testBBox( Vector( -radius, -radius, -1.f ), Vector( radius, radius, 2.f ) );
	GCommonGame->GetActorsManager()->TQuery( pos, functor, testBBox, true, nullptr, 0 );

	if ( !functor.m_effected )
	{
		return;
	}

	Float importance;
	Float outputLen = functor.m_output.Mag();
	if ( outputLen > 1.f )
	{
		functor.m_output *= 1.f / outputLen;
		importance = m_headingImportanceMax;
	}
	else
	{
		importance = m_headingImportanceMin + ( m_headingImportanceMax - m_headingImportanceMin ) * outputLen;
	}

	if ( importance > NumericLimits< Float >::Epsilon() )
	{
		comm.AddHeading( functor.m_output, importance );
	}
}

void CMoveSTCollisionResponse::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	CollisionResponse( comm, data );
}

String CMoveSTCollisionResponse::GetTaskName() const
{
	static const String NAME( TXT( "CollisionResponse" ) );
	return NAME;
}

void CMoveSTCollisionResponse::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
}

void CMoveSTCollisionResponse::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	TBaseClass::OnInitData( agent, data );
}

void CMoveSTCollisionResponse::OnDeinitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	TBaseClass::OnDeinitData( agent, data );
}


///////////////////////////////////////////////////////////////////////////////
Float CMoveSTForwardCollisionResponse::ForwardCollisionResponse( IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	struct Functor : public CActorsManager::DefaultFunctor
	{
		enum { SORT_OUTPUT = false };
		Functor( const Vector& pos, Vector2& heading, Float separationDistance, CActor* owner, CNode* ignoreTarget )
			: m_pos( pos.AsVector2() )
			, m_posDestination( pos.AsVector2() + heading )
			, m_heading( heading )
			, m_separationDistance( separationDistance )
			, m_importanceRatio( 0.f )
			, m_output( 0.f, 0.f )
			, m_effected( false )
			, m_closestToRight( false )
			, m_owner( owner )
			, m_ignoreTarget( ignoreTarget ) {}
		RED_INLINE Bool operator()( CActor* actor )
		{
			if ( actor == m_owner || actor == m_ignoreTarget )
			{
				return true;
			}
			const Vector& actorPos = actor->GetWorldPositionRef();
			CMovingAgentComponent* actorMac = actor->GetMovingAgentComponent();
			if ( !( actorMac && actorMac->IsCharacterCollisionsEnabled() ) )
			{
				return true;
			}
			Float actorRadius = actorMac->GetAvoidanceRadius();
			Float separationDistance = m_separationDistance + actorRadius;
			Vector2 point;
			Float ratio;
			MathUtils::GeometryUtils::TestClosestPointOnLine2D( actorPos, m_posDestination, m_pos, ratio, point );

			Vector2 diff = point - actorPos.AsVector2();
			Float distSq = diff.SquareMag();
			if ( distSq < NumericLimits< Float >::Epsilon() )
			{
				m_output = MathUtils::GeometryUtils::PerpendicularR( m_heading.Normalized() );
				m_effected = true;
				m_importanceRatio = 1.f;
				return true;
			}
			else if ( distSq > separationDistance*separationDistance )
			{
				return true;
			}
			Float dist = sqrt( distSq );

			diff *= ratio * ( separationDistance - dist ) / dist;

			m_output += diff;

			m_effected = true;

			if ( ratio > m_importanceRatio )
			{
				m_importanceRatio = ratio;
				m_closestToRight = m_heading.CrossZ( actorPos - m_pos ) >= 0.f;
			}

			return true;
		}

		Vector2			m_pos;
		Vector2			m_posDestination;
		Vector2			m_heading;
		Float			m_personalSpace;
		Float			m_separationDistance;
		Float			m_importanceRatio;
		Vector2			m_output;
		Bool			m_effected;
		Bool			m_closestToRight;
		CActor*			m_owner;
		CNode*			m_ignoreTarget;
	};

	const auto& goal = comm.GetGoal();
	if ( !goal.IsHeadingGoalSet() )
	{
		TBaseClass::CollisionResponse( comm, data );
		return 0.f;
	}
	CMovingAgentComponent& mac = comm.GetAgent();
	Vector2 currHeading = comm.GetHeading() * (comm.GetSpeed() * m_probeDistanceInTime * mac.GetMaxSpeed());
	if ( currHeading.IsAlmostZero() )
	{
		TBaseClass::CollisionResponse( comm, data );
		return 0.f;
	}
	
	Float radius = mac.GetRadius() * m_radiusMult;
	const Vector& pos = mac.GetWorldPositionRef();
	CActor* actor = static_cast< CActor* >( mac.GetEntity() );

	Functor functor( pos, currHeading, radius, actor, goal.GetGoalTargetNode() );
	Box testBBox( Vector( -radius, -radius, -1.f ), Vector( radius, radius, 2.f ) );
	testBBox.Min.X = Min( testBBox.Min.X, currHeading.X - radius );
	testBBox.Min.Y = Min( testBBox.Min.Y, currHeading.Y - radius );
	testBBox.Max.X = Max( testBBox.Max.X, currHeading.X + radius );
	testBBox.Max.Y = Max( testBBox.Max.Y, currHeading.Y + radius );

	GCommonGame->GetActorsManager()->TQuery( pos, functor, testBBox, true, nullptr, 0 );

	if ( !functor.m_effected || functor.m_output.IsAlmostZero() )
	{
		return 0.f;
	}

	Float outputLen = functor.m_output.Mag();
	functor.m_output *= 1.f / outputLen;
	Float importance = m_headingImportanceMin + ( m_headingImportanceMax - m_headingImportanceMin ) * functor.m_importanceRatio;

	if ( importance > NumericLimits< Float >::Epsilon() )
	{
		comm.AddHeading( functor.m_output, importance );
		return functor.m_closestToRight ? functor.m_importanceRatio : -functor.m_importanceRatio;
	}
	return 0.f;
}

void CMoveSTForwardCollisionResponse::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	Float ratio = ForwardCollisionResponse( comm, data );
	comm.SetVar( BVC_Extra, m_crowdThroughVar, ratio );
	data[ i_crowdThrough ] = ratio;
}


void CMoveSTForwardCollisionResponse::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_crowdThrough;

	TBaseClass::OnBuildDataLayout( compiler );
}
void CMoveSTForwardCollisionResponse::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	data[ i_crowdThrough ] = 0.f;

	TBaseClass::OnInitData( agent, data );
}

void CMoveSTForwardCollisionResponse::DeactivateAnimation( CMovingAgentComponent& agent, InstanceBuffer& data ) const
{
	if ( !m_crowdThroughVar.Empty() && data[ i_crowdThrough ] != 0.f )
	{
		CBehaviorGraphStack* stack = agent.GetBehaviorStack();
		if ( stack )
		{
			stack->SetBehaviorVariable( m_crowdThroughVar, 0.f );
		}
	}
}

void CMoveSTForwardCollisionResponse::OnBranchDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const
{
	DeactivateAnimation( agent, data );

	TBaseClass::OnBranchDeactivation( agent, data );
}

void CMoveSTForwardCollisionResponse::OnGraphDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const
{
	DeactivateAnimation( agent, data );

	TBaseClass::OnGraphDeactivation( agent, data );
}

String CMoveSTForwardCollisionResponse::GetTaskName() const
{
	return TXT("ForwardCollisionResponse");
}
///////////////////////////////////////////////////////////////////////////////

CMoveSTAdjustRotationChanges::CMoveSTAdjustRotationChanges()
	: m_scenario( ARCS_None )
{
}

String CMoveSTAdjustRotationChanges::GetTaskName() const
{
	static const String NAME( TXT( "AdjustRotationChanges" ) );
	return NAME;
}

void CMoveSTAdjustRotationChanges::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_doingLargeTurnStart;
	compiler << i_largeTurnStartRotationRate;
	compiler << i_stopping;
}

void CMoveSTAdjustRotationChanges::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	TBaseClass::OnInitData( agent, data );

	data[ i_doingLargeTurnStart ] = false;
	data[ i_stopping ] = false;
}

void CMoveSTAdjustRotationChanges::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	/*
	CMovingAgentComponent& agent = comm.GetAgent();

	switch ( m_scenario )
	{
	case ARCS_CasualMovement:
		// modify only if we have somewhere to go
		if ( comm.GetHeading().SquareMag() > 0.0f )
		{
			const Vector2& requestedMovementDirection = comm.GetHeading();
			Float absFacingRotationToMovementDirectionDiff = Abs( EulerAngles::AngleDistance( agent.GetWorldYaw(), EulerAngles::YawFromXY( requestedMovementDirection.X, requestedMovementDirection.Y ) ) );
			if ( data[ i_stopping ] )
			{
				// stop before doing anything else
				comm.LockSpeed( 0.0f );
				comm.LockHeading( EulerAngles::YawToVector2( agent.GetMoveDirectionWorldSpace() ) );
				if ( agent.GetVelocityBasedOnRequestedMovement().Mag2() < 0.05f )
				{
					data[ i_stopping ] = false;
					const Float treshold = 40.0f;
					if ( absFacingRotationToMovementDirectionDiff > treshold )
					{
						// we want to do large turn while standing
						data[ i_doingLargeTurnStart ] = true;
						data[ i_largeTurnStartRotationRate ] = ( ( absFacingRotationToMovementDirectionDiff - treshold ) / ( 180.0f - treshold ) ) * 180.0f + 240.0f;

					}
				}
			}
			else if ( absFacingRotationToMovementDirectionDiff > 60.0f && agent.GetVelocityBasedOnRequestedMovement().Mag2() > 0.7f && ! data[ i_doingLargeTurnStart ] )
			{
				// we should stop now
				comm.LockSpeed( Min( comm.GetSpeed(), 0.6f ) );
				comm.LockHeading( EulerAngles::YawToVector2( agent.GetMoveDirectionWorldSpace() ) );
				data[ i_stopping ] = true;
			}
			else
			{
				if ( absFacingRotationToMovementDirectionDiff > 30.0f )
				{
					// slow down on sharp turns
					comm.LockSpeed( 0.6f ); 
				}
				if ( data[ i_doingLargeTurnStart ] )
				{
					agent.SetMaxRotation( data[ i_largeTurnStartRotationRate ] );
					agent.SetDirectionChangeRate( data[ i_largeTurnStartRotationRate ] );
					if ( absFacingRotationToMovementDirectionDiff < 10.0f )
					{
						// initial turn has finished
						data[ i_doingLargeTurnStart ] = false;
					}
				}
				else
				{
					// adjust rotation rate
					const Float lowFacingToMovmentDiffTre = 10.0f;
					const Float highFacingToMovmentDiffTre = 60.0f;
					const Float highFacingToMovmentDiffMax = 110.0f;
					const Float veryHighFacingToMovmentDiffTre = 120.0f;
					const Float vertHighFacingToMovmentDiffMax = 180.0f;
					if ( absFacingRotationToMovementDirectionDiff < lowFacingToMovmentDiffTre )
					{
						const Float useLowCoef = ( lowFacingToMovmentDiffTre - absFacingRotationToMovementDirectionDiff ) / lowFacingToMovmentDiffTre;
						const Float curMaxRotation = agent.GetMaxRotation();
						const Float lowRotationChange = Min( 30.0f, curMaxRotation );
						agent.SetMaxRotation( lowRotationChange * useLowCoef + (1.0f - useLowCoef ) * curMaxRotation );
					}
					else if ( absFacingRotationToMovementDirectionDiff > highFacingToMovmentDiffTre )
					{
						const Float useHighCoef = Min( 1.0f, ( absFacingRotationToMovementDirectionDiff - highFacingToMovmentDiffTre ) / ( highFacingToMovmentDiffMax - highFacingToMovmentDiffTre ) );
						const Float curMaxRotation = agent.GetMaxRotation();
						const Float highRotationChange = Max( 140.0f, curMaxRotation );
						agent.SetMaxRotation( highRotationChange * useHighCoef + (1.0f - useHighCoef ) * curMaxRotation );
					}
					else if ( absFacingRotationToMovementDirectionDiff > veryHighFacingToMovmentDiffTre )
					{
						const Float useHighCoef = Min( 1.0f, ( absFacingRotationToMovementDirectionDiff - veryHighFacingToMovmentDiffTre ) / ( vertHighFacingToMovmentDiffMax - veryHighFacingToMovmentDiffTre ) );
						const Float curMaxRotation = agent.GetMaxRotation();
						const Float highRotationChange = Max( 300.0f, curMaxRotation );
						agent.SetMaxRotation( highRotationChange * useHighCoef + (1.0f - useHighCoef ) * curMaxRotation );
					}
				}
			}
		}
		break;
	}*/
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSTKeepAwayTarget
///////////////////////////////////////////////////////////////////////////////
void CMoveSTKeepAwayTarget::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	// Getting the goal from where the gravity will apply
	const SMoveLocomotionGoal& goal	= comm.GetGoal();
	CNode *const targetNode		= goal.GetGoalTargetNode();
	if ( targetNode )
	{
		Vector heading	= comm.GetAgent().GetWorldPosition() - targetNode->GetWorldPositionRef();
		heading.Normalize2();
		if ( m_headingImportance < 0.f )
		{
			heading = -heading;
		}
		comm.AddHeading( heading, fabs( m_headingImportance ) );
		comm.AddSpeed( m_speed, 1.0f );
	}
}

String CMoveSTKeepAwayTarget::GetTaskName() const
{
	static const String TASKNAME( TXT( "Keep away Target" ) );
	return TASKNAME;
}
///////////////////////////////////////////////////////////////////////////////
// CMoveSTMoveTightening
///////////////////////////////////////////////////////////////////////////////
void CMoveSTMoveTightening::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	Vector tightening;
	if ( comm.GetGoal().GetFlag( CNAME( Tightening ), tightening ) )
	{
		Vector2 followPoint = tightening.AsVector2();
		Float tighteningDir = tightening.Z;
		Float tighteningRatio = tightening.W;
		CMovingAgentComponent& agent = comm.GetAgent();
		agent.GetMovementAdjustor()->TightenMovementTo( followPoint, tighteningDir, tighteningRatio );
	}
}

String CMoveSTMoveTightening::GetTaskName() const
{
	return TXT("Path tightening");
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSTResolveStucking
///////////////////////////////////////////////////////////////////////////////

void CMoveSTResolveStucking::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_isColliding;
	compiler << i_numberOfFramesStuck;
}

void CMoveSTResolveStucking::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	TBaseClass::OnInitData( agent, data );

	data[ i_isColliding ] = false;
	data[ i_numberOfFramesStuck ] = 0;
}

Bool CMoveSTResolveStucking::IsColliding( const CActor& actor, float radius ) const
{
	struct Functor : public CActorsManager::DefaultFunctor
	{
		enum { SORT_OUTPUT = false };

		Functor( const CActor& selfActor )
			: m_selfActor( selfActor )
			, m_selfRadius( selfActor.GetMovingAgentComponent()->GetAvoidanceRadius() )
			, m_isColliding( false )
		{}

		RED_INLINE Bool operator()( CActor* actor )
		{
			if ( actor == &m_selfActor)
			{
				return true;
			}

			CNewNPC* npc = Cast< CNewNPC >( actor );
			if ( npc )
			{
				const Float distanceSquared = m_selfActor.GetPosition().DistanceSquaredTo2D( actor->GetPosition() );
				const Float radiusesSum = m_selfRadius + actor->GetMovingAgentComponent()->GetAvoidanceRadius();
				if ( distanceSquared <= radiusesSum * radiusesSum )
				{
					m_isColliding = true;
					return false;
				}
			}
			return true;
		}

		const CActor& m_selfActor;
		Float m_selfRadius;
		Bool m_isColliding;

	} functor( actor );

	const Box testBox( Vector::ZEROS, radius );
	GCommonGame->GetActorsManager()->TQuery( actor, functor, testBox, true, nullptr, 0 );

	return functor.m_isColliding;
}

void CMoveSTResolveStucking::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	CMovingAgentComponent& agent = comm.GetAgent();

	if ( agent.WasSeparatedBy() )
	{
		CActor* collidingActor = Cast< CActor >( agent.WasSeparatedBy()->GetEntity() );
		// ask colliding actor to move away from path
		if ( collidingActor && collidingActor->SignalGameplayEventReturnInt( m_signalName, 0 ) )
		{
			return;
		}

		Bool disableCollision = false;
		if ( agent.GetVelocity().AsVector3().IsZero() && !agent.GetVelocityBasedOnRequestedMovement().AsVector3().IsAlmostZero() )
		{
			disableCollision = true;
		}
		else if ( agent.GetAgentPosition().DistanceSquaredTo( agent.GetAgentPrevPosition() ) < m_distanceThreshold )
		{
			if ( ++data[ i_numberOfFramesStuck ] == m_stuckFramesThreshold )
			{
				disableCollision = true;
			}
		}
		else
		{
			data[ i_numberOfFramesStuck ] = 0;
		}

		if ( disableCollision )
		{
			data[ i_isColliding ] = true;
			data[ i_numberOfFramesStuck ] = 0;
			CMovingPhysicalAgentComponent* physicalAgent = Cast< CMovingPhysicalAgentComponent >( &agent );
			if ( physicalAgent )
			{
				AI_LOG( TXT("CMoveSTResolveStucking::CalculateSteering: disabling collision for actor %s"), agent.GetEntity()->GetName().AsChar() );
				physicalAgent->EnableCharacterCollisions( false );
			}
		}
	}
	else
	{
		data[ i_numberOfFramesStuck ] = 0;

		if ( data[ i_isColliding ] )
		{
			CActor* actor = Cast< CActor >( agent.GetEntity() );
			if ( actor )
			{
				CMovingPhysicalAgentComponent* physicalAgent = Cast< CMovingPhysicalAgentComponent >( &agent );
				if ( physicalAgent && !IsColliding( *actor, physicalAgent->GetVirtualRadius() ) )
				{
					data[ i_isColliding ] = false;
					physicalAgent->EnableCharacterCollisions( true );
				}
			}
		}
	}
}

void CMoveSTResolveStucking::OnDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const
{
	if ( data[ i_isColliding ] )
	{
		data[ i_isColliding ] = false;
		CMovingPhysicalAgentComponent* physicalAgent = Cast< CMovingPhysicalAgentComponent >( &agent );
		if ( physicalAgent && !physicalAgent->IsCharacterCollisionsEnabled() )
		{
			physicalAgent->EnableCharacterCollisions( true );
		}
	}
}

void CMoveSTResolveStucking::OnGraphDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const
{
	OnDeactivation( agent, data );

	TBaseClass::OnGraphDeactivation( agent, data );
}


void CMoveSTResolveStucking::OnBranchDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const
{
	OnDeactivation( agent, data );

	TBaseClass::OnBranchDeactivation( agent, data );
}

String CMoveSTResolveStucking::GetTaskName() const
{
	return TXT("Resolve stucking");
}
