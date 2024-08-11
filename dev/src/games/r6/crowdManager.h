/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "crowdAgent.h"
#include "crowdSpaceImplementation.h"
#include "simpleCrowdSteering.h"
#include "crowdObstacle.h"

//#define CCrowdSpaceImplementation CCrowdSpace_Naive
//#define CSteeringImplementation CSimpleCrowdSteering

class CCrowdArea;
class CCrowdEntryPoint;

typedef CSimpleCrowdSteering CSteeringImpl;
class CCrowdDebuggerCanvas;

class CCrowdManager  : public IGameSystem
{
	DECLARE_ENGINE_CLASS( CCrowdManager, IGameSystem, 0 );

	// temporary
	friend class CCrowdDebuggerCanvas;

public:
	static const Float							AGENT_RADIUS;
	static const Float							AGENT_HEIGHT;

private:
	static const Float							AGENT_MAX_SPEED;
	static const Float							AGENT_MIN_SPEED;	
	static const TAgentIndex					AGRESSIVE_AGENT_COUNT	= 10;
	static const TAgentIndex					PAECEFUL_AGENT_COUNT	= 10;
	static const Float							GEOM_FETCH_DIST_SQ;


#ifdef PROXY_AGENT_TEST
	static const Float							TIME_TO_SPAWN_FAKE_AGENTS;
	static const TAgentIndex					MAX_AMOUNT_OF_PROXY_AGENT	= 10;
	Float										m_timeToSpawnFakeAgents;
#endif

	TDynArray< THandle< CCrowdEntryPoint > >	m_entyPoints;
	TDynArray< THandle< CCrowdArea	> >			m_areas;
	TDynArray< SCrowdObstacleEdge >				m_obstacles;
	CCrowdSpaceImpl								m_crowdSpace;

#ifdef DEBUG_CROWD
	CCrowdSpace_Naive							m_crowdSpaceNaive; // to verify the results of _Grid & _KdTree implementations
#endif // DEBUG_CROWD

	Vector3										m_lastGeometryFetchLocation;

	CSteeringImpl								m_steeringImplementation;

	TAreaIndex									m_currentSpawnArea;
	TAgentIndex									m_freeAgentIndex;

	SCrowdAgent									m_agents[ MAX_CROWD_AGENTS ];	
	//SCrowdAgent								m_proxyAgentsCreationQueue[ MAX_PROXY_CREATION_QUEUE ];	
	Float										m_zPositions[ MAX_CROWD_AGENTS ];

public:
												CCrowdManager();

private:
	// Update
	RED_INLINE void							Reset();
	RED_INLINE void							SpawnAgents( TAreaIndex areaIndex );
	RED_INLINE void							UpdateLogic( Float timeDelta );
	RED_INLINE void							UpdatePosition( Float timeDelta );
	void										UpdateZPositions();
	RED_INLINE void							RenderAgents( CRenderFrame *frame );
	RED_INLINE void							UpdateSpawning( Float timeDelta );
#ifdef PROXY_AGENT_TEST
				void							SpawnProxyAgentsIfNeeded( Float timeDelta );
#endif
private:
	// Geometry cache
	RED_INLINE void							FetchGeometryIfNeeded();
	RED_INLINE void							RenderGeometry( CRenderFrame *frame );
	


public:
	// Areas
	RED_INLINE TAreaIndex						GetNumAreas() const							{ return static_cast< TAreaIndex > ( m_areas.SizeInt() ); }
	CCrowdArea*									GetAreaByIndex( TAreaIndex idx ) const;
	Vector2										GetCenterPoint2() const;
	Vector3										GetCenterPoint3() const;
	void										RegisterEntryPoint( CCrowdEntryPoint* entryPoint );
	void										RegisterArea( CCrowdArea* crowdArea );

public:
	// Agents
	RED_INLINE TAgentIndex					GetNumAgents() const						{ return m_freeAgentIndex; }
	RED_INLINE Vector2						GetAgentPos2( TAgentIndex idx ) const		{ ASSERT( idx >= 0 && idx < MAX_CROWD_AGENTS ); return m_agents[ idx ].m_pos; }
	RED_INLINE Float							GetAgentZ( TAgentIndex idx ) const			{ return m_zPositions[ idx ]; }	
	RED_INLINE const SCrowdAgent*				GetAgentIn( TAgentIndex idx ) const			{ return &( m_agents[ idx ] ); }
	RED_INLINE Bool							IsAgentActive( TAgentIndex idx ) const		{ return m_agents[ idx ].m_active; }

public:
	// Obstacles
	RED_INLINE TObstacleIndex					NewObstacleIndex()							{ return static_cast< TObstacleIndex > ( m_obstacles.Grow( 1 ) ); }
	RED_INLINE SCrowdObstacleEdge&			NewObstacle()								{ return m_obstacles[ static_cast< Uint32 > ( m_obstacles.Grow( 1 ) ) ]; }
	RED_INLINE TObstacleIndex					GetNumObstacles() const						{ return static_cast< TObstacleIndex > ( m_obstacles.SizeInt() );  }
	RED_INLINE SCrowdObstacleEdge&			GetObstacle( TObstacleIndex idx )			{ return m_obstacles[ idx ]; }		
	RED_INLINE const SCrowdObstacleEdge&		GetObstacle( TObstacleIndex idx ) const		{ return m_obstacles[ idx ]; }	

public:
	// Crowd Space
	RED_INLINE const CCrowdSpaceImpl&			GetCrowdSpace() const						{ return m_crowdSpace; }
	#ifdef DEBUG_CROWD
		RED_INLINE const CCrowdSpace_Naive&	GetCrowdSpaceNaive_Debug() const			{ return m_crowdSpaceNaive; }
	#endif

public:
	// IGameSystem
	void										Tick( Float timeDelta ) override;
	void										OnGenerateDebugFragments( CRenderFrame* frame ) override;
	void										OnWorldStart( const CGameInfo& gameInfo ) override;
	void										OnWorldEnd( const CGameInfo& gameInfo ) override;
	void										OnGameStart( const CGameInfo& gameInfo ) override;

	ASSIGN_GAME_SYSTEM_ID( GS_CrowdManager );
};

BEGIN_CLASS_RTTI( CCrowdManager )
	PARENT_CLASS( IGameSystem );	
END_CLASS_RTTI();
