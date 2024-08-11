/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderSwarmBatcher.h"
#include "renderMeshBatcher.h"
#include "renderSkinningData.h"
#include "renderSceneBatchers.h"
#include "renderProxySwarm.h"
#include "renderTexture.h"
#include "renderSwarmAnimationManager.h"
#include "../engine/renderFragment.h"
#include "../engine/renderSwarmData.h"
#include "renderInterface.h"
#include "renderSkinManager.h"

CRenderSwarmBatcher::CRenderSwarmBatcher()
	: m_instancesDataElemOffset( 0 )
{}

CRenderSwarmBatcher::~CRenderSwarmBatcher()
{
}

void CRenderSwarmBatcher::RenderMeshes( const CRenderFrameInfo& info, const RenderingContext& context, const TDynArray< CRenderElement_MeshChunk* >& batchList, Uint32 renderFlags, class MeshDrawingStats& outMeshStats )
{
	if( batchList.Size() == 0 )
	{
		return;
	}

	PC_SCOPE_RENDER_LVL1( SwarmBatcher_Render );

	// not supported (yet?)
	const bool isDuringShadowsRender = 0 != ( renderFlags & RMBF_Shadowmap );
	if( isDuringShadowsRender )
	{
		return;
	}

	// Assure that draw context won't change in this function
	const CGpuApiScopedDrawContext drawContextOriginalRestore;
	const CGpuApiScopedTwoSidedRender scopedForcedTwoSided;

	// Reset local to world matrix
	GetRenderer()->GetStateManager().SetLocalToWorld( nullptr );

	// Construct list
	CRenderElement_MeshChunk* list = nullptr;
	for ( Uint32 i=0; i<batchList.Size(); i++ )
	{
		CRenderElement_MeshChunk* elem = batchList[ i ];
		elem->SetBatchNext( list );
		list = elem;
	}

	CRenderElement_MeshChunk* fragList = nullptr;
	CRenderMaterial* batchMaterial = nullptr;
	CRenderMaterialParameters* batchMaterialParams = nullptr;
	CRenderMesh* batchMesh = nullptr;
	EMaterialVertexFactory batchVertexFactory;
	Uint32 batchChunkIndex = 0;

	// Group by material
	CRenderBatchByMaterialParams< CRenderElement_MeshChunk > byMaterial( batchList );
	while ( byMaterial.Pop( batchMaterialParams, batchMaterial, fragList ) )
	{
		// Group by mesh
		CRenderBatchByMesh byMesh( fragList );
		while ( byMesh.Pop( batchMesh, fragList ) )
		{
			// Group by vertex type ( vertex shader )
			CRenderBatchByMeshVertexFactory byFactory( fragList );
			while ( byFactory.Pop( batchVertexFactory, fragList ) )
			{
				// Group by mesh chunk
				Uint32 fragCount = 0;
				CRenderBatchByMeshChunkIndex byChunkIndex( fragList );
				while ( byChunkIndex.Pop( batchChunkIndex, fragCount, fragList ) )
				{
					ASSERT( batchChunkIndex < batchMesh->GetNumChunks() );

					// Skip batches with no material
					if ( nullptr == batchMaterial )
					{
						continue;
					}

					// Build batch
					Batch newBatch;
					newBatch.m_elements = fragList;
					newBatch.m_numFragments = fragCount;
					newBatch.m_chunkIndex = batchChunkIndex;
					newBatch.m_material = batchMaterial;
					newBatch.m_vertexFactory = batchVertexFactory;
					newBatch.m_mesh = batchMesh;
					newBatch.m_parameters = batchMaterialParams;
					newBatch.m_isSkinned = fragList->IsSkinnable();
					newBatch.m_isSelected = false;
					newBatch.m_lightChannel = 0;						// Correct?
					newBatch.m_hasExtraStreams = fragList->HasFlag( RMCF_UsesExtraStreams );
					newBatch.m_hasVertexCollapse = fragList->HasFlag( RMCF_UsesVertexCollapse );
					newBatch.m_clippingEllipse = false;					// What's this?
					newBatch.m_isTwoSided = batchMaterial->IsTwoSided() || batchMesh->IsTwoSided();

					// Modify batch that is going to be rendered
					//SBatchModifier::GetInstance().ModifyBatch( &newBatch );		// Needed?

					// Set render states needed to render this batch
					if ( SelectBatchRenderStates( newBatch, context ) )
					{
						// Render meshes
						DrawBatchOfMeshes( info, context, newBatch, newBatch.m_mesh, newBatch.m_chunkIndex, renderFlags, newBatch.m_elements, outMeshStats );
					}
				}
			}
		}
	}
}

