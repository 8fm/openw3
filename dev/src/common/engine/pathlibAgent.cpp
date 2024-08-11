#include "build.h"
#include "pathlibAgent.h"

#include "game.h"
#include "pathlibWorld.h"
#include "pathlibAreaDescription.h"
#include "pathlibMetalink.h"
#include "pathlibNavgraph.h"
#include "pathlibNavmeshArea.h"
#include "pathlibSearchEngine.h"
#include "pathlibTaskManager.h"
#include "pathlibTaskPlotPath.h"
#include "pathlibUtils.h"

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// CAgent
////////////////////////////////////////////////////////////////////////////
namespace
{
	static const Float STOP_DISTANCE = 0.1f;
	static const Float FOLLOW_UPDATE_FREQUENCY( 0.333f );
	static const Float FOLLOW_UPDATE_DIST_RATIO_SQ( 0.25f * 0.25f );
};

CAgent::CAgent( CPathLibWorld* world, Float personalSpace, ClassId classId )
	: CSearchData( personalSpace, classId | CLASS_CAgent )
	, m_position( 0, 0, 0 )
	, m_currentWaypoint( 0 )
	, m_currentMetalink( 0 )
	, m_metalinkIsRunning( false )
	, m_pathIsLimitedBecauseOfStreamingRange( false )
	, m_lastFollowPosition( 0, 0, 0 )
	, m_nextFollowUpdate()
	, m_state( STATE_IDLE )
	, m_isOnNavdata( true )
	, m_pathfollowFlags( 0 )
	, m_approachDistance( 0.5f )
	, m_targetTolerance( 0.f )
	, m_world( world )
{
	Initialize( world, personalSpace );
}

void CAgent::OnPathCollectionSync()
{

}

RED_INLINE CAreaDescription* CAgent::UpdateCurrentArea()
{
	if ( m_currentArea != INVALID_AREA_ID )
	{
		CAreaDescription* area = m_world->GetAreaDescription( m_currentArea );
		if ( area && area->IsReady() )
		{
			if ( area->VContainsPoint( m_position ) )
			{
				return area;
			}
		}
	}

	CAreaDescription* area = m_world->GetAreaAtPosition( m_position );
	if ( area && area->IsReady() )
	{
		m_currentArea = area->GetId();
		return area;
	}
	m_currentArea = INVALID_AREA_ID;
	return nullptr;
}

void CAgent::OnPathfindResult( PathLib::EPathfindResult result )
{
	switch ( result )
	{
	case PATHRESULT_FAILED_OUTOFNAVDATA:
		m_isOnNavdata = false;
		// no break
	case PATHRESULT_FAILED:
		m_state = STATE_IDLE;
		break;

	case PATHRESULT_SUCCESS:
		{
			ClearPath();
			m_state = STATE_PATHFOLLOW;
			m_currentWaypoint = 0;
			m_currentMetalink = 0;
			
			m_targetTolerance = m_searchTolerance;
			// copy waypoints data
			m_waypoints.ResizeFast( m_outputWaypoints.Size() );
			Red::System::MemoryCopy( m_waypoints.Data(), m_outputWaypoints.Data(), m_outputWaypoints.DataSize() );
			m_outputWaypoints.ClearFast();
			m_metalinksStack.ResizeFast( m_outputMetalinksStack.Size() );
			if ( !m_outputMetalinksStack.Empty() )
			{
				for ( Uint32 i = 0, n = m_outputMetalinksStack.Size(); i != n; ++i )
				{
					m_metalinksStack[ i ] = Move( m_outputMetalinksStack[ i ] );
				}

				m_outputMetalinksStack.ClearFast();
			}
			Uint16 proceduralPathfollowFlags = 0;

			if ( m_outputPathUseTolerance )
			{
				proceduralPathfollowFlags |= PATHFOLLOW_MOVING_WITH_TOLERANCE;
			}

			m_pathfollowFlags = ( m_pathfollowFlags & (~PATHFOLLOW_PATHFINDING_FLAGS) ) | proceduralPathfollowFlags;
			m_pathIsLimitedBecauseOfStreamingRange = m_outputPathDestinationWasOutsideOfStreamingRange;

			OnPathCollectionSync();
		}
		
		break;

	case PATHRESULT_PENDING:
		m_state = STATE_PATHFIND_PENDING;
		break;

	default:
		ASSERT( false, TXT( "Unsupported!" ) );
		ASSUME( false );
	}
}

