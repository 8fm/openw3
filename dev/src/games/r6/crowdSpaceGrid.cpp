/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "crowdSpaceGrid.h"
#include "crowdManager.h"
#include "crowdArea.h"
#include "gridCrawler.inl"
#include "../../common/core/mathUtils.h"

CCrowdSpace_Grid::CCrowdSpace_Grid()
	: m_manager( nullptr )
	, m_ready( false )
{
}

TAgentIndex CCrowdSpace_Grid::GetAgentsCountWithinArea( const CCrowdArea* area ) const 
{
	TAgentIndex result = 0;
	for ( SGridCrawlerInOrder crawler( RelativeBox2( area->GetBoundingBox2() ) ); crawler; ++crawler )
	{
		for ( SGridCellForAgents::Iterator it( this, crawler.X, crawler.Y ); it; ++it )	
		{ 
			if ( area->ContainsPosition2( GetAgentPos2( *it ) ) )
			{
				++result;
			}
		} 
	}
	return result;
}

void CCrowdSpace_Grid::OnInit( CCrowdManager* manager )
{
	ASSERT( manager && m_manager == nullptr );	
	m_manager = manager;
}

void CCrowdSpace_Grid::Reset()
{
	#ifndef NO_GET_AGENTS_AREA
		m_areasData.Clear();
	#endif
	m_ready = false;
	m_manager = nullptr;
}

#ifndef NO_GET_AGENTS_AREA
	TAreaIndex CCrowdSpace_Grid::GetAgentsAreaIndex( const SCrowdAgent& agent ) const
	{
		Uint32 x, y;
		CellCoords( agent.m_pos, x, y );
		for ( SGridCellForAreas::Iterator it( this, x, y ); it; ++it )
		{
			const CCrowdArea* area = m_manager->GetAreaByIndex( *it );
			if ( area && area->ContainsPosition2( agent.m_pos ) )
			{
				return *it;
			}
		}
		return INVALID_AREA_INDEX;
	}

	const CCrowdArea* CCrowdSpace_Grid::GetAgentsArea( const SCrowdAgent& agent ) const
	{
		return m_manager->GetAreaByIndex( GetAgentsAreaIndex( agent ) );
	}
#endif // NO_GET_AGENTS_AREA

RED_INLINE Vector2 CCrowdSpace_Grid::GetObstacleStart2( TObstacleIndex idx ) const
{
	return m_manager->GetObstacle( idx ).m_begin;
}

RED_INLINE Vector2 CCrowdSpace_Grid::GetObstacleEnd2( TObstacleIndex idx ) const
{
	return m_manager->GetObstacle( idx ).m_end;
}

RED_INLINE void CCrowdSpace_Grid::CellCoords( const Vector2& pos, Uint32& x, Uint32& y ) const
{
	const Vector2 posInGridSpace = pos - m_aabb.Min;
	x = Uint32 ( Clamp< Int32 > ( Int32( posInGridSpace.X / GRID_CELL_SIZE ), 0, NUM_GRID_CELLS_XY - 1 ) ); 
	y = Uint32 ( Clamp< Int32 > ( Int32( posInGridSpace.Y / GRID_CELL_SIZE ), 0, NUM_GRID_CELLS_XY - 1 ) ); 
}

RED_INLINE Box2 CCrowdSpace_Grid::RelativeBox2( const Box2& absoluteBox ) const
{
	ASSERT( m_ready );
	return Box2(	Vector2(	Clamp< Float > ( absoluteBox.Min.X - m_aabb.Min.X, 0.f, NUM_GRID_CELLS_XY * GRID_CELL_SIZE ),
								Clamp< Float > ( absoluteBox.Min.Y - m_aabb.Min.Y, 0.f, NUM_GRID_CELLS_XY * GRID_CELL_SIZE ) ),
					Vector2(	Clamp< Float > ( absoluteBox.Max.X - m_aabb.Min.X, 0.f, NUM_GRID_CELLS_XY * GRID_CELL_SIZE ),
								Clamp< Float > ( absoluteBox.Max.Y - m_aabb.Min.Y, 0.f, NUM_GRID_CELLS_XY * GRID_CELL_SIZE ) )
				);
}

RED_INLINE Bool CCrowdSpace_Grid::IsAgentActive( TAgentIndex agent ) const
{
	return m_manager->IsAgentActive( agent );
}

