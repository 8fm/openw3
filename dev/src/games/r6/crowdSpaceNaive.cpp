/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "crowdSpaceNaive.h"
#include "crowdManager.h"
#include "crowdArea.h"
#include "../../common/core/mathUtils.h"

CCrowdSpace_Naive::CCrowdSpace_Naive()
	: m_manager( nullptr )
{
}

TAgentIndex CCrowdSpace_Naive::GetAgentsCountWithinArea( const CCrowdArea* area ) const 
{
	const Box boxArea4 = area->GetBoundingBox(); 
	const Box boxArea2( boxArea4.Min.AsVector2(), boxArea4.Max.AsVector2() );
	TAgentIndex retVal( 0 );

	for ( TAgentIndex i = 0; i < GetNumAgents(); ++i )
	{
		// skip inactive agents
		if ( false == IsAgentActive( i ) )
		{
			continue;
		}

		retVal += boxArea2.Contains( GetAgentPos2( i ) ) ? 1 : 0;
	}

	return retVal;
}

void CCrowdSpace_Naive::OnInit( CCrowdManager* manager )
{
	ASSERT( manager && m_manager == nullptr );	
	m_manager = manager;
}

void CCrowdSpace_Naive::Reset()
{
	m_manager = nullptr;
}

#ifndef NO_GET_AGENTS_AREA
	TAgentIndex CCrowdSpace_Naive::GetAgentsAreaIndex( const SCrowdAgent& agent ) const
	{
		// simply, check all areas
		TAgentIndex numAreas = m_manager->GetNumAreas();
		for ( TAreaIndex i = 0; i < numAreas; ++i )
		{
			const CCrowdArea* area = m_manager->GetAreaByIndex( i );
			if ( area && area->ContainsPosition2( agent.m_pos ) )
			{
				return i;
			}
		}
		return INVALID_AREA_INDEX;
	}

	const CCrowdArea* CCrowdSpace_Naive::GetAgentsArea( const SCrowdAgent& agent ) const
	{
		// simply, check all areas
		TAgentIndex numAreas = m_manager->GetNumAreas();
		for ( TAreaIndex i = 0; i < numAreas; ++i )
		{
			const CCrowdArea* area = m_manager->GetAreaByIndex( i );
			if ( area && area->ContainsPosition2( agent.m_pos ) )
			{
				return area;
			}
		}
		return nullptr;
	}
#endif // NO_GET_AGENTS_AREA

void CCrowdSpace_Naive::UpdateSpace()
{
	// do nothing
}


RED_INLINE Vector2 CCrowdSpace_Naive::GetAgentPos2( TAgentIndex agent ) const
{
	return m_manager->GetAgentPos2( agent );
}

RED_INLINE TAgentIndex CCrowdSpace_Naive::GetNumAgents() const
{
	return m_manager->GetNumAgents();
}

RED_INLINE Float CCrowdSpace_Naive::GetAgentZ( TAgentIndex agent ) const
{
	return m_manager->GetAgentZ( agent );
}

RED_INLINE Box CCrowdSpace_Naive::CalcAgentBox( TAgentIndex agent ) const
{
	const Vector2 pos = GetAgentPos2( agent );
	const Float z = GetAgentZ( agent );
	return 
		Box( 
			Vector( pos.X - CCrowdManager::AGENT_RADIUS, pos.Y - CCrowdManager::AGENT_RADIUS, z ), 
			Vector( pos.X + CCrowdManager::AGENT_RADIUS, pos.Y + CCrowdManager::AGENT_RADIUS, z + CCrowdManager::AGENT_HEIGHT )  
		);
}

RED_INLINE Bool CCrowdSpace_Naive::IsAgentActive( TAgentIndex agent ) const
{
	return m_manager->IsAgentActive( agent );
}

