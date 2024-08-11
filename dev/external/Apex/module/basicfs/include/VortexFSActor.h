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

#ifndef __VORTEX_FS_ACTOR_H__
#define __VORTEX_FS_ACTOR_H__

#include "BasicFSActor.h"
#include "NxVortexFSActor.h"

#include "VortexFSCommon.h"


namespace physx
{
namespace apex
{

class NxRenderMeshActor;	
	
namespace basicfs
{

class VortexFSAsset;
class BasicFSScene;
class VortexFSActorParams;

class VortexFSActor : public BasicFSActor, public NxVortexFSActor
{
public:
	/* NxVortexFSActor methods */
	VortexFSActor(const VortexFSActorParams&, VortexFSAsset&, NxResourceList&, BasicFSScene&);
	~VortexFSActor();

	NxBasicFSAsset* 		getVortexFSAsset() const;

	physx::PxMat44			getCurrentPose() const
	{
		return PxMat44(mPose);
	}

	void					setCurrentPose(const physx::PxMat44& pose)
	{
		mPose = pose;
		mFieldSamplerChanged = true;
	}

	physx::PxVec3			getCurrentPosition() const
	{		
		return mPose.t;
	}
	void					setCurrentPosition(const physx::PxVec3& pos)
	{
		mPose.t = pos;
		mFieldSamplerChanged = true;
	}
	void					setAxis(const physx::PxVec3& axis)
	{
		mAxis = axis;
		mFieldSamplerChanged = true;
	}
	void					setHeight(physx::PxF32 height)
	{
		mHeight = height;
		mFieldSamplerChanged = true;
	}
	void					setBottomRadius(physx::PxF32 radius)
	{
		mBottomRadius = radius;
		mFieldSamplerChanged = true;
	}
	void					setTopRadius(physx::PxF32 radius)
	{
		mTopRadius = radius;
		mFieldSamplerChanged = true;
	}

	void					setRotationalStrength(physx::PxF32 strength);
	void					setRadialStrength(physx::PxF32 strength);
	void					setLiftStrength(physx::PxF32 strength);

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
	VortexFSAsset*			mAsset;
	physx::PxMat34Legacy	mPose;
	
	physx::PxVec3			mAxis;
	physx::PxF32			mHeight;
	physx::PxF32			mBottomRadius;
	physx::PxF32			mTopRadius;

	physx::PxF32			mRotationalStrength;
	physx::PxF32			mRadialStrength;
	physx::PxF32			mLiftStrength;

	VortexFSParams			mExecuteParams; 

	physx::Array<physx::PxVec3> mDebugPoints;

	friend class BasicFSScene;
};

class VortexFSActorCPU : public VortexFSActor
{
public:
	VortexFSActorCPU(const VortexFSActorParams&, VortexFSAsset&, NxResourceList&, BasicFSScene&);
	~VortexFSActorCPU();

	/* NiFieldSampler */
	virtual void executeFieldSampler(const ExecuteData& data);

private:
};

#if defined(APEX_CUDA_SUPPORT)

class VortexFSActorGPU : public VortexFSActor
{
public:
	VortexFSActorGPU(const VortexFSActorParams&, VortexFSAsset&, NxResourceList&, BasicFSScene&);
	~VortexFSActorGPU();

	/* NiFieldSampler */
	virtual bool updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled);

	virtual void getFieldSamplerCudaExecuteInfo(CudaExecuteInfo& info) const
	{
		info.executeType = 4;
		info.executeParamsHandle = mParamsHandle;
	}

private:
	ApexCudaConstMemGroup			mConstMemGroup;
	InplaceHandle<VortexFSParams>	mParamsHandle;

};

#endif

}
}
} // end namespace apex

#endif
