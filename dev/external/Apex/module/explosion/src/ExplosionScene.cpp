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
#include "MinPhysxSdkVersion.h"
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2

#include "NxApex.h"
#include "PsShare.h"

#if NX_SDK_VERSION_MAJOR == 2
#include "nxmath.h"
#endif

#include "ExplosionScene.h"
#include "ExplosionActor.h"
#include "NiApexScene.h"
#include "ModulePerfScope.h"
#include "NiModuleFieldSampler.h"

#if defined(APEX_CUDA_SUPPORT)
#include "ApexCudaSource.h"
#endif

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

namespace physx
{
namespace apex
{
namespace explosion
{

ExplosionScene::ExplosionScene(ModuleExplosion& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list)
	: mRenderDebug(renderDebug)
	, mFieldSamplerManager(NULL)
	, mUpdateTask(*this)
{
	mModule = &module;
	mApexScene = &scene;
	mPhysXScene = NULL;
	list.add(*this);		// Add self to module's list of ExplosionScenes

	/* Initialize reference to ExplosionDebugRenderParams */
	mDebugRenderParams = DYNAMIC_CAST(DebugRenderParams*)(mApexScene->getDebugRenderParams());
	PX_ASSERT(mDebugRenderParams);
	NxParameterized::Handle handle(*mDebugRenderParams), memberHandle(*mDebugRenderParams);
	int size;

	if (mDebugRenderParams->getParameterHandle("moduleName", handle) == NxParameterized::ERROR_NONE)
	{
		handle.getArraySize(size, 0);
		handle.resizeArray(size + 1);
		if (handle.getChildHandle(size, memberHandle) == NxParameterized::ERROR_NONE)
		{
			memberHandle.initParamRef(ExplosionDebugRenderParams::staticClassName(), true);
		}
	}

	/* Load reference to ExplosionDebugRenderParams */
	NxParameterized::Interface* refPtr = NULL;
	memberHandle.getParamRef(refPtr);
	mExplosionDebugRenderParams = DYNAMIC_CAST(ExplosionDebugRenderParams*)(refPtr);
	PX_ASSERT(mExplosionDebugRenderParams);
}

ExplosionScene::~ExplosionScene()
{
}

// Called by scene task graph between LOD and PhysX::simulate()
void ExplosionScene::TaskUpdate::run()
{
	setProfileStat((PxU16) mOwner.mActorArray.size());
	physx::PxF32 dt = mOwner.mApexScene->getElapsedTime();
	mOwner.updateActors(dt);
}

// Called by updateTask between LOD and PhysX simulate.  Any writes
// to render data must be protected by acquiring the actor's render data lock
void ExplosionScene::updateActors(physx::PxF32 dt)
{
	mApexScene->acquirePhysXLock();
	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		ExplosionActor* actor = DYNAMIC_CAST(ExplosionActor*)(mActorArray[i]);
		actor->updateExplosion(dt);
	}
	mApexScene->releasePhysXLock();
}

// submit the task that updates the explosion actors
// called from ApexScene::simulate()
void ExplosionScene::submitTasks(PxF32 /*elapsedTime*/, PxF32 /*substepSize*/, PxU32 /*numSubSteps*/)
{
	physx::PxTaskManager* tm = mApexScene->getTaskManager();
	tm->submitUnnamedTask(mUpdateTask);
	mUpdateTask.startAfter(tm->getNamedTask(AST_LOD_COMPUTE_BENEFIT));
	mUpdateTask.finishBefore(tm->getNamedTask(AST_PHYSX_SIMULATE));
}

// Called by ApexScene::fetchResults() with all actors render data locked.
void ExplosionScene::fetchResults()
{
	PX_PROFILER_PERF_SCOPE("ExplosionSceneFetchResults");

	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		ExplosionActor* actor = DYNAMIC_CAST(ExplosionActor*)(mActorArray[i]);
		actor->updatePoseAndBounds();
	}
}

void ExplosionScene::visualize()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (!mExplosionDebugRenderParams->VISUALIZE_EXPLOSION_ACTOR)
	{
		return;
	}

	// save the rendering state
	mRenderDebug->pushRenderState();
	{
		visualizeExplosionForceFields();
		visualizeExplosionForces();
	}
	// restore the rendering state
	mRenderDebug->popRenderState();
#endif
}

void ExplosionScene::visualizeExplosionForces()
{
	if (mExplosionDebugRenderParams->VISUALIZE_EXPLOSION_FORCES)
	{
#if NX_SDK_VERSION_MAJOR == 2
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			ExplosionActor* actor = DYNAMIC_CAST(ExplosionActor*)(mActorArray[i]);
			if ((actor->isEnable()) && (!(actor->mInRelease)) && (actor->mNxForceField != NULL))
			{
				actor->visualizeExplosionForces();
			}
		}
#endif
	}
}

