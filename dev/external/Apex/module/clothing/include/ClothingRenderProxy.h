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

#ifndef CLOTHING_RENDER_PROXY_H
#define CLOTHING_RENDER_PROXY_H

#include "NxClothingRenderProxy.h"
#include "PxMat44.h"
#include "PxBounds3.h"
#include "PsHashMap.h"
#include "ApexString.h"

namespace physx
{
namespace apex
{
class NiApexRenderMeshActor;
class NiApexRenderMeshAsset;

namespace clothing
{
class ClothingActorParam;
class ClothingScene;


class ClothingRenderProxy : public NxClothingRenderProxy, public UserAllocated
{
public:

	ClothingRenderProxy(NiApexRenderMeshAsset* rma, bool useFallbackSkinning, bool useCustomVertexBuffer, const physx::shdfnd::HashMap<PxU32, ApexSimpleString>& overrideMaterials, const PxVec3* morphTargetNewPositions, const PxU32* morphTargetVertexOffsets, ClothingScene* scene);
	virtual ~ClothingRenderProxy();

	// from NxApexInterface
	virtual void release();

	// from NxApexRenderable
	virtual void dispatchRenderResources(NxUserRenderer& api);
	virtual PxBounds3 getBounds() const
	{
		return mBounds;
	}
	void setBounds(const PxBounds3& bounds)
	{
		mBounds = bounds;
	}

	// from NxApexRenderDataProvider.h
	virtual void lockRenderResources();
	virtual void unlockRenderResources();
	virtual void updateRenderResources(bool rewriteBuffers = false, void* userRenderData = 0);

	void setPose(const PxMat44& pose)
	{
		mPose = pose;
	}

	// from NxClothingRenderProxy.h
	virtual bool hasSimulatedData() const;

	NiApexRenderMeshActor* getRenderMeshActor();
	NiApexRenderMeshAsset* getRenderMeshAsset();

	bool usesFallbackSkinning() const
	{
		return mUseFallbackSkinning;
	}

	bool usesCustomVertexBuffer() const
	{
		return renderingDataPosition != NULL;
	}

	const PxVec3* getMorphTargetBuffer() const
	{
		return mMorphTargetNewPositions;
	}

	void setOverrideMaterial(PxU32 i, const char* overrideMaterialName);
	bool overrideMaterialsEqual(const physx::shdfnd::HashMap<PxU32, ApexSimpleString>& overrideMaterials);

	PxU32 getTimeInPool()  const;
	void setTimeInPool(PxU32 time);

	void notifyAssetRelease();

	PxVec3* renderingDataPosition;
	PxVec3* renderingDataNormal;
	PxVec4* renderingDataTangent;

private:
	NiApexRenderMeshActor* createRenderMeshActor(NiApexRenderMeshAsset* renderMeshAsset, ClothingActorParam* actorDesc);

	PxBounds3 mBounds;
	PxMat44 mPose;

	NiApexRenderMeshActor* mRenderMeshActor;
	NiApexRenderMeshAsset* mRenderMeshAsset;

	ClothingScene* mScene;

	bool mUseFallbackSkinning;
	HashMap<PxU32, ApexSimpleString> mOverrideMaterials;
	const PxVec3* mMorphTargetNewPositions; // just to compare, only read it in constructor (it may be released)

	PxU32 mTimeInPool;

	Mutex mRMALock;
};

}
} // namespace apex
} // namespace physx

#endif // CLOTHING_RENDER_PROXY_H
