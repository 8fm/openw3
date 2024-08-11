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
#include "ModuleIofx.h"
#include "IofxScene.h"
#include "IofxAsset.h"
#include "IofxActor.h"
#include "IofxActorCPU.h"
#include "IofxDebugRenderParams.h"
#include "DebugRenderParams.h"
#include "ApexRenderVolume.h"
#include "NiApexScene.h"
#include "ModulePerfScope.h"
#include "NiApexRenderDebug.h"

#if defined(APEX_CUDA_SUPPORT)
#include <cuda.h>
#include "ApexCutil.h"
#include "IofxActorGPU.h"

#include "ApexCudaSource.h"
#endif

namespace physx
{
namespace apex
{
namespace iofx
{

static ApexStatsInfo IOFXStatsData[] =
{
	{"IOFX: SimulatedSpriteParticlesCount",	ApexStatDataType::INT,   {{0}} },
	{"IOFX: SimulatedMeshParticlesCount",	ApexStatDataType::INT,   {{0}} }
};

IofxScene::IofxScene(ModuleIofx& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list)
	: mModule(&module)
	, mApexScene(&scene)
	, mDebugRender(debugRender)
	, mPrevTotalSimulatedSpriteParticles(0)
	, mPrevTotalSimulatedMeshParticles(0)
{
	list.add(*this);		// Add self to module's list of IofxScenes

	/* Initialize reference to IofxDebugRenderParams */
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
			memberHandle.initParamRef(IofxDebugRenderParams::staticClassName(), true);
		}
	}

	/* Load reference to IofxDebugRenderParams */
	NxParameterized::Interface* refPtr = NULL;
	memberHandle.getParamRef(refPtr);
	mIofxDebugRenderParams = DYNAMIC_CAST(IofxDebugRenderParams*)(refPtr);
	PX_ASSERT(mIofxDebugRenderParams);

	createModuleStats();
}

IofxScene::~IofxScene()
{
	destroyModuleStats();
}

NxModule* IofxScene::getNxModule()
{
	return mModule;
}

void IofxScene::release()
{
	mModule->releaseNiModuleScene(*this);
}

IofxManager* IofxScene::createIofxManager(const NxIofxAsset& asset, const NiIofxManagerDesc& desc)
{
	mManagersLock.lockWriter();

	IofxManager* iofxManager = PX_NEW(IofxManager)(*this, desc, asset.getMeshAssetCount() > 0);

	mManagersLock.unlockWriter();

	return iofxManager;
}

void IofxScene::releaseIofxManager(IofxManager* manager)
{
	mManagersLock.lockWriter();

	manager->destroy();

	mManagersLock.unlockWriter();
}

void IofxScene::createModuleStats(void)
{
	mModuleSceneStats.numApexStats		= NumberOfStats;
	mModuleSceneStats.ApexStatsInfoPtr	= (ApexStatsInfo*)PX_ALLOC(sizeof(ApexStatsInfo) * NumberOfStats, PX_DEBUG_EXP("ApexStatsInfo"));

	for (physx::PxU32 i = 0; i < NumberOfStats; i++)
	{
		mModuleSceneStats.ApexStatsInfoPtr[i] = IOFXStatsData[i];
	}
}

void IofxScene::destroyModuleStats(void)
{
	mModuleSceneStats.numApexStats = 0;
	if (mModuleSceneStats.ApexStatsInfoPtr)
	{
		PX_FREE_AND_RESET(mModuleSceneStats.ApexStatsInfoPtr);
	}
}

void IofxScene::setStatValue(StatsDataEnum index, ApexStatValue dataVal)
{
	if (mModuleSceneStats.ApexStatsInfoPtr)
	{
		mModuleSceneStats.ApexStatsInfoPtr[index].StatCurrentValue = dataVal;
	}
}

void IofxScene::visualize()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (!mIofxDebugRenderParams->VISUALIZE_IOFX_ACTOR)
	{
		return;
	}

	PxMat44 cameraFacingPose((mApexScene->getViewMatrix(0)).inverseRT());

	
	mDebugRender->pushRenderState();
	mDebugRender->setCurrentTextScale(3.0f);

