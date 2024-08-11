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
#include "NiApexSDK.h"
#include "NiApexScene.h"
#include "NxIofxActor.h"
#include "IofxActor.h"
#include "IofxScene.h"
#include "ApexRenderVolume.h"
#include "IosObjectData.h"
#include "IofxRenderData.h"

#include "ModuleIofx.h"

namespace physx
{
namespace apex
{
namespace iofx
{

IofxActor::IofxActor(NxApexAsset* renderAsset, IofxScene* iscene, IofxManager& mgr)
	: mRenderAsset(renderAsset)
	, mIofxScene(iscene)
	, mMgr(mgr)
	, mRenderVolume(NULL) // IOS will set this after creation
	, mSemantics(0)
	, mActiveRenderData(NULL)
{
	//asset.add(*this);

	mResultBounds.setEmpty();
	mResultRange.startIndex = 0;
	mResultRange.objectCount = 0;
	mResultVisibleCount = 0;

	addSelfToContext(*iscene->mApexScene->getApexContext());    // Add self to ApexScene
	addSelfToContext(*iscene);							      // Add self to IofxScene
}

IofxActor::~IofxActor()
{
}

void IofxActor::getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly)
{
	PX_UNUSED(min);
	PX_UNUSED(max);
	PX_UNUSED(intOnly);
	APEX_INVALID_OPERATION("not implemented");
}


physx::PxF32 IofxActor::getActivePhysicalLod()
{
	APEX_INVALID_OPERATION("NxBasicIosActor does not support this operation");
	return -1.0f;
}


void IofxActor::forcePhysicalLod(physx::PxF32 lod)
{
	PX_UNUSED(lod);
	APEX_INVALID_OPERATION("not implemented");
}

void IofxActor::release()
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	destroy();
}


void IofxActor::destroy()
{
	if (mRenderVolume)
	{
		mRenderVolume->removeIofxActor(*this);
	}

	// Removes self from scenes and IOFX manager
	// should be called after mRenderVolume->removeIofxActor to avoid dead-lock!!!
	ApexActor::destroy();

	for (PxU32 i = 0 ; i < mRenderDataArray.size() ; i++)
	{
		IofxActorRenderData* renderData = mRenderDataArray[i];
		PX_DELETE(renderData);
	}
	mRenderDataArray.clear();

	delete this;
}

void IofxActor::lockRenderResources()
{
	return ApexRenderable::renderDataLock();
}

void IofxActor::unlockRenderResources()
{
	return ApexRenderable::renderDataUnLock();
}

void IofxActor::updateRenderResources(bool rewriteBuffers, void* userRenderData)
{
	if (mActiveRenderData != NULL)
	{
		mActiveRenderData->updateRenderResources(rewriteBuffers, userRenderData);
	}
}

void IofxActor::dispatchRenderResources(NxUserRenderer& renderer)
{
	if (mActiveRenderData != NULL)
	{
		mActiveRenderData->dispatchRenderResources(renderer);
	}
}

bool IofxActor::prepareRenderResources(IosObjectBaseData* obj)
{
	mRenderBounds = mResultBounds;
	mRenderRange = mResultRange;
	mRenderVisibleCount = mResultVisibleCount;

	if (mRenderRange.objectCount > 0 && obj->renderData->checkSemantics(mSemantics))
	{
		const PxU32 instanceID = obj->renderData->getInstanceID();
		if (mRenderDataArray.size() <= instanceID)
		{
			mRenderDataArray.resize(instanceID + 1, NULL);
		}
		mActiveRenderData = mRenderDataArray[instanceID];
		if (mActiveRenderData == NULL)
		{
			if (mMgr.mIsMesh)
			{
				NxRenderMeshActor* renderMeshActor = loadRenderMeshActor(obj->maxObjectCount);
				mActiveRenderData = PX_NEW(IofxActorRenderDataMesh)(this, renderMeshActor);
			}
			else
			{
				NxApexAsset* spriteMaterialAsset = mRenderAsset;
				mActiveRenderData = PX_NEW(IofxActorRenderDataSprite)(this, spriteMaterialAsset);
			}
			mRenderDataArray[instanceID] = mActiveRenderData;
		}
		mActiveRenderData->setSharedRenderData(obj->renderData);
		return true;
	}
	else
	{
		mRenderBounds.setEmpty();
		mRenderRange.objectCount = 0;
		mRenderVisibleCount = 0;

		mActiveRenderData = NULL;
		return false;
	}
}

NxRenderMeshActor* IofxActor::loadRenderMeshActor(physx::PxU32 maxInstanceCount)
{
	NxRenderMeshActor* rmactor = NULL;

	NxRenderMeshActorDesc renderableMeshDesc;
	renderableMeshDesc.maxInstanceCount = maxInstanceCount;
	renderableMeshDesc.keepVisibleBonesPacked = false;

	NxRenderMeshAsset* meshAsset = static_cast<NxRenderMeshAsset*>(mRenderAsset);
	if (meshAsset)
	{
		rmactor = meshAsset->createActor(renderableMeshDesc);

		ApexActor* aa = NiGetApexSDK()->getApexActor(rmactor);
		if (aa)
		{
			aa->addSelfToContext(*this);
		}
	}
	return rmactor;
}

}
}
} // namespace physx::apex
