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
#include "BasicIosScene.h"
#include "ModuleBasicIos.h"
#include "BasicIosActor.h"
#include "BasicIosActorCPU.h"
#include "PsShare.h"
#include "NiApexScene.h"
#include "ModulePerfScope.h"
#include "NxApexReadWriteLock.h"
#include "NiModuleFieldSampler.h"
#include "NiApexRenderDebug.h"

#if defined(APEX_CUDA_SUPPORT)
#include <cuda.h>
#include "ApexCutil.h"
#include "BasicIosActorGPU.h"

#include "ApexCudaSource.h"
#endif

#if NX_SDK_VERSION_MAJOR == 2
#include <NxScene.h>
#elif NX_SDK_VERSION_MAJOR == 3
#include <PxScene.h>
#endif

#define CUDA_OBJ(name) SCENE_CUDA_OBJ(*this, name)

namespace physx
{
namespace apex
{
namespace basicios
{

#pragma warning(push)
#pragma warning(disable:4355)

BasicIosScene::BasicIosScene(ModuleBasicIos& _module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list)
	: mPhysXScene(NULL)
	, mModule(&_module)
	, mApexScene(&scene)
	, mDebugRender(debugRender)
	, mSumBenefit(0.0f)
	, mFieldSamplerManager(NULL)
	, mInjectorAllocator(this)
{
	list.add(*this);

	/* Initialize reference to BasicIosDebugRenderParams */
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
			memberHandle.initParamRef(BasicIosDebugRenderParams::staticClassName(), true);
		}
	}

	/* Load reference to BasicIosDebugRenderParams */
	NxParameterized::Interface* refPtr = NULL;
	memberHandle.getParamRef(refPtr);
	mBasicIosDebugRenderParams = DYNAMIC_CAST(BasicIosDebugRenderParams*)(refPtr);
	PX_ASSERT(mBasicIosDebugRenderParams);
}

#pragma warning(pop)

BasicIosScene::~BasicIosScene()
{
}

void BasicIosScene::destroy()
{
	removeAllActors();
	mApexScene->moduleReleased(*this);
	delete this;
}

#if NX_SDK_VERSION_MAJOR == 2
void BasicIosScene::setModulePhysXScene(NxScene* s)
#elif NX_SDK_VERSION_MAJOR == 3
void BasicIosScene::setModulePhysXScene(PxScene* s)
#endif
{
	if (mPhysXScene == s)
	{
		return;
	}

	mPhysXScene = s;
	for (physx::PxU32 i = 0; i < mActorArray.size(); ++i)
	{
		BasicIosActor* actor = DYNAMIC_CAST(BasicIosActor*)(mActorArray[i]);
		actor->setPhysXScene(mPhysXScene);
	}
}

void BasicIosScene::visualize()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (!mBasicIosDebugRenderParams->VISUALIZE_BASIC_IOS_ACTOR)
	{
		return;
	}

	mDebugRender->pushRenderState();
	for (PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		BasicIosActor* testActor = DYNAMIC_CAST(BasicIosActor*)(mActorArray[ i ]);
		testActor->visualize();
	}
	mDebugRender->popRenderState();
#endif
}

physx::PxF32	BasicIosScene::getBenefit()
{
	ApexActor** ss = mActorArray.begin();
	ApexActor** ee = mActorArray.end();

	// the address of a BasicIosActor* and ApexActor* must be identical, otherwise the reinterpret cast will break
	PX_ASSERT(ss == NULL || ((void*)DYNAMIC_CAST(BasicIosActor*)(*ss) == (void*)(*ss)));

	mSumBenefit = LODCollection<BasicIosActor>::computeSumBenefit(reinterpret_cast<BasicIosActor**>(ss), reinterpret_cast<BasicIosActor**>(ee));
	return mSumBenefit;
}

physx::PxF32	BasicIosScene::setResource(physx::PxF32 suggested, physx::PxF32 maxRemaining, physx::PxF32 relativeBenefit)
{
	PX_UNUSED(maxRemaining);

	return LODCollection<BasicIosActor>::distributeResource(reinterpret_cast<BasicIosActor**>(mActorArray.begin()), reinterpret_cast<BasicIosActor**>(mActorArray.end()), mSumBenefit, relativeBenefit, suggested);
}

void BasicIosScene::submitTasks(PxF32 /*elapsedTime*/, PxF32 /*substepSize*/, PxU32 /*numSubSteps*/)
{
	for (PxU32 i = 0; i < mActorArray.size(); ++i)
	{
		BasicIosActor* actor = DYNAMIC_CAST(BasicIosActor*)(mActorArray[i]);
		physx::PxVec3 gravity;
		if (mPhysXScene)
		{
			physx::PxVec3 gravity;
#if NX_SDK_VERSION_MAJOR == 2
			NxVec3 nxGravity;
			mPhysXScene->getGravity(nxGravity);
			gravity = PxFromNxVec3Fast(nxGravity);
#elif NX_SDK_VERSION_MAJOR == 3
			SCOPED_PHYSX3_LOCK_READ(mPhysXScene);
			gravity = mPhysXScene->getGravity();
#endif

			if (actor->getGravity() != gravity)
			{
				actor->setGravity(gravity);
			}
		}
		actor->submitTasks();
	}
}

void BasicIosScene::setTaskDependencies()
{
	for (physx::PxU32 i = 0; i < mActorArray.size(); ++i)
	{
		BasicIosActor* actor = DYNAMIC_CAST(BasicIosActor*)(mActorArray[i]);
		actor->setTaskDependencies();
	}

	onSimulationStart();
}

