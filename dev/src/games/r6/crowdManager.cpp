/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "crowdManager.h"
#include "crowdArea.h"
#include "crowdEntryPoint.h"
#include "../../common/engine/pathlibWorld.h"
#include "../../common/engine/renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CCrowdManager );

const Float	CCrowdManager::AGENT_RADIUS	= 0.4f;
const Float	CCrowdManager::AGENT_HEIGHT = 1.8f;
const Float CCrowdManager::GEOM_FETCH_DIST_SQ = 50.f * 50.f;

#ifdef PROXY_AGENT_TEST
	const Float	CCrowdManager::TIME_TO_SPAWN_FAKE_AGENTS = 5.0f;
#endif

CCrowdManager::CCrowdManager()
	: m_currentSpawnArea( 0 )
	, m_freeAgentIndex( 0 )
{
}

RED_INLINE void CCrowdManager::Reset()
{
	m_entyPoints.Clear( );
	m_areas.Clear( );
	m_currentSpawnArea = 0;
	m_freeAgentIndex = 0;
	m_crowdSpace.Reset();
	#ifdef DEBUG_CROWD
		m_crowdSpaceNaive.Reset();
	#endif
	m_obstacles.Clear();
	m_obstacles.Reserve( 10 * 1000 );										// 10k edges for now, we'll see how many do we need
	m_lastGeometryFetchLocation = Vector3( FLT_MAX, FLT_MAX, FLT_MAX );		// a very distant place ;)

#ifdef PROXY_AGENT_TEST
	m_timeToSpawnFakeAgents = 0;
#endif
}

void CCrowdManager::OnGameStart( const CGameInfo& gameInfo )
{
	Reset();
}

void CCrowdManager::OnWorldStart( const CGameInfo& gameInfo )
{	
	m_crowdSpace.OnInit( this );
	#ifdef DEBUG_CROWD
		m_crowdSpaceNaive.OnInit( this );
	#endif
}

void CCrowdManager::OnWorldEnd( const CGameInfo& gameInfo )
{
	for ( Uint32 i = 0; i < m_areas.Size(); ++i )
	{
		if( m_areas[ i ].Get() )
		{
			m_areas[ i ].Get()->ClearEntries();
		}
	}	
	Reset();
}

void CCrowdManager::RegisterEntryPoint( CCrowdEntryPoint* entryPoint )
{
	if ( entryPoint )
	{
		m_entyPoints.PushBackUnique( entryPoint );
		for ( Uint32 i = 0; i < m_areas.Size(); ++i )
		{
			if ( m_areas[ i ].Get() )
			{
				m_areas[ i ].Get()->AddIfInside( entryPoint );
			}
		}
	}
}

void CCrowdManager::RegisterArea( CCrowdArea* crowdArea )
{
	if ( crowdArea )
	{
		m_areas.PushBackUnique( crowdArea );
	}
}

void CCrowdManager::Tick( Float timeDelta )
{
	// build obstacles array (temporary solution)
	FetchGeometryIfNeeded();

	// spawn agents
	UpdateSpawning( timeDelta );
			
	// update crowd space before updating logic (it makes use of crowd space)
	m_crowdSpace.UpdateSpace();
	#ifdef DEBUG_CROWD
		m_crowdSpaceNaive.UpdateSpace();
	#endif

	// update logic
	UpdateLogic( timeDelta );

	// update positions after updating logic
	UpdatePosition( timeDelta );
	UpdateZPositions();
}

