// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.

#ifndef __MODULE_DESTRUCTIBLE_H__
#define __MODULE_DESTRUCTIBLE_H__

#include "NxApex.h"
#include "NiApexSDK.h"
#include "NiModule.h"
#include "Module.h"

#include "NxModuleDestructible.h"
#if NX_SDK_VERSION_MAJOR == 2
#include "NxUserContactReport.h"
#elif NX_SDK_VERSION_MAJOR == 3

#endif
#include "NxDestructibleActorJoint.h"
#include "DestructibleAsset.h"
#include "PsMutex.h"
#include "ApexAuthorableObject.h"

#include "ApexSDKCachedData.h"

#include "ApexRand.h"

#include "DestructibleParamClasses.h"
#ifndef USE_DESTRUCTIBLE_RWLOCK
#define USE_DESTRUCTIBLE_RWLOCK 0
#endif

#define DECLARE_DISABLE_COPY_AND_ASSIGN(Class) private: Class(const Class &); Class & operator = (const Class &);

#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2
#include "NxScene.h"

/* === Wrapper class to restrict the GRB features the user has access to when creating a GRB with createTwoWayRb() === */

#define GRB_API_RESTRICT(x)	NX_ASSERT(0 && "NxActor method not supported for actors created with createTwoWayRb(): " x)

class GrbActorExternal : public NxActor, public physx::UserAllocated
{
public:

	GrbActorExternal(NxActor* actor) : actor(actor) {}

