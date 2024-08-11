#include "build.h"
#include "moveOnRoadSteering.h"

#include "../../common/engine/pathComponent.h"
#include "../../common/engine/stripeComponent.h"
#include "../../common/engine/renderFrame.h"

#include "../../common/core/mathUtils.h"
#include "../../common/core/instanceDataLayoutCompiler.h"

#include "../../common/game/movableRepresentationPathAgent.h"
#include "../../common/game/movementCommandBuffer.h"


IMPLEMENT_ENGINE_CLASS( CMoveSCRoadMovement )
IMPLEMENT_ENGINE_CLASS( CMoveSTOnRoad )


namespace
{
	static RED_INLINE const CNode* GetRoadCurve( SMoveLocomotionGoal& goal, const SMultiCurve*& roadCurve, Float& roadWidth )
	{
		CNode* node;
		if ( !goal.TGetFlag( CNAME( Road ), node ) )
		{
			return nullptr;
		}
		{
			CStripeComponent* stripe = Cast< CStripeComponent >( node );
			if ( stripe )
			{
				roadCurve = &stripe->RequestCurve();
				roadWidth = stripe->GetStripeWidth() * 0.5f;
				return stripe;
			}
		}
		{
			CPathComponent* pathComponent = Cast< CPathComponent >( node );
			if ( pathComponent )
			{
				roadCurve = &pathComponent->GetCurve();
				roadWidth = 1.f;
				return pathComponent;
			}
		}
		return nullptr;
	}

};

///////////////////////////////////////////////////////////////////////////////
// CMoveSCRoadMovement	
///////////////////////////////////////////////////////////////////////////////
Bool CMoveSCRoadMovement::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	Bool isFollowingRoad = false;
	comm.GetGoal().GetFlag( CNAME( IsFollowingRoad ), isFollowingRoad );
	return isFollowingRoad;
}