Bool CAgent::GetCurrentMetalinkWaypoints( Vector3& outOrigin, Vector3& outDestination ) const
{
	if ( Uint32( m_currentMetalink ) < m_metalinksStack.Size() && m_metalinkIsRunning )
	{
		const MetalinkInteraction& interaction = m_metalinksStack[ m_currentMetalink ];
		outOrigin = interaction.m_position;
		outDestination = interaction.m_destination;
		return true;
	}
	return false;
}

const CAgent::MetalinkInteraction* CAgent::GetCurrentMetalinkInterraction() const
{
	if ( Uint32( m_currentMetalink ) < m_metalinksStack.Size() && m_metalinkIsRunning )
	{
		return &m_metalinksStack[ m_currentMetalink ];
	}
	return nullptr;
}

void CAgent::ClearPath()
{
	if ( m_metalinkIsRunning ) 
	{
		if ( Uint32( m_currentMetalink ) < m_metalinksStack.Size() )
		{
			MetalinkInteraction& interaction = m_metalinksStack[ m_currentMetalink ];
			interaction.m_metalinkSetup->AgentPathfollowOver( interaction.m_metalinkRuntime, this, interaction.m_position, interaction.m_destination );
		}
		m_metalinkIsRunning = false;
	}
	m_waypoints.ClearFast();
	m_metalinksStack.ClearFast();
	m_asyncTaskInvalidated = true;
}

PathLib::EPathfindResult CAgent::PlotPath( const Vector3& destinationPos, Float tolerance )
{
	//if ( !m_isOnNavdata )
	//{
	//	return PATHRESULT_FAILED;
	//}
	PathLib::EPathfindResult result = CSearchData::PlotPath( m_world, m_position, destinationPos, tolerance );
	OnPathfindResult( result );
	return result;
}

Bool CAgent::NextMetalink()
{
	if ( m_currentMetalink < Int32( m_metalinksStack.Size() ) )
	{
		if ( m_metalinkIsRunning )
		{
			MetalinkInteraction& interaction = m_metalinksStack[ m_currentMetalink ];
			if ( !interaction.m_metalinkSetup->AgentPathfollowOver( interaction.m_metalinkRuntime, this, interaction.m_position, interaction.m_destination ) )
			{
				return false;
			}
		}
		m_metalinkIsRunning = false;
		++m_currentMetalink;
		m_nextFollowUpdate = EngineTime::ZERO;
	}
	return true;
}

Bool CAgent::UpdateMetalinkAwareness()
{
	if ( !m_metalinksStack.Empty() )
	{
		Int32 metalinksStackSize = m_metalinksStack.Size();
		if ( m_currentMetalink >= metalinksStackSize )
		{
			return false;
		}

		while ( m_metalinksStack[ m_currentMetalink ].m_waypointIndex+1 < m_currentWaypoint )
		{
			MetalinkInteraction& interaction = m_metalinksStack[ m_currentMetalink ];
			if ( !interaction.m_metalinkSetup->AgentPathfollowOver( interaction.m_metalinkRuntime, this, interaction.m_position, interaction.m_destination ) )
			{
				return false;
			}
			m_metalinkIsRunning = false;
			if ( ++m_currentMetalink >= metalinksStackSize )
			{
				return false;
			}
		}

		MetalinkInteraction& interaction = m_metalinksStack[ m_currentMetalink ];
		if ( m_currentWaypoint >= interaction.m_waypointIndex && m_currentWaypoint <= interaction.m_waypointIndex+1 )
		{
			m_metalinkIsRunning = true;

			if ( !interaction.m_metalinkSetup->AgentPathfollowUpdate( interaction.m_metalinkRuntime, this, interaction.m_position, interaction.m_destination ) )
			{
				return true;
			}
		}

		
		
	}
	return false;
}

