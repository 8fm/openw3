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

#ifndef __EXPLOSION_ACTOR_H__
#define __EXPLOSION_ACTOR_H__

#include "NxApex.h"

#include "NxExplosionAsset.h"
#include "NxExplosionActor.h"
#include "ExplosionAsset.h"
#include "ApexActor.h"
#include "ApexInterface.h"
#include "ApexString.h"

#include "NiFieldSampler.h"

#if defined(APEX_CUDA_SUPPORT)
#include "ApexCudaWrapper.h"
#endif

#include "ExplosionFSCommon.h"

#if NX_SDK_VERSION_MAJOR == 2
#include <NxForceFieldDesc.h>
#endif

#define MAX_FORCE_PERCENTAGE_OF_SPACING		(0.9f)	// max force arrow is 90% of the spacing
#define MIN_FORCE_PERCENTAGE_OF_SPACING		(0.1f)	// min force arrow is 10% of the spacing

class ExplosionAssetParam;

#if NX_SDK_VERSION_MAJOR == 2
class NxForceField;
class NxForceFieldLinearKernel;
class NxForceFieldShape;
class NxForceFieldShapeGroup;
#endif

namespace physx
{
namespace apex
{
namespace explosion
{

class ExplosionAsset;
class ExplosionScene;

class ExplosionActor : public NxExplosionActor, public ApexActor, public ApexActorSource, public NxApexResource, public ApexResource, public NiFieldSampler
{
public:
	/* ExplosionActor methods */
	ExplosionActor(const NxExplosionActorDesc&, ExplosionAsset&, NxResourceList&, ExplosionScene&);
	~ExplosionActor()		{}
	NxExplosionAsset* 	getExplosionAsset() const;

	bool				isEnable()
	{
		return !mDisable;
	}
	bool				disable();
	bool				enable();
	physx::PxMat44		getPose() const
	{
		return mPose;
	}
	void				setPose(const physx::PxMat44& pose);
	physx::PxF32		getScale() const
	{
		return mScale;
	}
	void				setScale(physx::PxF32 scale);
	const char*			getName() const
	{
		return mName.c_str();
	}
	void				setName(const char* name)
	{
		mName = name;
	}

#if NX_SDK_VERSION_MAJOR == 2
	NxActor*			getAttachedNxActor() const
	{
		return mAttachedNxActor;
	}
	void				setAttachedNxActor(NxActor* a)
	{
		mAttachedNxActor = a;
	}

	void				addFieldBoundary(NxFieldBoundaryActor& bound);
	void				removeFieldBoundary(NxFieldBoundaryActor& bound);

	physx::PxF32		getOuterBoundRadius() const;
	void				setOuterBoundRadius(physx::PxF32 r);
	physx::PxF32		getInnerBoundRadius() const;
	void				setInnerBoundRadius(physx::PxF32 r);
#endif

	void                updatePoseAndBounds();  // Called by ExampleScene::fetchResults()

#if NX_SDK_VERSION_MAJOR == 2
	/* NxApexActorSource; templates for generating NxActors and NxShapes */
	void				setActorTemplate(const NxActorDescBase*);
	void				setShapeTemplate(const NxShapeDesc*);
	void				setBodyTemplate(const NxBodyDesc*);
	bool				getActorTemplate(NxActorDescBase& dest) const;
	bool				getShapeTemplate(NxShapeDesc& dest) const;
	bool				getBodyTemplate(NxBodyDesc& dest) const;
#endif

	/* NxApexResource, ApexResource */
	void				release();
	physx::PxU32				getListIndex() const
	{
		return m_listIndex;
	}
	void				setListIndex(class NxResourceList& list, physx::PxU32 index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	/* NxApexActor, ApexActor */
	void                destroy();
	NxApexAsset*		getOwner() const;

	/* PhysX scene management */
#if NX_SDK_VERSION_MAJOR == 2
	void				setPhysXScene(NxScene*);
	NxScene*			getPhysXScene() const;
#elif NX_SDK_VERSION_MAJOR == 3
	void				setPhysXScene(PxScene*);
	PxScene*			getPhysXScene() const;
#endif

	void				getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly);
	physx::PxF32		getActivePhysicalLod();
	void				forcePhysicalLod(physx::PxF32 lod);

