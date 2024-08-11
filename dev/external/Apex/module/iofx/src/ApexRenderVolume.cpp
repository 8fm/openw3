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

#include "NxApex.h"

#include "ApexRenderVolume.h"
#include "ModuleIofx.h"
#include "IofxAsset.h"
#include "IofxActor.h"
#include "IofxScene.h"
#include "NiApexScene.h"

#include "PsArray.h"

namespace physx
{
namespace apex
{
namespace iofx
{

ApexRenderVolume::ApexRenderVolume(IofxScene& scene, const PxBounds3& b, PxU32 priority, bool allIofx)
	: mPriority(priority)
	, mAllIofx(allIofx)
	, mPendingDelete(false)
	, mScene(scene)
{
	setOwnershipBounds(b);

	mScene.mAddedRenderVolumesLock.lock();
	mScene.mAddedRenderVolumes.pushBack(this);
	mScene.mAddedRenderVolumesLock.unlock();
}

ApexRenderVolume::~ApexRenderVolume()
{
	ApexRenderable::renderDataLock();
	while (mIofxActors.size())
	{
		NxIofxActor* iofx = mIofxActors.popBack();
		iofx->release();
	}
	ApexRenderable::renderDataUnLock();
}

void ApexRenderVolume::destroy()
{
	if (!mPendingDelete)
	{
		mPendingDelete = true;

		mScene.mDeletedRenderVolumesLock.lock();
		mScene.mDeletedRenderVolumes.pushBack(this);
		mScene.mDeletedRenderVolumesLock.unlock();
	}
}

void ApexRenderVolume::release()
{
	if (!mPendingDelete)
	{
		mScene.mModule->releaseRenderVolume(*this);
	}
}

bool ApexRenderVolume::addIofxAsset(NxIofxAsset& iofx)
{
	if (mAllIofx || mPendingDelete)
	{
		return false;
	}

	ApexRenderable::renderDataLock();
	mIofxAssets.pushBack(&iofx);
	ApexRenderable::renderDataUnLock();
	return true;
}

bool ApexRenderVolume::addIofxActor(NxIofxActor& iofx)
{
	if (mPendingDelete)
	{
		return false;
	}

	ApexRenderable::renderDataLock();
	mIofxActors.pushBack(&iofx);
	ApexRenderable::renderDataUnLock();
	return true;
}

bool ApexRenderVolume::removeIofxActor(const NxIofxActor& iofx)
{
	ApexRenderable::renderDataLock();
	for (PxU32 i = 0 ; i < mIofxActors.size() ; i++)
	{
		if (mIofxActors[ i ] == &iofx)
		{
			mIofxActors.replaceWithLast(i);
			ApexRenderable::renderDataUnLock();
			return true;
		}
	}

	ApexRenderable::renderDataUnLock();
	return false;
}

void ApexRenderVolume::setPosition(const PxVec3& pos)
{
	physx::PxVec3 ext = mRenderBounds.getExtents();
	ApexRenderable::mRenderBounds = physx::PxBounds3(pos - ext, pos + ext);
}

PxBounds3 ApexRenderVolume::getBounds() const
{
	if (mPendingDelete)
	{
		return PxBounds3::empty();
	}

	PxBounds3 b = PxBounds3::empty();
	physx::Array<NxIofxActor*>::ConstIterator i;
	for (i = mIofxActors.begin() ; i != mIofxActors.end() ; i++)
	{
		(*i)->lockRenderResources();
		b.include((*i)->getBounds());
		(*i)->unlockRenderResources();
	}

	return b;
}

void ApexRenderVolume::updateRenderResources(bool rewriteBuffers, void* userRenderData)
{
	if (mPendingDelete)
	{
		return;
	}

	physx::Array<NxIofxActor*>::Iterator i;
	for (i = mIofxActors.begin() ; i != mIofxActors.end() ; i++)
	{
		(*i)->lockRenderResources();
		(*i)->updateRenderResources(rewriteBuffers, userRenderData);
		(*i)->unlockRenderResources();
	}
}


void ApexRenderVolume::dispatchRenderResources(NxUserRenderer& renderer)
{
	if (mPendingDelete)
	{
		return;
	}

	physx::Array<NxIofxActor*>::Iterator i;
	for (i = mIofxActors.begin() ; i != mIofxActors.end() ; i++)
	{
		(*i)->lockRenderResources();
		(*i)->dispatchRenderResources(renderer);
		(*i)->unlockRenderResources();
	}
}

// Callers must acquire render lock for thread safety
bool ApexRenderVolume::affectsIofxAsset(const NxIofxAsset& iofx) const
{
	if (mPendingDelete)
	{
		return false;
	}

	if (mAllIofx)
	{
		return true;
	}

	physx::Array<NxIofxAsset*>::ConstIterator i;
	for (i = mIofxAssets.begin() ; i != mIofxAssets.end() ; i++)
	{
		if (&iofx == *i)
		{
			return true;
		}
	}

	return false;
}

}
}
} // namespace physx::apex
