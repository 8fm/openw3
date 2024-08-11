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


#include "ScBodySim.h"
#include "ScScene.h"
#include "ScConstraintSim.h"
#include "ScConstraintInteraction.h"
#include "ScArticulationSim.h"
#include "PxsContext.h"
#include "PxsRigidBody.h"
#include "PxsIslandManager.h"
#include "ScShapeIterator.h"
#include "ScShapeSim.h"
#include "ScConstraintCore.h"

using namespace physx;

/*
BodySim map:

Actor:
	Interaction*				mInlineInteractionMem[INLINE_INTERACTION_CAPACITY];
	Cm::OwnedArray<Sc::Interaction*, Sc::Actor, PxU32, &Sc::Actor::reallocInteractions> mInteractions;
*	Element*					mElements;
*	InteractionScene&			mInteractionScene;
*	PxU32						mSceneArrayIndex;				// Used by InteractionScene 
	PxU32						mTimestamp;
*	PxU32						mNumTransferringInteractions;	// PT: probably doesn't need 32bits here
*	PxU32						mStaticTouchCount;				// PT: probably doesn't need 32bits here
	PxU32						mIslandIndex;					// PT: probably doesn't need 32bits here
*	PxU16						mNumCountedInteractions;		// PT: stored on PxU16 to save space
*	PxU8						mActorType;						// PT: stored on a byte to save space
*	PxU8						mIslandNodeInfo;
ActorSim:
*	ActorCore&					mCore;
RigidSim:
*	PxU32						mBpGroup;
BodySim:
	PxsIslandManagerHook		mLLIslandHook;
*	PxU32						mActiveAtomListIndex;		// To remove from active atom list in O(1)
*	KinematicData*				mKinematicData;				// Only valid/initialized if this is kinematic
*	PxsArticulation*			mArticulation;				// zero if not in an articulation
*	PxU16						mBodyConstraints;			// Used by the USE_ADAPTIVE_FORCE mode only to keep track of how many constraints are on a body
*	PxU16						mLastBodyConstraints;
*	PxU16						mInternalFlags;				// Internal flags
*	ConstraintGroupNode*		mConstraintGroup;
*	PxVec3						mSleepLinVelAcc;
*	PxVec3						mSleepAngVelAcc;
	VelocityMod					mVelocityMod;
*	PxU8						mVelModState;	// Marks open acceleration/velocity changes
*	PxsRigidBody				mLLBody;
*/

