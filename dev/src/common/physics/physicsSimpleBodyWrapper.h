/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once


#ifndef PHYSICS_BODY_SIMPLE_WRAPPER_H
#define PHYSICS_BODY_SIMPLE_WRAPPER_H

#include "../physics/physicsEngine.h"
#include "../physics/physicalCallbacks.h"
#include "../physics/physicalCollision.h"
#include "../physics/physicsWrapperPool.h"
#include "../physics/physicsWrapper.h"

class CPhysicsSimpleWrapper : public CPhysicsWrapperInterface
{
	friend class TWrappersPool< CPhysicsSimpleWrapper, SWrapperContext >;

	Vector							m_scale;
	void*							m_rigidDynamicActor;
	Float							m_boundingDimensions;
	Float							m_occlusionAttenuation;
	Float							m_occlusionDiameterLimit;
	Matrix							m_componentOffsetInverse;
	Float							m_linearVelocityClamp;
	Float							m_floatingRatio;
	RedQuaternion					m_swimmingRotationQuaternion;
	CompiledCollisionPtr			m_compiledCollision;
	Bool							m_dontUpdateBouyancy;

protected:
	CPhysicsSimpleWrapper( CPhysicalCollision collisionTypeAndGroup, CompiledCollisionPtr& compiledCollision );
	CPhysicsSimpleWrapper( CPhysicsEngine::CollisionMask collisionTypeMask, CPhysicsEngine::CollisionMask collisionGroupMask, CompiledCollisionPtr& compiledCollision, const Vector* boxDimensions, Float* sphereRadius, const Matrix& localOffset = Matrix::IDENTITY );
	virtual ~CPhysicsSimpleWrapper();

	Bool MakeReadyToDestroy( TDynArray< void* >* toRemove );
	void PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray<void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd );
	virtual void PostSimulationUpdateTransform( const Matrix& transform, void* actor );

	Bool SetOcclusionParameters( Uint32 actorIndex = 0, Float diagonalLimit = 1, Float attenuation = -1 );

	virtual Bool UpdatesEntity() { return true; }

public:
	virtual void Release( Uint32 actorIndex = 0 );

	virtual Bool IsReady() const;

	virtual void SetPose( const Matrix& localToWorld, Uint32 actorIndex = 0 );
	virtual Matrix GetPose( Uint32 actorIndex = 0 ) const;

	virtual void SetFlag( EPhysicsRigidBodyWrapperFlags flag, Bool decision );

	virtual Uint32 GetActorsCount() const;
	virtual void* GetActor( Uint32 actorIndex = 0 ) const;

	virtual Int32 GetIndex( const char* actorName );

    void SetLinearVelocityClamp( Float maxLinearVelocity );

	Bool GetOcclusionParameters( Uint32 actorIndex = 0, Float* diagonalLimit = 0, Float* attenuation = 0 );
};

#endif
