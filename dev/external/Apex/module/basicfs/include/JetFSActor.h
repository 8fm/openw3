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

#ifndef __JET_FS_ACTOR_H__
#define __JET_FS_ACTOR_H__

#include "BasicFSActor.h"
#include "NxJetFSActor.h"

#include "JetFSCommon.h"

#include "variable_oscillator.h"


namespace physx
{
namespace apex
{

class NxRenderMeshActor;

namespace basicfs
{

class JetFSAsset;
class BasicFSScene;
class JetFSActorParams;

class JetFSActor : public BasicFSActor, public NxJetFSActor
{
public:
	/* NxJetFSActor methods */
	JetFSActor(const JetFSActorParams&, JetFSAsset&, NxResourceList&, BasicFSScene&);
	~JetFSActor();

	NxBasicFSAsset* 		getJetFSAsset() const;

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

	physx::PxF32			getCurrentScale() const
	{
		return mScale;
	}

	void					setCurrentScale(const physx::PxF32& scale)
	{
		mScale = scale;
		mFieldSamplerChanged = true;
	}

	void					setFieldStrength(physx::PxF32 strength);
	void					setFieldDirection(const physx::PxVec3& direction);

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
	JetFSAsset*				mAsset;

	physx::PxVec3			mFieldDirection;
	variableOscillator*		mFieldDirectionVO1;
	variableOscillator*		mFieldDirectionVO2;

	physx::PxF32			mFieldStrength;
	variableOscillator*		mFieldStrengthVO;

	physx::PxF32			mStrengthVar;
	physx::PxVec3			mLocalDirVar;
	physx::PxMat34Legacy	mDirToWorld;

	JetFSParams				mExecuteParams; 

	physx::Array<physx::PxVec3> mDebugPoints;

	friend class BasicFSScene;
};

class JetFSActorCPU : public JetFSActor
{
public:
	JetFSActorCPU(const JetFSActorParams&, JetFSAsset&, NxResourceList&, BasicFSScene&);
	~JetFSActorCPU();

	/* NiFieldSampler */
	virtual void executeFieldSampler(const ExecuteData& data);

private:
};

#if defined(APEX_CUDA_SUPPORT)

class JetFSActorGPU : public JetFSActor
{
public:
	JetFSActorGPU(const JetFSActorParams&, JetFSAsset&, NxResourceList&, BasicFSScene&);
	~JetFSActorGPU();

	/* NiFieldSampler */
	virtual bool updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled);

	virtual void getFieldSamplerCudaExecuteInfo(CudaExecuteInfo& info) const
	{
		info.executeType = 1;
		info.executeParamsHandle = mParamsHandle;
	}

private:
	ApexCudaConstMemGroup           mConstMemGroup;
	InplaceHandle<JetFSParams>		mParamsHandle;

};

#endif

}
}
} // end namespace apex

#endif
