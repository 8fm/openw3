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

#include "NxDestructibleRenderable.h"
#include "NxRenderMeshActor.h"
#include "DestructibleActor.h"
#include "ModulePerfScope.h"

namespace physx
{
namespace apex
{
namespace destructible
{

DestructibleRenderable::DestructibleRenderable(NxRenderMeshActor* renderMeshActors[NxDestructibleActorMeshType::Count], DestructibleAsset* asset, PxI32 listIndex)
: mAsset(asset)
, mListIndex(listIndex)
, mRefCount(1)	// Ref count initialized to 1, assuming that whoever calls this constructor will store a reference
{
	for (physx::PxU32 i = 0; i < NxDestructibleActorMeshType::Count; ++i)
	{
		mRenderMeshActors[i] = renderMeshActors[i];
	}
}

DestructibleRenderable::~DestructibleRenderable()
{
	for (physx::PxU32 i = 0; i < NxDestructibleActorMeshType::Count; ++i)
	{
		if (mRenderMeshActors[i])
		{
			mRenderMeshActors[i]->release();
			mRenderMeshActors[i] = NULL;
		}
	}
}

void DestructibleRenderable::release()
{
	bool triggerDelete = false;
	lockRenderResources();
	if (mRefCount > 0)
	{
		triggerDelete = !(--mRefCount);
	}
	unlockRenderResources();
	if (triggerDelete)
	{
		delete this;
	}
}

void DestructibleRenderable::updateRenderResources(bool rewriteBuffers, void* userRenderData)
{
	PX_PROFILER_PERF_SCOPE("DestructibleRenderableUpdateRenderResources");

	for (PxU32 typeN = 0; typeN < NxDestructibleActorMeshType::Count; ++typeN)
	{
		NiApexRenderMeshActor* renderMeshActor = (NiApexRenderMeshActor*)mRenderMeshActors[typeN];
		if (renderMeshActor != NULL)
		{
			renderMeshActor->updateRenderResources((typeN == NxDestructibleActorMeshType::Skinned), rewriteBuffers, userRenderData);
		}
	}

	// Render instanced meshes
	if (mAsset->m_instancingRepresentativeActorIndex == -1)
	{
		mAsset->m_instancingRepresentativeActorIndex = (physx::PxI32)mListIndex;	// using this actor as our representative
	}
	if ((physx::PxI32)mListIndex == mAsset->m_instancingRepresentativeActorIndex)	// doing it this way, in case (for some reason) someone wants to call this fn twice per frame
	{
		for (physx::PxU32 i = 0; i < mAsset->m_instancedChunkRenderMeshActors.size(); ++i)
		{
			PX_ASSERT(i < mAsset->m_chunkInstanceBufferData.size());
			NiApexRenderMeshActor* renderMeshActor = (NiApexRenderMeshActor*)mAsset->m_instancedChunkRenderMeshActors[i];
			if (renderMeshActor != NULL)
			{
				NxApexRenderInstanceBufferData data;
				const physx::PxU32 instanceBufferSize = mAsset->m_chunkInstanceBufferData[i].size();

				if (instanceBufferSize > 0)
				{
					mAsset->m_chunkInstanceBuffers[i]->writeBuffer(&mAsset->m_chunkInstanceBufferData[i][0], 0, instanceBufferSize);
				}

				renderMeshActor->setInstanceBufferRange(0, instanceBufferSize);
				renderMeshActor->updateRenderResources(false, rewriteBuffers, userRenderData);
			}
		}

		for (physx::PxU32 i = 0; i < mAsset->m_scatterMeshInstanceInfo.size(); ++i)
		{
			DestructibleAsset::ScatterMeshInstanceInfo& info = mAsset->m_scatterMeshInstanceInfo[i];
			NiApexRenderMeshActor* renderMeshActor = (NiApexRenderMeshActor*)info.m_actor;
			if (renderMeshActor != NULL)
			{
				NxApexRenderInstanceBufferData data;
				physx::Array<DestructibleAsset::ScatterInstanceBufferDataElement>& instanceBufferData = info.m_instanceBufferData;
				const physx::PxU32 instanceBufferSize = instanceBufferData.size();

				if (info.m_instanceBuffer != NULL && instanceBufferSize > 0)
				{
					info.m_instanceBuffer->writeBuffer(&instanceBufferData[0], 0, instanceBufferSize);
				}
				renderMeshActor->setInstanceBufferRange(0, instanceBufferSize);
				renderMeshActor->updateRenderResources(false, rewriteBuffers, userRenderData);
			}
		}
	}

#if APEX_RUNTIME_FRACTURE
	mRTrenderable.updateRenderResources(rewriteBuffers,userData);
#endif
}

void DestructibleRenderable::dispatchRenderResources(NxUserRenderer& renderer)
{
	PX_PROFILER_PERF_SCOPE("DestructibleRenderableDispatchRenderResources");

	for (PxU32 typeN = 0; typeN < NxDestructibleActorMeshType::Count; ++typeN)
	{
		NxRenderMeshActor* renderMeshActor = mRenderMeshActors[typeN];
		if (renderMeshActor != NULL)
		{
			renderMeshActor->dispatchRenderResources(renderer);
		}
	}

	// Render instanced meshes
	if ((physx::PxI32)mListIndex == mAsset->m_instancingRepresentativeActorIndex)
	{
		for (physx::PxU32 i = 0; i < mAsset->m_instancedChunkRenderMeshActors.size(); ++i)
		{
			PX_ASSERT(i < mAsset->m_chunkInstanceBufferData.size());
			if (mAsset->m_instancedChunkRenderMeshActors[i] != NULL)
			{
				mAsset->m_instancedChunkRenderMeshActors[i]->dispatchRenderResources(renderer);
			}
		}

		for (physx::PxU32 i = 0; i < mAsset->m_scatterMeshInstanceInfo.size(); ++i)
		{
			DestructibleAsset::ScatterMeshInstanceInfo& scatterMeshInstanceInfo = mAsset->m_scatterMeshInstanceInfo[i];
			if (scatterMeshInstanceInfo.m_actor != NULL)
			{
				scatterMeshInstanceInfo.m_actor->dispatchRenderResources(renderer);
			}
		}
	}
#if APEX_RUNTIME_FRACTURE
	mRTrenderable.dispatchRenderResources(renderer);
#endif
}

DestructibleRenderable* DestructibleRenderable::incrementReferenceCount()
{
	DestructibleRenderable* returnValue = NULL;
	lockRenderResources();
	if (mRefCount > 0)
	{
		++mRefCount;
		returnValue = this;
	}
	unlockRenderResources();
	return returnValue;
}

}
}
} // end namespace physx::apex
