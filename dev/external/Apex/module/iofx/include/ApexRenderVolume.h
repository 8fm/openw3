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

#ifndef __APEX_RENDER_VOLUME_H__
#define __APEX_RENDER_VOLUME_H__

#include "NxApex.h"
#include "NxApexRenderVolume.h"
#include "PsArray.h"
#include "ApexInterface.h"
#include "ApexRenderable.h"

namespace physx
{
namespace apex
{

class NxIofxAsset;
class NxIofxActor;

namespace iofx
{
class IofxScene;

class ApexRenderVolume : public NxApexRenderVolume, public ApexRenderable, public NxApexResource, public ApexResource
{
public:
	ApexRenderVolume(IofxScene& scene, const PxBounds3& b, PxU32 priority, bool allIofx);
	~ApexRenderVolume();

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
	void				lockRenderResources()
	{
		ApexRenderable::renderDataLock();
	}
	void				unlockRenderResources()
	{
		ApexRenderable::renderDataUnLock();
	}
	void				updateRenderResources(bool rewriteBuffers, void* userRenderData);
	void				dispatchRenderResources(NxUserRenderer&);

	void				setOwnershipBounds(const PxBounds3& b)
	{
		ApexRenderable::mRenderBounds = b;
	}
	PxBounds3			getOwnershipBounds(void) const
	{
		return ApexRenderable::getBounds();
	}
	PxBounds3			getBounds() const;

	// methods for use by IOS or IOFX actor
	bool				addIofxActor(NxIofxActor& iofx);
	bool				removeIofxActor(const NxIofxActor& iofx);

	bool				addIofxAsset(NxIofxAsset& iofx);
	void				setPosition(const PxVec3& pos);

	bool				getAffectsAllIofx() const
	{
		return mAllIofx;
	}
	NxIofxActor* const* getIofxActorList(PxU32& count) const
	{
		count = mIofxActors.size();
		return count ? &mIofxActors.front() : NULL;
	}
	NxIofxAsset* const* getIofxAssetList(PxU32& count) const
	{
		count = mIofxAssets.size();
		return count ? &mIofxAssets.front() : NULL;
	}
	PxVec3				getPosition() const
	{
		return mRenderBounds.getCenter();
	}
	PxU32				getPriority() const
	{
		return mPriority;
	}
	bool				affectsIofxAsset(const NxIofxAsset& iofx) const;

protected:
	// bounds is stored in ApexRenderable::mRenderBounds
	PxU32						 mPriority;
	bool						 mAllIofx;
	bool                         mPendingDelete;
	IofxScene&                   mScene;
	physx::Array<NxIofxAsset*>	 mIofxAssets;
	physx::Array<NxIofxActor*>	mIofxActors;
};

}
}
} // namespace apex

#endif // __APEX_RENDER_VOLUME_H__
