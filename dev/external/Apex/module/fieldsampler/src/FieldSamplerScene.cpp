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

#include "FieldSamplerScene.h"
#include "FieldSamplerManager.h"
#include "FieldSamplerPhysXMonitor.h"
#include "NiApexScene.h"
#include "NiApexRenderDebug.h"
#include "ModulePerfScope.h"

#include "NxFromPx.h"


#if defined(APEX_CUDA_SUPPORT)
#include "PxGpuTask.h"
#include "ApexCudaSource.h"
#endif


namespace physx
{
namespace apex
{
namespace fieldsampler
{

FieldSamplerScene::FieldSamplerScene(ModuleFieldSampler& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list)
	: mModule(&module)
	, mApexScene(&scene)
	, mDebugRender(debugRender)
	, mManager(NULL)
{
	list.add(*this);		// Add self to module's list of FieldSamplerScenes
}

FieldSamplerScene::~FieldSamplerScene()
{
}

void FieldSamplerScene::visualize()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	mDebugRender->pushRenderState();
	// This is using the new debug rendering
	mDebugRender->popRenderState();
#endif
}

void FieldSamplerScene::destroy()
{
#if NX_SDK_VERSION_MAJOR == 3
	PX_DELETE(mPhysXMonitor);
#endif
	PX_DELETE(mManager);

	removeAllActors();
	mApexScene->moduleReleased(*this);
	delete this;
}

NiFieldSamplerManager* FieldSamplerScene::getManager()
{
	if (mManager == NULL)
	{
		mManager = createManager();
		PX_ASSERT(mManager != NULL);
	}
	return mManager;
}


#if NX_SDK_VERSION_MAJOR == 2
void FieldSamplerScene::setModulePhysXScene(NxScene* /*s*/)
{
}
#elif NX_SDK_VERSION_MAJOR == 3
void FieldSamplerScene::setModulePhysXScene(PxScene* s)
{
	if (s)
	{
		mPhysXMonitor->setPhysXScene(s);
	}
	mPhysXScene = s;
}
#endif


void FieldSamplerScene::submitTasks(PxF32 /*elapsedTime*/, PxF32 /*substepSize*/, PxU32 /*numSubSteps*/)
{
#if NX_SDK_VERSION_MAJOR == 3
	mApexScene->getTaskManager()->submitNamedTask(&mPhysXMonitorFetchTask, FSST_PHYSX_MONITOR_FETCH);
	mApexScene->getTaskManager()->submitNamedTask(&mPhysXMonitorLoadTask, FSST_PHYSX_MONITOR_LOAD);
#endif
	if (mManager != NULL)
	{
		mManager->submitTasks();
	}
}

void FieldSamplerScene::setTaskDependencies()
{
#if NX_SDK_VERSION_MAJOR == 3
	if (mPhysXMonitor->isEnable())
	{
		mPhysXMonitor->update();
	}
#endif
	if (mManager != NULL)
	{
		mManager->setTaskDependencies();
	}

#if NX_SDK_VERSION_MAJOR == 3
	// Just in case one of the scene conditions doesn't set a bounding dependency, let's not let these dangle
	PxTaskManager* taskManager = mApexScene->getTaskManager();
	mPhysXMonitorFetchTask.finishBefore(taskManager->getNamedTask(AST_PHYSX_FETCH_RESULTS));
	mPhysXMonitorLoadTask.finishBefore(taskManager->getNamedTask(AST_PHYSX_FETCH_RESULTS));
#endif
}

void FieldSamplerScene::fetchResults()
{
	if (mManager != NULL)
	{
		mManager->fetchResults();
	}
}

#if NX_SDK_VERSION_MAJOR == 3
void FieldSamplerScene::enablePhysXMonitor(bool enable)
{
	PX_UNUSED(enable);
	mPhysXMonitor->enablePhysXMonitor(enable);
}

void FieldSamplerScene::addPhysXFilterData(physx::PxFilterData filterData)
{
	mPhysXMonitor->addPhysXFilterData(filterData);
}

void FieldSamplerScene::removePhysXFilterData(physx::PxFilterData filterData)
{
	mPhysXMonitor->removePhysXFilterData(filterData);
}
#endif

#ifdef APEX_TEST
bool FieldSamplerScene::setPhysXMonitorParticlesData(physx::PxU32 numParticles, physx::PxVec4** positions, physx::PxVec4** velocities)
{
	PX_UNUSED(numParticles);
	PX_UNUSED(positions);
	PX_UNUSED(velocities);
#if NX_SDK_VERSION_MAJOR == 3
	return mPhysXMonitor->setPhysXMonitorParticlesData(numParticles, positions, velocities);
#else
	return false;
#endif
}

void FieldSamplerScene::getPhysXMonitorParticlesData(physx::PxVec4** velocities)
{
	PX_UNUSED(velocities);
#if NX_SDK_VERSION_MAJOR == 3
	mPhysXMonitor->getPhysXMonitorParticlesData(velocities);
#endif
}
#endif

/******************************** CPU Version ********************************/


FieldSamplerSceneCPU::FieldSamplerSceneCPU(ModuleFieldSampler& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list) :
	FieldSamplerScene(module, scene, debugRender, list)
{
#if NX_SDK_VERSION_MAJOR == 3
	mPhysXMonitor = PX_NEW(FieldSamplerPhysXMonitorCPU)(*this);
#endif
}

FieldSamplerSceneCPU::~FieldSamplerSceneCPU()
{
}

FieldSamplerManager* FieldSamplerSceneCPU::createManager()
{
	return PX_NEW(FieldSamplerManagerCPU)(this);
}

/******************************** GPU Version ********************************/

#if defined(APEX_CUDA_SUPPORT)

FieldSamplerSceneGPU::FieldSamplerSceneGPU(ModuleFieldSampler& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list)
	: FieldSamplerScene(module, scene, debugRender, list)
	, CudaModuleScene(scene, *mModule)
{
#if NX_SDK_VERSION_MAJOR == 3
	mPhysXMonitor = PX_NEW(FieldSamplerPhysXMonitorGPU)(*this);
#endif
	{
		physx::PxGpuDispatcher* gd = mApexScene->getTaskManager()->getGpuDispatcher();
		PX_ASSERT(gd != NULL);
		mCtxMgr = gd->getCudaContextManager();
		physx::PxScopedCudaLock _lock_(*mCtxMgr);

//CUDA module objects
#include "../cuda/include/fieldsampler.h"
	}
}

FieldSamplerSceneGPU::~FieldSamplerSceneGPU()
{
	CudaModuleScene::destroy(*mApexScene);
}

FieldSamplerManager* FieldSamplerSceneGPU::createManager()
{
	return PX_NEW(FieldSamplerManagerGPU)(this);
}

#endif

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
