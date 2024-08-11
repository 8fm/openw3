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

#ifndef __FORCEFIELD_SCENE_H__
#define __FORCEFIELD_SCENE_H__

#include "NxApex.h"

#include "ModuleForceField.h"

#include "ApexInterface.h"
#include "ApexContext.h"
#include "ApexSDKHelpers.h"

#include "NiApexRenderDebug.h"
#include "NiApexSDK.h"
#include "NiModule.h"

#include "DebugRenderParams.h"
#include "ForceFieldDebugRenderParams.h"

#include "PxTask.h"

#include "NiFieldSamplerScene.h"

#if defined(APEX_CUDA_SUPPORT)
#include "ApexCudaWrapper.h"
#include "ApexCuda.h"
#include "CudaModuleScene.h"

#include "../cuda/include/common.h"

#define SCENE_CUDA_OBJ(scene, name) static_cast<ForceFieldSceneGPU*>(scene)->APEX_CUDA_OBJ_NAME(name)
#define CUDA_OBJ(name) SCENE_CUDA_OBJ(mForceFieldScene, name)
#endif

namespace physx
{
namespace apex
{
class NiApexScene;
class DebugRenderParams;
class NiFieldSamplerManager;

namespace forcefield
{
class ModuleForceField;
class ForceFieldActor;
class NxForceFieldActorDesc;


class ForceFieldScene : public NiFieldSamplerScene, public ApexContext, public NxApexResource, public ApexResource
{
public:
	ForceFieldScene(ModuleForceField& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list);
	~ForceFieldScene();

	/* NiModuleScene */
	void						updateActors(physx::PxF32 deltaTime);
	void						submitTasks(PxF32 elapsedTime, PxF32 substepSize, PxU32 numSubSteps);
	void						setTaskDependencies();

	virtual void				visualize(void);
	virtual void				visualizeForceFieldForceFields(void);
	virtual void				visualizeForceFieldForces(void);
	virtual void				fetchResults(void);

	virtual void				setModulePhysXScene(PxScene* s);
	virtual PxScene*			getModulePhysXScene() const
	{
		return mPhysXScene;
	}

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

	virtual ForceFieldActor*	createForceFieldActor(const NxForceFieldActorDesc& desc, ForceFieldAsset& asset, NxResourceList& list) = 0;

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

	ModuleForceField* 			mModule;
	NiApexScene*                mApexScene;
	PxScene*                    mPhysXScene;

	NiApexRenderDebug* 			mRenderDebug;

	DebugRenderParams*				mDebugRenderParams;
	ForceFieldDebugRenderParams*	mForceFieldDebugRenderParams;

	NiFieldSamplerManager*		mFieldSamplerManager;

private:
	class TaskUpdate : public physx::PxTask
	{
	public:
		TaskUpdate(ForceFieldScene& owner) : mOwner(owner) {}
		const char* getName() const
		{
			return "ForceFieldScene::Update";
		}
		void run();
	protected:
		ForceFieldScene& mOwner;
	private:
		TaskUpdate& operator=(const  TaskUpdate&);
	};

	TaskUpdate					mUpdateTask;

	friend class ModuleForceField;
	friend class ForceFieldActor;
	friend class TaskUpdate;
};

class ForceFieldSceneCPU : public ForceFieldScene
{
public:
	ForceFieldSceneCPU(ModuleForceField& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list);
	~ForceFieldSceneCPU();

	ForceFieldActor*	createForceFieldActor(const NxForceFieldActorDesc& desc, ForceFieldAsset& asset, NxResourceList& list);

	/* NiFieldSamplerScene */

protected:
};

#if defined(APEX_CUDA_SUPPORT)
class ForceFieldSceneGPU : public ForceFieldScene, public CudaModuleScene
{
public:
	ForceFieldSceneGPU(ModuleForceField& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list);
	~ForceFieldSceneGPU();

	ForceFieldActor*	createForceFieldActor(const NxForceFieldActorDesc& desc, ForceFieldAsset& asset, NxResourceList& list);

//CUDA module objects
#include "../cuda/include/ForceField.h"

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
