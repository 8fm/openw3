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
#include "ParticleIosScene.h"
#include "ModuleParticleIos.h"
#include "ParticleIosActor.h"
#include "ParticleIosActorCPU.h"
#include "NiApexScene.h"
#include "NiModuleFieldSampler.h"
#include "ModulePerfScope.h"
#include "PsShare.h"
#include "NiApexRenderDebug.h"


#if defined(APEX_CUDA_SUPPORT)
#include <cuda.h>
#include "ApexCutil.h"
#include "ParticleIosActorGPU.h"
#include "ApexCudaSource.h"
#endif

#define CUDA_OBJ(name) SCENE_CUDA_OBJ(*this, name)

namespace physx
{
namespace apex
{
namespace pxparticleios
{


#pragma warning(push)
#pragma warning(disable:4355)

ParticleIosScene::ParticleIosScene(ModuleParticleIos& _module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list)
	: mPhysXScene(NULL)
	, mModule(&_module)
	, mApexScene(&scene)
	, mRenderDebug(renderDebug)
	, mSumBenefit(0.0f)
	, mFieldSamplerManager(NULL)
	, mInjectorAllocator(this)
{
	list.add(*this);

	/* Initialize reference to ParticleIosDebugRenderParams */
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
			memberHandle.initParamRef(ParticleIosDebugRenderParams::staticClassName(), true);
		}
	}

	/* Load reference to ParticleIosDebugRenderParams */
	NxParameterized::Interface* refPtr = NULL;
	memberHandle.getParamRef(refPtr);
	mParticleIosDebugRenderParams = DYNAMIC_CAST(ParticleIosDebugRenderParams*)(refPtr);
	PX_ASSERT(mParticleIosDebugRenderParams);
}
#pragma warning(pop)

ParticleIosScene::~ParticleIosScene()
{
}

void ParticleIosScene::destroy()
{
	removeAllActors();
	mApexScene->moduleReleased(*this);
	delete this;
}

#if NX_SDK_VERSION_MAJOR == 2
void ParticleIosScene::setModulePhysXScene(NxScene* s)
#elif NX_SDK_VERSION_MAJOR == 3
void ParticleIosScene::setModulePhysXScene(PxScene* s)
#endif
{
	if (mPhysXScene == s)
	{
		return;
	}

	mPhysXScene = s;
	for (physx::PxU32 i = 0; i < mActorArray.size(); ++i)
	{
		ParticleIosActor* actor = DYNAMIC_CAST(ParticleIosActor*)(mActorArray[i]);
		actor->setPhysXScene(mPhysXScene);
	}
}

void ParticleIosScene::visualize()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (!mParticleIosDebugRenderParams->VISUALIZE_PARTICLE_IOS_ACTOR)
	{
		return;
	}

	mRenderDebug->pushRenderState();
	for (PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		ParticleIosActor* testActor = DYNAMIC_CAST(ParticleIosActor*)(mActorArray[ i ]);
		testActor->visualize();
	}
	mRenderDebug->popRenderState();
#endif
}

physx::PxF32	ParticleIosScene::getBenefit()
{
	ApexActor** ss = mActorArray.begin();
	ApexActor** ee = mActorArray.end();

	// the address of a ParticleIosActor* and ApexActor* must be identical, otherwise the reinterpret cast will break
	PX_ASSERT(ss == NULL || ((void*)DYNAMIC_CAST(ParticleIosActor*)(*ss) == (void*)(*ss)));

	mSumBenefit = LODCollection<ParticleIosActor>::computeSumBenefit(reinterpret_cast<ParticleIosActor**>(ss), reinterpret_cast<ParticleIosActor**>(ee));
	return mSumBenefit;
}

physx::PxF32	ParticleIosScene::setResource(physx::PxF32 suggested, physx::PxF32 maxRemaining, physx::PxF32 relativeBenefit)
{
	PX_UNUSED(maxRemaining);

	physx::PxF32 resourceUsed = LODCollection<ParticleIosActor>::distributeResource(reinterpret_cast<ParticleIosActor**>(mActorArray.begin()), reinterpret_cast<ParticleIosActor**>(mActorArray.end()), mSumBenefit, relativeBenefit, suggested);
	return resourceUsed;
}

void ParticleIosScene::submitTasks(PxF32 /*elapsedTime*/, PxF32 /*substepSize*/, PxU32 /*numSubSteps*/)
{
	physx::PxTaskManager* tm	= mApexScene->getTaskManager();
	physx::PxTaskID		taskFinishBeforeID	= tm->getNamedTask(AST_PHYSX_SIMULATE);


	for (PxU32 i = 0; i < mActorArray.size(); ++i)
	{
		ParticleIosActor* actor = DYNAMIC_CAST(ParticleIosActor*)(mActorArray[i]);
		taskFinishBeforeID	= actor->submitTasks(tm, taskFinishBeforeID);
	}
}