TAgentIndex CCrowdSpace_Naive::GetAgentsInFrustum( const CFrustum& frustum, TAgentIndex maxAgents, TAgentIndex* result ) const
{
	ASSERT( maxAgents > 0 );

	TAgentIndex currOuputIndex = 0;
	for ( TAgentIndex i = 0; i < GetNumAgents(); ++i )
	{
		// skip inactive agents
		if ( false == IsAgentActive( i ) )
		{
			continue;
		}

		if ( FrustumTest( i, frustum ) )
		{
			result[ currOuputIndex ] = i;

			if ( ++currOuputIndex >= maxAgents )
			{
				break;
			}
		}
	}

	return currOuputIndex;
}

TAgentIndex CCrowdSpace_Naive::GetNNearestAgents( const Vector2& pos, TAgentIndex n, TAgentIndex* result ) const
{
	ASSERT( m_manager );

	// Naive implementation :)
	Float lastFoundDistSq = -1.f;
	TAgentIndex lastFoundIdx = INVALID_AGENT_INDEX;

	const TAgentIndex numAgents = GetNumAgents();
	n = min( numAgents, n );

	for ( TAgentIndex i = 0; i < n; ++i )
	{
		Float distSq;
		Float currBestDistSq = FLT_MAX;
		TAgentIndex currBestIdx = INVALID_AGENT_INDEX;
		for ( TAgentIndex k = 0; k < numAgents; ++k )
		{
			// skip inactive agents
			if ( false == IsAgentActive( k ) )
			{
				continue;
			}

			// calculate distance
			distSq = ( pos - GetAgentPos2( k ) ).SquareMag();

			// ignore this one if already in results table
			if ( distSq < lastFoundDistSq )
			{
				continue;
			}

			// there might be a situation, where few agents have the same distance
			// we have to check the results table if we encounter one that have the same distnce as last added
			if ( distSq == lastFoundDistSq )
			{
				if ( lastFoundIdx == k )
				{
					continue;
				}

				Bool alreadyFound( false );
				for ( TAgentIndex l = 0; l < i; ++l )
				{
					if ( result[ l ] == k )
					{
						alreadyFound = true;
						break;
					}
				}

				if ( alreadyFound )
				{
					continue;
				}
			}

			// check if distSq is best
			if ( distSq < currBestDistSq )
			{
				currBestDistSq = distSq;
				currBestIdx = k;
			}
		}

		// ok, so now best agent should be found
		result[ i ] = currBestIdx;
		lastFoundIdx = currBestIdx;
		lastFoundDistSq = currBestDistSq;
	}

#if defined( DEBUG_CROWD ) && !defined( PROFILE_CROWD )
	// verify the results

	// 1. check if i-th agent is really closer or equally close than (i + 1)-th agent
	for ( TAgentIndex i = 0; i < n - 1; ++i )
	{
		ASSERT( ( pos - GetAgentPos2( result[ i ] ) ).SquareMag() <= ( pos - GetAgentPos2( result[ i + 1 ] ) ).SquareMag(), 
			TXT( "CCrowdSpace_Naive::GetNNearestAgents() impementation is BROKEN, please DEBUG!" ) );
	}

	// 2. check if the result table doesn't contain duplicates
	for ( TAgentIndex i = 0; i < n - 1; ++i )
	{
		for ( TAgentIndex k = i + 1; k < n; ++k )
		{
			ASSERT( result[ i ] != result[ k ],
				TXT( "CCrowdSpace_Naive::GetNNearestAgents() impementation is BROKEN, please DEBUG!" ) );
		}
	}
#endif

	// return number of found agents
	return n;
}