Sc::BodySim::BodySim(Scene& scene, BodyCore& core) :
	RigidSim				(scene, core, IslandNodeInfo::eTWO_WAY),
	mConstraintGroup		(NULL),
	mLLBody					(&core.getCore()),
	mInternalFlags			(0),
	mVelModState			(VMF_GRAVITY_DIRTY),
	mBodyConstraints		(0),
	mLastBodyConstraints	((PxU16)-1),
	mArticulation			(NULL),
	mSleepLinVelAcc			(PxVec3(0)),
	mSleepAngVelAcc			(PxVec3(0))
{
	PX_ASSERT(!mLLIslandHook.isManaged());

	// For 32-bit, sizeof(BodyCore) = 160 bytes, sizeof(BodySim) = 192 bytes

	Sc::InteractionScene& iscene = scene.getInteractionScene();

	if (core.getActorFlags()&PxActorFlag::eDISABLE_GRAVITY)
		mInternalFlags = BF_DISABLE_GRAVITY;

	//If a body pending insertion was given a force/torque then it will have 
	//the dirty flags stored in a separate structure.  Copy them across
	//so we can use them now that the BodySim is constructed.
	SimStateData* simStateData = core.getSimStateData(false);
	bool hasPendingForce = false;
	if(simStateData)
	{
		VelocityMod* velmod = simStateData->getVelocityModData();
		hasPendingForce = (velmod->flags != 0) && 
			(!velmod->getLinearVelModPerSec().isZero() || !velmod->getAngularVelModPerSec().isZero() ||
			 !velmod->getLinearVelModPerStep().isZero() || !velmod->getAngularVelModPerStep().isZero());
		mVelModState = velmod->flags;
		velmod->flags = 0;
	}

	// PT: don't read the core ptr we just wrote, use input param
	// PT: at time of writing we get a big L2 here because even though bodycore has been prefetched, the wake counter is 160 bytes away
	const bool isAwake =	(core.getWakeCounter() > 0) || 
							(!core.getLinearVelocity().isZero()) ||
							(!core.getAngularVelocity().isZero()) || 
							hasPendingForce;

	const Ps::IntBool isKine = isKinematic();

	PxsIslandManager& islandManager = getInteractionScene().getLLIslandManager();
	if (!isArticulationLink())
	{
		islandManager.addBody(this, mLLIslandHook, isKine!=0);
	}
	else
	{
		getInteractionScene().getLLIslandManager().addArticulationLink(mLLIslandHook);
		if(mArticulation)
		{
			const PxsArticulationLinkHandle articLinkhandle = mArticulation->getLinkHandle(*this);
			if(!isArticulationRootLink(articLinkhandle))
			{
				getInteractionScene().getLLIslandManager().setArticulationLinkHandle(articLinkhandle, this, mLLIslandHook);
			}
			else
			{
				getInteractionScene().getLLIslandManager().setArticulationRootLinkHandle(articLinkhandle, this, mLLIslandHook);
			}
		}
	}

	iscene.addActor(*this, isAwake);
	if (isAwake)
	{
		islandManager.setAwake(mLLIslandHook);
	}
	else
	{
		PX_ASSERT(getInteractionScene().getLLIslandManager().getIsReadyForSleeping(mLLIslandHook));  // make sure the initial setup in the sleep island system is as expected
		PX_ASSERT(!getInteractionScene().getLLIslandManager().getIsInSleepingIsland(mLLIslandHook));  // make sure the initial setup in the sleep island system is as expected
	}

	if (isKine)
	{
		initKinematicStateBase(core);

		const SimStateData* kd = core.getSimStateData(true);
		if (!kd)
		{
			core.setupSimStateData(getScene().getSimStateDataPool(), true, false);
			notifyPutToSleep();  // sleep state of kinematics is fully controlled by the simulation controller not the island manager
		}
		else
		{
			PX_ASSERT(kd->isKine());
			PX_ASSERT(kd->getKinematicData()->targetValid);  // the only reason for the kinematic data to exist at that point already is if the target has been set
			PX_ASSERT(isAwake);  // the expectation is that setting a target also sets the wake counter to a positive value
			PX_ASSERT(!getInteractionScene().getLLIslandManager().getIsReadyForSleeping(mLLIslandHook));
			PX_ASSERT(!getInteractionScene().getLLIslandManager().getIsInSleepingIsland(mLLIslandHook));
			postSetKinematicTarget();
		}
	}

	PX_ASSERT(mLLIslandHook.isManaged());
}


Sc::BodySim::~BodySim()
{
	Sc::InteractionScene &iScene = getScene().getInteractionScene();
	if(isActive())
		Ps::prefetch(iScene.getActiveBodiesArray()[iScene.getNumActiveBodies()-1],sizeof(Actor));

	getBodyCore().tearDownSimStateData(getScene().getSimStateDataPool(), isKinematic() ? true : false);

	PX_ASSERT(!readInternalFlag(BF_ON_DEATHROW)); // Before 3.0 it could happen that destroy could get called twice. Assert to make sure this is fixed.
	raiseInternalFlag(BF_ON_DEATHROW);

	getScene().removeBody(*this);
	PX_ASSERT(!getConstraintGroup());  // Removing from scene should erase constraint group node if it existed

	if(mArticulation)
		mArticulation->removeBody(*this);

	if(mLLIslandHook.isManaged())
		iScene.getLLIslandManager().removeNode(mLLIslandHook);

	PX_ASSERT(!mLLIslandHook.isManaged());
	PX_ASSERT(!iScene.getLowLevelContext()->getBodyTransformVault().isInVault(mLLBody.getCore()));

	iScene.removeActor(*this);
	mCore.setSim(NULL);
}


