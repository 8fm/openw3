/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "crowd.h"

struct SCrowdAgent
{
	static const Float AGENT_MAX_SPEED;
	static const Float AGENT_MIN_SPEED;
	static const Float MIN_COLLISION_AVOIDANCE;
	static const Float PEACEFUL_COLLISION_AVOIDANCE;

	SCrowdAgent();

	Vector2			m_pos;					// 8
	Vector2			m_vel;					// 8
	Vector2			m_newVel;				// 8
	Vector2			m_tgt;					// 8
	Vector2			m_prefVelocity;			// 8
	Float			m_collisionAvoidence;	// 4
	TAreaIndex		m_area;					// 2
											// ...
	// One-bit flags						// 1 ( all flags )
	Bool			m_active	: 1;		// ...					// not active agent will be replaced when new agent needs to be spawn 
	Bool			m_proxy		: 1;		// ...					// proxy agent is not simulated by collision Solver. All other agent avoids him.
	Int16						: 0;		// 1 ( padding )
											// -------------
											// 48 bytes											

	RED_INLINE	Bool	IsSimulated()	const	{ return m_active && !m_proxy; }
	RED_INLINE	Bool	IsActive()		const	{ return m_active; }	
	RED_INLINE	Bool	IsProxy()		const	{ return m_proxy; }		


	RED_INLINE	Float	DistanceToTargetSqr() const { return ( m_pos - m_tgt ).SquareMag(); }
	RED_INLINE	Float	ComputeCollisionAvoidence( const SCrowdAgent* other ) const{ return other->m_proxy ? 1 : m_collisionAvoidence / ( m_collisionAvoidence + other->m_collisionAvoidence ); }
					void	AssignRandomVelToTarget();
};
