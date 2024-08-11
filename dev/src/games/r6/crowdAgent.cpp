/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "crowdAgent.h"

const Float SCrowdAgent::AGENT_MAX_SPEED				= 3.0f;
const Float SCrowdAgent::AGENT_MIN_SPEED				= 1.5f;
const Float SCrowdAgent::MIN_COLLISION_AVOIDANCE		= 0.01f;	// it is not 0, because only proxy agents ( fake agents ) can have 0 
const Float SCrowdAgent::PEACEFUL_COLLISION_AVOIDANCE	= 1.f;		// it is not 0, because only proxy agents ( fake agents ) can have 0 

SCrowdAgent::SCrowdAgent()
	: m_collisionAvoidence( 0.5f )
	, m_active( false )
	, m_proxy( false )
{

}

void SCrowdAgent::AssignRandomVelToTarget()
{
	const Float speed = GEngine->GetRandomNumberGenerator().Get< Float >( AGENT_MIN_SPEED , AGENT_MAX_SPEED );
	Vector2 vel = m_tgt - m_pos;
	vel.Normalize();
	vel *= speed;
	m_vel = vel;
	m_newVel = vel;
	m_prefVelocity = vel;
}