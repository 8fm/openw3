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

#ifndef __BASIC_FS_ACTOR_H__
#define __BASIC_FS_ACTOR_H__

#include "NxApex.h"

#include "ApexActor.h"
#include "ApexInterface.h"
#include "NiFieldSampler.h"
#include "BasicFSAsset.h"

#include "PxTask.h"

#if defined(APEX_CUDA_SUPPORT)
#include "ApexCudaWrapper.h"
#endif


namespace physx
{
namespace apex
{

class NxRenderMeshActor;

namespace basicfs
{

class BasicFSScene;

class BasicFSActor : public ApexActor, public NxApexResource, public ApexResource, public NiFieldSampler
{
public:
	BasicFSActor(BasicFSScene&);
	virtual ~BasicFSActor();

	/* NxApexResource, ApexResource */
	PxU32					getListIndex() const
	{
		return m_listIndex;
	}
	void					setListIndex(class NxResourceList& list, PxU32 index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	virtual void			visualize()
	{
	}

	virtual void			simulate(physx::PxF32 dt)
	{
		PX_UNUSED(dt);
	}


#if NX_SDK_VERSION_MAJOR == 2
	void					setPhysXScene(NxScene*);
	NxScene*				getPhysXScene() const;
#elif NX_SDK_VERSION_MAJOR == 3
	void					setPhysXScene(PxScene*);
	PxScene*				getPhysXScene() const;
#endif

	/* NiFieldSampler */
	virtual bool			updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled) = 0;

protected:

	BasicFSScene*			mScene;

	physx::PxMat34Legacy	mPose;
	physx::PxF32			mScale;

	bool					mFieldSamplerChanged;
	bool					mFieldSamplerEnabled;

	physx::PxMat34Legacy	mDirToWorld;

	physx::Array<physx::PxVec3> mDebugPoints;

	friend class BasicFSScene;
};

}
}
} // end namespace apex

#endif