void ParticleIosScene::setTaskDependencies()
{
	for (physx::PxU32 i = 0; i < mActorArray.size(); ++i)
	{
		ParticleIosActor* actor = DYNAMIC_CAST(ParticleIosActor*)(mActorArray[i]);
		actor->setTaskDependencies();
	}

	onSimulationStart();
}

void ParticleIosScene::fetchResults()
{
	onSimulationFinish();

	for (physx::PxU32 i = 0; i < mActorArray.size(); ++i)
	{
		ParticleIosActor* actor = DYNAMIC_CAST(ParticleIosActor*)(mActorArray[i]);
		actor->fetchResults();
	}
}

NiFieldSamplerManager* ParticleIosScene::getNiFieldSamplerManager()
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

ParticleIosSceneCPU::ParticleIosSceneCPU(ModuleParticleIos& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list) :
	ParticleIosScene(module, scene, debugRender, list)
{
}

ParticleIosSceneCPU::~ParticleIosSceneCPU()
{
}

ParticleIosActor* ParticleIosSceneCPU::createIosActor(NxResourceList& list, ParticleIosAsset& asset, NxIofxAsset& iofxAsset)
{
	return PX_NEW(ParticleIosActorCPU)(list, asset, *this, iofxAsset);
}

/******************************** GPU Version ********************************/

#if defined(APEX_CUDA_SUPPORT)

ParticleIosSceneGPU::EventCallback::EventCallback() : mIsCalled(false), mEvent(NULL)
{
}
void ParticleIosSceneGPU::EventCallback::init()
{
	if (mEvent == NULL)
	{
		CUT_SAFE_CALL(cuEventCreate((CUevent*)(&mEvent), CU_EVENT_DEFAULT));
	}
}

ParticleIosSceneGPU::EventCallback::~EventCallback()
{
	if (mEvent != NULL)
	{
		CUT_SAFE_CALL(cuEventDestroy((CUevent)mEvent));
	}
}

void ParticleIosSceneGPU::EventCallback::operator()(void* stream)
{
	if (mEvent != NULL)
	{
		CUT_SAFE_CALL(cuEventRecord((CUevent)mEvent, (CUstream)stream));
		mIsCalled = true;
	}
}

ParticleIosSceneGPU::ParticleIosSceneGPU(ModuleParticleIos& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list)
	: ParticleIosScene(module, scene, debugRender, list)
	, CudaModuleScene(scene, *mModule)
	, mInjectorConstMemGroup(APEX_CUDA_OBJ_NAME(simulateConstMem))
{
	{
		physx::PxGpuDispatcher* gd = mApexScene->getTaskManager()->getGpuDispatcher();
		PX_ASSERT(gd != NULL);
		physx::PxScopedCudaLock _lock_(*gd->getCudaContextManager());

		mOnSimulationStart.init();
		mTimer.init();
//CUDA module objects
#include "../cuda/include/moduleList.h"
	}

	{
		mInjectorConstMemGroup.begin();
		mInjectorParamsArrayHandle.alloc(mInjectorConstMemGroup.getStorage());
		//injectorParamsArray.resize( mInjectorConstMemGroup.getStorage(), MAX_INJECTOR_COUNT );
		mInjectorConstMemGroup.end();
	}

}

ParticleIosSceneGPU::~ParticleIosSceneGPU()
{
	for (physx::PxU32 i = 0; i < mOnStartCallbacks.size(); i++)
	{
		PX_DELETE(mOnStartCallbacks[i]);
	}
	for (physx::PxU32 i = 0; i < mOnFinishCallbacks.size(); i++)
	{
		PX_DELETE(mOnFinishCallbacks[i]);
	}
	CudaModuleScene::destroy(*mApexScene);
}

ParticleIosActor* ParticleIosSceneGPU::createIosActor(NxResourceList& list, ParticleIosAsset& asset, NxIofxAsset& iofxAsset)
{
	ParticleIosActorGPU* actor = PX_NEW(ParticleIosActorGPU)(list, asset, *this, iofxAsset);
	mOnStartCallbacks.pushBack(PX_NEW(EventCallback)());
	mOnFinishCallbacks.pushBack(PX_NEW(EventCallback)());
	{
		physx::PxGpuDispatcher* gd = mApexScene->getTaskManager()->getGpuDispatcher();
		PX_ASSERT(gd != NULL);
		physx::PxScopedCudaLock _lock_(*gd->getCudaContextManager());

		mOnStartCallbacks.back()->init();
		mOnFinishCallbacks.back()->init();
	}
	actor->setOnStartFSCallback(mOnStartCallbacks.back());
	actor->setOnFinishIOFXCallback(mOnFinishCallbacks.back());
	return actor;
}

