/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderMeshBatcher.h"
#include "renderSkinningData.h"
#include "renderSceneBatchers.h"
#include "renderProxyMesh.h"
#include "renderProxyMorphedMesh.h"
#include "renderTexture.h"
#include "../engine/baseEngine.h"
#include "../engine/renderFragment.h"
#include "../engine/normalBlendComponent.h"
#include "renderSkinManager.h"
#include "../redMath/redMatrix4x4_simd.h"
#include "renderCollector.h"
#include "renderShaderPair.h"
#include "../core/frustum.h"

#if defined( RED_PLATFORM_DURANGO )
#include "..\gpuApiDX10\gpuApiBase.h"
#endif

CRenderMeshBatcher::CRenderMeshBatcher( Uint32 maxFragmentsPerBatch )
	: m_maxFragmentsPerBatch( maxFragmentsPerBatch )
	, m_lastMaterial( nullptr )
	, m_lastParameters( nullptr )
	, m_lastCharactersLightingBoost( -1 )
	, m_lastProxy( nullptr )
	, m_lastProxy_Instanced( nullptr )
	, m_instancesDataElemOffset( 0 )
{}

CRenderMeshBatcher::~CRenderMeshBatcher()
{
	GpuApi::SafeRelease( m_instancesBufferRef );

	ReleaseCsCullingBuffers();
}

void CRenderMeshBatcher::RenderMeshes( const CRenderFrameInfo& info, const RenderingContext& context, Uint32 numChunks, CRenderElement_MeshChunk * const *chunks, Uint32 renderFlags, class MeshDrawingStats& outMeshStats )
{
	if ( numChunks > 0 )
	{
		// Construct list
		// TO DO: this is not efficient so much....
		CRenderElement_MeshChunk* list = nullptr;
		for ( Uint32 i=0; i<numChunks; i++ )
		{
			CRenderElement_MeshChunk* elem = chunks[i];
			elem->SetBatchNext( list );
			list = elem;
		}

		// Render using list
		RenderMeshes( info, context, list, renderFlags, outMeshStats );
	}
}

namespace Config
{
	TConfigVar< Int32 >	cvDispatchThreadsX( "Rendering/MergedShadows", "cvDispatchThreadsX",	1024, eConsoleVarFlag_Developer );
}

void CRenderMeshBatcher::RenderMergedMeshes(const CRenderFrameInfo& info, const RenderingContext& context, const TDynArray< CRenderElement_MeshChunk* >& batchList, Uint32 renderFlags, class MeshDrawingStats& outMeshStats)
{
	// Do not draw meshes at all
	if ( !info.IsShowFlagOn( SHOW_Meshes ) )
	{
		return;
	}

	// Filter by skinned or static 
	if ( !info.IsShowFlagOn( SHOW_GeometryStatic ) && !info.IsShowFlagOn( SHOW_GeometrySkinned ) )
	{
		return;
	}

	if( batchList.Size() == 0 )
	{
		return;
	}

	{
		PC_SCOPE_RENDER_LVL1( MergedMeshesCulling );

		CRenderStateManager& stateManager = GetRenderer()->GetStateManager();

		MeshDrawingStats stats;
		Uint32 chunksNum = (Uint32)batchList.Size();

		Vector frustum[6];
		context.GetCamera().CalculateFrustumPlanes( frustum );

		GetRenderer()->m_simpleDrawCsVs->BindVS();
		GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexMeshStaticPositionOnly );

		for( Uint32 k = 0; k < chunksNum; ++k )
		{
			CRenderElement_MeshChunk* chunk = batchList[k];
			CRenderMesh* mesh = chunk->GetMesh();
			if( !mesh->IsFullyLoaded() || !mesh->IsClusterDataReady() )
			{
				continue;
			}

			TDynArray< CRenderMesh::SClusterData >& clusters = mesh->GetCullingClusterData( chunk->GetChunkIndex() );
			const Matrix& localToWorld = chunk->GetProxy()->GetLocalToWorld();
			stateManager.SetLocalToWorld( &localToWorld );
			mesh->BindBuffersVS( chunk->GetChunkIndex() );

			Uint32 start = 0, end = 0;
			Bool draw = false, prevClusterSkipped = false;

			for( CRenderMesh::SClusterData c : clusters )
			{
				if( draw ) 
				{
					GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, 0, start, (end - start + 1)/3 );

					draw = false;
					start = c.IStart;
					end = start;
				}
				else if( prevClusterSkipped )
				{
					start = c.IStart;
					end = start;
				}

				prevClusterSkipped = false;

				Vector center(c.Centroid.X, c.Centroid.Y, c.Centroid.Z);
				Float r = c.Centroid.W;

				center = localToWorld.TransformPoint(center);
				r *= localToWorld.GetScale33().X;

				for( Uint8 i = 0; i < 6; ++i )
				{
					if( frustum[i].Dot4( Vector(center.X, center.Y, center.Z, 1.0f) ) + r < 0 )
					{ 
						draw = start != end; 
						prevClusterSkipped = true;
						break;
					}
				}
				
				if( prevClusterSkipped ) continue;

				end = c.IEnd;
			}

			if( start != end )
			{
				GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, 0, start, (end - start + 1)/3 );
			}
		}
	}
}

