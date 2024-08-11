/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../physics/physicalCallbacks.h"

//////////////////////////////////////////////////////////////////////////////////////
/// Contact info - result of physical trace or collision, not all fileds may be set
struct SPhysicsContactInfo
{
	Vector						m_position;
	Vector						m_normal;
	Float						m_distance;
	SActorShapeIndex			m_rigidBodyIndexA;
	SActorShapeIndex			m_rigidBodyIndexB;
	CPhysicsWrapperInterface*	m_userDataA;
	CPhysicsWrapperInterface*	m_userDataB;
	Box*						m_actorVolume;
	Box*						m_shapeVolume;

	RED_INLINE SPhysicsContactInfo()
		: m_position( 0.0f ,0.0f ,0.0f )
		, m_normal( 0.0f ,0.0f ,0.0f )
		, m_distance( 0.f )
		, m_rigidBodyIndexA()
		, m_rigidBodyIndexB()
		, m_userDataA( NULL )
		, m_userDataB( NULL )
		, m_actorVolume( NULL )
		, m_shapeVolume( NULL )
	{
	}
};

//////////////////////////////////////////////////////////////////////////////////////
/// Overlap info - result of physical overlap
struct SPhysicsOverlapInfo
{
	CPhysicsWrapperInterface*	m_userData;
	SActorShapeIndex			m_actorNshapeIndex;
	Float						m_penetration;
	Vector						m_position;
	Box*						m_actorVolume;
	Box*						m_shapeVolume;

	RED_INLINE SPhysicsOverlapInfo()
		: m_userData( NULL )
		, m_actorNshapeIndex()
		, m_position( Vector::ZEROS )
		, m_actorVolume( NULL )
		, m_shapeVolume( NULL )
	{
	}
};
