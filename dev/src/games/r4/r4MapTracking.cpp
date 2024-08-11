/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "r4MapTracking.h"

#include "../../common/engine/engineMetalinkTypes.h"
#include "../../common/engine/pathlibAreaDescription.h"
#include "../../common/engine/pathlibConst.h"
#include "../../common/engine/pathlibMetalink.h"
#include "../../common/engine/pathlibStreamingManager.h"
#include "../../common/engine/pathlibWorld.h"
#include "../../common/engine/stripeComponent.h"

#include "commonMapManager.h"

namespace
{
	static const Float WAYPOINT_UPDATE_TOLERANCE_DIST = 0.5f;
	static const Float PATH_LOOK_AHEAD_DELAY = 1.111111112f;
	static const Float PATH_LOOK_AHEAD_DISTANCE = 22.5f;
	static const Float PATH_DETAILED_PATHFOLLOWING_TEST_DELAY = 6.12123f;
	static const Float PATH_DETAILED_PATHFOLLOWING_TEST_DELAY_METALINKS = 0.8211f;
	static const Float MAX_PATH_DISTANCE = 20.f;
	static const Float COS45 = 0.70710678118654752440084436210485f;

	void LineSegmentCircle2D( const Vector2& p0, const Vector2& p1, const Vector2& c, Float r, Vector2& intersection )
	{
		Vector2 diff = p1 - p0;

		Float a = diff.X*diff.X + diff.Y*diff.Y;
		Float b = ( p0.X*diff.X + p0.Y*diff.Y - diff.X*c.X - diff.Y*c.Y ) * 2.f;
		Float d = p0.X*p0.X + p0.Y*p0.Y + c.X*c.X + c.Y*c.Y - r*r - ( p0.X*c.X + p0.Y*c.Y ) * 2.f;

		Float delta = Abs( b*b - 4.f*a*d );
		Float deltaSqrt = sqrt( delta );

		//Float a0 = (-b - deltaSqrt) / (2.f*a);
		//Float a1 = (-b + deltaSqrt) / (2.f*a);
		Float ratio = (-b + deltaSqrt) / (2.f*a);
		ASSERT( ratio >= 0.f && ratio < 1.01f );
		intersection = p0 + diff*ratio;
		

		/*if ( a0 >= 0.f || a0 <= 1.f )
		{
			intersection = p0 + diff * a0;
			return true;
		}
		else if ( a1 >= 0.f || a1 <= 1.f )
		{
			intersection = p0 + diff * a1;
			return true;
		}
		intersection = p1;
		return false;*/
	}
};

///////////////////////////////////////////////////////////////////////////////
// CR4TrackingAgent
///////////////////////////////////////////////////////////////////////////////
CR4TrackingAgent::CR4TrackingAgent( CR4MapTracking& owner, CPathLibWorld* world, Float personalSpace )
	: Super( world, personalSpace, CLASS_CR4MapTracking )
	, m_owner( owner )
{

}
CR4TrackingAgent::~CR4TrackingAgent()
{

}