//--------------------------------------------------------------
//
// Actor implementation
//
//--------------------------------------------------------------


void Sc::BodySim::onActivate()
{
	PX_ASSERT((!isKinematic()) || notInScene() || readInternalFlag(BF_KINEMATIC_MOVED));	// kinematics should only get activated when a target is set.
																							// exception: object gets newly added, then the state change will happen later

	PX_ASSERT(mLLIslandHook.isManaged());
	PX_ASSERT(getInteractionScene().getLLIslandManager().getIsReadyForSleeping(mLLIslandHook));

	if (!isArticulationLink())
	{
		// Put in list of activated bodies. The list gets cleared at the end of a sim step after the sleep callbacks have been fired.
		getScene().onBodyWakeUp(this);
	}
}

void Sc::BodySim::updateCachedTransforms(PxsTransformCache& cache)
{
	Sc::ShapeIterator iterator;
	iterator.init(*this);
	Sc::ShapeSim* sim;
	while((sim = iterator.getNext())!=NULL)
	{
		if(sim->getTransformCacheID() != PX_INVALID_U32)
		{
			//Update transforms
			cache.setTransformCache(sim->getAbsPose(), sim->getTransformCacheID());
		}
	}
}


void Sc::BodySim::onDeactivate()
{
	PX_ASSERT((!isKinematic()) || notInScene() || !readInternalFlag(BF_KINEMATIC_MOVED));	// kinematics should only get deactivated when no target is set.
																							// exception: object gets newly added, then the state change will happen later

	PX_ASSERT(mLLIslandHook.isManaged());
	PX_ASSERT(getInteractionScene().getLLIslandManager().getIsReadyForSleeping(mLLIslandHook));  // on deactivation, the sleep island generation system should have registered the node as ready for sleeping

	if (!readInternalFlag(BF_ON_DEATHROW))
	{
		// Set velocity to 0.
		// Note: this is also fine if the method gets called because the user puts something to sleep (this behavior is documented in the API)
		BodyCore& core = getBodyCore();
		PX_ASSERT(getBodyCore().getWakeCounter() == 0.0f);
		PxVec3 zero(0.f, 0.f, 0.f);
		core.setLinearVelocity(zero);
		core.setAngularVelocity(zero);
		setForcesToDefaults(!readInternalFlag(BF_DISABLE_GRAVITY));
	}

	if (!isArticulationLink())  // Articulations have their own sleep logic.
		getScene().onBodySleep(this);
}


//--------------------------------------------------------------
//
// BodyCore interface implementation
//
//--------------------------------------------------------------


void Sc::BodySim::notifyAddSpatialAcceleration()
{
	//The dirty flag is stored separately in the BodySim so that we query the dirty flag before going to 
	//the expense of querying the simStateData for the velmod values.
	raiseVelocityModFlag(VMF_ACC_DIRTY);
}

void Sc::BodySim::notifyClearSpatialAcceleration()
{
	//The dirty flag is stored separately in the BodySim so that we query the dirty flag before going to 
	//the expense of querying the simStateData for the velmod values.
	raiseVelocityModFlag(VMF_ACC_DIRTY);
}


void Sc::BodySim::notifyAddSpatialVelocity()
{
	//The dirty flag is stored separately in the BodySim so that we query the dirty flag before going to 
	//the expense of querying the simStateData for the velmod values.
	raiseVelocityModFlag(VMF_VEL_DIRTY);
}

void Sc::BodySim::notifyClearSpatialVelocity()
{
	//The dirty flag is stored separately in the BodySim so that we query the dirty flag before going to 
	//the expense of querying the simStateData for the velmod values.
	raiseVelocityModFlag(VMF_VEL_DIRTY);
}


