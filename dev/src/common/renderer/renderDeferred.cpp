/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "gameplayFx.h"
#include "renderCollector.h"
#include "renderScene.h"
#include "renderPostProcess.h"
#include "renderMeshBatcher.h"
#include "renderTexture.h"
#include "renderTextureArray.h"
#include "renderRenderSurfaces.h"
#include "renderElementApex.h"
#include "renderProxyWater.h"
#include "renderProxySpeedTree.h"

#ifdef USE_APEX
#include "renderApexBatcher.h"
#endif
#include "renderShaderPair.h"
#include "../engine/environmentManager.h"
#include "../engine/baseEngine.h"
#include "../engine/renderFragment.h"
#include "../engine/viewport.h"

namespace Config
{
	extern TConfigVar< Bool > cvEnableDeferredLightsStencil;
}

Bool CRenderInterface::DrawSSAO( const CRenderFrameInfo &info, ERenderTargetName rtnTexture, EPostProcessCategoryFlags postProcessFlags )
{
	CRenderSurfaces* surfaces = GetSurfaces();

	// Render ssao buffer
	if ( ( info.IsShowFlagOn( SHOW_PostProcess ) ) && RM_Shaded == info.m_renderingMode && (postProcessFlags & PPCF_SSAO) )
	{
		// Disable wireframe
		GpuApi::SetRenderSettingsWireframe( false );

		// Draw enrichment
		if( GetPostProcess()->DrawSSAO( info, surfaces, rtnTexture, postProcessFlags ) )
		{
			// We need to reset the state manager here because HBAO is messing with our states
			m_stateManager->Reset();
			return true;
		}

	}

	// Clear ssao surface if needed
	ClearColorTarget( surfaces->GetRenderTargetTex( rtnTexture ), Vector::ONES );
	return false;

}


void CRenderInterface::RenderDeferredSetupGBuffer( const CRenderFrameInfo &info )
{
	CRenderSurfaces* surfaces = GetSurfaces();
	RED_ASSERT( surfaces );

	ClearGBuffer( info, info.m_forceGBufferClear );

	GpuApi::RenderTargetSetup rtSetup_GBuffers;
	rtSetup_GBuffers.SetViewport( info.m_width, info.m_height, 0, 0 );
	if ( IsMSAAEnabled( info ) )
	{
		rtSetup_GBuffers.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_GBuffer0MSAA ) );
		rtSetup_GBuffers.SetColorTarget( 1, surfaces->GetRenderTargetTex( RTN_GBuffer1MSAA ) );
		rtSetup_GBuffers.SetColorTarget( 2, surfaces->GetRenderTargetTex( RTN_GBuffer2MSAA ) );
		rtSetup_GBuffers.SetDepthStencilTarget( surfaces->GetDepthBufferTexMSAA() );
	}
	else
	{
		rtSetup_GBuffers.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_GBuffer0 ) );
		rtSetup_GBuffers.SetColorTarget( 1, surfaces->GetRenderTargetTex( RTN_GBuffer1 ) );
		rtSetup_GBuffers.SetColorTarget( 2, surfaces->GetRenderTargetTex( RTN_GBuffer2 ) );
		rtSetup_GBuffers.SetDepthStencilTarget( surfaces->GetDepthBufferTex());
	}

	GpuApi::SetupRenderTargets( rtSetup_GBuffers );
}