Px3InjectorParams& ParticleIosSceneGPU::getInjectorParams(PxU32 injectorID)
{
	APEX_CUDA_CONST_MEM_GROUP_SCOPE(mInjectorConstMemGroup);

	InjectorParamsArray* injectorParamsArray = mInjectorParamsArrayHandle.resolve(_storage_);
	PX_ASSERT(injectorID < injectorParamsArray->getSize());
	return injectorParamsArray->getElems(_storage_)[ injectorID ];
}

void ParticleIosSceneGPU::fetchResults()
{
	ParticleIosScene::fetchResults();

	physx::apex::ApexStatValue val;	
	val.Float = 0.f;
	PxF32 minTime = 1e30;
	
	for (physx::PxU32 i = 0 ; i < this->mOnStartCallbacks.size(); i++)
	{
		if (mOnStartCallbacks[i]->mIsCalled && mOnFinishCallbacks[i]->mIsCalled)
		{
			mOnStartCallbacks[i]->mIsCalled = false;
			mOnFinishCallbacks[i]->mIsCalled = false;
			CUT_SAFE_CALL(cuEventSynchronize((CUevent)mOnStartCallbacks[i]->getEvent()));
			CUT_SAFE_CALL(cuEventSynchronize((CUevent)mOnFinishCallbacks[i]->getEvent()));
			PxF32 tmp;
			CUT_SAFE_CALL(cuEventElapsedTime(&tmp, (CUevent)mOnSimulationStart.getEvent(), (CUevent)mOnStartCallbacks[i]->getEvent()));
			minTime = physx::PxMin(tmp, minTime);
			CUT_SAFE_CALL(cuEventElapsedTime(&tmp, (CUevent)mOnSimulationStart.getEvent(), (CUevent)mOnFinishCallbacks[i]->getEvent()));
			val.Float = physx::PxMax(tmp, val.Float);
		}
	}
	val.Float -= physx::PxMin(minTime, val.Float);	
	
	if (val.Float > 0.f)
	{
		mApexScene->setApexStatValue(NiApexScene::GpuParticleTime, val);
	}
}

bool ParticleIosSceneGPU::growInjectorStorage(physx::PxU32 newSize)
{
	APEX_CUDA_CONST_MEM_GROUP_SCOPE(mInjectorConstMemGroup);

	InjectorParamsArray* injectorParamsArray = mInjectorParamsArrayHandle.resolve(_storage_);
	return injectorParamsArray->resize(_storage_, newSize);
}


void ParticleIosSceneGPU::onSimulationStart()
{
	ParticleIosScene::onSimulationStart();

	physx::PxGpuDispatcher* gd = mApexScene->getTaskManager()->getGpuDispatcher();
	PX_ASSERT(gd != NULL);
	physx::PxScopedCudaLock _lock_(*gd->getCudaContextManager());

	//we pass default 0 stream so that this copy happens before any kernel launches
	CUDA_OBJ(simulateConstMem).copyToDevice(0);

	mOnSimulationStart(NULL);
}

#endif

// ParticleIosInjectorAllocator
physx::PxU32 ParticleIosInjectorAllocator::allocateInjectorID()
{
	if (mFreeInjectorListStart == NULL_INJECTOR_INDEX)
	{
		//try to get new injectors
		physx::PxU32 size = mInjectorList.size();
		if (mStorage->growInjectorStorage(size + 1) == false)
		{
			return NULL_INJECTOR_INDEX;
		}

		mFreeInjectorListStart = size;
		mInjectorList.resize(size + 1);
		mInjectorList.back() = NULL_INJECTOR_INDEX;
	}
	physx::PxU32 injectorID = mFreeInjectorListStart;
	mFreeInjectorListStart = mInjectorList[injectorID];
	mInjectorList[injectorID] = USED_INJECTOR_INDEX;
	return injectorID;
}

void ParticleIosInjectorAllocator::releaseInjectorID(physx::PxU32 injectorID)
{
	//add to released injector list
	PX_ASSERT(mInjectorList[injectorID] == USED_INJECTOR_INDEX);
	mInjectorList[injectorID] = mReleasedInjectorListStart;
	mReleasedInjectorListStart = injectorID;
}

void ParticleIosInjectorAllocator::flushReleased()
{
	//add all released injectors to free injector list
	while (mReleasedInjectorListStart != NULL_INJECTOR_INDEX)
	{
		physx::PxU32 injectorID = mInjectorList[mReleasedInjectorListStart];

		//add to free injector list
		mInjectorList[mReleasedInjectorListStart] = mFreeInjectorListStart;
		mFreeInjectorListStart = mReleasedInjectorListStart;

		mReleasedInjectorListStart = injectorID;
	}
}

}
}
} // namespace physx::apex

#endif // NX_SDK_VERSION_MAJOR == 3