#ifdef PROXY_AGENT_TEST
void CCrowdManager::SpawnProxyAgentsIfNeeded( Float timeDelta )
{
	if ( m_areas.Empty() )
	{
		return;
	}

	m_timeToSpawnFakeAgents -= timeDelta;
	if ( m_timeToSpawnFakeAgents <= 0 )
	{
		// deactivate old proxy agents
		for ( TAgentIndex i = 0; i < m_freeAgentIndex; ++i )
		{
			if ( m_agents[ i ].IsProxy() )
			{
				m_agents[ i ].m_active = false;			
			}
		}		

		// activate new proxy agents
		TAgentIndex amountOfAgentsToSpawn = GEngine->GetRandomNumberGenerator().Get< TAgentIndex >( MAX_AMOUNT_OF_PROXY_AGENT );
		TAgentIndex current = 0;

		CCrowdArea* crowdArea = m_areas[ 0 ].Get();

		for ( TAgentIndex spawned = 0; spawned <= amountOfAgentsToSpawn && current < MAX_CROWD_AGENTS; ++current  )
		{
			if ( !m_agents[ current ].m_active )
			{
				m_agents[ current ].m_pos		= crowdArea->RandomPositionInside2();
				m_agents[ current ].m_proxy		= true;
				m_agents[ current ].m_active	= true;
				++spawned;
			}
		}
		
		if( current >= m_freeAgentIndex )
		{
			m_freeAgentIndex = current + 1;
		}

		m_timeToSpawnFakeAgents = TIME_TO_SPAWN_FAKE_AGENTS;
	}
}
#endif

RED_INLINE void CCrowdManager::UpdateSpawning( Float timeDelta )
{
#ifdef PROXY_AGENT_TEST
	SpawnProxyAgentsIfNeeded( timeDelta );
#endif
	if ( m_currentSpawnArea < m_areas.SizeInt() )
	{
		SpawnAgents( m_currentSpawnArea );
		++m_currentSpawnArea;
	}
	else
	{
		m_currentSpawnArea = 0;
	}
}

RED_INLINE void CCrowdManager::SpawnAgents( TAreaIndex areaIndex )
{
	CCrowdArea* crowdArea = GetAreaByIndex( areaIndex );
	if ( crowdArea->AreAllAgentsSpawned() )
	{
		return;
	}

	Int16 toSpawn		= Min< Int16 >( crowdArea->GetDesiredAmountOfAgents(), MAX_CROWD_AGENTS - m_freeAgentIndex );
	Int16 freeIndex		= m_freeAgentIndex;
	m_freeAgentIndex   += toSpawn;

	for ( TAgentIndex i = 0; i < toSpawn; ++i )
	{
		CCrowdEntryPoint* entryPoint	= crowdArea->RandomEntry();
		ASSERT( entryPoint, TXT("No entry point found") );
		CCrowdEntryPoint* exitPoint		= crowdArea->RandomEntryDifferent( entryPoint );
		ASSERT( exitPoint, TXT("No exit point found") );

		if ( entryPoint )
		{
			const Vector2 entryPosition2 = entryPoint->RandomPositionInside2();
			const Vector2 exitPosition2	= exitPoint ? exitPoint->RandomPositionInside2() : entryPoint->RandomPositionInside2();
	
			const TAgentIndex idx = freeIndex + i;
			SCrowdAgent& agent = m_agents[ idx ];
			agent.m_pos = entryPosition2;
			agent.m_tgt = exitPosition2;
			agent.AssignRandomVelToTarget();			
			agent.m_area = areaIndex; 
			agent.m_active = true;

			m_zPositions[ idx ] = entryPoint->GetWorldPositionRef().Z;		
	
			if ( i < AGRESSIVE_AGENT_COUNT )
			{
				m_agents[ idx ].m_collisionAvoidence = SCrowdAgent::MIN_COLLISION_AVOIDANCE;
			}
			else if ( i < AGRESSIVE_AGENT_COUNT + PAECEFUL_AGENT_COUNT )
			{
				m_agents[ idx ].m_collisionAvoidence = SCrowdAgent::PEACEFUL_COLLISION_AVOIDANCE;
			}
		}
	}

	crowdArea->MarkSpawned();
}

void CCrowdManager::OnGenerateDebugFragments( CRenderFrame* frame )
{
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_Crowd ) )
	{
		RenderAgents( frame );
		RenderGeometry( frame );
	}
}