void CRenderInterface::RenderDeferredPostGBuffer( CRenderCollector& collector, Float effectStrength )
{
	const CRenderFrameInfo& info = collector.m_frame->GetFrameInfo();	

	// Get rendering surfaces to use
	CRenderSurfaces* surfaces = GetSurfaces();
	RED_ASSERT( surfaces );

	// currently using 2 temp buffers, and swap pointers
	// it can go in 2 passes with 1 buffer if needed	
	GpuApi::TextureRef gbuffTempNormal = surfaces->GetRenderTargetTex( RTN_FinalColor );
	GpuApi::TextureRef gbuffTempSpecular = surfaces->GetRenderTargetTex( RTN_GlobalShadowAndSSAO );

	{

		// Set sampler state
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );
		GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );
		GpuApi::SetSamplerStatePreset( 2, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );
		GpuApi::SetSamplerStatePreset( 6, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );

		// The render target setup
		GpuApi::RenderTargetSetup rtSetup_GBuffers;
		rtSetup_GBuffers.SetDepthStencilTarget( GpuApi::TextureRef::Null(), -1 );
		rtSetup_GBuffers.SetColorTarget( 0, gbuffTempSpecular );
		rtSetup_GBuffers.SetColorTarget( 1, gbuffTempNormal );
		rtSetup_GBuffers.SetViewport( info.m_width, info.m_height, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetup_GBuffers );

		// Needed for the volume cut
		GpuApi::TextureRef sceneDepth =  surfaces->GetDepthBufferTex();
		GpuApi::BindTextures( PSSMP_SceneDepth, 1, &sceneDepth, GpuApi::PixelShader );

		// Also reading gbuff color / normal / roughness
		GpuApi::TextureRef n = IsMSAAEnabled( info ) ? surfaces->GetRenderTargetTex( RTN_GBuffer1MSAA ) : surfaces->GetRenderTargetTex( RTN_GBuffer1 );
		GpuApi::BindTextures( 1, 1, &n, GpuApi::PixelShader );

		GpuApi::TextureRef s = IsMSAAEnabled( info ) ? surfaces->GetRenderTargetTex( RTN_GBuffer2MSAA ) : surfaces->GetRenderTargetTex( RTN_GBuffer2 );
		GpuApi::BindTextures( 2, 1, &s, GpuApi::PixelShader );

		// Volumes cancel the effect
		GpuApi::TextureRef volumes = surfaces->GetRenderTargetTex( RTN_InteriorVolume );
		GpuApi::BindTextures( VOLUME_TEXTURE_SLOT, 1, &volumes, GpuApi::PixelShader );

		// Underwater cancel the effect
		GpuApi::TextureRef waterIntersectionTexture = GpuApi::GetInternalTexture( GpuApi::INTERTEX_Blank2D );
		CRenderProxy_Water* waterProxy = collector.m_scene->GetWaterProxy();

		if( waterProxy != nullptr && waterProxy->ShouldRenderUnderwater() )
		{
			GpuApi::TextureRef w = waterProxy->GetUnderwaterIntersectionTexture();
			if( w != GpuApi::TextureRef::Null() ) waterIntersectionTexture = w;
		}

		GpuApi::BindTextures( 6, 1, &waterIntersectionTexture, GpuApi::PixelShader );

		const Int32 sampleFullWidth	= surfaces->GetWidth();
		const Int32 sampleFullHeight = surfaces->GetHeight();
		const PostProcessUtilities::TexelArea sampleArea = PostProcessUtilities::TexelArea( info.m_width, info.m_height );

		GetStateManager().SetVertexConst( VSC_Custom_0 + 0,	PostProcessUtilities::CalculateTexCoordTransformOneToOne( sampleArea, sampleFullWidth, sampleFullHeight ) );
		GetStateManager().SetPixelConst(  PSC_Custom_0 + 0,	PostProcessUtilities::CalculateTexCoordClamp( sampleArea, sampleFullWidth, sampleFullHeight ) );

		// Setup render context	
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		GpuApi::BindConstantBuffer( 13, m_computeConstantBuffer, GpuApi::PixelShader );

		m_gbuffPostProc->Bind();
		GetStateManager().SetPixelConst( PSC_Custom_2, Vector( effectStrength, 0.0f, 0.0f, 0.0f )  );
		GetDebugDrawer().DrawQuad( Vector( -1, -1, 0, 0 ), Vector( 1, 1, 0, 0 ), 0.5f );

		GpuApi::BindConstantBuffer( 13, GpuApi::BufferRef::Null(), GpuApi::PixelShader );

		StretchRect( gbuffTempNormal, IsMSAAEnabled( info ) ? surfaces->GetRenderTargetTex( RTN_GBuffer1MSAA ) : surfaces->GetRenderTargetTex( RTN_GBuffer1 ) );
		StretchRect( gbuffTempSpecular, IsMSAAEnabled( info ) ? surfaces->GetRenderTargetTex( RTN_GBuffer2MSAA ) : surfaces->GetRenderTargetTex( RTN_GBuffer2 ) );

		GpuApi::BindTextures( PSSMP_SceneDepth, 1, nullptr, GpuApi::PixelShader );
		GpuApi::BindTextures( 1, 2, nullptr, GpuApi::PixelShader );		
		GpuApi::BindTextures( 6, 1, nullptr, GpuApi::PixelShader );
		GpuApi::BindTextures( VOLUME_TEXTURE_SLOT, 1, nullptr, GpuApi::PixelShader );	
	}

}