void BasicIosScene::fetchResults()
{
	onSimulationFinish();

	for (physx::PxU32 i = 0; i < mActorArray.size(); ++i)
	{
		BasicIosActor* actor = DYNAMIC_CAST(BasicIosActor*)(mActorArray[i]);
		actor->fetchResults();
	}
}

NiFieldSamplerManager* BasicIosScene::getNiFieldSamplerManager()
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

BasicIosSceneCPU::BasicIosSceneCPU(ModuleBasicIos& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list) :
	BasicIosScene(module, scene, debugRender, list)
{
}

BasicIosSceneCPU::~BasicIosSceneCPU()
{
}

BasicIosActor* BasicIosSceneCPU::createIosActor(NxResourceList& list, BasicIosAsset& asset, physx::apex::NxIofxAsset& iofxAsset)
{
	return PX_NEW(BasicIosActorCPU)(list, asset, *this, iofxAsset);
}

/******************************** GPU Version ********************************/

#if defined(APEX_CUDA_SUPPORT)

BasicIosSceneGPU::EventCallback::EventCallback() : mIsCalled(false), mEvent(NULL)
{
}
void BasicIosSceneGPU::EventCallback::init()
{
	if (mEvent == NULL)
	{
		CUT_SAFE_CALL(cuEventCreate((CUevent*)(&mEvent), CU_EVENT_DEFAULT));
	}
}

BasicIosSceneGPU::EventCallback::~EventCallback()
{
	if (mEvent != NULL)
	{
		CUT_SAFE_CALL(cuEventDestroy((CUevent)mEvent));
	}
}

void BasicIosSceneGPU::EventCallback::operator()(void* stream)
{
	if (mEvent != NULL)
	{
		CUT_SAFE_CALL(cuEventRecord((CUevent)mEvent, (CUstream)stream));
		mIsCalled = true;
	}
}

BasicIosSceneGPU::BasicIosSceneGPU(ModuleBasicIos& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list)
	: BasicIosScene(module, scene, debugRender, list)
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
		//injectorParamsArray.resize( mInjectorConstMemGroup.getStorage(), HISTOGRAM_SIMULATE_BIN_COUNT );

		mInjectorConstMemGroup.end();
	}

}

BasicIosSceneGPU::~BasicIosSceneGPU()
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

BasicIosActor* BasicIosSceneGPU::createIosActor(NxResourceList& list, BasicIosAsset& asset, physx::apex::NxIofxAsset& iofxAsset)
{
	BasicIosActorGPU* actor = PX_NEW(BasicIosActorGPU)(list, asset, *this, iofxAsset);
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

InjectorParams& BasicIosSceneGPU::getInjectorParams(PxU32 injectorID)
{
	APEX_CUDA_CONST_MEM_GROUP_SCOPE(mInjectorConstMemGroup);

	InjectorParamsArray* injectorParamsArray = mInjectorParamsArrayHandle.resolve(_storage_);
	PX_ASSERT(injectorID < injectorParamsArray->getSize());
	return injectorParamsArray->getElems(_storage_)[ injectorID ];
}

bool BasicIosSceneGPU::growInjectorStorage(physx::PxU32 newSize)
{
	APEX_CUDA_CONST_MEM_GROUP_SCOPE(mInjectorConstMemGroup);

	if (mApexScene->isSimulating())
	{
		APEX_INTERNAL_ERROR("BasicIosSceneGPU::growInjectorStorage - is called while ApexScene in simulating!");
		PX_ASSERT(0);
	}

	InjectorParamsArray* injectorParamsArray = mInjectorParamsArrayHandle.resolve(_storage_);
	return injectorParamsArray->resize(_storage_, newSize);
}


void BasicIosSceneGPU::onSimulationStart()
{
	BasicIosScene::onSimulationStart();

	physx::PxGpuDispatcher* gd = mApexScene->getTaskManager()->getGpuDispatcher();
	PX_ASSERT(gd != NULL);
	physx::PxScopedCudaLock _lock_(*gd->getCudaContextManager());

	//we pass default 0 stream so that this copy happens before any kernel launches
	CUDA_OBJ(simulateConstMem).copyToDevice(0);

	mOnSimulationStart(NULL);
}

void BasicIosSceneGPU::fetchResults()
{
	BasicIosScene::fetchResults();

	physx::PxGpuDispatcher* gd = mApexScene->getTaskManager()->getGpuDispatcher();
	PX_ASSERT(gd != NULL);
	physx::PxScopedCudaLock _lock_(*gd->getCudaContextManager());

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
#endif

// BasicIosInjectorAllocator
physx::PxU32 BasicIosInjectorAllocator::allocateInjectorID()
{
	physx::PxU32 size = mInjectorList.size();
	if (mFreeInjectorListStart == NULL_INJECTOR_INDEX)
	{
		//try to get new injectors
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

void BasicIosInjectorAllocator::releaseInjectorID(physx::PxU32 injectorID)
{
	//add to released injector list
	PX_ASSERT(mInjectorList[injectorID] == USED_INJECTOR_INDEX);
	mInjectorList[injectorID] = mReleasedInjectorListStart;
	mReleasedInjectorListStart = injectorID;
}

void BasicIosInjectorAllocator::flushReleased()
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

#ifdef APEX_TEST
NxBasicIosActor* BasicIosScene::getApexActor(PxU32 index) const
{
	if (index < mActorArray.size())
	{
		BasicIosActor* actor = DYNAMIC_CAST(BasicIosActor*)(mActorArray[index]);
		return static_cast<NxBasicIosActor*>(actor);
	}
	return NULL;
}
#endif

}
}
} // namespace physx::apex