void CCrowdSpace_Grid::UpdateSpace()
{
	// TODO: optimize?
	// for now, building the grid is single-threaded

	ASSERT( m_manager );

#ifdef PROFILE_CROWD
	Double time, arTime( 0.0 );
	{
		Red::System::ScopedStopClock clk( time );
#endif
		// 1. Update aabb
		const Float halfExtent = GRID_CELL_SIZE * Float( NUM_GRID_CELLS_XY ) * 0.5f;
		const Vector2 halfExtents( halfExtent, halfExtent );
		m_aabb = Box2( m_manager->GetCenterPoint2() - halfExtents, m_manager->GetCenterPoint2() + halfExtents ); 
		m_ready = true;

		// 2. Clear the grid
		union 
		{	
			struct { Int16 a, b; };	
			struct { Int32 i; }; 
		} u;
		u.a = u.b = -1; // yeah, i know it's the same on each platform ( 0xffffffff ), but I think this way it looks cleaner 
		// This is based on knowledge that SGridCell's only data member is m_firstAgent, with default value of "-1".
		// Otherwide it would require iteratively going through all the cells to reset them.
		Red::System::MemorySet( m_gridForAgents, u.i, sizeof( m_gridForAgents ) );

		// 3. Refresh number of agents
		m_numAgents = m_manager->GetNumAgents();

		// 4. Go through all the agents and assign them to proper cells
		Uint32 x, y;
		for ( TAgentIndex i = 0; i < m_numAgents; ++i )
		{
			// skip inactive agents
			if ( false == IsAgentActive( i ) )
			{
				continue;
			}

			// get the cell
			CellCoords( GetAgentPos2( i ), x, y );
			SGridCellForAgents& cell = m_gridForAgents[ y ][ x ];

			// connect cell's first agent as next of current agent
			m_nextAgents[ i ] = cell.m_firstAgent;

			// put current agent as cell's first
			cell.m_firstAgent = i;
		}

		// 5. Go through all the cells and determine their areas
		// Removed for now, due to lack of optimal implementation. This was just too slow.
		// We can either optimize this part, or remove it all. Either way, I don't think it's really needed.
		#ifndef NO_GET_AGENTS_AREA
			#ifdef PROFILE_CROWD
			{
				Red::System::ScopedStopClock clk( arTime );
			#endif
				m_areasData.ClearFast();
				const TAreaIndex numAreas = m_manager->GetNumAreas();
				for ( y = 0; y < NUM_GRID_CELLS_XY; ++y )
				{
					for ( x = 0; x < NUM_GRID_CELLS_XY; ++x )
					{
						SGridCellForAreas& cell = m_gridForAreas[ y ][ x ];
						cell.Clear();

						Vector2 vert; 	
						vert.X = Float( x ) * GRID_CELL_SIZE;
						vert.Y = Float( y ) * GRID_CELL_SIZE;
						vert += m_aabb.Min;

						for ( TAreaIndex i = 0; i <	numAreas; ++i )
						{
							const CCrowdArea* area = m_manager->GetAreaByIndex( i );
							if (	area && (
									area->ContainsPosition2( vert ) ||
									area->ContainsPosition2( vert + Vector2( GRID_CELL_SIZE, 0.f ) ) ||
									area->ContainsPosition2( vert + Vector2( 0.f, GRID_CELL_SIZE ) ) ||
									area->ContainsPosition2( vert + Vector2( GRID_CELL_SIZE, GRID_CELL_SIZE ) ) ) )
							{
								++cell.m_numAreas;
								if ( cell.m_numAreas == 1 )
								{
									cell.m_indexOrOffset = i;
								}
								else if ( cell.m_numAreas == 2 )
								{
									TAreaIndex firstIdx = cell.m_indexOrOffset;
									cell.m_indexOrOffset = TAreaIndex( m_areasData.Grow( 2 ) );
									m_areasData[ cell.m_indexOrOffset ] = firstIdx;
									m_areasData[ cell.m_indexOrOffset + 1 ] = i;
								}
								else
								{
									m_areasData.PushBack( i );
								}
							}
						}
					}
				}
			#ifdef PROFILE_CROWD
			}
			#endif
		#endif // NO_GET_AGENTS_AREA

		// 6. Clear the obstacle grid
		Red::System::MemorySet( m_gridForObstacles, -1, sizeof( m_gridForObstacles ) );

		// 7. Go through all the obstacles and assign them to proper cells
		m_obstaclesData.ResizeFast( size_t( m_manager->GetNumObstacles() ) );
		for ( TObstacleIndex i = 0; i < m_obstaclesData.SizeInt(); ++i )
		{
			// get the cell
			CellCoords( GetObstacleStart2( i ), x, y );
			SGridCellForObstacles& cell = m_gridForObstacles[ y ][ x ];

			// connect cell's first obstacle as next of current obstacle
			m_obstaclesData[ i ] = cell.m_firstObstacle;

			// put current agent as cell's first
			cell.m_firstObstacle = i;
		}

#ifdef PROFILE_CROWD
	}

	Uint32 numEmptyCells( 0 );
	Uint32 numCellsWithOneAgent( 0 );
	Uint32 numCellsWithLessThan7Agents( 0 );
	Uint32 maxNumAgentsInCell( 0 );

	// 5. DEBUG: count the stats
	for ( Uint32 y = 0; y < NUM_GRID_CELLS_XY; ++y )
	{
		for ( Uint32 x = 0; x < NUM_GRID_CELLS_XY; ++x )
		{
			Uint32 numAgentsInThisCell( CountAgentsInCell( x, y ) );

			if ( numAgentsInThisCell < 7 )
			{
				if ( 1 == numAgentsInThisCell )
				{
					++numCellsWithOneAgent;
				}
				else if ( 0 == numAgentsInThisCell )
				{
					++numEmptyCells;
				}
				else
				{
					++numCellsWithLessThan7Agents;
				}
			}

			maxNumAgentsInCell = max( maxNumAgentsInCell, numAgentsInThisCell );
		}
	}

	RED_LOG( Crowd, TXT("stats: time (ms): %.3lf, no areas time: %.3lf, empty: %ld, 1agent: %ld, 2-7agent: %ld, max: %ld"), time *  1000.0, ( time - arTime ) *  1000.0, numEmptyCells, numCellsWithOneAgent, numCellsWithLessThan7Agents, maxNumAgentsInCell );
#endif
}