void CRenderInterface::RenderDeferredFillGBuffer_StaticMeshes( CRenderCollector& collector )
{
	PC_SCOPE_RENDER_LVL0(RenderDeferredFillGBuffer_Statics);

	const CRenderFrameInfo& info = collector.m_frame->GetFrameInfo();

	// Setup render context
	RenderingContext rc( info.m_camera );
	rc.m_terrainToolStampVisible	= info.IsTerrainToolStampVisible();
	rc.m_grassMaskPaintMode			= info.IsGrassMaskPaintMode();
	rc.m_materialDebugMode			= info.m_materialDebugMode;
	rc.m_pass						= RP_GBuffer;


	// Bind best fit normal texture
	BindBestFitNormalsTexture( info );

	// Render base geometry
	const Bool renderWireframe = info.IsShowFlagOn( SHOW_Wireframe );
	const Bool renderDeferred = true;
	if ( renderDeferred )
	{
		// Save wireframe info
		const Bool origWireframe = GpuApi::GetRenderSettings().wireframe;

		// Render solid, simple, static geometry
		// Draw solid deferred lit objects
		{
			PC_SCOPE_RENDER_LVL0( RenderStaticSolidDeferredElements );

			// Setup wireframe state
			GpuApi::SetRenderSettingsWireframe( renderWireframe );

			// Set draw context
			CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_GBufferSolid );

			// Render groups
			ERenderingSortGroup groups[] = { RSG_LitOpaque, RSG_LitOpaqueWithEmissive };

			collector.RenderElementsAllPlanesFrontFirst( ARRAY_COUNT(groups), groups, rc, RECG_SolidStaticMesh );
		}

		GpuApi::SetRenderSettingsWireframe( origWireframe );
	}
}

