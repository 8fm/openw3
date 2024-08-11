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
#if NX_SDK_VERSION_MAJOR == 3

#include "NxApex.h"
#include "NiApexScene.h"
#include "NiApexSDK.h"
#include "NxApexReadWriteLock.h"
#include "NxParticleIosActor.h"
#include "ParticleIosActor.h"
#include "ParticleIosAsset.h"
#include "NxIofxAsset.h"
#include "NxIofxActor.h"
#include "ModuleParticleIos.h"
#include "ParticleIosScene.h"
#include "NiApexRenderDebug.h"
#include "NiApexAuthorableObject.h"
#include "NiModuleIofx.h"
#include "NiFieldSamplerManager.h"
#include "NiFieldSamplerQuery.h"
#include "NiFieldSamplerScene.h"
#include "ApexResourceHelper.h"
#include "ApexMirroredArray.h"

#include <PxScene.h>
#include <PxPhysics.h>
#include <PxAsciiConversion.h>

namespace physx
{
namespace apex
{
namespace pxparticleios
{

void ParticleIosActor::initStorageGroups(InplaceStorage& storage)
{
	mSimulationStorageGroup.init(storage);
}

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

ParticleIosActor::ParticleIosActor(
    NxResourceList& list,
    ParticleIosAsset& asset,
    ParticleIosScene& scene,
    NxIofxAsset& iofxAsset,
    bool isDataOnDevice)
	: mAsset(&asset)
	, mParticleIosScene(&scene)
	, mIsParticleSystem(false)
	, mParticleActor(NULL)
	, mIofxMgr(NULL)
	, mTotalElapsedTime(0.0f)
	, mParticleCount(0)
	, mParticleBudget(0)
	, mInjectedCount(0)
	, mLastActiveCount(0)
	, mLastBenefitSum(0)
	, mLastBenefitMin(+FLT_MAX)
	, mLastBenefitMax(-FLT_MAX)
	, mLifeSpan(scene.getApexScene(), NV_ALLOC_INFO("mLifeSpan", PARTICLES))
	, mLifeTime(scene.getApexScene(), NV_ALLOC_INFO("mLifeTime", PARTICLES))
	, mInjector(scene.getApexScene(), NV_ALLOC_INFO("mInjector", PARTICLES))
	, mBenefit(scene.getApexScene(), NV_ALLOC_INFO("mBenefit", PARTICLES))
	, mInjectorsCounters(scene.getApexScene(), NV_ALLOC_INFO("mInjectorsCounters", PARTICLES))
	, mInputIdToParticleIndex(scene.getApexScene(), NV_ALLOC_INFO("mInputIdToParticleIndex", PARTICLES))
	, mGridDensityGrid(scene.getApexScene(), NV_ALLOC_INFO("mGridDensityGrid", PARTICLES))
	, mGridDensityGridLowPass(scene.getApexScene(), NV_ALLOC_INFO("mGridDensityGridLowPass", PARTICLES))
	, mFieldSamplerQuery(NULL)
	, mField(scene.getApexScene(), NV_ALLOC_INFO("mField", PARTICLES))
	, mInjectTask(*this)
	, mDensityOrigin(0.f,0.f,0.f)
{
	list.add(*this);

	mMaxParticleCount = mAsset->mParams->maxParticleCount;
	physx::PxF32 maxInjectCount = mAsset->mParams->maxInjectedParticleCount;
	mMaxTotalParticleCount = mMaxParticleCount + physx::PxU32(maxInjectCount <= 1.0f ? mMaxParticleCount * maxInjectCount : maxInjectCount);

	NiIofxManagerDesc desc;
	desc.iosAssetName         = mAsset->getName();
	desc.iosSupportsDensity   = mAsset->getSupportsDensity();
	desc.iosSupportsCollision = true;
	desc.iosOutputsOnDevice   = isDataOnDevice;
	desc.maxObjectCount       = mMaxParticleCount;
	desc.maxInputCount        = mMaxTotalParticleCount;
	desc.maxInStateCount      = mMaxTotalParticleCount;

	NiModuleIofx* moduleIofx = mAsset->mModule->getNiModuleIofx();
	if (moduleIofx)
	{
		mIofxMgr = moduleIofx->createActorManager(*mParticleIosScene->mApexScene, iofxAsset, desc);
		mIofxMgr->createSimulationBuffers(mBufDesc);

		for (PxU32 i = 0 ; i < mMaxParticleCount ; i++)
		{
			mBufDesc.pmaInStateToInput->get(i) = NiIosBufferDesc::NOT_A_PARTICLE;
		}
	}

	NiFieldSamplerManager* fieldSamplerManager = mParticleIosScene->getNiFieldSamplerManager();
	if (fieldSamplerManager)
	{
		NiFieldSamplerQueryDesc queryDesc;
		queryDesc.maxCount = mMaxParticleCount;
		queryDesc.samplerFilterData = ApexResourceHelper::resolveCollisionGroup64(mAsset->mParams->fieldSamplerFilterData);
		mFieldSamplerQuery = fieldSamplerManager->createFieldSamplerQuery(queryDesc);
	}

	addSelfToContext(*scene.mApexScene->getApexContext());		// add self to NxApexScene
	addSelfToContext(*DYNAMIC_CAST(ApexContext*)(&scene));		// add self to ParticleIosScene	

	// Pull Grid Density Parameters
	{
		if(mIsParticleSystem && mBufDesc.pmaDensity)
		{
			ParticleIosAssetParam* params = (ParticleIosAssetParam*)(mAsset->getAssetNxParameterized());
			const SimpleParticleSystemParams* gridParams = static_cast<SimpleParticleSystemParams*>(params->particleType);
			mGridDensityParams.Enabled = gridParams->GridDensity.Enabled;
			mGridDensityParams.GridSize = gridParams->GridDensity.GridSize;
			mGridDensityParams.GridMaxCellCount = gridParams->GridDensity.MaxCellCount;
			mGridDensityParams.GridResolution = general_string_parsing2::PxAsc::strToU32(&gridParams->GridDensity.Resolution[4],NULL);
			mGridDensityParams.DensityOrigin = mDensityOrigin;
		}		
		else
		{
			mGridDensityParams.Enabled = false;
			mGridDensityParams.GridSize = 1.f;
			mGridDensityParams.GridMaxCellCount = 1u;
			mGridDensityParams.GridResolution = 8;
			mGridDensityParams.DensityOrigin = mDensityOrigin;
		}
	}
}

ParticleIosActor::~ParticleIosActor()
{
}

void ParticleIosActor::release()
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	mAsset->releaseIosActor(*this);
}

void ParticleIosActor::destroy()
{
	ApexActor::destroy();

	setPhysXScene(NULL);

	// remove ourself from our asset's resource list, in case releasing our emitters
	// causes our asset's resource count to reach zero and for it to be released.
	ApexResource::removeSelf();

	// Release all injectors, releasing all emitters and their IOFX asset references
	while (mInjectorList.getSize())
	{
		ParticleParticleInjector* inj = DYNAMIC_CAST(ParticleParticleInjector*)(mInjectorList.getResource(mInjectorList.getSize() - 1));
		inj->release();
	}

	if (mIofxMgr)
	{
		mIofxMgr->release();
	}
	if (mFieldSamplerQuery)
	{
		mFieldSamplerQuery->release();
	}

	delete this;
}

void ParticleIosActor::setPhysXScene(PxScene* scene)
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

PxScene* ParticleIosActor::getPhysXScene() const
{
	if (mParticleActor)
	{
		return mParticleActor->getScene();
	}
	else
	{
		return NULL;
	}
}

void ParticleIosActor::putInScene(PxScene* scene)
{
	SCOPED_PHYSX3_LOCK_WRITE(scene);
	mUp = scene->getGravity();

	// apply asset's scene gravity scale and external acceleration
	// mUp *= mAsset->getSceneGravityScale();
	// mUp += mAsset->getExternalAcceleration();

	mGravity = mUp.magnitude();
	if (!physx::PxIsFinite(mGravity))
	{
		// and they could set both to 0,0,0
		mUp = physx::PxVec3(0.0f, -1.0f, 0.0f);
		mGravity = 1.0f;
	}
	mUp *= -1.0f;

	const ParticleIosAssetParam*	desc = mAsset->getParticleDesc();

	if (!isParticleDescValid(desc))
	{
		PX_ASSERT(0);
		return;
	}

	PxU32	maxParticles = mMaxParticleCount;
	PxParticleBase*		particle	= NULL;
	PxParticleFluid*	fluid		= NULL;

	ApexSimpleString className(mAsset->getParticleTypeClassName());
	if (className == SimpleParticleSystemParams::staticClassName())
	{
		mIsParticleSystem	= true;
		particle			= scene->getPhysics().createParticleSystem(maxParticles, desc->PerParticleRestOffset);
	}
	else
	{
		mIsParticleSystem	= false;
		fluid				= scene->getPhysics().createParticleFluid(maxParticles, desc->PerParticleRestOffset);
		particle			= fluid;
	}

	if (particle)
	{
		particle->setMaxMotionDistance(desc->maxMotionDistance);
		particle->setContactOffset(desc->contactOffset);
		particle->setRestOffset(desc->restOffset);
		particle->setGridSize(desc->gridSize);
		particle->setDamping(desc->damping);
		particle->setExternalAcceleration(desc->externalAcceleration);
		particle->setProjectionPlane(desc->projectionPlaneNormal, desc->projectionPlaneDistance);
		particle->setParticleMass(desc->particleMass);
		particle->setRestitution(desc->restitution);
		particle->setDynamicFriction(desc->dynamicFriction);
		particle->setStaticFriction(desc->staticFriction);
		if (desc->simulationFilterData && desc->simulationFilterData[0])
		{
			NiResourceProvider* nrp = mAsset->mModule->mSdk->getInternalResourceProvider();

			NxResID cgmns = mAsset->mModule->mSdk->getCollisionGroup128NameSpace();
			NxResID cgmresid = nrp->createResource(cgmns, desc->simulationFilterData);
			void* tmpCGM = nrp->getResource(cgmresid);
			if (tmpCGM)
			{
				particle->setSimulationFilterData(*(static_cast<PxFilterData*>(tmpCGM)));
			}
			//nrp->releaseResource( cgresid );
		}
		particle->setParticleBaseFlag(PxParticleBaseFlag::eCOLLISION_TWOWAY, desc->CollisionTwoway);
		particle->setParticleBaseFlag(PxParticleBaseFlag::eCOLLISION_WITH_DYNAMIC_ACTORS, desc->CollisionWithDynamicActors);
		particle->setParticleBaseFlag(PxParticleBaseFlag::eENABLED, desc->Enable);
		particle->setParticleBaseFlag(PxParticleBaseFlag::ePROJECT_TO_PLANE, desc->ProjectToPlane);
		// PxParticleBaseFlag::ePER_PARTICLE_REST_OFFSET is set in create() function
		particle->setParticleBaseFlag(PxParticleBaseFlag::ePER_PARTICLE_COLLISION_CACHE_HINT, desc->PerParticleCollisionCacheHint);
		// set hardware flag only if hardware is available
		particle->setParticleBaseFlag(PxParticleBaseFlag::eGPU, NULL != scene->getTaskManager()->getGpuDispatcher());
		
		particle->setParticleReadDataFlag(PxParticleReadDataFlag::ePOSITION_BUFFER, true);
		particle->setParticleReadDataFlag(PxParticleReadDataFlag::eVELOCITY_BUFFER, true);
		particle->setParticleReadDataFlag(PxParticleReadDataFlag::eREST_OFFSET_BUFFER, true);
		particle->setParticleReadDataFlag(PxParticleReadDataFlag::eFLAGS_BUFFER, true);
		particle->setParticleReadDataFlag(PxParticleReadDataFlag::eCOLLISION_NORMAL_BUFFER, true);
		if (fluid)
		{
			particle->setParticleReadDataFlag(PxParticleReadDataFlag::eDENSITY_BUFFER, desc->DensityBuffer);

			const FluidParticleSystemParams*	fluidDesc	= (FluidParticleSystemParams*)desc->particleType;
			fluid->setRestParticleDistance(fluidDesc->restParticleDistance);
			fluid->setStiffness(fluidDesc->stiffness);
			fluid->setViscosity(fluidDesc->viscosity);
		}
	}
	mParticleActor = particle;

	PX_ASSERT(mParticleActor);

	scene->addActor(*mParticleActor);
	if (mParticleIosScene->getNiFieldSamplerManager())
	{
		mParticleIosScene->getNiFieldSamplerManager()->registerUnhandledParticleSystem(mParticleActor);
	}
	PX_ASSERT(mParticleActor->getScene());

	mIofxMgr->setSimulationParameters(desc->restOffset, mUp, mGravity, 1 / desc->restOffset);
}

void ParticleIosActor::removeFromScene()
{
	if (mParticleActor)
	{
		if (mParticleIosScene->getNiFieldSamplerManager())
		{
			mParticleIosScene->getNiFieldSamplerManager()->unregisterUnhandledParticleSystem(mParticleActor);
		}
		SCOPED_PHYSX3_LOCK_WRITE(mParticleActor->getScene());
		mParticleActor->getScene()->removeActor(*mParticleActor);
		mParticleActor->release();
	}
	mParticleActor = NULL;
	mParticleCount = 0;
}



void ParticleIosActor::getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly)
{
	PX_UNUSED(min);
	PX_UNUSED(max);
	PX_UNUSED(intOnly);
	APEX_INVALID_OPERATION("not implemented");
}


physx::PxF32 ParticleIosActor::getActivePhysicalLod()
{
	APEX_INVALID_OPERATION("NxParticleIosActor does not support this operation");
	return -1.0f;
}


void ParticleIosActor::forcePhysicalLod(physx::PxF32 lod)
{
	PX_UNUSED(lod);
	APEX_INVALID_OPERATION("not implemented");
}



const physx::PxVec3* ParticleIosActor::getRecentPositions(physx::PxU32& count, physx::PxU32& stride) const
{
	count = mParticleCount;
	stride = sizeof(PxVec4);
	return (const PxVec3*) mBufDesc.pmaPositionMass->getPtr();
}

NiIosInjector* ParticleIosActor::allocateInjector(const char* iofxAssetName)
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

