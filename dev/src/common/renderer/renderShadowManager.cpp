/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderShadowDynamicAllocator.h"
#include "renderShadowStaticAllocator.h"
#include "renderShadowManager.h"
#include "renderShadowRegions.h"
#include "renderProxyDrawable.h"
#include "renderProxyMesh.h"
#include "renderProxyApex.h"
#include "renderScene.h"
#include "renderMeshBatcher.h"
#include "renderApexBatcher.h"
#include "../engine/renderFragment.h"

static RED_INLINE Int32 AlignUp( Int32 size, Int32 alignment )
{
	return (size + (alignment - 1)) & ~( alignment - 1 );
}

CRenderShadowManager::CRenderShadowManager()
{
	const Uint32 maxTextureSize = DYNAMIC_SHADOWMAP_RESOLUTION;
	const Uint32 maxCubeSize = STATIC_SHADOWMAP_RESOLUTION;
	const Uint32 numSlices = NUM_DYNAMIC_SLICES;
	const Uint32 numCubes = NUM_STATIC_CUBES;

	// Create dynamic shadow allocator
	m_dynamicAllocator = new CRenderShadowDynamicAllocator( maxTextureSize, numSlices );

	// Create static ( cube ) shadow allocator
	m_staticAllocator = new CRenderShadowStaticAllocator( maxCubeSize, numCubes );

	// Create the depth buffer
	GpuApi::TextureDesc desc;
	desc.sliceNum		= 1;
	desc.initLevels		= 1;
	desc.width			= maxTextureSize;
	desc.height			= maxTextureSize;
	desc.type			= GpuApi::TEXTYPE_2D;
	desc.format			= GpuApi::TEXFMT_D16U;
	desc.usage			= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_DepthStencil | GpuApi::TEXUSAGE_ESRAMResident;

	const Uint32 alignment = 64 * 1024;

	// Last memory space in ESRAM
	desc.esramSize		= maxTextureSize * maxTextureSize * 2;
	desc.esramOffset	= AlignUp( 32 * 1024*1024 - desc.esramSize - alignment , alignment );

	m_sharedDepth = GpuApi::CreateTexture( desc, GpuApi::TEXG_Shadow );
	GpuApi::SetTextureDebugPath( m_sharedDepth, "localShadowDepth" );

}

CRenderShadowManager::~CRenderShadowManager()
{
	// Reset packing kere to release any potential references
	ResetDynamicPacking();
	ResetStaticPacking();

	GpuApi::SafeRelease( m_sharedDepth );

	// Delete allocators
	delete m_dynamicAllocator;
	delete m_staticAllocator;
}

GpuApi::TextureRef CRenderShadowManager::GetDynamicAtlas() const
{
	return m_dynamicAllocator->GetTexture();
}

GpuApi::TextureRef CRenderShadowManager::GetStaticAtlas() const
{
	return m_staticAllocator->GetTexture();
}

void CRenderShadowManager::ResetDynamicPacking()
{
	// Reset dynamic allocator
	m_dynamicAllocator->Reset();

	// Release references
	for ( Uint32 i=0; i<m_drawProxiesDynamic.Size(); ++i )
	{
		m_drawProxiesDynamic[i]->Release();
	}

	// Release regions
	for ( Uint32 i=0; i<m_drawListDynamic.Size(); ++i )
	{
		m_drawListDynamic[i].m_region->Release();
	}

	// Reset dynamic update list
	m_drawListDynamic.ClearFast();
	m_drawProxiesDynamic.ClearFast();
}

void CRenderShadowManager::ResetStaticPacking()
{
	// Release references
	for ( Uint32 i=0; i<m_drawProxiesStatic.Size(); ++i )
	{
		m_drawProxiesStatic[i]->Release();
	}

	// Reset static update list
	m_drawListStatic.ClearFast();
	m_drawProxiesStatic.ClearFast();
}

CRenderShadowStaticCube* CRenderShadowManager::AllocateCube( Uint32 currentFrame, const Box& lightBounds )
{
	// Use the static allocator
	return m_staticAllocator->AllocateCube( currentFrame, lightBounds );
}

void CRenderShadowManager::InvalidateStaticLights( const Box& box )
{
	// Invalidate static cubes
	m_staticAllocator->InvalidateBounds( box );
}

CRenderShadowDynamicRegion* CRenderShadowManager::AllocateDynamicRegion( Uint16 size )
{
	// Use the dynamic allocator
	return m_dynamicAllocator->Allocate( size );
}

