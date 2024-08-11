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

#ifndef __BASIC_FS_SCENE_H__
#define __BASIC_FS_SCENE_H__

#include "NxApex.h"

#include "ModuleBasicFS.h"
#include "NiApexSDK.h"
#include "NiModule.h"
#include "ApexInterface.h"
#include "ApexContext.h"
#include "ApexSDKHelpers.h"

#include "DebugRenderParams.h"
#include "BasicFSDebugRenderParams.h"

#include "PxTask.h"

#include "NiFieldSamplerScene.h"

#if defined(APEX_CUDA_SUPPORT)
#include "ApexCudaWrapper.h"
#include "ApexCuda.h"
#include "CudaModuleScene.h"

#include "../cuda/include/common.h"

#define SCENE_CUDA_OBJ(scene, name) static_cast<BasicFSSceneGPU*>(scene)->APEX_CUDA_OBJ_NAME(name)
#define CUDA_OBJ(name) SCENE_CUDA_OBJ(mScene, name)
#endif


namespace physx
{
namespace apex
{

class NiApexScene;
class NiFieldSamplerManager;

namespace basicfs
{

class ModuleBasicFS;

class BasicFSAsset;
class NxBasicFSActorDesc;
class BasicFSActor;

class JetFSAsset;
class JetFSActor;

class AttractorFSAsset;
class AttractorFSActor;

class VortexFSAsset;
class VortexFSActor;
class NoiseFSAsset;
class NoiseFSActor;

class BasicFSScene : public NiFieldSamplerScene, public ApexContext, public NxApexResource, public ApexResource
{
public:
	BasicFSScene(ModuleBasicFS& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list);
	~BasicFSScene();

	/* NiModuleScene */
	void						visualize();
#if NX_SDK_VERSION_MAJOR == 2
	void						setModulePhysXScene(NxScene* s);
	NxScene*					getModulePhysXScene() const
	{
		return mPhysXScene;
	}
#elif NX_SDK_VERSION_MAJOR == 3
	void						setModulePhysXScene(PxScene* s);
	PxScene*					getModulePhysXScene() const
	{
		return mPhysXScene;
	}
#endif
	PxReal						setResource(PxReal, PxReal, PxReal)
	{
		return 0.0f;
	}
	PxReal						getBenefit()
	{
		return 0.0f;
	}

	void						submitTasks(PxF32 elapsedTime, PxF32 substepSize, PxU32 numSubSteps);
	void						fetchResults();

	virtual NxModule*			getNxModule()
	{
		return mModule;
	}

	/* NxApexResource */
	PxU32						getListIndex() const
	{
		return m_listIndex;
	}
	void						setListIndex(NxResourceList& list, PxU32 index)
	{
		m_listIndex = index;
		m_list = &list;
	}
	void						release()
	{
		mModule->releaseNiModuleScene(*this);
	}

	virtual JetFSActor*			createJetFSActor(const JetFSActorParams&, JetFSAsset&, NxResourceList&) = 0;
	virtual AttractorFSActor*	createAttractorFSActor(const AttractorFSActorParams&, AttractorFSAsset&, NxResourceList&) = 0;
	virtual VortexFSActor*		createVortexFSActor(const VortexFSActorParams&, VortexFSAsset&, NxResourceList&) = 0;
	virtual NoiseFSActor*		createNoiseFSActor(const NoiseFSActorParams&, NoiseFSAsset&, NxResourceList&) = 0;

	NiApexScene&				getApexScene() const
	{
		return *mApexScene;
	}

	NiFieldSamplerManager* 		getNiFieldSamplerManager();

	/* NiFieldSamplerScene */
	virtual void				getFieldSamplerSceneDesc(NiFieldSamplerSceneDesc& desc) const
	{
		PX_UNUSED(desc);
	}

protected:
	void						destroy();

	ModuleBasicFS*				mModule;
	NiApexScene*				mApexScene;
#if NX_SDK_VERSION_MAJOR == 2
	NxScene*					mPhysXScene;
#elif NX_SDK_VERSION_MAJOR == 3
	PxScene*					mPhysXScene;
#endif
	NiApexRenderDebug*			mDebugRender;

	DebugRenderParams*			mDebugRenderParams;
	BasicFSDebugRenderParams*	mBasicFSDebugRenderParams;

	NiFieldSamplerManager* 		mFieldSamplerManager;

	friend class ModuleBasicFS;
	friend class JetFSActor;
	friend class AttractorFSActor;
	friend class VortexFSActor;
	friend class NoiseFSActor;
};

class BasicFSSceneCPU : public BasicFSScene
{
public:
	BasicFSSceneCPU(ModuleBasicFS& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list);
	~BasicFSSceneCPU();

	JetFSActor*					createJetFSActor(const JetFSActorParams&, JetFSAsset&, NxResourceList&);
	AttractorFSActor*			createAttractorFSActor(const AttractorFSActorParams&, AttractorFSAsset&, NxResourceList&);
	VortexFSActor*				createVortexFSActor(const VortexFSActorParams&, VortexFSAsset&, NxResourceList&);
	NoiseFSActor*				createNoiseFSActor(const NoiseFSActorParams&, NoiseFSAsset&, NxResourceList&);

	/* NiFieldSamplerScene */

protected:
};

#if defined(APEX_CUDA_SUPPORT)
class BasicFSSceneGPU : public BasicFSScene, public CudaModuleScene
{
public:
	BasicFSSceneGPU(ModuleBasicFS& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list);
	~BasicFSSceneGPU();

	JetFSActor*					createJetFSActor(const JetFSActorParams&, JetFSAsset&, NxResourceList&);
	AttractorFSActor*			createAttractorFSActor(const AttractorFSActorParams&, AttractorFSAsset&, NxResourceList&);
	VortexFSActor*				createVortexFSActor(const VortexFSActorParams&, VortexFSAsset&, NxResourceList&);
	NoiseFSActor*				createNoiseFSActor(const NoiseFSActorParams&, NoiseFSAsset&, NxResourceList&);

//CUDA module objects
#include "../cuda/include/basicfs.h"

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
