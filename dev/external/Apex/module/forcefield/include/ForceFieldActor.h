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

#ifndef __FORCEFIELD_ACTOR_H__
#define __FORCEFIELD_ACTOR_H__

#include "NxApex.h"

#include "NxForceFieldAsset.h"
#include "NxForceFieldActor.h"
#include "ForceFieldAsset.h"
#include "ApexActor.h"
#include "ApexInterface.h"
#include "ApexString.h"

#include "NiFieldSampler.h"

#if defined(APEX_CUDA_SUPPORT)
#include "ApexCudaWrapper.h"
#endif

#include "ForceFieldFSCommon.h"

class ForceFieldAssetParams;

namespace physx
{
namespace apex
{

/*
PX_INLINE bool operator != (const NxGroupsMask64& d1, const NxGroupsMask64& d2)
{
	return d1.bits0 != d2.bits0 || d1.bits1 != d2.bits1;
}*/
PX_INLINE bool operator != (const physx::PxFilterData& d1, const physx::PxFilterData& d2)
{
	//if (d1.word3 != d2.word3) return d1.word3 < d2.word3;
	//if (d1.word2 != d2.word2) return d1.word2 < d2.word2;
	//if (d1.word1 != d2.word1) return d1.word1 < d2.word1;
	return d1.word0 != d2.word0 || d1.word1 != d2.word1 || d1.word2 != d2.word2 || d1.word3 != d2.word3;
}

namespace forcefield
{

class ForceFieldAsset;
class ForceFieldScene;

class ForceFieldActor : public NxForceFieldActor, public ApexActor, public ApexActorSource, public NxApexResource, public ApexResource, public NiFieldSampler
{
public:
	/* ForceFieldActor methods */
	ForceFieldActor(const NxForceFieldActorDesc&, ForceFieldAsset&, NxResourceList&, ForceFieldScene&);
	~ForceFieldActor() {}
	NxForceFieldAsset* 	getForceFieldAsset() const;

	bool				disable();
	bool				enable();
	bool				isEnable()
	{
		return mEnable;
	}
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

	void				setStrength(const physx::PxF32 strength);
	void				setLifetime(const physx::PxF32 lifetime);
	void				setIncludeShapeType(const char* shape);
	void				setFalloffType(const char* type);
	void				setFalloffMultiplier(const physx::PxF32 multiplier);

	void                updatePoseAndBounds();  // Called by ExampleScene::fetchResults()

	/* NxApexResource, ApexResource */
	void				release();
	physx::PxU32		getListIndex() const
	{
		return m_listIndex;
	}
	void				setListIndex(class NxResourceList& list, physx::PxU32 index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	void				addFilterData(const physx::PxFilterData& data);
	void				removeFilterData(const physx::PxFilterData& data);
	void				getFilterData(physx::PxFilterData* data, physx::PxU32& size);


	/* NxApexActor, ApexActor */
	void                destroy();
	NxApexAsset*		getOwner() const;

	/* PhysX scene management */
	void				setPhysXScene(PxScene*);
	PxScene*			getPhysXScene() const;

	void				getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly);
	physx::PxF32		getActivePhysicalLod();
	void				forcePhysicalLod(physx::PxF32 lod);

	/* NiFieldSampler */
	virtual bool		updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled);

	virtual physx::PxVec3 queryFieldSamplerVelocity() const
	{
		return physx::PxVec3(0.0f);
	}

protected:
	void				updateForceField(physx::PxF32 dt);
	void				releaseNxForceField();

protected:
	ForceFieldScene*		mForceFieldScene;

	physx::PxMat44			mPose;
	physx::PxF32			mScale;

	physx::PxU32			mFlags;

	ApexSimpleString		mName;

	ForceFieldAsset*		mAsset;

	bool					mEnable;
	physx::PxF32			mElapsedTime;

	Array<physx::PxFilterData>	mFilterData;

	/* Force field actor parameters */
	ForceFieldShapeDesc		mIncludeShape;
	physx::PxF32			mStrength;
	physx::PxF32			mLifetime;
	TableLookup				mFalloffTable;
	NoiseParams				mNoiseParams;
	void					initActorParams();

	/* Field Sampler Stuff */
	bool					mFieldSamplerChanged;
	void					initFieldSampler(const NxForceFieldActorDesc& desc);
	void					releaseFieldSampler();

	/* Debug Rendering Stuff */
	void					visualize();
	void					visualizeIncludeShape();
	void					visualizeForces();

	ForceFieldFSParams		mExecuteParams;	// for execute field sampler use

	friend class ForceFieldScene;
};

class ForceFieldActorCPU : public ForceFieldActor
{
public:
	ForceFieldActorCPU(const NxForceFieldActorDesc& desc, ForceFieldAsset& asset, NxResourceList& list, ForceFieldScene& scene);
	~ForceFieldActorCPU();

	/* NiFieldSampler */
	virtual void executeFieldSampler(const ExecuteData& data);

private:
};

#if defined(APEX_CUDA_SUPPORT)

class ForceFieldActorGPU : public ForceFieldActor
{
public:
	ForceFieldActorGPU(const NxForceFieldActorDesc& desc, ForceFieldAsset& asset, NxResourceList& list, ForceFieldScene& scene);
	~ForceFieldActorGPU();

	/* NiFieldSampler */
	virtual bool updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled);

	virtual void getFieldSamplerCudaExecuteInfo(CudaExecuteInfo& info) const
	{
		info.executeType = 0;
		info.executeParamsHandle = mParamsHandle;
	}

private:
	ApexCudaConstMemGroup				mConstMemGroup;
	InplaceHandle<ForceFieldFSParams>	mParamsHandle;
};

#endif

}
}
} // end namespace physx::apex

#endif