void CRenderShadowManager::AddDynamicRegionToRender( CRenderShadowDynamicRegion* region, const CRenderCamera& shadowCamera, const TDynArray< IRenderProxyDrawable* >& dynamicProxies )
{
	ASSERT( !dynamicProxies.Empty() );

	// Append the proxies to global list
	const Uint32 firstProxy = m_drawProxiesDynamic.Size();
	m_drawProxiesDynamic.PushBack( dynamicProxies );

	// Create draw entry
	DrawEntryDynamic* entry = new ( m_drawListDynamic ) DrawEntryDynamic;
	entry->m_camera = shadowCamera;
	entry->m_region = region;
	entry->m_firstProxy = firstProxy;
	entry->m_numProxies = dynamicProxies.Size();

	// Keep reference to region
	region->AddRef();

	// Keep references to proxies - Added by Dex: I think that this is superficial but I'm trying to track a bug here.
	for ( Uint32 i=0; i<dynamicProxies.Size(); ++i )
	{
		IRenderObject* proxy = dynamicProxies[i];
		proxy->AddRef();
	}
}

void CRenderShadowManager::AddStaticCubeToRender( CRenderShadowStaticCube* cube, const Vector& lightPos, Float lightRadius, const TDynArray< IRenderProxyDrawable* >& staticProxies )
{
	// Append the proxies to global list
	const Uint32 firstProxy = m_drawProxiesStatic.Size();
	m_drawProxiesStatic.PushBack( staticProxies );

	// Create draw entry
	DrawEntryStatic* entry = new ( m_drawListStatic ) DrawEntryStatic;
	entry->m_cube = cube;
	entry->m_lightPosition = lightPos;
	entry->m_lightRadius = lightRadius;
	entry->m_firstProxy = firstProxy;
	entry->m_numProxies = staticProxies.Size();

	// Keep references to proxies - Added by Dex: I think that this is superficial but I'm trying to track a bug here.
	for ( Uint32 i=0; i<staticProxies.Size(); ++i )
	{
		IRenderObject* proxy = staticProxies[i];
		proxy->AddRef();
	}
}

