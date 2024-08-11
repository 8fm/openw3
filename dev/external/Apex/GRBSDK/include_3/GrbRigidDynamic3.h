#ifndef GRB_RIGID_DYNAMIC3_H
#define GRB_RIGID_DYNAMIC3_H

#include "PxVersionNumber.h"
#include "PxRigidDynamic.h"

namespace physx
{
//-----------------------------------------------------------------------------
#define RIGIDDYNAMIC_API_UNDEF( x )	PX_ASSERT( 0 && "PxRigidDynamic method not implemented in GRB: "##x )
//-----------------------------------------------------------------------------
class GrbRigidDynamic3 : public PxRigidDynamic
{
public:

#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR >= 3)
	GrbRigidDynamic3() : PxRigidDynamic(PxConcreteType::eRIGID_DYNAMIC, PxBaseFlag::eIS_RELEASABLE | PxBaseFlag::eOWNS_MEMORY) { }
#endif

	//---------------------------------------------------------------------------------
	// PxActor implementation
	//---------------------------------------------------------------------------------

	virtual		void						release() = 0;
	virtual		PxActorType::Enum			getType() const = 0;

	//---------------------------------------------------------------------------------
	// PxRigidActor implementation
	//---------------------------------------------------------------------------------

	// Pose
	virtual		void 						setGlobalPose(const PxTransform& pose, bool autowake) = 0;
	PX_FORCE_INLINE		PxTransform			getGlobalPoseFast() const { RIGIDDYNAMIC_API_UNDEF("getGlobalPoseFast"); }
	virtual		PxTransform					getGlobalPose() const = 0;

	//---------------------------------------------------------------------------------
	// PxRigidBody implementation
	//---------------------------------------------------------------------------------

	// Center of mass pose
	virtual		void						setCMassLocalPose(const PxTransform&) = 0;

	// Velocity
	virtual		PxVec3						getLinearVelocity() const = 0;
	virtual		void						setLinearVelocity(const PxVec3&, bool autowake) = 0;
	virtual		PxVec3						getAngularVelocity() const = 0;
	virtual		void						setAngularVelocity(const PxVec3&, bool autowake) = 0;

	// Force/Torque modifiers
	virtual		void						addForce(const PxVec3 &, PxForceMode::Enum mode, bool autowake) = 0;
#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR < 3)
	virtual		void						clearForce(PxForceMode::Enum /*mode*/, bool /*autowake*/) { RIGIDDYNAMIC_API_UNDEF("clearForce"); }
#else
	virtual		void						clearForce(PxForceMode::Enum /*mode*/ = PxForceMode::eFORCE) { RIGIDDYNAMIC_API_UNDEF("clearForce"); }
#endif
	virtual		void						addTorque(const PxVec3 &, PxForceMode::Enum mode, bool autowake) = 0;
#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR < 3)
	virtual		void						clearTorque(PxForceMode::Enum /*mode*/, bool /*autowake*/) { RIGIDDYNAMIC_API_UNDEF("clearTorque"); }
#else
	virtual		void						clearTorque(PxForceMode::Enum /*mode*/ = PxForceMode::eFORCE) { RIGIDDYNAMIC_API_UNDEF("clearTorque"); }
#endif

	//---------------------------------------------------------------------------------
	// PxRigidDynamic implementation
	//---------------------------------------------------------------------------------

	virtual		void						setKinematicTarget(const PxTransform& destination) = 0;
	virtual		bool						getKinematicTarget(PxTransform& target) = 0;

	// Damping
	virtual		void						setLinearDamping(PxReal) = 0;
	virtual		PxReal						getLinearDamping() const = 0;
	virtual		void						setAngularDamping(PxReal) = 0;
	virtual		PxReal						getAngularDamping() const = 0;

	// Velocity
	virtual		void						setMaxAngularVelocity(PxReal) = 0;
	virtual		PxReal						getMaxAngularVelocity() const { RIGIDDYNAMIC_API_UNDEF("getMaxAngularVelocity"); return 0.0f; }

	// Sleeping
	virtual		bool						isSleeping() const = 0;
	virtual		PxReal						getSleepThreshold() const = 0;
	virtual		void						setSleepThreshold(PxReal threshold) = 0;

#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR < 3)
	virtual		void						wakeUp(PxReal wakeCounterValue=PX_SLEEP_INTERVAL) = 0;
#else
	virtual		void						wakeUp() = 0;
	virtual		void						setWakeCounter(PxReal /*wakeCounterValue*/) { RIGIDDYNAMIC_API_UNDEF("setWakeCounter"); }
	virtual		PxReal						getWakeCounter() const { RIGIDDYNAMIC_API_UNDEF("getWakeCounter"); return 0.0f; }
#endif

	virtual		void						putToSleep() { RIGIDDYNAMIC_API_UNDEF("putToSleep"); }

	virtual		void						setSolverIterationCounts(PxU32 positionIters, PxU32 velocityIters) = 0;
	virtual		void						getSolverIterationCounts(PxU32 & /*positionIters*/, PxU32 & /*velocityIters*/) const { RIGIDDYNAMIC_API_UNDEF("getSolverIterationCounts"); }

	virtual		void						setContactReportThreshold(PxReal threshold) = 0;
	virtual		PxReal						getContactReportThreshold() const = 0;

#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR < 3)
	// Flags
	virtual		void						setRigidDynamicFlag(PxRigidDynamicFlag::Enum, bool value) = 0;
	virtual		void						setRigidDynamicFlags(PxRigidDynamicFlags inFlags) = 0;
	PX_FORCE_INLINE	PxRigidDynamicFlags		getRigidDynamicFlagsFast() const { RIGIDDYNAMIC_API_UNDEF("getRigidDynamicFlagsFast"); return PxRigidDynamicFlags(); }
	virtual		PxRigidDynamicFlags			getRigidDynamicFlags() const = 0;
#else
	virtual		void						setRigidBodyFlag(PxRigidBodyFlag::Enum, bool value) = 0;
	virtual		void						setRigidBodyFlags(PxRigidBodyFlags inFlags) = 0;
	virtual		PxRigidBodyFlags			getRigidBodyFlags() const = 0;
#endif

	// GRB specific
	virtual		PxReal						getSleepDamping() const = 0;
	virtual		void						setSleepDamping(PxReal damping) = 0;
};
//-----------------------------------------------------------------------------

}
#endif
