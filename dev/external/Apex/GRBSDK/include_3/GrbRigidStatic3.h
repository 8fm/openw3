#ifndef GRB_RIGID_STATIC3_H
#define GRB_RIGID_STATIC3_H

#include "PxVersionNumber.h"
#include "PxRigidBody.h"
#include "PxRigidStatic.h"

namespace physx
{
//-----------------------------------------------------------------------------
class GrbRigidStatic3 : public PxRigidStatic
{
public:

#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR >= 3)
	GrbRigidStatic3() : PxRigidStatic(PxConcreteType::eRIGID_STATIC, PxBaseFlag::eIS_RELEASABLE | PxBaseFlag::eOWNS_MEMORY) { }
#endif

	//---------------------------------------------------------------------------------
	// PxActor implementation
	//---------------------------------------------------------------------------------
	virtual			void					release() = 0;

	virtual			PxActorType::Enum		getType() const = 0;

	//---------------------------------------------------------------------------------
	// PxRigidActor implementation
	//---------------------------------------------------------------------------------

	// Pose
	virtual			void 					setGlobalPose(const PxTransform& pose, bool wake) = 0;
	virtual			PxTransform				getGlobalPose() const = 0;
};
//-----------------------------------------------------------------------------
}
#endif