	virtual		NxScene&			getScene()	const
	{
		GRB_API_RESTRICT("getScene");
		static char dummy[sizeof(NxScene)];
		return *(reinterpret_cast<NxScene*>(dummy));
	};
	virtual		void				saveToDesc(NxActorDescBase&)
	{
		GRB_API_RESTRICT("saveToDesc");
	}
	virtual		void				setName(const char*)
	{
		GRB_API_RESTRICT("setName");
	}
	virtual		const char*			getName()			const
	{
		GRB_API_RESTRICT("getName");
		return NULL;
	}
	virtual		void				setGlobalPose(const NxMat34& mat)
	{
		actor->setGlobalPose(mat);
	}
	virtual		void				setGlobalPosition(const NxVec3& v)
	{
		actor->setGlobalPosition(v);
	}
	virtual		void				setGlobalOrientation(const NxMat33& m)
	{
		actor->setGlobalOrientation(m);
	}
	virtual		void				setGlobalOrientationQuat(const NxQuat& q)
	{
		actor->setGlobalOrientationQuat(q);
	}
	virtual		NxMat34 			getGlobalPose() const
	{
		return actor->getGlobalPose();
	}
	virtual		NxVec3 				getGlobalPosition() const
	{
		return actor->getGlobalPosition();
	}
	virtual		NxMat33 			getGlobalOrientation() const
	{
		return actor->getGlobalOrientation();
	}
	virtual		NxQuat 				getGlobalOrientationQuat()const
	{
		return actor->getGlobalOrientationQuat();
	}
	virtual		void				moveGlobalPose(const NxMat34&)
	{
		GRB_API_RESTRICT("moveGlobalPose");
	}
	virtual		void				moveGlobalPosition(const NxVec3&)
	{
		GRB_API_RESTRICT("moveGlobalPosition");
	}
	virtual		void				moveGlobalOrientation(const NxMat33&)
	{
		GRB_API_RESTRICT("moveGlobalOrientation");
	}
	virtual		void				moveGlobalOrientationQuat(const NxQuat&)
	{
		GRB_API_RESTRICT("moveGlobalOrientationQuat");
	}
	virtual		NxShape*			createShape(const NxShapeDesc&)
	{
		GRB_API_RESTRICT("createShape");
		return NULL;
	}
	virtual		void				releaseShape(NxShape&)
	{
		GRB_API_RESTRICT("releaseShape");
	}
	virtual		NxU32				getNbShapes() const
	{
		GRB_API_RESTRICT("getNbShapes");
		return 0;
	}
	virtual		NxShape* const* 		getShapes() const
	{
		GRB_API_RESTRICT("getShapes");
		return 0;
	}
	virtual		void				setGroup(NxActorGroup)
	{
		GRB_API_RESTRICT("setGroup");
	}
	virtual		NxActorGroup		getGroup() const
	{
		GRB_API_RESTRICT("getGroup");
		return (NxActorGroup)0;
	}
	virtual		void				setDominanceGroup(NxDominanceGroup)
	{
		GRB_API_RESTRICT("setDominanceGroup");
	}
	virtual		NxDominanceGroup	getDominanceGroup() const
	{
		GRB_API_RESTRICT("getDominanceGroup");
		return (NxDominanceGroup)0;
	}
	virtual		void				raiseActorFlag(NxActorFlag)
	{
		GRB_API_RESTRICT("raiseActorFlag");
	}
	virtual		void				clearActorFlag(NxActorFlag)
	{
		GRB_API_RESTRICT("clearActorFlag");
	}
	virtual		bool				readActorFlag(NxActorFlag)	const
	{
		GRB_API_RESTRICT("readActorFlag");
		return false;
	}
	virtual		void				resetUserActorPairFiltering()
	{
		GRB_API_RESTRICT("resetUserActorPairFiltering");
	}
	virtual		bool				isDynamic()	const
	{
		GRB_API_RESTRICT("isDynamic");
		return false;
	}
	virtual		void				setCMassOffsetLocalPose(const NxMat34&)
	{
		GRB_API_RESTRICT("setCMassOffsetLocalPose");
	}
	virtual		void				setCMassOffsetLocalPosition(const NxVec3&)
	{
		GRB_API_RESTRICT("setCMassOffsetLocalPosition");
	}
	virtual		void				setCMassOffsetLocalOrientation(const NxMat33&)
	{
		GRB_API_RESTRICT("setCMassOffsetLocalOrientation");
	}
	virtual		void				setCMassOffsetGlobalPose(const NxMat34&)
	{
		GRB_API_RESTRICT("setCMassOffsetGlobalPose");
	}
	virtual		void				setCMassOffsetGlobalPosition(const NxVec3&)
	{
		GRB_API_RESTRICT("setCMassOffsetGlobalPosition");
	}
	virtual		void				setCMassOffsetGlobalOrientation(const NxMat33&)
	{
		GRB_API_RESTRICT("setCMassOffsetGlobalOrientation");
	}
	virtual		void				setCMassGlobalPose(const NxMat34&)
	{
		GRB_API_RESTRICT("setCMassGlobalPose");
	}
	virtual		void				setCMassGlobalPosition(const NxVec3&)
	{
		GRB_API_RESTRICT("setCMassGlobalPosition");
	}
	virtual		void				setCMassGlobalOrientation(const NxMat33&)
	{
		GRB_API_RESTRICT("setCMassGlobalOrientation");
	}
	virtual		NxMat34 			getCMassLocalPose() const
	{
		GRB_API_RESTRICT("getCMassLocalPose");
		return NxMat34();
	}
	virtual		NxVec3 				getCMassLocalPosition() const
	{
		GRB_API_RESTRICT("getCMassLocalPosition");
		return NxVec3(0.0f);
	}
	virtual		NxMat33 			getCMassLocalOrientation() const
	{
		GRB_API_RESTRICT("getCMassLocalOrientation");
		return NxMat33(NX_IDENTITY_MATRIX);
	}
	virtual		NxMat34 			getCMassGlobalPose() const
	{
		GRB_API_RESTRICT("getCMassGlobalPose");
		return NxMat34();
	}
	virtual		NxVec3 				getCMassGlobalPosition() const
	{
		GRB_API_RESTRICT("getCMassGlobalPosition");
		return NxVec3();
	}
	virtual		NxMat33 			getCMassGlobalOrientation() const
	{
		GRB_API_RESTRICT("getCMassGlobalOrientation");
		return NxMat33(NX_IDENTITY_MATRIX);
	}
	virtual		void				setMass(NxReal)
	{
		GRB_API_RESTRICT("setMass");
	}
	virtual		NxReal				getMass() const
	{
		GRB_API_RESTRICT("getMass");
		return 0.0f;
	}
	virtual		void				setMassSpaceInertiaTensor(const NxVec3&)
	{
		GRB_API_RESTRICT("setMassSpaceInertiaTensor");
	}
	virtual		NxVec3				getMassSpaceInertiaTensor() const
	{
		GRB_API_RESTRICT("getMassSpaceInertiaTensor");
		return NxVec3();
	}
	virtual		NxMat33				getGlobalInertiaTensor() const
	{
		GRB_API_RESTRICT("getGlobalInertiaTensor");
		return NxMat33(NX_IDENTITY_MATRIX);
	}
	virtual		NxMat33				getGlobalInertiaTensorInverse() const
	{
		GRB_API_RESTRICT("getGlobalInertiaTensorInverse");
		return NxMat33(NX_IDENTITY_MATRIX);
	}
	virtual		bool				updateMassFromShapes(NxReal, NxReal)
	{
		GRB_API_RESTRICT("updateMassFromShapes");
		return false;
	}
	virtual		void				setLinearDamping(NxReal)
	{
		GRB_API_RESTRICT("setLinearDamping");
	}
	virtual		NxReal				getLinearDamping() const
	{
		GRB_API_RESTRICT("getLinearDamping");
		return 0.0f;
	}
	virtual		void				setAngularDamping(NxReal)
	{
		GRB_API_RESTRICT("setAngularDamping");
	}
	virtual		NxReal				getAngularDamping() const
	{
		GRB_API_RESTRICT("getAngularDamping");
		return 0.0f;
	}
	virtual		void				setLinearVelocity(const NxVec3& linVel)
	{
		actor->setLinearVelocity(linVel);
	}
	virtual		void				setAngularVelocity(const NxVec3& angVel)
	{
		actor->setAngularVelocity(angVel);
	}
	virtual		NxVec3				getLinearVelocity() const
	{
		GRB_API_RESTRICT("getLinearVelocity");
		return NxVec3();
	}
	virtual		NxVec3				getAngularVelocity() const
	{
		GRB_API_RESTRICT("getAngularVelocity");
		return NxVec3();
	}
	virtual		void				setMaxAngularVelocity(NxReal)
	{
		GRB_API_RESTRICT("setMaxAngularVelocity");
	}
	virtual		NxReal				getMaxAngularVelocity()	const
	{
		GRB_API_RESTRICT("getMaxAngularVelocity");
		return 0.0f;
	}
	virtual		void				setCCDMotionThreshold(NxReal)
	{
		GRB_API_RESTRICT("setCCDMotionThreshold");
	}
	virtual		NxReal				getCCDMotionThreshold()	const
	{
		GRB_API_RESTRICT("getCCDMotionThreshold");
		return 0.0f;
	}
	virtual		void				setLinearMomentum(const NxVec3&)
	{
		GRB_API_RESTRICT("setLinearMomentum");
	}
	virtual		void				setAngularMomentum(const NxVec3&)
	{
		GRB_API_RESTRICT("setAngularMomentum");
	}
	virtual		NxVec3				getLinearMomentum() const
	{
		GRB_API_RESTRICT("getLinearMomentum");
		return NxVec3(0.0f);
	}
	virtual		NxVec3				getAngularMomentum() const
	{
		GRB_API_RESTRICT("getAngularMomentum");
		return NxVec3(0.0f);
	}
	virtual		void				addForceAtPos(const NxVec3&, const NxVec3&, NxForceMode, bool)
	{
		GRB_API_RESTRICT("addForceAtPos");
	}
	virtual		void				addForceAtLocalPos(const NxVec3&, const NxVec3&, NxForceMode, bool)
	{
		GRB_API_RESTRICT("addForceAtLocalPos");
	}
	virtual		void				addLocalForceAtPos(const NxVec3&, const NxVec3&, NxForceMode, bool)
	{
		GRB_API_RESTRICT("addLocalForceAtPos");
	}
	virtual		void				addLocalForceAtLocalPos(const NxVec3&, const NxVec3&, NxForceMode, bool)
	{
		GRB_API_RESTRICT("addLocalForceAtLocalPos");
	}
	virtual		void				addForce(const NxVec3&, NxForceMode, bool)
	{
		GRB_API_RESTRICT("addForce");
	}
	virtual		void				addLocalForce(const NxVec3&, NxForceMode, bool)
	{
		GRB_API_RESTRICT("addLocalForce");
	}
	virtual		void				addTorque(const NxVec3&, NxForceMode, bool)
	{
		GRB_API_RESTRICT("addTorque");
	}
	virtual		void				addLocalTorque(const NxVec3&, NxForceMode, bool)
	{
		GRB_API_RESTRICT("addLocalTorque");
	}
	virtual		NxReal				computeKineticEnergy() const
	{
		GRB_API_RESTRICT("computeKineticEnergy");
		return 0.0f;
	}
	virtual		NxVec3				getPointVelocity(const NxVec3&)	const
	{
		GRB_API_RESTRICT("getPointVelocity");
		return NxVec3(0.0f);
	}
	virtual		NxVec3				getLocalPointVelocity(const NxVec3&)	const
	{
		GRB_API_RESTRICT("getLocalPointVelocity");
		return NxVec3(0.0f);
	}
	virtual		bool				isGroupSleeping() const
	{
		GRB_API_RESTRICT("isGroupSleeping");
		return false;
	}
	virtual		bool				isSleeping() const
	{
		return actor->isSleeping();
	}
	virtual		NxReal				getSleepLinearVelocity() const
	{
		GRB_API_RESTRICT("getSleepLinearVelocity");
		return 0.0f;
	}
	virtual		void				setSleepLinearVelocity(NxReal)
	{
		GRB_API_RESTRICT("setSleepLinearVelocity");
	}
	virtual		NxReal				getSleepAngularVelocity() const
	{
		GRB_API_RESTRICT("getSleepAngularVelocity");
		return 0.0f;
	}
	virtual		void				setSleepAngularVelocity(NxReal)
	{
		GRB_API_RESTRICT("setSleepAngularVelocity");
	}
	virtual		NxReal				getSleepEnergyThreshold() const
	{
		GRB_API_RESTRICT("getSleepEnergyThreshold");
		return 0.0f;
	}
	virtual		void				setSleepEnergyThreshold(NxReal)
	{
		GRB_API_RESTRICT("setSleepEnergyThreshold");
	}
	virtual		void				wakeUp(NxReal)
	{
		GRB_API_RESTRICT("wakeCounterValue");
	}
	virtual		void				putToSleep()
	{
		GRB_API_RESTRICT("putToSleep");
	}
	virtual		void				raiseBodyFlag(NxBodyFlag)
	{
		GRB_API_RESTRICT("raiseBodyFlag");
	}
	virtual		void				clearBodyFlag(NxBodyFlag)
	{
		GRB_API_RESTRICT("clearBodyFlag");
	}
	virtual		bool				readBodyFlag(NxBodyFlag flag) const
	{
		PX_UNUSED(flag);
		GRB_API_RESTRICT("readBodyFlag");
		return 0;
	}
	virtual		bool				saveBodyToDesc(NxBodyDesc&)
	{
		GRB_API_RESTRICT("saveBodyToDesc");
		return false;
	}
	virtual		void				setSolverIterationCount(NxU32)
	{
		GRB_API_RESTRICT("setSolverIterationCount");
	}
	virtual		NxU32				getSolverIterationCount() const
	{
		GRB_API_RESTRICT("getSolverIterationCount");
		return 0;
	}
	virtual		NxReal				getContactReportThreshold() const
	{
		GRB_API_RESTRICT("getContactReportThreshold");
		return 0.0f;
	}
	virtual		void				setContactReportThreshold(NxReal)
	{
		GRB_API_RESTRICT("setContactReportThreshold");
	}
	virtual		NxU32				getContactReportFlags() const
	{
		GRB_API_RESTRICT("getContactReportFlags");
		return 0;
	}
	virtual		void				setContactReportFlags(NxU32)
	{
		GRB_API_RESTRICT("setContactReportFlags");
	}
	virtual		NxU32				linearSweep(const NxVec3&, NxU32, void*, NxU32, NxSweepQueryHit*, NxUserEntityReport<NxSweepQueryHit>*, const NxSweepCache*)
	{
		GRB_API_RESTRICT("linearSweep");
		return 0;
	}
	virtual		NxCompartment* 		getCompartment() const
	{
		GRB_API_RESTRICT("getCompartment");
		return NULL;
	}
	virtual		NxForceFieldMaterial getForceFieldMaterial() const
	{
		GRB_API_RESTRICT("getForceFieldMaterial");
		return (NxForceFieldMaterial)0;
	}
	virtual		void				setForceFieldMaterial(NxForceFieldMaterial)
	{
		GRB_API_RESTRICT("setForceFieldMaterial");
	}

