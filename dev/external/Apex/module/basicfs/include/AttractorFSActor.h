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

#ifndef __ATTRACTOR_FS_ACTOR_H__
#define __ATTRACTOR_FS_ACTOR_H__

#include "BasicFSActor.h"
#include "NxAttractorFSActor.h"

#include "AttractorFSCommon.h"


namespace physx
{
namespace apex
{

class NxRenderMeshActor;

namespace basicfs
{

class AttractorFSAsset;
class BasicFSScene;
class AttractorFSActorParams;

class AttractorFSActor : public BasicFSActor, public NxAttractorFSActor
{
public:
	/* NxAttractorFSActor methods */
	AttractorFSActor(const AttractorFSActorParams&, AttractorFSAsset&, NxResourceList&, BasicFSScene&);
	~AttractorFSActor();

	NxBasicFSAsset* 		getAttractorFSAsset() const;

	physx::PxVec3			getCurrentPosition() const
	{
		return mPosition;
	}
	void					setCurrentPosition(const physx::PxVec3& pos)
	{
		mPosition = pos;
		mFieldSamplerChanged = true;
	}
	void					setFieldRadius(physx::PxF32 radius)
	{
		mRadius = radius;
		mFieldSamplerChanged = true;
	}
	void					setConstFieldStrength(physx::PxF32 strength);

	void					setVariableFieldStrength(physx::PxF32 strength);

	void					setEnabled(bool isEnabled)
	{
		mFieldSamplerEnabled = isEnabled;
	}

	/* NxApexRenderable, NxApexRenderDataProvider */
	void					updateRenderResources(bool rewriteBuffers, void* userRenderData);
	void					dispatchRenderResources(NxUserRenderer& renderer);

	PxBounds3				getBounds() const
	{
		return ApexRenderable::getBounds();
	}

	void					lockRenderResources()
	{
		ApexRenderable::renderDataLock();
	}
	void					unlockRenderResources()
	{
		ApexRenderable::renderDataUnLock();
	}

	void					getPhysicalLodRange(PxReal& min, PxReal& max, bool& intOnly);
	physx::PxF32			getActivePhysicalLod();
	void					forcePhysicalLod(PxReal lod);

	NxApexRenderable*		getRenderable()
	{
		return this;
	}
	NxApexActor*			getNxApexActor()
	{
		return this;
	}

	/* NxApexResource, ApexResource */
	void					release();

	/* NxApexActor, ApexActor */
	void					destroy();
	NxApexAsset*			getOwner() const;

	virtual void			simulate(physx::PxF32 dt);

	virtual void			visualize();

	/* NiFieldSampler */
	virtual bool			updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled);

protected:
	AttractorFSAsset*		mAsset;
	physx::PxVec3			mPosition;

	physx::PxF32			mRadius;

	physx::PxF32			mConstFieldStrength;
	physx::PxF32			mVariableFieldStrength;

	AttractorFSParams		mExecuteParams; 

	physx::Array<physx::PxVec3> mDebugPoints;

	friend class BasicFSScene;
};

class AttractorFSActorCPU : public AttractorFSActor
{
public:
	AttractorFSActorCPU(const AttractorFSActorParams&, AttractorFSAsset&, NxResourceList&, BasicFSScene&);
	~AttractorFSActorCPU();

	/* NiFieldSampler */
	virtual void executeFieldSampler(const ExecuteData& data);

private:
};

#if defined(APEX_CUDA_SUPPORT)

class AttractorFSActorGPU : public AttractorFSActor
{
public:
	AttractorFSActorGPU(const AttractorFSActorParams&, AttractorFSAsset&, NxResourceList&, BasicFSScene&);
	~AttractorFSActorGPU();

	/* NiFieldSampler */
	virtual bool updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled);

	virtual void getFieldSamplerCudaExecuteInfo(CudaExecuteInfo& info) const
	{
		info.executeType = 2;
		info.executeParamsHandle = mParamsHandle;
	}

private:
	ApexCudaConstMemGroup				mConstMemGroup;
	InplaceHandle<AttractorFSParams>	mParamsHandle;

};

#endif

}
}
} // end namespace apex

#endif