	// iofx current bounding volume
	// there is a bug with setting the color. it can change under certain circumstances. use default color for now
	//mDebugRender->setCurrentColor(mDebugRender->getDebugColor(DebugColors::LightBlue));
	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		IofxActor* actor = DYNAMIC_CAST(IofxActor*)(mActorArray[ i ]);
		mDebugRender->setCurrentUserPointer((void*)(NxApexActor*)(mActorArray[i]));
		if (mIofxDebugRenderParams->VISUALIZE_IOFX_ACTOR_NAME)
		{
			PxVec3 textLocation = actor->getBounds().maximum;
			cameraFacingPose.setPosition(textLocation);
			if(actor->getOwner() != NULL)
			{
				mDebugRender->debugOrientedText(cameraFacingPose, " %s %s", actor->getOwner()->getObjTypeName(), actor->getOwner()->getName());
			}
		}
		if (mIofxDebugRenderParams->VISUALIZE_IOFX_BOUNDING_BOX)
		{
			mDebugRender->debugBound(actor->getBounds());
		}

		mDebugRender->setCurrentUserPointer(NULL);
	}

	// iofx max bounding volume
	// there is a bug with setting the color. it can change under certain circumstances. use default color for now
	//mDebugRender->setCurrentColor(mDebugRender->getDebugColor(DebugColors::Orange));
	mLiveRenderVolumesLock.lockReader();
	for (physx::PxU32 i = 0 ; i < mLiveRenderVolumes.size() ; i++)
	{
		if (mIofxDebugRenderParams->VISUALIZE_IOFX_BOUNDING_BOX)
		{
			PxVec3 textLocation = mLiveRenderVolumes[i]->getOwnershipBounds().maximum;
			cameraFacingPose.setPosition(textLocation);
			mDebugRender->debugOrientedText(cameraFacingPose, " Max Render Volume %d", i);
			mDebugRender->debugBound(mLiveRenderVolumes[i]->getOwnershipBounds());
		}
	}
	mLiveRenderVolumesLock.unlockReader();

	mDebugRender->popRenderState();
#endif
}

void IofxScene::destroy()
{
	removeAllActors();
	mApexScene->moduleReleased(*this);

	{
		mLiveRenderVolumesLock.lockWriter();

		/* Handle deferred insertions/deletions of ApexRenderVolumes */
		processDeferredRenderVolumes();

		/* Delete all Live ApexRenderVolumes */
		for (PxU32 i = 0 ; i < mLiveRenderVolumes.size() ; i++)
		{
			ApexRenderVolume* arv = mLiveRenderVolumes[ i ];
			PX_DELETE(arv);
		}
		mLiveRenderVolumes.clear();

		mLiveRenderVolumesLock.unlockWriter();
	}
	delete this;
}

#if NX_SDK_VERSION_MAJOR == 2
void IofxScene::setModulePhysXScene(NxScene* nxScene)
#elif NX_SDK_VERSION_MAJOR == 3
void IofxScene::setModulePhysXScene(PxScene* nxScene)
#endif
{
	if (nxScene)
	{
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			IofxActor* actor = DYNAMIC_CAST(IofxActor*)(mActorArray[ i ]);
			actor->setPhysXScene(nxScene);
		}
	}
	else
	{
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			IofxActor* actor = DYNAMIC_CAST(IofxActor*)(mActorArray[ i ]);
			actor->setPhysXScene(NULL);
		}
	}

	mPhysXScene = nxScene;
}

void IofxScene::processDeferredRenderVolumes()
{
	mAddedRenderVolumesLock.lock();
	while (mAddedRenderVolumes.size())
	{
		ApexRenderVolume* arv = mAddedRenderVolumes.popBack();
		mLiveRenderVolumes.pushBack(arv);
	}
	mAddedRenderVolumesLock.unlock();

	mDeletedRenderVolumesLock.lock();
	while (mDeletedRenderVolumes.size())
	{
		ApexRenderVolume* arv = mDeletedRenderVolumes.popBack();
		mLiveRenderVolumes.findAndReplaceWithLast(arv);
		PX_DELETE(arv);
	}
	mDeletedRenderVolumesLock.unlock();
}