	NxActor* actor;
};

#elif APEX_USE_GRB && NX_SDK_VERSION_MAJOR == 3

#include "PxScene.h"
#include "PxBase.h"

/* === Wrapper class to restrict the GRB features the user has access to when creating a GRB with createTwoWayRb() === */

#define GRB_API_RESTRICT(x)	PX_ASSERT(0 && "PxActor method not supported for actors created with createTwoWayRb(): " x)

using physx::PxU32;
using physx::PxReal;
using physx::PxVec3;

class GrbActorExternal : public physx::PxRigidDynamic, public physx::shdfnd::UserAllocated
{
public:

	GrbActorExternal(PxRigidDynamic* actor, physx::PxScene* s)
	: PxRigidDynamic(physx::PxBaseFlag::eIS_RELEASABLE | physx::PxBaseFlag::eOWNS_MEMORY)
	, actor(actor)
	, scene(s)
	{++refCount;}

	virtual		PxU32						getObjectSize()										const
	{
		return sizeof(*this);
	}

	virtual		physx::PxScene*			getScene()  const
	{
		GRB_API_RESTRICT("getScene");
		return (physx::PxScene *) 0;
	}

	virtual		void				setName(const char*)
	{
		GRB_API_RESTRICT("setName");
	}
	virtual		const char*			getName()			const
	{
		GRB_API_RESTRICT("getName");
		return NULL;
	}
	virtual		void			setGlobalPose(const physx::PxTransform& pose, bool wake = true)
	{
		actor->setGlobalPose(pose, wake);
	}
	virtual		physx::PxTransform 			getGlobalPose() const
	{
		return actor->getGlobalPose();
	}