void CRenderInterface::RenderDeferredFillGBuffer_NonStatics( CRenderCollector& collector )
{
	PC_SCOPE_RENDER_LVL0(RenderDeferredFillGBuffer_NonStaticsEtc);

	const CRenderFrameInfo& info = collector.m_frame->GetFrameInfo();

	// Get the surfaces
	CRenderSurfaces* surfaces = GetSurfaces();
	RED_ASSERT( surfaces );

	// Setup render context
	RenderingContext rc( info.m_camera );
	rc.m_terrainToolStampVisible	= info.IsTerrainToolStampVisible();
	rc.m_grassMaskPaintMode			= info.IsGrassMaskPaintMode();
	rc.m_materialDebugMode			= info.m_materialDebugMode;
	rc.m_pass						= RP_GBuffer;


	// Render base geometry
	const Bool renderWireframe = info.IsShowFlagOn( SHOW_Wireframe );
	const Bool renderDeferred = true;
	if ( renderDeferred )
	{
		// Save wireframe info
		const Bool origWireframe = GpuApi::GetRenderSettings().wireframe;

		// Render solid geometry
		{
			PC_SCOPE_RENDER_LVL1(RenderSolid);

			// Setup wireframe state
			GpuApi::SetRenderSettingsWireframe( renderWireframe );

			// Draw solid deferred lit objects
			{
				PC_SCOPE_RENDER_LVL0(RenderSolidDeferredElements);

				// Set draw context
				CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_GBufferSolid );

				// Render groups
				ERenderingSortGroup groups[] = { RSG_LitOpaque, RSG_LitOpaqueWithEmissive };

				collector.RenderElementsAllPlanesFrontFirst( ARRAY_COUNT(groups), groups, rc, RECG_ALL & ~RECG_SolidStaticMesh );
			}

			// Lay down depth+normals+materialInfo for forward shaded stuff (needed for ssao, materialInfo is for globalShadow)
			{
				PC_SCOPE_RENDER_LVL0(RenderSolidForwardElements);

				ERenderingSortGroup groups[] = { RSG_Skin, RSG_Forward, RSG_Hair };
				Uint32 count = ARRAY_COUNT(groups);
				if ( !collector.GetRenderFrameInfo().IsShowFlagOn( SHOW_HairAndFur ) )
				{
					count = (ARRAY_COUNT(groups) - 1);
				}

				if ( collector.HasAnySortGroupElements( count, groups ) )
				{
					// Save material debug mode
					const EMaterialDebugMode origMaterialDebugMode = rc.m_materialDebugMode;

					// Override material debug mode in case of debug preview enabled so that we
					// would have the full gbuffer renderer even in case of forward shaded materials
					{
						const Bool isDebugZoomAllowed = collector.IsWorldScene();
						if ( GetPostProcess()->IsDebugOverlayActivated( info, isDebugZoomAllowed ) )
						{
							const EEnvManagerModifier modifier = (EEnvManagerModifier)info.m_envParametersGame.m_displaySettings.m_displayMode;
							if ( info.IsShowFlagOn( SHOW_Wireframe ) || ( modifier >= EMM_GBUFF_FIRST && modifier <= EMM_GBUFF_LAST ) )
							{
								rc.m_materialDebugMode = MDM_FullGBuffer;
							}
						}
					}

					// Setup rendertargets
					const GpuApi::RenderTargetSetup rtSetupOrig = GpuApi::GetRenderTargetSetup();
					Bool useCustomRenderTargets = MDM_FullGBuffer !=  rc.m_materialDebugMode;
					if ( useCustomRenderTargets )
					{
						GpuApi::RenderTargetSetup rtSetup = rtSetupOrig;

						ASSERT( 3 == rtSetup.numColorTargets );
						GpuApi::TextureRef rtTexNormals = rtSetup.colorTargets[1];
						GpuApi::TextureRef rtTexMaterialInfo = rtSetup.colorTargets[2];
						ASSERT( -1 == rtSetup.colorTargetsSlices[1] );
						ASSERT( -1 == rtSetup.colorTargetsSlices[2] );
						rtSetup.SetNullColorTarget();
						rtSetup.SetColorTarget( 0, rtTexNormals );
						rtSetup.SetColorTarget( 1, rtTexMaterialInfo );

						// Setup target
						GpuApi::SetupRenderTargets( rtSetup );
					}

					// Set draw context
					CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_GBufferSolid );

					// Render
					{
						const Uint32 originalLightChannelsForcedMask = rc.m_lightChannelForcedMask;
						rc.m_lightChannelForcedMask |= (Config::cvEnableDeferredLightsStencil.Get() ? LC_ForwardShaded : 0);

						collector.RenderElementsAllPlanesFrontFirst( count, groups, rc, RECG_ALL );

						rc.m_lightChannelForcedMask = originalLightChannelsForcedMask;
					}

					// Restore rendertarget
					if ( useCustomRenderTargets )
					{
						GpuApi::SetupRenderTargets( rtSetupOrig );
					}

					// Restore material debug mode
					rc.m_materialDebugMode = origMaterialDebugMode;
				}
			}

#ifdef USE_NVIDIA_FUR
			if ( 0 ) // disable gbuffer for now due to AA issue
			{
				PC_SCOPE_RENDER_LVL1(RenderFurGBuffer);

				// Set draw context
				GpuApi::SetReversedProjectionState( true );
				CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_GBufferSolid );

				// Render Fur
				collector.RenderFur( rc );

				//SetGlobalShaderConstants( *m_stateManager, info, surfaces->GetWidth(), surfaces->GetHeight() );
			}