void Sc::BodySim::postActorFlagChange(PxU32 oldFlags, PxU32 newFlags)
{
	// PT: don't convert to bool if not needed
	const PxU32 wasWeightless = oldFlags & PxActorFlag::eDISABLE_GRAVITY;
	const PxU32 isWeightless = newFlags & PxActorFlag::eDISABLE_GRAVITY;

	if (isWeightless != wasWeightless)
	{
		if (mVelModState == 0) raiseVelocityModFlag(VMF_GRAVITY_DIRTY);

		if (isWeightless)
			mInternalFlags |= BF_DISABLE_GRAVITY;
		else
			mInternalFlags &= ~BF_DISABLE_GRAVITY;
	}
}


void Sc::BodySim::postBody2WorldChange()
{
	mLLBody.saveLastCCDTransform();
	mLLBody.updatePoseDependenciesV(*getInteractionScene().getLowLevelContext());

	updateCachedShapeState();
	updateCachedTransforms(getScene().getInteractionScene().getLowLevelContext()->getTransformCache());
}



void Sc::BodySim::postSetWakeCounter(PxReal t, bool forceWakeUp)
{
	if ((t > 0.0f) || forceWakeUp)
		notifyNotReadyForSleeping();
	else
	{
		bool readyForSleep = checkSleepReadinessBesidesWakeCounter();
		if (readyForSleep)
			notifyReadyForSleeping();
	}
}


void Sc::BodySim::postSetKinematicTarget()
{
	PX_ASSERT(getBodyCore().getSimStateData(true));
	PX_ASSERT(getBodyCore().getSimStateData(true)->isKine());
	PX_ASSERT(getBodyCore().getSimStateData(true)->getKinematicData()->targetValid);

	raiseInternalFlag(BF_KINEMATIC_MOVED);	// Important to set this here already because trigger interactions need to have this information when being activated.
}


void Sc::BodySim::postSwitchToKinematic()
{
	initKinematicStateBase(getBodyCore());

	if(mLLIslandHook.isManaged())
		getInteractionScene().getLLIslandManager().setKinematic(mLLIslandHook, true);
}


void Sc::BodySim::postSwitchToDynamic()
{
	if(mLLIslandHook.isManaged())
		getInteractionScene().getLLIslandManager().setKinematic(mLLIslandHook, false);

	setForcesToDefaults(true);

	if (getConstraintGroup())
		getConstraintGroup()->rebuildProjectionTrees();

	// Force recalculation of sips
	setActorsInteractionsDirty(CoreInteraction::CIF_DIRTY_BODY_KINEMATIC, NULL, PX_INTERACTION_FLAG_FILTERABLE);

	clearInternalFlag(BF_KINEMATIC_MOVE_FLAGS);

	setIslandNodeType(IslandNodeInfo::eTWO_WAY);
}


//--------------------------------------------------------------
//
// Sleeping
//
//--------------------------------------------------------------


void Sc::BodySim::wakeUp()
{
	setActive(true);
	notifyWakeUp();
}


void Sc::BodySim::putToSleep()
{
	PX_ASSERT(getBodyCore().getWakeCounter() == 0.0f);
	PX_ASSERT(getBodyCore().getLinearVelocity().isZero());
	PX_ASSERT(getBodyCore().getAngularVelocity().isZero());
#ifdef _DEBUG
	// pending forces should have been cleared at this point
	const SimStateData* sd = getBodyCore().getSimStateData(false);
	if (sd)
	{
		const VelocityMod* vm = sd->getVelocityModData();
		PX_ASSERT(vm->linearPerSec.isZero() && vm->linearPerStep.isZero() && vm->angularPerSec.isZero() && vm->angularPerStep.isZero());
	}
#endif

	setActive(false);
	notifyPutToSleep();
	clearInternalFlag(BF_KINEMATIC_SETTLING);	// putToSleep is used when a kinematic gets removed from the scene while the sim is running and then gets re-inserted immediately.
												// We can move this code when we look into the open task of making buffered re-insertion more consistent with the non-buffered case.
}


