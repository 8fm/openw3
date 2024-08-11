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

#include "NxApexRenderDebug.h"
#include "NxGroundEmitterPreview.h"
#include "GroundEmitterAssetPreview.h"
#include "ApexPreview.h"
#include "foundation/PxFoundation.h"
#include "PsShare.h"

namespace physx
{
namespace apex
{
namespace emitter
{

void GroundEmitterAssetPreview::drawEmitterPreview(void)
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (!mApexRenderDebug)
	{
		return;
	}
	physx::PxVec3 tmpUpDirection(0.0f, 1.0f, 0.0f);

	//asset preview init
	if (mGroupID == 0)
	{
		mGroupID = mApexRenderDebug->beginDrawGroup(physx::PxMat44().createIdentity());
		// Cylinder that describes the refresh radius, upDirection, and spawnHeight
		mApexRenderDebug->setCurrentColor(mApexRenderDebug->getDebugColor(physx::DebugColors::Green));
		mApexRenderDebug->debugCylinder(
		    physx::PxVec3(0.0f),
		    tmpUpDirection * (mAsset->getSpawnHeight() + mAsset->getRaycastHeight() + 0.01f),
		    mAsset->getRadius());

		// Ray that describes the raycast spawn height
		mApexRenderDebug->setCurrentColor(mApexRenderDebug->getDebugColor(physx::DebugColors::Yellow), mApexRenderDebug->getDebugColor(physx::DebugColors::Yellow));
		mApexRenderDebug->setCurrentArrowSize(mScale);
		mApexRenderDebug->debugRay(tmpUpDirection * mAsset->getRaycastHeight(), physx::PxVec3(0.0f));
		mApexRenderDebug->endDrawGroup();
	}

	//asset preview set pose
	physx::PxMat44 groupPose = mPose;
	mApexRenderDebug->setDrawGroupPose(mGroupID, groupPose);

	//asset preview set visibility
	mApexRenderDebug->setDrawGroupVisible(mGroupID, true);
#endif
}

void GroundEmitterAssetPreview::destroy(void)
{
	if (mApexRenderDebug)
	{
		mApexRenderDebug->reset();
		mApexRenderDebug->release();
	}
	mApexRenderDebug = NULL;
	ApexPreview::destroy();
	delete this;
}

GroundEmitterAssetPreview::~GroundEmitterAssetPreview(void)
{
}

void GroundEmitterAssetPreview::setPose(const physx::PxMat44& pose)
{
	mPose = pose;
	drawEmitterPreview();
}

void GroundEmitterAssetPreview::setScale(physx::PxF32 scale)
{
	mScale = scale;
	drawEmitterPreview();
}

const physx::PxMat44 GroundEmitterAssetPreview::getPose() const
{
	return(mPose);
}

// from NxApexRenderDataProvider
void GroundEmitterAssetPreview::lockRenderResources(void)
{
	ApexRenderable::renderDataLock();
}

void GroundEmitterAssetPreview::unlockRenderResources(void)
{
	ApexRenderable::renderDataUnLock();
}

void GroundEmitterAssetPreview::updateRenderResources(bool /*rewriteBuffers*/, void* /*userRenderData*/)
{
	mApexRenderDebug->updateRenderResources();
}

// from NxApexRenderable.h
void GroundEmitterAssetPreview::dispatchRenderResources(NxUserRenderer& renderer)
{
	mApexRenderDebug->dispatchRenderResources(renderer);
}

physx::PxBounds3 GroundEmitterAssetPreview::getBounds(void) const
{
	return mApexRenderDebug->getBounds();
}

void GroundEmitterAssetPreview::release(void)
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	const_cast<GroundEmitterAsset*>(mAsset)->releaseEmitterPreview(*this);
}

}
}
} // namespace physx::apex
