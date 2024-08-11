#pragma once

#include "build.h"

struct SActorShapeIndex;
class CPhysicsWrapperInterface;

struct ICameraFadeOutOp
{
	virtual void operator()( CPhysicsWrapperInterface* physicsInterface, const SActorShapeIndex* actorIndex ) const = 0;
};

#ifdef USE_PHYSX
class CCustomCameraSweepHelper
{
public:
	static Bool SphereSweep( const Vector& from, const Vector& to, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, CPhysicsWorld * physicsWorld, ICameraFadeOutOp* fadeOutOperation );
	static Bool BoxOverlap( const Vector& position, const Vector3& halfExtents, const Matrix& orientation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, CPhysicsWorld * physicsWorld, ICameraFadeOutOp* fadeOutOperation );
};
#endif