void CRenderMeshBatcher::RenderMergedMeshesCsVs( const CRenderFrameInfo& info, const RenderingContext& context, const TDynArray< CRenderElement_MeshChunk* >& batchList, Uint32 renderFlags, class MeshDrawingStats& outMeshStats )
{
		// Do not draw meshes at all
	if ( !info.IsShowFlagOn( SHOW_Meshes ) )
	{
		return;
	}

	// Filter by skinned or static 
	if ( !info.IsShowFlagOn( SHOW_GeometryStatic ) && !info.IsShowFlagOn( SHOW_GeometrySkinned ) )
	{
		return;
	}

	if( batchList.Size() == 0 )
	{
		return;
	}

	{
		PC_SCOPE_RENDER_LVL1( MergedMeshesCulling );

		InitCsCullingBuffers();

		//// here will go the loop through the compute dispatches
		Uint32 stride = 3 * sizeof(Float), offset = 0;
		GpuApi::BindConstantBuffer( 3, m_customCsCB, GpuApi::ComputeShader );

		CRenderStateManager& stateManager = GetRenderer()->GetStateManager();

		Matrix worldToView = context.GetCamera().GetWorldToView().Transposed();
		Matrix worldToScreen = context.GetCamera().GetWorldToScreenRevProjAware().Transposed();

		MeshDrawingStats stats;
		Uint32 chunksNum = (Uint32)batchList.Size();
		
		for( Uint32 k = 0; k < chunksNum; ++k )
		{
			CRenderElement_MeshChunk* chunk = batchList[k];
			CRenderMesh* mesh = chunk->GetMesh();
			if( !mesh->IsFullyLoaded() || !mesh->IsClusterDataReady() )
			{
				continue;
			}

			Uint32 numIndices = mesh->GetChunks()[chunk->GetChunkIndex()].m_numIndices;
			Uint32 factor = Max(1u, 1u + (Uint32)(numIndices / (3 * Config::cvDispatchThreadsX.Get())));

			// COMPUTE CULLING
			Vector qs, qb;
			Uint32 indexOffset, vertexOffset;
			mesh->BindCsVsBuffersCS( chunk->GetChunkIndex(), qs, qb, indexOffset, vertexOffset );

			{
				void* lockedConstantsData = GpuApi::LockBuffer( m_customCsCB, GpuApi::BLF_Discard, 0, sizeof( SCsCullCB ) );
				RED_ASSERT( lockedConstantsData );
				SCsCullCB* lockedConstants = static_cast< SCsCullCB* >( lockedConstantsData );

				lockedConstants->m_localToWorld = chunk->GetProxy()->GetLocalToWorld().Transposed();
				lockedConstants->m_worldToScreen = worldToScreen;
				lockedConstants->m_qs = Vector(qs.X, qs.Y, qs.Z, 0);
				lockedConstants->m_qb = Vector(qb.X, qb.Y, qb.Z, 0);
				lockedConstants->m_count = Vector( numIndices/3.0f, (Float)(indexOffset/2), (Float)(vertexOffset/8), 0 );

				GpuApi::UnlockBuffer( m_customCsCB );
			}

			GetRenderer()->m_computeCull->Dispatch(factor, 1, 1);

			GpuApi::BindBufferUAV(GpuApi::BufferRef::Null(), 0); // indirect args
			GpuApi::BindBufferUAV(GpuApi::BufferRef::Null(), 1); // append index buffer
			GpuApi::BindBufferSRV(GpuApi::BufferRef::Null(), 0, GpuApi::eShaderType::ComputeShader);
			GpuApi::BindBufferSRV(GpuApi::BufferRef::Null(), 1, GpuApi::eShaderType::ComputeShader); 

			GetRenderer()->m_simpleDrawCsVs->BindVS();
			GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexMeshStaticPositionOnly );

			stateManager.SetLocalToWorld( &chunk->GetProxy()->GetLocalToWorld() );
			mesh->BindCsVsBuffersVS( chunk->GetChunkIndex() );

			GpuApi::DrawInstancedIndexedPrimitiveIndirect( GpuApi::PRIMTYPE_TriangleList );
		}

		GpuApi::BindBufferUAV(GpuApi::BufferRef::Null(), 0); // indirect args
		GpuApi::BindBufferUAV(GpuApi::BufferRef::Null(), 1); // append index buffer
		GpuApi::BindBufferSRV(GpuApi::BufferRef::Null(), 0, GpuApi::eShaderType::ComputeShader);
		GpuApi::BindBufferSRV(GpuApi::BufferRef::Null(), 1, GpuApi::eShaderType::ComputeShader); 

		stateManager.SetShader( GpuApi::ShaderRef::Null(), ERenderShaderType::RST_ComputeShader );
	}
}

void CRenderMeshBatcher::RenderMeshes( const CRenderFrameInfo& info, const RenderingContext& context, const TDynArray< CRenderElement_MeshChunk* >& batchList, Uint32 renderFlags, class MeshDrawingStats& outMeshStats )
{
	RenderMeshes( info, context, batchList.Size(), batchList.TypedData(), renderFlags, outMeshStats );
}

#ifndef NO_DEBUG_PAGES

class CRenderMeshChunkCounter : public IRenderDebugCounter
{
public:
	CRenderMeshChunkCounter()
		: IRenderDebugCounter( TXT("Chunks"), 3500.0f )
	{};

	//! Get the color for drawing the bar
	virtual Color GetColor() const
	{
		if ( m_current < 1250.0f ) return Color( 0, 128, 0 );
		if ( m_current < 1500.0f ) return Color( 128, 128, 0 );
		return Color( 128, 0, 0 );
	}

	// Get the text to display
	virtual String GetText() const
	{
		return String::Printf( TXT("Mesh chunks: %1.0f"), m_current );
	}
};

class CRenderMeshChunkCharactersCounter : public IRenderDebugCounter
{
public:
	CRenderMeshChunkCharactersCounter()
		: IRenderDebugCounter( TXT("Character chunks"), 3500.0f )
	{};

	//! Get the color for drawing the bar
	virtual Color GetColor() const
	{
		if ( m_current < 1250.0f ) return Color( 0, 128, 0 );
		if ( m_current < 1500.0f ) return Color( 128, 128, 0 );
		return Color( 128, 0, 0 );
	}

	// Get the text to display
	virtual String GetText() const
	{
		return String::Printf( TXT("Character mesh chunks: %1.0f"), m_current );
	}
};

#endif

#define MESH_INSTANCING_BUFFER_SIZE (64*1024)