Bool CAgent::FollowPath( Vector3& outFollowPosition )
{
	ASSERT( m_state == STATE_PATHFOLLOW || m_state == STATE_PATHFIND_PENDING );
	{
		//{
		//	const Vector3& destination = GetDestination();
		//	// check success
		//	if ( ((m_position.AsVector2() - destination.AsVector2()).SquareMag() <= m_approachDistance*m_approachDistance)		// distance test
		//		&& (((m_pathfollowFlags & PATHFOLLOW_LINE_TEST) == 0) || TestLine( m_position, destination, 0.f ))				// line test
		//		)
		//	{
		//		//OnMovementComplete();
		//		return false;
		//	}
		//}
		
		EngineTime engineTime = GGame->GetEngineTime();
		if ( engineTime >= m_nextFollowUpdate || (m_position - m_lastFollowUpdateOriginPosition).SquareMag() > m_nextFollowUpdateDistanceSq )
		{
			if ( !UpdateFollowPoint( m_lastFollowPosition ) )
			{
				//OnMovementBlocked();
				return false;
			}
			m_nextFollowUpdate = engineTime + FOLLOW_UPDATE_FREQUENCY;
			m_lastFollowUpdateOriginPosition = m_position;
			m_nextFollowUpdateDistanceSq = ( m_lastFollowPosition - m_lastFollowUpdateOriginPosition ).SquareMag() * FOLLOW_UPDATE_DIST_RATIO_SQ;
		}
	}
	if ( m_state == STATE_PATHFOLLOW || m_state == STATE_PATHFIND_PENDING )
	{
		outFollowPosition = m_lastFollowPosition;
		return true;
	}
	else
	{
		return false;
	}
}
void CAgent::LookAheadForBetterWaypoint( Float distance )
{
	Float distanceSq = distance*distance;
	Uint32 wp = m_currentWaypoint + 2;
	Bool boundingOptimizable;
	
	Uint32 wpCount = GetOptimizableWaypointsBounding( boundingOptimizable );
	while ( wp < wpCount )
	{
		const Vector3& wpPos = m_waypoints[ wp ];
		if ( TestLine( m_world, m_position, wpPos, m_personalSpace, m_defaultCollisionFlags ) )
		{
			m_currentWaypoint = wp;
		}
		if ( ( wpPos - m_position ).SquareMag() > distanceSq )
		{
			break;
		}
		++wp;
	}
}

Float CAgent::UpdateFollowingBasedOnClosestPositionOnPath( Vector3& outClosestPosition )
{
	// find closest wp
	Float closestDistSq = FLT_MAX;
	const Vector3& referencePos = m_position;
	Uint32 closestWP = m_currentWaypoint;
	for ( Uint32 wp = 1, count = m_waypoints.Size(); wp < count; ++wp )
	{
		Float ratio;
		Vector3 spot;
		const Vector3& wp0 = m_waypoints[ wp-1 ];
		const Vector3& wp1 = m_waypoints[ wp ];
		MathUtils::GeometryUtils::TestClosestPointOnLine2D( referencePos.AsVector2(), wp0.AsVector2(), wp1.AsVector2(), ratio, spot.AsVector2() );
		spot.Z = wp0.Z + (wp1.Z - wp0.Z) * ratio;
		Float distSq = (spot - referencePos).SquareMag();
		if ( distSq < closestDistSq )
		{
			closestDistSq = distSq;
			closestWP = wp-1;
			outClosestPosition = spot;
		}
	}

	// pathfollowing wp update
	if ( closestWP != m_currentWaypoint )
	{
		m_currentWaypoint = closestWP;
		m_nextFollowUpdate = EngineTime::ZERO;

		// metalinks update
		if ( !m_metalinksStack.Empty() )
		{
			struct Predicate
			{
				Bool operator()( const MetalinkInteraction& m, Uint32 wp ) const
				{
					return m.m_waypointIndex < wp;
				}
			} pred;
			auto it = ::LowerBound( m_metalinksStack.Begin(), m_metalinksStack.End(), m_currentWaypoint, pred );
			Uint32 newMetalinkIdx = Uint32( it - m_metalinksStack.Begin() );
			if ( newMetalinkIdx != m_currentMetalink )
			{
				if ( m_metalinkIsRunning )
				{
					MetalinkInteraction& interaction = m_metalinksStack[ m_currentMetalink ];
					if ( !interaction.m_metalinkSetup->AgentPathfollowOver( interaction.m_metalinkRuntime, this, interaction.m_position, interaction.m_destination ) )
					{
						return false;
					}
					m_metalinkIsRunning = false;
				}

				m_currentMetalink = newMetalinkIdx;
			}
		}
	}
	

	return closestDistSq;
}

Bool CAgent::IsPathfollowingOnMetalink()
{
	if ( m_currentMetalink < Int32( m_metalinksStack.Size() ) )
	{
		MetalinkInteraction& interaction = m_metalinksStack[ m_currentMetalink ];
		if ( m_currentWaypoint >= interaction.m_waypointIndex && m_currentWaypoint <= interaction.m_waypointIndex+1 )
		{
			return true;
		}
	}
	return false;
}

Bool CAgent::UpdatePathfollowFromVirtualPosition( const Vector3& virtualPosition, Float acceptWaypointDistance )
{
	Float acceptWaypointDistanceSq = acceptWaypointDistance*acceptWaypointDistance;

	Uint32 currentWaypoint = m_currentWaypoint;
	Bool waypointSimplified = false;
	Bool triedDestinationSpot = false;
	do
	{
		const Vector3& destination = GetWaypoint( currentWaypoint );
		if ( (destination.AsVector2() - virtualPosition.AsVector2()).SquareMag() <= acceptWaypointDistanceSq )
		{
			waypointSimplified = true;
		}
		else if ( triedDestinationSpot )
		{
			break;
		}
		triedDestinationSpot = true;

		if ( currentWaypoint + 1 >= m_waypoints.Size() )
		{
			break;
		}
		currentWaypoint = currentWaypoint + 1;

	}
	while ( true );

	if ( waypointSimplified )
	{
		m_currentWaypoint = currentWaypoint;
	}

	return true;
}

