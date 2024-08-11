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

#ifndef __BASIC_IOS_SCENE_H__
#define __BASIC_IOS_SCENE_H__

#include "NxApex.h"
#include "NxModuleBasicIos.h"
#include "NiApexSDK.h"
#include "NiModule.h"
#include "ModuleBasicIos.h"
#include "ApexSharedUtils.h"
#include "ApexSDKHelpers.h"
#include "ApexContext.h"
#include "ApexActor.h"
#include "ModulePerfScope.h"

#include "DebugRenderParams.h"
#include "BasicIosDebugRenderParams.h"

#include "BasicIosCommon.h"
#include "BasicIosCommonSrc.h"

#include "NiFieldSamplerQuery.h"

#if defined(APEX_CUDA_SUPPORT)
#include "../cuda/include/common.h"

#include "ApexCudaWrapper.h"
#include "CudaModuleScene.h"

#define SCENE_CUDA_OBJ(scene, name) static_cast<BasicIosSceneGPU&>(scene).APEX_CUDA_OBJ_NAME(name)

#endif

namespace physx
{
namespace apex
{

class NiApexRenderDebug;
class NiFieldSamplerManager;

namespace basicios
{

class BasicIosInjectorStorage
{
public:
	virtual bool growInjectorStorage(physx::PxU32 newSize) = 0;
};
class BasicIosInjectorAllocator
{
public:
	BasicIosInjectorAllocator(BasicIosInjectorStorage* storage) : mStorage(storage)
	{
		mFreeInjectorListStart = NULL_INJECTOR_INDEX;
		mReleasedInjectorListStart = NULL_INJECTOR_INDEX;
	}

	physx::PxU32				allocateInjectorID();
	void						releaseInjectorID(physx::PxU32);
	void						flushReleased();

	static const PxU32			NULL_INJECTOR_INDEX = 0xFFFFFFFFu;
	static const PxU32			USED_INJECTOR_INDEX = 0xFFFFFFFEu;

private:
	BasicIosInjectorStorage*	mStorage;

	physx::Array<PxU32>			mInjectorList;
	PxU32						mFreeInjectorListStart;
	PxU32						mReleasedInjectorListStart;
};


class BasicIosScene : public NiModuleScene, public ApexContext, public NxApexResource, public ApexResource, protected BasicIosInjectorStorage
{
public:
	BasicIosScene(ModuleBasicIos& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list);
	~BasicIosScene();

	/* NiModuleScene */
	void									release()
	{
		mModule->releaseNiModuleScene(*this);
	}

#if NX_SDK_VERSION_MAJOR == 2
	NxScene*                                getModulePhysXScene() const
	{
		return mPhysXScene;
	}
	void                                    setModulePhysXScene(NxScene*);
	NxScene* 								mPhysXScene;
#elif NX_SDK_VERSION_MAJOR == 3
	PxScene*                                getModulePhysXScene() const
	{
		return mPhysXScene;
	}
	void                                    setModulePhysXScene(PxScene*);
	PxScene* 								mPhysXScene;
#endif

	void									visualize();

	virtual NxModule*						getNxModule()
	{
		return mModule;
	}

	virtual NxApexSceneStats* getStats()
	{
		return 0;
	}

	physx::PxF32							getBenefit();
	physx::PxF32							setResource(physx::PxF32 , physx::PxF32, physx::PxF32);

	/* NxApexResource */
	physx::PxU32							getListIndex() const
	{
		return m_listIndex;
	}
	void                                    setListIndex(NxResourceList& list, physx::PxU32 index)
	{
		m_listIndex = index;
		m_list = &list;
	}

	virtual BasicIosActor*					createIosActor(NxResourceList& list, BasicIosAsset& asset, physx::apex::NxIofxAsset& iofxAsset) = 0;

	virtual void							submitTasks(PxF32 elapsedTime, PxF32 substepSize, PxU32 numSubSteps);
	virtual void							setTaskDependencies();
	virtual void							fetchResults();

	NiFieldSamplerManager* 					getNiFieldSamplerManager();

	NiApexScene&							getApexScene() const
	{
		return *mApexScene;
	}

	PX_INLINE BasicIosInjectorAllocator&	getInjectorAllocator()
	{
		return mInjectorAllocator;
	}
	virtual InjectorParams&					getInjectorParams(PxU32 injectorID) = 0;
#ifdef APEX_TEST
	NxBasicIosActor* 						getApexActor(PxU32 index) const;
#endif

protected:
	virtual void onSimulationStart() {}
	virtual void onSimulationFinish()
	{
		mInjectorAllocator.flushReleased();
	}


	ModuleBasicIos* 						mModule;
	NiApexScene* 							mApexScene;

	void									destroy();

	physx::PxF32							computeAABBDistanceSquared(const physx::PxBounds3& aabb);

	NiApexRenderDebug* 						mDebugRender;
	physx::PxF32							mSumBenefit;

	DebugRenderParams*						mDebugRenderParams;
	BasicIosDebugRenderParams*				mBasicIosDebugRenderParams;

	NiFieldSamplerManager* 					mFieldSamplerManager;

	BasicIosInjectorAllocator				mInjectorAllocator;

	friend class BasicIosActor;
	friend class BasicIosAsset;
	friend class ModuleBasicIos;
};

class BasicIosSceneCPU : public BasicIosScene
{
public:
	BasicIosSceneCPU(ModuleBasicIos& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list);
	~BasicIosSceneCPU();

	virtual BasicIosActor*		createIosActor(NxResourceList& list, BasicIosAsset& asset, physx::apex::NxIofxAsset& iofxAsset);

	virtual InjectorParams&		getInjectorParams(PxU32 injectorID)
	{
		PX_ASSERT(injectorID < mInjectorParamsArray.size());
		return mInjectorParamsArray[ injectorID ];
	}

protected:
	virtual bool growInjectorStorage(physx::PxU32 newSize)
	{
		mInjectorParamsArray.resize(newSize);
		return true;
	}

private:
	physx::Array<InjectorParams> mInjectorParamsArray;

	friend class BasicIosActorCPU;
};

#if defined(APEX_CUDA_SUPPORT)
class BasicIosSceneGPU : public BasicIosScene, public CudaModuleScene
{
	class EventCallback : public NiFieldSamplerCallback, public NiIofxManagerCallback, public physx::UserAllocated
	{
		void* mEvent;
	public:
		EventCallback();
		void init();
		virtual ~EventCallback();
		void operator()(void* stream);
		PX_INLINE void* getEvent()
		{
			return mEvent;
		}
		bool mIsCalled;
	};
public:
	BasicIosSceneGPU(ModuleBasicIos& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list);
	~BasicIosSceneGPU();

	virtual BasicIosActor*		createIosActor(NxResourceList& list, BasicIosAsset& asset, physx::apex::NxIofxAsset& iofxAsset);

	virtual InjectorParams&		getInjectorParams(PxU32 injectorID);
	virtual void				fetchResults();

//CUDA module objects
#include "../cuda/include/moduleList.h"

protected:
	virtual bool growInjectorStorage(physx::PxU32 newSize);

	void onSimulationStart();

private:
	ApexCudaConstMemGroup				mInjectorConstMemGroup;
	InplaceHandle<InjectorParamsArray>	mInjectorParamsArrayHandle;

	EventCallback						mOnSimulationStart;
	physx::Array<EventCallback*>		mOnStartCallbacks;
	physx::Array<EventCallback*>		mOnFinishCallbacks;
	ApexCudaTimer						mTimer;

	friend class BasicIosActorGPU;
};
#endif

}
}
} // namespace physx::apex

#endif // __BASIC_IOS_SCENE_H__
