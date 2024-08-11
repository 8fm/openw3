#include "build.h"
#include "physicsRagdollWrapper.h"
#include "../physics/physicsWorldPhysXImpl.h"

#ifdef USE_PHYSX
using namespace physx;
#endif

Float CPhysicsRagdollWrapper::GetMotionIntensity()
{
	Float maxForce = 0.0f;

#ifdef USE_PHYSX
	Uint32 actorCount = GetActorsCount();
	for( Uint32 i = 0; i != actorCount; ++i )
	{
		physx::PxActor* actor = ( physx::PxActor* ) GetActor( i );
		if( !actor ) continue;

		physx::PxRigidActor* rigidActor = actor->isRigidActor();
		if( !rigidActor ) continue;

		PxU32 constrainsCount = rigidActor->getNbConstraints();
		for( PxU32 i = 0; i != constrainsCount; ++i )
		{
			PxConstraint* constrain = 0;
			rigidActor->getConstraints( &constrain, 1, i );
			PxVec3 linear, angular;
			constrain->getForce( linear, angular );

			Float maglinear = linear.magnitude();
			Float magangular = angular.magnitude();

			if( maxForce < magangular )
			{
				maxForce = magangular;
			}

			if( maxForce < maglinear )
			{
				maxForce = maglinear;
			}
		}
	}
#endif

	return maxForce;
}