Bool CAgent::UpdatePathDestination( const Vector3& newDestination, Bool testForTrivialCase )
{
	if ( m_state == STATE_PATHFOLLOW )
	{
		if ( testForTrivialCase && TestLine( m_world, m_position, newDestination, m_personalSpace, CT_DEFAULT ) )
		{
			ClearPath();
			m_currentWaypoint = 0;
			m_currentMetalink = 0;
			m_waypoints.ResizeFast( 2 );
			m_waypoints[ 0 ] = m_position;
			m_waypoints[ 1 ] = newDestination;
			m_nextFollowUpdate = EngineTime::ZERO;
			return true;
		}
		if ( m_pathfollowFlags & PATHFOLLOW_MOVING_WITH_TOLERANCE )
		{
			if ( ( newDestination - m_waypoints.Last() ).SquareMag() < m_targetTolerance*m_targetTolerance )
			{
				return true;
			}
		}
		else
		{
			if ( m_waypoints.Size() >= 2 )
			{
				const Vector3& lastPoint = m_waypoints[ m_waypoints.Size() - 2 ];
				if ( TestLine( lastPoint, newDestination ) )
				{
					m_waypoints[ m_waypoints.Size() - 1 ] = newDestination;
					m_nextFollowUpdate = EngineTime::ZERO;
					return true;
				}
			}
		}
	}
	return false;
}

void CAgent::SetupPathfollowing( Float tolerateDistance, Uint16 pathfollowFlags )
{
	ASSERT( m_waypoints.Size() > 0 );
	m_approachDistance = tolerateDistance;
	m_pathfollowFlags = (m_pathfollowFlags & (~PATHFOLLOW_USER_FLAGS)) | pathfollowFlags;
}
void CAgent::DynamicEnablePathfollowingFlags( Uint32 pathfollowFlags )
{
	m_pathfollowFlags |= pathfollowFlags;
}
void CAgent::DynamicDisablePathfollowingFlags( Uint32 pathfollowFlags )
{
	m_pathfollowFlags &= ~pathfollowFlags;
}
void CAgent::StopMovement()
{
	ClearPath();
	m_currentWaypoint = 0;
	m_currentMetalink = 0;
	m_state = STATE_IDLE;
}
void CAgent::MoveToPosition( const Vector3& newPosition )
{
	m_position = newPosition;
}

Bool CAgent::StayOnNavdataRec( Vector3& inOutDeltaMovement, Int32 recursionLimit )
{
	Vector3 deltaMovement = inOutDeltaMovement;
	Vector3 destination = m_position + deltaMovement;

	Vector3 geometryPoint;
	Vector3 basePoint = destination;
	Float obstacle2DDistance = m_world->GetClosestObstacle( m_currentArea, destination, m_personalSpace, geometryPoint, m_defaultCollisionFlags | CT_NO_ENDPOINT_TEST );
	if ( obstacle2DDistance >= m_personalSpace )
	{
		// final position is ok but line test fails
		obstacle2DDistance = m_world->GetClosestObstacle( m_currentArea, m_position, destination, m_personalSpace, geometryPoint, basePoint, m_defaultCollisionFlags | CT_NO_ENDPOINT_TEST );
		if ( obstacle2DDistance >= m_personalSpace )
		{
			// its actually possible
			return true;
		}
	}
	if ( obstacle2DDistance < NumericLimits< Float >::Epsilon() )
	{
		inOutDeltaMovement.AsVector2().Set( 0.f, 0.f );
		return false;
	}
	Float prevLenSq = deltaMovement.AsVector2().SquareMag();
	Vector2 diff = (basePoint.AsVector2() - geometryPoint.AsVector2()) * ((m_personalSpace + 0.01f) / obstacle2DDistance);
	Vector2 desiredDestination = geometryPoint.AsVector2() + diff;
	inOutDeltaMovement.AsVector2() = desiredDestination - m_position.AsVector2();
	Float desiredLenSq = inOutDeltaMovement.AsVector2().SquareMag();
	if ( desiredLenSq > prevLenSq )
	{
		// decrease new delta movement length to match previous one
		inOutDeltaMovement.AsVector2() *= sqrt(prevLenSq / desiredLenSq);
	}
	else if ( desiredLenSq < 0.02f*0.02f )
	{
		inOutDeltaMovement.AsVector2().Set( 0.f, 0.f );
		return false;
	}
	destination.AsVector2() = m_position.AsVector2() + inOutDeltaMovement.AsVector2();

	if ( --recursionLimit == 0 )
	{
		if ( !m_world->TestLine( m_currentArea, m_position, destination, m_personalSpace, m_defaultCollisionFlags | CT_NO_ENDPOINT_TEST ) )
		{
			inOutDeltaMovement.AsVector2().Set( 0.f, 0.f );
			return false;
		}
		return true;
	}
	return StayOnNavdataRec( inOutDeltaMovement, recursionLimit );
}

