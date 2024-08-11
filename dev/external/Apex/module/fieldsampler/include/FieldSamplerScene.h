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

#ifndef __FIELD_SAMPLER_SCENE_H__
#define __FIELD_SAMPLER_SCENE_H__

#include "NxApex.h"

#include "ModuleFieldSampler.h"
#include "NiFieldSamplerScene.h"
#include "NiApexSDK.h"
#include "NiModule.h"
#include "ApexInterface.h"
#include "ApexContext.h"
#include "ApexSDKHelpers.h"

#include "PxTask.h"

#if defined(APEX_CUDA_SUPPORT)
#include "ApexCudaWrapper.h"
#include "ApexCuda.h"
#include "CudaModuleScene.h"

#include "../cuda/include/common.h"

#define SCENE_CUDA_OBJ(scene, name) static_cast<FieldSamplerSceneGPU*>(scene)->APEX_CUDA_OBJ_NAME(name)
#endif


namespace physx
{
namespace apex
{
class NiApexScene;

namespace fieldsampler
{

class ModuleFieldSampler;
class FieldSamplerPhysXMonitor;
class FieldSamplerManager;

class FieldSamplerScene : public NiModuleScene, public ApexContext, public NxApexResource, public ApexResource
{
public:
	FieldSamplerScene(ModuleFieldSampler& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list);
	~FieldSamplerScene();

	/* NiModuleScene */
	void				visualize();

#if NX_SDK_VERSION_MAJOR == 2
	NxScene*			getModulePhysXScene() const
	{
		return mPhysXScene;
	}
	void				setModulePhysXScene(NxScene*);
	NxScene* 			mPhysXScene;
#elif NX_SDK_VERSION_MAJOR == 3
	PxScene*			getModulePhysXScene() const
	{
		return mPhysXScene;
	}
	void				setModulePhysXScene(PxScene*);
	PxScene* 			mPhysXScene;
#endif

	PxReal				setResource(PxReal, PxReal, PxReal)
	{
		return 0.0f;
	}
	PxReal				getBenefit()
	{
		return 0.0f;
	}

	void				submitTasks(PxF32 elapsedTime, PxF32 substepSize, PxU32 numSubSteps);
	void				setTaskDependencies();
	void				fetchResults();

	virtual NxModule*	getNxModule()
	{
		return mModule;
	}

	virtual NxApexSceneStats* getStats()
	{
		return 0;
	}

	/* NxApexResource */
	PxU32				getListIndex() const
	{
		return m_listIndex;
	}
	void				setListIndex(NxResourceList& list, PxU32 index)
	{
		m_listIndex = index;
		m_list = &list;
	}
	void				release()
	{
		mModule->releaseNiModuleScene(*this);
	}
	NiApexScene& getApexScene() const
	{
		return *mApexScene;
	}

	NiFieldSamplerManager* getManager();

#if NX_SDK_VERSION_MAJOR == 3
	/* Toggle PhysX Monitor on/off */
	void enablePhysXMonitor(bool enable);

	void addPhysXFilterData(physx::PxFilterData filterData);
	void removePhysXFilterData(physx::PxFilterData filterData);
#endif

#ifdef APEX_TEST
	bool setPhysXMonitorParticlesData(physx::PxU32 numParticles, physx::PxVec4** positions, physx::PxVec4** velocities);
	void getPhysXMonitorParticlesData(physx::PxVec4** velocities);
#endif

protected:
	void                destroy();

	virtual FieldSamplerManager* createManager() = 0;

	class TaskPhysXMonitorLoad : public physx::PxTask
	{
	public:
		TaskPhysXMonitorLoad() {}
		const char* getName() const
		{
			return FSST_PHYSX_MONITOR_LOAD;
		}		
		void run() {/* void task */};
	};
	TaskPhysXMonitorLoad	mPhysXMonitorLoadTask;
	class TaskPhysXMonitorFetch : public physx::PxTask
	{
	public:
		TaskPhysXMonitorFetch() {}
		const char* getName() const
		{
			return FSST_PHYSX_MONITOR_FETCH;
		}		
		void run() {/* void task */};
	};
	TaskPhysXMonitorFetch	mPhysXMonitorFetchTask;

	ModuleFieldSampler*		mModule;
	NiApexScene*			mApexScene;
	NiApexRenderDebug*		mDebugRender;
	FieldSamplerPhysXMonitor* mPhysXMonitor;

	FieldSamplerManager*	mManager;

	friend class ModuleFieldSampler;
	friend class FieldSamplerManager;
};

class FieldSamplerSceneCPU : public FieldSamplerScene
{
public:
	FieldSamplerSceneCPU(ModuleFieldSampler& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list);
	~FieldSamplerSceneCPU();

protected:
	virtual FieldSamplerManager* createManager();

};

#if defined(APEX_CUDA_SUPPORT)
class FieldSamplerSceneGPU : public FieldSamplerScene, public CudaModuleScene
{
public:
	FieldSamplerSceneGPU(ModuleFieldSampler& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list);
	~FieldSamplerSceneGPU();

//CUDA module objects
#include "../cuda/include/fieldsampler.h"

	PxCudaContextManager* getCudaContext() const
	{
		return mCtxMgr;
	}

protected:
	virtual FieldSamplerManager* createManager();

	/* keep a convenience pointer to the cuda context manager */
	PxCudaContextManager* mCtxMgr;
};
#endif

}
}
} // end namespace physx::apex

#endif