bool CRenderSwarmBatcher::SelectBatchRenderStates( const Batch& newBatch, const RenderingContext& context )
{	
	// Prepare material context
	MaterialRenderingContext materialContext( context );
	materialContext.m_vertexFactory			= newBatch.m_vertexFactory;
	materialContext.m_selected				= newBatch.m_isSelected;
	materialContext.m_uvDissolveSeparateUV	= false;
	materialContext.m_materialDebugMode		= context.m_materialDebugMode;
	materialContext.m_hasExtraStreams		= newBatch.m_hasExtraStreams;
	materialContext.m_hasVertexCollapse		= newBatch.m_hasVertexCollapse;
	materialContext.m_useInstancing			= newBatch.m_elements->CanUseInstancing();
	materialContext.m_discardingPass		= newBatch.m_parameters->IsMasked();

	// Invalid vertex factory, do not draw
	if ( materialContext.m_vertexFactory == MVF_Invalid )
	{
		// Failed to find vertex factory
		return false;
	}

	GetRenderer()->GetStateManager().SetPixelConst( PSC_DiscardFlags, Vector( newBatch.m_parameters->IsMasked(), 0, 0, 0 ) );

	CRenderMaterial::CompiledTechnique* technique = nullptr;
	if ( newBatch.m_material->BindShaders( materialContext, technique ) )
	{
		// swarms should always have really really small textures, so setting the distance to 0 should not be a problem
		// in this case we have to change this, we have to find a way how to calculate a reasonable distance for the whole swarm
		if ( !newBatch.m_material->BindParams( materialContext, newBatch.m_parameters, technique, 0.f ) )
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	// Bind buffers
	Uint32 streamsToBind = RMS_PositionSkinning | RMS_TexCoords | RMS_Custom0;
	{
		streamsToBind |= RMS_TangentFrame | RMS_Extended;
	}

	newBatch.m_mesh->Bind( newBatch.m_chunkIndex, streamsToBind );

	// Set lighting group
	if ( GpuApi::IsStencilLightChannelsContext( GpuApi::GetDrawContext() ) )
	{
		GpuApi::SetDrawContext( GpuApi::GetDrawContext(), newBatch.m_lightChannel );
	}

	// Set two sided
	GpuApi::SetForcedTwoSidedRender( newBatch.m_isTwoSided );

	// Can render
	return true;
}

#define SWARM_INSTANCING_BUFFER_SIZE (2*1024)

void CRenderSwarmBatcher::DrawBatchOfMeshes( const CRenderFrameInfo& info, const RenderingContext& context, const Batch& batch, CRenderMesh* mesh, Uint32 chunkIndex, Uint32 renderFlags, CRenderElement_MeshChunk* batchList, class MeshDrawingStats& outMeshStats )
{
	PC_SCOPE_RENDER_LVL1( SwarmBatcher_DrawBatch );

	Bool isSkinned = batch.m_isSkinned;

	if (isSkinned)
	{
		GetRenderer()->GetSkinManager()->BindSkinningBuffer();
	}

	TDynArray< SSkinnedMeshInstanceDescriptor > instances;

	const CEnvGlobalLightParametersAtPoint &globalLightParams = info.m_envParametersArea.m_globalLight;

#ifndef RED_FINAL_BUILD
	outMeshStats.m_numBatches += 1;
	outMeshStats.m_biggestBatch = Max( outMeshStats.m_biggestBatch, batch.m_numFragments );
	outMeshStats.m_smallestBatch = Min( outMeshStats.m_smallestBatch, batch.m_numFragments );
#endif

	// Draw fragments
	CRenderElement_MeshChunk* frag = nullptr;
	CRenderBatchListIterator< CRenderElement_MeshChunk > listIterator( batchList );
	while ( listIterator.Pop( frag ) )
	{
		Bool canInstantiate = frag->CanUseInstancing();

		IRenderProxyDrawable* proxy = frag->GetProxy();
		CRenderProxy_Swarm* swarmProxy = frag->GetProxy()->GetType() == RPT_Swarm ? (CRenderProxy_Swarm*)proxy : nullptr;
		Uint32 fragLOD =  (Uint32)frag->GetLodGroupIndex();

		if( !swarmProxy )
		{
			continue;
		}

		// get list of agents in LOD group
		const TDynArray<const CSwarmBoidData*>& boidsInLOD = swarmProxy->GetInstanceArrayForLOD( fragLOD );

		for( Uint32 i=0; i<boidsInLOD.Size(); ++i )			// TODO: remove this loop over all agents per render element
		{
			const CSwarmBoidData* boid = boidsInLOD[ i ];

			// calculate agent world matrix
			Matrix mPos = Matrix::IDENTITY;
			Matrix mRot = Matrix::IDENTITY;
			Matrix mSca = Matrix::IDENTITY;
			mPos.SetTranslation( boid->m_position );
			boid->m_rotation.ToMatrix( mRot );
			mSca.SetScale33( Vector::ONES * boid->m_scale );
			Matrix mWorld = mSca * mRot * mPos;

			Vector skinningData;

			// OK, we need skinning
			if ( isSkinned )
			{
				ASSERT( frag->HasFlag( RMCF_UsesSkinning ) );

				const CRenderSkinningData* skinData = nullptr;
				if ( proxy->GetType() == RPT_Swarm )
				{
					CRenderProxy_Swarm* swarmProxy = static_cast< CRenderProxy_Swarm* >(proxy);
					skinData = swarmProxy->GetSkinningDataForBoid( boid );
				}

				ASSERT( skinData );

				// Set the skinning data via the vertex shader constants
				if ( skinData )
				{
					skinningData = skinData->GetBindData();
				}
			}

			// Draw the bonded chunk
			if ( canInstantiate )
			{
				instances.Grow();
				SSkinnedMeshInstanceDescriptor& instance = instances.Back();
				RED_STATIC_ASSERT( 3 * 4 == ARRAY_COUNT( instance.m_localToWorld ) );
				mWorld.GetColumnMajor3x4( instance.m_localToWorld );
				instance.m_detailLevelParams = Vector::ZEROS;
				instance.m_skinningData = skinningData;
			}
			else
			{
				GetRenderer()->GetStateManager().SetLocalToWorld( &mWorld );

				// Setup bone data
				GpuApi::SetVertexShaderConstF( VSC_SkinningData, &(skinningData.A[0]), 1 );

				// Draw
				mesh->DrawChunkNoBind( chunkIndex, 1, outMeshStats );
			}
		}

		if ( canInstantiate && instances.Size() > 0 )
		{
			if ( !m_instancesBufferRef )
			{
				// Create the instance buffer
				const Uint32 chunkSize = sizeof( SSkinnedMeshInstanceDescriptor );
				const Uint32 instanceDataSize = chunkSize * SWARM_INSTANCING_BUFFER_SIZE;
				m_instancesBufferRef = GpuApi::CreateBuffer( instanceDataSize, GpuApi::BCC_Vertex, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
				GpuApi::SetBufferDebugPath( m_instancesBufferRef, "batchedMeshInstance" );
				ASSERT( m_instancesBufferRef );
			}

			const Uint32 streamStride = sizeof( SSkinnedMeshInstanceDescriptor );
			Uint32 numInstances = instances.Size();

			void *instancedPtr = nullptr;

			// Lock the instance buffer for write
			const Uint32 lockSize = streamStride * numInstances;
			Uint32 lockOffset = streamStride * m_instancesDataElemOffset;
			if ( m_instancesDataElemOffset + numInstances >= SWARM_INSTANCING_BUFFER_SIZE )
			{
				m_instancesDataElemOffset = 0;
				lockOffset = 0;
				instancedPtr = GpuApi::LockBuffer( m_instancesBufferRef, GpuApi::BLF_Discard, lockOffset, lockSize );
			}
			else
			{
				instancedPtr = GpuApi::LockBuffer( m_instancesBufferRef, GpuApi::BLF_NoOverwrite, lockOffset, lockSize );
			}

			Red::System::MemoryCopy( instancedPtr, &instances[0], lockSize );

			// Return the buffer
			GpuApi::UnlockBuffer( m_instancesBufferRef );

			//clear the instance array
			instances.ClearFast();

			// Bind
			GpuApi::BindVertexBuffers( 7, 1, &m_instancesBufferRef, &streamStride, &lockOffset );

			mesh->DrawChunkNoBindInstanced( chunkIndex, 1, numInstances, outMeshStats );

			// Update offset in instances buffer
			m_instancesDataElemOffset += numInstances;
		}
	}

	// Draw collected instances if any
	//DrawInstances( mesh, chunkIndex, m_instances, outMeshStats );
}

void CRenderSwarmBatcher::OnDeviceLost()
{
	// Do nothing
}

void CRenderSwarmBatcher::OnDeviceReset()
{
	// Do nothing
}

CName CRenderSwarmBatcher::GetCategory() const
{
	return CNAME( RenderSwarmBatcher );
}

Uint32 CRenderSwarmBatcher::GetUsedVideoMemory() const
{
	return 0;
}