void ExplosionScene::visualizeExplosionForceFields()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (mExplosionDebugRenderParams->VISUALIZE_EXPLOSION_FORCE_FIELDS)
	{
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			ExplosionActor* actor = DYNAMIC_CAST(ExplosionActor*)(mActorArray[i]);
			if (!actor->isEnable())
			{
				continue;
			}

#if NX_SDK_VERSION_MAJOR == 2
			physx::PxF32 innerRadius = actor->getInnerBoundRadius();
			physx::PxF32 outerRadius = actor->getOuterBoundRadius();
			physx::PxMat34Legacy pose = actor->getPose();

			if (outerRadius >= 0)
			{
				mRenderDebug->setCurrentColor(mRenderDebug->getDebugColor(DebugColors::Red));
				mRenderDebug->debugDetailedSphere(pose.t, outerRadius, 16);
			}
			if (innerRadius >= 0)
			{
				mRenderDebug->setCurrentColor(mRenderDebug->getDebugColor(DebugColors::Red));
				mRenderDebug->debugDetailedSphere(pose.t, innerRadius, 16);
			}
#endif
		}
	}
#endif
}

void ExplosionScene::destroy()
{
	removeAllActors();
	mApexScene->moduleReleased(*this);
	delete this;
}

#if NX_SDK_VERSION_MAJOR == 2
void ExplosionScene::setModulePhysXScene(NxScene* nxScene)
{
	if (nxScene)
	{
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			ExplosionActor* actor = DYNAMIC_CAST(ExplosionActor*)(mActorArray[i]);
			actor->setPhysXScene(nxScene);
		}
	}
	else
	{
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			ExplosionActor* actor = DYNAMIC_CAST(ExplosionActor*)(mActorArray[i]);
			actor->setPhysXScene(NULL);
		}
	}

	mPhysXScene = nxScene;
}
#elif NX_SDK_VERSION_MAJOR == 3
void ExplosionScene::setModulePhysXScene(PxScene* pxScene)
{
	if (pxScene)
	{
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			ExplosionActor* actor = DYNAMIC_CAST(ExplosionActor*)(mActorArray[i]);
			actor->setPhysXScene(pxScene);
		}
	}
	else
	{
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			ExplosionActor* actor = DYNAMIC_CAST(ExplosionActor*)(mActorArray[i]);
			actor->setPhysXScene(NULL);
		}
	}

	mPhysXScene = pxScene;
}
#endif

NiFieldSamplerManager* ExplosionScene::getNiFieldSamplerManager()
{
	if (mFieldSamplerManager == NULL)
	{
		NiModuleFieldSampler* moduleFieldSampler = mModule->getNiModuleFieldSampler();
		if (moduleFieldSampler != NULL)
		{
			mFieldSamplerManager = moduleFieldSampler->getNiFieldSamplerManager(*mApexScene);
			PX_ASSERT(mFieldSamplerManager != NULL);
		}
	}
	return mFieldSamplerManager;
}

/******************************** CPU Version ********************************/

ExplosionSceneCPU::ExplosionSceneCPU(ModuleExplosion& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list) :
	ExplosionScene(module, scene, renderDebug, list)
{
}

ExplosionSceneCPU::~ExplosionSceneCPU()
{
}

ExplosionActor* ExplosionSceneCPU::createExplosionActor(const NxExplosionActorDesc& desc, ExplosionAsset& asset, NxResourceList& list)
{
	return PX_NEW(ExplosionActorCPU)(desc, asset, list, *this);
}

/******************************** GPU Version ********************************/

#if defined(APEX_CUDA_SUPPORT)

ExplosionSceneGPU::ExplosionSceneGPU(ModuleExplosion& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list)
	: ExplosionScene(module, scene, renderDebug, list)
	, CudaModuleScene(scene, module)
{
	{
		physx::PxGpuDispatcher* gd = mApexScene->getTaskManager()->getGpuDispatcher();
		PX_ASSERT(gd != NULL);
		mCtxMgr = gd->getCudaContextManager();
		physx::PxScopedCudaLock _lock_(*mCtxMgr);

//CUDA module objects
#include "../cuda/include/Explosion.h"
	}
}

ExplosionSceneGPU::~ExplosionSceneGPU()
{
	CudaModuleScene::destroy(*mApexScene);
}

ExplosionActor* ExplosionSceneGPU::createExplosionActor(const NxExplosionActorDesc& desc, ExplosionAsset& asset, NxResourceList& list)
{
	return PX_NEW(ExplosionActorGPU)(desc, asset, list, *this);
}

ApexCudaConstMem* ExplosionSceneGPU::getFieldSamplerCudaConstMem()
{
	return &SCENE_CUDA_OBJ(this, fieldsamplerConstMem);
}

bool ExplosionSceneGPU::launchFieldSamplerCudaKernel(const fieldsampler::NiFieldSamplerKernelLaunchData& launchData)
{
	LAUNCH_FIELD_SAMPLER_KERNEL(launchData);
}

#endif

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
