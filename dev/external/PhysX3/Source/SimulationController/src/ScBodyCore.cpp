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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#include "ScBodyCore.h"
#include "ScBodySim.h"
#include "ScPhysics.h"

using namespace physx;

/*
static PX_FORCE_INLINE PxVec3 invertDiagInertia(const PxVec3& m)
{
	const PxVec3 minv(1.0f/m.x, 1.0f/m.y, 1.0f/m.z);
	return minv.isFinite() ? minv : PxVec3(0.0f);
}


BodyCore map:

ActorCore:
	PxU32				mCompoundID;			// PT: TODO: this one makes us waste 12 bytes in bodycore. Use virtuals to store it somewhere else.
	ActorSim*			mSim;
	PxActorFlags		mActorFlags;			// PxActor's flags (PxU16)
	PxU8				mActorType;				// store as 8 bits to save space.
	PxU8				mClientBehaviorBits;	// similarly
	PxDominanceGroup	mDominanceGroup;		// PxU8
	PxClientID			mOwnerClient;			// PxU8
BodyCore:
	PxReal				mWakeCounter;
	PxRigidBodyFlags	mFlags;				// API body flags
	PX_ALIGN(16, PxsBodyCore mCore);
	PxReal				mSleepThreshold;
	PxReal				mSleepDamping;
*/

Sc::BodyCore::BodyCore(PxActorType::Enum type, const PxTransform& bodyPose) 
:	RigidCore(type)
{
	const PxTolerancesScale& scale = Physics::getInstance().getTolerancesScale();
	// sizeof(BodyCore) = 176 => 160 => 144 bytes

	mCore.inverseInertia			= PxVec3(1,1,1);
	mCore.inverseMass				= 1.0f;
	mCore.body2World				= bodyPose;

	PX_ASSERT(mCore.body2World.p.isFinite());
	PX_ASSERT(mCore.body2World.q.isFinite());


	mSleepThreshold					= 5e-5f * scale.speed * scale.speed;
	mSleepDamping					= 0.0f;
	mSimStateData					= NULL;
	mCore.mWakeCounter				= Physics::sWakeCounterOnCreation;
	mCore.mFlags					= PxRigidBodyFlags();
	mCore.linearVelocity			= PxVec3(0);
	mCore.angularVelocity			= PxVec3(0);
	mCore.linearDamping				= 0.0f;
	mCore.maxLinearVelocitySq		= PX_MAX_F32;
	mCore.solverIterationCounts		= (1 << 8) | 4;	
	mCore.contactReportThreshold	= PX_MAX_F32;	
	mCore.body2Actor				= PxTransform(PxIdentity);
	mCore.ccdAdvanceCoefficient		= 0.15f;

	if(type == PxActorType::eRIGID_DYNAMIC)
	{
		mCore.angularDamping			= 0.05f;
		mCore.maxAngularVelocitySq		= 7.0f * 7.0f;
	}
	else
	{
		mCore.angularDamping			= 0.0f;
		mCore.maxAngularVelocitySq		= PX_MAX_F32;
	}
}

Sc::BodyCore::~BodyCore()
{
	PX_ASSERT(getSim() == 0);
	PX_ASSERT(!mSimStateData);
}

Sc::BodySim* Sc::BodyCore::getSim() const
{
	return static_cast<BodySim*>(Sc::ActorCore::getSim());
}

void	Sc::BodyCore::setCCDAdvanceCoefficient(PxReal ccdAdvanceCoefficient)
{ 
	mCore.ccdAdvanceCoefficient = ccdAdvanceCoefficient;
}

size_t	Sc::BodyCore::getSerialCore(PxsBodyCore& serialCore)
{
	PX_ASSERT(!mSimStateData || mSimStateData->isKine());

	serialCore = mCore;
	if(mSimStateData)
	{	
		const Kinematic* kine			= mSimStateData->getKinematicData();
		serialCore.inverseMass			= kine->backupInvMass;
		serialCore.inverseInertia		= kine->backupInverseInertia;
		serialCore.linearDamping		= kine->backupLinearDamping;
		serialCore.angularDamping		= kine->backupAngularDamping;
		serialCore.maxAngularVelocitySq	= kine->backupMaxAngVelSq;
		serialCore.maxLinearVelocitySq	= kine->backupMaxLinVelSq;
	}
	return 	reinterpret_cast<size_t>(&mCore);
}

//--------------------------------------------------------------
//
// BodyCore interface implementation
//
//--------------------------------------------------------------

void Sc::BodyCore::setBody2World(const PxTransform& p)
{
	mCore.body2World = p;
	PX_ASSERT(p.p.isFinite());
	PX_ASSERT(p.q.isFinite());

	BodySim* sim = getSim();
	if(sim)
		sim->postBody2WorldChange();
}