void Sc::BodySim::internalWakeUp(PxReal wakeCounterValue)
{
	if(mArticulation)
		mArticulation->internalWakeUp(wakeCounterValue);
	else
		internalWakeUpBase(wakeCounterValue);
}


void Sc::BodySim::internalWakeUpArticulationLink(PxReal wakeCounterValue)
{
	PX_ASSERT(mArticulation);
	internalWakeUpBase(wakeCounterValue);
}


void Sc::BodySim::internalWakeUpBase(PxReal wakeCounterValue)	//this one can only increase the wake counter, not decrease it, so it can't be used to put things to sleep!
{
	if ((!isKinematic()) && (getBodyCore().getWakeCounter() < wakeCounterValue))
	{
		PX_ASSERT(wakeCounterValue > 0.0f);
		getBodyCore().setWakeCounterFromSim(wakeCounterValue);

		setActive(true);
		notifyWakeUpAndNotReadyForSleeping();
	}
}


void Sc::BodySim::notifyReadyForSleeping()
{
	PX_ASSERT(mLLIslandHook.isManaged());
	getInteractionScene().getLLIslandManager().notifyReadyForSleeping(mLLIslandHook);
}


void Sc::BodySim::notifyNotReadyForSleeping()
{
	PX_ASSERT(mLLIslandHook.isManaged());
	getInteractionScene().getLLIslandManager().notifyNotReadyForSleeping(mLLIslandHook);
}


void Sc::BodySim::notifyWakeUp()
{
	PX_ASSERT(mLLIslandHook.isManaged());
	getInteractionScene().getLLIslandManager().setAwake(mLLIslandHook);
}

void Sc::BodySim::notifyWakeUpAndNotReadyForSleeping()
{
	getInteractionScene().getLLIslandManager().setAwake(mLLIslandHook);
}

void Sc::BodySim::notifyPutToSleep()
{
	PX_ASSERT(mLLIslandHook.isManaged());
	getInteractionScene().getLLIslandManager().setAsleep(mLLIslandHook);
}


bool Sc::BodySim::sleepStateIntegrityCheck()
{
	if (isActive())
	{
		PX_ASSERT(mLLIslandHook.isManaged());
		return (getInteractionScene().getLLIslandManager().getIsInSleepingIsland(mLLIslandHook) == 0);
	}
	else
	{
		PX_ASSERT(mLLIslandHook.isManaged());
		return (getInteractionScene().getLLIslandManager().getIsInSleepingIsland(mLLIslandHook) != 0);
	}
}


void Sc::BodySim::resetSleepFilter()
{
	mSleepLinVelAcc = PxVec3(0);
	mSleepAngVelAcc = PxVec3(0);
}


