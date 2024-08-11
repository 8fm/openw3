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

#ifndef __EXPLOSION_SCENE_H__
#define __EXPLOSION_SCENE_H__

#include "NxApex.h"

#include "ModuleExplosion.h"

#include "ApexInterface.h"
#include "ApexContext.h"
#include "ApexSDKHelpers.h"

#include "NiApexRenderDebug.h"
#include "NiApexSDK.h"
#include "NiModule.h"

#include "DebugRenderParams.h"
#include "ExplosionDebugRenderParams.h"

#include "PxTask.h"

#include "NiFieldSamplerScene.h"

#if defined(APEX_CUDA_SUPPORT)
#include "ApexCudaWrapper.h"
#include "ApexCuda.h"
#include "CudaModuleScene.h"

#include "../cuda/include/common.h"

#define SCENE_CUDA_OBJ(scene, name) static_cast<ExplosionSceneGPU*>(scene)->APEX_CUDA_OBJ_NAME(name)
#define CUDA_OBJ(name) SCENE_CUDA_OBJ(mExplosionScene, name)
#endif

namespace physx
{
namespace apex
{

class NiApexScene;
class NiFieldSamplerManager;
class DebugRenderParams;

namespace explosion
{
class ModuleExplosion;
class ExplosionActor;
class NxExplosionActorDesc;

class ExplosionScene : public NiFieldSamplerScene, public ApexContext, public NxApexResource, public ApexResource
{
public:
	ExplosionScene(ModuleExplosion& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list);
	~ExplosionScene();

	/* NiModuleScene */
	void						updateActors(physx::PxF32 deltaTime);
	void						submitTasks(PxF32 elapsedTime, PxF32 substepSize, PxU32 numSubSteps);

	virtual void				visualize(void);
	virtual void				visualizeExplosionForceFields(void);
	virtual void				visualizeExplosionForces(void);
	virtual void				fetchResults(void);

#if NX_SDK_VERSION_MAJOR == 2
	virtual void				setModulePhysXScene(NxScene* s);
	virtual NxScene*			getModulePhysXScene() const
	{
		return mPhysXScene;
	}
#elif NX_SDK_VERSION_MAJOR == 3
	virtual void				setModulePhysXScene(PxScene* s);
	virtual PxScene*			getModulePhysXScene() const
	{
		return mPhysXScene;
	}
#endif

	virtual physx::PxF32		setResource(physx::PxF32, physx::PxF32, physx::PxF32)
	{
		return 0.0f;
	}
	virtual physx::PxF32		getBenefit(void)
	{
		return 0.0f;
	}
	virtual NxModule*			getNxModule()
	{
		return mModule;
	}

	/* NxApexResource */
	PxU32						getListIndex(void) const
	{
		return m_listIndex;
	}
	void						setListIndex(NxResourceList& list, PxU32 index)
	{
		m_listIndex = index;
		m_list = &list;
	}
	virtual void				release(void)
	{
		mModule->releaseNiModuleScene(*this);
	}

	virtual ExplosionActor*	createExplosionActor(const NxExplosionActorDesc& desc, ExplosionAsset& asset, NxResourceList& list) = 0;

	NiApexScene& getApexScene() const
	{
		return *mApexScene;
	}

	NiFieldSamplerManager*	getNiFieldSamplerManager();

	/* NiFieldSamplerScene */
	virtual void getFieldSamplerSceneDesc(NiFieldSamplerSceneDesc& ) const
	{
	}

protected:
	void						destroy();

	ModuleExplosion* 			mModule;
	NiApexScene*                mApexScene;

#if NX_SDK_VERSION_MAJOR == 2
	NxScene*                    mPhysXScene;
#elif NX_SDK_VERSION_MAJOR == 3
	PxScene*                    mPhysXScene;
#endif

	NiApexRenderDebug* 			mRenderDebug;

	DebugRenderParams*			mDebugRenderParams;
	ExplosionDebugRenderParams*	mExplosionDebugRenderParams;

	NiFieldSamplerManager*		mFieldSamplerManager;

private:
	class TaskUpdate : public physx::PxTask
	{
	public:
		TaskUpdate(ExplosionScene& owner) : mOwner(owner) {}
		const char* getName() const
		{
			return "ExplosionScene::Update";
		}
		void run();

	protected:
		ExplosionScene& mOwner;

	private:
		TaskUpdate& operator=(const TaskUpdate&);
	};

	TaskUpdate					mUpdateTask;

	friend class ModuleExplosion;
	friend class ExplosionActor;
	friend class TaskUpdate;
};

class ExplosionSceneCPU : public ExplosionScene
{
public:
	ExplosionSceneCPU(ModuleExplosion& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list);
	~ExplosionSceneCPU();

	ExplosionActor*	createExplosionActor(const NxExplosionActorDesc& desc, ExplosionAsset& asset, NxResourceList& list);

	/* NiFieldSamplerScene */

protected:
};

#if defined(APEX_CUDA_SUPPORT)
class ExplosionSceneGPU : public ExplosionScene, public CudaModuleScene
{
public:
	ExplosionSceneGPU(ModuleExplosion& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list);
	~ExplosionSceneGPU();

	ExplosionActor*	createExplosionActor(const NxExplosionActorDesc& desc, ExplosionAsset& asset, NxResourceList& list);

//CUDA module objects
#include "../cuda/include/Explosion.h"

	/* NiFieldSamplerScene */
	virtual ApexCudaConstMem*	getFieldSamplerCudaConstMem();
	virtual bool				launchFieldSamplerCudaKernel(const fieldsampler::NiFieldSamplerKernelLaunchData&);

protected:
	/* keep a convenience pointer to the cuda context manager */
	PxCudaContextManager* mCtxMgr;
};
#endif

}
}
} // end namespace physx::apex

#endif