void IofxScene::submitTasks(PxF32 /*elapsedTime*/, PxF32 /*substepSize*/, PxU32 /*numSubSteps*/)
{
	{
		mLiveRenderVolumesLock.lockWriter();

		/* Handle deferred insertions/deletions of ApexRenderVolumes */
		processDeferredRenderVolumes();

		mLiveRenderVolumesLock.unlockWriter();
	}

	mManagersLock.lockReader();

	//IofxManager::submitTasks reads mLiveRenderVolumes so we lock it here and unlock later
	mLiveRenderVolumesLock.lockReader();
	for (physx::PxU32 i = 0; i < mActorManagers.getSize(); ++i)
	{
		IofxManager* mgr = DYNAMIC_CAST(IofxManager*)(mActorManagers.getResource(i));
		mgr->submitTasks();
	}
	mLiveRenderVolumesLock.unlockReader();

	mManagersLock.unlockReader();
}

void IofxScene::fetchResults()
{
	mManagersLock.lockReader();

	mFetchResultsLock.lock();

	physx::PxU32 totalSimulatedSpriteParticles = 0,
				totalSimulatedMeshParticles = 0;
	for (physx::PxU32 i = 0; i < mActorManagers.getSize(); ++i)
	{
		IofxManager* mgr = DYNAMIC_CAST(IofxManager*)(mActorManagers.getResource(i));
		mgr->fetchResults();

		if(mgr->isMesh())
			totalSimulatedMeshParticles += mgr->getSimulatedParticlesCount();
		else
			totalSimulatedSpriteParticles += mgr->getSimulatedParticlesCount();
	}

	ApexStatValue dataVal;
	dataVal.Int = totalSimulatedSpriteParticles - mPrevTotalSimulatedSpriteParticles;
	setStatValue(SimulatedSpriteParticlesCount, dataVal);
	mPrevTotalSimulatedSpriteParticles = totalSimulatedSpriteParticles;
	dataVal.Int = totalSimulatedMeshParticles - mPrevTotalSimulatedMeshParticles;
	setStatValue(SimulatedMeshParticlesCount, dataVal);
	mPrevTotalSimulatedMeshParticles = totalSimulatedMeshParticles;

	mFetchResultsLock.unlock();

	mManagersLock.unlockReader();
}

void IofxScene::prepareRenderResources()
{
	mManagersLock.lockReader();

	lockLiveRenderVolumes();
	mFetchResultsLock.lock();

	for (physx::PxU32 i = 0; i < mActorManagers.getSize(); ++i)
	{
		IofxManager* mgr = DYNAMIC_CAST(IofxManager*)(mActorManagers.getResource(i));
		mgr->prepareRenderResources();
	}

	mFetchResultsLock.unlock();
	unlockLiveRenderVolumes();

	for (physx::PxU32 i = 0; i < mActorManagers.getSize(); ++i)
	{
		IofxManager* mgr = DYNAMIC_CAST(IofxManager*)(mActorManagers.getResource(i));
		mgr->postPrepareRenderResources();
	}

	mManagersLock.unlockReader();
}


void IofxScene::lockLiveRenderVolumes()
{
	mLiveRenderVolumesLock.lockReader();
	for (PxU32 i = 0 ; i < mLiveRenderVolumes.size() ; i++)
	{
		mLiveRenderVolumes[ i ]->lockRenderResources();
	}
}

void IofxScene::unlockLiveRenderVolumes()
{
	for (PxU32 i = 0 ; i < mLiveRenderVolumes.size() ; i++)
	{
		mLiveRenderVolumes[ i ]->unlockRenderResources();
	}
	mLiveRenderVolumesLock.unlockReader();
}

/******************************** CPU Version ********************************/

IofxSceneCPU::IofxSceneCPU(ModuleIofx& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list) :
	IofxScene(module, scene, debugRender, list)
{
}

/******************************** GPU Version ********************************/

#if defined(APEX_CUDA_SUPPORT)
IofxSceneGPU::IofxSceneGPU(ModuleIofx& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list) :
	IofxScene(module, scene, debugRender, list),
	CudaModuleScene(scene, *mModule),
	mContextManager(scene.getTaskManager()->getGpuDispatcher()->getCudaContextManager())
{
	physx::PxGpuDispatcher* gd = scene.getTaskManager()->getGpuDispatcher();
	PX_ASSERT(gd != NULL);
	physx::PxScopedCudaLock _lock_(*gd->getCudaContextManager());

//CUDA module objects
#include "../cuda/include/moduleList.h"

	if (gd->getCudaContextManager()->supportsArchSM20() == true)
	{
		CUmodule cuModule = APEX_CUDA_OBJ_NAME(spriteTextureModifiersKernel).getModule();
		//manual surface setup
		APEX_CUDA_OBJ_NAME(surfRefOutput0).init(cudaObjList, cuModule);
		APEX_CUDA_OBJ_NAME(surfRefOutput1).init(cudaObjList, cuModule);
		APEX_CUDA_OBJ_NAME(surfRefOutput2).init(cudaObjList, cuModule);
		APEX_CUDA_OBJ_NAME(surfRefOutput3).init(cudaObjList, cuModule);
	}
}