TAgentIndex CCrowdSpace_Naive::GetNearestAgentsWithinRadius( const Vector2& pos, Float radius, TAgentIndex n, TAgentIndex* result ) const
{
	ASSERT( m_manager );

	// Naive implementation :)
	const Float radiusSq = radius * radius;
	Float lastFoundDistSq = -1.f;
	TAgentIndex lastFoundIdx = INVALID_AGENT_INDEX;

	const TAgentIndex numAgents = GetNumAgents();
	n = min( numAgents, n );

	TAgentIndex i;
	for ( i = 0; i < n; ++i )
	{
		Float distSq;
		Float currBestDistSq = FLT_MAX;
		TAgentIndex currBestIdx = INVALID_AGENT_INDEX;
		for ( TAgentIndex k = 0; k < numAgents; ++k )
		{
			// skip inactive agents
			if ( false == IsAgentActive( k ) )
			{
				continue;
			}

			// calculate distance
			distSq = ( pos - GetAgentPos2( k ) ).SquareMag();

			// ignore this one if already in results table
			if ( distSq < lastFoundDistSq )
			{
				continue;
			}

			// ignore if too far away
			if ( distSq > radiusSq )
			{
				continue;
			}

			// there might be a situation, where few agents have the same distance
			// we have to check the results table if we encounter one that have the same distnce as last added
			if ( distSq == lastFoundDistSq )
			{
				if ( lastFoundIdx == k )
				{
					continue;
				}

				Bool alreadyFound( false );
				for ( TAgentIndex l = 0; l < i; ++l )
				{
					if ( result[ l ] == k )
					{
						alreadyFound = true;
						break;
					}
				}

				if ( alreadyFound )
				{
					continue;
				}
			}

			// check if distSq is best
			if ( distSq < currBestDistSq )
			{
				currBestDistSq = distSq;
				currBestIdx = k;
			}
		}

		// no agent matching the criteria?
		if ( IS_NOT_VALID_AGENT_INDEX( currBestIdx ) )
		{
			break;
		}

		// ok, agent found
		result[ i ] = currBestIdx;
		lastFoundIdx = currBestIdx;
		lastFoundDistSq = currBestDistSq;
	}

#if defined( DEBUG_CROWD ) && !defined( PROFILE_CROWD )
	// verify the results

	// 1. check if j-th agent is really closer or equally close than (j + 1)-th agent
	for ( TAgentIndex j = 0; j < i - 1; ++j )
	{
		ASSERT( ( pos - GetAgentPos2( result[ j ] ) ).SquareMag() <= ( pos - GetAgentPos2( result[ j + 1 ] ) ).SquareMag(), 
			TXT( "CCrowdSpace_Naive::GetNearestAgentsWithinRadius() impementation is BROKEN, please DEBUG!" ) );
	}

	// 2. check if the result table doesn't contain duplicates
	for ( TAgentIndex j = 0; j < i - 1; ++j )
	{
		for ( TAgentIndex k = j + 1; k < i; ++k )
		{
			ASSERT( result[ j ] != result[ k ],
				TXT( "CCrowdSpace_Naive::GetNearestAgentsWithinRadius() impementation is BROKEN, please DEBUG!" ) );
		}
	}

	// 3. check if all agents are really within radius
	for ( TAgentIndex j = 0; j < i; ++j )
	{
		ASSERT( ( pos - GetAgentPos2( result[ j ] ) ).SquareMag() <= radiusSq, 
			TXT( "CCrowdSpace_Naive::GetNearestAgentsWithinRadius() impementation is BROKEN, please DEBUG!" ) );
	}
#endif

	// return number of found agents
	return i;
}

TAgentIndex CCrowdSpace_Naive::GetAnyAgentsWithinRadius( const Vector2& pos, Float radius, TAgentIndex maxAgents, TAgentIndex* result ) const
{
	const Float radiusSq = radius * radius;
	TAgentIndex nFound = 0;
	for ( TAgentIndex i = 0; i < GetNumAgents(); ++i )
	{
		// skip inactive agents
		if ( false == IsAgentActive( i ) )
		{
			continue;
		}

		if ( ( pos - GetAgentPos2( i ) ).SquareMag() <= radiusSq )
		{
			result[ nFound ] = i;
			if ( ++nFound >= maxAgents )
			{
				break;
			}
		}
	}

#if defined( DEBUG_CROWD ) && !defined( PROFILE_CROWD )
	// verify the results

	// 1. check if the result table doesn't contain duplicates
	for ( TAgentIndex j = 0; j < nFound - 1; ++j )
	{
		for ( TAgentIndex k = j + 1; k < nFound; ++k )
		{
			ASSERT( result[ j ] != result[ k ],
				TXT( "CCrowdSpace_Naive::GetAnyAgentsWithinRadius() impementation is BROKEN, please DEBUG!" ) );
		}
	}

	// 3. check if all agents are really within radius
	for ( TAgentIndex j = 0; j < nFound; ++j )
	{
		ASSERT( ( pos - GetAgentPos2( result[ j ] ) ).SquareMag() <= radiusSq, 
			TXT( "CCrowdSpace_Naive::GetAnyAgentsWithinRadius() impementation is BROKEN, please DEBUG!" ) );
	}

#endif

	return nFound;
}

