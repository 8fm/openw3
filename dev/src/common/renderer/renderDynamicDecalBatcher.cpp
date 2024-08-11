/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderDynamicDecalBatcher.h"
#include "renderDynamicDecalChunk.h"
#include "renderDynamicDecal.h"
#include "renderProxyDrawable.h"
#include "renderRenderSurfaces.h"
#include "renderShaderPair.h"
#include "renderSkinningData.h"


struct ChunkSortKey
{
	CRenderDynamicDecalChunk*	m_chunk;
	CRenderShaderTriple*		m_shader;

	ChunkSortKey(){}

	ChunkSortKey( CRenderDynamicDecalChunk* chunk )
	{
		m_chunk		= chunk;
		// Cache shader, since it takes a bit of work to evaluate.
		m_shader	= chunk->GetShader();
	}

	Bool operator <( const ChunkSortKey& key ) const
	{
		// First group shaders together. Above all else, we want to minimize the number of times we change shader.
		if ( m_shader != key.m_shader )
		{
			return m_shader < key.m_shader;
		}

		// Next, chunks from the same decal. Means textures and many parameters will be the same.
		if ( m_chunk->GetOwnerDecal() != key.m_chunk->GetOwnerDecal() )
		{
			return m_chunk->GetOwnerDecal() < key.m_chunk->GetOwnerDecal();
		}

		// Next, by proxy. This implicitly puts the same skinning and clipping ellipse together.
		if ( m_chunk->GetTargetProxy() != key.m_chunk->GetTargetProxy() )
		{
			return m_chunk->GetTargetProxy() < key.m_chunk->GetTargetProxy();
		}
		return m_chunk < key.m_chunk;
	}
};


CRenderDynamicDecalBatcher::CRenderDynamicDecalBatcher()
{
}

CRenderDynamicDecalBatcher::~CRenderDynamicDecalBatcher()
{
}

void CRenderDynamicDecalBatcher::RenderDecalChunks( const CRenderFrameInfo& frameInfo, const RenderingContext& context, const TDynArray< CRenderDynamicDecalChunk* >& chunks )
{
	PC_SCOPE_PIX(RenderDynamicDecals);


	// If there were no decals collected, early out.
	if ( chunks.Size() == 0 )
	{
		return;
	}

	GpuApi::RenderTargetSetup rtSetupOld = GpuApi::GetRenderTargetSetup();

	// Use existing RT setup, except forcing a read-only depth buffer. We want to read depth in the shader and fade out based on
	// distance difference between the decal and underlying mesh. This is of course less ideal, but allows us to deal with "animated"
	// vertices, where the mesh's vertex shader animates it, as well as variation from using a different LOD.

	GpuApi::RenderTargetSetup rtSetupDecals = rtSetupOld;
	rtSetupDecals.depthTargetReadOnly = true;
	GpuApi::SetupRenderTargets( rtSetupDecals );

	// Bind the scene normals, so that we can use existing normals when rendering the decals.
	// Bind the scene depth, so we can discard pixels that aren't directly on a surface
	CRenderSurfaces* surfaces = GetRenderer()->GetSurfaces();
	Bool msaa = GetRenderer()->IsMSAAEnabled( frameInfo );
	GpuApi::TextureRef gbufferNormals = surfaces->GetRenderTargetTex( msaa ? RTN_GBuffer1MSAA : RTN_GBuffer1 );
	GpuApi::TextureRef sceneDepthStencil = msaa ? surfaces->GetDepthBufferTexMSAA() : surfaces->GetDepthBufferTex();

	// 0 and 1 are used for diffuse/normal.
	GpuApi::TextureRef textures[] = { gbufferNormals, sceneDepthStencil };
	GpuApi::BindTextures( 2, 2, textures, GpuApi::PixelShader );
	GpuApi::BindTextureStencil( 4, sceneDepthStencil, GpuApi::PixelShader );

	// No depth test, default culling, premul blending.
	CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_DynamicDecalPremulBlend );



	TSortedArray< ChunkSortKey > sortedChunks;
	{
		PC_SCOPE_PIX(RenderDynamicDecals_SortChunks);

		// Simple sorting of the decal chunks... Nothing fancy going on here, would be nice if we could sort the chunks directly instead of
		// building a new array of wrapper structs... But, we're sorting by shader, and GetShader() is non-trivial. Chunks could maybe cache
		// their shader?
		sortedChunks.Reserve( chunks.Size() );
		for ( Uint32 i = 0; i < chunks.Size(); ++i )
		{
			CRenderDynamicDecalChunk* chunk = chunks[i];
			sortedChunks.Insert( ChunkSortKey( chunk ) );
		}
	}


	// Track the constants we're setting, so we don't do it multiple times.
	CRenderDynamicDecal* lastDecal = nullptr;
	CRenderSkinningData* lastSkinning = nullptr;
	const SRenderProxyDrawableClippingEllipseParams* lastClippingEllipse = nullptr;

	for ( Uint32 i = 0; i < sortedChunks.Size(); ++i )
	{
		ChunkSortKey& chunkKey = sortedChunks[i];

		CRenderDynamicDecal* decal = chunkKey.m_chunk->GetOwnerDecal();
		RED_ASSERT( decal != nullptr, TXT("DynDecal chunk without an owner decal!") );
		if ( decal == nullptr )
		{
			continue;
		}

		CRenderSkinningData* skinningData = chunkKey.m_chunk->GetSkinningData();
		const SRenderProxyDrawableClippingEllipseParams* clippingEllipse = chunkKey.m_chunk->GetClippingEllipseParams();

		// RenderStateManager takes care of redundant calls
		if ( chunkKey.m_shader != nullptr )
		{
			chunkKey.m_shader->Bind();
#ifdef DYNAMIC_DECAL_NO_GS_CULLING
			GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_GeometryShader );
#endif
		}

		if ( decal != lastDecal )
		{
			decal->BindRenderParameters();
			lastDecal = decal;
		}

		// Shouldn't really need to sort by this, because it should come from the target proxy, which we've already grouped.
		if ( clippingEllipse != lastClippingEllipse )
		{
			if ( clippingEllipse != nullptr )
			{
				GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_Matrix, clippingEllipse->m_localToEllipse );
			}
			else
			{
				// This matrix will force the clipping positions to (1,1,1), outside the ellipse.
				// TODO : Instead of this, could have a separate shader for non-clipped. Then we wouldn't need to update it in most cases.
				GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_Matrix, Matrix(
					Vector( 0, 0, 0, 0 ),
					Vector( 0, 0, 0, 0 ),
					Vector( 0, 0, 0, 0 ),
					Vector( 1, 1, 1, 1 )
					) );
			}

			lastClippingEllipse = clippingEllipse;
		}
		// Same as clipping ellipse, this should come from proxies, and we've those grouped.
		if ( skinningData != lastSkinning )
		{
			if ( skinningData != nullptr )
			{
				skinningData->Bind();
			}
			lastSkinning = skinningData;
		}

		chunkKey.m_chunk->Render( context, frameInfo );
	}


	GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_GeometryShader );

	GpuApi::BindTextures( 2, 3, nullptr, GpuApi::PixelShader ); //< stencil texture included

	GpuApi::SetupRenderTargets( rtSetupOld );
}

void CRenderDynamicDecalBatcher::OnDeviceLost()
{
}

void CRenderDynamicDecalBatcher::OnDeviceReset()
{
}

CName CRenderDynamicDecalBatcher::GetCategory() const
{
	return RED_NAME( RenderDynamicDecalBatcher );
}
