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

#ifndef __DESTRUCTIBLEPREVIEW_PROXY_H__
#define __DESTRUCTIBLEPREVIEW_PROXY_H__

#include "NxApex.h"
#include "NxDestructiblePreview.h"
#include "DestructiblePreview.h"
#include "PsUserAllocated.h"

namespace physx
{
namespace apex
{
namespace destructible
{

class DestructiblePreviewProxy : public NxDestructiblePreview, public NxApexResource, public physx::UserAllocated
{
public:
	DestructiblePreview impl;

#pragma warning(disable : 4355) // disable warning about this pointer in argument list
	DestructiblePreviewProxy(DestructibleAsset& asset, NxResourceList& list, const NxParameterized::Interface* params)
		: impl(this, asset, params)
	{
		list.add(*this);
	}

	virtual ~DestructiblePreviewProxy()
	{
	}

	// NxApexAssetPreview methods
	virtual void setPose(const physx::PxMat44& pose)
	{
		impl.setPose(pose);
	}

	virtual const physx::PxMat44 getPose() const
	{
		return impl.getPose();
	}

	// NxDestructiblePreview methods

	virtual const NxRenderMeshActor* getRenderMeshActor() const
	{
		return const_cast<const NxRenderMeshActor*>(impl.getRenderMeshActor());
	}

	virtual void setExplodeView(physx::PxU32 depth, physx::PxF32 explode)
	{
		return impl.setExplodeView(depth, explode);
	}

	// NxApexInterface methods
	virtual void release()
	{
		impl.release();
		delete this;
	}
	virtual void destroy()
	{
		impl.destroy();
	}

	// NxApexRenderable methods
	virtual void updateRenderResources(bool rewriteBuffers, void* userRenderData)
	{
		impl.updateRenderResources(rewriteBuffers, userRenderData);
	}
	virtual void dispatchRenderResources(NxUserRenderer& api)
	{
		impl.dispatchRenderResources(api);
	}
	virtual physx::PxBounds3 getBounds() const
	{
		return impl.getBounds();
	}
	virtual void lockRenderResources()
	{
		impl.ApexRenderable::renderDataLock();
	}
	virtual void unlockRenderResources()
	{
		impl.ApexRenderable::renderDataUnLock();
	}

	// NxApexResource methods
	virtual void	setListIndex(NxResourceList& list, physx::PxU32 index)
	{
		impl.m_listIndex = index;
		impl.m_list = &list;
	}
	virtual physx::PxU32	getListIndex() const
	{
		return impl.m_listIndex;
	}
};

}
}
} // end namespace physx::apex

#endif // __DESTRUCTIBLEPREVIEW_PROXY_H__
