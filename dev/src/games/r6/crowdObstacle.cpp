/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "crowdObstacle.h"
#include "crowdManager.h"

const Float SCrowdObstacleEdge::MAX_EDGE_LEN		= 1.0f;
const Float SCrowdObstacleEdge::MAX_EDGE_LEN_SQR	= SCrowdObstacleEdge::MAX_EDGE_LEN * SCrowdObstacleEdge::MAX_EDGE_LEN;


SCrowdObstacleEdge::SCrowdObstacleEdge()
	: m_prevEdge( INVALID_OBSTACLE_INDEX )
	, m_nextEdge( INVALID_OBSTACLE_INDEX )
#ifdef DEBUG_CROWD
	, m_begin( Vector::ZEROS.AsVector2() )
	, m_end( Vector::ZEROS.AsVector2() )
	, m_direction( Vector::ZEROS.AsVector2() )
#endif // DEBUG_CROWD

{
}

void SCrowdObstacleEdge::Init( CCrowdManager* mgr )
{
	if ( IS_VALID_OBSTACLE_INDEX( m_nextEdge ) )
	{
		m_direction = ( m_begin - mgr->GetObstacle( m_nextEdge ).m_begin  ).Normalized();
		m_end = mgr->GetObstacle( m_nextEdge ).m_begin;
	}
	else if ( IS_VALID_OBSTACLE_INDEX( m_prevEdge ) )
	{
		m_direction = ( mgr->GetObstacle( m_prevEdge ).m_begin - m_begin ).Normalized();
	}
	else
	{
		m_direction = m_begin - m_end;
	}
}

