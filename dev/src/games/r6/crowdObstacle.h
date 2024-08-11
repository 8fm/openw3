/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "crowd.h"

class CCrowdManager;

struct SCrowdObstacleEdge
{	
	static const Float MAX_EDGE_LEN;
	static const Float MAX_EDGE_LEN_SQR;

	SCrowdObstacleEdge();

	Vector2				m_begin;	
	Vector2				m_end;	
	Vector2				m_direction;

	Float				m_height;		

	TObstacleIndex		m_prevEdge;		
	TObstacleIndex		m_nextEdge;		
	
	RED_INLINE Vector2 GetDirection() const	{ return m_direction; }
	RED_INLINE Bool IsValid() const			{ return IS_VALID_OBSTACLE_INDEX( m_prevEdge ) && IS_VALID_OBSTACLE_INDEX( m_nextEdge ); }
	RED_INLINE Bool IsConvex() const			{ return true; } // temp

	void Init( CCrowdManager* mgr );
};