RED_INLINE Float CCrowdSpace_Grid::GetAgentZ( TAgentIndex agent ) const
{
	return m_manager->GetAgentZ( agent );
}

RED_INLINE Vector2 CCrowdSpace_Grid::GetAgentPos2( TAgentIndex agent ) const
{
	return m_manager->GetAgentPos2( agent );
}

RED_INLINE Box CCrowdSpace_Grid::CalcAgentBox( TAgentIndex agent ) const
{
	const Vector2& pos = GetAgentPos2( agent );
	const Float z = GetAgentZ( agent );
	return 
		Box( 
			Vector( pos.X - CCrowdManager::AGENT_RADIUS, pos.Y - CCrowdManager::AGENT_RADIUS, z ), 
			Vector( pos.X + CCrowdManager::AGENT_RADIUS, pos.Y + CCrowdManager::AGENT_RADIUS, z + CCrowdManager::AGENT_HEIGHT )  
		);
}

RED_INLINE Uint32 CCrowdSpace_Grid::EstimateCrawlerSizeForNAgents( TAgentIndex n ) const
{
	// this logic is a little bit... poor...
	// TODO: some better logic, perhaps?
	const Float avgAgentsPerSqareMeter = 0.5f;
	const Float avgAgentsPerCell = Clamp( avgAgentsPerSqareMeter * GRID_CELL_SIZE * GRID_CELL_SIZE, 0.1f, 1000.f );
	const Float estimatedCellsNeeded = Float( n ) / avgAgentsPerCell;
	return Uint32( MSqrt( estimatedCellsNeeded ) ) + 2; 
}