void CRenderShadowManager::RenderDynamicShadowmaps( const CRenderCollector& collector )
{
	PC_SCOPE_RENDER_LVL0( RenderDynamicShadowmaps );

	// Nothing to draw
	if ( m_drawListDynamic.Empty() )
	{
		return;
	}

	// Texture size
	const Float textureSize = (Float)( GpuApi::GetTextureDesc( m_dynamicAllocator->GetTexture() ).width );

	// Check how many slices we need to update
	Uint16 maxSliceIndex = 0;
	for ( Uint32 i=0; i<m_drawListDynamic.Size(); ++i )
	{
		const DrawEntryDynamic& entry = m_drawListDynamic[i];
		maxSliceIndex = Max<Uint16>( maxSliceIndex, entry.m_region->GetSlice() );
	}

	// Draw to shadowmap slices
	for ( Uint16 i=0; i<=maxSliceIndex; ++i )
	{
		// Bind render targets
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetDepthStencilTarget( GetDepth() );
		rtSetup.SetViewportFromTarget( GetDepth() );
		GpuApi::SetupRenderTargets( rtSetup );

		// Clear at first use
		{
			ASSERT( !GpuApi::IsReversedProjectionState() );
			GetRenderer()->ClearDepthTarget( 1.0f );
		}

		// Process each draw entry that renders to this target 
		// TODO: this visits the list to many times
		Uint32 numRegionsDrawn = 0;
		DrawEntryDynamic* drawnRegions = nullptr;
		for ( Uint32 j = 0; j < m_drawListDynamic.Size(); ++j )
		{
			// Mesh stats for dynamic shadowmaps
			MeshDrawingStats meshStats;

			DrawEntryDynamic& entry = m_drawListDynamic[ j ];
			if ( entry.m_region->GetSlice() != i )
			{
				// not this array texture slice
				continue;
			}			
			
			// Process the proxies to get the final list of fragments to render
			SShadowCascade& localData = collector.m_renderCollectorData->m_cubeFaceFragments;
			{
				PC_SCOPE( Collect );

				localData.Reset();
				
				for ( Uint32 k = 0; k < entry.m_numProxies; ++k )
				{
					IRenderProxyDrawable* proxy = m_drawProxiesDynamic[ entry.m_firstProxy + k ];
					RED_FATAL_ASSERT( proxy && proxy->GetScene(), "Ivalid proxy encountered when rendering local Shadowmap." );
					RED_FATAL_ASSERT( proxy->IsCastingShadowsFromLocalLightsOnly() || proxy->CanCastShadows(), "How did proxy that does not cast shadows end up here?" );
					if ( proxy->GetType() == RPT_Mesh )
					{
						CRenderProxy_Mesh* meshProxy = static_cast< CRenderProxy_Mesh* >( proxy );
						meshProxy->CollectLocalShadowElements( collector, entry.m_camera, localData );
					}
#ifdef USE_APEX
					else if ( proxy->GetType() == RPT_Apex )
					{
						CRenderProxy_Apex* apexProxy = static_cast< CRenderProxy_Apex* >( proxy );
						apexProxy->CollectLocalShadowElements( collector, entry.m_camera, localData );			
					}
#endif
				}
			}

			// Anything to draw ?
			const Uint32 numElements = localData.GetSolidElements().Size() + localData.GetDiscardElements().Size() + localData.GetApexElements().Size();
			if ( numElements > 0 ) //< TODO in case of zero elements the whole region should not be created, but that's would need too big refactor for now..
			{				
				// Get original render camera and expand it's FOV to cover extra pixels
				const Uint32 baseSize = entry.m_region->GetSize();
				const Float expandFraction = (Float)(baseSize) / (Float)( baseSize + (2*SHADOW_BORDER_SIZE) );
				CRenderCamera camera(
					entry.m_camera.GetPosition(),
					entry.m_camera.GetRotation(),
					entry.m_camera.GetFOV(),
					1.0f,
					0.05f,
					entry.m_camera.GetFarPlane(),
					expandFraction );

				// Set the viewport for this region
				{
					GpuApi::ViewportDesc vp;
					vp.width = entry.m_region->GetSize() + (2*SHADOW_BORDER_SIZE);
					vp.height = entry.m_region->GetSize() + (2*SHADOW_BORDER_SIZE);
					vp.x = entry.m_region->GetOffsetX() - SHADOW_BORDER_SIZE;
					vp.y = entry.m_region->GetOffsetY() - SHADOW_BORDER_SIZE;
					vp.minZ = 0.0f;
					vp.maxZ = 1.0f;
					GpuApi::SetViewport( vp );
				}

				{
					PC_SCOPE( Render );

					// This all collected proxies so far..
					if( collector.m_scene )
					{
						collector.m_scene->TickCollectedProxies();
					}
					// Set draw context
					// TODO: this for some reason cannot be moved outside :(
					CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_ShadowMapGenCSM_DepthTex );
		
					// Bind camera
					GetRenderer()->GetStateManager().SetCamera( camera );
					GetRenderer()->GetStateManager().SetLocalToWorld( NULL );

					// Draw fragments
					RenderingContext theShadowDrawingContext( entry.m_camera );
					theShadowDrawingContext.m_pass = RP_ShadowDepthSolid;

					GetRenderer()->GetMeshBatcher()->RenderMeshes( collector.GetRenderFrameInfo(), theShadowDrawingContext, localData.GetSolidElements(), RMBF_Shadowmap, meshStats );

					theShadowDrawingContext.m_pass = RP_ShadowDepthMasked;
#ifdef USE_APEX
					GetRenderer()->GetApexBatcher()->RenderApex( collector.GetRenderFrameInfo(), theShadowDrawingContext, localData.GetApexElements(), RABF_All, meshStats );
#endif
					GetRenderer()->GetMeshBatcher()->RenderMeshes( collector.GetRenderFrameInfo(), theShadowDrawingContext, localData.GetDiscardElements(), RMBF_Shadowmap, meshStats );
				}
			}

#ifndef RED_FINAL_BUILD
			// Update stats
			{
				extern SceneRenderingStats GRenderingStats;
				GRenderingStats.m_numDynamicShadowsRegions += 1;
				GRenderingStats.m_numDynamicShadowsProxies += entry.m_numProxies;
				GRenderingStats.m_numDynamicShadowsChunks += meshStats.m_numChunks;
				GRenderingStats.m_numDynamicShadowsTriangles += meshStats.m_numTriangles;
			}
#endif

			// Filter only the rendered regions
			numRegionsDrawn += 1;
			entry.m_pointer = drawnRegions;
			drawnRegions = &entry;
		}

		// Filter the slices
		if ( numRegionsDrawn > 0 )
		{
			// Bind render targets
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, m_dynamicAllocator->GetTexture(), i );
			rtSetup.SetViewportFromTarget( m_dynamicAllocator->GetTexture() );
			GpuApi::SetupRenderTargets( rtSetup );

			// Allocate temporary vertices
			DebugVertexUV* drawPoints = (DebugVertexUV*) RED_ALLOCA( sizeof(DebugVertexUV) * 6 * numRegionsDrawn );

			// Fill the regions
			DebugVertexUV* writePoint = drawPoints;
			for ( DrawEntryDynamic* cur=drawnRegions; cur; cur=cur->m_pointer, writePoint += 6 )
			{
				// Directions
				// Compute offline tanget value for given FOV and pack it into BA components of color
				Uint32 fovAngle = static_cast<Uint32>( MTan( cur->m_camera.GetFOV() * 0.5f * ( M_PI / 180.0f ) ) * 1024 );
				Uint8 encodeA = fovAngle >> 8;
				Uint8 encodeB = fovAngle &  0xFF;
				Color color00(   0,   0, encodeA, encodeB );
				Color color01(   0, 255, encodeA, encodeB );
				Color color10( 255,   0, encodeA, encodeB );
				Color color11( 255, 255, encodeA, encodeB );

				// Calculate positions
				const Uint32 size = cur->m_region->GetSize();
				const Float x0 = (Float)(cur->m_region->GetOffsetX()-1) / textureSize;
				const Float y0 = (Float)(cur->m_region->GetOffsetY()-1) / textureSize;
				const Float x1 = (Float)(cur->m_region->GetOffsetX()+size+2) / textureSize;
				const Float y1 = (Float)(cur->m_region->GetOffsetY()+size+2) / textureSize;

				const Float zf = cur->m_camera.GetFarPlane();
				const Float zn = 0.05f;
				const Float nf = zn * zf;

				// Assemble vertices
				const Float pixelOffset = -2.0f;
				const Float radius = cur->m_camera.GetFarPlane();
				writePoint[0].Set( Vector( x0, y0, 0.50f ), color00, pixelOffset, nf );
				writePoint[1].Set( Vector( x1, y0, 0.50f ), color10, pixelOffset, nf );
				writePoint[2].Set( Vector( x1, y1, 0.50f ), color11, pixelOffset, nf );
				writePoint[3].Set( Vector( x0, y0, 0.50f ), color00, pixelOffset, nf );
				writePoint[4].Set( Vector( x1, y1, 0.50f ), color11, pixelOffset, nf );
				writePoint[5].Set( Vector( x0, y1, 0.50f ), color01, pixelOffset, nf );
			}

			// Filter all the regions at once
			GetRenderer()->GetDebugDrawer().DrawDepthBufferPatchWithGaussN( GetDepth(), drawPoints, 6*numRegionsDrawn);
			GpuApi::BindTextures( 0, 2, nullptr, GpuApi::PixelShader );
		}
	}
}