String CMoveSCRoadMovement::GetConditionName() const
{
	return TXT("IsOnRoad");
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSTOnRoad
///////////////////////////////////////////////////////////////////////////////
CMoveSTOnRoad::CMoveSTOnRoad()
	: m_headingImportance( 1.0f )
	, m_speedImportance( 1.0f )
	, m_anticipatedPositionDistance( 4.f )
	, m_roadMaxDist( 4.f )
{
}

Float CMoveSTOnRoad::DetermineDirection( const SMultiCurve& curve, InstanceBuffer& data, const CPathAgent& pathAgent ) const
{
	if ( pathAgent.GetCurrentWaypointIdx() == data[ i_cachedWaypoint ] )
	{
		return data[ i_cachedDirection ];
	}
	SMultiCurvePosition firstWaypointPositionOnCurve;
	SMultiCurvePosition secondWaypointPositionOnCurve;
	Vector firstWaypointClosestSpot;
	Vector secondWaypointClosestSpot;
	Vector3 posOrigin, posDestination;
	pathAgent.GetCurrentMetalinkWaypoints( posOrigin, posDestination );
	curve.GetClosestPointOnCurve( posOrigin, firstWaypointPositionOnCurve, firstWaypointClosestSpot );
	curve.GetClosestPointOnCurve( posDestination, secondWaypointPositionOnCurve, secondWaypointClosestSpot );

	Float roadDirection = 0.0f;
	if ( firstWaypointPositionOnCurve.m_edgeIdx != secondWaypointPositionOnCurve.m_edgeIdx )
	{
		roadDirection = MSign( (Float)secondWaypointPositionOnCurve.m_edgeIdx - firstWaypointPositionOnCurve.m_edgeIdx ); 
	}
	else
	{
		if ( firstWaypointPositionOnCurve.m_edgeAlpha != secondWaypointPositionOnCurve.m_edgeAlpha )
		{
			roadDirection = MSign( secondWaypointPositionOnCurve.m_edgeAlpha - firstWaypointPositionOnCurve.m_edgeAlpha ); 
		}
	}

	data[ i_cachedDirection ] = roadDirection;
	data[ i_cachedWaypoint ] = pathAgent.GetCurrentWaypointIdx();

	return roadDirection;
}

void CMoveSTOnRoad::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	SMoveLocomotionGoal&	goal		= comm.GetGoal();

	const SMultiCurve*		roadCurve;
	Float					roadWidth;

	const CNode* roadNode = ::GetRoadCurve( goal, roadCurve, roadWidth );
	if ( !roadNode || !goal.IsHeadingGoalSet() )
	{
		SetRState( data, RSTATE_OFF );
		return;
	}
	const CMovingAgentComponent& mac	= comm.GetAgent();
	CPathAgent* pathAgent				= mac.GetPathAgent();
			
	const Vector actorPosition	= mac.GetWorldPositionRef();
	const Vector2 actorHeading	= mac.GetWorldForward();

	// fist test if our current position is making us 'go away' of road
	pathAgent->UpdatePathfollowFromVirtualPosition( pathAgent->GetPosition(), 1.f );

	SMultiCurvePosition &actorCurvePosition			= data[ i_actorCurvePosition ];
	SMultiCurvePosition &anticipatedCurvePosition	= data[ i_anticipatedCurvePosition ];
	THandle< CNode > &lastCurveNode					= data[ i_lastCurve ];
	//Float &randPositionOnRoad						= data[ i_randPositionOnRoad ];

	// at first we need to determine if we are continuing movement, or do we need to recalculate our position from start
	Bool continueMovement = false;

	if ( lastCurveNode.Get() == roadNode )
	{
		continueMovement = true;
		// NOTICE: Looks like above will be a-ok without more detailed version below
		//Vector lastCurvePoint;
		//roadCurve->GetCurvePoint( actorCurvePosition, lastCurvePoint );
		//Float toleranceDistance = roadWidth + m_anticipatedPositionDistance + 1.f;
		//if ( (lastCurvePoint.AsVector2() - actorPosition.AsVector2()).SquareMag() < toleranceDistance*toleranceDistance )
		//{
		//	continueMovement = true;
		//}
	}
	else
	{
		lastCurveNode = roadNode;
	}

	Vector actorPositionOnPath;

	if ( continueMovement )
	{
		roadCurve->CorrectCurvePoint( actorPosition, actorCurvePosition, actorPositionOnPath );
	}
	else
	{
		roadCurve->GetClosestPointOnCurve( actorPosition, actorCurvePosition, actorPositionOnPath );

		//randPositionOnRoad = GEngine->GetRandomNumberGenerator().Get< Float >( 0.2f, 1.0f );
	}
	
	pathAgent->UpdatePathfollowFromVirtualPosition( actorPositionOnPath.AsVector3(), 1.f );

	Float distanceFromPathSq = (actorPosition.AsVector2() - actorPositionOnPath.AsVector2()).SquareMag();
	Float maxDist = m_roadMaxDist+roadWidth;
	if ( distanceFromPathSq > maxDist*maxDist )
	{
		SetRState( data, RSTATE_TOO_FAR );
		return;
	}
	
	if ( !pathAgent->TestLine( actorPositionOnPath ) )
	{
		SetRState( data, RSTATE_NO_LINE_TO_ROAD );
		return;
	}

	// determine shouldn't we stop following on roads because we are closing to way out
	// HACK: This should be more customizable, more general and stuff. But we need to close this feature. Now.
	{
		Uint32 currentMetalinkIdx = pathAgent->GetCurrentMetalinkIdx();
		Uint32 metalinksCount = pathAgent->GetMetalinksCount();
		if ( currentMetalinkIdx < metalinksCount )
		{
			const auto& metalinkData = pathAgent->GetMetalink( currentMetalinkIdx );
			Uint32 currentWaypoint = metalinkData.m_waypointIndex;
			Uint32 waypointCount = pathAgent->GetWaypointsCount();
			if ( currentWaypoint+2 < waypointCount
				&& (currentMetalinkIdx+1 >= metalinksCount || pathAgent->GetMetalink(currentMetalinkIdx+1).m_waypointIndex > currentWaypoint+2 ) )
			{
				// try to leave the road
				const Float COS60 = 0.5f; // HACK HACK HACK
				Vector2 wp = pathAgent->GetWaypoint( currentWaypoint+1 ).AsVector2();
				Vector3 nextWp = pathAgent->GetWaypoint( currentWaypoint+2 );

				Vector2 diffNextPoint = (nextWp.AsVector2() - actorPosition.AsVector2()).Normalized();
				Vector2 diffCurrPoint = (wp - actorPosition.AsVector2()).Normalized();

				if ( diffNextPoint.Dot( diffCurrPoint ) < COS60 && pathAgent->TestLine( nextWp ) )
				{
					SetRState( data, RSTATE_WALKING_OUT );
					// If we can reach further waypoint, mark metalink completed
					if ( pathAgent->TestLine( actorPosition.AsVector3(), nextWp, pathAgent->GetPersonalSpace() ) )
					{
						pathAgent->NextWaypoint();
					}
					return;
				}
			}
				
		}
	}
	
	// determine direction we are moving to
	Float direction = DetermineDirection( *roadCurve, data, *pathAgent );

	// determine anticipated position
	anticipatedCurvePosition = actorCurvePosition;
	Vector anticipatedPositionOnPath;
	Bool isEndOfPath;
	roadCurve->GetPointOnCurveInDistance( anticipatedCurvePosition, m_anticipatedPositionDistance * direction, anticipatedPositionOnPath, isEndOfPath );

	// we have anticipated position, now lets move it away from curve
	Vector pathTangentAtAnticipatedPosition;
	roadCurve->CalculateAbsoluteTangentFromCurveDirection( anticipatedCurvePosition, pathTangentAtAnticipatedPosition );

	Float distanceFromPath = sqrt( distanceFromPathSq );
	Float desiredDistanceToPath = Clamp( distanceFromPath, 0.2f * roadWidth, 0.65f * roadWidth );			// FUN FACT: It actually nicely convert case of being on wrong pavement side
	Vector2 perpendicularOffset = MathUtils::GeometryUtils::PerpendicularR( pathTangentAtAnticipatedPosition.AsVector2() );
	perpendicularOffset.Normalize();
	perpendicularOffset *= direction*desiredDistanceToPath;

	Vector3 desiredSpot = anticipatedPositionOnPath.AsVector3();
	desiredSpot.AsVector2() += perpendicularOffset;

#ifdef DEBUG_ROAD_MOVEMENT
	data[ i_currentFollowPos ] = desiredSpot;
#endif


	if ( !pathAgent->TestLine( desiredSpot ) )
	{
		SetRState( data, RSTATE_NO_LINE_TO_DEST );
		return;
	}

	Vector2 headingOutput = ( desiredSpot.AsVector2() - actorPosition.AsVector2() ).Normalized();
	comm.AddHeading( headingOutput, m_headingImportance );

	SetRState( data, RSTATE_OK );

	if ( m_speedImportance > 0.f )
	{
		const Float speed = goal.IsSpeedGoalSet() ? (goal.GetDesiredSpeed() / mac.GetMaxSpeed()) : 1.f;
		comm.AddSpeed( speed, m_speedImportance );

	}
	goal.SetFlag( CNAME( IsFollowingRoad ), true );
}

