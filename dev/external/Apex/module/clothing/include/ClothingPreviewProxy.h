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

#ifndef CLOTHING_PREVIEW_PROXY_H
#define CLOTHING_PREVIEW_PROXY_H

#include "NxClothingPreview.h"
#include "ClothingActor.h"
#include "NxApexRenderable.h"

namespace physx
{
namespace apex
{
namespace clothing
{

class ClothingPreviewProxy : public NxClothingPreview, public physx::UserAllocated, public NxApexResource
{
	ClothingActor impl;

public:
#pragma warning( disable : 4355 ) // disable warning about this pointer in argument list
	ClothingPreviewProxy(const NxParameterized::Interface& desc, ClothingAsset* asset, NxResourceList* list) :
		impl(desc, NULL, this, asset, NULL)
	{
		list->add(*this);
	}

	virtual void release()
	{
		impl.release();
	}

	virtual physx::PxU32 getListIndex() const
	{
		return impl.m_listIndex;
	}

	virtual void setListIndex(class NxResourceList& list, physx::PxU32 index)
	{
		impl.m_list = &list;
		impl.m_listIndex = index;
	}

	virtual void setPose(const physx::PxMat44& pose)
	{
		impl.updateState(pose, NULL, 0, 0, ClothingTeleportMode::Continuous);
	}

	virtual const physx::PxMat44 getPose() const
	{
		return impl.getGlobalPose_DEPRECATED();
	}

	virtual void lockRenderResources()
	{
		impl.lockRenderResources();
	}

	virtual void unlockRenderResources()
	{
		impl.unlockRenderResources();
	}

	virtual void updateRenderResources(bool rewriteBuffers = false, void* userRenderData = 0)
	{
		NxApexRenderable* renderable = impl.getRenderable();
		if (renderable != NULL)
		{
			renderable->updateRenderResources(rewriteBuffers, userRenderData);
		}
	}

	virtual void dispatchRenderResources(NxUserRenderer& renderer)
	{
		NxApexRenderable* renderable = impl.getRenderable();
		if (renderable != NULL)
		{
			renderable->dispatchRenderResources(renderer);
		}
	}

	virtual physx::PxBounds3 getBounds() const
	{
		return impl.getBounds();
	}


	virtual void updateState(const physx::PxMat44& globalPose, const physx::PxMat44* newBoneMatrices, physx::PxU32 boneMatricesByteStride, physx::PxU32 numBoneMatrices)
	{
		impl.updateState(globalPose, newBoneMatrices, boneMatricesByteStride, numBoneMatrices, ClothingTeleportMode::Continuous);
	}

	void destroy()
	{
		impl.destroy();
		delete this;
	}

	virtual ~ClothingPreviewProxy() {}
};

}
} // namespace apex
} // namespace physx

#endif // CLOTHING_PREVIEW_PROXY_H