Bool CAgent::StayOnNavdata( Vector3& inOutDeltaMovement )
{
	if ( !m_isOnNavdata )
	{
		if ( TestLocation( m_world, m_position, m_personalSpace, CT_FORCE_BASEPOS_ZTEST | CT_IGNORE_METAOBSTACLE ) )
		{
			m_isOnNavdata = true;
		}
	}
	if ( m_isOnNavdata )
	{
		Vector3 destination = m_position + inOutDeltaMovement;

		if ( inOutDeltaMovement.IsAlmostZero( 0.015f ) )
		{
			if ( !m_world->TestLocation( m_currentArea, destination, m_personalSpace, m_defaultCollisionFlags | CT_IGNORE_METAOBSTACLE ) )
			{
				inOutDeltaMovement.AsVector2().Set( 0.f, 0.f );
				return false;
			}
		}
		else if ( !m_world->TestLine( m_currentArea, m_position, destination, m_personalSpace + 0.001f, m_defaultCollisionFlags | CT_NO_ENDPOINT_TEST ) )
		{
			if ( !StayOnNavdataRec( inOutDeltaMovement ) )
			{
				if ( !m_world->TestLocation( m_currentArea, m_position, m_personalSpace, m_defaultCollisionFlags | CT_FORCE_BASEPOS_ZTEST | CT_IGNORE_METAOBSTACLE ) )
				{
					m_isOnNavdata = false;
				}
				return false;
			}
				
			destination = m_position + inOutDeltaMovement;
			
			if ( !m_world->TestLocation( m_currentArea, destination, m_personalSpace, m_defaultCollisionFlags | CT_IGNORE_METAOBSTACLE ) )
			{
				inOutDeltaMovement.AsVector2().Set( 0.f, 0.f );
				return false;
			}
			else
			{
				ASSERT( TestLine( m_world, m_position, m_position + Vector3( inOutDeltaMovement.X, inOutDeltaMovement.Y, 0.f ), m_personalSpace, CT_IGNORE_METAOBSTACLE ), TXT("Inconsistency in stay-on-navdata.") );
			}
		}
		
	}
	
	return true;
}

Bool CAgent::ReturnToNavdataRec( Vector3& outPosition, Int32 recursionLimit )
{
	Vector3 geometryPos;
	Float obstacleDistance = m_world->GetClosestObstacle( m_currentArea, outPosition, m_personalSpace, geometryPos, m_defaultCollisionFlags );
	if ( obstacleDistance > m_personalSpace )
	{
		return true;
	}
	if ( obstacleDistance < NumericLimits< Float >::Epsilon() )
	{
		return false;
	}
	if ( recursionLimit == 0 )
	{
		return false;
	}
	Vector2 correction = ( outPosition.AsVector2() - geometryPos.AsVector2() ) * ( (m_personalSpace + 0.01f) / obstacleDistance );
	outPosition.AsVector2() += correction;

	 return ReturnToNavdataRec( outPosition, recursionLimit-1 );
}

Uint32 CAgent::GetOptimizableWaypointsBounding( Bool& isBoundingOptimizable )
{
	if ( Int32( m_metalinksStack.Size() ) > m_currentMetalink )
	{
		const auto& metalinkData = m_metalinksStack[ m_currentMetalink ];
		if ( metalinkData.m_waypointIndex+1 < m_waypoints.Size() )
		{
			isBoundingOptimizable =
				metalinkData.m_waypointIndex+2 < m_waypoints.Size() && (metalinkData.m_metalinkFlags & IMetalinkSetup::METALINK_PATHFOLLOW_ALLOW_PATHOPTIMIZATION) != 0;
			return metalinkData.m_waypointIndex+1;
		}
	}
	isBoundingOptimizable = false;
	return m_waypoints.Size();
}

Bool CAgent::ReturnToNavdata( Vector3& outPosition )
{
	outPosition = m_position;

	return ReturnToNavdataRec( outPosition );
}