String CMoveSTOnRoad::GetTaskName() const
{
	static const String TASKNAME( TXT( "MoveOnRoad" ) );
	return TASKNAME;
}

void CMoveSTOnRoad::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_actorCurvePosition;
	compiler << i_anticipatedCurvePosition;
	compiler << i_lastCurve;
	compiler << i_cachedWaypoint;
	compiler << i_cachedDirection;

#ifdef DEBUG_ROAD_MOVEMENT
	compiler << i_currentFollowPos;
	compiler << i_currentState;
#endif

	TBaseClass::OnBuildDataLayout( compiler );
}

void CMoveSTOnRoad::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	data[ i_actorCurvePosition ].m_edgeIdx			= -1;
	data[ i_actorCurvePosition ].m_edgeAlpha		= 0;
	data[ i_anticipatedCurvePosition ].m_edgeIdx	= -1;
	data[ i_anticipatedCurvePosition ].m_edgeAlpha	= 0;
	data[ i_cachedWaypoint ] = -1;
	data[ i_cachedDirection ] = 0.0f;

	
#ifdef DEBUG_ROAD_MOVEMENT
	data[ i_currentFollowPos ] = agent.GetWorldPositionRef();
	data[ i_currentState ] = 0;
#endif

	TBaseClass::OnInitData( agent, data );
}