#endif

			// Draw speed tree foliage
			if ( info.IsShowFlagOn(SHOW_Foliage) )
			{
				PC_SCOPE_RENDER_LVL1(RenderFoliage);

				// Let speed tree SDK draw stuff (binds constant buffers etc.)
				// NOTE: the below check should be put earlier but at the current stage of the project I don't want to spawn new bugs because of
				//       let's say BindMainConstantBuffers below being critical for the frame to stay consistent.
				if ( collector.GetScene() &&  collector.GetScene()->GetSpeedTree() )
				{
					collector.GetScene()->GetSpeedTree()->Render( rc, collector.GetRenderFrameInfo() );
				}

				// Bind our constant buffers again
				m_stateManager->BindGlobalConstants();
				GpuApi::BindMainConstantBuffers();

				GetStateManager().SetCamera( rc.GetCamera() );

				// Bind best fit normal texture again
				BindBestFitNormalsTexture( info );
			}
			else if ( collector.GetScene() &&  collector.GetScene()->GetSpeedTree() )
			{
				collector.GetScene()->GetSpeedTree()->NoRender_FinalizeProcessing();
			}

			// Draw terrain
			if (info.IsShowFlagOn(SHOW_Terrain))
			{
				PC_SCOPE_RENDER_LVL1(RenderTerrain);

				// Set draw context
				CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_Terrain, LC_Terrain );

				// Setup wireframe state
				GpuApi::SetRenderSettingsWireframe( renderWireframe );

				GetStateManager().SetCamera( rc.GetCamera() );

				// Render NEW terrain
				if ( collector.m_scene )
				{
					collector.m_scene->RenderTerrain( rc, info, collector.m_sceneStats );
				}
			}
		}

		// Render regular stripes (before decals so we can apply decals on stripes)
		if ( info.IsShowFlagOn( SHOW_Stripes ) )
		{
			PC_SCOPE_RENDER_LVL1(RenderStripes);

			// Build rendertarget setups
			GpuApi::RenderTargetSetup rtSetupStripes;
			rtSetupStripes.SetDepthStencilTarget( IsMSAAEnabled( info ) ? surfaces->GetDepthBufferTexMSAA() : surfaces->GetDepthBufferTex(), -1, true );
			rtSetupStripes.SetColorTarget( 0, surfaces->GetRenderTargetTex( IsMSAAEnabled( info ) ? RTN_GBuffer0MSAA : RTN_GBuffer0 ) );
			rtSetupStripes.SetColorTarget( 1, surfaces->GetRenderTargetTex( IsMSAAEnabled( info ) ? RTN_GBuffer1MSAA : RTN_GBuffer1 ) );
			rtSetupStripes.SetViewport( info.m_width, info.m_height, 0, 0 );

			// Setup render targets
			GpuApi::SetupRenderTargets( rtSetupStripes );

			if ( collector.m_scene && info.IsShowFlagOn( SHOW_Stripes ) )
			{
				rc.m_pass = RP_GBuffer;

				// Render terrain projected stripes
				{
					CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_GBufferProjectedStripes, LC_Terrain );
					GpuApi::TextureRef sceneDepth = IsMSAAEnabled( info ) ? surfaces->GetDepthBufferTexMSAA() : surfaces->GetDepthBufferTex();
					GpuApi::BindTextures( PSSMP_SceneDepth, 1, &sceneDepth, GpuApi::PixelShader );

					collector.RenderStripes( rc, true );

					GpuApi::BindTextures( PSSMP_SceneDepth, 1, nullptr, GpuApi::PixelShader );
				}

				// Render normal stripes
				{
					CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_GBufferStripes );
					collector.RenderStripes( rc, false );
				}
			}
		}

		// Render decals
		if ( collector.m_scene && collector.HasAnyDecals() && info.IsShowFlagOn( SHOW_Decals ) )
		{
			PC_SCOPE_RENDER_LVL0(RenderDecals);

			for( Uint32 i = 0; i < EDDI_MAX; ++i )
			if( collector.HasAnyDecalsType( (EDynamicDecalRenderIndex)( i ) ) )
			{
				GpuApi::TextureRef gbufferNormals = surfaces->GetRenderTargetTex( RTN_GBuffer1 );

				// Build rendertarget setups
				GpuApi::RenderTargetSetup rtSetupDecals;
				Uint32 gBufferId = 0;

				rtSetupDecals.SetColorTarget( gBufferId++ , surfaces->GetRenderTargetTex( RTN_GBuffer0 ) );

				// If its speculared decals, allow them to render to specularity g-buffer.
				if( i == EDDI_SPECULARED || i == EDDI_SPECULARED_NORMALMAPPED )
				{
					rtSetupDecals.SetColorTarget( gBufferId++, surfaces->GetRenderTargetTex( RTN_GBuffer2 ) );
				}

				// If its speculared decals, allow them to render to specularity g-buffer.
				if( i == EDDI_NORMALMAPPED || i == EDDI_SPECULARED_NORMALMAPPED )
				{
					rtSetupDecals.SetColorTarget( gBufferId++ , gbufferNormals );
				}

				rtSetupDecals.SetViewport( info.m_width, info.m_height, 0, 0 );
				// Bind depth target for readonly. This way we can also bind it for reading in the shader.
				rtSetupDecals.SetDepthStencilTarget( IsMSAAEnabled( info ) ? surfaces->GetDepthBufferTexMSAA() : surfaces->GetDepthBufferTex(), -1, true );
				// Set draw context

				// Setup render targets
				GpuApi::SetupRenderTargets( rtSetupDecals );

				CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_GBufferDecalsBlended, LC_DynamicObject );


				// If its speculared decals, allow them to render to specularity g-buffer.
				if( i != EDDI_NORMALMAPPED && i != EDDI_SPECULARED_NORMALMAPPED )
				{
					GpuApi::BindTextures( 2, 1, &gbufferNormals, GpuApi::PixelShader );
				}

				// FIXME these can't be set in one call because they are not next to each other and the PSSMP_SceneDepth can change
				GpuApi::TextureRef sceneDepth = IsMSAAEnabled( info ) ? surfaces->GetDepthBufferTexMSAA() : surfaces->GetDepthBufferTex();
				GpuApi::BindTextures( PSSMP_SceneDepth, 1, &sceneDepth, GpuApi::PixelShader );

				rc.m_pass = RP_GBuffer;
				collector.RenderDecals( (EDynamicDecalRenderIndex)( i ) , rc );

				// If its speculared decals, we might render normalmapped ones next so we need to unbind the normalmap
				if( i != EDDI_NORMALMAPPED && i != EDDI_SPECULARED_NORMALMAPPED )
				{
					GpuApi::BindTextures( 2, 1, nullptr, GpuApi::PixelShader );
				}

			}

			GpuApi::BindTextures( PSSMP_SceneDepth, 1, nullptr, GpuApi::PixelShader );
			GpuApi::BindTextures( 2, 1, nullptr, GpuApi::PixelShader );

		}

		// Restore wireframe setting
		GpuApi::SetRenderSettingsWireframe( origWireframe );
	}
	
	// Unbind sampler
	GpuApi::BindTextures( PSSMP_NormalsFitting, 1, nullptr, GpuApi::PixelShader );

}
namespace Config
{
	TConfigVar< Bool >	cvDrawMerged( "Rendering/MergedShadows", "cvDrawMerged",	true, eConsoleVarFlag_Developer );
}
void CRenderInterface::RenderShadows( const CRenderFrameInfo &info, const SMergedShadowCascades &cascades, const CCascadeShadowResources &cascadesResources )
{
	PC_SCOPE_RENDER_LVL0( RenderShadows );

	// Get flags
	const Bool renderShadows = info.IsShowFlagOn( SHOW_Shadows );
	const Bool hasCascades = cascades.m_numCascades > 0;

	// Render cascades
	if ( renderShadows && hasCascades )
	{
		// Render cascades
		{
			// Lets not render cascades that are further than camera far plane				
			Uint16 effectiveCascadesNum = 0;

			const Float farPlane = info.m_camera.GetFarPlane();			
			for( Uint32 i=0; i<cascades.m_numCascades; ++i )
			{
				++effectiveCascadesNum;
				if( farPlane < info.GetCascadeDistance( i ) ) break;
			}
	
			// Draw
			for ( Uint16 i=0; i<effectiveCascadesNum; i++ )
			{
				PC_SCOPE_RENDER_LVL1( RenderCascade )
				const SShadowCascade& cascade = cascades.m_cascades[i];

				MeshDrawingStats meshStats;

				// Minimal filter texel size prevents from subpixel filtering - sampling disk cannot be 
				// inside one texel, bcoz this will give regular linear sampling pattern.
				const Float minFilterTexelSize = SMergedShadowCascades::MIN_FILTER_SIZE;
				const Float shadowFilterTexelSize = info.m_cascadeFilterSizes[i] * cascadesResources.GetResolution() / (Float)cascade.m_camera.GetZoom();
				const Float slopeBias = ceilf( 1.4142f * Max<Float>( minFilterTexelSize, shadowFilterTexelSize ) );

				GpuApi::SetupShadowDepthBias( 0, info.m_shadowBiasOffsetSlopeMul * slopeBias );

				// Set draw context
				CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_ShadowMapGenCSM_DepthTex );

				// bind render target view for each cascade ( we are using texture arrays )
				GpuApi::RenderTargetSetup rtSetup;
				rtSetup.SetNullColorTarget();
				rtSetup.SetDepthStencilTarget( cascadesResources.GetDepthStencilArrayWrite(), i ); // SLICE!
				rtSetup.SetViewportFromTarget( cascadesResources.GetDepthStencilArrayWrite() );
				GpuApi::SetupRenderTargets( rtSetup );

				// Clear the depth targets
				ASSERT( !GpuApi::IsReversedProjectionState() );
				ClearDepthStencilTarget( 1.0f, 0 );

				// Set drawing viewport to subpart of shadowmap
				GpuApi::ViewportDesc viewport;
				viewport.minZ = 0.0f;
				viewport.maxZ = 1.0f;
				viewport.x = 0;
				//dex++: no vertical packing
				viewport.y = 0;
				//dex--
				viewport.width = cascadesResources.GetResolution();
				viewport.height = cascadesResources.GetResolution();
				GpuApi::SetViewport( viewport );

				// Calculate biased camera
				CRenderCamera biasedCamera = cascade.m_camera;
				{
					const Float biasValue = info.m_shadowBiasOffsetConst + info.m_shadowBiasOffsetConstPerCascade[i] + i * info.m_shadowBiasCascadeMultiplier;
					biasedCamera.SetNearPlane( biasedCamera.GetNearPlane() - biasValue );
					biasedCamera.SetFarPlane( biasedCamera.GetFarPlane() - biasValue );
				}

				// Setup depth rendering context
				RenderingContext rc( biasedCamera );

				// Setup global camera parameters
				m_stateManager->SetLocalToWorld( NULL );
				m_stateManager->SetCamera( rc.GetCamera() );

				// Render solid merged meshes
				{
					PC_SCOPE_RENDER_LVL1( MergedSolidMeshes )
					rc.m_pass = RP_ShadowDepthSolid;
#ifdef RED_PLATFORM_ORBIS
					if( Config::cvDrawMerged.Get() )
					{
						m_meshBatcher->RenderMergedMeshesCsVs( info, rc, cascade.GetMergedSolidElements(), RMBF_Shadowmap | RMBF_CascadeShadows, meshStats );
					}
					else
#endif
					{
						m_meshBatcher->RenderMeshes( info, rc, cascade.GetMergedSolidElements(), RMBF_Shadowmap | RMBF_CascadeShadows, meshStats );
					}
				}

				// Render solid meshes
				{
					PC_SCOPE_RENDER_LVL0( SolidMeshes )
					rc.m_pass = RP_ShadowDepthSolid;
					m_meshBatcher->RenderMeshes( info, rc, cascade.GetSolidElements(), RMBF_Shadowmap | RMBF_CascadeShadows, meshStats );
				}

				// Render elements with discards
				{
					PC_SCOPE_RENDER_LVL1( DissolvedMeshes )
					rc.m_pass = RP_ShadowDepthMasked;
					m_meshBatcher->RenderMeshes( info, rc, cascade.GetDiscardElements(), RMBF_CascadeShadows | RMBF_Shadowmap, meshStats );
				}

				// Render the speed tree shadows
				const Bool renderSpeedTreeShadows = info.IsShowFlagOn( SHOW_SpeedTreeShadows );
				if ( renderSpeedTreeShadows && cascades.m_collector && cascades.m_collector->m_scene && cascades.m_collector->m_scene->IsWorldScene() )
				{
					PC_SCOPE_RENDER_LVL0( SpeedTreeShadows )
					rc.m_pass = RP_ShadowDepthMasked;
					cascades.m_collector->m_scene->RenderSpeedTreeCascadeShadows( rc, info, &cascade );

					// Restore previous camera and buffer bindings
					m_stateManager->BindGlobalConstants();
					m_stateManager->SetCamera( rc.GetCamera() );

					GpuApi::BindMainConstantBuffers();
					BindBestFitNormalsTexture( info );
				}

#ifdef USE_APEX
				// Render apex shadows
				if ( info.IsShowFlagOn( SHOW_Apex ) && info.IsShowFlagOn( SHOW_AllowApexShadows ) )
				{
					PC_SCOPE_RENDER_LVL0( ApexShadows )
					// Render the APEX shadows
					rc.m_pass = RP_ShadowDepthSolid;
					m_apexBatcher->RenderApex( info, rc, cascade.GetApexElements(), RABF_Solid, meshStats );
					rc.m_pass = RP_ShadowDepthMasked;
					m_apexBatcher->RenderApex( info, rc, cascade.GetApexElements(), RABF_SolidMasked, meshStats );
				}
#endif

#ifdef USE_NVIDIA_FUR
				// Render hair shadows
				if ( info.IsShowFlagOn( SHOW_HairAndFur ) )
				{
					PC_SCOPE_RENDER_LVL0( HairShadows )
					// Render the NVIDIA Hair shadows
					rc.m_pass = RP_ShadowDepthSolid;

					// Set draw context
					CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_Default );
					GpuApi::SetDrawContext( GpuApi::GetDrawContext(), LC_Default );

					cascades.m_collector->RenderFur( rc );
				}
#endif

#ifndef RED_FINAL_BUILD
				// Update scene stats
				{
					extern SceneRenderingStats GRenderingStats;
					GRenderingStats.m_numCascadeTriangles[ i ] = meshStats.m_numTriangles;
					GRenderingStats.m_numCascadeChunks[ i ] = meshStats.m_numChunks;
				}
#endif

#ifdef RED_PLATFORM_DURANGO
				GpuApi::CopyTextureDataDMA( cascadesResources.GetDepthStencilArrayRead(), 0, i, cascadesResources.GetDepthStencilArrayWrite(), 0, i );
#endif
			}

			// Restore default depth bias settings
			GpuApi::SetupShadowDepthBias( 0, 0 );
		}
	}
}

