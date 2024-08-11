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

#ifndef __IOFX_ACTOR_H__
#define __IOFX_ACTOR_H__

#include "NxApex.h"
#include "NxIofxActor.h"
#include "NiInstancedObjectSimulation.h"
#include "NiResourceProvider.h"
#include "ApexInterface.h"
#include "ApexActor.h"
#include "IofxAsset.h"
#include "NiIofxManager.h"
#include "IofxManager.h"
#include "Modifier.h"


namespace physx
{
namespace apex
{

class NiApexScene;
class NxModifier;

namespace iofx
{

class IofxScene;
class ApexRenderVolume;

struct ObjectRange
{
	ObjectRange() : startIndex(0), objectCount(0) {}

	PxU32			startIndex;	    //!< start index for this IOFX actor
	PxU32			objectCount;	//!< count of object instances this frame
};


class IofxActorRenderData;

class IofxActor :
	public NxIofxActor,
	public NxApexResource,
	public ApexActor,
	public ApexResource
{
public:
	IofxActor(NxApexAsset*, IofxScene*, IofxManager&);
	~IofxActor();

	// NxApexActor methods
#if NX_SDK_VERSION_MAJOR == 2
	void				setPhysXScene(NxScene*)	{ }
	NxScene* 			getPhysXScene() const
	{
		return NULL;
	}
#elif NX_SDK_VERSION_MAJOR == 3
	void				setPhysXScene(PxScene*)	{ }
	PxScene* 			getPhysXScene() const
	{
		return NULL;
	}
#endif
	NxApexAsset* getOwner() const
	{
		return NULL;
	}
	NxApexRenderable* 	getRenderable()
	{
		return this;
	}
	NxApexActor*        getNxApexActor()
	{
		return this;
	}
	physx::PxBounds3	getBounds() const
	{
		return ApexRenderable::getBounds();
	}
	const char* 		getIosAssetName() const
	{
		return mMgr.mIosAssetName.c_str();
	}

	void				getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly);
	physx::PxF32		getActivePhysicalLod();
	void				forcePhysicalLod(physx::PxF32 lod);

	// NxApexResource methods
	void				release();
	void			    destroy();

	physx::PxU32	    getListIndex() const
	{
		return m_listIndex;
	}
	void	            setListIndex(NxResourceList& list, physx::PxU32 index)
	{
		m_listIndex = index;
		m_list = &list;
	}

	// NxApexRenderable API
	virtual void		lockRenderResources();
	virtual void		unlockRenderResources();

	virtual void		updateRenderResources(bool rewriteBuffers, void* userRenderData);
	virtual void		dispatchRenderResources(NxUserRenderer&);

	virtual PxF32		getObjectRadius() const
	{
		return mMgr.getObjectRadius();
	}

	virtual PxU32		getObjectCount() const
	{
		return mRenderRange.objectCount;
	}

	virtual PxU32		getVisibleCount() const
	{
		return mRenderVisibleCount;
	}

	bool				prepareRenderResources(IosObjectBaseData* obj);

	NxRenderMeshActor* 			loadRenderMeshActor(physx::PxU32 maxInstanceCount);

	NxApexAsset*                mRenderAsset;
	IofxScene*					mIofxScene;
	IofxManager&                mMgr;
	ApexRenderVolume*           mRenderVolume;

	/* Set by IOFX Manager at creation time, immutable */
	PxU32						mActorClassID;

	PxBounds3                   mResultBounds;
	ObjectRange					mResultRange;
	PxU32						mResultVisibleCount;

	ObjectRange					mRenderRange;
	PxU32						mRenderVisibleCount;

	physx::Array<IofxActorRenderData*> mRenderDataArray;

	PxU32						mSemantics;
	IofxActorRenderData*		mActiveRenderData;

	bool						mDistanceSortingEnabled;

	friend class IofxManager;
	friend class IofxManagerGPU;
};

}
}
} // namespace apex

#endif // __IOFX_ACTOR_H__