RED_INLINE void CCrowdManager::RenderAgents( CRenderFrame *frame )
{			
	for ( Int16 i = 0; i < m_freeAgentIndex; ++i )
	{
		if( !m_agents[ i ].IsActive() )
		{
			continue;
		}

		const Vector2& agentPos = m_agents[ i ].m_pos;
		FixedCapsule capsule( Vector( agentPos.X, agentPos.Y, GetAgentZ( i ) + AGENT_HEIGHT * 0.5f ), CCrowdManager::AGENT_RADIUS, AGENT_HEIGHT );		
	
		if ( m_agents[ i ].IsProxy( ) )
		{		
			frame->AddDebugCapsule( capsule, Matrix::IDENTITY, Color::YELLOW );		
		}
		else if ( m_agents[ i ].m_collisionAvoidence >= SCrowdAgent::PEACEFUL_COLLISION_AVOIDANCE )
		{
			frame->AddDebugCapsule( capsule, Matrix::IDENTITY, Color::GREEN );		
		}
		else if ( m_agents[ i ].m_collisionAvoidence > SCrowdAgent::MIN_COLLISION_AVOIDANCE )
		{
			frame->AddDebugCapsule( capsule, Matrix::IDENTITY, Color::BLUE );		
		}		
		else
		{
			frame->AddDebugCapsule( capsule, Matrix::IDENTITY, Color::RED );		
		}
	}
}

RED_INLINE void CCrowdManager::RenderGeometry( CRenderFrame *frame )
{
	TObstacleIndex numObstacles = GetNumObstacles();
	for ( TObstacleIndex j = 0;  j < numObstacles; ++j )
	{
		if ( false == m_obstacles[ j ].IsValid() )
		{
			frame->AddDebugLineWithArrow( m_obstacles[ j ].m_begin, m_obstacles[ j ].m_end, 0.5f, 0.1f, 0.1f, Color::RED, true );
		}			
		else
		{
			frame->AddDebugLineWithArrow( m_obstacles[ j ].m_begin, m_obstacles[ m_obstacles[ j ].m_nextEdge ].m_begin, 0.5f, 0.1f, 0.1f, Color::BROWN, true );
		}
	}
}

RED_INLINE void CCrowdManager::UpdatePosition( Float timeDelta )
{
	// done in every frame for all agents
	for ( TAgentIndex i = 0; i < m_freeAgentIndex; ++i )
	{
		if ( !IsFinite( m_agents[ i ].m_newVel.X ) || !IsFinite( m_agents[ i ].m_newVel.Y ) )
		{	
			// almost never happpends		
			m_agents[ i ].m_newVel.X = 0.f;
			m_agents[ i ].m_newVel.Y = 0.f;
		}
		m_agents[ i ].m_vel = m_agents[ i ].m_newVel;
		m_agents[ i ].m_pos += m_agents[ i ].m_vel * timeDelta;		
	}	
}

void CCrowdManager::UpdateLogic( Float timeDelta )
{
	if ( GetNumAgents() < 1 )
	{
		return;
	}

	m_steeringImplementation.UpdateLOD1( m_agents, m_freeAgentIndex, this, timeDelta );	
}