void CRenderInterface::BindBestFitNormalsTexture( const CRenderFrameInfo &info )
{
	CRenderTexture* tex = info.m_envParametersDayPoint.GetBestFitNormalsTexture().Get<CRenderTexture>();
	if ( tex )
	{
		tex->Bind( PSSMP_NormalsFitting, GpuApi::SAMPSTATEPRESET_ClampPointNoMip );
	}
}

void CRenderInterface::BindGlobalSkyShadowTextures( const CRenderFrameInfo &info, GpuApi::eShaderType shaderTarget )
{
	CRenderTextureArray* cloudsRenderTexture = info.m_envParametersDayPoint.m_cloudsShadowTexture.Get< CRenderTextureArray >();
	CRenderTextureArray* fakeCloudsRenderTexture = info.m_envParametersDayPoint.m_fakeCloudsShadowTexture.Get< CRenderTextureArray >();

	CRenderTextureArray* cloudsShadowTextureToBind = cloudsRenderTexture;
	if ( !cloudsShadowTextureToBind )
	{
		cloudsShadowTextureToBind = fakeCloudsRenderTexture;
	}
	
	if ( cloudsShadowTextureToBind && info.IsCloudsShadowVisible() )
	{
		cloudsShadowTextureToBind->Bind( 14, Map( shaderTarget ) );
	}
	else
	{
		GpuApi::BindTextures( 14, 1, nullptr, shaderTarget );
	}

	GpuApi::SetSamplerStatePreset( 14, GpuApi::SAMPSTATEPRESET_WrapLinearNoMip, shaderTarget );
}