void CMoveSTOnRoad::OnGraphActivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const
{
	CPathAgent* pathAgent = agent.GetPathAgent();
	if ( pathAgent )
	{
		pathAgent->SupportRoads( true );
	}
}
void CMoveSTOnRoad::OnGraphDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const
{
	CPathAgent* pathAgent = agent.GetPathAgent();
	if ( pathAgent )
	{
		pathAgent->SupportRoads( false );
	}

	data[ i_cachedWaypoint ] = -1;
	data[ i_cachedDirection ] = 0.0f;
}

#ifdef DEBUG_ROAD_MOVEMENT
void CMoveSTOnRoad::GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const
{
	TBaseClass::GenerateDebugFragments( agent, frame );

	InstanceBuffer * data = agent.GetCurrentSteeringRuntimeData();
	if ( data )
	{
		String desc;
		Bool showTarget = false;
		switch ( (*data)[ i_currentState ] )
		{
		case RSTATE_OK:
			desc = TXT("road: Ok");
			showTarget = true;
			break;
		case RSTATE_OFF:
			break;
		case RSTATE_TOO_FAR:
			desc = TXT("road: Too far");
			break;
		case RSTATE_WALKING_OUT:
			desc = TXT("road: go out");
			break;
		case RSTATE_NO_LINE_TO_ROAD:
			desc = TXT("road: no line to road");
			break;
		case RSTATE_NO_LINE_TO_DEST:
			desc = TXT("roadL no line to dest");
			showTarget = true;
			break;
		default:
			break;
		}

		if ( !desc.Empty() )
		{
			frame->AddDebugText( agent.GetWorldPositionRef(), desc, -30, -2, true, Color::GREEN );
		}

		if ( showTarget )
		{
			const Vector3& roadTargetPosition = (*data)[ i_currentFollowPos ];
			frame->AddDebugSphere( roadTargetPosition, 0.2f, Matrix::IDENTITY, Color::RED ); 	
		}

		Int32 edgeIdx = (*data)[ i_actorCurvePosition ].m_edgeIdx;
		Float alpha = (*data)[ i_actorCurvePosition ].m_edgeAlpha;


		/* debug stuff foe drawing current waypoint, edge index and edge alpha
		{
			frame->AddDebugText( agent.GetWorldPositionRef(), String::Printf( TXT("edgeIdx = %d, edgeAlpha = %f"), edgeIdx, alpha ).AsChar(), -30, -3, true, Color::GREEN );

			CPathAgent* pathAgent = agent.GetPathAgent();

			if ( pathAgent->GetWaypointsCount() > 0 )
			{
				Uint32 currentWaypointIdx = pathAgent->GetCurrentWaypointIdx();
				if ( currentWaypointIdx > 0 )
				{
					frame->AddDebugSphere( pathAgent->GetWaypoint(currentWaypointIdx - 1), 0.1f, Matrix::IDENTITY, Color::LIGHT_GREEN, true, true ); 	
				}

				frame->AddDebugSphere( pathAgent->GetWaypoint(currentWaypointIdx), 0.1f, Matrix::IDENTITY, Color::GREEN, true, true ); 

				if ( currentWaypointIdx + 1 < pathAgent->GetOutputWaypointsCount() )
				{
					frame->AddDebugSphere( pathAgent->GetWaypoint(currentWaypointIdx + 1), 0.1f, Matrix::IDENTITY, Color::DARK_GREEN, true, true ); 	
				}
			}
		} */ 


	}
	
}
#endif