RED_INLINE void CCrowdManager::FetchGeometryIfNeeded()
{
	// THIS SHOULD NOT BE BUILT IN RUNTIME
	// this is a temporary solution, just for the milestone.
	// final solution should be: editor prepares geometry, and it is being (async) streamed in runtime when needed.

	if ( ( m_lastGeometryFetchLocation - GetCenterPoint3() ).SquareMag() < GEOM_FETCH_DIST_SQ )
	{
		return;	
	}

	CWorld* world = GGame->GetActiveWorld();
	if ( nullptr == world )
	{
		return;
	}

	CPathLibWorld* pathLibWorld = world->GetPathLibWorld();
	if ( nullptr == pathLibWorld )
	{
		return;
	}

	#ifdef PROFILE_CROWD
	Double time;
	{
		Red::System::ScopedStopClock clk( time );
	#endif

		m_lastGeometryFetchLocation = GetCenterPoint3();
		m_obstacles.ClearFast();

		TObstacleIndex firstIndex = 0;
		TObstacleIndex lastIndex = -1;
		TDynArray< Vector > geometry;
		geometry.Reserve( 4096 );

		pathLibWorld->CollectGeometryAtLocation( GetCenterPoint3(), MAX_CROWD_DISTANCE, PathLib::CT_DEFAULT, geometry );
	
		for ( TObstacleIndex i = 0; i < geometry.SizeInt() - 1; i += 2 )
		{
			const Vector edge = geometry[ i + 1 ] - geometry[ i ];		
			const Int16 counter = static_cast< Int16 > ( ( edge.Mag3() - 0.001f ) / SCrowdObstacleEdge::MAX_EDGE_LEN );
			const Vector2 initPos = geometry[ i ];
			const Vector edgeDir = edge.Normalized3();

			for ( Int16 j = 0; j < counter; ++j )
			{
				// i have to cut it to smaller
				SCrowdObstacleEdge& obstacleEdge = NewObstacle();
				obstacleEdge.m_begin = initPos + edgeDir * static_cast< Float >( j ) * SCrowdObstacleEdge::MAX_EDGE_LEN;
				obstacleEdge.m_end = initPos + edgeDir * static_cast< Float >( j + 1 ) * SCrowdObstacleEdge::MAX_EDGE_LEN;
			}	

			SCrowdObstacleEdge& obstacleEdge = NewObstacle();
			obstacleEdge.m_begin = initPos + edgeDir * counter * SCrowdObstacleEdge::MAX_EDGE_LEN;
			obstacleEdge.m_end = geometry[ i + 1 ];	

			lastIndex += counter + 1;
		}

		static const Float VECT_EQ_EPS_SQR = 0.000001f * 0.000001f;
		for ( TObstacleIndex i = firstIndex; i <= lastIndex; ++i )
		{
			SCrowdObstacleEdge& obstacleI = GetObstacle( i );
			for ( TObstacleIndex j = firstIndex; j <= lastIndex; ++j )
			{
				SCrowdObstacleEdge& obstacleJ = GetObstacle( j );
				if ( i == j || ( IS_VALID_OBSTACLE_INDEX( obstacleJ.m_prevEdge ) ) )
				{
					continue;
				}

				if ( ( obstacleI.m_end - obstacleJ.m_begin ).SquareMag() < VECT_EQ_EPS_SQR )
				{
					obstacleI.m_nextEdge = j;
					obstacleJ.m_prevEdge = i;
					break;
				}
			}
		}

		for ( TObstacleIndex i = 0; i <= lastIndex; ++i )
		{
			GetObstacle( i ).Init( this );
			/*#ifdef DEBUG_CROWD
				ASSERT( GetObstacle( i ).IsValid(), TXT("invalid obstacle edge?") );
				ASSERT( false == GetObstacle( i ).m_begin.IsZero(), TXT("invalid obstacle edge?") );
				ASSERT( false == GetObstacle( i ).m_end.IsZero(), TXT("invalid obstacle edge?") );
			#endif*/
		}	

	#ifdef PROFILE_CROWD
	}
	RED_LOG( Crowd, TXT("Geometry fetched in %.3lf msec."), time * 1000.0 );
	#endif
}

Vector2 CCrowdManager::GetCenterPoint2() const
{
	// for now, the player
	const CEntity* ent = GGame->GetPlayerEntity();
	return ent ? ent->GetWorldPositionRef().AsVector2() : Vector::ZEROS.AsVector2();
}

Vector3 CCrowdManager::GetCenterPoint3() const
{
	// for now, the player
	const CEntity* ent = GGame->GetPlayerEntity();
	return ent ? ent->GetWorldPositionRef().AsVector3() : Vector::ZEROS.AsVector3();
}

CCrowdArea* CCrowdManager::GetAreaByIndex( TAreaIndex idx ) const
{
	return ( idx < 0 || idx >= GetNumAreas() ) ? nullptr : m_areas[ idx ].Get();
}