	virtual		physx::PxReal				getInvMass(void) const
	{
		GRB_API_RESTRICT("getInvMass");
		return 0.0f;
	}

	virtual		PxVec3			getMassSpaceInvInertiaTensor()			const
	{
		GRB_API_RESTRICT("getMassSpaceInvInertiaTensor");
		return PxVec3();
	}

	virtual		physx::PxShape*		createShape(const physx::PxGeometry& geometry, physx::PxMaterial*const* materials, physx::PxU16 materialCount, physx::PxShapeFlags shapeFlags = physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE);

	virtual		void				attachShape(physx::PxShape& shape)
	{
		actor->attachShape(shape);
	}

	virtual		void				detachShape(physx::PxShape& shape, bool wakeOnLostTouch = true)
	{
		actor->detachShape(shape, wakeOnLostTouch);
	}

	virtual		PxU32				getNbShapes() const
	{
		return actor->getNbShapes();
	}
	virtual		PxU32			getShapes(physx::PxShape** userBuffer, PxU32 bufferSize, PxU32 startIndex=0)			const
	{
		return actor->getShapes(userBuffer, bufferSize, startIndex);
	}

	virtual		void			setCMassLocalPose(const physx::PxTransform& pose) 
	{
		actor->setCMassLocalPose(pose);
	}

	virtual		physx::PxTransform 	getCMassLocalPose() const
	{
		GRB_API_RESTRICT("getCMassLocalPose");
		return physx::PxTransform();
	}

	virtual		void			setMass(PxReal mass)
	{
		actor->setMass(mass);
	}

	virtual		PxReal			getMass() const
	{
		GRB_API_RESTRICT("getMass");
		return 0.0f;
	}

	virtual		void			setMassSpaceInertiaTensor(const PxVec3& m)
	{
		actor->setMassSpaceInertiaTensor(m);
	}

	virtual		PxVec3			getMassSpaceInertiaTensor()			const
	{
		GRB_API_RESTRICT("getMassSpaceInertiaTensor");
		return PxVec3();
	}

	virtual		void				setLinearDamping(PxReal )
	{
		GRB_API_RESTRICT("setLinearDamping");
	}

	virtual		PxReal				getLinearDamping() const 
	{
		GRB_API_RESTRICT("getLinearDamping");
		return 0.0f;
	}

	virtual		void				setAngularDamping(PxReal angDamp)
	{
		actor->setAngularDamping(angDamp);
	}

	virtual		PxReal				getAngularDamping() const
	{
		GRB_API_RESTRICT("getAngularDamping");
		return 0.0f;
	}

	virtual		PxVec3			getLinearVelocity()		const
	{
		GRB_API_RESTRICT("getLinearVelocity");
		return PxVec3();
	}

	virtual		void			setLinearVelocity(const PxVec3& linVel, bool autowake = true )
	{
		actor->setLinearVelocity(linVel, autowake);
	}

	virtual		PxVec3			getAngularVelocity()	const
	{
		GRB_API_RESTRICT("getAngularVelocity");
		return PxVec3();
	}

	virtual		void			setAngularVelocity(const PxVec3& angVel, bool autowake = true )
	{
		actor->setAngularVelocity(angVel, autowake);
	}

	virtual		void				setMaxAngularVelocity(PxReal )
	{
		GRB_API_RESTRICT("setMaxAngularVelocity");
	}
	virtual		PxReal				getMaxAngularVelocity()	const 
	{
		GRB_API_RESTRICT("getMaxAngularVelocity");
		return 0.0f;
	}

