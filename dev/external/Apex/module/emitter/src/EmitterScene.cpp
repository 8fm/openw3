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
#include "EmitterScene.h"
#include "NiApexScene.h"
#include "ModulePerfScope.h"

namespace physx
{
namespace apex
{
namespace emitter
{

EmitterScene::EmitterScene(ModuleEmitter& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list) :
	mSumBenefit(0.0f),
	mDebugRender(debugRender)
{
	mModule = &module;
	mApexScene = &scene;
	list.add(*this);		// Add self to module's list of EmitterScenes

	/* Initialize reference to EmitterDebugRenderParams */
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
			memberHandle.initParamRef(EmitterDebugRenderParams::staticClassName(), true);
		}
	}

	/* Load reference to EmitterDebugRenderParams */
	NxParameterized::Interface* refPtr = NULL;
	memberHandle.getParamRef(refPtr);
	mEmitterDebugRenderParams = DYNAMIC_CAST(EmitterDebugRenderParams*)(refPtr);
	PX_ASSERT(mEmitterDebugRenderParams);
}

EmitterScene::~EmitterScene()
{
}

void EmitterScene::visualize()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		EmitterActorBase* actor = DYNAMIC_CAST(EmitterActorBase*)(mActorArray[ i ]);
		actor->visualize(*mDebugRender);
	}
#endif
}

void EmitterScene::destroy()
{
	removeAllActors();
	mApexScene->moduleReleased(*this);
	delete this;
}

#if NX_SDK_VERSION_MAJOR == 2
void EmitterScene::setModulePhysXScene(NxScene* scene)
#elif NX_SDK_VERSION_MAJOR == 3
void EmitterScene::setModulePhysXScene(PxScene* scene)
#endif
{
	if (scene)
	{
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			EmitterActorBase* actor = DYNAMIC_CAST(EmitterActorBase*)(mActorArray[ i ]);
			actor->setPhysXScene(scene);
		}
	}
	else
	{
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			EmitterActorBase* actor = DYNAMIC_CAST(EmitterActorBase*)(mActorArray[ i ]);
			actor->setPhysXScene(NULL);
		}
	}

	mPhysXScene = scene;
}


physx::PxF32	EmitterScene::getBenefit()
{
	if (!mActorArray.size())
	{
		return 0.0f;
	}
	ApexActor** ss = mActorArray.begin();
	ApexActor** ee = mActorArray.end();

	// the address of a EmitterActorBase* and ApexActor* must be identical, otherwise the reinterpret cast will break
	PX_ASSERT(ss == NULL || ((void*)DYNAMIC_CAST(EmitterActorBase*)(*ss) == (void*)(*ss)));

	mSumBenefit = LODCollection<EmitterActorBase>::computeSumBenefit(reinterpret_cast<EmitterActorBase**>(ss), reinterpret_cast<EmitterActorBase**>(ee));
	return mSumBenefit;
}

physx::PxF32	EmitterScene::setResource(physx::PxF32 suggested, physx::PxF32 maxRemaining, physx::PxF32 relativeBenefit)
{
	PX_UNUSED(maxRemaining);

	physx::PxF32 resourceUsed = LODCollection<EmitterActorBase>::distributeResource(reinterpret_cast<EmitterActorBase**>(mActorArray.begin()), reinterpret_cast<EmitterActorBase**>(mActorArray.end()), mSumBenefit, relativeBenefit, suggested);
	return resourceUsed;
}

void EmitterScene::submitTasks(PxF32 /*elapsedTime*/, PxF32 /*substepSize*/, PxU32 /*numSubSteps*/)
{
	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		EmitterActorBase* actor = DYNAMIC_CAST(EmitterActorBase*)(mActorArray[ i ]);
		actor->submitTasks();
	}
}

void EmitterScene::setTaskDependencies()
{
	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		EmitterActorBase* actor = DYNAMIC_CAST(EmitterActorBase*)(mActorArray[ i ]);
		actor->setTaskDependencies();
	}
}

// Called by ApexScene simulation thread after PhysX scene is stepped. All
// actors in the scene are render-locked.
void EmitterScene::fetchResults()
{
	PX_PROFILER_PERF_SCOPE("EmitterSceneFetchResults");

	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		EmitterActorBase* actor = DYNAMIC_CAST(EmitterActorBase*)(mActorArray[ i ]);
		actor->fetchResults();
	}
}

}
}
} // namespace physx::apex