Bool CCrowdSpace_Naive::Ray2Test_Any( const SCrowdRay2& ray ) const
{
	// Naive impl
	Vector2 pointOnLine;
	const Float agentRadiusSq = ( ray.m_radius + CCrowdManager::AGENT_RADIUS ) * ( ray.m_radius + CCrowdManager::AGENT_RADIUS );
	for ( TAgentIndex i = 0; i < GetNumAgents(); ++i )
	{
		// skip inactive agents
		if ( false == IsAgentActive( i ) )
		{
			continue;
		}

		if ( MathUtils::GeometryUtils::DistanceSqrPointToLineSeg2D( GetAgentPos2( i ), ray.m_start, ray.m_end, pointOnLine ) <= agentRadiusSq )
		{
			return true;
		}
	}	

	return false;
}

Bool CCrowdSpace_Naive::Ray2Test_Closest( const SCrowdRay2& ray, TAgentIndex& hitAgent ) const
{
	// Naive impl
	Vector2 pointOnLine;
	Float bestDisplSqSoFar = FLT_MAX;
	TAgentIndex bestAgentSoFar = INVALID_AGENT_INDEX;
	const Float agentRadiusSq = ( ray.m_radius + CCrowdManager::AGENT_RADIUS ) * ( ray.m_radius + CCrowdManager::AGENT_RADIUS );
	for ( TAgentIndex i = 0; i < GetNumAgents(); ++i )
	{
		// skip inactive agents
		if ( false == IsAgentActive( i ) )
		{
			continue;
		}

		if ( MathUtils::GeometryUtils::DistanceSqrPointToLineSeg2D( GetAgentPos2( i ), ray.m_start, ray.m_end, pointOnLine ) <= agentRadiusSq )
		{
			// calculate squared displacement 
			const Float sqDispl = ( pointOnLine - ray.m_start ).SquareMag();

			// see if this one is closer than previously found one
			if ( sqDispl < bestDisplSqSoFar )
			{
				bestDisplSqSoFar = sqDispl;
				bestAgentSoFar = i;
			}
		}
	}	

	if ( IS_VALID_AGENT_INDEX( bestAgentSoFar ) )
	{
		hitAgent = bestAgentSoFar;
		return true;
	}

	return false;
}

