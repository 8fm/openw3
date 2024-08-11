/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#define PROXY_AGENT_TEST

#define DEBUG_CROWD
// #define PROFILE_CROWD

// Comment this out to enable methods like GetAgentsArea() in crowd space
// I had to remove them due to lack of optimal implementation (and I don't think those methods are even needed).
#define NO_GET_AGENTS_AREA

typedef Int16 TAgentIndex;
typedef Int16 TAreaIndex;
typedef Red::Threads::CAtomic< Int32 > TAtomicAgentIndex;
typedef Int32 TObstacleIndex;

#define INVALID_AGENT_INDEX TAgentIndex( -1 )
#define INVALID_AREA_INDEX TAreaIndex( -1 )
#define INVALID_OBSTACLE_INDEX TObstacleIndex( -1 )
#define IS_VALID_AGENT_INDEX( idx ) ( idx >= 0 )
#define IS_NOT_VALID_AGENT_INDEX( idx ) ( idx < 0 )
#define IS_VALID_AREA_INDEX( idx ) ( idx >= 0 )
#define IS_NOT_VALID_AREA_INDEX( idx ) ( idx < 0 )
#define IS_VALID_OBSTACLE_INDEX( idx ) ( idx >= 0 )
#define IS_NOT_VALID_OBSTACLE_INDEX( idx ) ( idx < 0 )

template< typename TVecType >
struct SCrowdRay
{
	TVecType	m_start;		// Ray start point
	TVecType	m_end;			// Ray end point
	Float		m_radius;		// Ray radius - 0.f by default, but can be higher (meaning: sweep test will be done instead of a raycast)

	SCrowdRay() : m_radius( 0.f ) {}
};

typedef SCrowdRay< Vector2 > SCrowdRay2;
typedef SCrowdRay< Vector3 > SCrowdRay3;

#define NUM_CROWD_PARALLEL_TASKS Int32( 5 )
#define MAX_CROWD_AGENTS 1000
#define MAX_PROXY_CREATION_QUEUE 20					    
#define MAX_CROWD_DISTANCE 200.f

#ifdef PROFILE_CROWD
	#define CROWD_PROFILE_SCOPE( name ) PC_SCOPE_PIX( name )
#else
	#define CROWD_PROFILE_SCOPE( name ) 
#endif