void CRenderShadowManager::RenderStaticShadowmaps( const CRenderCollector& collector )
{
	PC_SCOPE_RENDER_LVL0( RenderStaticShadowmaps );

	// Get cubemap size
	const Uint32 baseSize = GpuApi::GetTextureDesc( m_staticAllocator->GetTexture() ).width;

	// Update the cubes
	for ( Uint16 i = 0; i < m_drawListStatic.Size(); ++i )
	{
		const DrawEntryStatic& entry = m_drawListStatic[ i ];

		// Mesh stats
		MeshDrawingStats meshStats;

		// For each face
		for ( Uint16 j = 0; j < 6; ++j )
		{
			// Bind render targets
			{
				GpuApi::RenderTargetSetup rtSetup;
				rtSetup.SetDepthStencilTarget( GetDepth() );
				rtSetup.SetViewport( baseSize + 2*SHADOW_BORDER_SIZE, baseSize + 2*SHADOW_BORDER_SIZE );
				GpuApi::SetupRenderTargets( rtSetup );
			}

			// Clear at first use
			{
				ASSERT( !GpuApi::IsReversedProjectionState() );
				GetRenderer()->ClearDepthTarget( 1.0f );
			}

			// Get cubemap face rotation
			// THIS IS A HAND TWEEKED LIST, DO NOT CHANGE
			const Float radius = entry.m_lightRadius;
			EulerAngles cameraRotation;

			switch ( j )
			{
				case 0: cameraRotation = EulerAngles(-90,0,90);		break; // +X
				case 1: cameraRotation = EulerAngles(90,0,-90);		break; // -X
				case 2: cameraRotation = EulerAngles(0,0,180);		break; // +Y
				case 3: cameraRotation = EulerAngles(180,0,0);		break; // -Y
				case 4: cameraRotation = EulerAngles(180,-90,0);	break; // +Z
				case 5: cameraRotation = EulerAngles(0,90,0);		break; // -Z
			}					

			// Get original render camera and expand it's FOV to cover extra pixels
			const Float expandFraction = (Float)(baseSize) / (Float)( baseSize + (2*SHADOW_BORDER_SIZE) );
			const CRenderCamera camera(
				entry.m_lightPosition,
				cameraRotation,
				90.0f,
				1.0f,
				0.05f,
				radius,
				expandFraction );			

			// Process the proxies to get the final list of fragments to render
			SShadowCascade& localData = collector.m_renderCollectorData->m_cubeFaceFragments;
			{
				PC_SCOPE( Collect );

				localData.Reset();
				for ( Uint32 k = 0; k < entry.m_numProxies; ++k )
				{
					IRenderProxyDrawable* proxy = m_drawProxiesStatic[ entry.m_firstProxy + k ];
					RED_FATAL_ASSERT( proxy->IsCastingShadowsFromLocalLightsOnly() || proxy->CanCastShadows(), "How did a proxy that does not cast shadows end up here?" );
					if ( proxy->GetType() == RPT_Mesh )
					{
						CRenderProxy_Mesh* meshProxy = static_cast<CRenderProxy_Mesh*>( proxy );
						meshProxy->CollectStaticShadowElements( collector, camera, localData );
					}
				}
			}

			// Entything to draw ?
			const Uint32 numElements = localData.GetSolidElements().Size() + localData.GetDiscardElements().Size();
			if ( numElements > 0 )
			{
				PC_SCOPE( Render );

				// This all collected proxies so far..
				if( collector.m_scene )
				{
					collector.m_scene->TickCollectedProxies();
				}
				// Set draw context
				// TODO: this for some reason cannot be moved outside :(
				CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_ShadowMapGenCSM_DepthTex );
		
				// Bind camera
				GetRenderer()->GetStateManager().SetCamera( camera );
				GetRenderer()->GetStateManager().SetLocalToWorld( NULL );

				// Draw fragments
				RenderingContext theShadowDrawingContext( camera );
				theShadowDrawingContext.m_pass = RP_ShadowDepthSolid;
				GetRenderer()->GetMeshBatcher()->RenderMeshes( collector.GetRenderFrameInfo(), theShadowDrawingContext, localData.GetSolidElements(), RMBF_Shadowmap, meshStats );

				theShadowDrawingContext.m_pass = RP_ShadowDepthMasked;
				GetRenderer()->GetMeshBatcher()->RenderMeshes( collector.GetRenderFrameInfo(), theShadowDrawingContext, localData.GetDiscardElements(), RMBF_Shadowmap, meshStats );
			}

			// Filter
			{
				// Calculate cube and render target index
				const Uint16 cubeIndex = entry.m_cube->GetIndex();
				const Uint16 rtIndex = GpuApi::CalculateCubemapSliceIndex( GpuApi::GetTextureDesc( m_staticAllocator->GetTexture() ), cubeIndex, j, 0 );

				// Set draw context
				CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );

				// Set final render target in the cubemap
				GpuApi::RenderTargetSetup rtSetup;
				rtSetup.SetColorTarget( 0, m_staticAllocator->GetTexture(), rtIndex );
				rtSetup.SetViewportFromTarget( m_staticAllocator->GetTexture() );
				GpuApi::SetupRenderTargets( rtSetup );

				// Draw the filtering shader
				GetRenderer()->GetDebugDrawer().DrawDepthBufferPatchWithGauss( GetDepth(), SHADOW_BORDER_SIZE, radius );
			}
		}
#ifndef RED_FINAL_BUILD
		// Update stats
		{
			extern SceneRenderingStats GRenderingStats;
			GRenderingStats.m_numStaticShadowsCubes += 1;
			GRenderingStats.m_numStaticShadowsProxies += entry.m_numProxies;
			GRenderingStats.m_numStaticShadowsChunks += meshStats.m_numChunks;
			GRenderingStats.m_numStaticShadowsTriangles += meshStats.m_numTriangles;
		}
#endif
	}
}