TAgentIndex CCrowdSpace_Grid::GetAgentsInFrustum( const CFrustum& frustum, TAgentIndex maxAgents, TAgentIndex* result ) const
{
	// this one is implemented the same way as in CCrowdSpace_Naive
	// I see no point in using grid for this. Probably it won't be faster than naive implementation.
	// In case this causes any performance problem, we might want to revisit this place.

	ASSERT( maxAgents > 0 );

	TAgentIndex currOuputIndex = 0;
	for ( TAgentIndex i = 0; i < GetNumAgents(); ++i )
	{
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

TAgentIndex CCrowdSpace_Grid::GetNNearestAgents( const Vector2& pos, TAgentIndex n, TAgentIndex* result ) const
{
	Uint32 x, y;
	SSortedByDistanceInplaceAgentsArray agents( m_manager, pos, n, result );
	const Uint32 crawlerSize = EstimateCrawlerSizeForNAgents( n ); // crawler will go through crawlerSize * crawlerSize cells only

	CellCoords( pos, x, y );

	// crawl through the grid...
	for ( SGridCrawlerCCW crawler( x, y, crawlerSize ); crawler; ++crawler )
	{
		// ...and iterate over cell's agents...
		for ( SGridCellForAgents::Iterator it( this, crawler.X, crawler.Y ); it; ++it )
		{
			// ...to let the sorted array do it's stuff
			agents.Insert( *it );
		}
	}

#if defined( DEBUG_CROWD ) && !defined( PROFILE_CROWD )
	// verify the results

	TAgentIndex i = agents.CurrSize();

	// 1. check if j-th agent is really closer or equally close than (j + 1)-th agent
	for ( TAgentIndex j = 0; j < i - 1; ++j )
	{
		ASSERT( ( pos - GetAgentPos2( result[ j ] ) ).SquareMag() <= ( pos - GetAgentPos2( result[ j + 1 ] ) ).SquareMag(), 
			TXT( "CCrowdSpace_Grid::GetNNearestAgents() impementation is BROKEN, please DEBUG!" ) );
	}

	// 2. check if the result table doesn't contain duplicates
	for ( TAgentIndex j = 0; j < i - 1; ++j )
	{
		for ( TAgentIndex k = j + 1; k < i; ++k )
		{
			ASSERT( result[ j ] != result[ k ],
				TXT( "CCrowdSpace_Grid::GetNNearestAgents() impementation is BROKEN, please DEBUG!" ) );
		}
	}
#endif

	return agents.CurrSize();
}

TAgentIndex CCrowdSpace_Grid::GetNearestAgentsWithinRadius( const Vector2& pos, Float radius, TAgentIndex n, TAgentIndex* result ) const
{
	SSortedByDistanceInplaceAgentsArray agents( m_manager, pos, n, result );
	const Float sqRadius = radius * radius;

	// crawl through the grid...
	for ( SGridCrawlerInOrder crawler( RelativeBox2( Box2( pos, radius ) ) ); crawler; ++crawler )
	{
		// ...and iterate over cell's agents...
		for ( SGridCellForAgents::Iterator it( this, crawler.X, crawler.Y ); it; ++it )
		{
			// ...checking the radius...
			if ( ( pos - GetAgentPos2( *it ) ).SquareMag() <= sqRadius )
			{
				// ...to let the sorted array do it's stuff
				agents.Insert( *it );
			}
		}
	}

#if defined( DEBUG_CROWD ) && !defined( PROFILE_CROWD )
	// verify the results

	TAgentIndex i = agents.CurrSize();

	// 1. check if j-th agent is really closer or equally close than (j + 1)-th agent
	for ( TAgentIndex j = 0; j < i - 1; ++j )
	{
		ASSERT( ( pos - GetAgentPos2( result[ j ] ) ).SquareMag() <= ( pos - GetAgentPos2( result[ j + 1 ] ) ).SquareMag(), 
			TXT( "CCrowdSpace_Grid::GetNearestAgentsWithinRadius() impementation is BROKEN, please DEBUG!" ) );
	}

	// 2. check if the result table doesn't contain duplicates
	for ( TAgentIndex j = 0; j < i - 1; ++j )
	{
		for ( TAgentIndex k = j + 1; k < i; ++k )
		{
			ASSERT( result[ j ] != result[ k ],
				TXT( "CCrowdSpace_Grid::GetNearestAgentsWithinRadius() impementation is BROKEN, please DEBUG!" ) );
		}
	}

	// 3. check if all agents are really within radius
	for ( TAgentIndex j = 0; j < i; ++j )
	{
		ASSERT( ( pos - GetAgentPos2( result[ j ] ) ).SquareMag() <= sqRadius, 
			TXT( "CCrowdSpace_Grid::GetNearestAgentsWithinRadius() impementation is BROKEN, please DEBUG!" ) );
	}
#endif

	return agents.CurrSize();
}

TAgentIndex CCrowdSpace_Grid::GetAnyAgentsWithinRadius( const Vector2& pos, Float radius, TAgentIndex maxAgents, TAgentIndex* result ) const
{
	Uint32 x, y;
	const Uint32 crawlerSize = Uint32( radius * 2.f / GRID_CELL_SIZE ) + 1;
	const Float sqRadius = radius * radius;
	TAgentIndex numResults( 0 );

	CellCoords( pos, x, y );

	// crawl through the grid...
	for ( SGridCrawlerCCW crawler( x, y, crawlerSize ); crawler; ++crawler )
	{
		// ...and iterate over cell's agents...
		for ( SGridCellForAgents::Iterator it( this, crawler.X, crawler.Y ); it; ++it )
		{
			// ...checking the radius...
			if ( ( pos - GetAgentPos2( *it ) ).SquareMag() <= sqRadius )
			{
				// ...to put them into the array, disregarding the order.
				result[ numResults ] = ( *it );
				++numResults;

				// break if the array is full
				if ( numResults == maxAgents )
				{
					break;
				}
			}
		}
	}

#if defined( DEBUG_CROWD ) && !defined( PROFILE_CROWD )
	// verify the results

	// 1. check if the result table doesn't contain duplicates
	for ( TAgentIndex j = 0; j < numResults - 1; ++j )
	{
		for ( TAgentIndex k = j + 1; k < numResults; ++k )
		{
			ASSERT( result[ j ] != result[ k ],
				TXT( "CCrowdSpace_Grid::GetAnyAgentsWithinRadius() impementation is BROKEN, please DEBUG!" ) );
		}
	}

	// 2. check if all agents are really within radius
	for ( TAgentIndex j = 0; j < numResults; ++j )
	{
		ASSERT( ( pos - GetAgentPos2( result[ j ] ) ).SquareMag() <= sqRadius, 
			TXT( "CCrowdSpace_Grid::GetAnyAgentsWithinRadius() impementation is BROKEN, please DEBUG!" ) );
	}
#endif

	return numResults;
}

Bool CCrowdSpace_Grid::Ray2Test_Any( const SCrowdRay2& ray ) const
{
	const Float agentRadiusSq = ( ray.m_radius + CCrowdManager::AGENT_RADIUS ) * ( ray.m_radius + CCrowdManager::AGENT_RADIUS );
	Vector2 pointOnLine;

	// crawl through the grid...
	for ( SGridCrawlerRay crawler( ray, m_aabb.Min ); crawler; ++crawler )
	{
		// ...and iterate over cell's agents...
		for ( SGridCellForAgents::Iterator it( this, crawler.X, crawler.Y ); it; ++it )
		{
			// ...checking the ray...
			if ( MathUtils::GeometryUtils::DistanceSqrPointToLineSeg2D( GetAgentPos2( *it ), ray.m_start, ray.m_end, pointOnLine ) <= agentRadiusSq )
			{
				// ... to exit on first hit
				return true;
			}
		}
	}

	return false;
}

Bool CCrowdSpace_Grid::Ray2Test_Closest( const SCrowdRay2& ray, TAgentIndex& hitAgent ) const
{
	SSortedByDistanceInplaceAgentsArray agents( m_manager, ray.m_start, 1, &hitAgent );
	const Float agentRadiusSq = ( ray.m_radius + CCrowdManager::AGENT_RADIUS ) * ( ray.m_radius + CCrowdManager::AGENT_RADIUS );
	Vector2 pointOnLine;

	// crawl through the grid...
	for ( SGridCrawlerRay crawler( ray, m_aabb.Min ); crawler; ++crawler )
	{
		// ...and iterate over cell's agents...
		for ( SGridCellForAgents::Iterator it( this, crawler.X, crawler.Y ); it; ++it )
		{
			// ...checking the ray...
			if ( MathUtils::GeometryUtils::DistanceSqrPointToLineSeg2D( GetAgentPos2( *it ), ray.m_start, ray.m_end, pointOnLine ) <= agentRadiusSq )
			{
				// ...to let the sorted array do it's stuff
				agents.Insert( *it );

				// TODO: early exit?
			}
		}
	}

	return agents.CurrSize() == 1;
}

TAgentIndex CCrowdSpace_Grid::Ray2Test_NClosest( const SCrowdRay2& ray, TAgentIndex maxAgents, TAgentIndex* result ) const
{
	SSortedByDistanceInplaceAgentsArray agents( m_manager, ray.m_start, maxAgents, result );
	const Float agentRadiusSq = ( ray.m_radius + CCrowdManager::AGENT_RADIUS ) * ( ray.m_radius + CCrowdManager::AGENT_RADIUS );
	Vector2 pointOnLine;

	// crawl through the grid...
	for ( SGridCrawlerRay crawler( ray, m_aabb.Min ); crawler; ++crawler )
	{
		// ...and iterate over cell's agents...
		for ( SGridCellForAgents::Iterator it( this, crawler.X, crawler.Y ); it; ++it )
		{
			// ...checking the ray...
			if ( MathUtils::GeometryUtils::DistanceSqrPointToLineSeg2D( GetAgentPos2( *it ), ray.m_start, ray.m_end, pointOnLine ) <= agentRadiusSq )
			{
				// ...to let the sorted array do it's stuff
				agents.Insert( *it );

				// TODO: early exit?
			}
		}
	}

#if defined( DEBUG_CROWD ) && !defined( PROFILE_CROWD )
	// verify the results

	TAgentIndex i = agents.CurrSize();

	// 1. check if j-th agent is really closer or equally close than (j + 1)-th agent
	for ( TAgentIndex j = 0; j < i - 1; ++j )
	{
		ASSERT( ( ray.m_start - GetAgentPos2( result[ j ] ) ).SquareMag() <= ( ray.m_start - GetAgentPos2( result[ j + 1 ] ) ).SquareMag(), 
			TXT( "CCrowdSpace_Grid::GetNearestAgentsWithinRadius() impementation is BROKEN, please DEBUG!" ) );
	}

	// 2. check if the result table doesn't contain duplicates
	for ( TAgentIndex j = 0; j < i - 1; ++j )
	{
		for ( TAgentIndex k = j + 1; k < i; ++k )
		{
			ASSERT( result[ j ] != result[ k ],
				TXT( "CCrowdSpace_Grid::GetNearestAgentsWithinRadius() impementation is BROKEN, please DEBUG!" ) );
		}
	}
#endif

	return agents.CurrSize();
}

Bool CCrowdSpace_Grid::Ray3Test_Any( const SCrowdRay3& ray ) const
{
	ASSERT( false, TXT("Not implemented yet." ) );
	return false;
}

Bool CCrowdSpace_Grid::Ray3Test_Closest( const SCrowdRay3& ray, TAgentIndex& hitAgent ) const
{
	ASSERT( false, TXT("Not implemented yet." ) );
	return false;
}

TAgentIndex CCrowdSpace_Grid::Ray3Test_NClosest( const SCrowdRay3& ray, TAgentIndex maxAgents, TAgentIndex* outputArray ) const
{
	ASSERT( false, TXT("Not implemented yet." ) );
	return 0;
}

TObstacleIndex CCrowdSpace_Grid::GetObstaclesWithinRadius( const Vector2& pos, Float radius, TObstacleIndex maxObstacles, TObstacleIndex* result ) const
{
	// increase the search radius by edge len 
	// ( we're taking only one end when assigning to cells, so during the search we need to include nearby cells also )
	const Float sqRadius = radius * radius;
	radius += SCrowdObstacleEdge::MAX_EDGE_LEN;

	Vector2 linePoint;
	TObstacleIndex currResult( 0 );
	ASSERT( maxObstacles > 0 );

	// start from the cell at the exact spot to prevent overflowing the result table with more distant edges
	Uint32 x, y;
	CellCoords( pos, x, y );
	for ( SGridCrawlerCCW crawler( x, y, Uint32( ( radius + 1.f ) / GRID_CELL_SIZE ) + 1 ); crawler; ++crawler )
	{
		// ...and iterate over cell's obstacles...
		for ( SGridCellForObstacles::Iterator it( this, crawler.X, crawler.Y ); it; ++it )
		{
			// ...checking the radius...
			if ( MathUtils::GeometryUtils::DistanceSqrPointToLineSeg2D( pos, GetObstacleStart2( *it ), GetObstacleEnd2( *it ), linePoint ) <= sqRadius )
			{
				// ...to add it to the array
				result[ currResult ] = *it;
				if ( ++currResult >= maxObstacles )
				{
					return maxObstacles;
				}
			}
		}
	}

	return currResult;
}

//------------------------------------------------------------------------------------------------------------------
// Cell iterators
//------------------------------------------------------------------------------------------------------------------
CCrowdSpace_Grid::SGridCellForAgents::Iterator::Iterator( const CCrowdSpace_Grid* self, Uint32 x, Uint32 y ) 
	: m_nextAgents( self->m_nextAgents ) 
	, m_currentAgent( self->m_gridForAgents[ y ][ x ].m_firstAgent )
{

}

RED_INLINE TAgentIndex CCrowdSpace_Grid::CountAgentsInCell( Uint32 x, Uint32 y ) const
{
	TAgentIndex c( 0 ); 
	for ( SGridCellForAgents::Iterator i( this, x, y ); i; ++i )	
	{ 
		++c;
	} 
	return c;
}

#ifndef NO_GET_AGENTS_AREA
	RED_INLINE CCrowdSpace_Grid::SGridCellForAreas::Iterator::Iterator( const CCrowdSpace_Grid* self, Uint32 x, Uint32 y )
		: m_numLeft( self->m_gridForAreas[ y ][ x ].m_numAreas )
	{
		ASSERT( m_numLeft >= 0 );
	
		if ( m_numLeft < 1 )
		{
			static TAreaIndex dummyInvalidData = INVALID_AREA_INDEX;
			m_data = &dummyInvalidData;
		}
		else if ( m_numLeft == 1 )
		{
			m_data = &self->m_gridForAreas[ y ][ x ].m_indexOrOffset;
		}
		else
		{
			m_data = &self->m_areasData[ self->m_gridForAreas[ y ][ x ].m_indexOrOffset ]; 
		}
	}
#endif // NO_GET_AGENTS_AREA

RED_INLINE CCrowdSpace_Grid::SGridCellForObstacles::Iterator::Iterator( const CCrowdSpace_Grid* self, Uint32 x, Uint32 y )
	: m_nextObstacles( self->m_obstaclesData.TypedData() )
	, m_currentObstacle( self->m_gridForObstacles[ y ][ x ].m_firstObstacle )
{
}

//------------------------------------------------------------------------------------------------------------------
// Sorted agents array
//------------------------------------------------------------------------------------------------------------------
CCrowdSpace_Grid::SSortedByDistanceInplaceAgentsArray::SSortedByDistanceInplaceAgentsArray( CCrowdManager* manager, const Vector2& pos, TAgentIndex max, TAgentIndex* array ) 
	: m_manager( manager )
	, m_array( array )
	, m_referencePosition( pos )
	, m_maxSize( max )
	, m_currSize( 0 )
{

}

RED_INLINE Bool CCrowdSpace_Grid::SSortedByDistanceInplaceAgentsArray::Insert( TAgentIndex agent )
{
	// calc this agent's dist
	const Float thisAgentsSqDist = ( m_referencePosition - m_manager->GetAgentPos2( agent ) ).SquareMag();

	// search for insertion place
	TAgentIndex whereToInsert( 0 );
	for ( ; whereToInsert < m_currSize; ++whereToInsert )
	{
		if ( m_array[ whereToInsert ] == agent )
		{
			// duplicate, not inserting!
			return false;
		}

		if ( ( m_referencePosition - m_manager->GetAgentPos2( m_array[ whereToInsert ] ) ).SquareMag() > thisAgentsSqDist )
		{
			// have it!
			break;
		}
	}

	// check if insertion place is not out of maximum array bounds
	if ( whereToInsert >= m_maxSize )
	{
		return false;
	}

	// move elements if needed
	for ( TAgentIndex i = min( m_currSize, m_maxSize - 1 ); i > whereToInsert; --i )
	{
		m_array[ i ] = m_array[ i - 1 ];
	}

	// insert!
	m_array[ whereToInsert ] = agent;

	// mark that size have increased (if it did...)
	m_currSize = min( m_currSize + 1, m_maxSize );

#if defined( DEBUG_CROWD ) && !defined( PROFILE_CROWD )
	// check the results
	// 1. check if size is ok
	ASSERT( m_currSize > 0 && m_currSize <= m_maxSize, TXT("Broken implementation of CCrowdSpace_Grid::SSortedByDistanceInplaceAgentsArray::Insert(), please FIX.") );

	// 2. Check if agents are in order
	for ( TAgentIndex i = 0; i < m_currSize - 1; ++i )
	{
		ASSERT( ( m_referencePosition - m_manager->GetAgentPos2( m_array[ i ] ) ).SquareMag() <= ( m_referencePosition - m_manager->GetAgentPos2( m_array[ i + 1 ] ) ).SquareMag(), 
			TXT("Broken implementation of CCrowdSpace_Grid::SSortedByDistanceInplaceAgentsArray::Insert(), please FIX.") );
	}

#endif

	// inserted!
	return true;
}