void Sc::BodyCore::addSpatialAcceleration(Ps::Pool<SimStateData>* simStateDataPool, const PxVec3* linAcc, const PxVec3* angAcc)
{
	//The dirty flag is stored separately in the BodySim so that we query the dirty flag before going to 
	//the expense of querying the simStateData for the velmod values.
	BodySim* sim = getSim();
	if(sim)
	{
		sim->notifyAddSpatialAcceleration();
	}

	if(!mSimStateData || !mSimStateData->isVelMod())
	{
		setupSimStateData(simStateDataPool, false);
	}

	VelocityMod* velmod = mSimStateData->getVelocityModData();
	velmod->notifyAddAcceleration();
	if(linAcc) velmod->accumulateLinearVelModPerSec(*linAcc);
	if(angAcc) velmod->accumulateAngularVelModPerSec(*angAcc);
}

void Sc::BodyCore::clearSpatialAcceleration()
{
	//The dirty flag is stored separately in the BodySim so that we query the dirty flag before going to 
	//the expense of querying the simStateData for the velmod values.
	BodySim* sim = getSim();
	if(sim)
	{
		sim->notifyClearSpatialAcceleration();
	}

	if(mSimStateData)
	{
		PX_ASSERT(mSimStateData->isVelMod());
		VelocityMod* velmod = mSimStateData->getVelocityModData();
		velmod->notifyClearAcceleration();
		velmod->clearLinearVelModPerSec();
		velmod->clearAngularVelModPerSec();
	}
}


void Sc::BodyCore::addSpatialVelocity(Ps::Pool<SimStateData>* simStateDataPool, const PxVec3* linVelDelta, const PxVec3* angVelDelta)
{
	//The dirty flag is stored separately in the BodySim so that we query the dirty flag before going to 
	//the expense of querying the simStateData for the velmod values.
	BodySim* sim = getSim();
	if(sim)
	{
		sim->notifyAddSpatialVelocity();
	}

	if(!mSimStateData || !mSimStateData->isVelMod())
	{
		setupSimStateData(simStateDataPool, false);
	}

	VelocityMod* velmod = mSimStateData->getVelocityModData();
	velmod->notifyAddVelocity();
	if(linVelDelta) velmod->accumulateLinearVelModPerStep(*linVelDelta);
	if(angVelDelta) velmod->accumulateAngularVelModPerStep(*angVelDelta);
}

void Sc::BodyCore::clearSpatialVelocity()
{
	//The dirty flag is stored separately in the BodySim so that we query the dirty flag before going to 
	//the expense of querying the simStateData for the velmod values.
	BodySim* sim = getSim();
	if(sim)
	{
		sim->notifyClearSpatialVelocity();
	}

	if(mSimStateData)
	{
		PX_ASSERT(mSimStateData->isVelMod());
		VelocityMod* velmod = mSimStateData->getVelocityModData();
		velmod->notifyClearVelocity();
		velmod->clearLinearVelModPerSec();
		velmod->clearAngularVelModPerSec();
	}
}


PxReal Sc::BodyCore::getInverseMass() const
{
	return mSimStateData && mSimStateData->isKine() ? mSimStateData->getKinematicData()->backupInvMass : mCore.inverseMass;
}


void Sc::BodyCore::setInverseMass(PxReal m)
{
	if(mSimStateData && mSimStateData->isKine())
		mSimStateData->getKinematicData()->backupInvMass = m;
	else
		mCore.inverseMass = m;
}


const PxVec3& Sc::BodyCore::getInverseInertia() const
{
	return mSimStateData && mSimStateData->isKine() ? mSimStateData->getKinematicData()->backupInverseInertia : mCore.inverseInertia;
}


void Sc::BodyCore::setInverseInertia(const PxVec3& i)
{
	if(mSimStateData && mSimStateData->isKine())
		mSimStateData->getKinematicData()->backupInverseInertia = i;
	else
		mCore.inverseInertia = i;
}


PxReal Sc::BodyCore::getLinearDamping() const
{
	return mSimStateData && mSimStateData->isKine() ? mSimStateData->getKinematicData()->backupLinearDamping : mCore.linearDamping;
}


void Sc::BodyCore::setLinearDamping(PxReal d)
{
	if(mSimStateData && mSimStateData->isKine())
		mSimStateData->getKinematicData()->backupLinearDamping = d;
	else
		mCore.linearDamping = d;
}


PxReal Sc::BodyCore::getAngularDamping() const
{
	return mSimStateData && mSimStateData->isKine() ? mSimStateData->getKinematicData()->backupAngularDamping : mCore.angularDamping;
}


