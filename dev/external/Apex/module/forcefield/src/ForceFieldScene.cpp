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
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

#include "NxApex.h"
#include "PsShare.h"

#include "ForceFieldScene.h"
#include "ForceFieldActor.h"
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
namespace forcefield
{

ForceFieldScene::ForceFieldScene(ModuleForceField& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list)
	: mRenderDebug(renderDebug)
	, mFieldSamplerManager(NULL)
	, mUpdateTask(*this)
{
	mModule = &module;
	mApexScene = &scene;
	mPhysXScene = NULL;
	list.add(*this);		// Add self to module's list of ForceFieldScenes

	/* Initialize reference to ForceFieldDebugRenderParams */
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
			memberHandle.initParamRef(ForceFieldDebugRenderParams::staticClassName(), true);
		}
	}

	/* Load reference to ForceFieldDebugRenderParams */
	NxParameterized::Interface* refPtr = NULL;
	memberHandle.getParamRef(refPtr);
	mForceFieldDebugRenderParams = DYNAMIC_CAST(ForceFieldDebugRenderParams*)(refPtr);
	PX_ASSERT(mForceFieldDebugRenderParams);
}

ForceFieldScene::~ForceFieldScene()
{
}

// Called by scene task graph between LOD and PhysX::simulate()
void ForceFieldScene::TaskUpdate::run()
{
	setProfileStat((PxU16) mOwner.mActorArray.size());
	physx::PxF32 dt = mOwner.mApexScene->getElapsedTime();
	mOwner.updateActors(dt);
}

// Called by updateTask between LOD and PhysX simulate.  Any writes
// to render data must be protected by acquiring the actor's render data lock
void ForceFieldScene::updateActors(physx::PxF32 dt)
{
	SCOPED_PHYSX_LOCK_WRITE(*mApexScene);
	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		ForceFieldActor* actor = DYNAMIC_CAST(ForceFieldActor*)(mActorArray[i]);
		actor->updateForceField(dt);
	}
}

// submit the task that updates the explosion actors
// called from ApexScene::simulate()
void ForceFieldScene::submitTasks(PxF32 /*elapsedTime*/, PxF32 /*substepSize*/, PxU32 /*numSubSteps*/)
{
	physx::PxTaskManager* tm = mApexScene->getTaskManager();
	tm->submitUnnamedTask(mUpdateTask);
	//mUpdateTask.startAfter(tm->getNamedTask(AST_LOD_COMPUTE_BENEFIT));
	mUpdateTask.startAfter(tm->getNamedTask(FSST_PHYSX_MONITOR_LOAD));
	//mUpdateTask.finishBefore(tm->getNamedTask(AST_PHYSX_SIMULATE));
	mUpdateTask.finishBefore(tm->getNamedTask(FSST_PHYSX_MONITOR_FETCH));
}

void ForceFieldScene::setTaskDependencies()
{
}

// Called by ApexScene::fetchResults() with all actors render data locked.
void ForceFieldScene::fetchResults()
{
	PX_PROFILER_PERF_SCOPE("ForceFieldSceneFetchResults");

	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		ForceFieldActor* actor = DYNAMIC_CAST(ForceFieldActor*)(mActorArray[i]);
		actor->updatePoseAndBounds();
	}
}

void ForceFieldScene::visualize()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (!mForceFieldDebugRenderParams->VISUALIZE_FORCEFIELD_ACTOR)
	{
		return;
	}

	mRenderDebug->pushRenderState();
	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		ForceFieldActor* actor = DYNAMIC_CAST(ForceFieldActor*)(mActorArray[i]);
		actor->visualize();
	}
	mRenderDebug->popRenderState();
#endif
}

void ForceFieldScene::visualizeForceFieldForces()
{
}

void ForceFieldScene::visualizeForceFieldForceFields()
{
}

void ForceFieldScene::destroy()
{
	removeAllActors();
	mApexScene->moduleReleased(*this);
	delete this;
}

void ForceFieldScene::setModulePhysXScene(PxScene* pxScene)
{
	if (pxScene)
	{
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			ForceFieldActor* actor = DYNAMIC_CAST(ForceFieldActor*)(mActorArray[i]);
			actor->setPhysXScene(pxScene);
		}
	}
	else
	{
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			ForceFieldActor* actor = DYNAMIC_CAST(ForceFieldActor*)(mActorArray[i]);
			actor->setPhysXScene(NULL);
		}
	}

	mPhysXScene = pxScene;
}

NiFieldSamplerManager* ForceFieldScene::getNiFieldSamplerManager()
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

ForceFieldSceneCPU::ForceFieldSceneCPU(ModuleForceField& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list) :
	ForceFieldScene(module, scene, renderDebug, list)
{
}

ForceFieldSceneCPU::~ForceFieldSceneCPU()
{
}

ForceFieldActor* ForceFieldSceneCPU::createForceFieldActor(const NxForceFieldActorDesc& desc, ForceFieldAsset& asset, NxResourceList& list)
{
	return PX_NEW(ForceFieldActorCPU)(desc, asset, list, *this);
}

/******************************** GPU Version ********************************/

#if defined(APEX_CUDA_SUPPORT)

ForceFieldSceneGPU::ForceFieldSceneGPU(ModuleForceField& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list)
	: ForceFieldScene(module, scene, renderDebug, list)
	, CudaModuleScene(scene, *mModule)
{
	{
		physx::PxGpuDispatcher* gd = mApexScene->getTaskManager()->getGpuDispatcher();
		PX_ASSERT(gd != NULL);
		mCtxMgr = gd->getCudaContextManager();
		physx::PxScopedCudaLock _lock_(*mCtxMgr);

//CUDA module objects
#include "../cuda/include/ForceField.h"
	}
}

ForceFieldSceneGPU::~ForceFieldSceneGPU()
{
	CudaModuleScene::destroy(*mApexScene);
}

ForceFieldActor* ForceFieldSceneGPU::createForceFieldActor(const NxForceFieldActorDesc& desc, ForceFieldAsset& asset, NxResourceList& list)
{
	return PX_NEW(ForceFieldActorGPU)(desc, asset, list, *this);
}

ApexCudaConstMem* ForceFieldSceneGPU::getFieldSamplerCudaConstMem()
{
	return &SCENE_CUDA_OBJ(this, fieldsamplerConstMem);
}

bool ForceFieldSceneGPU::launchFieldSamplerCudaKernel(const fieldsampler::NiFieldSamplerKernelLaunchData& launchData)
{
	LAUNCH_FIELD_SAMPLER_KERNEL(launchData);
}

#endif

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