	virtual		void			addForce(const PxVec3& force, physx::PxForceMode::Enum mode = physx::PxForceMode::eFORCE, bool autowake = true)
	{
		PX_UNUSED(force);
		PX_UNUSED(mode);
		PX_UNUSED(autowake);

		GRB_API_RESTRICT("addForce");
	}

	virtual		void			addTorque(const PxVec3& torque, physx::PxForceMode::Enum mode = physx::PxForceMode::eFORCE, bool autowake = true)
	{
		PX_UNUSED(torque);
		PX_UNUSED(mode);
		PX_UNUSED(autowake);

		GRB_API_RESTRICT("addTorque");
	}

	virtual		void			clearForce(physx::PxForceMode::Enum mode = physx::PxForceMode::eFORCE, bool autowake = true)
	{
		PX_UNUSED(mode);
		PX_UNUSED(autowake);

		GRB_API_RESTRICT("clearForce");
	}

	virtual		void			clearTorque(physx::PxForceMode::Enum mode = physx::PxForceMode::eFORCE, bool autowake = true)
	{
		PX_UNUSED(mode);
		PX_UNUSED(autowake);

		GRB_API_RESTRICT("clearTorque");
	}

	virtual		bool				isSleeping() const
	{
		GRB_API_RESTRICT("isSleeping");
		return false;
	}

	virtual		void				setSleepThreshold(PxReal )
	{
		GRB_API_RESTRICT("setSleepThreshold");
	}

	virtual		PxReal				getSleepThreshold() const
	{
		GRB_API_RESTRICT("getSleepThreshold");
		return 0.0f;
	}

	virtual		void				wakeUp(PxReal wakeCounterValue=APEX_DEFAULT_WAKE_UP_COUNTER)
	{
		PX_UNUSED(wakeCounterValue);

		GRB_API_RESTRICT("wakeUp");
	}

	virtual		void				putToSleep()
	{
		GRB_API_RESTRICT("putToSleep");
	}

	virtual		void				setSolverIterationCounts(PxU32 minPositionIters, PxU32 minVelocityIters = 1)
	{
		PX_UNUSED(minPositionIters);
		PX_UNUSED(minVelocityIters);

		GRB_API_RESTRICT("setSolverIterationCounts");
	}

	virtual		void				getSolverIterationCounts(PxU32&, PxU32& ) const 
	{
		GRB_API_RESTRICT("getSolverIterationCounts");
	}

	virtual     PxReal				getContactReportThreshold() const 
	{
		GRB_API_RESTRICT("getContactReportThreshold");
		return 0.0f;
	}

	virtual     void				setContactReportThreshold(PxReal) 
	{
		GRB_API_RESTRICT("setContactReportThreshold");
	}


	virtual		void				setMinCCDAdvanceCoefficient(PxReal /*advanceCoefficient*/) 
	{
		GRB_API_RESTRICT("setMinCCDAdvanceCoefficient");
	}

	virtual		PxReal				getMinCCDAdvanceCoefficient() const 
	{
		GRB_API_RESTRICT("getMinCCDAdvanceCoefficient"); return 0.0f;
	}


	virtual		void				setRigidBodyFlag(physx::PxRigidBodyFlag::Enum, bool)
	{
		GRB_API_RESTRICT("setRigidBodyFlag");
	}

	virtual		void				setRigidBodyFlags(physx::PxRigidBodyFlags)
	{
		GRB_API_RESTRICT("setRigidBodyFlags");
	}

	virtual		physx::PxRigidBodyFlags getRigidBodyFlags() const 
	{
		GRB_API_RESTRICT("getRigidBodyFlags");
		return physx::PxRigidBodyFlags();
	}

	virtual		PX_DEPRECATED void					setRigidDynamicFlag(physx::PxRigidDynamicFlag::Enum, bool)
	{
		GRB_API_RESTRICT("setRigidDynamicFlag");
	}

	virtual		PX_DEPRECATED void					setRigidDynamicFlags(physx::PxRigidDynamicFlags)
	{
		GRB_API_RESTRICT("setRigidDynamicFlags");
	}

	virtual		PX_DEPRECATED physx::PxRigidDynamicFlags	getRigidDynamicFlags()	const
	{
		GRB_API_RESTRICT("getRigidDynamicFlags");
		return physx::PxRigidDynamicFlags();
	}

	virtual		PxU32			getNbConstraints()		const
	{
		GRB_API_RESTRICT("getNbConstraints");
		return 0;
	}

	virtual		PxU32			getConstraints(physx::PxConstraint** userBuffer, PxU32 bufferSize, PxU32 startIndex=0) const
	{
		PX_UNUSED(userBuffer);
		PX_UNUSED(bufferSize);
		PX_UNUSED(startIndex);

		GRB_API_RESTRICT("getConstraints");
		return 0;
	}

	virtual		void				setKinematicTarget(const physx::PxTransform& )
	{
		GRB_API_RESTRICT("setKinematicTarget");
	}

	virtual		bool				getKinematicTarget(physx::PxTransform& )
	{
		GRB_API_RESTRICT("getKinematicTarget");
		return false;
	}

	virtual		void					clearForce(physx::PxForceMode::Enum )
	{
		GRB_API_RESTRICT("clearForce");
	}

	virtual		void					clearTorque(physx::PxForceMode::Enum )
	{
		GRB_API_RESTRICT("clearTorque");
	}

	virtual		void					setWakeCounter(PxReal )
	{
		GRB_API_RESTRICT("setWakeCounter");
	}

	virtual		PxReal					getWakeCounter() const
	{
		GRB_API_RESTRICT("getWakeCounter");

		return 0.0f;
	}