IofxSceneGPU::~IofxSceneGPU()
{
	physx::PxScopedCudaLock s(*mContextManager);
	CudaModuleScene::destroy(*mApexScene);
}

void IofxSceneGPU::submitTasks(PxF32 elapsedTime, PxF32 substepSize, PxU32 numSubSteps)
{
	IofxScene::submitTasks(elapsedTime, substepSize, numSubSteps);

	{
		physx::PxScopedCudaLock _lock_(*mContextManager);

		APEX_CUDA_OBJ_NAME(volumeConstMem).copyToDevice(0);
		APEX_CUDA_OBJ_NAME(modifierConstMem).copyToDevice(0);
	}
}

bool IofxSceneGPU::copyDirtySceneData(PxGpuCopyDescQueue& /*queue*/)
{
	bool copies = false;

	//we pass default 0 stream so that this copy happends before any kernel launches
	//copies |= APEX_CUDA_OBJ_NAME(volumeConstMem).copyToDeviceQ(queue);
	//copies |= APEX_CUDA_OBJ_NAME(modifierConstMem).copyToDeviceQ(queue);
	return copies;
}

void IofxSceneGPU::prepareRenderResources()
{
	mManagersLock.lockReader();

	lockLiveRenderVolumes();
	mFetchResultsLock.lock();

	if (mContextManager->getInteropMode() != physx::PxCudaInteropMode::NO_INTEROP)
	{
		mToMapArray.clear();
		mToUnmapArray.clear();

		for (physx::PxU32 i = 0; i < mActorManagers.getSize(); ++i)
		{
			IofxManager* mgr = DYNAMIC_CAST(IofxManager*)(mActorManagers.getResource(i));
			mgr->fillMapUnmapArraysForInterop(mToMapArray, mToUnmapArray);
		}

		bool mapSuccess = false;
		bool unmapSuccess = false;
		{
			physx::PxScopedCudaLock s(*mContextManager);

			if (!mToMapArray.empty())
			{
				for (physx::PxU32 i = 0; i < mToMapArray.size(); ++i)
				{
					cuGraphicsResourceSetMapFlags( mToMapArray[i], CU_GRAPHICS_MAP_RESOURCE_FLAGS_WRITE_DISCARD );
				}
				CUresult res = cuGraphicsMapResources(mToMapArray.size(), &mToMapArray.front(), 0);
				mapSuccess = (res == CUDA_SUCCESS || res == CUDA_ERROR_ALREADY_MAPPED);
				PX_ASSERT(mapSuccess);
			}
			if (!mToUnmapArray.empty())
			{
				CUresult res = cuGraphicsUnmapResources(mToUnmapArray.size(), &mToUnmapArray.front(), 0);
				unmapSuccess = (res == CUDA_SUCCESS || res == CUDA_ERROR_NOT_MAPPED);
				PX_ASSERT(unmapSuccess);
			}
		}

		if (mapSuccess || unmapSuccess)
		{
			for (physx::PxU32 i = 0; i < mActorManagers.getSize(); ++i)
			{
				IofxManager* mgr = DYNAMIC_CAST(IofxManager*)(mActorManagers.getResource(i));
				mgr->mapBufferResults(mapSuccess, unmapSuccess);
			}
		}
	}

	for (physx::PxU32 i = 0; i < mActorManagers.getSize(); ++i)
	{
		IofxManager* mgr = DYNAMIC_CAST(IofxManager*)(mActorManagers.getResource(i));
		mgr->prepareRenderResources();
	}

	mFetchResultsLock.unlock();
	unlockLiveRenderVolumes();

	for (physx::PxU32 i = 0; i < mActorManagers.getSize(); ++i)
	{
		IofxManager* mgr = DYNAMIC_CAST(IofxManager*)(mActorManagers.getResource(i));
		mgr->postPrepareRenderResources();
	}

	mManagersLock.unlockReader();
}

#endif

}
}
} // namespace physx::apex