void CR4TrackingAgent::OnPathCollectionSync()
{
	// post process all metalinks, so they are nicely curved and such
	for ( Uint32 metalinkIdx = 0, metalinksCount = m_metalinksStack.Size(); metalinkIdx < metalinksCount; ++metalinkIdx )
	{
		auto& metalinkData = m_metalinksStack[ metalinkIdx ];
		switch ( metalinkData.m_metalinkSetup->GetClassId() )
		{
		case PathLib::MetalinkClassId( EEngineMetalinkType::T_STRIPE ):
			{
				// get stripe
				CStripeComponent* stripe = Cast< CStripeComponent >( metalinkData.m_metalinkRuntime.GetEngineComponent( this ) );
				if ( !stripe )
				{
					break;
				}
				// compute reference wp index
				Uint32 wp = metalinkData.m_waypointIndex;

				// compute curve sampling
				const SMultiCurve& curve = stripe->RequestCurve();

				SMultiCurvePosition curvePosBegin;
				SMultiCurvePosition curvePosEnd;
				Bool curveForward;
				{
					Vector tmp;
					curve.GetClosestPointOnCurve( m_waypoints[ wp ], curvePosBegin, tmp, 0.5f );
					curve.GetClosestPointOnCurve( m_waypoints[ wp+1 ], curvePosEnd, tmp, 0.5f );
					curveForward = curvePosEnd > curvePosBegin;
				}

				// sample curve
				TStaticArray< Vector3, 64 > roadPoints;
				roadPoints.PushBack( m_waypoints[ wp ] );

				{
					SMultiCurvePosition p = curvePosBegin;
					Float placingDistance = m_owner.GetParameterPlacingDistance();
					if ( !curveForward )
					{
						placingDistance = -placingDistance;
					}
					do 
					{
						Vector computedSpot;
						Bool eop;
						curve.GetPointOnCurveInDistance( p, placingDistance, computedSpot, eop );
						if ( eop )
						{
							break;
						}
						if ( curveForward )
						{
							if ( p > curvePosEnd )
							{
								break;
							}
						}
						else
						{
							if ( p < curvePosEnd )
							{
								break;
							}
						}
						roadPoints.PushBack( computedSpot.AsVector3() );
					} while ( roadPoints.Size() < 63 );			// notice we reserve last point for destination
				}

				roadPoints.PushBack( m_waypoints[ wp+1 ] );

				Int32 numSegments = roadPoints.Size();

				//Float curveLen = curve.CalculateLength( 1 );
				//Int32 numSegments = Int32( Red::Math::MCeil( curveLen / m_owner.GetParameterPlacingDistance() ) );
				//numSegments = Clamp( numSegments, 0, 63 );

				//
				//roadPoints.Resize( numSegments+1 );

				//Float numSegmentsF = Float( numSegments );

				//Float curveTime = curve.GetTotalTime();

				//// compute curve samples
				//for ( Float i = 1.f; i < numSegmentsF; ++i )
				//{
				//	Vector position;

				//	Float alpha = i / numSegmentsF;
				//	curve.GetPosition( alpha*curveTime, position );
				//	roadPoints[ Uint32( i ) ] = position.AsVector3();
				//}
				//roadPoints[ 0 ] = m_waypoints[ wp ];
				//roadPoints[ numSegments ] = m_waypoints[ wp+1 ];

				// cut samples from front and back - to smooth out curve entrance
				Vector3 prevWP = wp > 0 ? m_waypoints[ wp-1 ] : m_waypoints[ wp ];
				Vector3 nextWP = wp+2 < m_waypoints.Size() ? m_waypoints[ wp+2 ] : m_waypoints[ wp+1 ];

				Int32 initialVert = 0;
				for ( ; initialVert < numSegments-1; ++initialVert )
				{
					Vector2 dirPrev = ( roadPoints[ initialVert ].AsVector2() - prevWP.AsVector2() ).Normalized();
					Vector2 dirNext = ( roadPoints[ initialVert+1 ].AsVector2() - roadPoints[ initialVert ].AsVector2() ).Normalized();
					
					Float dot = dirPrev.Dot( dirNext );
					if ( dot >= COS45 || !TestLine( m_world, prevWP, roadPoints[ initialVert+1 ], m_personalSpace, m_defaultCollisionFlags ) )
					{
						// loop-break
						break;
					}
				}

				Int32 finalVert = numSegments-1;
				for ( ; finalVert > initialVert; --finalVert )
				{
					Vector2 dirPrev = ( nextWP.AsVector2() - roadPoints[ finalVert ].AsVector2() ).Normalized();
					Vector2 dirNext = ( roadPoints[ finalVert ].AsVector2() - roadPoints[ finalVert-1 ].AsVector2() ).Normalized();

					Float dot = dirPrev.Dot( dirNext );
					if ( dot >= COS45 || !TestLine( m_world, prevWP, roadPoints[ finalVert-1 ], m_personalSpace, m_defaultCollisionFlags ) )
					{
						// loop-break
						break;
					}
				}

				// what if the road itself is totally ignorable (rare case)
				if ( finalVert <= initialVert )
				{
					// TODO
					if ( TestLine( m_world, prevWP, nextWP, m_personalSpace, m_defaultCollisionFlags ) )
					{

					}
					
					break;
				}

				Int32 stripePoints = finalVert - initialVert + 1;
				Int32 sizeDiff = stripePoints - 2;
				ASSERT( sizeDiff >= 0 );
				// make place for new waypoints
				if ( sizeDiff > 0 )
				{
					m_waypoints.Grow( sizeDiff );
					for ( Int32 wpCopy = m_waypoints.Size()-1; wpCopy >= Int32( wp ) + stripePoints; --wpCopy )
					{
						m_waypoints[ wpCopy ] = m_waypoints[ wpCopy - sizeDiff ];
					}

					// modify metalinks list NOTICE: we are in middle of metalinks iteration!
					metalinkData.m_position = roadPoints[ initialVert ];
					metalinkData.m_destination = roadPoints[ initialVert + 1 ];
					MetalinkInteraction currentMetalink = metalinkData;

					m_metalinksStack.Grow( sizeDiff );
					for ( Int32 mlCopy = m_metalinksStack.Size()-1; mlCopy > Int32( metalinkIdx ) + sizeDiff; --mlCopy )
					{
						m_metalinksStack[ mlCopy ] = m_metalinksStack[ mlCopy - sizeDiff ];					// move metalink in array
						m_metalinksStack[ mlCopy ].m_waypointIndex += sizeDiff;								// change waypoint indexing
					}

					for ( Int32 i = 0; i < sizeDiff; ++i )
					{
						MetalinkInteraction& newMetalink = m_metalinksStack[ metalinkIdx + i + 1 ];
						newMetalink = currentMetalink;
						newMetalink.m_waypointIndex = wp + i + 1;
						newMetalink.m_position = roadPoints[ initialVert + 1 + i ];
						newMetalink.m_destination = roadPoints[ initialVert + 2 + i ];
					}

					// update iteration!
					metalinkIdx += sizeDiff;
					metalinksCount += sizeDiff;
				}

				// NOTICE: don't use metalinkData beyond that point
				
				// now push new waypoints
				for ( Int32 i = 0; i < stripePoints; ++i )
				{
					m_waypoints[ wp + i ] = roadPoints[ initialVert + i ];
				}
			}
			break;

		default:
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// CR4MapTracking
///////////////////////////////////////////////////////////////////////////////
CR4MapTracking::SPathPointTracking::SPathPointTracking( const CR4TrackingAgent& agent )
	: m_basePosition( agent.GetPosition() )
	, m_currentDestination( agent.GetCurrentFollowPosition() )
	, m_currentWaypoint( agent.GetCurrentWaypointIdx() )
	, m_currentMetalink( agent.GetCurrentMetalinkIdx() )
{

}

CR4MapTracking::CR4MapTracking( CCommonMapManager& mapManager )
	: m_manager( mapManager )
	, m_agent( nullptr )
	, m_pathFollowing( false )
	, m_updatePathRequest( false )
	, m_hasSafeSpot( false )
	, m_pathfindingTimeout( EngineTime::ZERO )
	, m_parameterRemovalDistance( 5 )
	, m_parameterPlacingDistance( 5 )
	, m_parameterRefreshInterval( 1.f )
	, m_parameterPathfindingTolerance( 20.f )
	, m_parameterMaxCount( 5 )
	, m_mapZoom( 1.f )
	, m_closestWaypoint( -1.f )
{
	OnParametersModified();
}
CR4MapTracking::~CR4MapTracking()
{

}

Bool CR4MapTracking::GetPathPointInDistance( Float distance, SPathPointTracking& tracking, Vector3& vOut )
{
	Uint32 waypointsCount = m_agent->GetWaypointsCount();

	Vector2 basePos = tracking.m_basePosition.AsVector2();
	Vector3 currentPos = tracking.m_basePosition;
	Vector3 nextPos = tracking.m_currentDestination;
	Uint32 waypoint = tracking.m_currentWaypoint;

	Float distanceSq = distance*distance;

	do
	{
		Float waypointDistSq = ( nextPos.AsVector2() - basePos ).SquareMag();
		if ( distanceSq <= waypointDistSq )
		{
			Vector3 intersection;
			::LineSegmentCircle2D( currentPos.AsVector2(), nextPos.AsVector2(), basePos, distance, intersection.AsVector2() );
			// Z is almost ignorable
			intersection.Z = currentPos.Z;

			vOut = intersection;

			tracking.m_basePosition = vOut;
			tracking.m_currentDestination = nextPos;
			tracking.m_currentWaypoint = waypoint;

			return true;
		}
		else
		{
			if ( ++waypoint >= waypointsCount )
			{
				vOut = m_agent->GetDestination();
				tracking.m_basePosition = vOut;
				tracking.m_currentDestination = vOut;
				tracking.m_currentWaypoint = waypoint;
				return false;
			}
			currentPos = nextPos;
			nextPos = m_agent->GetWaypoint( waypoint );
		}

	} while ( true );
}

Bool CR4MapTracking::PlotPath( const Vector3& targetPos, Float targetRadius )
{
	PC_SCOPE_PIX( CR4MapTracking_PlotPath );

	const Float SAFE_SPOT_SEARCH_RADIUS  = 4.0f;
	const Float PATH_TOLERANCE  = 4.0f;

	Float minimapRadius = m_manager.GetMinimapManager()->GetCurrentMinimapRadius() + m_computedPlacingDistance;
	Float minimapRadiusSq = minimapRadius * minimapRadius;

	// try to correct destination position 
	Vector3 safeTargetPos = targetPos;
	m_agent->FindSafeSpot( targetPos, SAFE_SPOT_SEARCH_RADIUS, safeTargetPos, m_agent->GetCollisionFlags() );			// notice: we doesn't check function return

	Float tolerance = Max( targetRadius, m_parameterPathfindingTolerance );
	// configure pathfinding tolerace paramter
	if ( tolerance > 0.f )
	{
		m_agent->EnableClosestSpotPathfinding( true );
		m_agent->SetClostPathfindingDistanceLimit( tolerance );
	}
	else
	{
		m_agent->EnableClosestSpotPathfinding( false );
	}

	Float pathfindDistSq = ( m_agent->GetPosition() - safeTargetPos ).SquareMag();
	Bool forcePathfindingInTrivialCases = pathfindDistSq > minimapRadiusSq;

	m_agent->ForcePathfindingInTrivialCases( forcePathfindingInTrivialCases );

	PathLib::EPathfindResult result = m_agent->PlotPath( safeTargetPos, PATH_TOLERANCE );
	switch ( result )
	{
	case PathLib::PATHRESULT_FAILED_OUTOFNAVDATA:
	case PathLib::PATHRESULT_FAILED:
		m_pathfindingTimeout = GGame->GetEngineTime() + m_parameterRefreshInterval;
		return false;
	case PathLib::PATHRESULT_SUCCESS:
		{
			EngineTime time = GGame->GetEngineTime();
			m_pathfindingTimeout = time + m_parameterRefreshInterval;
			m_lookAheadTimeout = time + PATH_LOOK_AHEAD_DELAY;
			m_lastDetailedUpdate = time;
			m_updatePathRequest = false;
			m_pathFollowing = true;
			m_forceWaypointsUpdate = true;
			m_pathLimitedByStreaming = m_agent->IsPathLimitedBecauseOfStreamingRange();
			m_pathStreamingVersionStamp = m_agent->GetPathLib()->GetStreamingManager()->GetVersionStamp();
		}
		
		return true;
	default:
	case PathLib::PATHRESULT_PENDING:
		return true;
	}
}

#ifndef NO_EDITOR_FRAGMENTS
void CR4MapTracking::OnGenerateDebugFragments( CRenderFrame* frame )
{
	const CRenderFrameInfo & frameInfo = frame->GetFrameInfo();
	if ( frameInfo.IsShowFlagOn( SHOW_MapTracking ) )
	{
		if ( m_agent )
		{
			for ( const Vector& wp : m_waypoints )
			{
				frame->AddDebugSphere( wp, 1.f, Matrix::IDENTITY, Color::BLUE, true );
			}

			Uint32 waypoints = m_agent->GetWaypointsCount();

			if ( waypoints > 0 )
			{
				Uint32 metalinksCount = m_agent->GetMetalinksCount();
				Uint32 currMetalinkIndex = 0;

				TDynArray< DebugVertex > verts;
				TDynArray< Uint16 > indices;

				Uint32 waypoints = m_agent->GetWaypointsCount();
				Uint32 metalinks = m_agent->GetMetalinksCount();
				verts.Resize( waypoints );
				for ( Uint16 i = 0; i < waypoints; ++i )
				{
					Bool isMetalink = false;
					if ( currMetalinkIndex < metalinksCount )
					{
						if ( m_agent->GetMetalink( currMetalinkIndex ).m_waypointIndex == i )
						{
							isMetalink = true;
						}
						while ( m_agent->GetMetalink( currMetalinkIndex ).m_waypointIndex <= i )
						{
							if ( ++currMetalinkIndex == metalinksCount )
							{
								break;
							}
						}
					}
					Color vertexColor = isMetalink
						? Color::LIGHT_BLUE
						: Color::WHITE;

					Vector pos = m_agent->GetWaypoint( i ) + Vector( 0,0,0.25f );
					verts[ i ] = DebugVertex( m_agent->GetWaypoint( i ) + Vector( 0,0,0.25f ), vertexColor );

					Color wpColor = vertexColor;
					if ( i == m_agent->GetCurrentWaypointIdx() )
					{
						wpColor = Color::YELLOW;
					}

					frame->AddDebugText( 
						pos,
						String::Printf( TXT("%d"), i ),
						-1, 0, false, wpColor 
						);
				}
				indices.Resize( (waypoints-1)*2 );
				for ( Uint16 i = 0; i < waypoints-1; ++i )
				{
					indices[ i*2+0 ] = i;
					indices[ i*2+1 ] = i+1;
				}
				
				frame->AddDebugIndexedLines( verts.TypedData(), waypoints, indices.TypedData(), (waypoints-1)*2, true );
				frame->AddDebugLine( m_agent->GetPosition(), m_agent->GetCurrentFollowPosition(), Color::YELLOW, true );
			}
			else
			{
				// help debug why pathfinding failed
				frame->AddDebugText( m_targetPos, TXT("MAPPIN"), 0, 0, true, Color::RED, Color::BLACK, nullptr, true );

				String pathfindingOutcome;
				m_agent->Debug_PathfindOutcome( pathfindingOutcome );
				frame->AddDebugText( m_targetPos, pathfindingOutcome, 0, -1, true, Color::RED, Color::BLACK, nullptr, true );
			}
		}
	}
}
#endif // !NO_EDITOR_FRAGMENTS


///////////////////////////////////////////////////////////////////////////////
// Main tracking update
void CR4MapTracking::UpdateMetalinkAwareness()
{
	PC_SCOPE_PIX( CR4MapTracking_UpdateMetalinkAwareness );

	m_agent->UpdateMetalinkAwareness();

	Vector3 origin;
	Vector3 destination;
	if ( m_agent->GetCurrentMetalinkWaypoints( origin, destination ) ) 
	{
		const Vector3& position = m_agent->GetPosition();
		Vector3 seg = destination - origin;
		Vector3 diff = destination - position;
		if ( seg.Dot( diff ) < 0 )
		{
			m_agent->NextMetalink();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Main tracking update
void CR4MapTracking::UpdateTracking( const Vector& startPos, const Vector& targetPos, Float targetRadius )
{
	PC_SCOPE_PIX( CR4MapTracking_UpdateTracking );

	const Float SAFE_SPOT_SEARCH_RADIUS  = 4.0f;
	Vector3 safeStartPos = startPos;

	m_startPos = startPos;

	Bool targetUpdated = false;
	if ( (m_targetPos - targetPos.AsVector3()).SquareMag() > WAYPOINT_UPDATE_TOLERANCE_DIST*WAYPOINT_UPDATE_TOLERANCE_DIST )
	{
		targetUpdated = true;
		m_forceWaypointsUpdate = true;
		m_targetPos = targetPos;
	}

	// fix up base spot
	if ( m_agent->FindSafeSpot( startPos, SAFE_SPOT_SEARCH_RADIUS, safeStartPos, m_agent->GetCollisionFlags() ) )
	{
		m_lastSafeSpot = safeStartPos;
		m_hasSafeSpot = true;
	}
	else
	{
		if ( !m_hasSafeSpot )
		{
			return;
		}
		safeStartPos = m_lastSafeSpot;
	}

	// map path agent to safe spot
	Bool hasMoved = !( safeStartPos.AsVector2() - m_agent->GetPosition().AsVector2() ).IsAlmostZero() || Abs( safeStartPos.Z - m_agent->GetPosition().Z ) > 0.5f;
	if ( hasMoved )
	{
		m_agent->MoveToPosition( safeStartPos );
	}

	// update waypoints & pathfollowing
	if ( m_pathFollowing )
	{
		Float minimapRadius = m_manager.GetMinimapManager()->GetCurrentMinimapRadius() + m_computedPlacingDistance;

		if ( !targetUpdated || m_agent->UpdatePathDestination( m_targetPos, ( safeStartPos - m_targetPos ).SquareMag() < minimapRadius * minimapRadius ) )
		{
			PC_SCOPE_PIX( CR4MapTracking_FollowPath );
			// test if we can keep following current path
			Vector3 currentFollowPosition;
			if ( m_agent->HasPath() && m_agent->FollowPath( currentFollowPosition ) )
			{
				// metalink awareness update
				UpdateMetalinkAwareness();
				
				// compute waypoints
				UpdateWaypoints();

				// Check if path is still valid. If not we will ask for its recomputation.
				// NOTICE: we are not breaking tracking moment we got path outdated, as that way everything is more smooth with less poping and dissapearing stuff
				if ( !m_updatePathRequest )
				{
					EngineTime time = GGame->GetEngineTime();
					if ( m_lookAheadTimeout < time )
					{
						PC_SCOPE_PIX( CR4MapTracking_LookAheadForBetterWaypoint );
						// verify if path is still cool
						m_lookAheadTimeout = time + PATH_LOOK_AHEAD_DELAY;
						m_agent->LookAheadForBetterWaypoint( PATH_LOOK_AHEAD_DISTANCE );
					}
					if ( m_lastDetailedUpdate + PATH_DETAILED_PATHFOLLOWING_TEST_DELAY < time )
						//m_agent->IsPathfollowingOnMetalink()
						//? m_lastDetailedUpdate + PATH_DETAILED_PATHFOLLOWING_TEST_DELAY_METALINKS < time
						//: m_lastDetailedUpdate + PATH_DETAILED_PATHFOLLOWING_TEST_DELAY < time )
					{
						PC_SCOPE_PIX( CR4MapTracking_UpdateFollowingBasedOnClosestPositionOnPath );
						m_lastDetailedUpdate = time;
						Vector3 closestPathSpot;
						Float closestSpotSq = m_agent->UpdateFollowingBasedOnClosestPositionOnPath( closestPathSpot );
						if ( closestSpotSq > MAX_PATH_DISTANCE*MAX_PATH_DISTANCE )
						{
							m_updatePathRequest = true;
						}
					}
				}
			}
			else
			{
				// request new query
				m_updatePathRequest = true;
			}
		}
		else
		{
			// request new query
			m_updatePathRequest = true;
		}
	}

	// update pathfinding
	if ( ( m_updatePathRequest || !m_pathFollowing || ( m_pathLimitedByStreaming && m_pathStreamingVersionStamp != m_agent->GetPathLib()->GetStreamingManager()->GetVersionStamp() ) )
		&& m_pathfindingTimeout < GGame->GetEngineTime() )
	{
		if ( !PlotPath( targetPos.AsVector3(), targetRadius ) )
		{
			PC_SCOPE_PIX( CR4MapTracking_ClearTracking );
			// couldn't find path
			ClearTracking();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Update waypoints list
void CR4MapTracking::UpdateWaypoints()
{
	PC_SCOPE_PIX( CR4MapTracking_UpdateWaypoints );

	Bool dirty = false;

	Float minimapRadius = m_manager.GetMinimapManager()->GetCurrentMinimapRadius() + m_computedPlacingDistance;
	Float minimapRadiusSq = minimapRadius * minimapRadius;

	Float distance = m_closestWaypoint;
	if ( m_closestWaypoint < m_computedRemovalDistance )
	{
		distance = m_computedRemovalDistance + 0.5f * m_computedPlacingDistance;
	}

	SPathPointTracking tracking( *m_agent );
	Vector3 basePos = m_agent->GetPosition();

	// get on path
	Uint32 processedWaypoint = 0;
	while ( !m_waypoints.Empty() )
	{
		Float wpDistance = (m_waypoints[ 0 ].AsVector3() - basePos).Mag();
		if( wpDistance < m_computedRemovalDistance )
		{
			// N-complexity. We assume waypoints counter is relatively small
			m_waypoints.RemoveAt( 0 );
			dirty = true;
			m_forceWaypointsUpdate = true;
			continue;
		}
		else if ( wpDistance > m_computedRemovalDistance + m_computedPlacingDistance )
		{
			Float firstWPDistance = wpDistance - m_computedPlacingDistance;
			// rare case when distance grown 'too much'. We could support it, but its too edge
			if ( firstWPDistance > m_computedRemovalDistance + m_computedPlacingDistance )
			{
				m_waypoints.ClearFast();
				break;
			}
			Vector3 waypointsSpot;
			GetPathPointInDistance( firstWPDistance, tracking, waypointsSpot );

			dirty = true;

			if ( m_waypoints.Size() >= m_parameterMaxCount )
			{
				m_waypoints.PopBack();
			}
			
			m_waypoints.Insert( 0, waypointsSpot );

			m_closestWaypoint = m_computedPlacingDistance;

			processedWaypoint = 1;

			distance = m_computedPlacingDistance;
			break;
		}

		distance = wpDistance;

		break;
	}
	

	for ( ; processedWaypoint < m_parameterMaxCount; ++processedWaypoint )
	{
		Vector3 waypointsSpot;
		Bool notLastSpot = GetPathPointInDistance( distance, tracking, waypointsSpot );

		distance = m_computedPlacingDistance;

		// check if we should stop tracking at this point
		Bool breakTracking = false;
		// Check if assumed waypoint position outside minimap
		if ( ( waypointsSpot.AsVector2() - m_startPos.AsVector2() ).SquareMag() > minimapRadiusSq )
		{
			breakTracking = true;
		}

		// Check if we thats the last spot, and that last spot is too close
		if ( !notLastSpot )
		{
			breakTracking = true;	
		}

		if ( breakTracking )
		{
			// we are not interested in waypoints outside radius
			if ( processedWaypoint < m_waypoints.Size() - 1 )
			{
				m_waypoints.ResizeFast( processedWaypoint );
				dirty = true;
			}
			break;
		}

		// compare with 

		if ( processedWaypoint == m_waypoints.Size() )
		{
			m_waypoints.PushBack( waypointsSpot );
			ASSERT( m_waypoints.Size() <= m_parameterMaxCount );
			dirty = true;
		}
		else
		{
			if ( ( m_waypoints[ processedWaypoint ].AsVector2() - waypointsSpot.AsVector2() ).SquareMag() < WAYPOINT_UPDATE_TOLERANCE_DIST*WAYPOINT_UPDATE_TOLERANCE_DIST )
			{
				// dont update wp
				if ( !m_forceWaypointsUpdate )
				{
					break;
				}
			}
			else
			{
				m_waypoints[ processedWaypoint ] = waypointsSpot;
				dirty = true;
			}
		}

		if( !notLastSpot )
		{
			if( m_waypoints.Size() > processedWaypoint+1 )
			{
				m_waypoints.ResizeFast( processedWaypoint+1 );
				dirty = true;
			}
		}
	}

	m_forceWaypointsUpdate = false;
	if ( dirty )
	{
		CMinimapManager* minimap = m_manager.GetMinimapManager();
		minimap->AddWaypoints( m_waypoints );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Clears tracking.
void CR4MapTracking::ClearTracking()
{
	if ( !m_waypoints.Empty() )
	{
		m_waypoints.ClearFast();
		CMinimapManager* minimap = m_manager.GetMinimapManager();
		minimap->DeleteWaypoints();
	}
	
	m_agent->StopMovement();
	m_pathFollowing = false;
}

///////////////////////////////////////////////////////////////////////////////
// Initialize system
void CR4MapTracking::CreateSearchData()
{
	DestroySearchData();

	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return;
	}
	CPathLibWorld* pathlibWorld = world->GetPathLibWorld();
	if ( !pathlibWorld )
	{
		return;
	}

	m_agent = new CR4TrackingAgent( *this, pathlibWorld, 0.4f );
	m_agent->SupportRoads( true );
	m_agent->EnableHeavyPathOptimization( true );
	m_agent->RemoveForbiddenPathfindFlag( PathLib::NF_PLAYER_ONLY_PORTAL );
	m_agent->AddRef();

	m_pathfindingTimeout = 0.f;
	m_waypoints.ClearFast();
}

///////////////////////////////////////////////////////////////////////////////
// Shuts down system
void CR4MapTracking::DestroySearchData()
{
	if ( m_agent )
	{
		m_agent->Release();
		m_agent = nullptr;
	}
}

///////////////////////////////////////////////////////////////////////////////
// External system configuration
void CR4MapTracking::SetParameters( Float removalDistance, Float placingDistance, Float refreshInterval, Float pathfindTolerance, Uint32 maxCount )
{
	m_parameterRemovalDistance = removalDistance;
	m_parameterPlacingDistance = placingDistance;
	m_parameterRefreshInterval = refreshInterval;
	m_parameterPathfindingTolerance = pathfindTolerance;
	m_parameterMaxCount = maxCount;

	OnParametersModified();
}

void CR4MapTracking::OnChangedMapZoom( Float zoom )
{
	m_mapZoom = zoom;
	m_forceWaypointsUpdate = true;

	OnParametersModified();
}

void CR4MapTracking::OnParametersModified()
{
	m_computedRemovalDistance = m_parameterRemovalDistance / m_mapZoom;
	m_computedPlacingDistance = m_parameterPlacingDistance / m_mapZoom;
}