	virtual		void					wakeUp()
	{
		GRB_API_RESTRICT("wakeUp");
	}

	virtual		physx::PxBounds3		getWorldBounds(float inflation=1.01f) const
	{
		GRB_API_RESTRICT("getWorldBounds");
		PX_UNUSED(inflation);
		return physx::PxBounds3();
	}

	virtual		void			setActorFlag(physx::PxActorFlag::Enum , bool)
	{
		GRB_API_RESTRICT("setActorFlag");
	}

	virtual		void			setActorFlags( physx::PxActorFlags)
	{
		GRB_API_RESTRICT("setActorFlags");
	}

	virtual		physx::PxActorFlags	getActorFlags()	const 
	{
		GRB_API_RESTRICT("getActorFlags");
		return physx::PxActorFlags();
	}

	virtual		void			setDominanceGroup(physx::PxDominanceGroup)
	{
		GRB_API_RESTRICT("setDominanceGroup");
	}

	virtual		physx::PxDominanceGroup	getDominanceGroup() const 
	{
		GRB_API_RESTRICT("getDominanceGroup");
		return physx::PxDominanceGroup();
	}

	virtual		void			setOwnerClient( physx::PxClientID )
	{
		GRB_API_RESTRICT("setOwnerClient");
	}

	virtual		physx::PxClientID		getOwnerClient() const 
	{
		GRB_API_RESTRICT("getOwnerClient");
		return physx::PxClientID();
	}

	virtual		void			setClientBehaviorFlags(physx::PxActorClientBehaviorFlags) 
	{
		GRB_API_RESTRICT("setClientBehaviorFlags");
	}

	virtual		physx::PxActorClientBehaviorFlags getClientBehaviorFlags()	const 
	{
		GRB_API_RESTRICT("getClientBehaviorFlags");
		return physx::PxActorClientBehaviorFlags();
	}

	virtual		physx::PxAggregate*	getAggregate()	const 
	{
		GRB_API_RESTRICT("getAggregate");
		return 0;
	}

	virtual		void			release();

	virtual		physx::PxActorType::Enum	getType() const
	{
		return actor->getType();
	}

	physx::PxRigidDynamic* actor;
	physx::PxScene* scene;
	static physx::PxMaterial * grbMaterial;
	static PxU32	refCount;
};

#endif // #if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2


#if APEX_USE_GRB && NX_SDK_VERSION_MAJOR == 2
class GrbPhysicsSDK;
class GrbActor;

// BRG - until GRB SDK uses Px foundation
class PxAllocatorWrapper : public NxUserAllocator
{
public:
	void* mallocDEBUG(size_t size, const char* fileName, int line);
	void* malloc(size_t size);
	void* realloc(void* memory, size_t size);
	void free(void* memory);
};

#include "NxUserOutputStream.h"
// BRG - until GRB SDK uses PxUserOutputStream
class PxUserOutputStreamWrapper : public NxUserOutputStream
{
public:
	PxUserOutputStreamWrapper(physx::PxErrorCallback* pxUserOutputStream) : m_pxUserOutputStream(pxUserOutputStream) {}

	void reportError(NxErrorCode code, const char* message, const char* file, int line);
	NxAssertResponse reportAssertViolation(const char* message, const char* file, int line);
	void print(const char* message);

private:
	physx::PxErrorCallback* m_pxUserOutputStream;
};
#endif	// #if APEX_USE_GRB

namespace physx
{
namespace apex
{
namespace destructible
{

/*** SyncParams ***/
typedef NxUserDestructibleSyncHandler<NxApexDamageEventHeader>		UserDamageEventHandler;
typedef NxUserDestructibleSyncHandler<NxApexFractureEventHeader>	UserFractureEventHandler;
typedef NxUserDestructibleSyncHandler<NxApexChunkTransformHeader>	UserChunkMotionHandler;

class DestructibleActor;
class DestructibleScene;

class DestructibleModuleCachedData : public NiApexModuleCachedData, public physx::UserAllocated
{
public:
	DestructibleModuleCachedData(NxAuthObjTypeID moduleID);
	virtual ~DestructibleModuleCachedData();

	virtual NxAuthObjTypeID				getModuleID() const
	{
		return mModuleID;
	}

	virtual NxParameterized::Interface*	getCachedDataForAssetAtScale(NxApexAsset& asset, const physx::PxVec3& scale);
	virtual physx::PxFileBuf&			serialize(physx::PxFileBuf& stream) const;
	virtual physx::PxFileBuf&			deserialize(physx::PxFileBuf& stream);
	virtual void						clear();

	void								clearAssetCollisionSet(const DestructibleAsset& asset);
	physx::Array<NxConvexMesh*>*			getConvexMeshesForActor(const DestructibleActor& destructible);

	DestructibleAssetCollision*			getAssetCollisionSetForActor(const DestructibleActor& destructible);

	virtual physx::PxFileBuf& serializeSingleAsset(NxApexAsset& asset, physx::PxFileBuf& stream);
	virtual physx::PxFileBuf& deserializeSingleAsset(NxApexAsset& asset, physx::PxFileBuf& stream);
private:
	DestructibleAssetCollision*			findAssetCollisionSet(const char* name);

	struct Version
	{
		enum Enum
		{
			First = 0,

			Count,
			Current = Count - 1
		};
	};

	NxAuthObjTypeID								mModuleID;
	physx::Array<DestructibleAssetCollision*>	mAssetCollisionSets;
	mutable physx::ReadWriteLock 				m_lock; // ctremblay +-
};

class ModuleDestructible : public NxModuleDestructible, public NiModule, public Module
{
public:

