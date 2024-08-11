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

#ifndef __IOFX_SCENE_H__
#define __IOFX_SCENE_H__

#include "NxApex.h"

#include "NiApexSDK.h"
#include "NiModule.h"
#include "ApexInterface.h"
#include "ApexContext.h"
#include "ApexSDKHelpers.h"
#include "ApexRenderVolume.h"

#include "PxGpuCopyDescQueue.h"

#if defined(APEX_CUDA_SUPPORT)
#include "ApexCudaWrapper.h"
#include "ApexCuda.h"
#include "CudaModuleScene.h"

#include "../cuda/include/common.h"

#define SCENE_CUDA_OBJ(scene, name) static_cast<IofxSceneGPU&>(scene).APEX_CUDA_OBJ_NAME(name)

#endif


namespace physx
{
namespace apex
{

class NiApexScene;
class DebugRenderParams;

namespace iofx
{

class ModuleIofx;
class IofxDebugRenderParams;
class IofxManager;

class IofxScene : public NiModuleScene, public ApexContext, public NxApexResource, public ApexResource
{
public:
	enum StatsDataEnum
	{
		SimulatedSpriteParticlesCount,
		SimulatedMeshParticlesCount,
		// insert new items before this line
		NumberOfStats			// The number of stats
	};
public:
	IofxScene(ModuleIofx& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list);
	~IofxScene();

	/* NiModuleScene */
	void				visualize();
#if NX_SDK_VERSION_MAJOR == 2
	void				setModulePhysXScene(NxScene* s);
	NxScene*			getModulePhysXScene() const
	{
		return mPhysXScene;
	}
	NxScene*			mPhysXScene;
#elif NX_SDK_VERSION_MAJOR == 3
	void				setModulePhysXScene(PxScene* s);
	PxScene*			getModulePhysXScene() const
	{
		return mPhysXScene;
	}
	PxScene*			mPhysXScene;
#endif
	physx::PxF32		setResource(physx::PxF32, physx::PxF32, physx::PxF32)
	{
		return 0.0f;
	}
	physx::PxF32		getBenefit()
	{
		return 0.0f;
	}
	virtual NxModule*	getNxModule();

	virtual NxApexSceneStats* getStats()
	{
		return &mModuleSceneStats;
	}

	/* NxApexResource */
	physx::PxU32		getListIndex() const
	{
		return m_listIndex;
	}
	void				setListIndex(NxResourceList& list, physx::PxU32 index)
	{
		m_listIndex = index;
		m_list = &list;
	}
	void				release();

	IofxManager*		createIofxManager(const NxIofxAsset& asset, const NiIofxManagerDesc& desc);
	void				releaseIofxManager(IofxManager* manager);

	virtual bool		copyDirtySceneData(PxGpuCopyDescQueue& queue) = 0;
	void				submitTasks(PxF32 elapsedTime, PxF32 substepSize, PxU32 numSubSteps);
	void				fetchResults();

	void				fetchResultsPreRenderLock()
	{
		lockLiveRenderVolumes();
	}
	void				fetchResultsPostRenderUnlock()
	{
		unlockLiveRenderVolumes();
	}

	void				prepareRenderResources();

	PX_INLINE void		lockLiveRenderVolumes();
	PX_INLINE void		unlockLiveRenderVolumes();

	void				createModuleStats(void);
	void				destroyModuleStats(void);
	void				setStatValue(StatsDataEnum index, ApexStatValue dataVal);

	ModuleIofx*		    mModule;
	NiApexScene*		mApexScene;
	NiApexRenderDebug*	mDebugRender;

	physx::Mutex		mFetchResultsLock;
	physx::ReadWriteLock mManagersLock;

	physx::ReadWriteLock mLiveRenderVolumesLock;
	physx::Mutex		mAddedRenderVolumesLock;
	physx::Mutex		mDeletedRenderVolumesLock;
	
	NxResourceList		mActorManagers;

	physx::Array<ApexRenderVolume*> mLiveRenderVolumes;
	physx::Array<ApexRenderVolume*> mAddedRenderVolumes;
	physx::Array<ApexRenderVolume*> mDeletedRenderVolumes;

	DebugRenderParams*				mDebugRenderParams;
	IofxDebugRenderParams*			mIofxDebugRenderParams;

	NxApexSceneStats	mModuleSceneStats;	

	physx::PxU32		mPrevTotalSimulatedSpriteParticles;
	physx::PxU32		mPrevTotalSimulatedMeshParticles;

	void                destroy();

	void				processDeferredRenderVolumes();
};

class IofxSceneCPU : public IofxScene
{
public:
	IofxSceneCPU(ModuleIofx& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list);
	bool		copyDirtySceneData(PxGpuCopyDescQueue&)
	{
		return false;
	}
};

#if defined(APEX_CUDA_SUPPORT)
class IofxSceneGPU : public IofxScene, public CudaModuleScene
{
public:
	IofxSceneGPU(ModuleIofx& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list);
	~IofxSceneGPU();

	void				submitTasks(PxF32 elapsedTime, PxF32 substepSize, PxU32 numSubSteps);
	bool				copyDirtySceneData(PxGpuCopyDescQueue& queue);

	void				prepareRenderResources();

//CUDA module objects
#include "../cuda/include/moduleList.h"

protected:
	/* device and host pinned buffers, etc */
	physx::PxCudaContextManager*	mContextManager;

	physx::Array<CUgraphicsResource>	mToMapArray, mToUnmapArray;
};
#endif

}
}
} // namespace physx::apex

#endif