	/* NiFieldSampler */
	virtual bool		updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled);

protected:
	void				updateExplosion(physx::PxF32 dt);
	void				releaseNxForceField();

protected:
	ExplosionScene*			mExplosionScene;
	bool					mDisable;
	NxApexForceFieldMode	mFfieldMode;

	physx::PxMat44			mPose;
	physx::PxF32			mScale;

#if NX_SDK_VERSION_MAJOR == 2
	NxActor*                mAttachedNxActor;
#endif

	physx::PxU32			mFlags;

#if NX_SDK_VERSION_MAJOR == 2
	NxForceFieldType		mFluidType;
	NxForceFieldType		mClothType;
	NxForceFieldType		mSoftBodyType;
	NxForceFieldType		mRigidBodyType;
	NxCollisionGroup		mGroup;
	NxGroupsMask			mGroupsMask;
	NxForceFieldVariety		mFfVariety;
#endif

	const ExplosionAssetParam* mParams;

	ApexSimpleString		mName;

	physx::Array<NxFieldBoundaryActor*>	mBoundariesActors;
	ExplosionAsset*			mAsset;

#if NX_SDK_VERSION_MAJOR == 2
	NxForceField*			mNxForceField;
	NxForceFieldLinearKernel* mLinearKernel;
	NxForceFieldShape*		mOuterBoundShape;
	NxForceFieldShape*		mInnerBoundShape;
	NxForceFieldShapeGroup* mDefaultIncludeSG;
	NxForceFieldShapeGroup* mDefaultExcludeSG;
#endif

	//field sampler stuff
	bool					mFieldSamplerChanged;
	void					initFieldSampler(/*const WindActorParameters& params*/);
	void					releaseFieldSampler(void);

protected:
	// debug rendering stuff
	bool							mDebugRenderingBuffersAllocated;
	static const physx::PxU32		mSamplePointsBufferSize = 100;
	physx::PxU32					mSamplePointsInBuffer;
	physx::PxVec3*					mSamplePointsLoc;
	physx::PxVec3*					mSamplePointsVel;
	physx::PxVec3*					mSamplePointsOutForces;
	physx::PxVec3*					mSamplePointsOutTorques;
	void							visualizeExplosionForces(void);
	void							addSamplePointToBuffer(physx::PxVec3 pt);
	void							SamplePointsOutputBuffer(void);
	void							ReleaseDebugRenderingBuffers(void);

	ExplosionFSParams				mExecuteParams;	// for execute field sampler use

	friend class ExplosionScene;
};

class ExplosionActorCPU : public ExplosionActor
{
public:
	ExplosionActorCPU(const NxExplosionActorDesc& desc, ExplosionAsset& asset, NxResourceList& list, ExplosionScene& scene);
	~ExplosionActorCPU();

	/* NiFieldSampler */
	virtual void executeFieldSampler(const ExecuteData& data);

private:
};

#if defined(APEX_CUDA_SUPPORT)

class ExplosionActorGPU : public ExplosionActor
{
public:
	ExplosionActorGPU(const NxExplosionActorDesc& desc, ExplosionAsset& asset, NxResourceList& list, ExplosionScene& scene);
	~ExplosionActorGPU();

	/* NiFieldSampler */
	virtual bool updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled);

	virtual void getFieldSamplerCudaExecuteInfo(CudaExecuteInfo& info) const
	{
		info.executeType = 0;
		info.executeParamsHandle = mParamsHandle;
	}

private:
	ApexCudaConstMemGroup				mConstMemGroup;
	InplaceHandle<ExplosionFSParams>	mParamsHandle;
};

#endif



}
}
} // end namespace physx::apex

#endif
