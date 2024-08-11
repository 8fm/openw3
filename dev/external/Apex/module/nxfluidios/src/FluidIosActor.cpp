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

#include "NxApexDefs.h"
#if NX_SDK_VERSION_MAJOR == 2

#include "NiApexScene.h"
#include "PsShare.h"
#include "NxFluidIosActor.h"
#include "FluidIosActor.h"
#include "FluidIosAsset.h"
#include "NxIofxAsset.h"
#include "FluidParticleInjector.h"
#include "ModuleFluidIos.h"
#include "NiApexRenderDebug.h"
#include "NiApexAuthorableObject.h"
#include "NxFromPx.h"
#include "NiModuleIofx.h"
#include "NiIofxManager.h"
#include "ApexMirroredArray.h"

#include <NxCompartment.h>
#include <NxScene.h>
#include <fluids/NxFluid.h>

#include "PsSort.h"

#define VIZ_RESCALE_MS	2000

namespace physx
{
namespace apex
{
namespace nxfluidios
{

static const PxU32 MaxNewParticleCount = 4096;

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

FluidIosActor::FluidIosActor(
    NxResourceList& list,
    FluidIosAsset& asset,
    FluidIosScene& scene,
    NxIofxAsset* iofxAsset)
	: mAsset(&asset)
	, mParticleScene(&scene)
	, mFluid(NULL)
	, mCompartment(NULL)
	, mIofxMgr(NULL)
	, mParticleCount(0)
	, mMaxStateID(0)
	, mNumDeletedIDs(0)
	, mLeastBenefit(0.0f)
	, mCreatedParticleCount(0)
	, mPrepareBenefitTask(*this)
	, mUpdateTask(*this)
	, mPostUpdateTask(*this)
	, mNxFluidBroken(false)
#if !defined(WITHOUT_DEBUG_VISUALIZE)
	, mLodRelativeBenefit(0.0f)
	, mDebugRenderGroupID(0)
	, mTotalElapsedMS(0)
	, mLastVisualizeMS(0)
#endif
{
	list.add(*this);

	mCollisionGroup = 0;
	if (mAsset->mParams->collisionGroupName && mAsset->mParams->collisionGroupName[0])
	{
		NiResourceProvider* nrp = NiGetApexSDK()->getInternalResourceProvider();
		NxResID	collisionGroupNS = mAsset->mModule->mSdk->getCollisionGroupNameSpace();
		NxResID id = nrp->createResource(collisionGroupNS, mAsset->mParams->collisionGroupName);
		mCollisionGroup = (PxU16)(size_t)(nrp->getResource(id));
		if (mCollisionGroup >= 32)
		{
			APEX_INVALID_PARAMETER("APEX Named Resource (fluid collision group) '%s' = %d.  Must be less than 32.",
			                       mAsset->mParams->collisionGroupName, mCollisionGroup);
		}
	}

	mMaxParticleCount = physx::PxClamp(mAsset->mParams->maxParticleCount, (physx::PxU32)1, (physx::PxU32)65535);

	NiIofxManagerDesc desc;
	desc.iosAssetName         = mAsset->getName();
	desc.iosSupportsDensity   = mAsset->getSupportsDensity();
	desc.iosOutputsOnDevice   = false;
	desc.iosSupportsCollision = true;
	desc.maxObjectCount       = mMaxParticleCount;
	desc.maxInputCount        = mMaxParticleCount;
	desc.maxInStateCount      = mMaxParticleCount;

	NiModuleIofx* nim = mAsset->mModule->getNiModuleIofx();
	if (nim)
	{
		mIofxMgr = nim->createActorManager(*mParticleScene->mApexScene, *iofxAsset, desc);
		mIofxMgr->createSimulationBuffers(mBufDesc);

		for (PxU32 i = 0 ; i < mMaxParticleCount ; i++)
		{
			mBufDesc.pmaInStateToInput->get(i) = NiIosBufferDesc::NOT_A_PARTICLE;
		}
	}

	addSelfToContext(*scene.mApexScene->getApexContext());		// add self to NxApexScene (can cause NxFluid creation)
	addSelfToContext(*DYNAMIC_CAST(ApexContext*)(&scene));		// add self to FluidIosScene
}

FluidIosActor::~FluidIosActor()
{
}

void FluidIosActor::release()
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	mAsset->releaseIosActor(*this);
}

void FluidIosActor::destroy()
{
	ApexActor::destroy();

	// destroy NxFluid
	setPhysXScene(NULL);

	// remove ourself from our asset's resource list, in case releasing our emitters
	// causes our asset's resource count to reach zero and for it to be released.
	ApexResource::removeSelf();

	// Release all injectors, releasing all emitters and their IOFX asset references
	while (mInjectors.getSize())
	{
		FluidParticleInjector* inj = DYNAMIC_CAST(FluidParticleInjector*)(mInjectors.getResource(mInjectors.getSize() - 1));
		inj->release();
	}

	if (mIofxMgr)
	{
		mIofxMgr->release();
	}

	delete this;
}

void FluidIosActor::setPhysXScene(NxScene* scene)
{
	if (scene)
	{
		putInScene(scene);
	}
	else
	{
		removeFromScene();
	}
}


NxScene* FluidIosActor::getPhysXScene() const
{
	if (mFluid)
	{
		return &mFluid->getScene();
	}
	else
	{
		return NULL;
	}
}

void FluidIosActor::putInScene(NxScene* scene)
{
	if (!mIofxMgr)
	{
		return;
	}

	PX_ASSERT(!mFluid);

	NxFluidDesc	fluidDesc;
	mAsset->getFluidTemplate(fluidDesc);

	if (fluidDesc.simulationMethod == NX_F_SPH)
	{
		if (mParticleScene->getSPHCompartment())
		{
			fluidDesc.compartment = const_cast<NxCompartment*>(mParticleScene->getSPHCompartment());
		}
		else
		{
			fluidDesc.compartment = const_cast<NxCompartment*>(mParticleScene->getCompartment());
		}
	}
	else
	{
		fluidDesc.compartment = const_cast<NxCompartment*>(mParticleScene->getCompartment());
	}

	mCompartment = fluidDesc.compartment;
	fluidDesc.collisionGroup = mCollisionGroup;

	fluidDesc.maxParticles = mMaxParticleCount;
	mMaxInsertionCount = physx::PxMin(mMaxParticleCount, (physx::PxU32) MaxNewParticleCount);

	// set hardware flag only if hardware is available
	if (NxGetApexSDK()->getPhysXSDK()->getHWVersion() == 0)
	{
		fluidDesc.flags &= ~NX_FF_HARDWARE;
	}

	if ((fluidDesc.flags & NX_FF_PRIORITY_MODE) || fluidDesc.numReserveParticles)
	{
		APEX_DEBUG_INFO("FluidIosActor::putInScene - Clearing NX_FF_PRIORITY_MODE settings");
		fluidDesc.flags &= ~NX_FF_PRIORITY_MODE;
		fluidDesc.numReserveParticles = 0;
	}

	// HL: this is where the particle radius is set. Before, the collision distance was combined
	// with the collisionAspect of the renderMesh -> 0.5f * mCollisionAspect. Now that we have
	// several meshes for one NxFluid it is not clear which collisionAspect to use.  Also it is less
	// confusing for the user if we leave out the collisionAspect. If radius is set it should
	// directly be the collisionDistance of the particle: (0.5 * 1/radius)
	fluidDesc.restParticlesPerMeter = 0.5f / mAsset->mParams->particleRadius;

#if 0
	// LRR: we want collision distance to always be 0.1 (so particles don't float above the ground)
	// collision distace = CDM / RPPM = 0.1 * RPPM / RPPM = 0.1
	// therefore, we must set CDM = 0.1 * RPPM
	fluidDesc.collisionDistanceMultiplier = 0.1f * fluidDesc.restParticlesPerMeter;
#endif

#if ( !defined(PX_PS3) && NX_SDK_VERSION_NUMBER == 283 )
	fluidDesc.flags |= NX_FF_ENABLE_STATIC_FRICTION;
#endif

	mInputToNx.resize(mMaxParticleCount);
	mNxToState.resize(mMaxParticleCount, 0);
	mLifetime.resize(mMaxParticleCount);
	mParticleData.resize(mMaxParticleCount);
	mDeletedIDs.resize(mMaxParticleCount);

	mDelBuffer.reserve(mMaxParticleCount);
	mCreatedIDs.resize(mMaxInsertionCount);
	mAddBuffer.reserve(mMaxInsertionCount);

	mForceDeleteArray.reserve(mMaxParticleCount);

	fluidDesc.particlesWriteData.numParticlesPtr = &mParticleCount;
	fluidDesc.particlesWriteData.bufferId = &mInputToNx[0];
	fluidDesc.particlesWriteData.bufferIdByteStride = sizeof(PxU32);
	fluidDesc.particlesWriteData.bufferLife = &mLifetime[0];
	fluidDesc.particlesWriteData.bufferLifeByteStride = sizeof(PxF32);

	fluidDesc.particlesWriteData.bufferPos = &mBufDesc.pmaPositionMass->getPtr()->x;
	fluidDesc.particlesWriteData.bufferPosByteStride = sizeof(PxVec4);
	fluidDesc.particlesWriteData.bufferDensity = mBufDesc.pmaDensity ? mBufDesc.pmaDensity->getPtr() : 0;
	fluidDesc.particlesWriteData.bufferDensityByteStride = sizeof(PxF32);

	fluidDesc.particlesWriteData.bufferVel = &mBufDesc.pmaVelocityLife->getPtr()->x;
	fluidDesc.particlesWriteData.bufferVelByteStride = sizeof(PxVec4);

	if (mBufDesc.pmaCollisionNormalFlags)
	{
		// Note, if NxfluidDesc.flags.NX_FF_COLLISION_TWOWAY is set, this won't be filled
		fluidDesc.particlesWriteData.bufferCollisionNormal = &mBufDesc.pmaCollisionNormalFlags->getPtr()->x;
		fluidDesc.particlesWriteData.bufferCollisionNormalByteStride = sizeof(PxVec4);
		fluidDesc.particlesWriteData.bufferFlag = (PxU32*) &mBufDesc.pmaCollisionNormalFlags->getPtr()->w;
		fluidDesc.particlesWriteData.bufferFlagByteStride = sizeof(PxVec4);
	}

	fluidDesc.particleCreationIdWriteData.numIdsPtr = &mCreatedParticleCount;
	fluidDesc.particleCreationIdWriteData.bufferId = &mCreatedIDs[0];
	fluidDesc.particleCreationIdWriteData.bufferIdByteStride = sizeof(PxU32);

	fluidDesc.particleDeletionIdWriteData.numIdsPtr = &mNumDeletedIDs;
	fluidDesc.particleDeletionIdWriteData.bufferId = &mDeletedIDs[0];
	fluidDesc.particleDeletionIdWriteData.bufferIdByteStride = sizeof(PxU32);

	mRestDensity = fluidDesc.restDensity;  // emitters may find this helpful
	mParticleMass = fluidDesc.restDensity / PxPow(fluidDesc.restParticlesPerMeter, 3.0f);

	mFluid = scene->createFluid(fluidDesc);
	if (!mFluid)
	{
		APEX_INTERNAL_ERROR("NxFluid creation failed.");
		return;
	}

	PxVec3 upV;
	if (fluidDesc.flags & NX_FF_DISABLE_GRAVITY)
	{
		upV = PXFROMNXVEC3(fluidDesc.externalAcceleration);
	}
	else
	{
		NxVec3 up;
		scene->getGravity(up);
		up += fluidDesc.externalAcceleration;
		upV = PXFROMNXVEC3(up);
	}
	PxF32 gravity = upV.magnitude();
	if (!physx::PxIsFinite(gravity))
	{
		// The user can configure externalAcceleration to -scene.gravity
		NxVec3 up;
		scene->getGravity(up);

		upV = PXFROMNXVEC3(up);
		gravity = upV.magnitude();
		if (!physx::PxIsFinite(gravity))
		{
			// and they could set both to 0,0,0
			upV = physx::PxVec3(0.0f, -1.0f, 0.0f);
			gravity = 1.0f;
		}
	}
	upV *= -1.0f;

	if (!(fluidDesc.flags & NX_FF_ENABLED))
	{
		mNxFluidBroken = true;
	}

	mIofxMgr->setSimulationParameters(getObjectRadius(), upV, gravity, getObjectDensity());
}


void FluidIosActor::removeFromScene()
{
	if (mFluid)
	{
		mFluid->getScene().releaseFluid(*mFluid);
		mFluid = NULL;
	}

	for (physx::PxU32 i = 0; i < mInjectors.getSize(); ++i)
	{
		FluidParticleInjector* inj = DYNAMIC_CAST(FluidParticleInjector*)(mInjectors.getResource(i));
		inj->reset();
	}

	mParticleCount = 0;
	mCreatedParticleCount = 0;
	mDelBuffer.resize(0);
	mAddBuffer.resize(0);
}



void FluidIosActor::submitTasks()
{
	if (mNxFluidBroken)
	{
		// mark all injectors as backlogged and clear them
		for (physx::PxU32 i = 0; i < mInjectors.getSize(); i++)
		{
			FluidParticleInjector* inj = DYNAMIC_CAST(FluidParticleInjector*)(mInjectors.getResource(i));

			inj->reset();
			inj->mIsBackLogged = true;
		}
		return;
	}

	if (mFluid && fluidSubStepCount())
	{
		physx::PxTaskManager* tm = mParticleScene->mApexScene->getTaskManager();
		tm->submitUnnamedTask(mPrepareBenefitTask);
		tm->submitUnnamedTask(mUpdateTask);
		tm->submitUnnamedTask(mPostUpdateTask);
	}
}

void FluidIosActor::setTaskDependencies()
{
	if (mFluid && fluidSubStepCount() && !mNxFluidBroken)
	{
		physx::PxTaskManager* tm = mParticleScene->mApexScene->getTaskManager();

		mPrepareBenefitTask.finishBefore(tm->getNamedTask(AST_LOD_COMPUTE_BENEFIT));

		mUpdateTask.startAfter(tm->getNamedTask(AST_LOD_COMPUTE_BENEFIT));
		mUpdateTask.finishBefore(tm->getNamedTask(AST_PHYSX_SIMULATE));

		PxTaskID iofxid = mIofxMgr->getUpdateEffectsTaskID(mPostUpdateTask.getTaskID());
		if (iofxid)
		{
			// Wrap IOS tasks around IOFX update task
			mUpdateTask.finishBefore(iofxid);
			mPostUpdateTask.startAfter(iofxid);
		}
		else
		{
			// IOFX update will be synchronous in updateEffectsData() call.
			mPostUpdateTask.startAfter(mUpdateTask.getTaskID());
		}
		mPostUpdateTask.finishBefore(tm->getNamedTask(AST_PHYSX_FETCH_RESULTS));
	}
}

/************ LOD *****************/


physx::PxF32 FluidIosActor::getBenefit()
{
	return 0.0f;
}

PxF32 FluidIosActor::calcParticleBenefit(
    const FluidParticleInjector& inj, const PxVec3& eyePos,
    const PxVec3& pos, const PxVec3& vel, PxF32 life) const
{
	physx::PxF32 benefit = inj.mLODBias;
	//distance term
	physx::PxF32 distance = (eyePos - pos).magnitude();
	benefit += inj.mLODDistanceWeight * (1.0f - PxMin(1.0f, distance / inj.mLODMaxDistance));
	//velocity term, TODO: clamp velocity
	benefit += inj.mLODSpeedWeight * vel.magnitude();
	//life term
	benefit += inj.mLODLifeWeight * life;

	return PxMin(1.0f, benefit);
}


/* setResource
 * <snip completely obsolete comment.  Re-add after this is working again>
 */
physx::PxF32 FluidIosActor::setResource(physx::PxF32 resourceBudget, physx::PxF32 maxRemaining, physx::PxF32 relativeBenefit)
{
	PX_UNUSED(resourceBudget);
	PX_UNUSED(maxRemaining);
	PX_UNUSED(relativeBenefit);
	return 0.0f;
}


// Called by scene task graph before LOD
void FluidIosActor::TaskPrepareBenefit::run()
{
	mOwner.prepareBenefit();
}

void FluidIosActor::prepareBenefit()
{
	for (physx::PxU32 i = 0; i < mInjectors.getSize(); i++)
	{
		FluidParticleInjector* inj = DYNAMIC_CAST(FluidParticleInjector*)(mInjectors.getResource(i));

		inj->mSimulatedCount = 0;
		inj->mSimulatedBenefit = 0.0f;
	}

	physx::PxVec3 eyePos = mParticleScene->getApexScene()->getEyePosition();

	for (PxU32 i = 0; i < mParticleCount; i++)
	{
		PxU32 nxid = mInputToNx[ i ];
		PxU32 state = mNxToState[ nxid ];

		PX_ASSERT(state < mMaxParticleCount);
		if (state >= mMaxParticleCount)
		{
			continue;
		}

		ParticleData& d = mParticleData[ nxid ];
		d.iosDeleted = false;

		mBufDesc.pmaInStateToInput->get(state) = i;
		mBufDesc.pmaActorIdentifiers->get(i) = d.iofxActorID;
		mBufDesc.pmaVelocityLife->get(i).w = mLifetime[ i ] / d.lifespan;
		*(PxU32*)(&mBufDesc.pmaCollisionNormalFlags->get(i).w) &= (NX_FP_COLLISION_WITH_STATIC | NX_FP_COLLISION_WITH_DYNAMIC);

		if (d.injector && d.iofxActorID.getVolumeID() != NiIofxActorID::NO_VOLUME)
		{
			PxVec3 pos = mBufDesc.pmaPositionMass->get(i).getXYZ();
			PxVec3 vel = mBufDesc.pmaVelocityLife->get(i).getXYZ();
			PxF32  life = mBufDesc.pmaVelocityLife->get(i).w;

			d.lodBenefit = calcParticleBenefit(*d.injector, eyePos, pos, vel, life);
			PX_ASSERT(PxIsFinite(d.lodBenefit));

			++(d.injector->mSimulatedCount);
			d.injector->mSimulatedBenefit += d.lodBenefit;
		}
	}

}

// Called by scene task graph between LOD and PhysX::simulate()
void FluidIosActor::TaskUpdate::run()
{
	setProfileStat((PxU16) mOwner.mParticleCount);
	mOwner.cullAndReplace();
}

FluidIosActor::BusyInjector::BusyInjector(FluidParticleInjector* i)
	: inj(i)
	, readid(0)
{
	remain = inj->getInjectedParticles().size();
}
IosNewObject& FluidIosActor::BusyInjector::read()
{
	physx::Array<IosNewObject>& list = inj->getInjectedParticles();
	while (list[ readid ].iofxActorID.getActorClassID() == NiIofxActorID::INV_ACTOR)
	{
		readid++;
	}
	remain--;
	return list[ readid++ ];
}
bool FluidIosActor::BusyInjector::empty()
{
	return remain == 0;
}
void FluidIosActor::BusyInjector::markDeleted(PxU32 i)
{
	physx::Array<IosNewObject>& list = inj->getInjectedParticles();

	PX_ASSERT(remain);
	PX_ASSERT(list[ i ].iofxActorID.getActorClassID() != NiIofxActorID::INV_ACTOR);

	list[ i ].iofxActorID.setActorClassID(NiIofxActorID::INV_ACTOR);
	remain--;
}

void FluidIosActor::distributeBudgetAmongInjectors()
{
	PxU32 numInjectors = mInjectors.getSize();
	mBudgetDistributor.resize(numInjectors);

	for (physx::PxU32 i = 0; i < numInjectors; i++)
	{
		FluidParticleInjector* inj = DYNAMIC_CAST(FluidParticleInjector*)(mInjectors.getResource(i));

		mBudgetDistributor.setBenefit(i, inj->mLODNodeBenefit);
		mBudgetDistributor.setTargetValue(i, inj->getInjectedParticlesCount());
	}

	mBudgetDistributor.solve(mMaxInsertionCount);

	PxF32 totalResource = 0;
	for (physx::PxU32 i = 0; i < numInjectors; i++)
	{
		FluidParticleInjector* inj = DYNAMIC_CAST(FluidParticleInjector*)(mInjectors.getResource(i));

		inj->setMaxInsertionCount(mBudgetDistributor.getResultValue(i));

		mBudgetDistributor.setBenefit(i, inj->mLODNodeBenefit);
		mBudgetDistributor.setTargetValue(i, inj->getTargetCount());

		totalResource += inj->mLODNodeResource;
	}

	PxF32 unitCost = mParticleScene->mModule->getLODUnitCost();
	PxU32 totalResourceBudget = static_cast<PxU32>(totalResource / unitCost);
	if (totalResourceBudget > mMaxParticleCount)
	{
		totalResourceBudget = mMaxParticleCount;
	}
	mBudgetDistributor.solve(totalResourceBudget);

	for (physx::PxU32 i = 0; i < numInjectors; i++)
	{
		FluidParticleInjector* inj = DYNAMIC_CAST(FluidParticleInjector*)(mInjectors.getResource(i));

		inj->setResourceBudget(mBudgetDistributor.getResultValue(i));
	}
}


void FluidIosActor::cullAndReplace()
{
	if (mNxFluidBroken)
	{
		return;
	}

	PX_ASSERT(mHomelessParticles.empty() && mDelBuffer.empty() && mAddBuffer.empty() && mBusyInj.empty());

#if !defined(WITHOUT_DEBUG_VISUALIZE)
	bool visualizeDeletions = mParticleScene->mDebugRenderParams->LodBenefits != 0.0f;
#endif

	distributeBudgetAmongInjectors();

	for (PxU32 i = 0; i < mParticleCount; i++)
	{
		PxU32 nxid = mInputToNx[ i ];
		PxU32 state = mNxToState[ nxid ];

		PX_ASSERT(state < mMaxParticleCount);
		if (state >= mMaxParticleCount)
		{
			continue;
		}

		ParticleData& d = mParticleData[ nxid ];

		if (d.injector && d.iofxActorID.getVolumeID() != NiIofxActorID::NO_VOLUME)
		{
			if (d.injector->mForceDeleteList.mElements.size())
			{
				d.injector->mForceDeleteList.insert(i, d.lodBenefit);
			}
		}
		else
		{
			mHomelessParticles.pushBack(i);

			IosDelParticle update;
			update.flags = NX_FP_DELETE;
			update.nxid = nxid;
			mDelBuffer.pushBack(update);

#if !defined(WITHOUT_DEBUG_VISUALIZE)
			if (visualizeDeletions)
			{
				DeleteVisInfo info;
				info.position = mBufDesc.pmaPositionMass->get(i).getXYZ();
				info.deletionTime = -((PxI32) mTotalElapsedMS);
				mVisualizeDeletedPositions.pushBack(info);
			}
#endif
		}
	}

	PxU32 totalResourceBudget = 0;
	for (physx::PxU32 i = 0; i < mInjectors.getSize(); i++)
	{
		FluidParticleInjector* inj = DYNAMIC_CAST(FluidParticleInjector*)(mInjectors.getResource(i));

		totalResourceBudget += inj->mResourceBudget;
		PX_ASSERT(inj->mForceDeleteList.validate());
		PX_ASSERT(inj->mForceDeleteList.mElements.size() == inj->mForceDeleteList.mCount);
	}
	PX_ASSERT(totalResourceBudget <= mMaxParticleCount);

	if (mHomelessParticles.size())
	{
		PX_PROFILER_PLOT((PxU32) mHomelessParticles.size(), "FluidIosHomelessDeletions");
	}

	mLeastBenefit = 0.0f;
	mBusyInj.reserve(mInjectors.getSize());

	for (physx::PxU32 i = 0; i < mInjectors.getSize(); ++i)
	{
		FluidParticleInjector* inj = DYNAMIC_CAST(FluidParticleInjector*)(mInjectors.getResource(i));

		PxU32 insertionCount = 0;
		if (inj->mResourceBudget > inj->mSimulatedCount)
		{
			insertionCount = inj->mResourceBudget - inj->mSimulatedCount;
		}

		BusyInjector* busyInj = NULL;
		if (inj->mMaxInsertionCount > 0)
		{
			mBusyInj.pushBack(BusyInjector(inj));
			busyInj = &mBusyInj.back();
		}
		if (inj->mForceDeleteList.mCount > 0)
		{
			PxU32 minForceDeleteCount = 0;
			if (inj->mSimulatedCount > inj->mResourceBudget)
			{
				minForceDeleteCount = inj->mSimulatedCount - inj->mResourceBudget;
			}
			PX_ASSERT(inj->mForceDeleteList.mCount >= minForceDeleteCount);
			PxF32 injLeastBenefit = inj->mForceDeleteList.mostBenefit();

			if (busyInj != NULL && inj->mForceDeleteList.mCount > minForceDeleteCount)
			{
				inj->mInjectorCullList.reset(inj->mForceDeleteList.mCount - minForceDeleteCount);
				for (PxU32 j = 0 ; j < inj->mMaxInsertionCount ; j++)
				{
					if (inj->mInjectedParticles[ j ].lodBenefit < injLeastBenefit)
					{
						inj->mInjectorCullList.insert(j, inj->mInjectedParticles[ j ].lodBenefit);
					}
				}
				PX_ASSERT(inj->mInjectorCullList.validate());

				while (inj->mInjectorCullList.mCount > 0 && inj->mForceDeleteList.mCount > minForceDeleteCount)
				{
					PxF32 injMostBenefit = inj->mInjectorCullList.mostBenefit();
					PxF32 iosMostBenefit = inj->mForceDeleteList.mostBenefit();
					if (injMostBenefit <= iosMostBenefit)
					{
						// One IOS particle gets a reprieve
						inj->mForceDeleteList.popBack();
						PxU32 index = inj->mInjectorCullList.popBack();
						busyInj->markDeleted(index);
					}
					else
					{
						// This injected particle gets a reprieve
						inj->mInjectorCullList.popBack();
					}
				}
				PX_ASSERT(inj->mForceDeleteList.mCount >= minForceDeleteCount);
				injLeastBenefit = inj->mForceDeleteList.mostBenefit();
			}
			if (mLeastBenefit < injLeastBenefit)
			{
				mLeastBenefit = injLeastBenefit;
			}
			insertionCount += (inj->mForceDeleteList.mCount - minForceDeleteCount);

			//add to IOS ForceDeleteArray
			while (inj->mForceDeleteList.mCount > 0)
			{
				PxU32 input = inj->mForceDeleteList.popBack();
				mForceDeleteArray.pushBack(input);
			}
		}
		PX_ASSERT(insertionCount <= inj->mMaxInsertionCount);
		inj->mCurInsertionCount = insertionCount;
	}

	for (PxU32 i = 0; i < mBusyInj.size(); i++)
	{
		FluidParticleInjector* inj = mBusyInj[ i ].inj;

		PX_ASSERT(mBusyInj[ i ].remain >= inj->mCurInsertionCount);

		while (inj->mCurInsertionCount > 0)
		{
			PxU32 input;
			PxU32 state;

			if (mForceDeleteArray.size() > 0)
			{
				// Replace culled particle in input and state arrays
				input = mForceDeleteArray.popBack();
				PxU32 nxid = mInputToNx[ input ];
				state = mNxToState[ nxid ];
				ParticleData& d = mParticleData[ nxid ];
				d.iosDeleted = true;

				IosDelParticle update;
				update.flags = NX_FP_DELETE;
				update.nxid = nxid;
				mDelBuffer.pushBack(update);

#if !defined(WITHOUT_DEBUG_VISUALIZE)
				if (visualizeDeletions)
				{
					DeleteVisInfo info;
					info.position = mBufDesc.pmaPositionMass->get(input).getXYZ();
					info.deletionTime = (PxI32) mTotalElapsedMS;
					mVisualizeDeletedPositions.pushBack(info);
				}
#endif
			}
			else if (mHomelessParticles.size() > 0)
			{
				// Replace homeless particle in input and state arrays
				input = mHomelessParticles.popBack();
				PxU32 nxid = mInputToNx[ input ];
				state = mNxToState[ nxid ];
				mParticleData[ nxid ].iosDeleted = true;
			}
			else if (mReuseStateIDs.size() > 0)
			{
				// Append ourselves to input array, reuse NxFluid deleted particle's stateID
				input = mParticleCount++;
				state = mReuseStateIDs.popBack();
			}
			else
			{
				// Append ourselves to input and state arrays
				input = mParticleCount++;
				state = mMaxStateID++;
			}

			PX_ASSERT(mParticleCount <= mMaxParticleCount);
			PX_ASSERT(mMaxStateID <= mMaxParticleCount);
			PX_ASSERT(mAddBuffer.size() < mMaxInsertionCount);

			mInputToNx[ input ] = mAddBuffer.size() | NiIosBufferDesc::NEW_PARTICLE_FLAG;
			mBufDesc.pmaInStateToInput->get(state) = input | NiIosBufferDesc::NEW_PARTICLE_FLAG;

			IosNewParticle p;
			p.object = mBusyInj[ i ].read();
			if (p.object.lifetime == 0.0f)
			{
				// hack to force 0 lifetime particles to render
				p.object.lifetime = 3600.0f * 24; //FLT_MAX;
			}

			ParticleData& d = p.data;
			d.injector = inj;
			d.lifespan = p.object.lifetime;
			d.iofxActorID = p.object.iofxActorID;
			d.lodBenefit = p.object.lodBenefit;

			// Make compiler happy
			p.newStateID = state | NiIosBufferDesc::NEW_PARTICLE_FLAG;
			d.iosDeleted = false;

			mAddBuffer.pushBack(p);

			mBufDesc.pmaActorIdentifiers->get(input) = d.iofxActorID;
			mBufDesc.pmaPositionMass->get(input) = PxVec4(p.object.initialPosition, mParticleMass);
			mBufDesc.pmaVelocityLife->get(input) = PxVec4(p.object.initialVelocity, 1.0);
			mBufDesc.pmaCollisionNormalFlags->get(input).setZero();
			if (mBufDesc.pmaDensity)
			{
				mBufDesc.pmaDensity->get(input) = 0.0f;
			}

			--inj->mCurInsertionCount;
		}

		PX_ASSERT(mBusyInj[ i ].readid <= inj->mMaxInsertionCount);
	}

	// Repack and recalc benefit of injectors that were not emptied
	// Not optimal, but hopefully not persistent either.  A FIFO would cost
	// more during LOD.  We can revisit this later, if necessary
	for (PxU32 i = 0 ; i < mBusyInj.size() ; i++)
	{
		FluidParticleInjector* inj = mBusyInj[i].inj;
		if (mBusyInj[i].empty())
		{
			inj->reset();
			continue;
		}

		inj->mInjectedBenefit = 0.0f;
		PxU32 j = 0;
		while (!mBusyInj[i].empty())
		{
			inj->mInjectedParticles[ j ] = mBusyInj[i].read();
			inj->mInjectedBenefit += inj->mInjectedParticles[ j ].lodBenefit;
			if (++j >= mMaxParticleCount)
			{
				break;
			}
		}
		inj->mInjectedParticles.resize(j);
		inj->mIsBackLogged = true;  // alert emitters
	}
	mBusyInj.clear();

	mMaxInputID = mParticleCount;
	mParticleCount = totalResourceBudget;

	// Mark holes in the state array
	while (mForceDeleteArray.size() > 0)
	{
		PxU32 input = mForceDeleteArray.popBack();
		PxU32 nxid = mInputToNx[ input ];
		PxU32 state = mNxToState[ nxid ];

		ParticleData& d = mParticleData[ nxid ];
		d.iosDeleted = true;

		// Do not allow the state ID associated with this NXID to be re-used,
		// since the output state IDs will not include this particle.
		mBufDesc.pmaActorIdentifiers->get(input).setActorClassID(NiIofxActorID::INV_ACTOR);
		mBufDesc.pmaInStateToInput->get(state) = NiIosBufferDesc::NOT_A_PARTICLE;

		IosDelParticle update;
		update.flags = NX_FP_DELETE;
		update.nxid = nxid;
		mDelBuffer.pushBack(update);

#if !defined(WITHOUT_DEBUG_VISUALIZE)
		if (visualizeDeletions)
		{
			DeleteVisInfo info;
			info.position = mBufDesc.pmaPositionMass->get(input).getXYZ();
			info.deletionTime = (PxI32) mTotalElapsedMS;
			mVisualizeDeletedPositions.pushBack(info);
		}
#endif
	}

	while (mHomelessParticles.size() > 0)
	{
		PxU32 input = mHomelessParticles.popBack();
		PxU32 nxid = mInputToNx[ input ];
		PxU32 state = mNxToState[ nxid ];
		// Do not allow the state ID associated with this NXID to be re-used,
		// since the output state IDs will not include this particle.
		mParticleData[ nxid ].iosDeleted = true;
		mBufDesc.pmaActorIdentifiers->get(input).setActorClassID(NiIofxActorID::INV_ACTOR);
		mBufDesc.pmaInStateToInput->get(state) = NiIosBufferDesc::NOT_A_PARTICLE;
	}

	while (mReuseStateIDs.size() > 0)
	{
		const PxU32 state = mReuseStateIDs.popBack();
		mBufDesc.pmaInStateToInput->get(state) = NiIosBufferDesc::NOT_A_PARTICLE;
	}

	if (mDelBuffer.size())
	{
		NxParticleUpdateData updateData;
		updateData.bufferFlag = &mDelBuffer[0].flags;
		updateData.bufferFlagByteStride = sizeof(IosDelParticle);
		updateData.bufferId = &mDelBuffer[0].nxid;
		updateData.bufferIdByteStride = sizeof(IosDelParticle);
		updateData.numUpdates = mDelBuffer.size();

		SCOPED_PHYSX_LOCK_WRITE(*mParticleScene->mApexScene);
		mFluid->updateParticles(updateData);
	}

	/* Oh! Manager of the IOFX! do your thing */
	mSubmittedParticleCount = mParticleCount;
	PxF32 deltaTime = mParticleScene->getApexScene()->getPhysXSimulateTime();
	mIofxMgr->updateEffectsData(deltaTime, mParticleCount, mMaxInputID, mMaxStateID);
}

void FluidIosActor::TaskPostUpdate::run()
{
	setProfileStat((PxU16) mOwner.mParticleCount);
	mOwner.postUpdateEffects();
}

/* Runs after IOFX Modifiers, before fetchResults */
void FluidIosActor::postUpdateEffects()
{
	/*
	Here's the deal.  There are four indices into this particle data.

	input - defined by the order NxFluid writes particles into our arrays
	nxid  - NxFluid's permanent identifier for every particle

	When NxFluid writes the input arrays, it writes an inputToNx mapping,
	so we know the NXID of every live particle that NxFluid has created.

	input state - the index of the particle in the IOFX data pre-modifiers
	output state - the index of the particle in the IOFX data post-modifiers

	(Note: output state != input state because of volume migration, etc)
	(Note: output state order is essentially the instance/sprite buffer order)

	We must pass a stateToInput mapping to the IOFX as input.  To maintain
	this, we must store the particle's output state ID in an array indexed
	by NXID, because the input ordering changes at NxFluid's whim.

	Newly inserted particles make this interesting.  They have no NXID until
	fetchResults(), but at fetchResults() we lose our inStateToInput mapping.
	So we store new particle data in an "addBuffer" and store their index
	into that buffer in the inputToNx table.  Now we can find the inserted
	particle and store its output state ID so in fetchResults the output state
	ID can be stored into the proper location in NxToState.

	Also, the IOFX is updating the volumeID in the input array.  The IOS is
	responsible for persisting that volumeID between frames.  If a volume
	ID of NO_VOLUME is returned, the particle should be culled.
	*/

	for (PxU32 newstate = 0 ; newstate < mSubmittedParticleCount ; newstate++)
	{
		PxU32 input = mBufDesc.pmaOutStateToInput->get(newstate);
		PX_ASSERT(input < mMaxInputID);

		if (input >= mMaxInputID)
		{
			continue;
		}

		PxU32 nxid = mInputToNx[ input ];

		if (nxid & NiIosBufferDesc::NEW_PARTICLE_FLAG)
		{
			PxU32 addIndex = nxid & ~NiIosBufferDesc::NEW_PARTICLE_FLAG;
			PX_ASSERT(addIndex < mAddBuffer.size());
			mAddBuffer[ addIndex ].newStateID = newstate;
			mAddBuffer[ addIndex ].data.iofxActorID = mBufDesc.pmaActorIdentifiers->get(input);
		}
		else
		{
			mNxToState[ nxid ] = newstate;
			mParticleData[ nxid ].iofxActorID = mBufDesc.pmaActorIdentifiers->get(input);
		}
	}

	/* In fetchResults(), mParticleCount will be overwritten by the NxFluid with the
	 * new NX particle count.  We record our mParticleCount here because we know that
	 * the IOFX output is compressed.
	 */
	mMaxStateID = mSubmittedParticleCount;
}



// Called from FluidIosScene::fetchResults()
void FluidIosActor::fetchResults()
{
	PX_PROFILER_PERF_SCOPE("FluidIosFetchResults");

	if (mNxFluidBroken)
	{
		return;
	}

	if (!mFluid || !fluidSubStepCount())
	{
		return;
	}

#if !defined(WITHOUT_DEBUG_VISUALIZE)
	mTotalElapsedMS = mParticleScene->mApexScene->getTotalElapsedMS();
#endif

	PX_ASSERT(mNumDeletedIDs >= mDelBuffer.size());
	PX_ASSERT(mReuseStateIDs.empty());
	mReuseStateIDs.clear();
	// Find NXIDs that were freed by NxFluid but were not deleted
	// and replaced by our LOD code (their state IDs were already reused)

	for (PxU32 i = 0; i < mNumDeletedIDs; ++i)
	{
		const PxU32 nxid = mDeletedIDs[ i ];
		const ParticleData& d = mParticleData[ nxid ];
		if (!d.iosDeleted)
		{
			mReuseStateIDs.pushBack(mNxToState[ nxid ]);
		}
	}
	mNumDeletedIDs = 0;

	physx::PxU32 numAddParticles = mAddBuffer.size();
	if (numAddParticles > 0)
	{
		PX_ASSERT(numAddParticles <= mMaxInsertionCount);
		PX_ASSERT(mParticleCount + numAddParticles <= mMaxParticleCount);

		NxParticleData data;
		data.numParticlesPtr = &numAddParticles;
		data.bufferPos = &mAddBuffer[0].object.initialPosition.x;
		data.bufferPosByteStride = sizeof(IosNewParticle);
		data.bufferVel = &mAddBuffer[0].object.initialVelocity.x;
		data.bufferVelByteStride = sizeof(IosNewParticle);
		data.bufferLife = &mAddBuffer[0].object.lifetime;
		data.bufferLifeByteStride = sizeof(IosNewParticle);

		mFluid->addParticles(data);

		if (!mCreatedParticleCount && !mParticleCount)
		{
			// fluid is broken, don't touch the NxFluid again
			APEX_INTERNAL_ERROR("NxFluid is broken, no longer simulating.");
			mNxFluidBroken = true;
		}
		else
		{
			PX_ASSERT(mCreatedParticleCount == numAddParticles);
			PX_ASSERT(mAddBuffer.size() == numAddParticles);
		}

		for (PxU32 i = 0; i < mCreatedParticleCount; ++i)
		{
			const PxU32 createdID = mCreatedIDs[ i ];
			IosNewParticle& inp = mAddBuffer[ i ];
			PX_ASSERT(inp.newStateID < mMaxStateID);
			if (inp.newStateID < mMaxStateID)
			{
				mNxToState[ createdID ] = inp.newStateID;
				mParticleData[ createdID ] = inp.data;
			}
			else
			{
				mNxToState[ createdID ] = 0xffffffff;
			}
		}
	}
	PX_ASSERT(!mNumDeletedIDs);
	PX_ASSERT(mParticleCount + mReuseStateIDs.size() <= mMaxStateID);

	mDelBuffer.clear();
	mAddBuffer.clear();
}



NiIosInjector* FluidIosActor::allocateInjector(const char* iofxAssetName)
{
	NiResourceProvider* nrp = mAsset->mModule->mSdk->getInternalResourceProvider();

	NiApexAuthorableObject* AO = mAsset->mModule->mSdk->getAuthorableObject(NX_IOFX_AUTHORING_TYPE_NAME);
	if (!AO)
	{
		APEX_INTERNAL_ERROR("Unknown authorable type: %s, please load the IOFX module.", NX_IOFX_AUTHORING_TYPE_NAME);
		return NULL;
	}
	NxResID iofxnsid = AO->getResID();

	if (iofxnsid == INVALID_RESOURCE_ID)
	{
		return NULL;
	}

	FluidParticleInjector* inj = PX_NEW(FluidParticleInjector)(mInjectors, *this);
	if (inj == 0)
	{
		return NULL;
	}

	NxResID iofxAssetResID = nrp->createResource(iofxnsid, iofxAssetName);
	if (iofxAssetResID == INVALID_RESOURCE_ID)
	{
		inj->release();
		return NULL;
	}

	NxIofxAsset* iofxAsset = DYNAMIC_CAST(NxIofxAsset*)(nrp->getResource(iofxAssetResID));
	if (iofxAsset == 0)
	{
		inj->release();
		return NULL;
	}

	inj->init(iofxAssetResID, iofxAsset);

	/* Add reference to IOFX asset table */
	for (PxU32 a = 0; a < mIofxAssets.size() ; a++)
	{
		if (mIofxAssets[a] == iofxAsset)
		{
			mIofxAssetRefs[a]++;
			return inj;
		}
	}

	/** New IOFX Asset */
	mIofxAssets.pushBack(iofxAsset);
	mIofxAssetRefs.pushBack(1);
	return inj;
}


const PxVec3* FluidIosActor::getRecentPositions(physx::PxU32& count, physx::PxU32& stride) const
{
	count = mParticleCount;
	stride = sizeof(PxVec4);
	return (const PxVec3*) mBufDesc.pmaPositionMass->getPtr();
}


// This method tells you if the fluid (in a scene or compartment) stepped or not
physx::PxU32 FluidIosActor::fluidSubStepCount()
{
	physx::PxF32 maxTimeStep;
	physx::PxU32 nbSubSteps, maxIter;
	NxTimeStepMethod method;
	if (mCompartment)
	{
		mCompartment->getTiming(maxTimeStep, maxIter, method, &nbSubSteps);
	}
	else
	{
		getPhysXScene()->getTiming(maxTimeStep, maxIter, method, &nbSubSteps);
	}

	return nbSubSteps;
}

/* Removing injectors is potentially expensive.  Suggest removing
 * all particles first, if the intent to to delete everything
 */
void FluidIosActor::releaseInjector(NiIosInjector& injector)
{
	FluidParticleInjector* inj = DYNAMIC_CAST(FluidParticleInjector*)(&injector);

	for (PxU32 i = 0 ; i < mParticleData.size() ; i++)
	{
		ParticleData& d = mParticleData[ i ];
		if (d.injector == inj)
		{
			d.injector = NULL;      // Zero benefit, fast cull
			d.iofxActorID.setActorClassID(NiIofxActorID::INV_ACTOR);
		}
	}

	/* Prune particles from add list */
	for (PxU32 i = 0 ; i < mAddBuffer.size() ; i++)
	{
		if (mAddBuffer[ i ].data.injector == inj)
		{
			mAddBuffer[ i ].data.injector = NULL;      // Zero benefit, fast cull
			mAddBuffer[ i ].data.iofxActorID.setActorClassID(NiIofxActorID::INV_ACTOR);
		}
	}

	for (PxU32 i = 0 ; i < mIofxAssets.size() ; i++)
	{
		if (mIofxAssets[i] != inj->mIofxAsset)
		{
			continue;
		}

		if (--mIofxAssetRefs[ i ] == 0)
		{
			mIofxMgr->releaseAssetID(inj->mIofxAsset);
			mIofxAssets.replaceWithLast(i);
			mIofxAssetRefs.replaceWithLast(i);
		}
	}

	inj->destroy();

	if (mInjectors.getSize() == 0)
	{
		//if we have no injectors - release self
		release();
	}
}


void FluidIosActor::getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly)
{
	PX_UNUSED(min);
	PX_UNUSED(max);
	PX_UNUSED(intOnly);
	APEX_INVALID_OPERATION("not implemented");
}


physx::PxF32 FluidIosActor::getActivePhysicalLod()
{
	APEX_INVALID_OPERATION("NxApexRenderMeshActor does not support this operation");
	return -1.0f;
}


void FluidIosActor::forcePhysicalLod(physx::PxF32 lod)
{
	PX_UNUSED(lod);
	APEX_INVALID_OPERATION("not implemented");
}



class F32Less
{
public:
	PX_FORCE_INLINE bool operator()(PxF32 v1, PxF32 v2) const
	{
		return v1 < v2;
	}
};


void FluidIosActor::visualize()
{
#if !defined(WITHOUT_DEBUG_VISUALIZE)
	physx::DebugGraphDesc*	gDescPtr;
	physx::PxMat34Legacy	pose(true);
	physx::PxF32			benefitScale;
	char					xLabel[128];
	char					yLabel[128];
	bool					rescaleNow = false;
	NxVec3					nxGravity;
	physx::PxVec3			pxNormGravity;

	benefitScale = mParticleScene->mDebugRenderParams->LodBenefits;
	if (benefitScale == 0.0f)
	{
		mVisualizeDeletedPositions.clear();
		return;
	}

	// we need gravity to draw lines for benefit and deletions
	mParticleScene->mApexScene->getPhysXScene()->getGravity(nxGravity);
	pxNormGravity = PxFromNxVec3Fast(nxGravity);
	physx::PxF32 m = pxNormGravity.normalize();
	if (m < 0.0001f)
	{
		pxNormGravity = physx::PxVec3(0.0f, -1.0f, 0.0f);
	}

	ApexRenderable::renderDataLock();

	mParticleScene->mRenderDebug->setCurrentUserPointer((void*)(NxApexActor*)this);

	// is it time to rescale the AXIS of the graphs?
	if ((mTotalElapsedMS - mLastVisualizeMS) > VIZ_RESCALE_MS)
	{
		rescaleNow = true;
		mLastVisualizeMS = mTotalElapsedMS;
	}

	// only draw graphs for this IOS if its index is selected
	bool drawGraphs = false;
	physx::PxU32 iosIdx;
	mParticleScene->mActorListLock.lockReader();
	for (iosIdx = 0 ; iosIdx < mParticleScene->mActorArray.size(); iosIdx++)
	{
		FluidIosActor* ios = DYNAMIC_CAST(FluidIosActor*)(mParticleScene->mActorArray[iosIdx]);
		if (ios == this &&
		        mParticleScene->mFluidIosDebugRenderParams->GRAPH_ACTOR_INDEX == iosIdx)
		{
			drawGraphs = true;
			break;
		}
	}
	mParticleScene->mActorListLock.unlockReader();

	mParticleScene->mRenderDebug->pushRenderState();
	if (mDebugRenderGroupID != 0)
	{
		mParticleScene->mRenderDebug->reset(mDebugRenderGroupID);
		mDebugRenderGroupID = 0;
	}
#if 0
	// start the draw group
	mDebugRenderGroupID = mParticleScene->mRenderDebug->beginDrawGroup(pose);
	mParticleScene->mRenderDebug->pushRenderState();
	mParticleScene->mRenderDebug->removeFromCurrentState(physx::DebugRenderState::InfiniteLifeSpan);
#else
	physx::PxMat44 savePose = mParticleScene->mRenderDebug->getPose();
	mParticleScene->mRenderDebug->setPose(pose);
#endif

	mParticleScene->mRenderDebug->addToCurrentState(physx::DebugRenderState::NoZbuffer);

	// determine which way to draw the deletion lines "up"
	physx::PxVec3 delLine(benefitScale);
	delLine = delLine.multiply(-pxNormGravity);

	for (physx::Array<DeleteVisInfo>::Iterator i = mVisualizeDeletedPositions.begin(); i != mVisualizeDeletedPositions.end(); i++)
	{
		// was it deleted less than a second ago?  If yes, draw it, otherwise remove it from the array.
		if (PxAbs((*i).deletionTime) + 1000 > (PxI32) mTotalElapsedMS)
		{
			const PxU32 colorYellow = mParticleScene->mRenderDebug->getDebugColor(DebugColors::Yellow);
			const PxU32 colorPurple = mParticleScene->mRenderDebug->getDebugColor(DebugColors::Purple);
			mParticleScene->mRenderDebug->setCurrentColor(((*i).deletionTime < 0) ? colorYellow : colorPurple);
			mParticleScene->mRenderDebug->debugLine((*i).position, (*i).position + delLine);
		}
		else
		{
			mVisualizeDeletedPositions.replaceWithLast(static_cast<physx::PxU32>(i - mVisualizeDeletedPositions.begin()));
			i--;
		}
	}

	if (mParticleCount && getCachedBenefit() > 0.0f)
	{
		physx::PxF32 benefitQuotient = 1.0f / getCachedBenefit();

		if (drawGraphs)
		{
			//graph display parameters -- relative to display surface x = -1 (left)..1 (right), y = -1 (bottom) .. 1 (top)
			const physx::PxU32 numGraphPoints = 100;
			physx::PxReal maxDensity = 0.0f, minDensity = PX_MAX_REAL;
			//direct benefit graph:
			physx::Array<physx::PxF32> benefitArray;
			benefitArray.reserve(mParticleCount);
			for (physx::PxU32 i = 0 ; i < mParticleCount ; i++)
			{
				physx::PxU32 nxid = mInputToNx[ i ];
				physx::PxF32 b;
				if (nxid & NiIosBufferDesc::NEW_PARTICLE_FLAG)
				{
					PxU32 addIndex = nxid & ~NiIosBufferDesc::NEW_PARTICLE_FLAG;
					PX_ASSERT(addIndex < mAddBuffer.size());
					b = mAddBuffer[ addIndex ].data.lodBenefit * benefitQuotient;
				}
				else
				{
					b = mParticleData[ nxid ].lodBenefit * benefitQuotient;
				}
				benefitArray.pushBack(b);

				if (mBufDesc.pmaDensity && (mBufDesc.pmaDensity->get(i) > maxDensity))
				{
					maxDensity = mBufDesc.pmaDensity->get(i);
				}
				if (mBufDesc.pmaDensity && (mBufDesc.pmaDensity->get(i) < minDensity))
				{
					minDensity = mBufDesc.pmaDensity->get(i);
				}
			}

			shdfnd::sort(benefitArray.begin(), benefitArray.size(), F32Less());
			physx::PxF32 maxBenefit = *(benefitArray.end() - 1);
			physx::PxF32 sortedPartsYAxis = mSortedPtsYAxis.getGraphAxis(maxBenefit, rescaleNow);

			physx::PxF32 deletionBenefitTreshold = mLeastBenefit * benefitQuotient;
			physx::PxF32 h = deletionBenefitTreshold * physx::GRAPH_HEIGHT_DEFAULT / sortedPartsYAxis;

			APEX_SPRINTF_S(xLabel, sizeof(xLabel), "sorted particles (0 .. %d)", mParticleCount);
			APEX_SPRINTF_S(yLabel, sizeof(yLabel), "benefit (0..%.3f)", sortedPartsYAxis);

			gDescPtr = mParticleScene->mRenderDebug->createDebugGraphDesc(1, benefitArray.size(), &benefitArray[0], sortedPartsYAxis, xLabel, yLabel);
			gDescPtr->mCutOffLevel = h;
			mParticleScene->mRenderDebug->debugGraph(*gDescPtr);
			mParticleScene->mRenderDebug->releaseDebugGraphDesc(gDescPtr);

			//histogram graphs:
			if (mParticleScene->mFluidIosDebugRenderParams->VISUALIZE_FLUID_DENSITY_HISTOGRAM &&
			        mAsset->getSupportsDensity() &&
			        mBufDesc.pmaDensity)
			{
				// particle density histogram
				physx::PxF32 maxBucket = mHistogramXAxis.getGraphAxis(maxDensity, rescaleNow);

				physx::Array<physx::PxF32> points;
				points.resize(numGraphPoints, 0.0f);
				physx::PxF32 graphMax = 0.0f;
				for (physx::PxU32 i = 0 ; i < mParticleCount ; i++)
				{
					physx::PxReal density = mBufDesc.pmaDensity->get(i);

					physx::PxU32 densityBucket = (physx::PxU32)((density * (numGraphPoints - 1)) / maxBucket);
					PX_ASSERT(densityBucket < numGraphPoints);
					points[densityBucket]++;
					if (points[densityBucket] > graphMax)
					{
						graphMax = points[densityBucket];
					}
				}
				graphMax = mHistogramYAxis.getGraphAxis(graphMax, rescaleNow);

				APEX_SPRINTF_S(xLabel, sizeof(xLabel), "density value (%.3f .. %.3f)", minDensity, maxDensity);
				APEX_SPRINTF_S(yLabel, sizeof(yLabel), "%s: nbParticles (0..%d)", mAsset->mName.c_str(), (physx::PxU32) graphMax);

				gDescPtr = mParticleScene->mRenderDebug->createDebugGraphDesc(0, points.size(), &points[0], graphMax, xLabel, yLabel);
				gDescPtr->mCutOffLevel = 0.0f;
				mParticleScene->mRenderDebug->debugGraph(*gDescPtr);
				mParticleScene->mRenderDebug->releaseDebugGraphDesc(gDescPtr);
			}
			else
			{
				// particle benefit histogram
				physx::PxF32 maxBucket = mHistogramXAxis.getGraphAxis(maxBenefit, rescaleNow);

				physx::Array<physx::PxF32> points;
				points.resize(numGraphPoints, 0.0f);
				physx::PxF32 graphMax = 0.0f;
				for (physx::PxU32 i = 0 ; i < mParticleCount ; i++)
				{
					physx::PxU32 benefitBucket = (physx::PxU32)((benefitArray[i] * (numGraphPoints - 1)) / maxBucket);
					PX_ASSERT(benefitBucket < numGraphPoints);
					points[benefitBucket]++;
					if (points[benefitBucket] > graphMax)
					{
						graphMax = points[benefitBucket];
					}
				}
				graphMax = mHistogramYAxis.getGraphAxis(graphMax, rescaleNow);
#if 0
				// the cutoff horizontal line will be the number of particles deleted last frame
				// this is not currently available in ::visualize, so draw no line for now
				physx::PxF32 hh = mDelBuffer.size() * physx::GRAPH_HEIGHT_DEFAULT / graphMax;
#endif
				APEX_SPRINTF_S(xLabel, sizeof(xLabel), "benefit value (0 .. %.3f)", maxBucket);
				APEX_SPRINTF_S(yLabel, sizeof(yLabel), "%s: nbParticles (0..%d)", mAsset->mName.c_str(), (physx::PxU32) graphMax);

				gDescPtr = mParticleScene->mRenderDebug->createDebugGraphDesc(0, points.size(), &points[0], graphMax, xLabel, yLabel);
				gDescPtr->mCutOffLevel = 0.0f;
				mParticleScene->mRenderDebug->debugGraph(*gDescPtr);
				mParticleScene->mRenderDebug->releaseDebugGraphDesc(gDescPtr);
			}
		}

		//'perspective-corrected' particle benefit bars overlaid on particles:
		//TODO: make these different items independently activatable!
		mParticleScene->mRenderDebug->addToCurrentState(physx::DebugRenderState::NoZbuffer);
		mParticleScene->mRenderDebug->removeFromCurrentState(physx::DebugRenderState::ScreenSpace);
		mParticleScene->mRenderDebug->setCurrentColor(mParticleScene->mRenderDebug->getDebugColor(DebugColors::Red));
		const physx::PxVec3& eyePos = mParticleScene->mApexScene->getEyePosition();

		for (physx::PxU32 i = 0 ; i < mParticleCount ; i++)
		{
			PxVec3 pos = mBufDesc.pmaPositionMass->get(i).getXYZ();
			physx::PxF32 distance = (eyePos - pos).magnitude();	//undo perspective
			physx::PxU32 nxid = mInputToNx[ i ];
			physx::PxF32 relativeBenefit;
			if (nxid & NiIosBufferDesc::NEW_PARTICLE_FLAG)
			{
				PxU32 addIndex = nxid & ~NiIosBufferDesc::NEW_PARTICLE_FLAG;
				PX_ASSERT(addIndex < mAddBuffer.size());
				relativeBenefit = mAddBuffer[ addIndex ].data.lodBenefit * benefitQuotient;
			}
			else
			{
				relativeBenefit = mParticleData[ nxid ].lodBenefit * benefitQuotient;
			}

			// determine which way to draw the deletion lines "up"
			physx::PxVec3 benefitLine(benefitScale * distance * relativeBenefit);
			benefitLine = benefitLine.multiply(-pxNormGravity);
			mParticleScene->mRenderDebug->debugLine(pos, pos + benefitLine);
		}
	}
#if 0
	mParticleScene->mRenderDebug->popRenderState();
	mParticleScene->mRenderDebug->endDrawGroup();
#else
	mParticleScene->mRenderDebug->setPose(savePose);
#endif
	mParticleScene->mRenderDebug->popRenderState();

	mParticleScene->mRenderDebug->setCurrentUserPointer(NULL);
	ApexRenderable::renderDataUnLock();
#endif
}

}
}
} // namespace physx::apex

#endif // NX_SDK_VERSION_MAJOR == 2