PxReal Sc::BodySim::updateWakeCounter(PxReal dt, PxReal energyThreshold)
{
	// update the body's sleep state and 
	BodyCore& core = getBodyCore();

	PxReal wakeCounterResetTime = ScInternalWakeCounterResetValue;

	PxReal wc = core.getWakeCounter();
	if(wc < wakeCounterResetTime * 0.5f || wc < dt)
	{
		const PxTransform& body2World = getBody2World();

		// accumulate velocity in object space
		mSleepLinVelAcc += mLLBody.getLinearMotionVelocity(dt);
		mSleepAngVelAcc += body2World.q.rotateInv(mLLBody.getAngularMotionVelocity(dt));
		/*mSleepLinVelAcc += mLLBody.getLinearVelocity();
		mSleepAngVelAcc += body2World.q.rotateInv(mLLBody.getAngularVelocity());*/

		// calculate normalized energy: kinetic energy divided by mass

		const PxVec3 t = core.getInverseInertia();
		const PxVec3 inertia(1.0f/t.x, 1.0f/t.y, 1.0f/t.z);

		const PxReal angular = mSleepAngVelAcc.multiply(mSleepAngVelAcc).dot(inertia) * core.getInverseMass();
		const PxReal linear = mSleepLinVelAcc.magnitudeSquared();
		const PxReal normalizedEnergy = 0.5f * (angular + linear);

		// scale threshold by cluster factor (more contacts => higher sleep threshold)

		const PxReal clusterFactor = PxReal(1 + getNumCountedInteractions());
		const PxReal threshold = clusterFactor * energyThreshold;
	
		if (normalizedEnergy >= threshold)
		{
			PX_ASSERT(isActive());

			resetSleepFilter();
			const float factor = threshold == 0.f ? 2.0f : PxMin(normalizedEnergy/threshold, 2.0f);
			PxReal oldWc = wc;
			wc = factor * 0.5f * wakeCounterResetTime + dt * (clusterFactor - 1.0f);
			core.setWakeCounterFromSim(wc);
			if (oldWc == 0.0f)  // for the case where a sleeping body got activated by the system (not the user) AND got processed by the solver as well
				notifyNotReadyForSleeping();

			return wc;
		}
	}

	wc = PxMax(wc-dt, 0.0f);
	core.setWakeCounterFromSim(wc);
	return wc;
}

void Sc::BodySim::sleepCheck(PxReal dt)
{
	BodyCore& core = getBodyCore();
	PxReal wakeCounterResetTime = ScInternalWakeCounterResetValue;

	PxReal wc = updateWakeCounter(dt, core.getSleepThreshold());
	bool wakeCounterZero = (wc == 0.0f);

	if(wakeCounterZero)
	{
		notifyReadyForSleeping();
		resetSleepFilter();
	}
	
	//Dampen bodies that are just about to go to sleep

	const PxReal sleepDamping = core.getSleepDamping();
	if (sleepDamping > 0.0f && wc < wakeCounterResetTime*0.5f)
	{
		// zero dampening factor since it has artifacts, so we use a floor value
		PxReal d = PxMax(wc / (wakeCounterResetTime*0.5f), 0.05f);		
		d = Ps::pow(d, sleepDamping*dt);		//Compensate for time step
		
		core.setLinearVelocity(core.getLinearVelocity()*d);
		core.setAngularVelocity(core.getAngularVelocity()*d);
	}
}


//--------------------------------------------------------------
//
// Kinematics
//
//--------------------------------------------------------------

PX_FORCE_INLINE void Sc::BodySim::initKinematicStateBase(BodyCore&)
{
	PX_ASSERT(!readInternalFlag(BF_KINEMATIC_MOVED));

	setIslandNodeType(IslandNodeInfo::eONE_WAY_DOMINATOR);

	mLLBody.setAccelerationV(Cm::SpatialVector::zero());

	// Need to be before setting setRigidBodyFlag::KINEMATIC

	if (getConstraintGroup())
		getConstraintGroup()->rebuildProjectionTrees();
}