	ModuleDestructible(NiApexSDK* sdk);
	~ModuleDestructible();

	void						destroy();

	// base class methods
	void						init(NxParameterized::Interface& desc);
	NxParameterized::Interface* getDefaultModuleDesc();
	void						release()
	{
		Module::release();
	}
	const char*					getName() const
	{
		return Module::getName();
	}
	physx::PxU32				getNbParameters() const
	{
		return Module::getNbParameters();
	}
	NxApexParameter**			getParameters()
	{
		return Module::getParameters();
	}
	void						setLODUnitCost(physx::PxF32 cost)
	{
		Module::setLODUnitCost(cost);
	}
	physx::PxF32				getLODUnitCost() const
	{
		return Module::getLODUnitCost();
	}
	void						setLODBenefitValue(physx::PxF32 value)
	{
		Module::setLODBenefitValue(value);
	}
	physx::PxF32				getLODBenefitValue() const
	{
		return Module::getLODBenefitValue();
	}
	void						setLODEnabled(bool enabled)
	{
		Module::setLODEnabled(enabled);
	}
	bool						getLODEnabled() const
	{
		return Module::getLODEnabled();
	}

	void						setIntValue(physx::PxU32 parameterIndex, physx::PxU32 value);
	NiModuleScene* 				createNiModuleScene(NiApexScene&, NiApexRenderDebug*);
	void						releaseNiModuleScene(NiModuleScene&);
	physx::PxU32				forceLoadAssets();
	NxAuthObjTypeID				getModuleID() const;
	NxApexRenderableIterator* 	createRenderableIterator(const NxApexScene&);

#if APEX_USE_GRB
	void                        setGrbMeshCellSize(float cellSize);
	void                        setGrbMaxLinAcceleration(float maxLinAcceleration);
	bool                        isGrbSimulationEnabled(const NxApexScene& apexScene) const;
	void                        setGrbSimulationEnabled(NxApexScene& apexScene, bool enabled);

#if NX_SDK_VERSION_MAJOR == 2
	NxActor*					createGrbActor(const NxActorDesc& desc, NxApexScene& apexScene);
#else
	PxRigidDynamic *			createGrbActor(const physx::PxTransform & transform, NxApexScene& apexScene, physx::PxScene** ppScene = 0);
	void						addGrbActor(PxRigidDynamic * desc, NxApexScene& apexScene);
#endif

//	void						releaseGrbActor(GrbActor& actor, NxApexScene& apexScene);
#endif

	NxDestructibleActorJoint*	createDestructibleActorJoint(const NxDestructibleActorJointDesc&, NxApexScene&);
	bool                        isDestructibleActorJointActive(const NxDestructibleActorJoint*, NxApexScene&) const;

	void						setMaxDynamicChunkIslandCount(physx::PxU32 maxCount);
	void						setMaxChunkCount(physx::PxU32 maxCount);

	void						setSortByBenefit(bool sortByBenefit);
	void						setMaxChunkDepthOffset(physx::PxU32 offset);
	void						setMaxChunkSeparationLOD(physx::PxF32 separationLOD);

	void						setChunkReport(NxUserChunkReport* chunkReport);
	void						setChunkReportBitMask(physx::PxU32 chunkReportBitMask);
	void						setDestructiblePhysXActorReport(NxUserDestructiblePhysXActorReport* destructiblePhysXActorReport);
	void						setChunkReportMaxFractureEventDepth(physx::PxU32 chunkReportMaxFractureEventDepth);
	void						setChunkCrumbleReport(NxUserChunkParticleReport* chunkCrumbleReport);
	void						setChunkDustReport(NxUserChunkParticleReport* chunkDustReport);
#if NX_SDK_VERSION_MAJOR == 2
	void						setWorldSupportPhysXScene(NxApexScene& apexScene, NxScene* physxScene);
#elif NX_SDK_VERSION_MAJOR == 3
	void						setWorldSupportPhysXScene(NxApexScene& apexScene, PxScene* physxScene);
#endif
#if NX_SDK_VERSION_MAJOR == 2
	bool						owns(const NxActor* actor) const;
#elif NX_SDK_VERSION_MAJOR == 3
	bool						owns(const PxRigidActor* actor) const;
#if APEX_RUNTIME_FRACTURE
	bool						isRuntimeFractureShape(const PxShape& shape) const;
#endif
#endif

	NxDestructibleActor*		getDestructibleAndChunk(const NxShape* shape, physx::PxI32* chunkIndex) const;

	void						applyRadiusDamage(NxApexScene& scene, physx::PxF32 damage, physx::PxF32 momentum,
	        const physx::PxVec3& position, physx::PxF32 radius, bool falloff);

	void						setMaxActorCreatesPerFrame(physx::PxU32 maxActorsPerFrame);
	void						setMaxFracturesProcessedPerFrame(physx::PxU32 maxActorsPerFrame);
	void                        setValidBoundsPadding(physx::PxF32);

	void						releaseBufferedConvexMeshes();

	NiApexModuleCachedData*		getModuleDataCache()
	{
		return mCachedData;
	}

	PX_INLINE NxParameterized::Interface* getApexDestructiblePreviewParams(void) const
	{
		return mApexDestructiblePreviewParams;
	}

#if NX_SDK_VERSION_MAJOR == 2
	NxActor*					createTwoWayRb(const NxActorDesc& desc, NxApexScene& apexScene);
	bool						releaseTwoWayRb(NxActor& actor);
#else
	PxRigidDynamic *			createTwoWayRb(const physx::PxTransform & transform, NxApexScene& apexScene);
	physx::PxRigidDynamic *		addTwoWayRb(PxRigidDynamic * actorExternal, NxApexScene& apexScene);
	bool						releaseTwoWayRb(PxRigidDynamic& actor);
#endif