TAgentIndex CCrowdSpace_Naive::Ray2Test_NClosest( const SCrowdRay2& ray, TAgentIndex maxAgents, TAgentIndex* result ) const
{
	// Naive impl
	Vector2 pointOnLine;
	const Float agentRadiusSq = ( ray.m_radius + CCrowdManager::AGENT_RADIUS ) * ( ray.m_radius + CCrowdManager::AGENT_RADIUS );
	maxAgents = min( maxAgents, GetNumAgents() );

	Float lastFoundDistSq = -1.f;
	TAgentIndex lastFoundIdx = INVALID_AGENT_INDEX;

	TAgentIndex i;
	for ( i = 0; i < maxAgents; ++i )
	{
		Float bestDisplSqSoFar = FLT_MAX;
		TAgentIndex bestAgentSoFar = INVALID_AGENT_INDEX;
		for ( TAgentIndex k = 0; k < GetNumAgents(); ++k )
		{
			// skip inactive agents
			if ( false == IsAgentActive( k ) )
			{
				continue;
			}

			if ( MathUtils::GeometryUtils::DistanceSqrPointToLineSeg2D( GetAgentPos2( k ), ray.m_start, ray.m_end, pointOnLine ) <= agentRadiusSq )
			{
				// calculate squared displacement 
				const Float sqDispl = ( pointOnLine - ray.m_start ).SquareMag();

				// ignore if found already
				if ( lastFoundDistSq > sqDispl )
				{
					continue;
				}

				// ignore if found already
				if ( lastFoundDistSq == sqDispl )
				{
					if ( lastFoundIdx == k )
					{
						continue;
					}

					Bool alreadyFound( false );
					for ( TAgentIndex l = 0; l < i; ++l )
					{
						if ( result[ l ] == k )
						{
							alreadyFound = true;
							break;
						}
					}

					if ( alreadyFound )
					{
						continue;
					}
				}

				// see if this one is closer than previously found one
				if ( sqDispl < bestDisplSqSoFar )
				{
					bestDisplSqSoFar = sqDispl;
					bestAgentSoFar = k;
				}
			}
		}	

		// no agent hit?
		if ( IS_NOT_VALID_AGENT_INDEX( bestAgentSoFar ) )
		{
			break;
		}

		// ok, agent hit
		result[ i ] = bestAgentSoFar;
		lastFoundIdx = bestAgentSoFar;
		lastFoundDistSq = bestDisplSqSoFar;
	}

#if defined( DEBUG_CROWD ) && !defined( PROFILE_CROWD )
	// verify the results

	// 1. check if the result table doesn't contain duplicates
	for ( TAgentIndex j = 0; j < i - 1; ++j )
	{
		for ( TAgentIndex k = j + 1; k < i; ++k )
		{
			ASSERT( result[ j ] != result[ k ],
				TXT( "CCrowdSpace_Naive::Ray2Test_NClosest() impementation is BROKEN, please DEBUG!" ) );
		}
	}

	// 2. check (using diffenet function) if distance (of each result) to the line is really smaller (or equal) than agents radius
	for ( TAgentIndex j = 0; j < i; ++j )
	{
		ASSERT( MathUtils::GeometryUtils::DistanceSqrPointToLine2D( GetAgentPos2( result[ j ] ), ray.m_start, ray.m_end ) <= agentRadiusSq,
			TXT( "CCrowdSpace_Naive::Ray2Test_NClosest() impementation is BROKEN, please DEBUG!" ) );
	}

	// TODO: 3. check if agents in results table are in proper order considering real ray-agent hit points
#endif

	return i;
}

Bool CCrowdSpace_Naive::Ray3Test_Any( const SCrowdRay3& ray ) const
{
	ASSERT( false, TXT("Not implemented yet." ) );
	return false;
}

Bool CCrowdSpace_Naive::Ray3Test_Closest( const SCrowdRay3& ray, TAgentIndex& hitAgent ) const
{
	ASSERT( false, TXT("Not implemented yet." ) );
	return false;
}

TAgentIndex CCrowdSpace_Naive::Ray3Test_NClosest( const SCrowdRay3& ray, TAgentIndex maxAgents, TAgentIndex* outputArray ) const
{
	ASSERT( false, TXT("Not implemented yet." ) );
	return 0;
}

TObstacleIndex CCrowdSpace_Naive::GetObstaclesWithinRadius( const Vector2& pos, Float radius, TObstacleIndex maxObstacles, TObstacleIndex* result ) const
{
	const Float sqRadius = radius * radius;

	Vector2 linePoint;
	TObstacleIndex currResult( 0 );
	ASSERT( maxObstacles > 0 );

	// simply, check all the obstacles
	const TObstacleIndex numObstacles = m_manager->GetNumObstacles();
	for ( TObstacleIndex i = 0; i < numObstacles; ++i )
	{
		// ...checking the radius...
		const SCrowdObstacleEdge& obstacle = m_manager->GetObstacle( i );
		if ( MathUtils::GeometryUtils::DistanceSqrPointToLineSeg2D( pos, obstacle.m_begin, obstacle.m_end, linePoint ) <= sqRadius )
		{
			// ...to add it to the array
			result[ currResult ] = i;
			if ( ++currResult >= maxObstacles )
			{
				return maxObstacles;
			}
		}
	}

	return currResult;
}