void Sc::BodySim::calculateKinematicVelocity(PxReal oneOverDt)
{
	PX_ASSERT(isKinematic());
	
	/*------------------------------------------------\
	| kinematic bodies are moved directly by the user and are not influenced by external forces
	| we simply determine the distance moved since the last simulation frame and 
	| assign the appropriate delta to the velocity. This vel will be used to shove dynamic
	| objects in the solver.
	| We have to do this like so in a delayed way, because when the user sets the target pos the dt is not
	| yet known.
	\------------------------------------------------*/
	PX_ASSERT(isActive());

	BodyCore& core = getBodyCore();

	if (readInternalFlag(BF_KINEMATIC_MOVED))
	{
		clearInternalFlag(BF_KINEMATIC_SETTLING);
		const SimStateData* kData = core.getSimStateData(true);
		PX_ASSERT(kData);
		PX_ASSERT(kData->isKine());
		PX_ASSERT(kData->getKinematicData()->targetValid);
		PxVec3 linVelLL, angVelLL;
		PxTransform targetPose = kData->getKinematicData()->targetPose;
		const PxTransform& currBody2World = getBody2World();

		//the kinematic target pose is now the target of the body (CoM) and not the actor.

		PxVec3 deltaPos = targetPose.p;
		deltaPos -= currBody2World.p;
		linVelLL = deltaPos * oneOverDt;

		PxQuat q = targetPose.q * currBody2World.q.getConjugate();

		if (q.w < 0)	//shortest angle.
			q = -q;

		PxReal angle;
 		PxVec3 axis;
		q.toRadiansAndUnitAxis(angle, axis);
		angVelLL = axis * angle * oneOverDt;

		core.setLinearVelocity(linVelLL);
		core.setAngularVelocity(angVelLL);

		// Moving a kinematic should trigger a wakeUp call on a higher level.
		PX_ASSERT(core.getCore().mWakeCounter>0);
		PX_ASSERT(isActive());
		
	}
	else
	{
		core.setLinearVelocity(PxVec3(0));
		core.setAngularVelocity(PxVec3(0));
	}
}

void Sc::BodySim::updateKinematicPose()
{
	PX_ASSERT(isKinematic());
	
	/*------------------------------------------------\
	| kinematic bodies are moved directly by the user and are not influenced by external forces
	| we simply determine the distance moved since the last simulation frame and 
	| assign the appropriate delta to the velocity. This vel will be used to shove dynamic
	| objects in the solver.
	| We have to do this like so in a delayed way, because when the user sets the target pos the dt is not
	| yet known.
	\------------------------------------------------*/
	PX_ASSERT(isActive());

	BodyCore& core = getBodyCore();

	if (readInternalFlag(BF_KINEMATIC_MOVED))
	{
		clearInternalFlag(BF_KINEMATIC_SETTLING);
		const SimStateData* kData = core.getSimStateData(true);
		PX_ASSERT(kData);
		PX_ASSERT(kData->isKine());
		PX_ASSERT(kData->getKinematicData()->targetValid);
		PxVec3 linVelLL, angVelLL;
		PxTransform targetPose = kData->getKinematicData()->targetPose;

		getBodyCore().getCore().body2World = targetPose;
	}
}



bool Sc::BodySim::deactivateKinematic()
{
	BodyCore& core = getBodyCore();
	if(readInternalFlag(BF_KINEMATIC_SETTLING))
	{
		clearInternalFlag(BF_KINEMATIC_SETTLING);
		core.setWakeCounterFromSim(0);	// For sleeping objects the wake counter must be 0. This needs to hold for kinematics too.
		notifyReadyForSleeping();
		notifyPutToSleep();
		setActive(false);
		return true;
	}
	else
	{
		clearInternalFlag(BF_KINEMATIC_MOVED);
		raiseInternalFlag(BF_KINEMATIC_SETTLING);
	}
	return false;
}



//--------------------------------------------------------------
//
// Miscellaneous
//
//--------------------------------------------------------------