	void						setUseLegacyChunkBoundsTesting(bool useLegacyChunkBoundsTesting);
	bool						getUseLegacyChunkBoundsTesting() const
	{
		return mUseLegacyChunkBoundsTesting;
	}

	void						setUseLegacyDamageRadiusSpread(bool useLegacyDamageRadiusSpread);
	bool						getUseLegacyDamageRadiusSpread() const
	{
		return mUseLegacyDamageRadiusSpread;
	}

	bool						setMassScaling(physx::PxF32 massScale, physx::PxF32 scaledMassExponent, NxApexScene& apexScene);

	DestructibleModuleParameters* getModuleParameters()
	{
			return mModuleParams;
	}

#if APEX_USE_GRB
	physx::Array<GrbActorExternal*>		twoWayRbList;
#endif

	physx::Array<NxConvexMesh*>			convexMeshKillList;

private:
	enum
	{
		DefaultMaxGrbChunkIslandCount =	0,			// set to zero to ensure that GRB simulate doesn't get called until a level with GRBs in it gets loaded
	};

	//	Private interface, used by Destructible* classes

	DestructibleScene* 				getDestructibleScene(const NxApexScene& apexScene) const;



	// Max chunk depth offset (for LOD) - effectively reduces the max chunk depth in all destructibles by this number
	physx::PxU32							m_maxChunkDepthOffset;
	// Where in the assets' min-max range to place the lifetime and max. separation
	physx::PxF32							m_maxChunkSeparationLOD;
	physx::PxF32							m_validBoundsPadding;
	physx::PxU32							m_maxFracturesProcessedPerFrame;
	physx::PxU32							m_maxActorsCreateablePerFrame;
	physx::PxU32							m_dynamicActorFIFOMax;
	physx::PxU32							m_chunkFIFOMax;
	bool									m_sortByBenefit;
	NxUserChunkReport*						m_chunkReport;
	physx::PxU32							m_chunkReportBitMask;
	NxUserDestructiblePhysXActorReport*		m_destructiblePhysXActorReport;
	physx::PxU32							m_chunkReportMaxFractureEventDepth;
	NxUserChunkParticleReport*				m_chunkCrumbleReport;
	NxUserChunkParticleReport*				m_chunkDustReport;

	physx::PxF32							m_massScale;
	physx::PxF32							m_scaledMassExponent;

	NxResourceList							m_destructibleSceneList;

	NxResourceList							mAuthorableObjects;

#	define PARAM_CLASS(clas) PARAM_CLASS_DECLARE_FACTORY(clas)
#	include "DestructibleParamClasses.inc"

	NxParameterized::Interface*					mApexDestructiblePreviewParams;
	DestructibleModuleParameters*				mModuleParams;

#if APEX_USE_GRB
	bool										mGrbInitialized;
	physx::PxF32								mGrbMeshCellSize;
	physx::PxF32								mGrbSkinWidth;
	physx::PxU32								mGrbNonPenSolverPosIterCount;
	physx::PxU32								mGrbFrictionSolverPosIterCount;
	physx::PxU32								mGrbFrictionSolverVelIterCount;
	physx::PxU32								mGrbMemTempDataSize;
	physx::PxU32								mGrbMemSceneSize;
	physx::PxF32								mGrbMaxLinAcceleration;
#if NX_SDK_VERSION_MAJOR == 2
	PxUserOutputStreamWrapper					mPxUserOutputStreamWrapper;
	GrbPhysicsSDK*							    mGrbPhysicsSDK;
#else
	physx::PxPhysics *							mGrbPhysicsSDK;
#endif
#endif

	DestructibleModuleCachedData*				mCachedData;

	physx::QDSRand								mRandom;

	bool										mUseLegacyChunkBoundsTesting;
	bool										mUseLegacyDamageRadiusSpread;

    /*** ModuleDestructible::SyncParams ***/
public:
	bool setSyncParams(UserDamageEventHandler * userDamageEventHandler, UserFractureEventHandler * userFractureEventHandler, UserChunkMotionHandler * userChunkMotionHandler);
public:
    class SyncParams
    {
		friend bool ModuleDestructible::setSyncParams(UserDamageEventHandler *, UserFractureEventHandler *, UserChunkMotionHandler *);
    public:
        SyncParams();
        ~SyncParams();
		UserDamageEventHandler *			getUserDamageEventHandler() const;
		UserFractureEventHandler *			getUserFractureEventHandler() const;
		UserChunkMotionHandler *			getUserChunkMotionHandler() const;
		template<typename T> physx::PxU32	getSize() const;
	private:
        DECLARE_DISABLE_COPY_AND_ASSIGN(SyncParams);
		UserDamageEventHandler *			userDamageEventHandler;
		UserFractureEventHandler *			userFractureEventHandler;
		UserChunkMotionHandler *			userChunkMotionHandler;
    };
	const ModuleDestructible::SyncParams &	getSyncParams() const;
private:
	SyncParams								mSyncParams;

private:
	friend class DestructibleActor;
	friend class DestructibleActorJoint;
	friend class DestructibleAsset;
	friend class DestructibleStructure;
	friend class DestructibleScene;
	friend class DestructibleContactReport;
	friend class DestructibleContactModify;
	friend class ApexDamageEventReportData;
	friend struct DestructibleNXActorCreator;
};

}
}
} // end namespace physx::apex


#endif // __MODULE_DESTRUCTIBLE_H__