RED_INLINE bool IsCharactersLightingBoostRender( const RenderingContext& context )
{
	return (RP_ForwardLightingSolid == context.m_pass || RP_ForwardLightingTransparent == context.m_pass);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
// List of batched functions

#ifdef RED_PLATFORM_ORBIS
	#define FAST_CALL
#else
	#define FAST_CALL __fastcall
#endif

typedef void (FAST_CALL *BatcherMacroCallback)( SBatcherInfo& info );

////////////////////////////////////////////////////////////////////////////////////////////////////////

class SBatcherMacroNode
{

	//---------------------------------------------------

#ifndef NO_COMPONENT_GRAPH
	RED_INLINE static void FAST_CALL HitProxyBatch( SBatcherInfo& info )
	{
		info.m_needsSeparateDraw = true;

		const CHitProxyID& id = info.m_context->m_useConstantHitProxyID ? info.m_context->m_constantHitProxyID : info.m_proxy->GetHitProxyID();
		info.m_stateManager->SetPixelConst( PSC_HitProxyColor, id.GetColor().ToVector() );
	}
#endif

	RED_INLINE static void FAST_CALL ColorShift( SBatcherInfo& info )
	{
		info.m_needsSeparateDraw = true;

		const SRenderProxyDrawableMeshColoringParams* colorParams = info.m_proxy->GetColoringParams();
		if ( colorParams )
		{
			info.m_stateManager->SetPixelConst( PSC_ColorOne, colorParams->m_colorShiftMatrix0 );
			info.m_stateManager->SetPixelConst( PSC_ColorTwo, colorParams->m_colorShiftMatrix1 );
		}
		else
		{
			info.m_stateManager->SetPixelConst( PSC_ColorOne, Matrix::IDENTITY );
			info.m_stateManager->SetPixelConst( PSC_ColorTwo, Matrix::IDENTITY );
		}
	}

	RED_INLINE static void FAST_CALL Selection( SBatcherInfo& info )
	{
		info.m_needsSeparateDraw = true;

		Vector selectionColor( 0.2f, 1.0f, 0.4f, 0.35f );
		Vector selectionEffect( 0.0f, 0.0f, 0.0f, 0.0f );

		SBatchModifier::GetInstance().ModifyMeshSelection( info.m_frag, selectionColor, selectionEffect );
		info.m_stateManager->SetPixelConst( PSC_ConstColor, selectionColor );
		info.m_stateManager->SetPixelConst( PSC_SelectionEffect, selectionEffect );
	}

	RED_INLINE static void FAST_CALL Skinning( SBatcherInfo& info )
	{
		if ( info.m_proxy->GetType() != RPT_Mesh )
			return;

		CRenderProxy_Mesh* meshProxy = static_cast< CRenderProxy_Mesh* >(info.m_proxy);
		CRenderSkinningData* skinData = meshProxy->GetSkinningData();
		
		// Set the skinning data via the vertex shader constants
		if ( skinData )
		{
			info.m_skinningData = skinData->GetBindData();
		}
		else
		{
			info.m_skinningData = GetRenderer()->GetSkinManager()->GetDefaultBindData();
		}

		// bind skin texture
		GetRenderer()->GetSkinManager()->BindSkinningBuffer();
	}

	// Setup eye ralated custom forward shading constants
	RED_INLINE static void FAST_CALL Eye( SBatcherInfo& info )
	{
		const ESkinningDataCustomMatrix eyeMatrixTypes[2] = { SDCM_EyeOrientationLeft, SDCM_EyeOrientationRight };
		const Uint16 eyeShaderRegisterIndices[2][3] = { 
			{ PSC_Custom_EyeOrientationLeft_AxisX, PSC_Custom_EyeOrientationLeft_AxisY, PSC_Custom_EyeOrientationLeft_AxisZ }, 
			{ PSC_Custom_EyeOrientationRight_AxisX, PSC_Custom_EyeOrientationRight_AxisY, PSC_Custom_EyeOrientationRight_AxisZ } 
		};

		info.m_needsSeparateDraw = true;

		CRenderSkinningData* skinData = nullptr;
		if ( info.m_batch->m_isSkinned && info.m_proxy->GetType() == RPT_Mesh )
		{
			CRenderProxy_Mesh* meshProxy = static_cast< CRenderProxy_Mesh* >(info.m_proxy);
			skinData = meshProxy->GetSkinningData();
		}

		for ( Int32 eye_i = 1; eye_i >= 0; --eye_i )
		{
			Matrix eyeMatrix;
			if ( skinData && skinData->GetCustomMatrix( eyeMatrixTypes[eye_i], eyeMatrix ) )
			{
				eyeMatrix = eyeMatrix * info.m_proxy->GetLocalToWorld();
			}
			else
			{
				eyeMatrix = info.m_proxy->GetLocalToWorld();
			}

			info.m_stateManager->SetPixelConst( eyeShaderRegisterIndices[eye_i][0], eyeMatrix.GetAxisX() );
			info.m_stateManager->SetPixelConst( eyeShaderRegisterIndices[eye_i][1], eyeMatrix.GetAxisY() );
			info.m_stateManager->SetPixelConst( eyeShaderRegisterIndices[eye_i][2], eyeMatrix.GetAxisZ() );
		}
	}

	// Setup antiLightbleed related constants
	RED_INLINE static void FAST_CALL Skin( SBatcherInfo& info )
	{
		info.m_needsSeparateDraw = true;

		CRenderSkinningData* skinData = nullptr;
		if ( info.m_info->IsShowFlagOn( SHOW_AntiLightbleed ) && info.m_batch->m_isSkinned && info.m_proxy->GetType() == RPT_Mesh )
		{
			CRenderProxy_Mesh* meshProxy = static_cast< CRenderProxy_Mesh* >( info.m_proxy );
			skinData = meshProxy->GetSkinningData();
		}

		Matrix headMatrix;
		Vector headCenterPos;
		Vector headFrontDirection;
		Vector headUpDirection;
		Vector worldHeadCenterPos;
		Vector worldHeadFrontDirection;
		Vector worldHeadUpDirection;

		if ( skinData && skinData->GetCustomMatrix( SDCM_HeadTransformation, headMatrix ) && skinData->GetCustomHeadData( headCenterPos, headFrontDirection, headUpDirection ) )
		{
			const Matrix combinedMatrix = headMatrix * info.m_proxy->GetLocalToWorld();

			worldHeadCenterPos = combinedMatrix.TransformPoint( headCenterPos );
			worldHeadCenterPos.W = 1.f;

			worldHeadFrontDirection = combinedMatrix.TransformVector( headFrontDirection ).Normalized3();
			worldHeadUpDirection = combinedMatrix.TransformVector( headUpDirection ).Normalized3();
		}
		else
		{
			worldHeadCenterPos = Vector::ZEROS; // (w=0 means there is no data to operate on)
			worldHeadFrontDirection = Vector::ZEROS;
			worldHeadUpDirection = Vector::ZEROS;			
		}

		info.m_stateManager->SetPixelConst( PSC_Custom_HeadMatrix, Matrix ( worldHeadFrontDirection, worldHeadCenterPos, worldHeadUpDirection, Vector::ZEROS ) );		
	}

	RED_INLINE static void FAST_CALL ClippingElipse( SBatcherInfo& info )
	{
		info.m_needsSeparateDraw = true;

		info.m_stateManager->SetVertexConst( VSC_Custom_Matrix, info.m_proxy->GetClippingEllipseParams()->m_localToEllipse );	
	}


	// Draw the bonded chunk
	RED_INLINE static void FAST_CALL NormalBlend( SBatcherInfo& info )
	{
		if ( info.m_proxy->GetType() != RPT_Mesh)
			return;

		CRenderProxy_Mesh* meshProxy = static_cast< CRenderProxy_Mesh* >(info.m_proxy);
		const Float* areas = meshProxy->GetNormalBlendAreas();

		if( !areas )
			return;

		//RED_ALIGNED_VAR(Float,16) alignAreas[16];
		//Red::System::MemoryCopy( alignAreas , areas , 4*4*4 );

		const Float* weights = meshProxy->GetNormalBlendWeights();

		// Copy normal-blend areas. Each takes a single register.
		info.m_stateManager->SetPixelShaderConstRaw( PSC_Custom_0, areas, NUM_NORMALBLEND_AREAS );

		if ( weights )
		{
			//RED_ALIGNED_VAR(Float,16) alignWeights[16];
			//Red::System::MemoryCopy( alignWeights , weights , 4*4*4 );
			// Copy normal-blend weights. 16 of them, so they take up 4 registers.
			info.m_stateManager->SetPixelShaderConstRaw( PSC_Custom_0 + NUM_NORMALBLEND_AREAS, weights, 4 );
			return;
		}

		for (Uint32 weight_i=0; weight_i<4; ++weight_i)
		{
			info.m_stateManager->SetPixelConst( PSC_Custom_0 + NUM_NORMALBLEND_AREAS + weight_i, Vector::ONES );
		}

	}

	// Effect parameters 0
	RED_INLINE static void FAST_CALL EffectParam0( SBatcherInfo& info )
	{
		info.m_needsSeparateDraw = true;

		const SRenderProxyDrawableEffectParams* effect = info.m_proxy->GetEffectParams();
		if ( effect )
		{
			info.m_stateManager->SetPixelConst( PSC_Custom0, effect->m_customParam0 );
			info.m_stateManager->SetVertexConst( VSC_Custom_0, effect->m_customParam0 );
			return;
		}
		
		info.m_stateManager->SetVertexConst( VSC_Custom_0, Vector::ZEROS );
		info.m_stateManager->SetPixelConst( PSC_Custom0, Vector::ZEROS );
	}

	// Effect parameters 1
	RED_INLINE static void FAST_CALL EffectParam1( SBatcherInfo& info )
	{
		info.m_needsSeparateDraw = true;

		const SRenderProxyDrawableEffectParams* effect = info.m_proxy->GetEffectParams();
		if ( effect )
		{
			info.m_stateManager->SetPixelConst( PSC_Custom1, effect->m_customParam1 );
			return;
		}

		info.m_stateManager->SetPixelConst( PSC_Custom1, Vector::ZEROS );
	}

	// Material override params
	RED_INLINE static void FAST_CALL MaterialOveride( SBatcherInfo& info )
	{
		const SRenderProxyDrawableEffectParams* effect = info.m_proxy->GetEffectParams();
		if ( effect )
		{
			info.m_needsSeparateDraw = true;
			info.m_stateManager->SetPixelConst( PSC_Custom0, effect->m_overrideParams );
			return;
		}
		
		info.m_stateManager->SetPixelConst( PSC_Custom0, Vector::ZEROS );
	}

	RED_INLINE static void FAST_CALL DissolveUV( SBatcherInfo& info )
	{
		// UV Dissolve uses specific texture, so can't instance it
		info.m_needsSeparateDraw = true;

		Vector uvDissolveData;
		uvDissolveData = info.m_frag->GetUVDissolveValues();
		info.m_dissolveData.Z = uvDissolveData.X;
		info.m_dissolveData.W = uvDissolveData.Y;
	}

	RED_INLINE static void FAST_CALL Morph( SBatcherInfo& info )
	{
		if ( info.m_proxy->GetType() != RPT_Mesh || !static_cast< CRenderProxy_Mesh* >( info.m_proxy )->IsMorphedMesh() )
			return;

		Float ratio = static_cast< CRenderProxy_MorphedMesh* >( info.m_proxy )->GetMorphRatio();
		info.m_stateManager->SetPixelConst( PSC_MorphRatio, Vector( ratio, 0, 0, 0 ) );
	}

	// Vertex collapse
	/*if ( vertexType == MVT_SkinnedMeshVertexCollapse || vertexType == MVT_StaticMeshVertexCollapse )
	{				
		const Vector vertexCollapse( frag->HasFlag( RMCF_UsesVertexCollapse ) ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f );

		// Hack - using VSC_LodParams, because it is only for leafcards so this const is free :) 
		GetRenderer()->GetStateManager().SetVertexConst( VSC_LodParams, vertexCollapse );
	}*/

public:

	//---------------------------------------------------

	RED_FORCE_INLINE static void FAST_CALL Render( SBatcherInfo& info )
	{

		const Bool isMeshSkinned = info.m_batch->m_isSkinned;
		const Bool isUseInstncing = info.m_frag->CanUseInstancing();

		Uint8 renderMask = ( FLAG(2) * !isUseInstncing ) | ( FLAG(1) * isMeshSkinned ) | ( FLAG(0) * info.m_needsSeparateDraw );

		if ( isUseInstncing && ( info.m_proxy != info.m_batcher->m_lastProxy_Instanced ) )
		{
			info.m_batcher->ComputeLocalToWorld( *info.m_context, info.m_proxy, info.m_frag, info.m_batcher->m_lastInstancedLocalToWorld );
			info.m_batcher->m_lastProxy_Instanced = info.m_proxy;
		}

		switch( renderMask )
		{
		// not skinned, not needsSeparateDraw
		case 0:
			{
				info.m_batcher->m_instances.Grow();
				RED_STATIC_ASSERT( 3 * 4 == ARRAY_COUNT( info.m_batcher->m_instances.Back().m_localToWorld ) );
				info.m_batcher->m_lastInstancedLocalToWorld.GetColumnMajor3x4( info.m_batcher->m_instances.Back().m_localToWorld );
				info.m_batcher->m_instances.Back().m_detailLevelParams = info.m_dissolveData;
			}
			return;

		// not skinned, needsSeparateDraw
		case 1:
			{
				SMeshInstanceDescriptor desc;
				desc.m_detailLevelParams = info.m_dissolveData;

				RED_STATIC_ASSERT( 3 * 4 == ARRAY_COUNT( desc.m_localToWorld ) );
				info.m_batcher->m_lastInstancedLocalToWorld.GetColumnMajor3x4( desc.m_localToWorld );
				info.m_batcher->DrawInstances( info.m_mesh, info.m_chunkIndex, &desc, 1, sizeof(SMeshInstanceDescriptor), *info.m_outMeshStats );
			}
			return;
			
		// skinned, not needsSeparateDraw
		case 2:
			{
				info.m_batcher->m_skinnedInstances.Grow();
				RED_STATIC_ASSERT( 3 * 4 == ARRAY_COUNT( info.m_batcher->m_skinnedInstances.Back().m_localToWorld ) );
				info.m_batcher->m_lastInstancedLocalToWorld.GetColumnMajor3x4( info.m_batcher->m_skinnedInstances.Back().m_localToWorld );
				info.m_batcher->m_skinnedInstances.Back().m_detailLevelParams = info.m_dissolveData;
				info.m_batcher->m_skinnedInstances.Back().m_skinningData = info.m_skinningData;
			}
			return;

		// skinned, needsSeparateDraw
		case 3:
			{
				SSkinnedMeshInstanceDescriptor desc;
				desc.m_skinningData = info.m_skinningData;
				desc.m_detailLevelParams = info.m_dissolveData;

				RED_STATIC_ASSERT( 3 * 4 == ARRAY_COUNT( desc.m_localToWorld ) );
				info.m_batcher->m_lastInstancedLocalToWorld.GetColumnMajor3x4( desc.m_localToWorld );
				info.m_batcher->DrawInstances( info.m_mesh, info.m_chunkIndex, &desc, 1, sizeof(SSkinnedMeshInstanceDescriptor), *info.m_outMeshStats );
			}
			return;

		}

		// any another case

		if ( info.m_proxy != info.m_batcher->m_lastProxy )
		{
			info.m_batcher->ComputeLocalToWorld( *info.m_context, info.m_proxy, info.m_frag, info.m_batcher->m_lastLocalToWorld );
			info.m_stateManager->SetLocalToWorld( &info.m_batcher->m_lastLocalToWorld );
			info.m_batcher->m_lastProxy = info.m_proxy;
		}

		info.m_stateManager->SetVertexConst( VSC_DissolveParams, info.m_dissolveData );
		GpuApi::SetVertexShaderConstF( VSC_SkinningData, &(info.m_skinningData.A[0]), 1 );

		// Draw
		info.m_mesh->DrawChunkNoBind( info.m_chunkIndex, 1, *info.m_outMeshStats );
	}

	//---------------------------------------------------

	RED_INLINE static void FAST_CALL Run( Uint64 mask, SBatcherInfo& info )	
	{
		// This mask validates all IDs that may go out of our pool of valid pointers
		// First bit depends on present of 'HitProxyBatch' that is not available on non editor builds
#ifndef NO_COMPONENT_GRAPH
		const Uint64 validationMask = 0x17F1F;
#else
		const Uint64 validationMask = 0x17F1E;
#endif

		static const BatcherMacroCallback flagCallbacks[] = {

			// context and batcher related
#ifndef NO_COMPONENT_GRAPH
			HitProxyBatch,		// 0
#else
			nullptr,			// 0
#endif
			Selection,			// 1
			Skinning,			// 2
			Eye,				// 3
			Skin,				// 4
			nullptr,			// 5
			nullptr,			// 6
			nullptr,			// 7

			// Fragment related
			ColorShift,			// 8
			NormalBlend,		// 9
			EffectParam0,		// 10
			EffectParam1,		// 11
			MaterialOveride,	// 12
			DissolveUV,			// 13
			Morph,				// 14
			nullptr,			// 15
			
			// Proxy related
			ClippingElipse,		// 16

		};

#ifndef NO_MESH_PROXY_DISSOLVE
		switch( info.m_dissolveType )
		{
		// case EBDT_None: break;
		case SBatcherInfo::EBDT_Shadowed: 
			info.m_dissolveData = info.m_frag->GetShadowDissolveValues();
			break;
		case SBatcherInfo::EBDT_Regular: 
			info.m_dissolveData = info.m_frag->GetDissolveValues();
			break;
		}
#endif

		info.m_needsSeparateDraw = false;
		
		Uint64 m = ( mask | info.m_batcherInclude ) & info.m_batcherMask;

		// Super sanity check - remove any bits that would cause crash on null-call
		// This magic is OR-ed bits from valid functions in table
		RED_FATAL_ASSERT( (m & ~validationMask) == 0 , "Incorect batcher setter mask. It seems that something overwrite batcher memory. Mask: %" RED_PRIWu64 , m );
		
		// Last chance AND. It will not help any way, bcoz if something is overwritten here, probably whole 'info' will be fucked up.
		m &= validationMask;

		while( m )
		{
#if defined( RED_PLATFORM_ORBIS )
			Uint64 id = __tzcnt_u64( m );
#else
			DWORD id;
			::_BitScanForward64( &id, m );
#endif
			flagCallbacks[id]( info );
			m ^= 1ULL << id;
		}

		Render( info );
	}

};

void CRenderMeshBatcher::RenderMeshes( const CRenderFrameInfo& info, const RenderingContext& context, CRenderElement_MeshChunk* batchList, Uint32 renderFlags, class MeshDrawingStats& outMeshStats )
{
	PC_SCOPE_PIX( RenderMeshes );

	CRenderMaterial* batchMaterial = nullptr;
	CRenderMaterialParameters* batchMaterialParams = nullptr;
	CRenderMesh* batchMesh = nullptr;
	Bool batchSelection = false;
	Uint32 batchChunkIndex = 0;
	EMaterialVertexFactory batchVertexFactory;
	CRenderElement_MeshChunk* fragList = nullptr;
	Uint32 batchLightChannel = 0;
	Bool batchClippingEllipse = false;

	// Do not draw meshes at all
	if ( !info.IsShowFlagOn( SHOW_Meshes ) )
	{
		return;
	}

	// Filter by skinned or static 
	if ( !info.IsShowFlagOn( SHOW_GeometryStatic ) && !info.IsShowFlagOn( SHOW_GeometrySkinned ) )
	{
		return;
	}

	// Nothing to draw, exit
	if ( !batchList )
	{
		return;
	}

	if ( !m_instancesBufferRef )
	{
		// Create the instance buffer
		const Uint32 chunkSize = sizeof( SMeshInstanceDescriptor );
		const Uint32 instanceDataSize = chunkSize * MESH_INSTANCING_BUFFER_SIZE;
		m_instancesBufferRef = GpuApi::CreateBuffer( instanceDataSize, GpuApi::BCC_Vertex, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		GpuApi::SetBufferDebugPath( m_instancesBufferRef, "batchedMeshInstance" );
		ASSERT( m_instancesBufferRef );
	}

	// Reset internal cache states
	m_lastMaterial = nullptr;
	m_lastParameters = nullptr;
	m_lastProxy = nullptr;
	m_lastCharactersLightingBoost = 0; //< let's assume that character boost is set as 'nautral in here..'

	// Assure that drawcontext won't change in this function
	const CGpuApiScopedDrawContext drawContextOriginalRestore;

	// Reset local to world matrix
	CRenderStateManager& stateManager = GetRenderer()->GetStateManager();
	stateManager.SetLocalToWorld( nullptr );

	// Rendering to GBuffer, mask by lighting channel, enable stencil
	const Bool deferredLighting = (context.m_pass == RP_GBuffer);
	const CGpuApiScopedDrawContext scopedDrawContext;
	if ( deferredLighting )
	{
		GpuApi::eDrawContext curContext = scopedDrawContext.GetOrigDrawContext();
		GpuApi::eDrawContext newContext = GpuApi::GetStencilLightChannelsContext( curContext );
		if ( newContext != curContext )
		{
			GpuApi::SetDrawContext( newContext, 0 );
		}
	}

	// Test whether we should use batch by light channels
	const Bool shouldBatchByLightChannels = deferredLighting || IsCharactersLightingBoostRender( context ) || ( context.m_lightChannelFilter != LCF_NoFilter );

	// Two sided render reset
	const CGpuApiScopedTwoSidedRender scopedForcedTwoSided;
	
	SBatcherInfo batcherInfo;
	batcherInfo.m_batcher		= this;
	batcherInfo.m_info			= &info;
	batcherInfo.m_context		= &context;
	batcherInfo.m_renderFlags	= renderFlags;
	batcherInfo.m_outMeshStats	= &outMeshStats;
	batcherInfo.m_stateManager	= &stateManager;

	if( renderFlags & RMBF_Shadowmap )
	{
		// If in shadow pass, turn off batcher for:
		Uint64 exludeMask = 
			SBatcherInfo::EBFT_HitProxyBatch | 
			SBatcherInfo::EBFT_Selection | 
			SBatcherInfo::EBFT_Eye | 
			SBatcherInfo::EBFT_Skin | 
			SBatcherInfo::EBFT_ColorShift | 
			SBatcherInfo::EBFT_NormalBlend;
		
		batcherInfo.m_batcherMask &= ~exludeMask;
	}

#ifndef NO_COMPONENT_GRAPH
	// HitProxyBatch
	batcherInfo.m_batcherInclude |= SBatcherInfo::EBFT_HitProxyBatch * ( context.m_pass == RP_HitProxies );
#endif

	Uint64 batcherOriginalIncludeBits = batcherInfo.m_batcherInclude;

	// Cache singleton access
	BatchModifier& batchModifier = SBatchModifier::GetInstance();

	// Group by material
	CRenderBatchByMaterialParams< CRenderElement_MeshChunk > byMaterial( batchList );
	while ( byMaterial.Pop( batchMaterialParams, batchMaterial, fragList ) )
	{
		// Skip batches with no material
		if ( nullptr == batchMaterial )
		{
			continue;
		}

		batcherInfo.m_batcherInclude = batcherOriginalIncludeBits;
		// Eye
		batcherInfo.m_batcherInclude |= SBatcherInfo::EBFT_Eye * ( batchMaterial->IsEye() && ( RP_ForwardLightingSolid == context.m_pass || ( RP_GBuffer == context.m_pass && MDM_FullGBuffer == context.m_materialDebugMode ) ) );
		// Skin
		batcherInfo.m_batcherInclude |= SBatcherInfo::EBFT_Skin * ( RP_ForwardLightingSolid == context.m_pass && ( RSG_Skin == batchMaterial->GetRenderSortGroup() || batchMaterial->IsEye() ) );

		// Group by selection flag
#ifndef NO_EDITOR
		const Bool renderSelection = info.IsShowFlagOn( SHOW_Selection );
		CRenderBatchBySelection< CRenderElement_MeshChunk > bySelection( fragList, renderSelection );
		while ( bySelection.Pop( batchSelection, fragList ) )
#endif
		{
			// Group by mesh
			CRenderBatchByMesh byMesh( fragList );
			while ( byMesh.Pop( batchMesh, fragList ) )
			{
				RED_ASSERT( batchMesh->IsFullyLoaded(), TXT("Rendering mesh batch where mesh is not fully loaded") );
				if ( !batchMesh->IsFullyLoaded() )
				{
					continue;
				}

				// Group by relevant RMCF_* flags (those that affect material binding)
				const ERenderMeshChunkFlags dissolveFlag = 0 != (renderFlags & RMBF_CascadeShadows) ? RMCF_UsesCascadesDissolve : RMCF_UsesDissolve;
				CRenderBatchByProxyFlags byProxyFlags( fragList, dissolveFlag );
				while ( byProxyFlags.Pop( fragList ) )
				{
					// If this batch is dissolved, select a dissolve type.
					if( !context.m_forceNoDissolves && fragList->HasFlag( dissolveFlag ) )
					{
						if( renderFlags & RMBF_Shadowmap )
						{
							batcherInfo.m_dissolveType = SBatcherInfo::EBDT_Shadowed;
						}
						else
						{
							batcherInfo.m_dissolveType = SBatcherInfo::EBDT_Regular;
						}

						// Also bind the dissolve pattern texture
						GpuApi::TextureRef dissolvePattern = GpuApi::GetInternalTexture( GpuApi::INTERTEX_DissolvePattern);
						GpuApi::BindTextures( PSSMP_Dissolve, 1, &dissolvePattern, GpuApi::PixelShader );
					}

					// Group by vertex type ( vertex shader )
					CRenderBatchByMeshVertexFactory byFactory( fragList );
					while ( byFactory.Pop( batchVertexFactory, fragList ) )
					{
						// Invalid vertex factory, do not draw
						if ( batchVertexFactory == MVF_Invalid )
						{
							// Failed to find vertex factory
							continue;
						}

						// Group by light channel
						CRenderBatchByLightChannel byLightChannel( fragList, shouldBatchByLightChannels );
						while ( byLightChannel.Pop( batchLightChannel, fragList ) )
						{
							if ( !context.CheckLightChannels( batchLightChannel ) )
							{
								continue;
							}

							// Include forced stencil bits to the light channels
							batchLightChannel |= context.m_lightChannelForcedMask;

							// Set lighting group
							if ( GpuApi::IsStencilLightChannelsContext( GpuApi::GetDrawContext() ) )
							{
								GpuApi::SetDrawContext( GpuApi::GetDrawContext(), batchLightChannel );
							}

							const Int32 currCharactersLightingBoost = IsCharactersLightingBoostRender( context ) && (batchLightChannel & LC_Characters) ? 1 : 0;

							if ( m_lastCharactersLightingBoost != currCharactersLightingBoost )
							{
								// Setup lighting boost
								if( ( m_lastCharactersLightingBoost = currCharactersLightingBoost ) )
								{
									const CEnvGlobalLightParametersAtPoint &globalLightParams = info.m_envParametersArea.m_globalLight;

									const Vector charactersLightingBoostValue ( 
										-globalLightParams.m_charactersLightingBoostAmbientLight.GetScalarClampMin( 0.0001f ),		// clamped to more than zero so that the wouldn't be any ambiguity on zero (negative values are used to recognize characters)		
										globalLightParams.m_charactersLightingBoostAmbientShadow.GetScalarClampMin( 0.f ),
										globalLightParams.m_charactersLightingBoostReflectionLight.GetScalarClampMin( 0.f ), 
										globalLightParams.m_charactersLightingBoostReflectionShadow.GetScalarClampMin( 0.f ) );

									stateManager.SetPixelConst( PSC_CharactersLightingBoost, charactersLightingBoostValue );
								}
								else
								{
									stateManager.SetPixelConst( PSC_CharactersLightingBoost, Vector::ONES );
								}		
							}

							CRenderBatchByClippingEllipse< CRenderElement_MeshChunk > byClippingEllipse( fragList );
							while ( byClippingEllipse.Pop( batchClippingEllipse, fragList ) )
							{
								// Group by mesh chunk
								Uint32 fragCount = 0;
								CRenderBatchByMeshChunkIndex byChunkIndex( fragList );
								while ( byChunkIndex.Pop( batchChunkIndex, fragCount, fragList ) )
								{
									ASSERT( batchChunkIndex < batchMesh->GetNumChunks() );

									Bool batchTwoSided;
									Float closestDistance = 0;
									CRenderBatchByTwoSided byTwoSided( fragList );
									while ( byTwoSided.Pop( batchTwoSided, fragList, closestDistance ) )
									{
										// Build batch
										Batch newBatch;
										newBatch.m_elements							= fragList;
										newBatch.m_numFragments						= fragCount;
										newBatch.m_chunkIndex			= batchChunkIndex;
										newBatch.m_material			= batchMaterial;
										newBatch.m_vertexFactory		= batchVertexFactory;
										newBatch.m_mesh				= batchMesh;
										newBatch.m_parameters			= batchMaterialParams;
										newBatch.m_isSkinned			= fragList->IsSkinnable();
										newBatch.m_isSelected			= batchSelection;
										newBatch.m_lightChannel		= batchLightChannel;
										newBatch.m_hasExtraStreams		= fragList->HasFlag( RMCF_UsesExtraStreams );
										newBatch.m_hasVertexCollapse	= fragList->HasFlag( RMCF_UsesVertexCollapse );
										newBatch.m_clippingEllipse		= batchClippingEllipse;
										newBatch.m_isTwoSided			= batchTwoSided || batchMaterial->IsTwoSided();
										newBatch.m_closestDistance					= closestDistance;
#ifndef RED_FINAL_BUILD
										// Modify batch that is going to be rendered
										batchModifier.ModifyBatch( &newBatch );
#endif

										// Set render states needed to render this batch
										//
										// NOTE : This will set some state based on RMCF_UsesUVDissolve, which isn't grouped explicitly
										// above. Not currently a problem, though, because UV Dissolve is only used for morphing, which
										// will be split up already by mesh (since each morph proxy uses a different render mesh).
										// Same with batch.m_elements->CanUseInstancing, which is covered by vertex factory and material.
										if ( SelectBatchRenderStates( newBatch, context, renderFlags ) )
										{
											batcherInfo.m_batch = &newBatch;
											batcherInfo.m_mesh	= newBatch.m_mesh;
											batcherInfo.m_batchList = newBatch.m_elements;									
											batcherInfo.m_chunkIndex = newBatch.m_chunkIndex;

#ifndef RED_FINAL_BUILD
											extern Bool GDumpMeshStats;
											if( GDumpMeshStats )
											{
												//DATABASE: STATS
												Bool isDuringShadowRendering = renderFlags & RMBF_Shadowmap;
												String msg = String::Printf( TXT("%ls;%d;%d;%d;%x;%ls;%ls\n"), newBatch.m_mesh->GetMeshDepotPath().AsChar(), newBatch.m_numFragments, newBatch.m_chunkIndex, context.m_pass, fragList->GetProxy(), batchMaterial->GetDisplayableName().AsChar(), isDuringShadowRendering ? TXT("shadowed") : TXT("lit") );

												String file = String::Printf( TXT("MeshStats.csv") );

												GFileManager->SaveStringToFile( file, msg, true );
											}
#endif
											// Render meshes
											DrawBatchOfMeshes( batcherInfo );
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// Disable the vertex texture
	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::VertexShader );

	// Disable characters boost
	{
		if ( -1 != m_lastCharactersLightingBoost && 0 != m_lastCharactersLightingBoost )
		{
			m_lastCharactersLightingBoost = 0;
			stateManager.SetPixelConst( PSC_CharactersLightingBoost, Vector ( 1, 1, 1, 1 ) );
		}
	}
}

void CRenderMeshBatcher::DrawBatchOfMeshes( SBatcherInfo& info )
{
	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Batcher flags

	Uint64 batchFlags = 0;
#ifndef NO_COMPONENT_GRAPH
	// Selection
	batchFlags |= SBatcherInfo::EBFT_Selection * info.m_batch->m_isSelected;
#endif
	// Skinning
	batchFlags |= SBatcherInfo::EBFT_Skinning * info.m_batch->m_isSkinned;

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

#ifndef RED_FINAL_BUILD
	info.m_outMeshStats->m_numBatches += 1;
	info.m_outMeshStats->m_biggestBatch = Max( info.m_outMeshStats->m_biggestBatch, info.m_batch->m_numFragments );
	info.m_outMeshStats->m_smallestBatch = Min( info.m_outMeshStats->m_smallestBatch, info.m_batch->m_numFragments );
#endif

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Clear instancing data
	info.m_batcher->m_lastProxy_Instanced = nullptr;

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Draw fragments
	CRenderElement_MeshChunk* frag = nullptr;
	CRenderBatchListIterator< CRenderElement_MeshChunk > listIterator( info.m_batchList );
	while ( listIterator.Pop( frag ) )
	{
		// Set the local transformation matrix for current proxy being rendered
		info.m_frag = frag;
		info.m_proxy = frag->GetProxy();

		//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
		Uint32 cacheFrag = frag->GetFlags();
		Uint64 fragFlags = 0;

		fragFlags |= SBatcherInfo::TransformFlag( cacheFrag , RMCF_UseColorShift ,		SBatcherInfo::EBFT_ColorShift );
		fragFlags |= SBatcherInfo::TransformFlag( cacheFrag , RMCF_NormalBlendMaterial , SBatcherInfo::EBFT_NormalBlend );
		fragFlags |= SBatcherInfo::TransformFlag( cacheFrag , RMCF_UseEffectParam0 ,	SBatcherInfo::EBFT_EffectParam0 );
		fragFlags |= SBatcherInfo::TransformFlag( cacheFrag , RMCF_UseEffectParam1 ,	SBatcherInfo::EBFT_EffectParam1 );
		fragFlags |= SBatcherInfo::TransformFlag( cacheFrag , RMCF_MaterialOverride ,	SBatcherInfo::EBFT_MaterialOveride );
		fragFlags |= SBatcherInfo::TransformFlag( cacheFrag , RMCF_UsesUVDissolve ,		SBatcherInfo::EBFT_DissolveUV );
		fragFlags |= SBatcherInfo::TransformFlag( cacheFrag , RMCF_MorphBlendMaterial ,	SBatcherInfo::EBFT_Morph );

		//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
		Uint64 proxyFrags = 0;

		proxyFrags |= SBatcherInfo::EBFT_ClippingElipse * info.m_proxy->HasClippingEllipse();

		//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

		SBatcherMacroNode::Run( batchFlags | fragFlags | proxyFrags , info );

		//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	}

	// Draw collected instances if any
	if ( info.m_batch->m_isSkinned && info.m_batcher->m_skinnedInstances.Size() > 0 )
	{
		info.m_batcher->DrawInstances( info.m_mesh, info.m_chunkIndex, info.m_batcher->m_skinnedInstances.Data(), info.m_batcher->m_skinnedInstances.Size(), sizeof( SSkinnedMeshInstanceDescriptor ), *info.m_outMeshStats );
		info.m_batcher->m_skinnedInstances.ClearFast();
	}
	else if ( info.m_batcher->m_instances.Size() > 0 )
	{
		info.m_batcher->DrawInstances( info.m_mesh, info.m_chunkIndex, info.m_batcher->m_instances.Data(), info.m_batcher->m_instances.Size(), sizeof( SMeshInstanceDescriptor ), *info.m_outMeshStats );
		info.m_batcher->m_instances.ClearFast();
	}
}

void CRenderMeshBatcher::DrawInstances( CRenderMesh* mesh, Uint32 chunkIndex, void* bufferData, Uint32 numInstances, Uint32 streamStride, MeshDrawingStats& outMeshStats )
{
	Uint32 bufferSize = numInstances * streamStride;

	Uint32 maxBufferSize = MESH_INSTANCING_BUFFER_SIZE * sizeof( SMeshInstanceDescriptor );
	
	if ( numInstances != 0 && bufferSize <= maxBufferSize )
	{
		void *instancedPtr = nullptr;

		// Lock the instance buffer for write
		if ( m_instancesDataElemOffset + bufferSize >= maxBufferSize )
		{
			m_instancesDataElemOffset = 0;
			instancedPtr = GpuApi::LockBuffer( m_instancesBufferRef, GpuApi::BLF_Discard, m_instancesDataElemOffset, bufferSize );
		}
		else
		{
			instancedPtr = GpuApi::LockBuffer( m_instancesBufferRef, GpuApi::BLF_NoOverwrite, m_instancesDataElemOffset, bufferSize );
		}

		Red::System::MemoryCopy( instancedPtr, bufferData, bufferSize );

		// Return the buffer
		GpuApi::UnlockBuffer( m_instancesBufferRef );

		// Bind
		GpuApi::BindVertexBuffers( 7, 1, &m_instancesBufferRef, &streamStride, &m_instancesDataElemOffset );

		// Draw
		mesh->DrawChunkNoBindInstanced( chunkIndex, 1, numInstances, outMeshStats );

		// Update offset in instances buffer
		m_instancesDataElemOffset += bufferSize;

#ifndef RED_FINAL_BUILD
		if ( numInstances > 1 ) outMeshStats.m_numInstancedBatches += 1;
		outMeshStats.m_biggestInstancedBatch = Max( outMeshStats.m_biggestInstancedBatch, numInstances );
		outMeshStats.m_smallestInstancedBatch = Min( outMeshStats.m_smallestInstancedBatch, numInstances );
#endif
	}
}

Bool CRenderMeshBatcher::SelectBatchRenderStates( const Batch& newBatch, const RenderingContext& context, Uint32 renderFlags )
{
	const Bool isDuringShadowsRender = 0 != ( renderFlags & RMBF_Shadowmap );

	const Bool isMasked			= newBatch.m_parameters->IsMasked();
	const Bool useDissolve		= !context.m_forceNoDissolves && newBatch.m_elements->HasFlag( (renderFlags & RMBF_CascadeShadows) ? RMCF_UsesCascadesDissolve : RMCF_UsesDissolve );
	const Bool useUVDissolve	= newBatch.m_elements->HasFlag(RMCF_UsesUVDissolve);

	// Prepare material context
	MaterialRenderingContext materialContext( context );
	materialContext.m_vertexFactory			= newBatch.m_vertexFactory;
	materialContext.m_selected				= newBatch.m_isSelected;
	materialContext.m_uvDissolveSeparateUV	= newBatch.m_elements->DoesUVDissolveUseSeparateTexCoord();
	materialContext.m_materialDebugMode		= context.m_materialDebugMode;
	materialContext.m_hasExtraStreams		= newBatch.m_hasExtraStreams;
	materialContext.m_hasVertexCollapse		= newBatch.m_hasVertexCollapse;
	materialContext.m_useInstancing			= newBatch.m_elements->CanUseInstancing();
	materialContext.m_discardingPass		= isMasked || newBatch.m_clippingEllipse || useDissolve || useUVDissolve;

	if ( materialContext.m_discardingPass )
	{
		Vector discardFlags(
			isMasked ? 1.0f : 0.0f,
			useDissolve ? 1.0f: 0.0f,
			useUVDissolve ? 1.0f : 0.0f,
			newBatch.m_clippingEllipse ? 1.0f : 0.0f
			);
		GetRenderer()->GetStateManager().SetPixelConst( PSC_DiscardFlags, discardFlags );
	}

#ifndef RED_FINAL_BUILD
	if ( materialContext.m_discardingPass && materialContext.m_uvDissolveSeparateUV )
	{
		RED_ASSERT( newBatch.m_material->CanUseOnMorphMeshes(), TXT("Material that is not selected for Morphing bound with morphed chunks.") );
	}
#endif

	CRenderMaterial::CompiledTechnique* technique = nullptr;
	if ( newBatch.m_material->BindShaders( materialContext, technique ) )
	{
		if ( !newBatch.m_material->BindParams( materialContext, newBatch.m_parameters, technique, newBatch.m_closestDistance ) )
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

	if ( !isDuringShadowsRender || context.m_pass == RP_ShadowDepthMasked )
	{
		streamsToBind |= RMS_TangentFrame | RMS_Extended;
	}
	else if ( (newBatch.m_hasVertexCollapse || newBatch.m_material->GetRenderSortGroup() == RSG_Hair) && newBatch.m_hasExtraStreams )
	{
		// If we're using vertex collapse, we always need to bind the extended stream, to get vertex color.
		// hair needs extra stream during shadow pass for "animate" in material (VertexColor is used to animate hair tips based on texture alpha).
		// Disabling this is impossible for the time being as material uses extra streams, just no "extended" binding happens.
		streamsToBind |= RMS_Extended;
	}

	newBatch.m_mesh->Bind( newBatch.m_chunkIndex, streamsToBind );

	if ( newBatch.m_hasVertexCollapse )
	{
		GpuApi::TextureRef boneTex = newBatch.m_mesh->GetBonePositionTexture();
		GpuApi::BindTextures( 1, 1, &boneTex, GpuApi::VertexShader );
	}
	
	if ( materialContext.m_discardingPass )
	{
		CRenderTexture* uvDissolveTexture = nullptr;
		if ( newBatch.m_elements->GetProxy()->GetType() == RPT_Mesh && useUVDissolve )
		{
			uvDissolveTexture = newBatch.m_elements->GetUVDissolveTexture();
		}

		if ( uvDissolveTexture )
		{
			uvDissolveTexture->Bind( PSSMP_UVDissolve, GpuApi::SAMPSTATEPRESET_WrapLinearMip, RST_PixelShader );
		}
		else
		{
			GpuApi::TextureRef dissolvePattern = GpuApi::GetInternalTexture( GpuApi::INTERTEX_DissolvePattern);
			GpuApi::BindTextures( PSSMP_UVDissolve, 1, &dissolvePattern, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( PSSMP_UVDissolve, GpuApi::SAMPSTATEPRESET_WrapLinearNoMip );
		}
	}

	// Set two sided
	GpuApi::SetForcedTwoSidedRender( newBatch.m_isTwoSided );


	// Can render
	return true;
}

void CRenderMeshBatcher::OnDeviceLost()
{
	// Do nothing
}

void CRenderMeshBatcher::OnDeviceReset()
{
	// Do nothing
}

CName CRenderMeshBatcher::GetCategory() const
{
	return CNAME( RenderMeshBatcher );
}

Uint32 CRenderMeshBatcher::GetUsedVideoMemory() const
{
	return 0;
}

void CRenderMeshBatcher::ComputeLocalToWorld( const RenderingContext& context, const IRenderProxyDrawable* proxy, const CRenderElement_MeshChunk* fragment, Matrix& outMatrix ) const
{
	outMatrix = proxy->GetLocalToWorld();
}

void CRenderMeshBatcher::InitCsCullingBuffers()
{
	// Create custom constant buffer
	if( !m_customCsCB )
	{	
		m_customCsCB = GpuApi::CreateBuffer( sizeof( SCsCullCB ), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		RED_FATAL_ASSERT( m_customCsCB, "Couldn't create custom CS constant buffer for merged meshes rendering!" );
	}
}

void CRenderMeshBatcher::ReleaseCsCullingBuffers()
{
	GpuApi::SafeRelease( m_customCsCB );
}