Bool CAgent::ComputeHeightBelow( const Vector3& pos, Float& outHeight )
{
	struct Functor : public CInstanceMap::CInstanceFunctor
	{
		Functor( const Vector3& pos, Float& outHeight )
			: m_position( pos )
			, m_outHeight( outHeight ) {}
		Bool Handle( CNavmeshAreaDescription* naviArea ) override
		{
			return naviArea->VComputeHeight( m_position.AsVector2(), -FLT_MAX, m_position.Z + GEOMETRY_AND_NAVMESH_MAX_DISTANCE, m_outHeight );
		}
		const Vector3&	m_position;
		Float&			m_outHeight;	
	} fun( pos, outHeight );
	return m_world->GetInstanceMap()->IterateAreasAt( pos, &fun );
}

Bool CAgent::GetPathPointInDistance( Float distance, Vector3& vOut, Bool computePreciseZ )
{
	Vector3 currentPos = GetPosition();
	Vector3 nextPos = m_lastFollowPosition;
	Uint32 waypoint = m_currentWaypoint;
	do
	{
		Float waypointDist = ( nextPos.AsVector2() - currentPos.AsVector2() ).Mag();
		if ( distance <= waypointDist )
		{
			vOut = currentPos + ( (nextPos) - currentPos ) * ( distance / waypointDist );
			if ( computePreciseZ )
			{
				if ( !m_world->ComputeHeightFrom( m_currentArea, vOut.AsVector2(), currentPos, vOut.Z ) )
				{
					m_world->ComputeHeight( m_currentArea, vOut, vOut.Z );
				}
			}
			return true;
		}
		else
		{
			if ( ++waypoint == m_waypoints.Size() )
			{
				vOut = m_waypoints.Back();
				return false;
			}
			distance -= waypointDist;
			currentPos = nextPos;
			nextPos = m_waypoints[ waypoint ];
		}

	} while ( true );

}

Bool CAgent::IsPointOnPath( const Vector3& point, Float toleranceRadius )
{
	Vector3 currentPos = GetPosition();
	Vector3 nextPos = m_lastFollowPosition;
	Uint32 nWaypoint = m_currentWaypoint;
	do
	{
		Float zMin = Min( nextPos.Z, currentPos.Z ) - 2.f;				// TODO: Determine tolerance
		Float zMax = Max( nextPos.Z, currentPos.Z ) + 2.f;
		if ( point.Z > zMin && point.Z < zMax &&  MathUtils::GeometryUtils::TestIntersectionCircleLine2D( point.AsVector2(), toleranceRadius, nextPos.AsVector2(), currentPos.AsVector2() ) )
		{
			return true;
		}
		if ( ++nWaypoint == m_waypoints.Size() )
		{
			return false;
		}
		currentPos = nextPos;
		nextPos = m_waypoints[ nWaypoint ];

	} while ( true );
}

Bool CAgent::CheckIfIsOnNavdata()
{
	// TODO: CT_IGNORE_METAOBSTACLE is a hack for scene ending
	m_isOnNavdata = TestLocation( m_world, m_position, m_personalSpace, CT_FORCE_BASEPOS_ZTEST | CT_IGNORE_METAOBSTACLE );
	return m_isOnNavdata;
}

void CAgent::UseRoad( CNode* road )
{
	if ( ( m_pathfollowFlags & PATHFOLLOW_USE_ROAD ) != 0 )
	{
		ASSERT( m_road.Get() == road );
		return;
	}
	m_road = road;
	m_pathfollowFlags |= PATHFOLLOW_USE_ROAD;
}
void CAgent::DontUseRoad()
{
	m_pathfollowFlags &= ~PATHFOLLOW_USE_ROAD;
	m_road = nullptr;
}
CNode* CAgent::GetUsedRoad()
{
	if ( ( m_pathfollowFlags & PATHFOLLOW_USE_ROAD ) == 0 )
	{
		return nullptr;
	}
	return m_road.Get();
}

Bool CAgent::FindRandomPositionInRadius( const Vector3& pos, Float radius, Vector3& posOut )
{
	AreaId areaId = m_currentArea;
	return m_world->FindRandomPositionInRadius( areaId, pos, radius, m_personalSpace, m_agentCategory, posOut ); 
}

