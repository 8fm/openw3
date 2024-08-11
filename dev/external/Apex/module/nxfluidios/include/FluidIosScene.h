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

#ifndef __FLUIDIOS_SCENE_H__
#define __FLUIDIOS_SCENE_H__

#include "NxApex.h"
#include "NxModuleFluidIos.h"
#include "NiApexSDK.h"
#include "NiModule.h"
#include "ModuleFluidIos.h"
#include "ApexSharedUtils.h"
#include "ApexSDKHelpers.h"
#include "ApexContext.h"
#include "ApexActor.h"
#include "ModulePerfScope.h"

#include "DebugRenderParams.h"
#include "FluidIosDebugRenderParams.h"

namespace physx
{
namespace apex
{

class NiApexRenderDebug;

namespace nxfluidios
{

class FluidIosScene : public NiModuleScene, public ApexContext, public NxApexResource, public ApexResource
{
public:
	FluidIosScene(ModuleFluidIos& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list);
	~FluidIosScene();

	/* NiModuleScene */
	void									submitTasks(PxF32 elapsedTime, PxF32 substepSize, PxU32 numSubSteps);
	void									setTaskDependencies();

	void                                    fetchResults();
	void                                    setModulePhysXScene(NxScene*);
	NxScene*                                getModulePhysXScene() const
	{
		return mPhysXScene;
	}
	void									release()
	{
		mModule->releaseNiModuleScene(*this);
	}
	void									visualize();
	virtual NxModule*						getNxModule()
	{
		return mModule;
	}
	const NxCompartment* 					getCompartment() const;
	const NxCompartment* 					getSPHCompartment() const;

	virtual NxApexSceneStats* getStats()
	{
		return 0;
	}

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

	physx::PxF32							getBenefit();
	physx::PxF32							setResource(physx::PxF32 , physx::PxF32, physx::PxF32);


	NiApexScene*							getApexScene() const
	{
		return mApexScene;
	}

protected:
	ModuleFluidIos*                        mModule;
	NxScene*                                mPhysXScene;
	NiApexScene*                            mApexScene;

	void									destroy();

	physx::PxF32							computeAABBDistanceSquared(const physx::PxBounds3& aabb);

	void									setCompartment(NxCompartment& comp);
	void									setSPHCompartment(NxCompartment& comp);

private:
	NxCompartment* 							mCompartment;
	NxCompartment* 							mSPHCompartment;
	physx::PxF32							mSumBenefit;
	NiApexRenderDebug* 						mRenderDebug;

	DebugRenderParams*						mDebugRenderParams;
	FluidIosDebugRenderParams*				mFluidIosDebugRenderParams;

	friend class FluidIosActor;
	friend class FluidIosAsset;
	friend class ModuleFluidIos;
};

}
}
} // namespace physx::apex

#endif // __FLUIDIOS_SCENE_H__