void Sc::BodyCore::setAngularDamping(PxReal v)
{
	if(mSimStateData && mSimStateData->isKine())
		mSimStateData->getKinematicData()->backupAngularDamping = v;
	else
		mCore.angularDamping = v;
}

PxReal Sc::BodyCore::getMaxAngVelSq() const
{
	return mSimStateData && mSimStateData->isKine() ? mSimStateData->getKinematicData()->backupMaxAngVelSq : mCore.maxAngularVelocitySq;
}


void Sc::BodyCore::setMaxAngVelSq(PxReal v)
{
	if(mSimStateData && mSimStateData->isKine())
		mSimStateData->getKinematicData()->backupMaxAngVelSq = v;
	else
		mCore.maxAngularVelocitySq = v;	
}


void Sc::BodyCore::setFlags(Ps::Pool<SimStateData>* simStateDataPool, PxRigidBodyFlags f)
{
	PxRigidBodyFlags old = mCore.mFlags;
	if(f != old)
	{
		PxU32 wasKinematic = old & PxRigidBodyFlag::eKINEMATIC;
		PxU32 isKinematic = f & PxRigidBodyFlag::eKINEMATIC;
		bool switchToKinematic = ((!wasKinematic) && isKinematic);
		bool switchToDynamic = (wasKinematic && (!isKinematic));

		if (switchToKinematic)
			putToSleep();

		mCore.mFlags = f;
		BodySim* sim = getSim();
		if(sim)
		{
			PX_ASSERT(simStateDataPool);

			// for those who might wonder about the complexity here:
			// our current behavior is that you are not allowed to set a kinematic target unless the object is in a scene.
			// Thus, the kinematic data should only be created/destroyed when we know for sure that we are in a scene.

			if (switchToKinematic)
			{
				setupSimStateData(simStateDataPool, true, false);
				sim->postSwitchToKinematic();
			}
			else if (switchToDynamic)
			{
				tearDownSimStateData(simStateDataPool, true);
				sim->postSwitchToDynamic();
			}
		}
	}
}


void Sc::BodyCore::setWakeCounter(PxReal wakeCounter, bool forceWakeUp)
{
	mCore.mWakeCounter = wakeCounter;

	BodySim* sim = getSim();
	if(sim)
	{
		if ((wakeCounter > 0.0f) || forceWakeUp)
			sim->wakeUp();
		sim->postSetWakeCounter(wakeCounter, forceWakeUp);
	}
}


bool Sc::BodyCore::isSleeping() const
{
	BodySim* sim = getSim();
	return sim ? sim->isSleeping() : true;
}


void Sc::BodyCore::putToSleep()
{
	setLinearVelocity(PxVec3(0.0f));
	setAngularVelocity(PxVec3(0.0f));

	//The dirty flag is stored separately in the BodySim so that we query the dirty flag before going to 
	//the expense of querying the simStateData for the velmod values.
	BodySim* sim = getSim();
	if (sim)
	{
		sim->notifyClearSpatialAcceleration();
		sim->notifyClearSpatialVelocity();
	}

	//The velmod data is stored in a separate structure so we can record forces before scene insertion.
	if(mSimStateData && mSimStateData->isVelMod())
	{
		VelocityMod* velmod = mSimStateData->getVelocityModData();
		velmod->clear();
	}

	// important to clear all values before setting the wake counter because the values decide
	// whether an object is ready to go to sleep or not.
	setWakeCounter(0.0f);

	if(sim)
		sim->putToSleep();
}


void Sc::BodyCore::setKinematicTarget(Ps::Pool<SimStateData>* simStateDataPool, const PxTransform& p, PxReal wakeCounter)
{
	PX_ASSERT(mCore.mFlags & PxRigidBodyFlag::eKINEMATIC);
	PX_ASSERT(!mSimStateData || mSimStateData->isKine());

	if(mSimStateData)
	{
		Kinematic* kine = mSimStateData->getKinematicData();
		kine->targetPose = p;
		kine->targetValid = 1;
		
		Sc::BodySim* bSim = getSim();
		if (bSim)
			bSim->postSetKinematicTarget();
	}
	else
	{
		bool success = setupSimStateData(simStateDataPool, true, true);
		if (success)
		{
			PX_ASSERT(!getSim());  // covers the following scenario: kinematic gets added to scene while sim is running and target gets set (at that point the sim object does not yet exist)

			Kinematic* kine = mSimStateData->getKinematicData();
			kine->targetPose = p;
			kine->targetValid = 1;
		}
		else
			Ps::getFoundation().error(PxErrorCode::eOUT_OF_MEMORY, __FILE__, __LINE__, "PxRigidDynamic: setting kinematic target failed, not enough memory.");
	}

	wakeUp(wakeCounter);
}