Bool CAgent::FindSafeSpot( const Vector3& pos, Float radius, Vector3& outPos, PathLib::CollisionFlags flags )
{
	AreaId areaId = m_currentArea;
	return m_world->FindSafeSpot( areaId, pos, radius, m_personalSpace, outPos, nullptr, nullptr, m_defaultCollisionFlags | flags ); 
}
Float CAgent::GetClosestObstacle( Float radius, Vector3& outPos )
{
	return m_world->GetClosestObstacle( m_currentArea, m_position, radius, outPos, m_defaultCollisionFlags | CT_DEFAULT );
}
Float CAgent::GetClosestObstacle(const Vector3& pos1, const Vector3& pos2, Float radius, Vector3& outGeometryPos, Vector3& outLinePos )
{
	return m_world->GetClosestObstacle( m_currentArea, pos1, pos2, radius, outGeometryPos, outLinePos, m_defaultCollisionFlags | CT_DEFAULT );
}
Bool CAgent::ComputeHeight( const Vector3& pos, Float& z )
{
	return m_world->ComputeHeight( pos.AsVector2(), pos.Z - 1.f, pos.Z + 1.f, z, m_currentArea );
}

Bool CAgent::UpdatePreciseFollowPoint( Vector3& outFollowPosition )
{
	static const Float CLOSE_ENOUGHT = 0.1f;

	do
	{
		const Vector3& currPoint = GetWaypoint( GetCurrentWaypointIdx() );
		if ( (currPoint.AsVector2() - m_position.AsVector2()).SquareMag() <= CLOSE_ENOUGHT*CLOSE_ENOUGHT )
		{
			if (!NextWaypoint())
			{			
				outFollowPosition = currPoint;			
				return false;
			}
		}
		else
			break;
	}
	while (true);
	
	CAreaDescription* area = UpdateCurrentArea();
	if ( !area )
	{
		return false;
	};

	{
		Uint32 currentWaypoint = GetCurrentWaypointIdx();

		if ( ( GetWaypoint( currentWaypoint ) - m_position ).SquareMag() <= 0.25f )
		{			
			SetCurrentWaypoint( ++currentWaypoint );
		}
		Vector3 currentWaypointPos = GetWaypoint(currentWaypoint);
		CWideLineQueryData::MultiArea query( CWideLineQueryData( m_defaultCollisionFlags, m_position, currentWaypointPos, m_personalSpace ) );
		if ( !area->TMultiAreaQuery( query ) )
		{
			// we can't get to current waypoint
			// try to get at line with prev waypoint
			if ( currentWaypoint == 0 )
			{
				// we can get to our first waypoint. Basically we are completely lost.
				return false;
			}
			const Vector3& lastWp = GetWaypoint( currentWaypoint -1 );
			const Vector3& nextWp = GetWaypoint( currentWaypoint );
			outFollowPosition = FunctionalBinarySearch( lastWp, nextWp, NavUtils::SBinSearchContext( area, m_position, lastWp, nextWp, m_personalSpace, 0.2f, m_defaultCollisionFlags | CT_NO_ENDPOINT_TEST ) );
		}
		else
		{
			outFollowPosition = currentWaypointPos;
		}
	}

	return true;
}