	ParticleParticleInjector* inj = 0;
	//createInjector
	{
		physx::PxU32 injectorID = mParticleIosScene->getInjectorAllocator().allocateInjectorID();
		if (injectorID != ParticleIosInjectorAllocator::NULL_INJECTOR_INDEX)
		{
			inj = PX_NEW(ParticleParticleInjector)(mInjectorList, *this, injectorID);
		}
	}
	if (inj == 0)
	{
		APEX_INTERNAL_ERROR("Failed to create new ParticleIos injector.");
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

void ParticleIosActor::releaseInjector(NiIosInjector& injector)
{
	ParticleParticleInjector* inj = DYNAMIC_CAST(ParticleParticleInjector*)(&injector);

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

	//destroyInjector
	{
		//set mLODBias to FLT_MAX to mark released injector
		//all particles from released injectors will be removed in simulation
		Px3InjectorParams& injParams = mParticleIosScene->getInjectorParams(inj->mInjectorID);
		injParams.mLODBias = FLT_MAX;

		mParticleIosScene->getInjectorAllocator().releaseInjectorID(inj->mInjectorID);
		inj->destroy();
	}

    if(mInjectorList.getSize() == 0)
    {
        //if we have no injectors - release self
        release();
    }
}

void ParticleIosActor::visualize()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if(mParticleIosScene->mParticleIosDebugRenderParams->VISUALIZE_PARTICLE_IOS_GRID_DENSITY)
	{
		physx::RenderDebug* renderer = mParticleIosScene->mRenderDebug;
		if(mGridDensityParams.Enabled)
		{					
			renderer->setCurrentColor(0x0000ff);
			PxU32 onScreenRes = mGridDensityParams.GridResolution - 4;
			for (PxU32 i = 0 ; i <= onScreenRes; i++)
			{
				PxF32 u = 2.f*((PxF32)i/(onScreenRes))-1.f;
				PxVec4 a = mDensityDebugMatInv.transform(PxVec4(u,-1.f,0.1f,1.f));
				PxVec4 b = mDensityDebugMatInv.transform(PxVec4(u, 1.f,0.1f,1.f));
				PxVec4 c = mDensityDebugMatInv.transform(PxVec4(-1.f,u,0.1f,1.f));
				PxVec4 d = mDensityDebugMatInv.transform(PxVec4( 1.f,u,0.1f,1.f));
				renderer->debugLine(PxVec3(a.getXYZ()/a.w),PxVec3(b.getXYZ()/b.w));
				renderer->debugLine(PxVec3(c.getXYZ()/c.w),PxVec3(d.getXYZ()/d.w));
			}
		}
	}
#endif
}


physx::PxTaskID ParticleIosActor::submitTasks(physx::PxTaskManager* tm, physx::PxTaskID /*taskFinishBeforeID*/)
{
	return tm->submitUnnamedTask(mInjectTask);
}

void ParticleIosActor::setTaskDependencies(physx::PxTask& iosTask, bool isDataOnDevice)
{
	physx::PxTaskManager* tm = mParticleIosScene->getApexScene().getTaskManager();

	physx::PxTaskID lodTaskID = tm->getNamedTask(AST_LOD_COMPUTE_BENEFIT);
	mInjectTask.finishBefore(lodTaskID);
	iosTask.startAfter(lodTaskID);

	if (mFieldSamplerQuery != NULL)
	{
		physx::PxF32 deltaTime = mParticleIosScene->getApexScene().getPhysXSimulateTime();

		NiFieldSamplerQueryData queryData;
		queryData.timeStep = deltaTime;
		queryData.count = mParticleCount;
		queryData.isDataOnDevice = isDataOnDevice;
		queryData.strideBytes = sizeof(physx::PxVec4);
		queryData.massStrideBytes = sizeof(physx::PxVec4);		
		if (isDataOnDevice)
		{
#if defined(APEX_CUDA_SUPPORT)
			queryData.pmaInPosition = (PxF32*)mBufDesc.pmaPositionMass->getGpuPtr();
			queryData.pmaInVelocity = (PxF32*)mBufDesc.pmaVelocityLife->getGpuPtr();
			queryData.pmaInMass = &mBufDesc.pmaPositionMass->getGpuPtr()->w;
			queryData.pmaOutField = mField.getGpuPtr();
#endif
		}
		else
		{
			queryData.pmaInPosition = (PxF32*)mBufDesc.pmaPositionMass->getPtr();
			queryData.pmaInVelocity = (PxF32*)mBufDesc.pmaVelocityLife->getPtr();
			queryData.pmaInMass = &mBufDesc.pmaPositionMass->getPtr()->w;
			queryData.pmaOutField = mField.getPtr();
		}

		mFieldSamplerQuery->submitFieldSamplerQuery(queryData, iosTask.getTaskID());
	}

	physx::PxTaskID postIofxTaskID = tm->getNamedTask(AST_PHYSX_FETCH_RESULTS);
	PxTaskID iofxTaskID = mIofxMgr->getUpdateEffectsTaskID(postIofxTaskID);
	if (iofxTaskID == 0)
	{
		iofxTaskID = postIofxTaskID;
	}
	iosTask.finishBefore(iofxTaskID);
}

void ParticleIosActor::fetchResults()
{
	for(PxU32 i = 0; i < mInjectorList.getSize(); ++i)
	{
		ParticleParticleInjector* inj = DYNAMIC_CAST(ParticleParticleInjector*)(mInjectorList.getResource(i));
		inj->assignSimParticlesCount(mInjectorsCounters.get(i));
	}
}


physx::PxF32 ParticleIosActor::getBenefit()
{
	if ( !PxIsFinite(mLastBenefitSum))
	{
#ifdef PX_DEBUG
		APEX_DEBUG_INFO("Invalid LastBenefitSum");
#endif
		mLastBenefitSum = 0.0f;
	}
	physx::PxF32 totalBenefit = mLastBenefitSum + mInjectedBenefitSum;
	PX_ASSERT(PxIsFinite(totalBenefit));
	physx::PxU32 totalCount = mLastActiveCount + mInjectedCount;
	
	physx::PxF32 averageBenefit = 0.0f;
	if (totalCount > 0)
	{
		averageBenefit = totalBenefit / totalCount;
		PX_ASSERT( PxIsFinite(averageBenefit));
	}
	return averageBenefit * mInjectorList.getSize();
}

physx::PxF32 ParticleIosActor::setResource(physx::PxF32 resourceBudget, physx::PxF32 maxRemaining, physx::PxF32 relativeBenefit)
{
	PX_UNUSED(maxRemaining);
	PX_UNUSED(relativeBenefit);

	physx::PxF32 unitCost = mParticleIosScene->mModule->getLODUnitCost();
	if (mParticleIosScene->mModule->getLODEnabled())
	{
		resourceBudget /= unitCost;
		mParticleBudget = (resourceBudget < UINT_MAX) ? static_cast<physx::PxU32>(resourceBudget) : UINT_MAX;
	}
	else
	{
		mParticleBudget = UINT_MAX;
	}

	if (mParticleBudget > mMaxParticleCount)
	{
		mParticleBudget = mMaxParticleCount;
	}

	physx::PxU32 activeCount = mLastActiveCount + mInjectedCount;
	if (mParticleBudget > activeCount)
	{
		mParticleBudget = activeCount;
	}

	return static_cast<physx::PxF32>(mParticleBudget) * unitCost;
}


void ParticleIosActor::injectNewParticles()
{
	mInjectedBenefitSum = 0;
	mInjectedBenefitMin = +FLT_MAX;
	mInjectedBenefitMax = -FLT_MAX;

	physx::PxU32 maxInjectCount = (mMaxTotalParticleCount - mParticleCount);

	physx::PxU32 injectCount = 0;
	physx::PxU32 lastInjectCount = 0;
	do
	{
		lastInjectCount = injectCount;
		for (physx::PxU32 i = 0; i < mInjectorList.getSize(); i++)
		{
			ParticleParticleInjector* inj = DYNAMIC_CAST(ParticleParticleInjector*)(mInjectorList.getResource(i));
			if (inj->mInjectedParticles.size() == 0)
			{
				continue;
			}

			if (injectCount < maxInjectCount)
			{
				IosNewObject obj;
				if (inj->mInjectedParticles.popFront(obj))
				{
					PxU32 injectIndex = mParticleCount + injectCount;

					physx::PxF32 particleMass = mAsset->getParticleMass();
					mBufDesc.pmaPositionMass->get(injectIndex) = PxVec4(obj.initialPosition.x, obj.initialPosition.y, obj.initialPosition.z, particleMass);
					mBufDesc.pmaVelocityLife->get(injectIndex) = PxVec4(obj.initialVelocity.x, obj.initialVelocity.y, obj.initialVelocity.z, 1.0f);
					mBufDesc.pmaCollisionNormalFlags->get(injectIndex).setZero();
					mBufDesc.pmaActorIdentifiers->get(injectIndex) = obj.iofxActorID;

					mBufDesc.pmaUserData->get(injectIndex) = obj.userData;

					mLifeSpan[injectIndex] = obj.lifetime;
					mInjector[injectIndex] = inj->mInjectorID;
					mBenefit[injectIndex] = obj.lodBenefit;

					mInjectedBenefitSum += obj.lodBenefit;
					mInjectedBenefitMin = PxMin(mInjectedBenefitMin, obj.lodBenefit);
					mInjectedBenefitMax = PxMax(mInjectedBenefitMax, obj.lodBenefit);

					++injectCount;
				}
			}
		}
	}
	while (injectCount > lastInjectCount);

	mInjectedCount = injectCount;

	//clear injectors FIFO
	for (physx::PxU32 i = 0; i < mInjectorList.getSize(); i++)
	{
		ParticleParticleInjector* inj = DYNAMIC_CAST(ParticleParticleInjector*)(mInjectorList.getResource(i));

		IosNewObject obj;
		while (inj->mInjectedParticles.popFront(obj))
		{
			;
		}
	}
}

bool ParticleIosActor::isParticleDescValid( const ParticleIosAssetParam* desc) const
{
	if (desc->gridSize <= 0.0f) return false;
	if (desc->maxMotionDistance <= 0.0f) return false;
	if (desc->maxMotionDistance + desc->contactOffset > desc->gridSize) return false;
	if (desc->contactOffset < 0.0f) return false;
	if (desc->contactOffset < desc->restOffset) return false;
	if (desc->particleMass < 0.0f) return false;
	if (desc->damping < 0.0f) return false;
	if (desc->projectionPlaneNormal.isZero()) return false;
	if (desc->restitution < 0.0f || desc->restitution > 1.0f) return false;
	if (desc->dynamicFriction < 0.0f || desc->dynamicFriction > 1.0f) return false;
	if (desc->staticFriction < 0.0f) return false;
	if (desc->maxParticleCount < 1) return false;

	ApexSimpleString className(mAsset->getParticleTypeClassName());
	if (className == SimpleParticleSystemParams::staticClassName())
	{
		return true;
	}
	else
	if (className == FluidParticleSystemParams::staticClassName())
	{
		const FluidParticleSystemParams*	fluidDesc	= (FluidParticleSystemParams*)desc->particleType;
		if (fluidDesc->restParticleDistance <= 0.0f) return false;

		if (fluidDesc->stiffness <= 0.0f) return false;
		if (fluidDesc->viscosity <= 0.0f) return false;

		return true;
	}
	else
	{
		return false;
	}
}
////////////////////////////////////////////////////////////////////////////////

ParticleParticleInjector::ParticleParticleInjector(NxResourceList& list, ParticleIosActor& actor, physx::PxU32 injectorID)
	: mIosActor(&actor)
	, mIofxAssetResID(INVALID_RESOURCE_ID)
	, mIofxAsset(NULL)
	, mVolume(NULL)
	, mLastRandomID(0)
	, mVolumeID(NiIofxActorID::NO_VOLUME)
	, mInjectorID(injectorID)
	, mSimulatedParticlesCount(0)
{
	list.add(*this);

	setLODWeights(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

void ParticleParticleInjector::setListIndex(NxResourceList& list, physx::PxU32 index)
{
	m_listIndex = index;
	m_list = &list;

	Px3InjectorParams& injParams = mIosActor->mParticleIosScene->getInjectorParams(mInjectorID);

	injParams.mLocalIndex = index;
}

/* Emitter calls this function to adjust their particle weights with respect to other emitters */
void ParticleParticleInjector::setLODWeights(physx::PxF32 maxDistance, physx::PxF32 distanceWeight, physx::PxF32 speedWeight, physx::PxF32 lifeWeight, physx::PxF32 separationWeight, physx::PxF32 bias)
{
	PX_UNUSED(separationWeight);

	Px3InjectorParams& injParams = mIosActor->mParticleIosScene->getInjectorParams(mInjectorID);

	injParams.mLODMaxDistance = maxDistance;
	injParams.mLODDistanceWeight = distanceWeight;
	injParams.mLODSpeedWeight = speedWeight;
	injParams.mLODLifeWeight = lifeWeight;
	injParams.mLODBias = bias;
}

physx::PxTaskID ParticleParticleInjector::getCompletionTaskID() const
{
	return mIosActor->mInjectTask.getTaskID();
}

void ParticleParticleInjector::init(NxResID iofxAssetResID, NxIofxAsset* iofxAsset)
{
	mIofxAssetResID = iofxAssetResID;
	mIofxAsset = iofxAsset;

	/* add this injector to the IOFX asset's context (so when the IOFX goes away our ::release() is called) */
	mIofxAsset->addDependentActor(this);

	mRandomActorClassIDs.clear();
	if (mIofxAsset->getMeshAssetCount() < 2)
	{
		mRandomActorClassIDs.pushBack(mIosActor->mIofxMgr->getActorClassID(mIofxAsset, 0));
		return;
	}

	/* Cache actorClassIDs for this asset */
	physx::Array<PxU16> temp;
	for (PxU32 i = 0 ; i < mIofxAsset->getMeshAssetCount() ; i++)
	{
		PxU32 w = mIofxAsset->getMeshAssetWeight(i);
		PxU16 acid = mIosActor->mIofxMgr->getActorClassID(mIofxAsset, (PxU16) i);
		for (PxU32 j = 0 ; j < w ; j++)
		{
			temp.pushBack(acid);
		}
	}

	mRandomActorClassIDs.reserve(temp.size());
	while (temp.size())
	{
		PxU32 index = physx::rand(0, temp.size() - 1);
		mRandomActorClassIDs.pushBack(temp[ index ]);
		temp.replaceWithLast(index);
	}
}


void ParticleParticleInjector::release()
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	mIosActor->releaseInjector(*this);
}

void ParticleParticleInjector::destroy()
{
	// Release reference counts to IOFX asset
	if (mIofxAssetResID != INVALID_RESOURCE_ID)
	{
		NiResourceProvider* nrp = NiGetApexSDK()->getInternalResourceProvider();
		nrp->releaseResource(mIofxAssetResID);
	}

	delete this;
}

void ParticleParticleInjector::setPreferredRenderVolume(NxApexRenderVolume* volume)
{
	mVolume = volume;
	mVolumeID = mVolume ? mIosActor->mIofxMgr->getVolumeID(mVolume) : NiIofxActorID::NO_VOLUME;
}

/* Emitter calls this virtual injector API to insert new particles.  It is safe for an emitter to
 * call this function at any time except for during the IOS::fetchResults().  Since
 * ParticleScene::fetchResults() is single threaded, it should be safe to call from
 * emitter::fetchResults() (destruction may want to do this because of contact reporting)
 */
void ParticleParticleInjector::createObjects(physx::PxU32 count, const IosNewObject* createList)
{
	PX_PROFILER_PERF_SCOPE("ParticleIosCreateObjects");

	if (mRandomActorClassIDs.size() == 0)
	{
		return;
	}

	const physx::PxVec3& eyePos = mIosActor->mParticleIosScene->getApexScene().getEyePosition();
	const Px3InjectorParams& injParams = mIosActor->mParticleIosScene->getInjectorParams(mInjectorID);
	// Append new objects to our FIFO.  We do copies because we must perform buffering for the
	// emitters.  We have to hold these new objects until there is room in the NxFluid and the
	// injector's virtID range to emit them.
	for (physx::PxU32 i = 0 ; i < count ; i++)
	{
		IosNewObject obj = *createList++;

		obj.lodBenefit = calcParticleBenefit(injParams, eyePos, obj.initialPosition, obj.initialVelocity, 1.0f);
		obj.iofxActorID.set(mVolumeID, mRandomActorClassIDs[ mLastRandomID++ ]);
		mLastRandomID = mLastRandomID == mRandomActorClassIDs.size() ? 0 : mLastRandomID;
		//mInjectedParticleBenefit += obj.lodBenefit;
		mInjectedParticles.pushBack(obj);
	}
}

#if defined(APEX_CUDA_SUPPORT)
void ParticleParticleInjector::createObjects(ApexMirroredArray<const IosNewObject>& createArray)
{
	PX_UNUSED(createArray);

	// An emitter will call this API when it has filled a host or device buffer.  The injector
	// should trigger a copy to the location it would like to see the resulting data when the
	// IOS is finally ticked.

	PX_ALWAYS_ASSERT(); /* Not yet supported */
}
#endif

physx::PxF32 ParticleParticleInjector::getBenefit()
{
	return 0.0f;
}

physx::PxF32 ParticleParticleInjector::setResource(physx::PxF32 suggested, physx::PxF32 maxRemaining, physx::PxF32 relativeBenefit)
{
	PX_UNUSED(suggested);
	PX_UNUSED(maxRemaining);
	PX_UNUSED(relativeBenefit);

	return 0.0f;
}

physx::PxU32 ParticleParticleInjector::getActivePaticleCount() const
{
	return mSimulatedParticlesCount;
}

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_MAJOR == 3
