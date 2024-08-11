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

#ifndef __DESTRUCTIBLEPREVIEW_H__
#define __DESTRUCTIBLEPREVIEW_H__

#include "NxApex.h"
#include "ApexInterface.h"
#include "ApexPreview.h"
#include "NxRenderMesh.h"
#include "NxDestructiblePreview.h"

namespace physx
{
namespace apex
{

class NxApexDestructiblePreviewParam;

namespace destructible
{
class DestructiblePreview : public ApexResource, public ApexPreview
{
public:

	DestructibleAsset*		m_asset;
	NxDestructiblePreview*	m_api;

	physx::PxU32			m_chunkDepth;
	physx::PxF32			m_explodeAmount;

	NxRenderMeshActor*		m_renderMeshActors[NxDestructibleActorMeshType::Count];	// Indexed by NxDestructibleActorMeshType::Enum

	physx::Array<NxRenderMeshActor*>	m_instancedChunkRenderMeshActors;	// One per render mesh actor per instanced chunk
	physx::Array<physx::PxU16>			m_instancedActorVisiblePart;

	physx::Array<physx::PxU16>			m_instancedChunkActorMap;	// from instanced chunk instanceInfo index to actor index

	physx::Array<NxUserRenderInstanceBuffer*>	m_chunkInstanceBuffers;
	physx::Array< physx::Array< DestructibleAsset::ChunkInstanceBufferDataElement > >	m_chunkInstanceBufferData;

	bool					m_drawUnexpandedChunksStatically;

	void*					m_userData;

	void				setExplodeView(physx::PxU32 depth, physx::PxF32 explode);

	void				updateRenderResources(bool rewriteBuffers, void* userRenderData);

	NxRenderMeshActor*	getRenderMeshActor() const
	{
		return m_renderMeshActors[(!m_drawUnexpandedChunksStatically || m_explodeAmount != 0.0f) ? NxDestructibleActorMeshType::Skinned : NxDestructibleActorMeshType::Static];
	}

	void				updateRenderResources(void* userRenderData);
	void				dispatchRenderResources(NxUserRenderer& renderer);

	// ApexPreview methods
	void				setPose(const physx::PxMat44& pose);
	NxApexRenderable*	getRenderable()
	{
		return DYNAMIC_CAST(NxApexRenderable*)(m_api);
	}
	void				release();
	void				destroy();

	DestructiblePreview(NxDestructiblePreview* _api, DestructibleAsset& _asset, const NxParameterized::Interface* params);
	virtual				~DestructiblePreview();

protected:
	void					setChunkVisibility(physx::PxU16 index, bool visibility)
	{
		PX_ASSERT((physx::PxI32)index < m_asset->mParams->chunks.arraySizes[0]);
		if (visibility)
		{
			mVisibleChunks.use(index);
		}
		else
		{
			mVisibleChunks.free(index);
		}
		DestructibleAssetParametersNS::Chunk_Type& sourceChunk = m_asset->mParams->chunks.buf[index];
		if ((sourceChunk.flags & DestructibleAsset::Instanced) == 0)
		{
			// Not instanced - need to choose the static or dynamic mesh, and set visibility for the render mesh actor
			const NxDestructibleActorMeshType::Enum typeN = (m_explodeAmount != 0.0f || !m_drawUnexpandedChunksStatically) ?
					NxDestructibleActorMeshType::Skinned : NxDestructibleActorMeshType::Static;
			m_renderMeshActors[typeN]->setVisibility(visibility, sourceChunk.meshPartIndex);
		}
	}


	void				setInstancedChunkCount(physx::PxU32 count);

	NxIndexBank<physx::PxU16>	mVisibleChunks;
};

}
}
} // end namespace physx::apex

#endif // __DESTRUCTIBLEACTOR_H__
