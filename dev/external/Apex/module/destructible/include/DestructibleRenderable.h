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

#ifndef __DESTRUCTIBLERENDERABLE_H__
#define __DESTRUCTIBLERENDERABLE_H__

#include "NxApex.h"
#include "NxDestructibleActor.h"
#include "NxDestructibleRenderable.h"
#include "ApexInterface.h"
#include "ApexActor.h"
#if APEX_RUNTIME_FRACTURE
#include "Renderable.h"
#endif

namespace physx
{
namespace apex
{

class NxRenderMeshActor;

namespace destructible
{

class DestructibleActor;

class DestructibleRenderable : public NxDestructibleRenderable, public ApexRenderable, public physx::UserAllocated
{
public:
								DestructibleRenderable(NxRenderMeshActor* renderMeshActors[NxDestructibleActorMeshType::Count], DestructibleAsset* asset, PxI32 listIndex);
								~DestructibleRenderable();

	// Begin NxDestructibleRenderable methods
	virtual NxRenderMeshActor*	getRenderMeshActor(NxDestructibleActorMeshType::Enum type = NxDestructibleActorMeshType::Skinned) const
	{
		return (physx::PxU32)type < NxDestructibleActorMeshType::Count ? mRenderMeshActors[type] : NULL;
	}

	virtual void				release();
	// End NxDestructibleRenderable methods

	// Begin NxApexRenderable methods
	virtual	void				updateRenderResources(bool rewriteBuffers, void* userRenderData);

	virtual	void				dispatchRenderResources(NxUserRenderer& api);

	virtual	PxBounds3			getBounds() const
	{
		PxBounds3 bounds = ApexRenderable::getBounds();
#if APEX_RUNTIME_FRACTURE
		bounds.include(mRTrenderable.getBounds());
#endif
		return bounds;
	}

	virtual	void				lockRenderResources()
	{
		ApexRenderable::renderDataLock();
	}

	virtual	void				unlockRenderResources()
	{
		ApexRenderable::renderDataUnLock();
	}
	// End NxApexRenderable methods

	// Begin DestructibleRenderable methods
	// Returns this if successful, NULL otherwise
	DestructibleRenderable*		incrementReferenceCount();

	PxI32						getReferenceCount()
	{
		return mRefCount;
	}

	void						setBounds(const physx::PxBounds3& bounds)
	{
		mRenderBounds = bounds;
	}
	// End DestructibleRenderable methods

#if APEX_RUNTIME_FRACTURE
	::physx::fracture::Renderable& getRTrenderable() { return mRTrenderable; }
#endif

private:

	NxRenderMeshActor*	mRenderMeshActors[NxDestructibleActorMeshType::Count];	// Indexed by NxDestructibleActorMeshType::Enum
	DestructibleAsset*	mAsset;
	PxI32				mListIndex;
	volatile PxI32		mRefCount;
#if APEX_RUNTIME_FRACTURE
	::physx::fracture::Renderable mRTrenderable;
#endif
};

}
}
} // end namespace physx::apex

#endif // __DESTRUCTIBLERENDERABLE_H__