void Sc::BodySim::updateForces(PxReal /*dt*/, PxReal /*oneOverDt*/, bool updateGravity, const PxVec3& gravity, bool hasStaticTouch, bool simUsesAdaptiveForce)
{
	const bool adaptiveFactorChange = mBodyConstraints != mLastBodyConstraints 
							 ||	hasStaticTouch ^ readInternalFlag(BF_HAS_STATIC_TOUCH);

	if (mVelModState != 0 ||
		(!readInternalFlag(BF_DISABLE_GRAVITY) &&
		 (updateGravity || (adaptiveFactorChange && simUsesAdaptiveForce))))
	{
		if(hasStaticTouch)
			raiseInternalFlag(BF_HAS_STATIC_TOUCH);
		else
			clearInternalFlag(BF_HAS_STATIC_TOUCH);

		PxReal adaptiveFactor;
		if (hasStaticTouch && mBodyConstraints > 1 && simUsesAdaptiveForce)
			adaptiveFactor = 1.0f / mBodyConstraints;
		else
			adaptiveFactor = 1.0f;

		const bool accDirty = readVelocityModFlag(VMF_ACC_DIRTY);
		const bool velDirty = readVelocityModFlag(VMF_VEL_DIRTY);

		BodyCore& bodyCore = getBodyCore();
		SimStateData* simStateData = NULL;
		PxVec3 linAcc, angAcc, linStep, angStep;
		if( (accDirty || velDirty) &&  ((simStateData = bodyCore.getSimStateData(false)) != NULL) )
		{
			VelocityMod* velmod = simStateData->getVelocityModData();
			linAcc = velmod->getLinearVelModPerSec()*adaptiveFactor;
			angAcc = velmod->getAngularVelModPerSec();
			linStep = velmod->getLinearVelModPerStep();
			angStep = velmod->getAngularVelModPerStep();
		}
		else
		{
			linAcc=PxVec3(0,0,0);
			angAcc=PxVec3(0,0,0);
			linStep=PxVec3(0,0,0);
			angStep=PxVec3(0,0,0);
		}

		if (!(readInternalFlag(BF_DISABLE_GRAVITY)))
			linAcc += gravity * adaptiveFactor;

		//The acceleration needs to be set so always set it.
		//We don't have the option to avoid the work the way we do with the velDirty flag.
		mLLBody.setAccelerationV(Cm::SpatialVector(linAcc, angAcc));

		if (velDirty)
			getBodyCore().updateVelocities(linStep, angStep);
	}

	setForcesToDefaults(readVelocityModFlag(VMF_ACC_DIRTY));

	mLastBodyConstraints = mBodyConstraints;
}


bool Sc::BodySim::isConnectedTo(const RigidSim& other, bool& collisionDisabled) const
{
	const Sc::Actor* actorToMatch;
	Cm::Range<Interaction*const> interactions;

	if (getActorInteractionCount() <= other.getActorInteractionCount())
	{
		interactions = getActorInteractions();
		actorToMatch = &other;
	}
	else
	{
		interactions = other.getActorInteractions();
		actorToMatch = this;
	}

	for(; !interactions.empty(); interactions.popFront())
	{
		Interaction* const interaction = interactions.front();
		if (interaction->getType() == PX_INTERACTION_TYPE_CONSTRAINTSHADER)
		{
			ConstraintInteraction* csi = static_cast<ConstraintInteraction*>(interaction);
			if ((&csi->getActor0() == actorToMatch) || (&csi->getActor1() == actorToMatch))
			{
				collisionDisabled = !((csi->getConstraint()->getCore().getFlags() & PxConstraintFlag::eCOLLISION_ENABLED));
				return true;
			}
		}
	}

	collisionDisabled = false;
	return false;
}


void Sc::BodySim::onConstraintDetach()
{
	PX_ASSERT(readInternalFlag(BF_HAS_CONSTRAINTS));

	Cm::Range<Interaction*const> interactions = getActorInteractions();

	for(; !interactions.empty(); interactions.popFront())
	{
		Interaction* const interaction = interactions.front();
		if (interaction->getType() == PX_INTERACTION_TYPE_CONSTRAINTSHADER)
			return;
	}

	clearInternalFlag(BF_HAS_CONSTRAINTS);  // There are no other constraint interactions left
}


void Sc::BodySim::setArticulation(Sc::ArticulationSim* a, PxReal wakeCounter, bool asleep)
{
	mArticulation = a; 
	if(a)
	{
		getBodyCore().setWakeCounterFromSim(wakeCounter);

		if (!asleep)
		{
			setActive(true);
			notifyWakeUpAndNotReadyForSleeping();
		}
		else
		{
			notifyReadyForSleeping();
			notifyPutToSleep();
			setActive(false);
		}
	}
}
