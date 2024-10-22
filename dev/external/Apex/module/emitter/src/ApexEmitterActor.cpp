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

#include "NxApex.h"
#include "PsShare.h"
#include "NiInstancedObjectSimulation.h"
#include "ApexEmitterActor.h"
#include "ApexEmitterAsset.h"
#include "EmitterScene.h"
#include "EmitterGeomExplicit.h"
#include "NiApexSDK.h"
#include "NiApexScene.h"
#include "NiApexRenderDebug.h"

#if NX_SDK_VERSION_MAJOR == 2
#include "NxActor.h"
#include "NxBounds3.h"
#include "NxSphere.h"
#include "NxScene.h"
#include "NxFromPx.h"
#elif NX_SDK_VERSION_MAJOR == 3
#include "PxRigidDynamic.h"
#endif

namespace physx
{
namespace apex
{
namespace emitter
{

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

ApexEmitterActor::ApexEmitterActor(const NxApexEmitterActorDesc& desc, ApexEmitterAsset& asset, NxResourceList& list, EmitterScene& scene)
	: mNxScene(NULL)
	, mAttachActor(NULL)
	, mInjector(NULL)
	, mAsset(&asset)
	, mScene(&scene)
	, mLastNonEmptyOverlapAABB(physx::PxBounds3::empty())
	, mPose(desc.initialPose)
	, mFirstStartEmitCall(true)
	, mEmitAssetParticles(desc.emitAssetParticles)
	, mPersist(false)
	, mRemainder(0)
	, mIsOldPoseInitialized(false)
	, mOverlapTestCollisionGroups(desc.overlapTestCollisionGroups)
	, mShouldUseGroupsMask(false)
	, mExplicitGeom(NULL)
	, mTickTask(*this)
{
	mEmitterValidateCallback = NULL;
	mIOS = NULL;
	mRand.setSeed(scene.mApexScene->getSeed());

	list.add(*this);            // Add self to asset's list of actors

	mPoses.pushBack(mPose);

	/* Get initial values from authored asset */
	mDensityRange = mAsset->getDensityRange();
	mRateRange = mAsset->getRateRange();
	mVelocityRange = mAsset->getVelocityRange();
	mLifetimeRange = mAsset->getLifetimeRange();
	mAttachActor = desc.attachActor;
	mAttachRelativePose = desc.attachRelativePose;
	mDoEmit = mAsset->mGeom->getNxEmitterGeom()->getEmitterType() == NxApexEmitterType::NX_ET_DENSITY_ONCE;
	mEmitterVolume = mAsset->mGeom->computeEmitterVolume();

	mEmitDuration = mDescEmitDuration = desc.emitterDuration;

	// create an instance of the explicit geometry for this actor's particles
	// if the asset is an explicit emitter
	if (mAsset->mGeom->getNxEmitterGeom()->isExplicitGeom())
	{
		mExplicitGeom = PX_NEW(EmitterGeomExplicit)();
		PX_ASSERT(mExplicitGeom);
	}

	NxIosAsset* iosAsset = mAsset->mIosAssetTracker.getIosAssetFromName(mAsset->getInstancedObjectSimulatorTypeName(),
	                       mAsset->getInstancedObjectSimulatorAssetName());
	if (!iosAsset)
	{
		return;
	}

	const char* iofxAssetName = mAsset->getInstancedObjectEffectsAssetName();

	NxApexActor* nxActor = iosAsset->createIosActor(*scene.mApexScene, iofxAssetName);
	NiInstancedObjectSimulation* ios = NULL;
	if (nxActor)
	{
		ApexActor* aa = NiGetApexSDK()->getApexActor(nxActor);
		if (aa)
		{
			ios = DYNAMIC_CAST(NiInstancedObjectSimulation*)(aa);
		}
	}
	if (!ios)
	{
		return;
	}
	
	mObjectRadius = ios->getObjectRadius();
	mInjector = ios->allocateInjector(iofxAssetName);
	if (!mInjector)
	{
		return;
	}
	mIOS = ios;
	mInjector->addSelfToContext(*this);
	setLodParamDesc(mAsset->getLodParamDesc());

	addSelfToContext(*scene.mApexScene->getApexContext());    // Add self to ApexScene
	addSelfToContext(scene);    // Add self to EmitterScene's list of actors

	mValid = true;
}

ApexEmitterActor::~ApexEmitterActor()
{
}

void ApexEmitterActor::setPreferredRenderVolume(physx::apex::NxApexRenderVolume* vol)
{
	if (mInjector)
	{
		mInjector->setPreferredRenderVolume(vol);
	}
}

void ApexEmitterActor::submitTasks()
{
	if (mInjector != 0)
	{
		mScene->mApexScene->getTaskManager()->submitUnnamedTask(mTickTask);
	}
}

void ApexEmitterActor::setTaskDependencies()
{
	if (mInjector != 0)
	{
		mTickTask.finishBefore(mInjector->getCompletionTaskID());
		mTickTask.finishBefore(mScene->mApexScene->getTaskManager()->getNamedTask(AST_PHYSX_SIMULATE));
	}
}

void ApexEmitterActor::fetchResults()
{
}

void ApexEmitterActor::removeActorAtIndex(physx::PxU32 index)
{
	// An injector has been deleted
	PX_ASSERT(mInjector == (NiIosInjector*) mActorArray[index]);
	mInjector = NULL;
	ApexContext::removeActorAtIndex(index);
	release();
}

/* Must be defined inside CPP file, since they require knowledge of asset class */
NxApexAsset*            ApexEmitterActor::getOwner() const
{
	return (NxApexAsset*) mAsset;
}
NxApexEmitterAsset*     ApexEmitterActor::getEmitterAsset() const
{
	return mAsset;
}

void ApexEmitterActor::release()
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	mAsset->releaseEmitterActor(*this);
}


void ApexEmitterActor::destroy()
{
	ApexActor::destroy();

	if (mExplicitGeom)
	{
		mExplicitGeom->destroy();
		mExplicitGeom = 0;
	}

	setPhysXScene(NULL);

	if (mInjector)
	{
		mInjector->release();
	}
	mInjector = NULL;
	mIOS = NULL;

	delete this;
}


void ApexEmitterActor::setLodParamDesc(const NxEmitterLodParamDesc& d)
{
	mLodParams = d;
	PX_ASSERT(mInjector);
	if (mInjector)
		mInjector->setLODWeights(d.maxDistance, d.distanceWeight, d.speedWeight,
		                         d.lifeWeight, d.separationWeight, d.bias);
}


void ApexEmitterActor::getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly)
{
	PX_UNUSED(min);
	PX_UNUSED(max);
	PX_UNUSED(intOnly);
	APEX_INVALID_OPERATION("not implemented");
}


physx::PxF32 ApexEmitterActor::getActivePhysicalLod()
{
	APEX_INVALID_OPERATION("NxApexEmitterActor does not support this operation");
	return -1.0f;
}


void ApexEmitterActor::forcePhysicalLod(physx::PxF32 lod)
{
	PX_UNUSED(lod);
	APEX_INVALID_OPERATION("not implemented");
}

#ifdef WITHOUT_DEBUG_VISUALIZE
void ApexEmitterActor::visualize(NiApexRenderDebug&)
{
}
#else
void ApexEmitterActor::visualize(NiApexRenderDebug& renderDebug)
{
	if (!mScene->mEmitterDebugRenderParams->apexEmitterParameters.VISUALIZE_APEX_EMITTER_ACTOR)
	{
		return;
	}

	renderDebug.setCurrentUserPointer((void*)(NxApexActor*)this);

	if (mAsset->mGeom)
	{
		mAsset->mGeom->visualize(mPose, renderDebug);
	}

	renderDebug.pushRenderState();
	renderDebug.setCurrentTextScale(2.0f);

	PxMat44 cameraFacingPose((mScene->mApexScene->getViewMatrix(0)).inverseRT());
	PxVec3 textLocation = this->getGlobalPose().column3.getXYZ();
	cameraFacingPose.setPosition(textLocation);
	//visualize actor name
	if (mScene->mEmitterDebugRenderParams->apexEmitterParameters.VISUALIZE_APEX_EMITTER_ACTOR_NAME &&
	        mScene->mEmitterDebugRenderParams->apexEmitterParameters.THRESHOLD_DISTANCE_APEX_EMITTER_ACTOR_NAME >
	        (-mScene->mApexScene->getEyePosition(0) + textLocation).magnitude())
	{
		renderDebug.debugOrientedText(cameraFacingPose, " %s %s", this->getOwner()->getObjTypeName(), this->getOwner()->getName());
	}
	//visualize actor pose
	if (mScene->mEmitterDebugRenderParams->apexEmitterParameters.VISUALIZE_APEX_EMITTER_ACTOR_POSE &&
	        mScene->mEmitterDebugRenderParams->apexEmitterParameters.THRESHOLD_DISTANCE_APEX_EMITTER_ACTOR_POSE >
	        (-mScene->mApexScene->getEyePosition(0) + textLocation).magnitude())
	{
		renderDebug.debugAxes(this->getGlobalPose(), 1);
	}

	if (mScene->mEmitterDebugRenderParams->apexEmitterParameters.VISUALIZE_TOTAL_INJECTED_AABB)
	{
		if (!mOverlapAABB.isEmpty())
		{
			mLastNonEmptyOverlapAABB.include(mOverlapAABB);
		}
		renderDebug.pushRenderState();
		renderDebug.setCurrentColor(renderDebug.getDebugColor(physx::DebugColors::Red));
		renderDebug.debugBound(mLastNonEmptyOverlapAABB);
		renderDebug.popRenderState();
	}

	renderDebug.popRenderState();

	renderDebug.setCurrentUserPointer(NULL);
}
#endif


NxEmitterExplicitGeom* ApexEmitterActor::isExplicitGeom()
{
	if (mAsset->mGeom->getNxEmitterGeom()->isExplicitGeom())
	{
		// return the actor's explicit geometry
		return const_cast<NxEmitterExplicitGeom*>(mExplicitGeom->getNxEmitterGeom()->isExplicitGeom());
	}
	else
	{
		return NULL;
	}
}


void ApexEmitterActor::startEmit(bool persistent)
{
	// persistent remains a global setting
	// simply store the current pose, emit all particles in the list for every pose
	// fix bug: we don't want two poses on the first frame unless startEmit is called twice
	if (mFirstStartEmitCall)
	{
		mFirstStartEmitCall = false;
		mPoses.clear();
	}

	mPoses.pushBack(mPose);

	mEmitDuration = mDescEmitDuration;

	mDoEmit = true;
	mPersist = persistent;
}


void ApexEmitterActor::stopEmit()
{
	mDoEmit = false;
	mPersist = false;
}


void ApexEmitterActor::tick()
{
	mOverlapAABB = physx::PxBounds3::empty();

	// Clamp emission time to a min of 15FPS, to prevent extremely long frames
	// from generating too many particles when rate emitters are in use.
	physx::PxF32 dt = PxMin(1 / 15.0f, mScene->mApexScene->getElapsedTime());

	if (!mInjector || !mAsset->mGeom)
	{
		return;
	}

	if (mAttachActor)
	{
		SCOPED_PHYSX_LOCK_WRITE(*mScene->mApexScene);

#if NX_SDK_VERSION_MAJOR == 2
		NxMat34 actorPose = mAttachActor->getGlobalPose();
		physx::PxMat34Legacy pxActorPose;
		PxFromNxMat34(pxActorPose, actorPose);
		physx::PxMat34Legacy newPose;
		newPose.multiply(mAttachRelativePose, pxActorPose);
		//mAttachRelativePose.multiply(pxActorPose, newPose);
		setCurrentPose(newPose);
#elif NX_SDK_VERSION_MAJOR == 3
		PxTransform t = mAttachActor->isRigidDynamic()->getGlobalPose();
		PxMat44 m(t.q);
		m.setPosition(t.p);
		physx::PxMat44 newPose;
		newPose = mAttachRelativePose*m;
		setCurrentPose(m);
#endif
	}

	physx::PxF32 curdensity = mAsset->mModule->getCurrentValue(mDensityRange, EMITTER_SCALABLE_PARAM_DENSITY);

	NxEmitterGeom* nxGeom = mAsset->mGeom->getNxEmitterGeom();
	if (mDoEmit && nxGeom->isExplicitGeom())
	{
		mNewPositions.clear();
		mNewVelocities.clear();
		mNewUserData.clear();

		// compute the fill positions from both the asset's and actor's explicit geom

		for (PxU32 i = 0; i < mPoses.size(); i++)
		{
			if (mEmitAssetParticles)
			{
				DYNAMIC_CAST(EmitterGeomExplicit*)(mAsset->mGeom)->computeFillPositions(mNewPositions, mNewVelocities, &mNewUserData, mPoses[i], curdensity, mOverlapAABB, mRand);
			}

			DYNAMIC_CAST(EmitterGeomExplicit*)(mExplicitGeom)->computeFillPositions(mNewPositions, mNewVelocities, &mNewUserData, mPoses[i], curdensity, mOverlapAABB, mRand);
		}
		mPoses.clear();

		if (mNewPositions.size() > 0)
		{
			// do a big overlap test first for all new particles
			bool doOverlapTests = overlapsWithCollision(mOverlapAABB);

			physx::PxU32 positionsSize = mNewPositions.size();
			physx::PxU32 velocitiesSize = mNewVelocities.size();
			physx::PxU32 userDataSize = mNewUserData.size();

			mNewObjectArray.clear();
			mNewObjectArray.reserve(positionsSize);
			for (physx::PxU32 i = 0; i < positionsSize; i++)
			{
				if (!doOverlapTests || !overlapsWithCollision(mNewPositions[i]))
				{
					IosNewObject& obj = mNewObjectArray.insert();

					obj.initialPosition = mNewPositions[i];
					if (i < velocitiesSize && mNewVelocities[i].isZero() == false)
					{
						obj.initialVelocity = mNewVelocities[i];
					}
					else
					{
						obj.initialVelocity = mRand.getScaled(mVelocityRange.minimum, mVelocityRange.maximum);
					}

					mPose.M.multiply(obj.initialVelocity, obj.initialVelocity);

					obj.lifetime = mRand.getScaled(mLifetimeRange.minimum, mLifetimeRange.maximum);
					obj.iofxActorID	= NiIofxActorID(0);
					obj.lodBenefit	= 0.0f;

					obj.userData = 0;
					if (i < userDataSize)
					{
						obj.userData = mNewUserData[i];
					}
				}
			}
			if (mNewObjectArray.size())
			{
				mInjector->createObjects(mNewObjectArray.size(), &mNewObjectArray[0]);
			}
		}

		if (mPersist)
		{
			mDoEmit = true;
		}
		else if (mEmitDuration > 0.0f)
		{
			mEmitDuration = PxMax(0.0f, mEmitDuration - dt);
			mDoEmit = (mEmitDuration > 0.0f);
		}
		else
		{
			mDoEmit = false;
		}

		if (mDoEmit)
		{
			// set the last pose as the single pose in the list if we're persisting
			mPoses.pushBack(mPose);
		}

		return;
	}

	if (mDoEmit)
	{
		physx::PxU32 fillNumParticles;
		fillNumParticles = (physx::PxU32)(curdensity * mEmitterVolume);

		switch (nxGeom->getEmitterType())
		{
		case NxApexEmitterType::NX_ET_RATE:
		{
			emitObjects(computeNbEmittedFromRate(dt), true);
		}
		break;

		case NxApexEmitterType::NX_ET_DENSITY_ONCE:
		{
			emitObjects(fillNumParticles, true);
		}
		break;

		case NxApexEmitterType::NX_ET_DENSITY_BRUSH:
			if (mIsOldPoseInitialized)
			{
				// compute newly covered volume
				// SJB: Notes
				// This is bizarre: we do 100 random samples to guess the amount of new area
				// we have to work with, then generate that percentage of new particles?
				physx::PxF32 percent = mAsset->mGeom->computeNewlyCoveredVolume(mOldPose, mPose, mRand);
				physx::PxU32 toEmitNum = (physx::PxU32)((physx::PxF32) fillNumParticles * percent);
				emitObjects(toEmitNum, false);
			}
			else
			{
				emitObjects(fillNumParticles, true);
			}
			break;

		case NxApexEmitterType::NX_ET_FILL:
		{
			mNewPositions.clear();
			mNewVelocities.clear();

			mAsset->mGeom->computeFillPositions(mNewPositions, mNewVelocities, mPose, mObjectRadius, mOverlapAABB, mRand);
			if (mNewPositions.size() > mAsset->getMaxSamples())
			{
				mNewPositions.resize(mAsset->getMaxSamples());
			}
			emitObjects(mNewPositions, mOverlapAABB);
		}
		break;

		default:
			PX_ASSERT(!"emitterType not supported.");
			break;
		}

		if (mPersist)
		{
			mDoEmit = true;
		}
		else if (mEmitDuration > 0.0f)
		{
			mEmitDuration = PxMax(0.0f, mEmitDuration - dt);
			mDoEmit = (mEmitDuration > 0.0f);
		}
		else
		{
			mDoEmit = false;
		}
	}

	mOldPose = mPose;
	mIsOldPoseInitialized = true;
}


physx::PxU32 ApexEmitterActor::computeNbEmittedFromRate(physx::PxF32 dt)
{
	physx::PxF32 currate = mAsset->mModule->getCurrentValue(mRateRange, EMITTER_SCALABLE_PARAM_RATE);

	// compute number of particles to be spawned
	physx::PxF32 nbEmittedReal = currate * dt;
	nbEmittedReal += mRemainder;
	physx::PxU32 currentNbEmitted = (physx::PxU32) PxFloor(nbEmittedReal);
	mRemainder = nbEmittedReal - (physx::PxF32) currentNbEmitted;

	return currentNbEmitted;
}


bool ApexEmitterActor::overlapsWithCollision(const physx::PxBounds3& bounds)
{
	if ((mOverlapTestCollisionGroups > 0 || mShouldUseGroupsMask) && mNxScene)
	{
		PX_PROFILER_PERF_SCOPE("EmitterActorOverlapAABBSceneQuery");
#if NX_SDK_VERSION_MAJOR == 2
		NxBounds3 nxBounds;
		NxFromPxBounds3(nxBounds, bounds);
		return mNxScene->checkOverlapAABB(nxBounds,
		                                  NX_ALL_SHAPES,
		                                  mOverlapTestCollisionGroups,
		                                  getOverlapTestCollisionGroupsMask());
#else
		PX_UNUSED(bounds);
#endif
	}
	return false;
}


bool ApexEmitterActor::overlapsWithCollision(const physx::PxVec3& pos)
{
	if ((mOverlapTestCollisionGroups > 0 || mShouldUseGroupsMask) && mNxScene)
	{
		PX_PROFILER_PERF_SCOPE("EmitterActorOverlapSphereSceneQuery");
#if NX_SDK_VERSION_MAJOR == 2
		return mNxScene->checkOverlapSphere(NxSphere(NxFromPxVec3Fast(pos),
		                                    mObjectRadius),
		                                    NX_ALL_SHAPES,
		                                    mOverlapTestCollisionGroups,
		                                    getOverlapTestCollisionGroupsMask());
#else
		PX_UNUSED(pos);
#endif
	}

	return false;
}


void ApexEmitterActor::emitObjects(physx::PxU32 toEmitNum, bool useFullVolume)
{
	if (!toEmitNum)
	{
		return;
	}

	physx::PxVec3 emitterOrigin = getGlobalPose().getPosition();
	// TODO: This could obviously be more efficient
	physx::PxU32 emittedCount = 0;
	mNewObjectArray.clear();
	mNewObjectArray.reserve(mAsset->getMaxSamples());
	for (physx::PxU32 i = 0; i < mAsset->getMaxSamples(); i++)
	{
		physx::PxVec3 pos;
		if (useFullVolume)
		{
			pos = mAsset->mGeom->randomPosInFullVolume(mPose, mRand);
		}
		else
		{
			pos = mAsset->mGeom->randomPosInNewlyCoveredVolume(mPose, mOldPose, mRand);
		}

		if ( mEmitterValidateCallback )
		{
			if ( !mEmitterValidateCallback->validateEmitterPosition(emitterOrigin, pos))
			{
				continue;
			}
		}


		mOverlapAABB.include(pos);

		IosNewObject& obj = mNewObjectArray.insert();
		obj.initialPosition = pos;
		obj.initialVelocity = mRand.getScaled(mVelocityRange.minimum, mVelocityRange.maximum);
		mPose.M.multiply(obj.initialVelocity, obj.initialVelocity);
		obj.lifetime = mRand.getScaled(mLifetimeRange.minimum, mLifetimeRange.maximum);
		obj.iofxActorID	= NiIofxActorID(0);
		obj.lodBenefit	= 0.0f;
		obj.userData = 0;

		if (++emittedCount >= toEmitNum)
		{
			break;
		}
	}

	// first check if there's any overlap at all
	if (mNewObjectArray.size() && overlapsWithCollision(mOverlapAABB))
	{
		// there are overlaps, so pull out the overlapping particles
		for (physx::PxU32 i = 0; i < mNewObjectArray.size(); i++)
		{
			if (overlapsWithCollision(mNewObjectArray[i].initialPosition))
			{
				mNewObjectArray.replaceWithLast(i--);
			}
		}
	}

	if (mNewObjectArray.size())
	{
		mInjector->createObjects(mNewObjectArray.size(), &mNewObjectArray[0]);
	}

	if (emittedCount < toEmitNum)
	{
		APEX_DEBUG_WARNING("Not all objects have been emitted: "
		                   "\n Emitter may be failing overlap tests"
		                   "\n The individual object radius may be too big to fit in the emitter"
		                   "\n The emitter rate or density may have requested more than the asset's 'maxSamples' parameter\n");
	}
}


void ApexEmitterActor::emitObjects(const physx::Array<physx::PxVec3>& positions, physx::PxBounds3& bounds)
{
	if (positions.empty())
	{
		return;
	}

	// do a big overlap test first for all new particles
	bool doOverlapTests = overlapsWithCollision(bounds);

	// TODO: This could obviously be more efficient
	mNewObjectArray.clear();

	physx::PxVec3 emitterOrigin = getGlobalPose().getPosition();

	for (physx::PxU32 i = 0; i <  positions.size(); i++)
	{
		physx::PxVec3 position = positions[i];
		if ( mEmitterValidateCallback )
		{
			if ( !mEmitterValidateCallback->validateEmitterPosition(emitterOrigin, position))
			{
				continue;
			}
		}
		if (doOverlapTests && overlapsWithCollision(position))
		{
			continue;
		}
		IosNewObject& obj = mNewObjectArray.insert();
		obj.initialPosition = position;
		obj.initialVelocity = mRand.getScaled(mVelocityRange.minimum, mVelocityRange.maximum);
		mPose.M.multiply(obj.initialVelocity, obj.initialVelocity);
		obj.lifetime = mRand.getScaled(mLifetimeRange.minimum, mLifetimeRange.maximum);
		obj.iofxActorID	= NiIofxActorID(0);
		obj.lodBenefit	= 0.0f;
		obj.userData = 0;
	}

	if (mNewObjectArray.size())
	{
		mInjector->createObjects(mNewObjectArray.size(), &mNewObjectArray[0]);
	}
}


physx::PxF32 ApexEmitterActor::getBenefit()
{
	return mInjector->getBenefit();
}

physx::PxF32 ApexEmitterActor::setResource(physx::PxF32 suggested, physx::PxF32 maxRemaining, physx::PxF32 relativeBenefit)
{
	return mInjector->setResource(suggested, maxRemaining, relativeBenefit);
}



#if NX_SDK_VERSION_MAJOR == 2
void ApexEmitterActor::setOverlapTestCollisionGroupsMask(NxGroupsMask* m)
{
	if (m != NULL)
	{
		mOverlapTestCollisionGroupsMask = *m;
		mShouldUseGroupsMask = true;
	}
	else
	{
		mShouldUseGroupsMask = false;
	}
}
#endif

PxU32	ApexEmitterActor::getActiveParticleCount() const
{
	PxU32 ret = 0;

	if ( mInjector )
	{
		ret = mInjector->getActivePaticleCount();
	}

	return ret;
}

void	ApexEmitterActor::setDensityGridPosition(const physx::PxVec3 &pos)
{
	if ( mIOS )
	{
		mIOS->setDensityOrigin(pos);
	}
}

}
}
} // namespace physx::apex
