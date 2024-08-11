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

#ifndef __FIELDBOUNDARY_SCENE_H__
#define __FIELDBOUNDARY_SCENE_H__

#include "NxApex.h"

#include "ModuleFieldBoundary.h"
#include "NiApexSDK.h"
#include "NiModule.h"
#include "ApexInterface.h"
#include "ApexContext.h"
#include "ApexSDKHelpers.h"

#include "DebugRenderParams.h"
#include "FieldBoundaryDebugRenderParams.h"
#include "PxTask.h"

namespace physx
{
namespace apex
{

class NiApexScene;
class NiFieldSamplerManager;

namespace fieldboundary
{

class ModuleFieldBoundary;

class FieldBoundaryScene : public NiModuleScene, public ApexContext, public NxApexResource, public ApexResource
{
public:
	FieldBoundaryScene(ModuleFieldBoundary& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list);
	~FieldBoundaryScene();

	/* NiModuleScene */
	void				updateActors(physx::PxF32 deltaTime);
	void				submitTasks(PxF32 elapsedTime, PxF32 substepSize, PxU32 numSubSteps);

	void				visualize();
	void				fetchResults();
	void				setModulePhysXScene(NxScene* s);
	NxScene*			getModulePhysXScene() const
	{
		return mPhysXScene;
	}
	physx::PxF32		setResource(physx::PxF32, physx::PxF32, physx::PxF32)
	{
		return 0.0f;
	}
	physx::PxF32		getBenefit()
	{
		return 0.0f;
	}
	NxModule*			getNxModule()
	{
		return mModule;
	}

	virtual NxApexSceneStats* getStats()
	{
		return 0;
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
	void				release()
	{
		mModule->releaseNiModuleScene(*this);
	}

	NiFieldSamplerManager* 				getNiFieldSamplerManager();

protected:
	void					destroy();

	ModuleFieldBoundary*	mModule;
	NiApexScene*			mApexScene;
	NiApexRenderDebug*		mRenderDebug;
	NxScene*				mPhysXScene;

	DebugRenderParams*					mDebugRenderParams;
	FieldBoundaryDebugRenderParams*		mFieldBoundaryDebugRenderParams;

	NiFieldSamplerManager* 				mFieldSamplerManager;

private:
	class TaskUpdate : public physx::PxTask
	{
	public:
		TaskUpdate(FieldBoundaryScene& owner) : mOwner(owner) {}
		const char* getName() const
		{
			return "FieldBoundaryScene::Update";
		}
		void run();

	protected:
		FieldBoundaryScene& mOwner;

	private:
		TaskUpdate& operator=(const TaskUpdate&);
	};
	TaskUpdate				mUpdateTask;

	friend class ModuleFieldBoundary;
	friend class FieldBoundaryActor;
	friend class TaskUpdate;
};

}
}
}

#endif