void Sc::BodyCore::disableInternalCaching(bool disable)
{
	PX_ASSERT(!mSimStateData || mSimStateData->isKine());

	if(mSimStateData)
	{
		PX_ASSERT(getFlags() & PxRigidBodyFlag::eKINEMATIC);
		
		if(disable)
			restore();
		else
			backup(*mSimStateData);
	}
}

bool Sc::BodyCore::setupSimStateData(Ps::Pool<SimStateData>* simStateDataPool, const bool isKinematic, const bool targetValid)
{
	if(isKinematic)
	{
		PX_ASSERT(!mSimStateData || !mSimStateData->isKine());

		SimStateData* kData = mSimStateData;
		if(!kData)
		{
			kData = simStateDataPool->construct();
		}

		new(kData) SimStateData(SimStateData::eKine);
		Kinematic* kine = kData->getKinematicData();
		kine->targetValid = targetValid ? 1 : 0;
		backup(*kData);
		mSimStateData = kData;

		return true;
	}
	else
	{
		PX_ASSERT(!mSimStateData || !mSimStateData->isVelMod());
		PX_ASSERT(!targetValid);

		SimStateData* vData = mSimStateData;
		if(!vData)
		{
			vData = simStateDataPool->construct();
		}

		new(vData) SimStateData(SimStateData::eVelMod);
		VelocityMod* velmod = vData->getVelocityModData();
		velmod->clear();
		velmod->flags = 0;
		mSimStateData = vData;

		return true;
	}
}


bool  Sc::BodyCore::checkSimStateKinematicStatus(const bool isKinematic) const 
{ 
	PX_ASSERT(mSimStateData);
	return mSimStateData->isKine() == isKinematic;
}

void Sc::BodyCore::tearDownSimStateData(Ps::Pool<SimStateData>* simStateDataPool, const bool isKinematic)
{
	PX_ASSERT(!mSimStateData || mSimStateData->isKine() == isKinematic);

	if (mSimStateData)
	{
		if(isKinematic)
		{
			restore();
		}
		simStateDataPool->destroy(mSimStateData);
		mSimStateData=NULL;
	}
}


void Sc::BodyCore::backup(SimStateData& b)
{
	PX_ASSERT(b.isKine());

	Kinematic* kine = b.getKinematicData();
	kine->backupLinearDamping	= mCore.linearDamping;
	kine->backupAngularDamping	= mCore.angularDamping;
	kine->backupInverseInertia	= mCore.inverseInertia;
	kine->backupInvMass			= mCore.inverseMass;
	kine->backupMaxAngVelSq		= mCore.maxAngularVelocitySq;
	kine->backupMaxLinVelSq		= mCore.maxLinearVelocitySq;

	mCore.inverseMass			= 0.0f;
	mCore.inverseInertia		= PxVec3(0);
	mCore.linearDamping			= 0.0f;
	mCore.angularDamping		= 0.0f;
	mCore.maxAngularVelocitySq	= PX_MAX_REAL;
	mCore.maxLinearVelocitySq	= PX_MAX_REAL;
}


void Sc::BodyCore::restore()
{
	PX_ASSERT(mSimStateData && mSimStateData->isKine());

	const Kinematic* kine		= mSimStateData->getKinematicData();
	mCore.inverseMass			= kine->backupInvMass;
	mCore.inverseInertia		= kine->backupInverseInertia;
	mCore.linearDamping			= kine->backupLinearDamping;
	mCore.angularDamping		= kine->backupAngularDamping;
	mCore.maxAngularVelocitySq	= kine->backupMaxAngVelSq;
	mCore.maxLinearVelocitySq	= kine->backupMaxLinVelSq;
}

void Sc::BodyCore::invalidateKinematicTarget()
{ 
	PX_ASSERT(mSimStateData && mSimStateData->isKine()); 
	mSimStateData->getKinematicData()->targetValid = 0; 
}

void Sc::BodyCore::onOriginShift(const PxVec3& shift)
{
	BodySim* b = getSim();
	PX_ASSERT(b);

	mCore.body2World.p -= shift;
	if (mSimStateData && (getFlags() & PxRigidBodyFlag::eKINEMATIC) && mSimStateData->getKinematicData()->targetValid)
	{
		mSimStateData->getKinematicData()->targetPose.p -= shift;
	}

	b->onOriginShift(shift);
}

const PxRigidActor* ScGetPxRigidBodyFromPxsRigidCore(const PxsRigidCore* core)
{
	const size_t offset = size_t(&(reinterpret_cast<Sc::BodyCore*>(0)->getCore()));
	const Sc::BodyCore* bodyCore = reinterpret_cast<const Sc::BodyCore*>(reinterpret_cast<const char*>(core)-offset);
	return static_cast<const PxRigidBody*>(bodyCore->getPxActor());
}