Bool CAgent::UpdateFollowPoint( Vector3& outFollowPosition )
{
	if( m_pathfollowFlags & PATHFOLLOW_PRECISE )
	{
		return UpdatePreciseFollowPoint( outFollowPosition );
	}
	
	const Float CLOSE_ENOUGHT = 0.1f;
	const Float LOOK_AHEAD_DISTANCE_SQ = 20.f * 20.f;
	const Float DETAILED_BINSEARCH_DISTANCE_SQ = 40.f * 40.f;

	// Check if we are very close to current waypoint
	do
	{
		const Vector3& destination = GetDestination();
		if ( (destination.AsVector2() - m_position.AsVector2()).SquareMag() <= CLOSE_ENOUGHT*CLOSE_ENOUGHT )
		{
			if (!NextWaypoint())
			{
				// No waypoints to follow left... Looks like we just finished our path following task.
				outFollowPosition = destination;
				//OnMovementComplete();
				return false;
			}
		}
		else
			break;
	}
	while (true);

	CAreaDescription* area = UpdateCurrentArea();
	if ( !area )
	{
		// we are in void
		//OnMovementBlocked();
		return false;
	};

	{
		Uint32 currentWaypoint = GetCurrentWaypointIdx();
		Bool isBoundingOptimizable;
		Uint32 optimizableWaypointsCount = GetOptimizableWaypointsBounding( isBoundingOptimizable );

		CWideLineQueryData::MultiArea query( CWideLineQueryData( m_defaultCollisionFlags, m_position, GetWaypoint( currentWaypoint ), m_personalSpace ) );
		if ( area->TMultiAreaQuery( query ) )
		{
			while (currentWaypoint + 1 < optimizableWaypointsCount)
			{
				const Vector3& prevWp = GetWaypoint( currentWaypoint );
				Float prevWpDistSq = (prevWp.AsVector2() - m_position.AsVector2()).SquareMag();
				if ( prevWpDistSq > LOOK_AHEAD_DISTANCE_SQ )
				{
					break;
				}
				CWideLineQueryData::MultiArea query( CWideLineQueryData( m_defaultCollisionFlags, m_position, GetWaypoint( currentWaypoint + 1 ), m_personalSpace ) );
				if ( area->TMultiAreaQuery( query ) )
				{
					currentWaypoint++;
				}
				else 
				{
					break;
				}
			}
			SetCurrentWaypoint(currentWaypoint);
		}
		else
		{
			if ( !area->VTestLocation( m_position, m_defaultCollisionFlags ) )
			{
				// We have lost contact with ground. Pathfinding should fail after certain time.
				outFollowPosition = m_position;
				return true;
			}
			Bool acceptWaypoint = false;
			for (Int32 i = 0; i < 3 && currentWaypoint > 0; i++)
			{
				currentWaypoint--;
				CWideLineQueryData::MultiArea query( CWideLineQueryData( m_defaultCollisionFlags, m_position, GetWaypoint( currentWaypoint ), m_personalSpace ) );
				if ( area->TMultiAreaQuery( query ) )
				{
					acceptWaypoint = true;
					break;
				}
			}
			if (!acceptWaypoint)
			{
				//OnMovementBlocked();
				return false;
			}
		}
#ifdef PRECISE_PATH_FOLLOWING
		vOurDestination = GetWaypoint(nWaypoint);
		ASSERT(pArea->TestLine( vFromPosition, GetWaypoint(GetCurrentWaypointIdx()), m_personalSpace, m_defaultCollisionFlags ));
#else
		if ( currentWaypoint < optimizableWaypointsCount - 1 || 
			( isBoundingOptimizable && ( currentWaypoint + 1 < m_waypoints.Size() ) ) )
		{
			const Vector3& lastWp = GetWaypoint( currentWaypoint );
			const Vector3& nextWp = GetWaypoint( currentWaypoint + 1 );

			Bool doBinSearch = true;
			if ( ( lastWp.AsVector2() - m_position.AsVector2() ).SquareMag() > DETAILED_BINSEARCH_DISTANCE_SQ )
			{
				if ( ( nextWp.AsVector2() - m_position.AsVector2() ).SquareMag() > DETAILED_BINSEARCH_DISTANCE_SQ )
				{
					doBinSearch = false;
				}
			}
			if ( doBinSearch )
			{
				outFollowPosition = FunctionalBinarySearch( lastWp, nextWp, NavUtils::SBinSearchContext( area, m_position, lastWp, nextWp, m_personalSpace, 0.25f, m_defaultCollisionFlags | CT_NO_ENDPOINT_TEST ).LimitIterations( 4 ) );
			}
			else
			{
				outFollowPosition = lastWp;
			}
			
			if ( !area->VComputeHeight( outFollowPosition, outFollowPosition.Z ) )
			{
				// TODO: only possible on navmesh areas so we could do a cast and navmesh custom code to get 'triangle visibile from'
				//CNavmesh::TriangleIndex n = area->GetTriangleVisibleFrom( vOurDestination.AsVec2(), vFromPosition );
				//ASSERT( n!= CNavmesh::INVALID_INDEX );
				//if ( n != CNavmesh::INVALID_INDEX )
				//{
				//	vOurDestination.Z = pArea->ComputeHeight( n, vOurDestination.AsVec2() );
				//}
				//// TODO: there could be else but f**k it - this shouldn't be much problem with unexpected Z
			}
			if ( (outFollowPosition.AsVector2() - m_position.AsVector2()).SquareMag() < 0.5*0.5f )
			{
				// check if the path is actually still valid
				if ( !m_world->TestLine( m_currentArea, GetWaypoint( currentWaypoint + 1 ), outFollowPosition, m_personalSpace, CT_NO_ENDPOINT_TEST ) )
				{
					// path is no longer valid
					//OnMovementBlocked();
					return false;
				}
			}
			
			//m_pathfollowFlags &= ~FLAG_PATHFOLLOW_IS_AT_THE_END;
		}
		else
		{
			outFollowPosition = GetWaypoint(currentWaypoint);
			//m_flags |= FLAG_PATHFOLLOW_IS_AT_THE_END;
		}
		//ASSERT( area->VTestLine( m_position, outFollowPosition, m_personalSpace, m_defaultCollisionFlags | CT_NO_ENDPOINT_TEST ) );
#endif
	}

	return true;
}

};			// namespace PathLib
