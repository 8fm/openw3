/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "renderInterface.h"

#include "../redMemoryFramework/redMemoryAllocators.h"
#include "../redMemoryFramework/redMemoryRegionAllocator.h"
#include "guiRenderSystemScaleform.h"

#include "renderTerrainShadows.h"
#include "renderShadowManager.h"
#include "renderTextureArray.h"
#include "renderViewport.h"

#include "gameplayFx.h"
#include "gameplayFxSurface.h"

#include "renderLodBudgetSystem.h"
#include "renderProxySpeedTree.h"
#include "renderScene.h"
#include "renderPostProcess.h"
#include "renderPostFx.h"
#include "renderParticleBatcher.h"
#include "renderThread.h"
#include "renderVideoPlayer.h"
#include "renderLoadingScreen.h"
#include "renderSkinManager.h"
#include "renderShaderPair.h"
#include "renderRenderSurfaces.h"
#include "renderEnvProbe.h"
#include "renderEnvProbeManager.h"
#include "renderParticleEmitter.h"
#include "renderSkybox.h"
#include "renderElementMap.h"
#include "renderProxyTerrain.h"
#include "renderMaterial.h"
#include "renderTextureStreaming.h"
#include "renderScaleformTexture.h"
#include "renderDistantLightBatcher.h"
#include "renderLoadingBlur.h"
#include "renderGameplayRenderTarget.h"
#include "renderDefragHelper.h"
#include "renderVisibilityQueryManager.h"
#include "../../../../bin/shaders/include/globalConstants.fx"
#include "../../common/engine/renderSettings.h"
#include "../../common/core/fileSystemProfilerWrapper.h"

#include "../gpuApiUtils/gpuApiTypes.h"

#ifdef USE_NVIDIA_FUR
#include "../../../external/NvidiaHair/include/GFSDK_HairWorks.h"
#endif // USE_NVIDIA_FUR

#ifdef RED_PLATFORM_ORBIS
	// HACK Gnm - for test frame purposes
	#include "../redIO/redIO.h"
	#include "../gpuApiUtils/gpuApiMemory.h"

	// used only in render tests
	#include "renderMesh.h"
#endif
#include "../core/gatheredResource.h"
#include "../engine/fonts.h"
#include "../engine/debugPageManagerBase.h"
#include "../engine/environmentManager.h"
#include "../engine/baseEngine.h"
#include "../engine/gameTimeManager.h"
#include "../engine/screenshotSystem.h"
#include "../engine/renderFragment.h"
#include "../engine/renderSettings.h"
#include "screenshotWatermark.inl"
#include "ngPlusWatermark.inl"

#if defined(USE_SCALEFORM) && !defined(NO_DEBUG_PAGES)
#include "../renderer/guiRenderSystemScaleform.h"
#include "../renderer/renderScaleformTexture.h"
#endif


// FIXME : Having issues on PS4, where the async compute water seems to be interfering with the CsVs shadow rendering,
// resulting in some shadows flickering sometimes. Not sure what's actually causing it, but in the interests of getting
// this finally into a patch, just doing the async compute _after_ the shadows on PS4.
#ifdef RED_PLATFORM_ORBIS
#	define START_ASYNC_WATER_LATER
#endif


namespace Config
{
	TConfigVar< Int32 >			cvHiResEntityCustomShadowmapSize(	"Rendering", "HiResEntityCustomShadowmapSize",		-1, eConsoleVarFlag_Developer );

	extern TConfigVar< Int32, Validation::IntRange< 0, 63 > >		cvHairLateAllocVSLimit;
}

#define USE_LODEPNG
#ifdef USE_LODEPNG
#include "../../../external/lodepng/lodepng.h"
#include "../../../external/lodepng/lodepng.cpp"
#endif

Bool GIsRendererTakingUberScreenshot = false;

Bool GIsDuringUberSampleIsFirst = false;

extern Float GRenderSettingsMipBias;

#ifdef USE_ANSEL
extern Bool isAnselCaptureActive;
#endif // USE_ANSEL

CRenderCollector::CRenderCollectorData GRenderCollectorData;

namespace Debug
{
	Int32			GRenderTargetZoomShift	= 0;
	Int32			GRenderTargetOffsetX	= 0;
	Int32			GRenderTargetOffsetY	= 0;
}

extern CGatheredResource resOnScreenTextFont;



void CRenderInterface::SetupForSceneRender( const CRenderFrameInfo& info )
{
	CRenderSurfaces* surfaces = GetSurfaces();
	RED_ASSERT( surfaces );

	m_stateManager->SetLocalToWorld( nullptr );
	m_stateManager->SetCamera( info.m_camera );

	m_stateManager->SetGlobalShaderConstants( info, surfaces->GetWidth(), surfaces->GetHeight(), m_gameplayFX );
	m_stateManager->BindGlobalConstants();

	BindGlobalSkyShadowTextures( info, GpuApi::PixelShader );
}


void CRenderInterface::RenderOpaqueAfterGBuffer( CRenderCollector& collector )
{
	PC_SCOPE_RENDER_LVL1(RenderOpaqueAfterGBuffer);

	const CRenderFrameInfo& info = collector.GetRenderFrameInfo();

	// Get rendering surfaces to use
	CRenderSurfaces* surfaces = GetSurfaces();
	RED_ASSERT( surfaces );


	// Setup render context
	RenderingContext rc( info.m_camera );
	rc.m_terrainToolStampVisible		= info.IsTerrainToolStampVisible();
	rc.m_materialDebugMode				= info.m_materialDebugMode;


	const Bool targetWireframeSetting	= info.IsShowFlagOn( SHOW_Wireframe );
	const Bool renderLighting			= info.IsShowFlagOn( SHOW_Lighting );
	const Bool msaaEnabled				= IsMSAAEnabled( info );


	// Render deferred shaded color


	// Create render target setup for drawing into Color, depth enabled
	// Setup and clear it
	GpuApi::RenderTargetSetup rtSetup_RTN_Color_WithDepth;
	{
		rtSetup_RTN_Color_WithDepth.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_Color ) );
		rtSetup_RTN_Color_WithDepth.SetDepthStencilTarget( msaaEnabled ? surfaces->GetDepthBufferTexMSAA() : surfaces->GetDepthBufferTex() );
		rtSetup_RTN_Color_WithDepth.SetViewport( info.m_width, info.m_height, 0, 0 );

		// Clear rendering targets
		// ace_optimize: wouldn't it be more optimal to clear just the part of rendertarget that we really make use of? 
		//               [in editor it typically is only a small fraction of the whole buffers - do this in some other places too]
		Vector clearColor = info.m_clearColor.ToVector();
		clearColor.W = 1.0f;
		ClearColorTarget( surfaces->GetRenderTargetTex( RTN_Color ), clearColor );
		// Don't clear depth, we want the existing scene stuff
	}


	GpuApi::RenderTargetSetup rtSetup_RTN_ColorUnfogged_WithDepth;
	if ( RM_Shaded == info.m_renderingMode && renderLighting && !targetWireframeSetting )
	{

		DrawSSAO( info, RTN_GlobalShadowAndSSAO, collector.m_postProcess );

		SetupForSceneRender( info );

		GpuApi::SetupRenderTargets( rtSetup_RTN_Color_WithDepth );

		// Lighting scope
		PC_SCOPE_RENDER_LVL0( Lighting );

		// Disable wireframe
		GpuApi::SetRenderSettingsWireframe( false );

		// Render shadowmask
		{
			GpuApi::TextureRef texRenderTarget = surfaces->GetRenderTargetTex( RTN_GlobalShadowAndSSAO );
			GpuApi::TextureRef texGBuffer1 = surfaces->GetRenderTargetTex( msaaEnabled ? RTN_GBuffer1MSAA : RTN_GBuffer1 );
			GpuApi::TextureRef texGBuffer2 = surfaces->GetRenderTargetTex( msaaEnabled ? RTN_GBuffer2MSAA : RTN_GBuffer2 );
			GpuApi::TextureRef texDepthBuffer = msaaEnabled ? surfaces->GetDepthBufferTexMSAA() : surfaces->GetDepthBufferTex();

			RenderGlobalShadowMask( collector, m_cascadeShadowResources, texRenderTarget, texGBuffer1, texGBuffer2, texDepthBuffer );
		}

		// Render base lighting
		RenderTiledDeferred( info, collector.m_postProcess, RTN_Color2 );

		// Build unfogged rtsetup
		rtSetup_RTN_ColorUnfogged_WithDepth.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_Color2 ) );
		rtSetup_RTN_ColorUnfogged_WithDepth.SetDepthStencilTarget( surfaces->GetDepthBufferTex() );
		rtSetup_RTN_ColorUnfogged_WithDepth.SetViewport( info.m_width, info.m_height, 0, 0 );
	}
	else // No lighting
	{
		PC_SCOPE_RENDER_LVL1( NoLighting );

		// Clear shadow/interorFactor/dimmers/ssao rendertarget
		ClearColorTarget( surfaces->GetRenderTargetTex( RTN_GlobalShadowAndSSAO ), Vector::ONES );

		//
		GpuApi::SetupRenderTargets( rtSetup_RTN_Color_WithDepth );

		//
		{
			// Copy albedo from gbuffer to the render target
			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

			// Set textures
			GpuApi::TextureRef gBuff0Texture = surfaces->GetRenderTargetTex( msaaEnabled ? RTN_GBuffer0MSAA : RTN_GBuffer0 );
			GpuApi::BindTextures( 0, 1, &gBuff0Texture, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

			m_postFXCopyDecode->Bind();
			GetDebugDrawer().DrawQuad( Vector( -1.f, 1.f, 0.f, 1.f ), Vector( 1.f, -1.f, 0.f, 1.f ), 0.5f ); //good
		}

		// Build unfogged rtsetup
		rtSetup_RTN_ColorUnfogged_WithDepth.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_Color ) );
		rtSetup_RTN_ColorUnfogged_WithDepth.SetDepthStencilTarget( surfaces->GetDepthBufferTex() );
		rtSetup_RTN_ColorUnfogged_WithDepth.SetViewport( info.m_width, info.m_height, 0, 0 );

		// Set draw context
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_Unlit );

		// Set wireframe
		GpuApi::SetRenderSettingsWireframe( targetWireframeSetting );

		// Render without lighting
		rc.m_pass = RP_NoLighting;

		// Render deferred fragments as normal shit
		ERenderingSortGroup groups[] = { RSG_LitOpaque, RSG_LitOpaqueWithEmissive, RSG_Skin, RSG_Forward };
		collector.RenderElementsAllPlanesFrontFirst( ARRAY_COUNT(groups), groups, rc, RECG_ALL );
	}

	// If we're in wireframe mode, clear zbuffer so that we could see through objects.
	if ( info.IsShowFlagOn( SHOW_Wireframe ) )
	{
		ClearDepthStencilTarget( GpuApi::GetClearDepthValueRevProjAware(), 0 );
	}

	BindForwardConsts( info, GetGlobalCascadesShadowResources(), surfaces, true, GpuApi::PixelShader );
	BindForwardConsts( info, GetGlobalCascadesShadowResources(), surfaces, true, GpuApi::VertexShader );
	BindSharedConstants( GpuApi::VertexShader );

	// --------------------------
	// Bind unfogged rendertarget
	// --------------------------
	GpuApi::SetupRenderTargets( rtSetup_RTN_ColorUnfogged_WithDepth );
	
	// ----------------------------
	// Render forward lit fragments
	// ----------------------------
	if ( info.IsShowFlagOn( SHOW_ForwardPass ) )
	{
		PC_SCOPE_RENDER_LVL1( RenderForwardFragments );

		// Set draw context
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_Unlit );
		GpuApi::SetRenderSettingsWireframe( targetWireframeSetting );

		// Forward lit elements
		{
			rc.m_pass = RP_ForwardLightingSolid;
			collector.RenderElementsAllPlanesFrontFirst( RSG_Forward, rc, RECG_ALL );
		}
	}

	// ------------------
	// Render eye overlay
	// ------------------
	{
		PC_SCOPE_RENDER_LVL1( RenderEyeOverlay );

		// Set draw context
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_EyeOverlay );
		GpuApi::SetRenderSettingsWireframe( targetWireframeSetting );

		// Forward lit elements
		{
			rc.m_pass = RP_NoLighting;
			collector.RenderElementsAllPlanesFrontFirst( RSG_EyeOverlay, rc, RECG_ALL );
		}
	}

	// ---------------------
	// Render skin fragments
	// ---------------------
	if ( info.IsShowFlagOn( SHOW_ForwardPass ) )
	{
		PC_SCOPE_RENDER_LVL0( RenderSkin );

		// Set draw context
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_Unlit );
		GpuApi::SetRenderSettingsWireframe( targetWireframeSetting );

		// Forward lit elements
		{
			rc.m_pass = RP_ForwardLightingSolid;
			collector.RenderElementsAllPlanesFrontFirst( RSG_Skin, rc, RECG_ALL );
		}
	}

	// ----------------------------------
	// Render hair fragments opaque parts
	// ----------------------------------
	if ( info.IsShowFlagOn( SHOW_HairAndFur ) && info.IsShowFlagOn( SHOW_ForwardPass ) )
	{
		PC_SCOPE_RENDER_LVL0( RenderHairOpaque );

		//GpuApi::SetVsWaveLimits( 0, Config::cvHairLateAllocVSLimit.Get() );

		// Set draw context
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_HairOpaque );
		GpuApi::SetRenderSettingsWireframe( targetWireframeSetting );

		rc.m_pass = RP_ForwardLightingSolid;
		collector.RenderElementsAllPlanesFrontFirst( RSG_Hair, rc, RECG_ALL );

		//GpuApi::ResetVsWaveLimits();
	}

	// ----------------
	// Apply global fog
	// ----------------
	if ( rtSetup_RTN_ColorUnfogged_WithDepth.colorTargets[0] != rtSetup_RTN_Color_WithDepth.colorTargets[0] )
	{
		PC_SCOPE_RENDER_LVL1( RenderGlobalFog );

		GpuApi::RenderTargetSetup rtSetupFog;
		rtSetupFog.SetColorTarget( 0, rtSetup_RTN_Color_WithDepth.colorTargets[0] );
		rtSetupFog.SetViewport( info.m_width, info.m_height, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetupFog );

		GpuApi::TextureRef shadowRefs[] =	{
			surfaces->GetDepthBufferTex(), 
			rtSetup_RTN_ColorUnfogged_WithDepth.colorTargets[0],
			surfaces->GetRenderTargetTex( RTN_GlobalShadowAndSSAO )
		};

		GpuApi::BindTextures( 0, ARRAY_COUNT(shadowRefs), shadowRefs, GpuApi::PixelShader );

		// Set ssao fog impact
		{
			const CEnvGlobalFogParametersAtPoint &fogParams = info.m_envParametersArea.m_globalFog;
			const Float distanceNear = fogParams.m_ssaoImpactNearDistance.GetScalarClampMin(0.f);
			const Float distanceFar = fogParams.m_ssaoImpactFarDistance.GetScalarClampMin( distanceNear + 0.1f );
			GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( distanceNear, distanceFar, fogParams.m_ssaoImpactNearValue.GetScalarClamp(0.f, 1.f), fogParams.m_ssaoImpactFarValue.GetScalarClamp(0.f, 1.f) ) );
			GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( fogParams.m_ssaoImpactClamp.GetScalarClamp(0.f, 1.f), 0, 0, 0 ) );
		}

		// Draw
		{
			CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );
			GetPostProcess()->m_drawer->DrawQuad( m_shaderGlobalFog );
		}

		GpuApi::BindTextures( 0, ARRAY_COUNT(shadowRefs), nullptr, GpuApi::PixelShader );
	}

	// -----------------------
	// Bind color rendertarget
	// -----------------------
	if ( rtSetup_RTN_ColorUnfogged_WithDepth.colorTargets[0] != rtSetup_RTN_Color_WithDepth.colorTargets[0] )
	{
		GpuApi::SetupRenderTargets( rtSetup_RTN_Color_WithDepth );
	}

	// ----------------------
	// Render unlit fragments
	// ----------------------
	{
		// Set draw context
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_Unlit );
		GpuApi::SetRenderSettingsWireframe( targetWireframeSetting );

		{
			PC_SCOPE_RENDER_LVL1( UnlitFragments );

			//dex++: only normal unlit geometry is rendered here, hair is separated into different pass
			rc.m_pass = RP_NoLighting;

			ERenderingSortGroup groups[] = { RSG_Unlit };
			collector.RenderElementsAllPlanesFrontFirst( ARRAY_COUNT(groups), groups, rc, RECG_ALL );
			//dex--
		}
	}

	// -------------------
	// Add some background texture
	// -------------------
	if( collector.m_frame->GetRenderTarget() && collector.m_frame->GetRenderTarget()->GetTextureMask() )
	{
		GpuApi::RenderTargetSetup rtSetupMain;
		rtSetupMain.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_Color) );
		rtSetupMain.SetDepthStencilTarget( surfaces->GetDepthBufferTex() , -1 , true );
		rtSetupMain.SetViewport( info.m_width, info.m_height, 0, 0 );

		GpuApi::SetupRenderTargets( rtSetupMain );

		CGpuApiScopedDrawContext context( GpuApi::DRAWCONTEXT_PostProcSet_DepthEqual );

		// Make sure we don't have any extra shaders.
		m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_GeometryShader );
		m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_HullShader );
		m_stateManager->SetShader( GpuApi::ShaderRef::Null(), RST_DomainShader );

		CRenderTexture* texture = static_cast<CRenderTexture*>( collector.m_frame->GetRenderTarget()->GetTextureMask() );
		RED_ASSERT( texture != nullptr, TXT("No texture mask") );
		if ( texture != nullptr )
		{
			texture->Bind( 0 );
		}
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Render
		m_stateManager->SetVertexConst( VSC_Custom_0,	Vector( 0.0, 0.0, 1.0f, 1.0f ) );
		m_stateManager->SetPixelConst(  PSC_Custom_0,	info.m_backgroundTextureColorScale );

		m_simpleCopyScale->Bind();

		GpuApi::DrawPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleStrip, 2 );

	}

	// ----------
	// Render fur
	// ----------
#ifdef USE_NVIDIA_FUR
	if ( m_hairSDK != nullptr && info.IsShowFlagOn( SHOW_HairAndFur ) && info.IsShowFlagOn( SHOW_ForwardPass ) && Config::cvHairWorksLevel.Get()>0 )
	{
		PC_SCOPE_RENDER_LVL1( RenderFurOpaque );

		// Setup render context
		RenderingContext rc( info.m_camera );
		rc.m_pass = RP_ForwardLightingSolid;

		// MSAA settings
		const Bool depthLess = !info.m_camera.IsReversedProjection();
		const Uint32 msaaSampleCount = Config::cvHairWorksAALevel.Get();

		// Render Fur
		m_hairSDK->StartMSAARendering(msaaSampleCount, depthLess);

		collector.RenderFur( rc );
		//
		m_hairSDK->FinishMSAARendering();
	}
#endif

	// --------------------------
	// Update linear depth buffer
	// --------------------------
	if ( info.IsViewportPresent() )
	{

#ifdef USE_NVIDIA_FUR
		if ( m_hairSDK != nullptr && info.IsShowFlagOn( SHOW_HairAndFur ) && Config::cvHairWorksLevel.Get()>0 )
		{
			// write depth buffer with nvidia hair for motion blur, dof, etc.
			m_hairSDK->DrawMSAAPostDepth(false);
		}
#endif

		// ace_hack: unbing scene depth
		GpuApi::BindTextures( PSSMP_SceneDepth, 1, nullptr, GpuApi::PixelShader );

		// update depth
		{
			CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );
			GetPostProcess()->DrawFeedbackDepthBuffer( info, surfaces );
		}

		// bind back
		GpuApi::TextureRef sceneDepth = surfaces->GetRenderTargetTex( RTN_GBuffer3Depth );
		GpuApi::BindTextures( PSSMP_SceneDepth, 1, &sceneDepth, GpuApi::PixelShader );

		// ensure sampler state preset is set
		GpuApi::SetSamplerStatePreset( PSSMP_SceneDepth, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );
	}
	
	// ---------------------------
	// Distance lights and impostors
	// ---------------------------
	if( collector.HasAnyDistantLights() && info.IsShowFlagOn( SHOW_DistantLights ) )
	{
		PC_SCOPE_RENDER_LVL1( RenderDistanceLights );

		const Float distantLightsIntensityScale = info.m_envParametersArea.m_globalFog.m_distantLightsIntensityScale.GetScalarClampMin( 0.f );
		if ( distantLightsIntensityScale > 0 )
		{
			GpuApi::TextureRef sceneDepth =  surfaces->GetDepthBufferTex();

			GpuApi::RenderTargetSetup oldRenderSetup = GpuApi::GetRenderTargetSetup();
			GpuApi::RenderTargetSetup rtSetupDeapthReadOnly = GpuApi::GetRenderTargetSetup();
			rtSetupDeapthReadOnly.SetDepthStencilTarget( sceneDepth, -1, true );
			GpuApi::SetupRenderTargets( rtSetupDeapthReadOnly );

			CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcAdd );

			// Bind textures
			GpuApi::TextureRef inputTextures[] = { sceneDepth , surfaces->GetRenderTargetTex( RTN_GBuffer0 ) , surfaces->GetRenderTargetTex( RTN_GBuffer1 ) };
			GpuApi::BindTextures( 0, 3, inputTextures, GpuApi::PixelShader );			
			
#ifndef RED_FINAL_BUILD
			//debug ON in the distant lights shader
			if (info.IsShowFlagOn(SHOW_DistantLightsDebug))
			{
				m_shaderDistantLightDebug->Bind();
			}
			else
			{
				m_shaderDistantLight->Bind();
			}
#else
			m_shaderDistantLight->Bind();
#endif

			CRenderDistantLightBatcher* batcher = GetDistantLightBacher();
			// Render lights
			collector.RenderDistantLights( *batcher, distantLightsIntensityScale );

			GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );

			GpuApi::SetupRenderTargets( oldRenderSetup );
		}
	}

	// --------------------
	// Render fur fragments
	// --------------------
#ifdef USE_NVIDIA_FUR
	if ( m_hairSDK != nullptr && info.IsShowFlagOn( SHOW_HairAndFur ) && info.IsShowFlagOn( SHOW_ForwardPass ) && Config::cvHairWorksLevel.Get()>0 )
	{
		PC_SCOPE_RENDER_LVL1( RenderFurTransparent );


		// We don't want to let the fur overwrite some stencil bits. Thats why we are using here masked stencil pass
		const Uint8 writeBits = LC_DynamicObject | LC_Characters;
		// Not all 256 stencil states are generated, so we need to use this macro to clip bits to be below array size (see: BuildDepthStencilStateModeIndex)
		const Uint8 writeMask = ( (Uint8)GpuApi::DSSM_STENCIL_VALUES_RANGE_REGULAR-1 ) & ~(Uint8)LC_VisibleThroughtWalls;
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet_SetStencilMasked, GpuApi::GetMaskedStencilValue( writeBits , 0 , writeMask ) );
		GpuApi::SetRenderSettingsWireframe( targetWireframeSetting );

		rc.m_pass = RP_ForwardLightingTransparent;

		m_hairSDK->DrawMSAAColor( true );
	}	
#endif
	// ---------------------
	// Render dynamic decals
	// ---------------------
	if ( info.IsShowFlagOn( SHOW_DynamicDecals ) )
	{
		// NOTE : If decals appear where hi-res shadows are in use, there will be a difference in the
		// shadow quality. If this is objectionable, we may be able to render the decals that are on
		// "hi-res shadowed" proxies separately, using the hi-res shadow rendering. May be very costly
		// though. For now, we'll just do it like this.
		PC_SCOPE_RENDER_LVL1( RenderDynamicDecals );
		collector.RenderDynamicDecals( rc );
	}

	// -----------------------------------------------------------
	// Surface Frost / Burn FX
	// -----------------------------------------------------------
	if ( GetGameplayFX()->IsEnabled( EPE_SURFACE ) )
	{
		GetGameplayFX()->GetEffect( EPE_SURFACE )->Apply( collector, info, RTN_Color, RTN_Color2 );
	}

	// -----------------------------------------------------------
	// Surface flow (rain drops)
	// -----------------------------------------------------------
	{
		m_postProcess->DrawSurfaceFlow( collector, info, surfaces, RTN_Color, RTN_Color2 );
	}

	// ---------------
	// Render emissive
	// ---------------
	if ( info.IsShowFlagOn( SHOW_Emissive ) && info.m_materialDebugMode == MDM_None )
	{
		PC_SCOPE_RENDER_LVL1( Emissive );

		// Set draw context
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_Emissive );

		// At this moment we render separate emissive pass for deferred and forward lit materials, which is
		// an overhead in case of forward lit materials. It simplifies things though, so I leave it this way.
		rc.m_pass = RP_Emissive;

		// Prepare render states
		GpuApi::SetRenderSettingsWireframe( targetWireframeSetting );

		ERenderingSortGroup groups[] = { RSG_LitOpaqueWithEmissive };
		collector.RenderElementsAllPlanesFrontFirst( ARRAY_COUNT(groups), groups, rc, RECG_ALL );
	}

	// ------------------------
	// Render modulative decals
	// ------------------------
	if ( info.m_materialDebugMode == MDM_None )
	{
		GpuApi::SetupRenderTargets( rtSetup_RTN_Color_WithDepth );

		PC_SCOPE_RENDER_LVL1( DecalsModulative );

		// Set draw context
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_DecalsModulative );

		// Render without lighting
		rc.m_pass = RP_NoLighting;

		// Prepare render states
		GpuApi::SetRenderSettingsWireframe( targetWireframeSetting );

		collector.RenderElementsAllPlanesFrontFirst( RSG_DecalModulativeColor, rc, RECG_ALL );
	}

	GpuApi::SetupRenderTargets( rtSetup_RTN_Color_WithDepth );

	UnbindForwardConsts( info, GpuApi::PixelShader );
	UnbindForwardConsts( info, GpuApi::VertexShader );
}

//dex++: texture preview system
void CRenderInterface::SetTexturePreview( GpuApi::TextureRef tex, Uint32 mipIndex/*=0*/, Uint32 sliceIndex/*=0*/, Float colorMin/*=0.0f*/, Float colorMax/*=1.0f*/ )
{
	m_DynamicPreviewTexture = tex;
	m_DynamicPreviewTextureMip = mipIndex;
	m_DynamicPreviewTextureSlice = sliceIndex;
	m_DynamicPreviewTextureColorMin = colorMin;
	m_DynamicPreviewTextureColorMax = colorMax;
}

void CRenderInterface::RenderTexturePreview()
{
	if ( m_DynamicPreviewTexture )
	{
		m_debugDrawer->DrawTexturePreviewTile( 
			50.0f, 50.0f, 
			512.0f, 512.0f, 
			m_DynamicPreviewTexture,
			m_DynamicPreviewTextureMip,
			m_DynamicPreviewTextureSlice,
			m_DynamicPreviewTextureColorMin,
			m_DynamicPreviewTextureColorMax,
			Vector::ONES);
	}
}
//dex--

void CRenderInterface::ClearGBuffer( const CRenderFrameInfo& info, bool forceClearAll )
{
	PC_SCOPE_RENDER_LVL1(ClearColorTargets);

	// Get the surfaces
	CRenderSurfaces* surfaces = GetRenderer()->GetSurfaces();
	ASSERT( surfaces );

#ifdef RED_PLATFORM_DURANGO
	// Always clear on Xbox, because we can have complete garbage in ESRAM, and end up with awful random colors along skyline (from interlaced
	// albedo/specular color packing).
	// Another option might be to change the interlaced sampling to grab the color/depth from the row above and below, and use the one with
	// depth closest to the pixel actually being considered. Since the buffers are in ESRAM it might be better to just clear them.
	const Bool clearHelperBuffers = true;
#else
	const Bool clearHelperBuffers = forceClearAll || GetPostProcess()->IsDebugOverlayActivated( info, true );
#endif
	const Bool msaaEnabled = IsMSAAEnabled( info );

	if ( clearHelperBuffers )
	{
		ClearColorTarget( surfaces->GetRenderTargetTex( msaaEnabled ? RTN_GBuffer0MSAA : RTN_GBuffer0 ), CRenderSurfaces::GetGBufferDefaultClearColor( 0 ) );
		ClearColorTarget( surfaces->GetRenderTargetTex( msaaEnabled ? RTN_GBuffer2MSAA : RTN_GBuffer2 ), CRenderSurfaces::GetGBufferDefaultClearColor( 2 ) );
		ClearColorTarget( surfaces->GetRenderTargetTex( RTN_GBuffer3Depth ), Vector(10000.f, 0.f, 0.f, 1.f) );
	}

	if ( clearHelperBuffers || msaaEnabled ) // clear normals if msaa enabled to not trash sky coverage mask
	{
		ClearColorTarget( surfaces->GetRenderTargetTex( msaaEnabled ? RTN_GBuffer1MSAA : RTN_GBuffer1 ), CRenderSurfaces::GetGBufferDefaultClearColor( 1 ) );
	}

	ClearDepthStencilTarget( msaaEnabled ? surfaces->GetDepthBufferTexMSAA() : surfaces->GetDepthBufferTex(), GpuApi::GetClearDepthValueRevProjAware(), 0 );
}

// rendering stats global
SceneRenderingStats GRenderingStats;
Bool				GDumpMeshStats = false;

void CRenderInterface::GrabLocalReflectionsData( CRenderCollector& collector )
{
	PC_SCOPE_RENDER_LVL1( RLR_Downsample );

	const CRenderFrameInfo &info = collector.GetRenderFrameInfo();
	CRenderSurfaces *surfaces = GetRenderer()->GetSurfaces();

	Uint32 rlrWidth	= info.m_width / REALTIME_REFLECTIONS_SIZE_DIV;
	Uint32 rlrHeight = info.m_height / REALTIME_REFLECTIONS_SIZE_DIV;
	Vector downsampleRatio ( 1.f / REALTIME_REFLECTIONS_SIZE_DIV, 1.f / REALTIME_REFLECTIONS_SIZE_DIV, (Float)REALTIME_REFLECTIONS_SIZE_DIV, (Float)REALTIME_REFLECTIONS_SIZE_DIV );
	#ifdef REALTIME_REFLECTIONS_FIXED_SIZE
	{
	# if REALTIME_REFLECTIONS_FIXED_SIZE
		rlrWidth = REALTIME_REFLECTIONS_SIZE_WIDTH;
		rlrHeight = REALTIME_REFLECTIONS_SIZE_HEIGHT;
		downsampleRatio.X = rlrWidth / (Float)Max<Uint32>(1,info.m_width);
		downsampleRatio.Y = rlrHeight / (Float)Max<Uint32>(1,info.m_height);
		downsampleRatio.Z = 1.f / Max( 0.0001f, downsampleRatio.X );
		downsampleRatio.W = 1.f / Max( 0.0001f, downsampleRatio.Y );
	# endif
	}
	#else
	# error Expected definition
	#endif
	
	const GpuApi::TextureRef texColor = GetSurfaces()->GetRenderTargetTex( RTN_RLRColor );
	const GpuApi::TextureRef texDepth = GetSurfaces()->GetRenderTargetTex( RTN_RLRDepth );
	
	CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

	// build color/depth textures
	if ( REALTIME_REFLECTIONS_SIZE_DIV > 1 )
	{
		// downsample color texture
		{
			PC_SCOPE_RENDER_LVL1( RLR_DownsampleColor );

			// Build rendertarget setups
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, texColor );
			rtSetup.SetViewport( rlrWidth, rlrHeight, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );

			// Copy albedo from gbuffer to the render target
			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

			// Setup constants
			GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)rlrWidth, (Float)rlrHeight, 0, 0 ) );
			GetStateManager().SetPixelConst( PSC_Custom_1, downsampleRatio );

			// Bind textures
			GpuApi::TextureRef textures[] = { surfaces->GetRenderTargetTex( RTN_Color ), surfaces->GetDepthBufferTex() };
			GpuApi::BindTextures( 0, ARRAY_COUNT( textures ), textures, GpuApi::PixelShader );

			// Render stuff
			m_shaderRLRDownsampleSourceColor->Bind();
			GetDebugDrawer().DrawQuad( Vector( -1.f, -1.f, 0.f, 1.f ), Vector( 1.f, 1.f, 0.f, 1.f ), 0.5f );

			// Unbind textures
			GpuApi::BindTextures( 0, ARRAY_COUNT( textures ), nullptr, GpuApi::PixelShader );
		}
	}

	// downsample depth texture
	// or just copy in case of one2one - it's needed because of the water.
	// it could be further optimized for the fullRes scenario (not copying in case of water not visible),
	// but we won't do the reflection fullres anyway so let's not complicate stuff to much.
	{
		PC_SCOPE_RENDER_LVL1( RLR_DownsampleDepth );

		// Build rendertarget setups
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, texDepth );
		rtSetup.SetViewport( rlrWidth, rlrHeight, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetup );

		// Copy albedo from gbuffer to the render target
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Bind textures
		GpuApi::TextureRef textures[] = { surfaces->GetDepthBufferTex() };
		GpuApi::BindTextures( 0, ARRAY_COUNT( textures ), textures, GpuApi::PixelShader );

		// Setup constants
		GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)rlrWidth, (Float)rlrHeight, 0, 0 ) );
		GetStateManager().SetPixelConst( PSC_Custom_1, downsampleRatio );

		// Render stuff
		m_shaderRLRDownsampleSourceDepth->Bind();
		GetDebugDrawer().DrawQuad( Vector( -1.f, -1.f, 0.f, 1.f ), Vector( 1.f, 1.f, 0.f, 1.f ), 0.5f );

		// Unbind textures
		GpuApi::BindTextures( 0, ARRAY_COUNT( textures ), nullptr, GpuApi::PixelShader );
	}
}

void CRenderInterface::ProcessLocalReflections( CRenderCollector& collector )
{
	const CRenderFrameInfo &info = collector.GetRenderFrameInfo();
	CRenderSurfaces * const surfaces = GetSurfaces();

	const Bool isMainRender = collector.IsWorldScene();

	if ( isMainRender && !info.m_camera.GetLastFrameData().m_isValid )
	{
		surfaces->SetPersistentSurfaceDirty( CRenderSurfaces::PS_RLRHistory, true );
	}

	const Bool shouldRenderGlobalWater = collector.m_scene && collector.m_scene->ShouldRenderWater( info );
	const Bool historyBufferReadAllowed = isMainRender && !surfaces->IsPersistentSurfaceDirty( CRenderSurfaces::PS_RLRHistory );
	const Bool historyBufferWriteAllowed = isMainRender;

	if ( !historyBufferWriteAllowed )
	{
		// We don't support non main RLR right now 
		// (needs a codepath that wouldn't change the RLR history buffer).
		return;
	}
	
	const ERenderTargetName		rtnRLRHistory		= RTN_RLRResultHistory;
	#ifdef RED_PLATFORM_DURANGO
	const ERenderTargetName		rtnRLRResult		= RTN_DURANGO_RLR_Result0;
	#else	
	const ERenderTargetName		rtnRLRResult		= RTN_Color2;
	#endif
	const ERenderTargetName		rtnRLRResult2		= RTN_Color3;
	const ERenderTargetName		rtnRLRSky			= RTN_RLRSky;
	const Uint32				rlrSkyResolution	= GpuApi::GetTextureLevelDesc( GetSurfaces()->GetRenderTargetTex( rtnRLRSky ), 0 ).width;

	Uint32 rlrWidth	= info.m_width / REALTIME_REFLECTIONS_SIZE_DIV;
	Uint32 rlrHeight = info.m_height / REALTIME_REFLECTIONS_SIZE_DIV;
	Vector downsampleRatio ( 1.f / REALTIME_REFLECTIONS_SIZE_DIV, 1.f / REALTIME_REFLECTIONS_SIZE_DIV, (Float)REALTIME_REFLECTIONS_SIZE_DIV, (Float)REALTIME_REFLECTIONS_SIZE_DIV );
	#ifdef REALTIME_REFLECTIONS_FIXED_SIZE
	{
	# if REALTIME_REFLECTIONS_FIXED_SIZE
		rlrWidth	= REALTIME_REFLECTIONS_SIZE_WIDTH;
		rlrHeight	= REALTIME_REFLECTIONS_SIZE_HEIGHT;
		downsampleRatio.X = rlrWidth / (Float)Max<Uint32>(1,info.m_width);
		downsampleRatio.Y = rlrHeight / (Float)Max<Uint32>(1,info.m_height);
		downsampleRatio.Z = 1.f / Max( 0.0001f, downsampleRatio.X );
		downsampleRatio.W = 1.f / Max( 0.0001f, downsampleRatio.Y );
	# endif
	}
	#else
	# error Expected definition
	#endif
	
	// Render fallback 'sky' reflection
	const Vector rlrSkyParams = Vector ( (Float)rlrSkyResolution, (Float)rlrSkyResolution, (Float)surfaces->GetRenderTarget(rtnRLRSky)->GetWidth(), (Float)surfaces->GetRenderTarget(rtnRLRSky)->GetHeight() );
	{
		PC_SCOPE_RENDER_LVL1( RLR_SkyFallback );

		// Build rendertarget setups
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnRLRSky ) );
		rtSetup.SetViewport( rlrSkyResolution, rlrSkyResolution, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetup );

		// Copy albedo from gbuffer to the render target
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		//
		GetStateManager().SetPixelConst( PSC_Custom_0, rlrSkyParams );

		// Render stuff
		CRenderShaderPair *shader = m_shaderSkyColorRLREnvProbe;
		if ( shader )
		{
			shader->Bind();
			GetDebugDrawer().DrawQuad( Vector( -1.f, -1.f, 0.f, 1.f ), Vector( 1.f, 1.f, 0.f, 1.f ), 0.5f );
		}
	}

	// Update local reflections
	if ( info.IsShowFlagOn( SHOW_LocalReflections ) )
	{
		CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

		GetStateManager().SetPixelConst( PSC_Custom_8, downsampleRatio );

		// build color/depth textures
		GpuApi::TextureRef texRLRColor = surfaces->GetRenderTargetTex( RTN_RLRColor );
		GpuApi::TextureRef texRLRDepth = surfaces->GetRenderTargetTex( RTN_RLRDepth );
		
		// generate reflection
		{
			PC_SCOPE_RENDER_LVL1( RLR_Calculate );

			// Build rendertarget setups
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnRLRResult ) );
			rtSetup.SetColorTarget( 1, surfaces->GetRenderTargetTex( rtnRLRResult2 ) );
			rtSetup.SetViewport( rlrWidth, rlrHeight, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );

			//
			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

			// Bind textures
			GpuApi::TextureRef textures[] = { surfaces->GetLocalReflectionsMaskTex(), texRLRColor, texRLRDepth, surfaces->GetRenderTargetTex( rtnRLRHistory ), surfaces->GetDepthBufferTex(), surfaces->GetRenderTargetTex( rtnRLRSky ) };
			GpuApi::BindTextures( 0, ARRAY_COUNT( textures ), textures, GpuApi::PixelShader );

			GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

			const GpuApi::TextureLevelDesc depthTexDesc		= GpuApi::GetTextureLevelDesc( texRLRDepth, 0 );
			const GpuApi::TextureLevelDesc historyTexDesc	= GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( rtnRLRHistory ), 0 );
			const GpuApi::TextureLevelDesc colorTexDesc		= GpuApi::GetTextureLevelDesc( texRLRColor, 0 );
			ASSERT( depthTexDesc.width * historyTexDesc.height == depthTexDesc.height * historyTexDesc.width && "RLRHistory and RLRDepth aspect ration mismatch" );
			ASSERT( depthTexDesc.width * colorTexDesc.height == depthTexDesc.height * colorTexDesc.width && "RLRHistory and RLRColor aspect ration mismatch" );

			GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)rlrWidth, (Float)rlrHeight, (Float)depthTexDesc.width, (Float)depthTexDesc.height ) );
			GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( 0.f, 0.f, 0.f, 0.f ) );
			GetStateManager().SetPixelConst( PSC_Custom_2, info.m_camera.GetLastFrameData().m_isValid ? info.m_camera.GetLastFrameData().m_position : info.m_camera.GetPosition() );
			GetStateManager().SetPixelConst( PSC_Custom_3, Vector ( historyTexDesc.width / (Float)depthTexDesc.width, colorTexDesc.width / (Float)depthTexDesc.width, 0, 0 ) );
			GetStateManager().SetPixelConst( PSC_Custom_4, rlrSkyParams );
			GetStateManager().SetPixelConst( PSC_Custom_5, Vector ( (Float)historyTexDesc.width, (Float)historyTexDesc.height, 1.f/historyTexDesc.width, 1.f/historyTexDesc.height ) );
			const Matrix prevWorldToView = info.m_camera.GetLastFrameData().m_isValid ? info.m_camera.GetLastFrameData().m_worldToView : info.m_camera.GetWorldToView();
			GetStateManager().SetPixelConst( PSC_Custom_10, prevWorldToView );				

			// Render stuff
			(shouldRenderGlobalWater ? m_shaderRLRCalculateWithOcean : m_shaderRLRCalculateNoOcean)->Bind();
			GetDebugDrawer().DrawQuad( Vector( -1.f, -1.f, 0.f, 1.f ), Vector( 1.f, 1.f, 0.f, 1.f ), 0.5f );

			// Unbind textures
			GpuApi::BindTextures( 0, ARRAY_COUNT( textures ), nullptr, GpuApi::PixelShader );
		}

		// calculate number of mips we want to use
		Uint32 numMips = 1;
		{
			const Uint32 numTexMips = surfaces->GetRenderTarget( rtnRLRHistory )->GetNumMipLevels();
			const Uint32 numNeededMips = Min<Uint32>( MLog2( rlrWidth ), MLog2( rlrHeight ) ) + 1; //< this is because of editor
			numMips = Max<Uint32>( 1, Min<Uint32>( numTexMips, numNeededMips ) );
		}

		// extrude
		{
			PC_SCOPE_RENDER_LVL1( RLR_Extrude );

			for ( Uint32 target_mip_i=1; target_mip_i<numMips; ++target_mip_i )
			{
				const GpuApi::TextureLevelDesc &targetMipDesc = GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( rtnRLRHistory), target_mip_i );
				const Uint32 targetWidth = Max<Uint32>( 1, rlrWidth >> target_mip_i );
				const Uint32 targetHeight = Max<Uint32>( 1, rlrHeight >> target_mip_i );

				const GpuApi::TextureRef sourceTexture = surfaces->GetRenderTargetTex( target_mip_i > 1 ? rtnRLRHistory : rtnRLRResult );
				const Uint32 sourceTextureMip = target_mip_i - 1;
				const Uint32 sourceWidth = Max<Uint32>( 1, rlrWidth >> sourceTextureMip );
				const Uint32 sourcetHeight = Max<Uint32>( 1, rlrHeight >> sourceTextureMip );
				const GpuApi::TextureLevelDesc sourceTextureDesc = GpuApi::GetTextureLevelDesc( sourceTexture, sourceTextureMip );

				// Build rendertarget setups
				GpuApi::RenderTargetSetup rtSetup;
				rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnRLRHistory ), target_mip_i );
				rtSetup.SetViewport( targetWidth, targetHeight, 0, 0 );
				GpuApi::SetupRenderTargets( rtSetup );

				//
				CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

				// Bind textures
				if ( 0 == sourceTextureMip )
				{
					GpuApi::BindTextures( 0, 1, &sourceTexture, GpuApi::PixelShader );
				}
				else
				{
					GpuApi::BindTextureMipLevel( 0, sourceTexture, 0, sourceTextureMip, GpuApi::PixelShader );
				}
				//GpuApi::BindTextures( 0, 1, &sourceTexture, GpuApi::PixelShader );
				
				GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );
				GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

				GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)sourceWidth, (Float)sourcetHeight, (Float)sourceTextureDesc.width, (Float)sourceTextureDesc.height ) );
				GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (Float)targetWidth, (Float)targetHeight, 0, 0 ) );
				GetStateManager().SetPixelConst( PSC_Custom_2, Vector ( (Float)sourceTextureMip, 0, 0, 0 ) );
				
				// Render stuff
				m_shaderRLRExtrude->Bind();
				GetDebugDrawer().DrawQuad( Vector( -1.f, -1.f, 0.f, 1.f ), Vector( 1.f, 1.f, 0.f, 1.f ), 0.5f );

				// Unbind textures
				GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
			}
		}

		// merge extruded
		const Uint32 mergeExtrudedMipOffset = Min<Uint32>( 2, numMips - 1 );
		{
			PC_SCOPE_RENDER_LVL1( RLR_MergeExtruded );

			// Build rendertarget setups
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnRLRResult ) );
			rtSetup.SetViewport( rlrWidth, rlrHeight, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );

			//
			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

			// Bind textures
			GpuApi::TextureRef textures[] = { surfaces->GetLocalReflectionsMaskTex(), surfaces->GetRenderTargetTex( rtnRLRHistory ) };
			GpuApi::BindTextures( 0, ARRAY_COUNT( textures ), textures, GpuApi::PixelShader );
			
			GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampPointMip, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearMip, GpuApi::PixelShader );

			GpuApi::TextureLevelDesc historyDesc = GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( rtnRLRHistory ), 0 );

			GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)rlrWidth, rlrHeight, (Float)historyDesc.width, (Float)historyDesc.height ) );
			GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (Float)numMips-1, (Float)Max<Uint32>( 1, mergeExtrudedMipOffset ), (Float)mergeExtrudedMipOffset, 0 ) );			
			GetStateManager().SetPixelConst( PSC_Custom_2, Vector ( 1.f / rtSetup.viewport.width * rlrWidth / historyDesc.width, 1.f / rtSetup.viewport.height * rlrHeight / historyDesc.height, 0, 0 ) );			

			// Render stuff
			m_shaderRLRMergeExtruded->Bind();
			GetDebugDrawer().DrawQuad( Vector( -1.f, -1.f, 0.f, 1.f ), Vector( 1.f, 1.f, 0.f, 1.f ), 0.5f );

			// Unbind textures
			GpuApi::BindTextures( 0, ARRAY_COUNT( textures ), nullptr, GpuApi::PixelShader );
		}

		// merge reflection
		{
			PC_SCOPE_RENDER_LVL1( RLR_Merge );

		#ifdef RED_PLATFORM_WINPC
			// Determine the blendover factor based on fps.
			// Magic values are setup to match our previously fixed blendover factor, with calculations
			// made with assumption that we were rendering with 30fps. For fps result of this will match previous factor.
			// Clamps are to ensure some sane values, especially we don't want the blendover to be too low so that we wouldnt't
			// introduce any numerical precision related issues.
			const Float predictionTimeDelta = GetLastTickDelta();
			const Float predictionPeriod = 0.5f;
			const Float predictionFrames = predictionPeriod / Max( 0.001f, (Float)predictionTimeDelta );
			const Float expectedCoverage = 0.0873542204f;
			const Float predictedBlendOverAlpha = 1 - powf( expectedCoverage, 1.f / predictionFrames );

			const Float blendOverAlphaMin = 0.06f;
			const Float blendOverAlphaMax = 0.15f;
			
			const Float blendOverAlpha = historyBufferReadAllowed ? Clamp( predictedBlendOverAlpha, blendOverAlphaMin, blendOverAlphaMax ) : 1.f;
		#else
			const Float blendOverAlpha = historyBufferReadAllowed ? 0.15f : 1.f;
		#endif

			// Build rendertarget setups
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnRLRHistory ) );
			rtSetup.SetViewport( rlrWidth, rlrHeight, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );

			//
			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

			// Bind textures
			GpuApi::TextureRef textures[] = { surfaces->GetLocalReflectionsMaskTex(), surfaces->GetRenderTargetTex( rtnRLRResult ), surfaces->GetRenderTargetTex( rtnRLRResult2 ), surfaces->GetRenderTargetTex( RTN_RLRDepth ), surfaces->GetRenderTargetTex( RTN_RLRColor ) };
			GpuApi::BindTextures( 0, ARRAY_COUNT( textures ), textures, GpuApi::PixelShader );
			const GpuApi::TextureLevelDesc resultTexDesc = GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( rtnRLRResult ), 0 );
			const GpuApi::TextureLevelDesc extrudedTexDesc = GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( RTN_RLRColor ), 0 );

			GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

			GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)rlrWidth, (Float)rlrHeight, 0, 0 ) );
			GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (Float)resultTexDesc.width, (Float)resultTexDesc.height, 1.f/resultTexDesc.width, 1.f/resultTexDesc.height ) );
			GetStateManager().SetPixelConst( PSC_Custom_2, Vector ( blendOverAlpha, 0.f, 0.f, 0.f ) );
			const Matrix prevWorldToView = info.m_camera.GetLastFrameData().m_isValid ? info.m_camera.GetLastFrameData().m_worldToView : info.m_camera.GetWorldToView();
			GetStateManager().SetPixelConst( PSC_Custom_3, prevWorldToView );				
			//GetStateManager().SetPixelConst( PSC_Custom_7, Vector ( (Float)Max<Uint32>( 1, rlrWidth >> mergeExtrudedMipOffset ), (Float)Max<Uint32>( 1, rlrHeight >> mergeExtrudedMipOffset ), (Float)extrudedTexDesc.width, (Float)extrudedTexDesc.height ) );

			const Float extrudedAreaWidth = (Float)Max<Uint32>(1, rlrWidth >> mergeExtrudedMipOffset);
			const Float extrudedAreaHeight = (Float)Max<Uint32>(1, rlrHeight >> mergeExtrudedMipOffset);
			const Float extrudedTextureWidth = (Float)extrudedTexDesc.width;
			const Float extrudedTextureHeight = (Float)extrudedTexDesc.height;
			const Float extrudedScaleX = 1.f / rlrWidth * extrudedAreaWidth / extrudedTextureWidth;
			const Float extrudedScaleY = 1.f / rlrHeight * extrudedAreaHeight / extrudedTextureHeight;
			const Float extrudedClampX = (extrudedAreaWidth - 0.5f) / extrudedTextureWidth;
			const Float extrudedClampY = (extrudedAreaHeight - 0.5f) / extrudedTextureHeight;
			GetStateManager().SetPixelConst( PSC_Custom_7, Vector ( extrudedScaleX, extrudedScaleY, extrudedClampX, extrudedClampY ) );

			// Render stuff
			m_shaderRLRMerge->Bind();
			GetDebugDrawer().DrawQuad( Vector( -1.f, -1.f, 0.f, 1.f ), Vector( 1.f, 1.f, 0.f, 1.f ), 0.5f );

			// Unbind textures
			GpuApi::BindTextures( 0, ARRAY_COUNT( textures ), nullptr, GpuApi::PixelShader );
		}

		// mark history buffer as non dirty
		if ( historyBufferWriteAllowed )
		{
			surfaces->SetPersistentSurfaceDirty( CRenderSurfaces::PS_RLRHistory, false );
		}
	}

	// Apply local reflections (or fallback envProbe only data)
	{
		const Uint32 rlrTotalMips = surfaces->GetRenderTarget( rtnRLRHistory )->GetNumMipLevels();
		const Bool rlrHasMips = rlrTotalMips > 1;
		if ( rlrHasMips )
		{
			PC_SCOPE_RENDER_LVL1( RLR_GenMips );
			GenerateMipmaps( surfaces->GetRenderTargetTex( rtnRLRHistory ) );
		}

		PC_SCOPE_RENDER_LVL1( RLR_Apply );

		const Bool isFullApply = info.IsShowFlagOn( SHOW_LocalReflections );

		// Build rendertarget setups
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_Color ) );
		rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetup );

		//
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcAdd );

		const GpuApi::TextureLevelDesc &maskDesc = GpuApi::GetTextureLevelDesc( surfaces->GetLocalReflectionsMaskTex(), 0 );

		GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)rlrWidth, (Float)rlrHeight, (Float)GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( rtnRLRHistory ), 0 ).width, (Float)GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( rtnRLRHistory ), 0 ).height ) );
		GetStateManager().SetPixelConst( PSC_Custom_1, rlrSkyParams );
		GetStateManager().SetPixelConst( PSC_Custom_2, Vector ( (Float)maskDesc.width, (Float)maskDesc.height, 1.f / maskDesc.width, 1.f / maskDesc.height ) );

		// Bind textures
		GpuApi::TextureRef textures[] = 
		{ 
			surfaces->GetRenderTargetTex( rtnRLRHistory ), 
			surfaces->GetLocalReflectionsMaskTex(), 
			isFullApply ? GpuApi::TextureRef::Null() : surfaces->GetDepthBufferTex(),
			surfaces->GetRenderTargetTex( rtnRLRSky )
		};
		GpuApi::BindTextures( 0, ARRAY_COUNT( textures ), textures, GpuApi::PixelShader );

		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 2, rlrHasMips ? GpuApi::SAMPSTATEPRESET_ClampLinearMip : GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Render stuff
		(isFullApply ? m_shaderRLRApplyTraced : m_shaderRLRApplyFallback)->Bind();
		GetDebugDrawer().DrawQuad( Vector( -1.f, -1.f, 0.f, 1.f ), Vector( 1.f, 1.f, 0.f, 1.f ), 0.5f );

		// Unbind textures
		GpuApi::BindTextures( 0, ARRAY_COUNT( textures ), nullptr, GpuApi::PixelShader );
	}
}


void CRenderInterface::UpdateCameraInteriorFactor( CRenderCollector& collector )
{
	if ( collector.m_scene )
	{
		PC_SCOPE_RENDER_LVL1( CameraInteriorFactor );

		if ( m_shaderCameraInteriorFactor )
		{
			const CRenderFrameInfo& info = collector.GetRenderFrameInfo();
			CRenderSurfaces* surfaces = GetSurfaces();
			RED_ASSERT( surfaces );

			GpuApi::TextureRef texInteriorFactor = surfaces->GetRenderTargetTex( RTN_CameraInteriorFactor );

			// Render interior value
			{
				GpuApi::RenderTargetSetup rtSetup;
				rtSetup.SetColorTarget( 0, texInteriorFactor );
				rtSetup.SetViewportFromTarget( texInteriorFactor );
				GpuApi::SetupRenderTargets( rtSetup );

				BindForwardConsts( info, GetGlobalCascadesShadowResources(), surfaces, true, GpuApi::PixelShader );

				const Vector cameraTestPosition = info.m_camera.GetPosition() + info.m_camera.GetCameraForward() * info.m_camera.GetNearPlane();
				GetStateManager().SetPixelConst( PSC_Custom_0, cameraTestPosition );

				CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );
				m_shaderCameraInteriorFactor->Bind();
				GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 0), Vector (1, 1, 0, 0), 0.5f );

				UnbindForwardConsts( info, GpuApi::PixelShader );
			}

			// Update value in scene
			collector.m_scene->UpdateCameraInteriorFactor( texInteriorFactor, false );
		}
	}
}


void CRenderInterface::UpdateLocalShadowMaps( CRenderCollector& collector )
{
	// Most of this only needs to be done once.
	if ( GIsDuringUberSampleIsFirst )
	{
		// Render local shadowmaps (only if shadows are enabled)
		if ( collector.GetRenderFrameInfo().IsShowFlagOn( SHOW_Shadows ) )
		{
			PC_SCOPE_RENDER_LVL1( RenderLocalLightsShadows );

			// Setup reversed projection
			const Bool origReversedProjection = GpuApi::IsReversedProjectionState();
			GpuApi::SetReversedProjectionState( false );

			CRenderSurfaces* surfaces = GetSurfaces();
			const CRenderFrameInfo& info = collector.GetRenderFrameInfo();

			SetupForSceneRender( info );

			// Setup shared constants
			CalculateSharedConstants( info, surfaces->GetWidth(), surfaces->GetHeight(), -1, -1 );
			BindSharedConstants( GpuApi::PixelShader );

			if ( collector.IsWorldScene() )
			{
				m_shadowManager->RenderStaticShadowmaps( collector );
			}

			m_shadowManager->RenderDynamicShadowmaps( collector );

			GpuApi::SetReversedProjectionState( origReversedProjection );
		}
	}
}


void CRenderInterface::PreRenderNormalFrame( CRenderCollector& collector )
{
	PC_SCOPE_RENDER_LVL0(PreRenderNormalFrame);

	const CRenderFrameInfo& info = collector.GetRenderFrameInfo();

	// Hack: Update debug preview index browser
	//       It's in this inappropriate place to minimise chance for buildmachine compilation failure.
	{
		Bool currChecked = info.IsShowFlagOn( SHOW_BrowseDebugPreviews );
		static Bool prevChecked = currChecked;

		if ( currChecked != prevChecked )
		{
			prevChecked = currChecked;
			if ( currChecked )
			{
				Config::cvForcedDebugPreviewIndex.Set( (Max( 0, Config::cvForcedDebugPreviewIndex.Get() + 1 ) + 1) % (EMM_MAX + 1) - 1 );
			}
		}

#ifndef RED_FINAL_BUILD
		EEnvManagerModifier modifier = CEnvironmentManager::GetModifier();
		if ( Config::cvForcedDebugPreviewIndex.Get() != (Int32)EMM_None )
		{
			modifier = (EEnvManagerModifier)Max( 0, Config::cvForcedDebugPreviewIndex.Get() );
		}
		if ( modifier != EMM_None )
		{
			collector.m_frame->AddDebugScreenFormatedText( info.m_width - 220, 50, TXT("DebugPreview: %s"), GetDebugPreviewName( modifier ) );
		}
#endif // RED_FINAL_BUILD
	}


	// Get rendering surfaces to use
	CRenderSurfaces* surfaces = GetSurfaces();
	ASSERT( surfaces );

	// Reset state manager
	m_stateManager->Reset();

	// Disable characters lighting boost
	m_stateManager->SetPixelConst( PSC_CharactersLightingBoost, Vector ( 1, 1, 1, 1 ) );

	// 
	const Bool origReversedProjection = GpuApi::IsReversedProjectionState();
	GpuApi::SetReversedProjectionState( false );
	ASSERT( !origReversedProjection );

#ifndef START_ASYNC_WATER_LATER
	// Simulate water
	if ( collector.m_scene && collector.m_scene->ShouldRenderWater( info ) )
	{
		PC_SCOPE_RENDER_LVL1( SimulateWater );
		collector.m_scene->SimulateWater(info);
	}
#endif

	// Global shadow pass
	// Needs to be before envProbes update (they make use of cascades and terrain shadows)
	{
		PC_SCOPE_RENDER_LVL0( GlobalShadowPass );

		SetupForSceneRender( info );

		// Setup shared constants
		CalculateSharedConstants( info, surfaces->GetWidth(), surfaces->GetHeight(), -1, -1 );
		BindSharedConstants( GpuApi::PixelShader );

		// Render shadows
		if ( collector.IsRenderingSunLightingEnabled() )
		{
			// Disable wireframe
			GpuApi::SetRenderSettingsWireframe( false );

			// if this wasn't already finished then make sure that it's finished here
			collector.FinishShadowCulling();

			// Render the shadow cascades
			RenderShadows( info, collector.m_cascades, GetGlobalCascadesShadowResources() );
		}

		// Terrain shadows integration, only for world scenes
		if ( collector.IsWorldScene() )
		{
			collector.m_scene->GetTerrainShadows()->PrepareShadows( collector );
		}
	}

	// Update envprobes
	if ( info.m_allowPostSceneRender && collector.IsWorldScene() && ( 0 != (info.m_renderFeaturesFlags & DMFF_ToneMappingLuminanceUpdate) ) )
	{
		GetEnvProbeManager()->Update( collector );
	}


#ifdef START_ASYNC_WATER_LATER
	// Simulate water
	if ( collector.m_scene && collector.m_scene->ShouldRenderWater( info ) )
	{
		PC_SCOPE_RENDER_LVL1( SimulateWater );
		collector.m_scene->SimulateWater(info);
	}
#endif


	// Setup global camera parameters
	SetupForSceneRender( info );

	// Setup shared constants
	CalculateSharedConstants( info, surfaces->GetWidth(), surfaces->GetHeight(), -1, -1 );
	BindSharedConstants( GpuApi::PixelShader );


	// Finish occlusion query before we start rendering. If we wait much longer than this, we may end up repeating the same
	// stuff multiple times (for ubersampling)
	collector.FinishSceneOcclusionQuery();

	if ( collector.m_scene )
	{
		if ( collector.m_scene->GetTerrain() != nullptr )
		{
			collector.m_scene->BuildTerrainQuadTree( collector.GetScene(), info );
		}

#ifdef USE_SPEED_TREE
		// Update speedtree
		if ( info.IsShowFlagOn( SHOW_Foliage ) )
		{
			PC_SCOPE_RENDER_LVL1( UpdateSpeedTree );

			CRenderProxy_SpeedTree* speedTree = collector.m_scene->GetSpeedTree();
			if ( speedTree )
			{
				speedTree->PreRenderUpdate( info, collector.m_scene, 
#ifdef USE_UMBRA					
					&collector.GetOcclusionData(), 
#endif // USE_UMBRA					
					collector.m_frame );
			}
		}
#endif

#if defined(USE_UMBRA) && !defined(RED_FINAL_BUILD)
		if ( collector.m_scene->GetOcclusionData() )
		{
			GRenderingStats.m_occlusionTimeQuery = collector.m_scene->GetOcclusionData()->GetQueryTime();
		}
#endif // USED_UMBRA && !RED_FINAL_BUILD
	}


	GpuApi::SetReversedProjectionState( origReversedProjection );
}

void CRenderInterface::RenderNormalFrame( CRenderCollector& collector )
{
	PC_SCOPE_RENDER_LVL0(RenderNormalFrame);

	const CRenderFrameInfo& info = collector.m_frame->GetFrameInfo();

	// Get rendering surfaces to use
	CRenderSurfaces* surfaces = GetSurfaces();
	ASSERT( surfaces );

	// Reset state manager
	m_stateManager->Reset();

	// Disable characters lighting boost
	m_stateManager->SetPixelConst( PSC_CharactersLightingBoost, Vector ( 1, 1, 1, 1 ) );

	const Bool msaaEnabled				= IsMSAAEnabled( info );
	const Bool isTakingScreenShot		= SScreenshotSystem::GetInstance().IsTakingScreenshot();


	// Build rendertarget setups
	GpuApi::RenderTargetSetup rtSetupMain;
	rtSetupMain.SetColorTarget( 0, surfaces->GetRenderTargetTex( msaaEnabled ? RTN_MSAAColor : RTN_Color) );
	rtSetupMain.SetDepthStencilTarget( msaaEnabled ? surfaces->GetDepthBufferTexMSAA() : surfaces->GetDepthBufferTex() );
	rtSetupMain.SetViewport( info.m_width, info.m_height, 0, 0 );

	// Save original wireframe setting
	const Bool origWireframeSetting		= GpuApi::GetRenderSettings().wireframe;
	const Bool targetWireframeSetting	= info.IsShowFlagOn( SHOW_Wireframe );

	// Setup for scene render
	SetupForSceneRender( info );

	// Setup shared constants
	CalculateSharedConstants( collector.m_frame->GetFrameInfo(), surfaces->GetWidth(), surfaces->GetHeight(), -1, -1 );
	BindSharedConstants( GpuApi::PixelShader );

	// Calculate tile deferred constants in case some shaders materials need them (we don't want to have data from previous frame in this case).
	// Do this without dimmers and lights, since they are not collected yet.
	{
		// Calculate constants
		{
			const CRenderCollector::CRenderCollectorData *rdata = collector.m_renderCollectorData;
			const CRenderFrameInfo &info = collector.GetRenderFrameInfo();

			CalculateTiledDeferredConstants_Various( collector.GetRenderFrameInfo() );
			CalculateTiledDeferredConstants_CascadeShadows( collector.GetRenderFrameInfo(), &collector.m_cascades, m_cascadeShadowResources );
			CalculateTiledDeferredConstants_TerrainShadows( collector.GetRenderFrameInfo(), collector.m_scene );

			// No dimmers/lights yet (passing 0 count)
			CalculateTiledDeferredConstants_Lights( &info, info.m_camera.GetPosition(), info.m_worldRenderSettings, info.m_envParametersArea.m_colorGroups, 0, nullptr, false, true, 0 );
			CalculateTiledDeferredConstants_Dimmers( collector.GetRenderFrameInfo(), 0, nullptr );
		}

		// Flush buffer
		FlushTiledDeferredConstants( false );
	}

	//////////////////////////////////////////////////////////////////////////
	// Wait for statics to finish culling. We'll start filling the gbuffer
	// right away, while dynamic objects are still going.
	//////////////////////////////////////////////////////////////////////////
	collector.FinishSceneStaticsCulling();

	// Setup reversed projection
	const Bool origReversedProjection = GpuApi::IsReversedProjectionState();
	ASSERT( !origReversedProjection );
	GpuApi::SetReversedProjectionState( info.m_camera.IsReversedProjection() );
	
	// clearing rtn3 here instead of in renderwaterblends, cause its not always called
	// clearing in special way also, since the RTN_Color3 is shared between volumes (RG) and water blends (B)
	{
		const Float volumesClearValue = 1000.f; //< high value to cover really deep volumes, so that when look from inside, the exterior value would be lower than interior.
		
#ifdef RED_PLATFORM_DURANGO
		ClearColorTarget( surfaces->GetRenderTargetTex( RTN_Color3 ), Vector( volumesClearValue, volumesClearValue, 0.0f, 0.0f ) ); //< leaving this clear here because waterBlends need this
		ClearColorTarget( surfaces->GetRenderTargetTex( RTN_DURANGO_InteriorVolume_TempSurface ), Vector( volumesClearValue, volumesClearValue, 0.0f, 0.0f ) );
#else
		ClearColorTarget( surfaces->GetRenderTargetTex( RTN_Color3 ), Vector( volumesClearValue, volumesClearValue, 0.0f, 0.0f ) );		
#endif
	}
	
	// Decals water blend
	const Bool shouldRenderGlobalWater = collector.m_scene && collector.m_scene->ShouldRenderWater( info );
	if( shouldRenderGlobalWater )
	{
		RenderWaterBlends( info, collector, surfaces->GetRenderTargetTex( RTN_Color3 ) );
	}

	// Interior volumes pass (needed for weather system - particles, rain surface response, underwater volumes cut etc.)	
	RenderVolumes( info, collector, surfaces->GetRenderTargetTex( RTN_InteriorVolume ) );

	// Setup for scene render
	SetupForSceneRender( info );

	// Clear and bind the gbuffer render targets
	RenderDeferredSetupGBuffer( info );

	RenderDeferredFillGBuffer_StaticMeshes( collector );


	//////////////////////////////////////////////////////////////////////////
	// Now wait for the rest of the scene culling so we can render everything.
	//////////////////////////////////////////////////////////////////////////
	collector.FinishSceneCulling();

	// Update camera interior factor
	// Needs to be after culling have completed !!!
	{
		// Bind dimmers to constant buffer (needed for interior factor update)
		const CRenderCollector::CRenderCollectorData *rdata = collector.m_renderCollectorData;
		const CRenderFrameInfo &info = collector.GetRenderFrameInfo();
		CalculateTiledDeferredConstants_Dimmers( collector.GetRenderFrameInfo(), rdata ? rdata->m_dimmers.Size() : 0, rdata ? rdata->m_dimmers.TypedData() : 0 );
		FlushTiledDeferredConstants( false );

		// Update camera interior factor
		{
			CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );
			GpuApi::SetupBlankRenderTargets();
			UpdateCameraInteriorFactor( collector );
		}

		// Setup for scene render 
		// Not sure it it's really needed, since it's hard to track the states.
		// Adding it just in case.
		SetupForSceneRender( info );
	}

	// A bit of updating required now that the scene has been collected. This stuff generally won't be required by the next
	// FillGBuffer step, but it should be done early so that, for example, the particle simulation task has more time to run.
	// Only do this stuff once when doing ubersampling.
	if ( GIsDuringUberSampleIsFirst )
	{
		// Invoke a job thread simulating particles
		GetParticleBatcher()->SimulateOnScreenParticles( collector );


		// Mark the frame as finished - we've collected everything for rendering
		if ( collector.m_scene && collector.m_scene->GetVisibilityQueryManager() )
		{
			collector.m_scene->GetVisibilityQueryManager()->FrameFinished();
		}


		// Update active flares
		if ( collector.m_scene )
		{
			PC_SCOPE_RENDER_LVL1( UpdateActiveFlares );

			if ( !isTakingScreenShot )
			{
				const Float frameTime = GetLastTickDelta();
				const CFrustum frustum ( info.m_camera.GetWorldToScreen() );
				collector.m_scene->UpdateActiveFlares( info.m_camera, frustum, frameTime );
			}
		}
	}

	// Finish filling gbuffer
	RenderDeferredFillGBuffer_NonStatics( collector );

	// Do all post-gbuffer opaque work (apply lighting, do forward opaques, etc)

	// Post (weather / rain specular hack)
	if( collector.m_scene && ( info.m_envParametersDayPoint.m_weatherEffectStrength > 0.0f || collector.m_scene->ShouldRenderUnderwater( info ) ) )
	{
		RenderDeferredPostGBuffer( collector, info.m_envParametersDayPoint.m_weatherEffectStrength );
	}


	//////////////////////////////////////////////////////////////////////////
	// GBuffer is filled. Now we need to generate shadow maps for the local
	// lights. Must be done before we can continue with scene rendering.
	//////////////////////////////////////////////////////////////////////////
	collector.FinishDynamicShadowsCollection();	

	UpdateLocalShadowMaps( collector );

	// Bind lights to constant buffer and perform interior factor based patching
	{
		const Float cameraInteriorFactor = collector.m_scene ? collector.m_scene->GetDelayedCameraInteriorFactor() : 0.f;
		const CRenderCollector::CRenderCollectorData *rdata = collector.m_renderCollectorData;
		const CRenderFrameInfo &info = collector.GetRenderFrameInfo();
		CalculateTiledDeferredConstants_Lights( &info, info.m_camera.GetPosition(), info.m_worldRenderSettings, info.m_envParametersArea.m_colorGroups, rdata ? rdata->m_lights.Size() : 0, rdata ? rdata->m_lights.TypedData() : 0, collector.GetRenderFrameInfo().IsShowFlagOn( SHOW_Shadows ), true, cameraInteriorFactor );
		FlushTiledDeferredConstants( true ); //< patching allowed (we already have a valid interiorFactor values)
	}

	//////////////////////////////////////////////////////////////////////////
	// And now we can finish the rest of rendering. The full scene has been
	// collected and we have the shadows maps ready, so we're good to go.
	//////////////////////////////////////////////////////////////////////////
	RenderOpaqueAfterGBuffer( collector );


	// Setup render context
	RenderingContext rc( info.m_camera );
	rc.m_terrainToolStampVisible		= info.IsTerrainToolStampVisible();
	rc.m_materialDebugMode				= info.m_materialDebugMode;


	// -------------------
	// Bind forward consts
	// -------------------
	BindForwardConsts( info, GetGlobalCascadesShadowResources(), surfaces, true, GpuApi::PixelShader );
	BindForwardConsts( info, GetGlobalCascadesShadowResources(), surfaces, true, GpuApi::VertexShader );

	// ---------------------------------
	// Define flares transparency helper
	// ---------------------------------
	GpuApi::TextureRef texFlaresTranspHelperOpaque;
	if ( !isTakingScreenShot )
	{
		if ( info.IsShowFlagOn( SHOW_Flares ) && info.m_materialDebugMode == MDM_None )
		{
			texFlaresTranspHelperOpaque = surfaces->GetRenderTargetTex( RTN_Color2 );
		}
	}

	// ----------------------------------
	// Render skybox without clouds layer
	// ----------------------------------
	if ( info.IsShowFlagOn( SHOW_Skybox ) && info.m_materialDebugMode == MDM_None )
	{
		PC_SCOPE_RENDER_LVL0( RenderTransparentBackground );

		// Set wireframe
		GpuApi::SetRenderSettingsWireframe( targetWireframeSetting );

		// Render skybox
		CRenderSkybox *skybox = collector.m_scene ? static_cast<CRenderSceneEx*>( collector.m_scene )->GetSkybox() : nullptr;
		if ( skybox )			
		{
			rc.m_pass = RP_NoLighting;
			skybox->Render( info, rc, collector.m_sceneStats, true, false );
		}
	}

	// --------------------------------------
	// Grab flares transparency helper OPAQUE
	// --------------------------------------
	if ( texFlaresTranspHelperOpaque )
	{
		collector.GrabFlaresTransparencyHelpers( info, texFlaresTranspHelperOpaque, rtSetupMain.colorTargets[0] );
	}

	// --------------------------
	// Render skybox clouds layer
	// --------------------------
	if ( info.IsShowFlagOn( SHOW_Skybox ) && info.m_materialDebugMode == MDM_None )
	{
		PC_SCOPE_RENDER_LVL1( RenderTransparentBackground );

		// Set wireframe
		GpuApi::SetRenderSettingsWireframe( targetWireframeSetting );

		// Render skybox
		CRenderSkybox *skybox = collector.m_scene ? static_cast<CRenderSceneEx*>( collector.m_scene )->GetSkybox() : nullptr;
		if ( skybox )			
		{
			rc.m_pass = RP_NoLighting;
			skybox->Render( info, rc, collector.m_sceneStats, false, true );
		}
	}

	// (for item thumbnail generation) get rid off of sky or sth that made it into the render target but isn't needed for thumbnails
	if ( info.m_renderTarget && info.m_renderTarget->GetCopyMode() == IRenderGameplayRenderTarget::CopyMode::BackgroundColor &&  info.m_renderTarget->GetBackgroundColor() != Vector::ZEROS )
	{
		GpuApi::SetupRenderTargets( rtSetupMain );
		GpuApi::SetCustomDrawContext( GpuApi::DSSM_NoStencilDepthTestEqual, GpuApi::RASTERIZERMODE_DefaultNoCull, GpuApi::BLENDMODE_Set );

		// Setup global camera parameters
		m_stateManager->SetLocalToWorld( NULL );
		m_stateManager->SetCamera2D();

		GetRenderer()->m_shaderSingleColor->Bind();
		GetRenderer()->GetStateManager().SetPixelConst( PSC_ConstColor, info.m_renderTarget->GetBackgroundColor() );

		GetDebugDrawer().DrawQuad( Vector( 0.f, 0.f, 0.f, 1.f ), Vector( rtSetupMain.viewport.width, rtSetupMain.viewport.height, 0.f, 1.f ), 0.0f );

		m_stateManager->SetCamera( info.m_camera );
	}

	// --------------------------------
	// Render 'background' transparency
	// --------------------------------
	if ( info.m_materialDebugMode == MDM_None )
	{
		PC_SCOPE_RENDER_LVL1( RenderTransparentBackground );

		// Set draw context
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_Transparency );

		// Render without lighting
		rc.m_pass = RP_NoLighting;		

		// Set wireframe
		GpuApi::SetRenderSettingsWireframe( targetWireframeSetting );

		// Render fragments
		COMPILE_ASSERT( 2 == RPl_Max );
		ERenderingSortGroup groups[] = { RSG_Transparent, RSG_TransparentFullres };
		collector.RenderElements( ARRAY_COUNT(groups), groups, rc, RPl_Background, RECG_ALL );
	}

	// -----------------------------------------------------------
	// Update sky flares
	// This stuff is updated right after background transparencies
	// get rendered (e.g. particle clouds). This is because we're using
	// temp buffer that stores color from before transparencies.
	// In theory we should do this after all transparencies, but
	// I prefer to do this here so that temp target could be reused 
	// for some other stuff.
	// -----------------------------------------------------------
	if ( !isTakingScreenShot )
	{
		if ( info.IsShowFlagOn( SHOW_Flares ) && info.m_materialDebugMode == MDM_None )
		{	
			GpuApi::RenderTargetSetup rtDepthOnlySetup = rtSetupMain;
			rtDepthOnlySetup.SetNullColorTarget();
			GpuApi::SetupRenderTargets( rtDepthOnlySetup );

			PC_SCOPE_RENDER_LVL1( UpdateFlaresOcclusion_Sky );
			collector.UpdateSkyFlaresOcclusion( texFlaresTranspHelperOpaque, rtSetupMain.colorTargets[0] );

			GpuApi::SetupRenderTargets( rtSetupMain );
		}
	}
	texFlaresTranspHelperOpaque = GpuApi::TextureRef::Null();

	{
		// ----------------------------
		// Clear local reflections mask
		// ----------------------------
		{
			CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );
			GetRenderer()->ClearColorTarget( surfaces->GetLocalReflectionsMaskTex(), REALTIME_REFLECTIONS_CLEAR_VALUE );
		}

		// Process local reflections
		Bool isGrabLocalReflectionDataPending = (collector.HasAnyReflectiveMaskedElements() || ( ( collector.m_scene && collector.m_scene->ShouldRenderWater( info )) && info.IsShowFlagOn( SHOW_LocalReflections ) ) );

		// -----------------------------
		// Render transparencyBackground
		// -----------------------------
		if ( info.m_materialDebugMode == MDM_None && info.IsShowFlagOn( SHOW_Refraction ) && collector.HasAnySortGroupElements( RSG_TransparentBackground ) )
		{
			PC_SCOPE_RENDER_LVL1( TransparencyBackground );

			// Set draw context
			CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_TransparentBackground );

			// Render without lighting
			rc.m_pass = RP_NoLighting;

			// Bind scene depth texture
			GpuApi::TextureRef sceneDepth = surfaces->GetRenderTargetTex( RTN_GBuffer3Depth );
			GpuApi::BindTextures( PSSMP_SceneDepth, 1, &sceneDepth, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( PSSMP_SceneDepth, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );

			GpuApi::TextureRef shadowRefs[2] = { GetGlobalCascadesShadowResources().GetDepthStencilArrayRead(), GpuApi::GetInternalTexture( GpuApi::INTERTEX_PoissonRotation) };
			GpuApi::BindTextures( 4, 2, &(shadowRefs[0]), GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 4, GpuApi::SAMPSTATEPRESET_ClampPointMip, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 5, GpuApi::SAMPSTATEPRESET_WrapPointNoMip, GpuApi::PixelShader );

			// Render fragments
			collector.RenderElementsAllPlanesBackFirst( RSG_TransparentBackground, rc, RECG_ALL );

			// Unbind scene depth texture
			GpuApi::BindTextures( PSSMP_SceneDepth, 1, nullptr, GpuApi::PixelShader );

			GpuApi::BindTextures( 4, 2, nullptr, GpuApi::PixelShader );
		}	

		// ------------------------------------------------------
		// Render background refractions (e.g. water and similar)
		// ------------------------------------------------------
		const Bool shouldRenderGlobalWater = collector.m_scene && collector.m_scene->ShouldRenderWater( info );
		if ( ( info.m_materialDebugMode == MDM_None || info.m_materialDebugMode == MDM_WaterMode ) && info.IsShowFlagOn( SHOW_Refraction ) && (collector.HasAnySortGroupElements( RSG_RefractiveBackground ) || collector.HasAnySortGroupElements( RSG_RefractiveBackgroundDepthWrite ) || shouldRenderGlobalWater ) )
		{
			PC_SCOPE_RENDER_LVL0( BackgroundRefraction );

			// Set draw context
			CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_RefractionBuffer );

			// Prepare feedback color buffer
			if ( msaaEnabled )
			{
				VERIFY( GetPostProcess()->ResolveMSAAColorSimple( info, surfaces, RTN_Color2 ) );
			}
			else
			{
				Rect rect( 0, info.m_width, 0, info.m_height );
				StretchRect( surfaces->GetRenderTargetTex( RTN_Color ), rect, surfaces->GetRenderTargetTex( RTN_Color2 ), rect );
			}

			// Setup rendertargets
			GpuApi::SetupRenderTargets( rtSetupMain );

			// Render without lighting
			rc.m_pass = RP_NoLighting;

			// Set wireframe
			GpuApi::SetRenderSettingsWireframe( targetWireframeSetting );

			// Bind scene color texture
			GpuApi::TextureRef sceneColor = surfaces->GetRenderTargetTex( RTN_Color2 );
			GpuApi::BindTextures( PSSMP_SceneColor, 1, &sceneColor, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( PSSMP_SceneColor, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

			// Bind scene depth texture
			GpuApi::TextureRef sceneDepth = surfaces->GetRenderTargetTex( RTN_GBuffer3Depth );
			GpuApi::BindTextures( PSSMP_SceneDepth, 1, &sceneDepth, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( PSSMP_SceneDepth, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );

			// Bind shadowmap, DX10+ supports depth stencil surfaces as shadowmaps
			// Bind poisson rotation

			GpuApi::TextureRef shadowRefs[2] = { GetGlobalCascadesShadowResources().GetDepthStencilArrayRead(), GpuApi::GetInternalTexture( GpuApi::INTERTEX_PoissonRotation) };
			GpuApi::BindTextures( 4, 2, &(shadowRefs[0]), GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 4, GpuApi::SAMPSTATEPRESET_ClampPointMip, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 5, GpuApi::SAMPSTATEPRESET_WrapPointNoMip, GpuApi::PixelShader );

			// Render fragments
			collector.RenderElementsAllPlanesBackFirst( RSG_RefractiveBackground, rc, RECG_ALL );

			GpuApi::SetupRenderTargets( rtSetupMain );

			// Bind scene depth texture once again
			GpuApi::BindTextures( PSSMP_SceneDepth, 1, &sceneDepth, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( PSSMP_SceneDepth, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );

			// render water proxy here
			GpuApi::BindTextures( PSSMP_SceneColor, 1, &sceneColor, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( PSSMP_SceneColor, GpuApi::SAMPSTATEPRESET_WrapLinearNoMip, GpuApi::PixelShader );


			SetupForSceneRender( info );


			// Render refractive background
			if ( collector.HasAnySortGroupElements( RSG_RefractiveBackgroundDepthWrite ) )
			{
				collector.RenderElementsAllPlanesBackFirst( RSG_RefractiveBackgroundDepthWrite, rc, RECG_ALL );
			}

			// Grab reflection data (before water, since it fucks up the depth buffer and we don't want it's color also)
			if ( isGrabLocalReflectionDataPending )
			{
				GrabLocalReflectionsData( collector );
				isGrabLocalReflectionDataPending = false;
			}

			// Render water
			if ( collector.m_scene && collector.m_scene->ShouldRenderWater( info ) )
			{
				PC_SCOPE_RENDER_LVL1( RenderWater );
				
				// fog is calculated per vertex
				BindSharedConstants( GpuApi::DomainShader );

				collector.m_scene->RenderWater( rc, info );
				// I am not unbinding this because GNM Api is broken - and fix is coming soon, right MG?
				//UnbindSharedConstants( GpuApi::DomainShader );
			}

			// Unbind scene depth texture
			GpuApi::BindTextures( PSSMP_SceneDepth, 1, nullptr, GpuApi::PixelShader );

			// Unbind scene color texture
			GpuApi::BindTextures( PSSMP_SceneColor, 1, nullptr, GpuApi::PixelShader );

			GpuApi::BindTextures( 4, 2, nullptr, GpuApi::PixelShader );

			// Restore main rendertargets
			GpuApi::SetupRenderTargets( rtSetupMain );
		}

		// -------------------------------------------
		// Grab reflection data if not already grabbed
		// -------------------------------------------
		if ( isGrabLocalReflectionDataPending )
		{
			GrabLocalReflectionsData( collector );
			isGrabLocalReflectionDataPending = false;
		}
	}
		
	// ----------------------
	// Render reflection mask
	// ----------------------
	if ( collector.HasAnyReflectiveMaskedElements() )
	{
		CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );
		
		// Setup targets
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, surfaces->GetLocalReflectionsMaskTex() );
		rtSetup.SetDepthStencilTarget( msaaEnabled ? surfaces->GetDepthBufferTexMSAA() : surfaces->GetDepthBufferTex() );
		rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetup );

		// Setup render context
		const ERenderingPass origRenderingPass = rc.m_pass;
		rc.m_pass = RP_ReflectionMask;

		// Set draw context
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_ReflectionMask );

		// Bind scene depth
		GpuApi::TextureRef sceneDepth = surfaces->GetRenderTargetTex( RTN_GBuffer3Depth );
		GpuApi::BindTextures( PSSMP_SceneDepth, 1, &sceneDepth, GpuApi::PixelShader );

		// Render elements
		collector.RenderReflectiveMaskedElements( rc );

		// Unbind scene depth
		GpuApi::BindTextures( PSSMP_SceneDepth, 1, nullptr, GpuApi::PixelShader );

		// Restore rendering context
		rc.m_pass = origRenderingPass;
	}

#ifndef NO_DEBUG_PAGES

	CRenderFrame* frame = collector.m_frame;

	// Draw the remaining compiling shaders
	{
		TDynArray< String > compiledMaterialNames;
		if ( info.IsShowFlagOn( SHOW_VisualDebug ) )
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > mutex( CRenderMaterial::st_recompileMutex );

			compiledMaterialNames.Reserve( CRenderMaterial::st_currentlyRecompilingMaterials.Size() );
			for ( Uint32 i = 0; i < CRenderMaterial::st_currentlyRecompilingMaterials.Size(); ++i )
			{
				compiledMaterialNames.PushBack( CRenderMaterial::st_currentlyRecompilingMaterials[i]->GetDisplayableName() );				
			}
		}

		if ( !compiledMaterialNames.Empty() )
		{
			CFont* font = resOnScreenTextFont.LoadAndGet< CFont >();
			Uint32 textYPos = 0;
			String text = TXT("");
			for ( Uint32 i = 0; i < compiledMaterialNames.Size(); ++i )
			{
				textYPos = i*-1;
				text = String::Printf( TXT("Compiling %u shaders: Compiling: %s"), compiledMaterialNames.Size(), compiledMaterialNames[i].AsChar() );
				frame->AddDebugScreenText( 2, collector.m_frame->GetFrameOverlayInfo().m_height - 2, text, textYPos, false, Color( 73, 255, 80 ), Color( 0, 0, 0, 0 ), font, true  );
			}
		}
	}

#endif

	// ----------------------------
	// Render debug unlit fragments	
	// ----------------------------
	{
		PC_SCOPE_RENDER_LVL1( DebugUnlit ); 

		// Set draw context
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_DebugUnlit );

		// Set wireframe
		GpuApi::SetRenderSettingsWireframe( targetWireframeSetting );

		// Render fragments
		rc.m_pass = RP_NoLighting;		
		collector.RenderDynamicFragments( RSG_DebugUnlit, rc );
	}

	// -------------------------
	// Process local reflections
	// -------------------------	
	if ( shouldRenderGlobalWater || collector.HasAnyReflectiveMaskedElements() )
	{
		ProcessLocalReflections( collector );
	}

	{
		PC_SCOPE_RENDER_LVL0( AllTransparent );

		Bool didDrawAccumulativeRefraction = false;

		// -----------------------------------------------------------
		// Update non sky flares
		// It's not along with sky flares so that we have depth layed 
		// down by the global water
		// -----------------------------------------------------------
		if ( !isTakingScreenShot )
		{
			if ( info.IsShowFlagOn( SHOW_Flares ) && ( collector.GetRenderFrameInfo().m_materialDebugMode == MDM_None ) )
			{	
				GpuApi::RenderTargetSetup rtDepthOnlySetup = rtSetupMain;
				rtDepthOnlySetup.SetNullColorTarget();
				GpuApi::SetupRenderTargets( rtDepthOnlySetup );

				PC_SCOPE_RENDER_LVL1( UpdateFlaresOcclusion_NonSky );
				collector.UpdateNonSkyFlaresOcclusion();

				GpuApi::SetupRenderTargets( rtSetupMain );
			}
		}

		// -----------------------------------------------------------
		// Resolve msaa depth
		// -----------------------------------------------------------
		if ( msaaEnabled )
		{
			VERIFY( GetPostProcess()->ResolveMSAADepth( info, surfaces ) );
		}

		// ---------------------------------------
		// Render hair fragments transparent parts
		// ---------------------------------------
		if ( collector.GetRenderFrameInfo().IsShowFlagOn( SHOW_HairAndFur ) && info.IsShowFlagOn( SHOW_ForwardPass ) )
		{
			PC_SCOPE_RENDER_LVL0( RenderHairTransparent );

			//GpuApi::SetVsWaveLimits( 0, Config::cvHairLateAllocVSLimit.Get() );

			GpuApi::SetupRenderTargets( rtSetupMain );

			CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_HairTransparency );
			rc.m_pass = RP_ForwardLightingTransparent;
			collector.RenderElementsAllPlanesFrontFirst( RSG_Hair, rc, RECG_ALL );

			//GpuApi::ResetVsWaveLimits();
		}

		// --------------------------------------------------------------------------------
		// Prepare feedback buffer for accumulative refraction and material's feedback data
		// --------------------------------------------------------------------------------
		{
			// TODO: do this only if we have either accumulative refraction or material with feedback data (there is no way of detecting that feedback data is used at this moment)
			Rect rect( 0, info.m_width, 0, info.m_height );
			StretchRect( surfaces->GetRenderTargetTex( RTN_Color ), rect, surfaces->GetRenderTargetTex( RTN_Color2 ), rect );			
		}

		// ------------------------------
		// Render accumulative refraction
		// ------------------------------
		if ( info.IsShowFlagOn( SHOW_Refraction ) && collector.HasAnyAccumulativeRefractionElements() && ( collector.GetRenderFrameInfo().m_materialDebugMode == MDM_None ) )
		{
			PC_SCOPE_RENDER_LVL0( AccumulativeRefraction );

			ERenderTargetName accumulationBuffer	= RTN_FinalColor;
			ERenderTargetName feedbackBuffer		= RTN_Color2;

			didDrawAccumulativeRefraction = true;

			// Set wireframe
			GpuApi::SetRenderSettingsWireframe( false );

			// Build accumulation buffer
			{
				// Render without lighting
				rc.m_pass = RP_RefractionDelta;

				// Bind accumulation buffer
				{
					GpuApi::RenderTargetSetup rtSetup;
					rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex(accumulationBuffer) );
					rtSetup.SetDepthStencilTarget( surfaces->GetDepthBufferTex() );
					rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
					GpuApi::SetupRenderTargets( rtSetup );
				}

				// Clear rendering targets
				ClearColorTarget( Vector::ZEROS );

				// Set draw context
				CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_RefractionAccumBuffer );

				// Bind scene depth texture
				GpuApi::TextureRef sceneDepth = surfaces->GetRenderTargetTex( RTN_GBuffer3Depth );
				GpuApi::BindTextures( PSSMP_SceneDepth, 1, &sceneDepth, GpuApi::PixelShader );
				GpuApi::SetSamplerStatePreset( PSSMP_SceneDepth, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );

				// Render elements
				collector.RenderAccumulativeRefractionElements( rc );

				// Unbind scene depth texture
				GpuApi::BindTextures( PSSMP_SceneDepth, 1, nullptr, GpuApi::PixelShader );

				// Don't restore render target setup yet, as the following steps may modify the setup.
			}			

			// Setup main rendertarget
			GpuApi::SetupRenderTargets( rtSetupMain );

			// Apply refraction
			{
				// Bind shader
				CRenderShaderPair *shader = msaaEnabled ? m_blitAccumRefraction_MSAA : m_blitAccumRefraction;
				shader->Bind();

				// Set parameters
				Vector refractionScale	= Vector ( info.m_width/(Float)surfaces->GetWidth(), info.m_height/(Float)surfaces->GetHeight(), 0.f, 0.f );
				Vector clampValue		= Vector ( 0.f, 0.f, info.m_width/(Float)surfaces->GetWidth(), info.m_height/(Float)surfaces->GetHeight() );
				m_stateManager->SetPixelConst( PSC_Custom_0 + 0, refractionScale );
				m_stateManager->SetPixelConst( PSC_Custom_0 + 1, clampValue );

				// Bind textures

				GpuApi::TextureRef refractionBuffers[] = { surfaces->GetRenderTargetTex( feedbackBuffer ), surfaces->GetRenderTargetTex( accumulationBuffer ) };
				GpuApi::BindTextures( 0, 2, &(refractionBuffers[0]), GpuApi::PixelShader );
				GpuApi::SetSamplerStateCommon( 0, 2, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );

				// Setup draw context
				CGpuApiScopedDrawContext drawContext ( msaaEnabled ? GpuApi::DRAWCONTEXT_PostProcBlend : GpuApi::DRAWCONTEXT_PostProcSet );

				// Blit
				GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 0), Vector (1, 1, 0, 0), 0.5f );

				// Unbind textures
				GpuApi::BindTextures( 0, 2, nullptr, GpuApi::PixelShader );
			}
		}
		else
		{
			// feedback buffer may have changed the render target setup, so restore.
			GpuApi::SetupRenderTargets( rtSetupMain );
		}

		// ---------------------------
		// Render 'scene' transparency
		// ---------------------------
		if ( collector.GetRenderFrameInfo().m_materialDebugMode == MDM_None )
		{
			PC_SCOPE_RENDER_LVL1( RenderTransparentScene );

			CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );
			ASSERT( !rtSetupMain.depthTargetReadOnly, TXT("Grab not needed") );
			if ( !rtSetupMain.depthTargetReadOnly )
			{
				GpuApi::RenderTargetSetup rtSetup = rtSetupMain;
				rtSetup.SetDepthStencilTarget( rtSetup.depthTarget, (Int16)rtSetup.depthTargetSlice, true );
				GpuApi::SetupRenderTargets( rtSetup );
			}

			// Set draw context
			CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_Transparency );

			// Render without lighting
			rc.m_pass = RP_NoLighting;		

			// Set wireframe
			GpuApi::SetRenderSettingsWireframe( targetWireframeSetting );

			GpuApi::TextureRef sceneColor = surfaces->GetRenderTargetTex( RTN_Color2 );
			GpuApi::BindTextures( PSSMP_SceneColor, 1, &sceneColor, GpuApi::PixelShader );

			GpuApi::TextureRef sceneDepth = surfaces->GetDepthBufferTex();
			GpuApi::BindTextures( PSSMP_SceneDepth, 1, &sceneDepth, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( PSSMP_SceneDepth, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );

			// Render fragments
			COMPILE_ASSERT( 2 == RPl_Max );
			ERenderingSortGroup groups[] = { RSG_Transparent, RSG_TransparentFullres };
			collector.RenderElements( ARRAY_COUNT(groups), groups, rc, RPl_Scene, RECG_ALL );

			// Unbind scene depth texture
			GpuApi::BindTextures( PSSMP_SceneDepth, 1, nullptr, GpuApi::PixelShader );
			GpuApi::BindTextures( PSSMP_SceneColor, 1, nullptr, GpuApi::PixelShader );
		}

		// --------------
		// Render sprites
		// --------------
		{
			PC_SCOPE_RENDER_LVL1( RenderSprites );

			// Set draw context
			CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_Sprites );

			// Set wireframe
			GpuApi::SetRenderSettingsWireframe( false );

			// Render unlit fragments
			rc.m_pass = RP_NoLighting;
			collector.RenderDynamicFragments( RSG_Sprites, rc );
		}

		// -----------------------------------------------------------
		// Draw debug flares occlusion shapes
		// -----------------------------------------------------------
		if ( info.IsShowFlagOn( SHOW_FlareOcclusionShapes ) && ( collector.GetRenderFrameInfo().m_materialDebugMode == MDM_None ) )
		{
			GpuApi::SetupRenderTargets( rtSetupMain );

			PC_SCOPE_RENDER_LVL1( DrawFlaresDebugOcclusionShapes );
			collector.DrawFlaresDebugOcclusionShapes();
		}

		// -----------------------------------------------------------
		// Render flares
		// -----------------------------------------------------------
		if ( info.IsShowFlagOn( SHOW_Flares ) )
		{
			PC_SCOPE_RENDER_LVL1( RenderFlares );

			GpuApi::SetRenderSettingsWireframe( false );
			rc.m_pass = RP_NoLighting;
			
			collector.RenderFlares( rc, true, false );
		}
	}

#ifndef RED_FINAL_BUILD
	UpdateRenderingStats( collector );
#endif

	GetRenderer()->UnbindForwardConsts( collector.GetRenderFrameInfo(), GpuApi::PixelShader );
	GetRenderer()->UnbindForwardConsts( collector.GetRenderFrameInfo(), GpuApi::VertexShader );
	// ----------------------------------
	// Restore original settings
	// ----------------------------------

	GpuApi::SetRenderSettingsWireframe( origWireframeSetting );
	GpuApi::SetReversedProjectionState( origReversedProjection );
	
	// ----------------------------------
	// Unbind constants
	// ----------------------------------

	UnbindSharedConstants( GpuApi::PixelShader );
}

void CRenderInterface::RenderDebugOverlay( CRenderCollector& collector, CRenderFrame* frame, class CRenderSurfaces* surfaces, class CRenderViewport* viewport )
{
	PC_SCOPE_RENDER_LVL1( RenderDebugOverlay );

	const CRenderFrameInfo& info = frame->GetFrameInfo();

	{
		// Bind target
		{
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_FinalColor ) );
			rtSetup.SetDepthStencilTarget( surfaces->GetDepthBufferTex() );
			rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );
		}

		RenderDebug(collector);
	}

	// Setup global camera parameters
	m_stateManager->SetLocalToWorld( NULL );
	m_stateManager->SetCamera2D();
}

void CRenderInterface::RenderHitProxyFrame( CRenderCollector& collector )
{
	const CRenderFrameInfo& info = collector.m_frame->GetFrameInfo();
	ASSERT( info.m_renderingMode == RM_HitProxies );

	// Setup global camera parameters
	m_stateManager->SetLocalToWorld( NULL );
	m_stateManager->SetCamera( info.m_camera );

	// Setup camera reversed projection
	const Bool origReversedProjection = GpuApi::IsReversedProjectionState();
	GpuApi::SetReversedProjectionState( info.m_camera.IsReversedProjection() );
	ASSERT( !origReversedProjection );

	// Hit proxy ID rendering requires screen cleared to black
	ClearColorTarget( Vector::ZEROS );
	ClearDepthStencilTarget( GpuApi::GetClearDepthValueRevProjAware(), 0 );

	// Setup render context
	RenderingContext rc( info.m_camera );
	rc.m_pass = RP_HitProxies;

	// Setup wireframe state
	GpuApi::SetRenderSettingsWireframe( false );
	
	// Common render
	{	
		// Set draw context
		CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_HitProxiesSolid );

		// Render
		ERenderingSortGroup groups[] = { RSG_LitOpaque, RSG_LitOpaqueWithEmissive, RSG_RefractiveBackgroundDepthWrite, RSG_Skin, RSG_Unlit, RSG_DebugUnlit, RSG_DecalModulativeColor, RSG_DecalBlendedColor, RSG_DecalBlendedNormalsColor, RSG_DecalBlendedNormals, RSG_Forward, RSG_Hair, RSG_EyeOverlay };
		collector.RenderElementsAllPlanesFrontFirst( ARRAY_COUNT(groups), groups, rc, RECG_ALL );
		collector.RenderDynamicFragments( ARRAY_COUNT(groups), groups, rc );
	}

	// Sprites
	{	
		// Set draw context
		CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_HitProxiesSprite );

		// Render
		collector.RenderDynamicFragments( RSG_Sprites, rc );
	}

	// Overlay
	if ( collector.m_frame->HasFragments( RSG_DebugOverlay ) )
	{
		// Clear Z buffer
		ClearDepthStencilTarget( GpuApi::GetClearDepthValueRevProjAware(), 0 );

		// Set draw context
		CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_HitProxiesSolid );

		// Render unlit fragments
		collector.RenderDynamicFragments( RSG_DebugOverlay, rc );
	}

	// Restore reversed projection
	GpuApi::SetReversedProjectionState( origReversedProjection );
}

void CRenderInterface::RenderThumbnailScreenshot( const CRenderFrameInfo& info )
{
	PC_SCOPE_RENDER_LVL1( ScreenshotStretchRect );

	GpuApi::TextureRef finalColor = GetSurfaces()->GetRenderTargetTex( RTN_FinalColor );

	// Create scaled target texture if we haven't already.
	if( !m_backBufferRescaled )
	{
		// get backbuffer params for screenshots. Same format etc as the rendered scene, but with screenshot size.
		GpuApi::TextureDesc desc;
		GpuApi::GetTextureDesc( finalColor, desc );
		desc.width = SAVE_SCREENSHOT_WIDTH;
		desc.height = SAVE_SCREENSHOT_HEIGHT;
#ifdef RED_PLATFORM_ORBIS
		desc.usage = GpuApi::TEXUSAGE_Staging;
#else
		desc.usage = GpuApi::TEXUSAGE_RenderTarget;
#endif
		m_backBufferRescaled = CreateTexture( desc, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_backBufferRescaled, "screenshotRescaled" );
	}

	if( !m_postProcess->IsBlackscreen() )
	{
		// copy the rescaled texture into a staging one
		// 1st step, render the texture into a smaller rendertarget
		GetRenderer()->StretchRect( finalColor, Rect( 0, info.m_width, 0, info.m_height ),
			m_backBufferRescaled, Rect( 0, SAVE_SCREENSHOT_WIDTH, 0, SAVE_SCREENSHOT_HEIGHT ) );
	}
	else
	{
		GetRenderer()->ClearColorTarget( m_backBufferRescaled, Vector::ZEROS );
	}
}

void CRenderInterface::RenderDebug( CRenderCollector& collector )
{	
	PC_SCOPE_RENDER_LVL1( RenderDebug );

	const CRenderFrameInfo& info = collector.m_frame->GetFrameInfo();
	const Bool hitProxies = info.m_renderingMode == RM_HitProxies;

	// Setup global camera parameters
	m_stateManager->SetLocalToWorld( NULL );
	m_stateManager->SetCamera( info.m_camera );

	// Setup camera reversed projection
	const Bool origReversedProjection = GpuApi::IsReversedProjectionState();
	GpuApi::SetReversedProjectionState( info.m_camera.IsReversedProjection() );
	ASSERT( !origReversedProjection );

	// Setup render context
	RenderingContext rc( info.m_camera );
	rc.m_pass = hitProxies ? RP_HitProxies : RP_NoLighting;

	// Render debug transparent	
	if ( !hitProxies && collector.m_frame->HasFragments( RSG_DebugTransparent ) )
	{
		// Set draw context
		CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_DebugTransparent );
		
		// Render unlit fragments
		const CGpuApiScopedTwoSidedRender scopedForcedTwoSided( false );
		collector.RenderDynamicFragments( RSG_DebugTransparent, rc );
	}

	// Setup global camera parameters
	m_stateManager->SetLocalToWorld( NULL );
	m_stateManager->SetCamera( info.m_camera );

	// Render overlay fragments ( editor mostly )
	if ( !hitProxies && collector.m_frame->HasFragments( RSG_DebugOverlay ) )
	{
		// Clear Z buffer
		ClearDepthStencilTarget( GpuApi::GetClearDepthValueRevProjAware(), 0 );

		// Set draw context
		CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_DebugOverlay );
		
		// Render unlit fragments
		collector.RenderDynamicFragments( RSG_DebugOverlay, rc );
	}

	// Restore reversed projection
	GpuApi::SetReversedProjectionState( origReversedProjection );
}

// With v-sync, keep SF from taking frames meant for the next interval
// and causing out of control stuttering. Hack, but jfc CRI video.
#if defined( USE_SCALEFORM ) 
# if WITH_SCALEFORM_VIDEO
#  define USE_VIDEO_RENDER_TICK
# endif
#endif

#ifdef USE_VIDEO_RENDER_TICK
namespace Scaleform { namespace GFx { namespace  Video {
extern Uint64 GRenderTick;
} } }
static void VideoRenderTick()
{
	++GFx::Video::GRenderTick;
}
#endif // USE_VIDEO_RENDER_TICK

void CRenderInterface::Render2D( CRenderCollector& collector, Bool supressSceneRendering )
{
	PC_SCOPE_RENDER_LVL1( Render2D );

	const CRenderFrameInfo& info = collector.m_frame->GetFrameInfo();
	CRenderThread* thread = static_cast< CRenderThread* >( GetRenderer()->GetRenderThread() );

	// Stop the game world from showing when switching fullscreen panels
	if (supressSceneRendering)
	{
		ClearColorTarget( Vector( 0, 0, 0, 1 ) );
	}

	//dex++: Draw texture preview
	{
		RenderTexturePreview();
	}
	//dex--

	//dex++: Draw shadowmap preview
#ifndef NO_DEBUG_PAGES
	if ( GGame != nullptr && (GGame->GetActiveWorld() != nullptr) && !IDebugPageManagerBase::GetInstance()->IsDebugPageActive() )
	{
		const Uint32 screenW = collector.GetRenderFrameOverlayInfo().m_width;
		const Uint32 screenH = collector.GetRenderFrameOverlayInfo().m_height;

		if ( collector.GetRenderFrameInfo().IsShowFlagOn( SHOW_ShadowStats ) )
		{
			ASSERT( collector.m_cascades.m_numCascades > 0 );

			const Uint32 size = ( screenH - 70) / Max<Uint32>( 1, collector.m_cascades.m_numCascades );

			Uint32 y = 50;
			Uint32 x = collector.GetRenderFrameOverlayInfo().m_width - size - 20;

			for ( Uint32 i=0; i<collector.m_cascades.m_numCascades; ++i )
			{
				m_debugDrawer->DrawTexturePreviewTile( 
					(Float)x, (Float)y, (Float)size, (Float)size,
					m_cascadeShadowResources.GetDepthStencilArrayRead(),
					0, i, 0.0f, 1.0f, Vector::ONES );

				y += size + 4;
			}

			// hi-res shadowmap/terrain shadowmap
			if ( collector.m_scene )
			{
				m_debugDrawer->DrawTexturePreviewTile( 
					(Float)x - 266, (Float)50, 256, 256,
					collector.m_scene->GetTerrainShadows()->GetShadowDepthBuffer(),
					0, 0, 0.0f, 1.0f, Vector::ONES );
			}
		}

		if( GetShadowManager() )
		{
			Int32 xOff = 10;
			if( collector.GetRenderFrameInfo().IsShowFlagOn( SHOW_ShadowPreviewDynamic ) )
			{
				// Render dynamic light atlases
				for( Uint32 i = 0; i < CRenderShadowManager::NUM_DYNAMIC_SLICES; ++i )
				{
					m_debugDrawer->DrawTexturePreviewTile( (Float)xOff, (Float)screenH - 266, 256, 256,
						GetShadowManager()->GetDynamicAtlas(),
						0, i, 0.0f, 1.0f, Vector::ONES );
					xOff += 266;
				}
			}
			if( collector.GetRenderFrameInfo().IsShowFlagOn( SHOW_ShadowPreviewStatic ) )
			{
				//--- helper strings >
				static const Char* axis[6] = {L"+X",L"-X",L"+Y",L"-Y",L"+Z",L"-Z"};
				static const Char* number[] = {L"0",L"1",L"2",L"3",L"4",L"5",L"6",L"7",L"8",L"9",L"10",L"11",L"12",L"13",L"14",L"15",L"16",L"17",L"18",L"19"}; // Ugly, but faster that sprintf on runtime
				static_assert( ARRAY_COUNT(number) >= CRenderShadowManager::NUM_STATIC_CUBES , "Change array above by adding new entries" );
				//--- < helper strings

				for( Uint32 i = 0; i < 6; ++i )
				{
					collector.m_frame->AddDebugScreenFormatedText( xOff, (Float)screenH - 32 - i * 65, Color( 255,255,255,255 ), axis[i] );
				}
				xOff += 24;

				const GpuApi::TextureDesc desc = GpuApi::GetTextureDesc( GetShadowManager()->GetStaticAtlas() );

				// Render all static cubes
				for( Uint32 i = 0; i < CRenderShadowManager::NUM_STATIC_CUBES; ++i )
				{
					collector.m_frame->AddDebugScreenFormatedText( xOff + 20, (Float)screenH - 12 - 6 * 65, Color( 255,255,255,255 ), number[i] );
					// Each face
					for( Uint32 j = 0; j < 6; ++j )
					{
						m_debugDrawer->DrawTexturePreviewTile( xOff, (Float)screenH - 64 - j * 65, 64, 64,
							GetShadowManager()->GetStaticAtlas(),
							0, GpuApi::CalculateCubemapSliceIndex( desc, i , j, 0 ) , 0.0f, 1.0f, Vector::ONES );
					}
					xOff += 65;
				}
			}
		}

	}
#endif
	
	// Draw GUI
#ifdef USE_VIDEO_RENDER_TICK
	VideoRenderTick();
#endif

#ifdef USE_SCALEFORM
	Bool canRenderScaleform = false;
	CRenderScaleform* renderScaleform = nullptr;

	{
		PC_SCOPE_RENDER_LVL1( Render Scaleform UI );
		const GpuApi::RenderTargetSetup& rtSetup = GpuApi::GetRenderTargetSetup();

		// Setup global camera parameters
		m_stateManager->SetLocalToWorld( NULL );
		m_stateManager->SetCamera2D();

		// Set draw context
		CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_Scaleform2D );

		// FIXME/HACK: let's intialize Scaleform with some sensible values instead of our 1x1 right at startup
		// which seems to be used for some cache values.
		const Bool supportedDimensions = rtSetup.viewport.height > 1 && rtSetup.viewport.width > 1;
		RED_ASSERT( supportedDimensions, TXT("Maybe we could have a non zero viewport?") );

		// Delayed initialization because when you initialize Scaleform's HAL, it blindly assumes
		// you're basically ready to render.
		renderScaleform = static_cast< CRenderScaleform* >( GRender->GetRenderScaleform() );
		if ( renderScaleform && renderScaleform->GetRenderState() == CRenderScaleform::RS_Uninitialized )
		{
			RED_VERIFY( renderScaleform->Init() );
		}
		else if ( renderScaleform && renderScaleform->GetRenderState() == CRenderScaleform::RS_DeviceLost )
		{
			renderScaleform->HandleDeviceReset();
			RED_ASSERT( renderScaleform->GetRenderState() == CRenderScaleform::RS_Ready );
		}

		canRenderScaleform = renderScaleform ? renderScaleform->GetRenderState() == CRenderScaleform::RS_Ready : false;
		if ( canRenderScaleform )
		{
			renderScaleform->BeginFrame();
		}

		// Render GUI
		if ( canRenderScaleform && ( collector.GetRenderFrameInfo().m_drawHUD ) )
		{
			PC_SCOPE_RENDER_LVL2( Render Scaleform GUI );

			renderScaleform->Render( collector.m_frame );
			// Scaleform changes the viewport for mask clipping. Restore it. NOTE: Or just reset the whole RT/DSS/VP shebang.
			GpuApi::SetViewportRaw( rtSetup.viewport );

			// Restore internal state cache.
			// Necessary since SF doesn't use this state manager to change render state.
			m_stateManager->Reset();

			// TODO: Needed?
			// Bind best fit normal texture again
			BindBestFitNormalsTexture( info );	
		}
	}
#endif

	// Render blackscreen quad
	// W3 HACK: Don't show if using the hacky videobackground color instead for now.
	if ( !GRenderThread->GetVideoPlayer() || !GRenderThread->GetVideoPlayer()->IsShowingBackground() )
	{
		PC_SCOPE_RENDER_LVL1( Render Blackscreen );
		// Setup global camera parameters
		m_stateManager->SetLocalToWorld( NULL );
		m_stateManager->SetCamera2D();

		// Draw full screen fade
		m_postProcess->DrawFade( collector.GetRenderFrameInfo() );
	}

	{
		PC_SCOPE_RENDER_LVL1( Render Flash Overlay );

		const GpuApi::RenderTargetSetup& rtSetup = GpuApi::GetRenderTargetSetup();

		// Setup global camera parameters
		m_stateManager->SetLocalToWorld( NULL );
		m_stateManager->SetCamera2D();

		// Set draw context
		CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_Scaleform2D );

#ifdef USE_SCALEFORM
		if ( canRenderScaleform )
		{
			renderScaleform->RenderOverlay( collector.m_frame );
			// Scaleform changes the viewport for mask clipping. Restore it. NOTE: Or just reset the whole RT/DSS/VP shebang.
			GpuApi::SetViewportRaw( rtSetup.viewport );

			// Restore internal state cache.
			// Necessary since SF doesn't use this state manager to change render state.
			m_stateManager->Reset();

			// TODO: Needed?
			// Bind best fit normal texture again
			BindBestFitNormalsTexture( info );	
		}
#endif
	}

	if ( canRenderScaleform )
	{
		renderScaleform->EndFrame();
	}

#if defined(USE_SCALEFORM) && !defined(NO_DEBUG_PAGES) && !defined(RED_FINAL_BUILD)
	// Display the memory of UI, when UI is active
	if( info.IsShowFlagOn( SHOW_ScaleformMemoryInfo ) )
	{
		const Float oneMB = 1024*1024;
		const Float invOneMB = 1.0f/oneMB;

		CRenderFrame* frame = collector.m_frame;

		CRenderScaleformTextureManager* textureManager = static_cast< CRenderScaleformTextureManager* >( GetRenderer()->GetRenderScaleform()->GetTextureManager() );

		Float scaleformTime = textureManager->GetCurrentTime();
		Float currentTime = frame->GetFrameInfo().m_engineTime;
		if( scaleformTime > currentTime - 2.0f )
		{
			Int32 x = 50;
			Int32 height = frame->GetFrameOverlayInfo().m_height - 150;

			CScaleformTextureCacheQueue* queue = textureManager->GetStreamingQueue();

			if( queue )
			{
				String memorySummary = String::Printf( TXT( "[UI] Streaming memory: %.2fmb | peek: %.2fmb" ), queue->GetCurrentMemoryUsage() * invOneMB , queue->GetPeakMemoryUsage() * invOneMB );

				const Float colorLerp = ::Clamp( ( queue->GetCurrentMemoryUsage() - 4*oneMB ) / (10*oneMB) , 0.0f, 1.0f );
				const Color color = Color::Lerp( colorLerp , Color::LIGHT_GREEN , Color::RED );

				frame->AddDebugScreenText( x, height, memorySummary, color );
			}

			{
				String memorySummary = String::Printf( TXT( "Other memory: %.2fmb | peek: %.2fmb" ), textureManager->GetCurrentMemoryUsage() * invOneMB , textureManager->GetPeakMemoryUsage() * invOneMB );

				const Float colorLerp = ::Clamp( ( textureManager->GetCurrentMemoryUsage() - 8*oneMB ) / (24*oneMB) , 0.0f, 1.0f );
				const Color color = Color::Lerp( colorLerp , Color::LIGHT_GREEN , Color::RED );

				frame->AddDebugScreenText( x + 350 , height, memorySummary, color );
			}

		}

	}
#endif // USE_SCALEFORM

	// Render 2D screen fragments
	{
		PC_SCOPE_RENDER_LVL1( Render Debug 2D fragments );

		// Set draw context
		CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_2D );

		// Setup global camera parameters
		m_stateManager->SetLocalToWorld( NULL );
		m_stateManager->SetCamera2D();

		// Setup render context
		CRenderCamera default2DCamera;
		RenderingContext rc( default2DCamera );
		rc.m_pass = RP_NoLighting;

		// Render
		collector.RenderDynamicFragments( RSG_2D, rc );
	}

	// Setup global camera parameters
	m_stateManager->SetLocalToWorld( NULL );
	m_stateManager->SetCamera2D();
}

#ifdef RED_PLATFORM_ORBIS
void* LoadShaderBinaryFromFile( const Char* path, Uint32& codeSize )
{
	Red::IO::CNativeFileHandle file;

	if ( !file.Open( path, Red::IO::eOpenFlag_Read ) )
	{
		RED_HALT( "Missing shader file: '%ls'", path );
		return nullptr;
	}

	const Uint64 fileSize = file.GetFileSize();
	void* code = GPU_API_ALLOCATE( GpuApi::GpuMemoryPool_Shaders, GpuApi::MC_Temporary, fileSize, 16 );
	Uint32 readBytes = 0;
	file.Read( code, fileSize, readBytes );
	codeSize = readBytes;

	return code;
}
#endif

void CRenderInterface::NewFrame()
{
	// Allow batchers to prepare for next frame
	if ( m_particleBatcher )
	{
		m_particleBatcher->OnNewFrame();
	}

	// Buffer current mesh streaming stats
	m_meshStreamingStats.NextFrame();

	// Let particle data buffers know about new frame
	IParticleData::OnNewFrame();
}


void CRenderInterface::EndFrame()
{
	if ( m_particleBatcher )
	{
		m_particleBatcher->RelinkProxies();
	}
}


void CRenderInterface::RenderFinal2D( CRenderFrame* frame, CRenderFrameInfo& info , Bool supressSceneRendering, Bool dynamicRescalingAllowedByLoadingBlur )
{
	RED_ASSERT( &frame->GetFrameInfo() == &info, TXT("CRenderFrameInfo mismatch") );

	// Early exits conditions
	if( info.m_renderingMode == RM_HitProxies ) 
	{
		return;
	}

	CRenderViewport* viewport = static_cast< CRenderViewport* >( info.m_viewport );	

	if( !supressSceneRendering && !( viewport && info.m_allowPostSceneRender ) )
	{
		return;
	}

	// Get rendering surfaces to use
	CRenderSurfaces* surfaces = GetSurfaces();
	ASSERT( surfaces );

	// If we need to rebind render targets just before futher rendering ?
	Bool rebindRenderTargets = false;

#ifdef RED_PLATFORM_DURANGO

	// If we suppress the scene rendering the main backbuffer might end up not being cleared, then rendering the UI into the overlay backbuffer will also not clear this one and we end up with some junk from the memory
	if (supressSceneRendering)
	{
		// Setup the render targets to the overlay swap chain
		{
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, GpuApi::GetBackBufferTexture() );
			rtSetup.SetViewportFromTarget( GpuApi::GetBackBufferTexture() );
			rtSetup.SetDepthStencilTarget( GpuApi::GetDepthStencilTexture() );
			GpuApi::SetupRenderTargets( rtSetup );
		}

		ClearColorTarget( Vector( 0, 0, 0, 1 ) );
	}
#endif

	// Set the viewport back
	{
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_FinalColor ) );
		rtSetup.depthTarget = surfaces->GetDepthBufferTex();
		rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetup );
	}

#if defined(RED_PLATFORM_WINPC)
	// Render cachets
	if( m_renderCachets && !GIsEditor )
	{
		CRenderViewport* viewport = static_cast< CRenderViewport* >( info.m_viewport );	
		Int32 cachetWidth = (viewport->GetFullWidth() - viewport->GetHorizontalCachet()) / 2;
		if( cachetWidth > 0.0f )	
		{
			GpuApi::SetCustomDrawContext( GpuApi::DSSM_NoStencilNoDepth, GpuApi::RASTERIZERMODE_DefaultNoCull, GpuApi::BLENDMODE_Set );

			// Setup global camera parameters
			m_stateManager->SetLocalToWorld( NULL );
			m_stateManager->SetCamera2D();

			GetRenderer()->m_shaderSingleColor->Bind();
			GetRenderer()->GetStateManager().SetPixelConst( PSC_ConstColor, Vector ( 0.f, 0.f, 0.f, 1.f ) );

			GetDebugDrawer().DrawQuad( Vector( 0.f, 0.0f, 0.f, 1.f ), Vector( cachetWidth, viewport->GetFullHeight(), 0.f, 1.f ), 0.5f );
			GetDebugDrawer().DrawQuad( Vector( viewport->GetFullWidth() - cachetWidth, 0.0f, 0.f, 1.f ), Vector( viewport->GetFullWidth(), viewport->GetFullHeight(), 0.f, 1.f ), 0.5f );
		}
	}
#endif

	if( supressSceneRendering )
	{
		if ( info.m_isLastFrameForced )
		{
			PC_SCOPE_RENDER_LVL1( RenderPostProcesses );

			// Draw post-process
			const EPostProcessCategoryFlags postProcess = m_collector.m_postProcess;

			// Draw final post processes
			GetPostProcess()->DrawFrozenFrame( m_collector, info, surfaces, viewport, postProcess );

			// Setup global camera parameters
			m_stateManager->SetLocalToWorld( NULL );
			m_stateManager->SetCamera2D();
		}
		else
		{
			// Clear, just in case
			ClearColorTarget( Vector( 0, 0, 0, 1 ) );
			ClearDepthStencilTarget( GpuApi::GetClearDepthValueRevProjAware(), 0 );
		}

		RenderDebug( m_collector );

	}

	if( GRenderThread->GetLoadingScreenBlur()->IsDrawable() )
	{
		RED_FATAL_ASSERT( !dynamicRescalingAllowedByLoadingBlur, "Sanity check. If this condition is not met, then the dynamic rescaling may corrupt some buffers used for dynamic rescaling. This shouldn't be the case since the only place we're changing the state of loading blur are rendercommands" );
		GRenderThread->GetLoadingScreenBlur()->DrawFade( *GetPostProcess()->m_drawer , info , GetLastTickDelta() , RTN_FinalColor );
		rebindRenderTargets = true;
	}
	
	if ( info.m_multiplanePresent )
	{
		CRenderViewport* viewport = static_cast< CRenderViewport* >( info.m_viewport );	

		// viewport can be NULL - e.g. generating thumbnails
		if ( viewport )
		{
			// Prepare viewport - if device lost, do not render
			if ( !viewport->PrepareViewportOverlay() || IsDeviceLost() )
			{
				return;
			}
		}

		// Setup the render targets to the overlay swap chain
		{
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, GpuApi::GetBackBufferTexture() );
			rtSetup.SetViewportFromTarget( GpuApi::GetBackBufferTexture() );
			rtSetup.SetDepthStencilTarget( GpuApi::GetDepthStencilTexture() );
			GpuApi::SetupRenderTargets( rtSetup );
		}

		{
			ClearColorTarget( Vector( 0, 0, 0, 0 ) );
		}
	}
	else if( rebindRenderTargets )
	{
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_FinalColor ) );
		rtSetup.depthTarget = surfaces->GetDepthBufferTex();
		rtSetup.SetViewport( frame->GetFrameInfo().m_width, frame->GetFrameInfo().m_height, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetup );
	}

	// Render 2D
	Render2D( m_collector, viewport ? viewport->IsSceneRenderingSuppressed() : false );
}

void CRenderInterface::InitShadowmapResources()
{
	// Init shadowmap
	{
		const Uint32 resolution = Config::cvCascadeShadowmapSize.Get();
		Uint16 maxCascades = Config::cvMaxCascadeCount.Get();
#ifdef RED_PLATFORM_CONSOLE
		//HACK moradin people keep changing this to 4 so I have to introduce a hack to make sure that noone will change this in the last moment
		if (maxCascades > 3)
		{
			maxCascades = 3;
		}
#endif
		m_cascadeShadowResources.Init( maxCascades, resolution, "CascadeShadowMap" );
	}

	// Create noise texture for poisson disk rotation
	{		
		const Uint32 SIZE = 32;

		GpuApi::TextureDesc desc;
		desc.type		= GpuApi::TEXTYPE_2D;	// array texture, we will render into slicces using VIEWs not the GS
		desc.width		= SIZE;
		desc.height		= SIZE;
		desc.sliceNum	= 1;
		desc.initLevels = 1;
		desc.format		= GpuApi::TEXFMT_R8G8B8A8;
		desc.usage		= GpuApi::TEXUSAGE_Samplable;

		Uint8 noiseData[SIZE*SIZE*4];
		Uint8 *writePtr = (Uint8*) noiseData;

		for ( Uint32 i=0; i<SIZE; ++i )
		{
			for ( Uint32 j=0; j<SIZE; ++j, writePtr += 4 )
			{
				const Float val1 = GetRenderer()->GetRandomNumberGenerator().Get< Float >();
				const Float val2 = GetRenderer()->GetRandomNumberGenerator().Get< Float >();
				const Float angle = 2.0f * M_PI * ( val1 );

				writePtr[0] = (Uint8)( Clamp< Float >( 127.5f + 127.5f*cosf(angle), 0.0f, 255.0f ) );
				writePtr[1] = (Uint8)( Clamp< Float >( 127.5f + 127.5f*sinf(angle), 0.0f, 255.0f ) );
				writePtr[2] = (Uint8)( 255.0f * val1 );
				writePtr[3] = (Uint8)( 255.0f * val2 );
			}
		}

		GpuApi::TextureLevelInitData levelInitData;
		levelInitData.m_data = noiseData;
		levelInitData.m_isCooked = false;

		GpuApi::TextureInitData initData;
		initData.m_isCooked = false;
		initData.m_mipsInitData = &levelInitData;

		m_shadowmapNoiseTexture = GpuApi::CreateTexture( desc, GpuApi::TEXG_System, &initData );
		ASSERT( m_shadowmapNoiseTexture );

		GpuApi::SetTextureDebugPath( m_shadowmapNoiseTexture, "shadowNoiseTexture" );
	}
}

void CRenderInterface::ReleaseShadowmapResources()
{
	m_cascadeShadowResources.Shut();
	GpuApi::SafeRelease( m_hiresEntityShadowmap );
	GpuApi::SafeRelease( m_shadowmapNoiseTexture );
}

GpuApi::eTextureSaveFormat Map( ESaveFormat saveFormat )
{
	switch ( saveFormat )
	{
		case SF_BMP:	return GpuApi::SAVE_FORMAT_BMP;
		case SF_DDS:	return GpuApi::SAVE_FORMAT_DDS;
		case SF_JPG:	return GpuApi::SAVE_FORMAT_JPG;
		case SF_PNG:	return GpuApi::SAVE_FORMAT_PNG;
	}

	ASSERT( false, TXT("Unknown format!") );
	return GpuApi::SAVE_FORMAT_BMP;
}

void CRenderInterface::TakeOneUberScreenshot( CRenderFrame* frame, CRenderSceneEx* scene, const SScreenshotParameters& screenshotParameters, Bool* status )
{
#ifndef RED_PLATFORM_CONSOLE
	// Prepare viewport for rendering
	CRenderFrameInfo& frameInfo = frame->GetFrameInfo();
	// override flags

	CRenderViewport* viewport = static_cast< CRenderViewport* >( frameInfo.m_viewport );
	CRenderCamera& camera = frameInfo.m_camera;

	const Float sceneFullWidth	= static_cast< Float >( viewport->GetRendererWidth() );
	const Float sceneFullHeight	= static_cast< Float >( viewport->GetRendererHeight() );

	Uint32 fullHDResX = screenshotParameters.m_width;
	Uint32 fullHDResY = screenshotParameters.m_height;

	Uint32 fullHDChunks = Max( static_cast<Uint32>( Red::Math::MCeil( screenshotParameters.m_width / sceneFullWidth ) ), static_cast<Uint32>( Red::Math::MCeil( screenshotParameters.m_height / sceneFullHeight ) ) );
	Uint32 fullHDChunkSizeX = fullHDResX / fullHDChunks;
	Uint32 fullHDChunkSizeY = fullHDResY / fullHDChunks;

	Uint32 oldViewportView[6]  = { viewport->GetRendererWidth() , viewport->GetRendererHeight() , viewport->GetWidth() , viewport->GetHeight() , viewport->GetFullWidth() , viewport->GetFullHeight() };
	Uint32 oldViewportFrame[2] = { frameInfo.m_width , frameInfo.m_height };

	viewport->SetCheatedSize( fullHDChunkSizeX, fullHDChunkSizeY, fullHDChunkSizeX, fullHDChunkSizeY, fullHDChunkSizeX, fullHDChunkSizeY );
	frameInfo.SetSize( fullHDChunkSizeX, fullHDChunkSizeY );

	camera.SetFOV( screenshotParameters.m_fov );

	const Uint32 numPassesX = Max< Uint32 >( 1, screenshotParameters.m_superSamplingSize );
	const Uint32 numPassesY = Max< Uint32 >( 1, screenshotParameters.m_superSamplingSize );

	m_lastTickDelta = 0.0f;

	GIsRendererTakingUberScreenshot = true;
	const GpuApi::RenderSettings prevRenderSettings = GpuApi::GetRenderSettings();
	{
		// Calculate mipmap bias
		static const Float log2f = logf( 2.0f );
		Float samplesPerPixel = static_cast< Float >( fullHDChunks * numPassesX );
		Float mipMapLODBias = -( logf( samplesPerPixel ) / log2f );		

		// Apply new gpuapi rendersettings
		GpuApi::RenderSettings newSettings = prevRenderSettings;
		newSettings.mipMapBias = mipMapLODBias;
		GpuApi::SetRenderSettings( newSettings );
	}

	Float subpixelOffsetDeltaX = 1.0f / ( (Float)fullHDChunks * (Float)numPassesX );
	Float subpixelOffsetDeltaY = 1.0f / ( (Float)fullHDChunks * (Float)numPassesY );
	for ( Uint32 x = 0; x < fullHDChunks * numPassesX; x++ )
	{
		Uint32 chunkNumX = x / numPassesX;

		for( Uint32 y = 0; y < fullHDChunks * numPassesY; y++ )
		{
			Uint32 chunkNumY = y / numPassesY;

			Float subPixelOffsetX = static_cast< Float >( x ) * subpixelOffsetDeltaX;
			Float subPixelOffsetY = static_cast< Float >( y ) * subpixelOffsetDeltaY;

			frameInfo.SetSubpixelOffset( subPixelOffsetX, subPixelOffsetY, fullHDChunkSizeX, fullHDChunkSizeY );

			// hack for dissolves 
			scene->m_frameCounter--;
			if ( m_particleBatcher )
				m_particleBatcher->OnNewFrameScreenshot();
			RenderFrame( frame, scene );

			GpuApi::Rect r;
			r.top = 0;
			r.left = 0;
			r.bottom = viewport->GetHeight();
			r.right = viewport->GetWidth();

			GpuApi::GrabBackBuffer( screenshotParameters.m_buffer, r, fullHDChunks, chunkNumX, chunkNumY, fullHDResX );
		}
	}

	GpuApi::SetRenderSettings( prevRenderSettings );

	*status = GpuApi::SaveBufferToFile( screenshotParameters.m_buffer, screenshotParameters.m_fileName.AsChar(), fullHDResX, fullHDResY, Map( screenshotParameters.m_saveFormat ), true, numPassesX * numPassesY );

	viewport->SetCheatedSize( oldViewportView[0], oldViewportView[1], oldViewportView[2], oldViewportView[3], oldViewportView[4], oldViewportView[5] );
	frameInfo.SetSize( oldViewportFrame[0], oldViewportFrame[1] );

	frameInfo.SetSubpixelOffset( 0.0f, 0.0f, 1, 1 );

	GIsRendererTakingUberScreenshot = false;
#else
	*status = false;
#endif
}

const Uint8 watermarkPosX = 178;
const Uint8 watermarkPosY = 96;

void SaveScreenshotToBuffer( GpuApi::TextureRef& stagingTexture, Uint32 width, Uint32 height, const SScreenshotParameters& parameters )
{
	if ( parameters.m_saveFormat == SF_PNG && GRender->IsBlackscreen() ) 
	{
		if ( parameters.m_bufferSizeWritten )
		{
			*parameters.m_bufferSizeWritten = 1; // this means there is no data at all, 0 means there is no data YET.
		}

		if ( parameters.m_completionFlag )
		{
			parameters.m_completionFlag->SetValue( true );
		}

		return; // don't do the screenshots of a blackscreen, it's pointless
	}

	Uint32 pitch;
	Uint8* lockedTexture = (Uint8*)GpuApi::LockLevel( stagingTexture, 0, 0, GpuApi::BLF_Read, pitch );

	if ( parameters.m_saveFormat == SF_BMP )
	{
		Uint32 filesize = 54 + 4*width*height;

		DataBuffer tempBuffer( DefaultDataBufferAllocator(), filesize );
		Uint8* target = (Uint8*)tempBuffer.GetData();

		unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
		unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 32,0};
		unsigned char bmppad[3] = {0,0,0};

		bmpfileheader[ 2] = (unsigned char)(filesize    );
		bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
		bmpfileheader[ 4] = (unsigned char)(filesize>>16);
		bmpfileheader[ 5] = (unsigned char)(filesize>>24);

		bmpinfoheader[ 4] = (unsigned char)( width);
		bmpinfoheader[ 5] = (unsigned char)( width>> 8);
		bmpinfoheader[ 6] = (unsigned char)( width>>16);
		bmpinfoheader[ 7] = (unsigned char)( width>>24);
		bmpinfoheader[ 8] = (unsigned char)( height);
		bmpinfoheader[ 9] = (unsigned char)( height>> 8);
		bmpinfoheader[10] = (unsigned char)( height>>16);
		bmpinfoheader[11] = (unsigned char)( height>>24);

		Red::System::MemoryCopy( target, bmpfileheader, 14 );
		Red::System::MemoryCopy( target+14, bmpinfoheader, 40 );

		target += 54;

		for ( Int32 j = (Int32)height-1; j >= 0; --j)
		{
			for ( Int32 i = 0; i < (Int32)width; ++i )
			{
				Int32 offset = j * pitch + i * 4;

				*target = lockedTexture[offset + 2];
				target++;
				*target = lockedTexture[offset + 1];
				target++;
				*target = lockedTexture[offset + 0];
				target++;
				*target = lockedTexture[offset + 3];
				target++;
			}
		}

		if ( parameters.m_bufferSizeWritten )
		{
			*parameters.m_bufferSizeWritten = filesize;
		}

		if ( !parameters.m_fileName.Empty() )
		{
			// Save to file
#ifndef RED_PLATFORM_CONSOLE
			IFile* writer = GFileManager->CreateFileWriter( parameters.m_fileName, FOF_AbsolutePath|FOF_Buffered );
			if ( writer )
			{
				writer->Serialize( tempBuffer.GetData(), tempBuffer.GetSize() );
				delete writer;
				writer = nullptr;
			}
#endif // RED_PLATFORM_CONSOLE
		}

		if ( parameters.m_buffer )
		{
			// write to external buffer
			RED_ASSERT( tempBuffer.GetSize() <= parameters.m_bufferSize, TXT("Data size mismatch") );
			Red::System::MemoryCopy( parameters.m_buffer, tempBuffer.GetData(), tempBuffer.GetSize() );
		}
	}
	else if ( parameters.m_saveFormat == SF_PNG )
	{
		const Uint32 numPixels = width * height;
		DataBuffer tempBuffer24( DefaultDataBufferAllocator(), numPixels * 3 );  
		Uint8* data = ( Uint8* ) tempBuffer24.GetData();

		// skip the alpha channel, skip the margin
		for ( Uint32 line = 0; line < height; ++line )
		{
			for ( Uint32 col = 0; col < width; ++col )
			{
				data[ 3 * ( line * width + col ) ]		= lockedTexture[ ( line * pitch ) + ( 4 * col ) ];
				data[ 3 * ( line * width + col ) + 1 ]	= lockedTexture[ ( line * pitch ) + ( 4 * col ) + 1 ];
				data[ 3 * ( line * width + col ) + 2 ]	= lockedTexture[ ( line * pitch ) + ( 4 * col ) + 2 ];
			}
		} 

		// paste the watermarks on the image
		Uint32 sourceRowSize;
		if( parameters.m_noWatermark == false )
		{
			sourceRowSize = watermarkWidth * 3;
			for ( Uint32 line = 0; line < watermarkHeight; ++line )
			{
				Uint32 destLine = watermarkPosY + line;
				Red::System::MemoryCopy( &data[ 3 * ( destLine * width + watermarkPosX ) ], &watermarkIcon[ line * sourceRowSize ], sourceRowSize );
			}
		}

		if ( parameters.ShouldPasteNgPlusWatermark() )
		{
			sourceRowSize = ngPlusWatermarkWidth * 3;
			for ( Uint32 line = 0; line < ngPlusWatermarkHeight; ++line )
			{
				Uint32 destLine = ngPlusWatermarkY + line;
				Red::System::MemoryCopy( &data[ 3 * ( destLine * width + ngPlusWatermarkX ) ], &ngPlusWatermark[ line * sourceRowSize ], sourceRowSize );
			}
		}

		Uint8* buffer = nullptr;
		size_t sizeWritten = 0;
		const Uint32 result = lodepng_encode24( &buffer, &sizeWritten, data, width, height );

		if ( result == 0 )
		{
			if ( parameters.m_buffer )
			{
				RED_ASSERT( sizeWritten <= parameters.m_bufferSize, TXT("Data size mismatch") );
				Red::System::MemoryCopy( parameters.m_buffer, buffer, sizeWritten );
			}

			if ( !parameters.m_fileName.Empty() )
			{
#ifndef RED_PLATFORM_CONSOLE
				if ( 0 != lodepng_save_file( buffer, sizeWritten, UNICODE_TO_ANSI( parameters.m_fileName.AsChar() ) ) )
				{
					RED_LOG_ERROR( Screenshots, TXT("Error saving screenshot file '%s'!" ), parameters.m_fileName.AsChar() );
				}
#endif // RED_PLATFORM_CONSOLE
			}

			if ( parameters.m_bufferSizeWritten )
			{
				*parameters.m_bufferSizeWritten = sizeWritten;
			}
		}
	}

	if ( parameters.m_completionFlag )
	{
		parameters.m_completionFlag->SetValue( true );
	}

	GpuApi::UnlockLevel( stagingTexture, 0, 0 );
}

void CRenderInterface::TakeOneRegularScreenshot( const SScreenshotParameters& parameters )
{
	RED_ASSERT( parameters.m_saveFormat == SF_BMP || parameters.m_saveFormat == SF_PNG, TXT("other texture formats not supported") );

	Uint32 width = parameters.m_width;
	Uint32 height = parameters.m_height;

#ifndef RED_PLATFORM_ORBIS
	// get backbuffer params for screenshots
	GpuApi::TextureDesc desc;
	GpuApi::GetTextureDesc( GetSurfaces()->GetRenderTargetTex(RTN_FinalColor), desc );
	desc.width = SAVE_SCREENSHOT_WIDTH;
	desc.height = SAVE_SCREENSHOT_HEIGHT;
	desc.usage = GpuApi::TEXUSAGE_Staging;
	GpuApi::TextureRef backBufferStagingCopy = CreateTexture( desc, GpuApi::TEXG_System );

	// 2nd step, copy the rescaled texture into a staging one
	GetRenderer()->StretchRect( m_backBufferRescaled , backBufferStagingCopy );

	SaveScreenshotToBuffer( backBufferStagingCopy, width, height, parameters );

	GpuApi::SafeRelease( backBufferStagingCopy );
#else
	SaveScreenshotToBuffer( m_backBufferRescaled, width, height, parameters );
#endif
}

void CRenderInterface::TakeOneRegularScreenshotNow(const SScreenshotParameters& screenshotParameters)
{
#ifdef RED_PLATFORM_CONSOLE
	RED_LOG_ERROR( Screenshots, TXT("Unsupported on consoles! For use in minimap generator & review system only." ) );
	return;
#endif

	GpuApi::TextureDesc desc;
	GpuApi::GetTextureDesc( GetSurfaces()->GetRenderTargetTex(RTN_FinalColor), desc );
	Uint32 width = Min( desc.width, screenshotParameters.m_width );
	Uint32 height = Min( desc.height, screenshotParameters.m_height );

	// get backbuffer params for screenshots
	desc.width = screenshotParameters.m_width;
	desc.height = screenshotParameters.m_height;
	desc.usage = GpuApi::TEXUSAGE_RenderTarget;
	GpuApi::TextureRef backBufferRescaled = CreateTexture( desc, GpuApi::TEXG_System );
	GpuApi::SetTextureDebugPath( backBufferRescaled, "screenshotRescaled" );
	desc.usage = GpuApi::TEXUSAGE_Staging;
	GpuApi::TextureRef backBufferStaging = CreateTexture( desc, GpuApi::TEXG_System );
	GpuApi::SetTextureDebugPath( backBufferStaging, "screenshotStaging" );

	// 1st step, render the texture into a smaller rendertarget
	GetRenderer()->StretchRect(
		GetSurfaces()->GetRenderTargetTex(RTN_FinalColor), Rect( 0, width, 0, height ),
		backBufferRescaled, Rect( 0, desc.width, 0, desc.height ) );

	// 2nd step, copy the rescaled texture into a staging one
	GetRenderer()->StretchRect( backBufferRescaled, backBufferStaging );

	SaveScreenshotToBuffer( backBufferStaging, width, height, screenshotParameters );

	GpuApi::SafeRelease( backBufferStaging );
	GpuApi::SafeRelease( backBufferRescaled );
}
#if MICROSOFT_ATG_DYNAMIC_SCALING
namespace GpuApi
{
	extern Uint32 g_DynamicScaleWidthFullRes;
	extern Uint32 g_DynamicScaleHeightFullRes;
}
#endif

void CRenderInterface::RenderFrame( CRenderFrame* frame, CRenderSceneEx* scene )
{
	// Moved before RenderFrame scope so GRenderingStats.m_gpuTimeStats is filled before the next frame starts rendering
	//dex++: reset rendering stats for current scene
#ifndef RED_FINAL_BUILD
	{
		GRenderingStats = SceneRenderingStats();
		m_lastGPUFrameTime = GpuApi::GetDeviceUsageStats().m_GPUFrameTime;
	}
	//dex--

	/*TDynArray< GpuScopeInfo > gpuTimes;
	m_gpuProfiler->GetEntries( gpuTimes );
	for ( TDynArray< GpuScopeInfo >::const_iterator it = gpuTimes.Begin(); it != gpuTimes.End(); ++it )
	{
	Float time = m_gpuProfiler->GetLastTime( *it );
	GRenderingStats.m_gpuTimeStats.PushBack( SceneRenderingStats::GpuTimesStat( it->m_name, it->m_parent, time ) );
	}*/
	m_gpuProfiler->GetEntriesTimes( GRenderingStats.m_gpuTimeStats );
	m_gpuProfiler->ClearHierarchyEntries();
#endif
	PC_SCOPE_RENDER_LVL0( RenderFrame );

	RedIOProfilerBlock ioBlock( TXT("RenderScene") );

	const bool rescalingAllowedByLoadingBlur = !GRenderThread->GetLoadingScreenBlur() || GRenderThread->GetLoadingScreenBlur()->AllowsDynamicRescaling();

#if MICROSOFT_ATG_DYNAMIC_SCALING
	bool rescalingAllowed = false;
	if ( frame->GetFrameInfo().m_customRenderResolution )
	{
		rescalingAllowed = false; 
	}
	else
	{
		frame->GetFrameInfo().m_width = GpuApi::g_DynamicScaleWidthFullRes;
		frame->GetFrameInfo().m_height = GpuApi::g_DynamicScaleHeightFullRes;

		Vector position = frame->GetFrameInfo().m_camera.GetPosition();
		Vector direction = frame->GetFrameInfo().m_camera.GetCameraForward();

		// allow rescaling only if the camera is moving or rotating
		static const float distanceTolerance = 0.05f;
		static const float cosineAngleTolerance = 0.9998f;	// 1 degree
		const bool cameraTravelledFarEnough = m_lastCameraPosition.DistanceSquaredTo(position) > (distanceTolerance * distanceTolerance);
		const bool cameraDirectionDifferentEnough = m_lastCameraDirection.Dot3(direction) < cosineAngleTolerance;
		rescalingAllowed = (cameraTravelledFarEnough || cameraDirectionDifferentEnough);
	
		m_lastCameraPosition = position;
		m_lastCameraDirection = direction;
	}
#endif
	// Apply config related settings
	{
		// Setup high precision rendering
		{
			GetSurfaces()->SetHighPrecision( Config::cvHighPrecisionRendering.Get() );
		}

		// Setup render settings (anizotropy etc).
		{
			const GpuApi::RenderSettings currentRenderSettings = GpuApi::GetRenderSettings();

			GpuApi::RenderSettings newRenderSettings = currentRenderSettings;
			newRenderSettings.anisotropy = Config::cvMaxTextureAnisotropy.Get();
			// already set using extern below
			// newRenderSettings.mipMapBias = Config::cvTextureMipBias.Get();

			const Bool error = newRenderSettings.Validate();
			// RED_ASSERT( error == false, TXT("Render settings neede to be fixed.") );

			if( GRenderSettingsMipBias != Config::cvTextureMipBias.Get() )
			{
				GRenderSettingsMipBias = Config::cvTextureMipBias.Get();
				GpuApi::ResetSamplerStates();
			}

			// If render settings has changed
			if( currentRenderSettings != newRenderSettings )
			{
				GpuApi::SetRenderSettings( newRenderSettings );
			}

		}

		// Setup hires entity shadowmap size
		{
			const Uint32 currentSize = m_hiresEntityShadowmap ? GpuApi::GetTextureLevelDesc( m_hiresEntityShadowmap, 0 ).width : 0;
			const Uint32 requestedSize = (Uint32) Max<Int32>( 0, Config::cvHiResEntityCustomShadowmapSize.Get() );
			if ( currentSize != requestedSize )
			{
				// Release current shadowmap
				GpuApi::SafeRelease( m_hiresEntityShadowmap );

				// Allocate new shadowmap
#if defined(RED_PLATFORM_WINPC) && !defined(NO_EDITOR)
				if ( requestedSize > 0 )
				{
					GpuApi::TextureDesc shadowmapDesc;
					shadowmapDesc.type = GpuApi::TEXTYPE_2D;
					shadowmapDesc.format = GpuApi::TEXFMT_D24S8;
					shadowmapDesc.width = requestedSize;
					shadowmapDesc.height = requestedSize;
					shadowmapDesc.initLevels = 1;
					shadowmapDesc.sliceNum = 1;
					shadowmapDesc.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_DepthStencil;
					m_hiresEntityShadowmap = GpuApi::CreateTexture( shadowmapDesc, GpuApi::TEXG_System );
					RED_ASSERT( m_hiresEntityShadowmap );
					if ( m_hiresEntityShadowmap )
					{
						GpuApi::AddDynamicTexture( m_hiresEntityShadowmap, "HiResEntityShadowmap" );
					}
				}
#endif
			}
		}
	}

	CRenderFrameInfo & info = frame->GetFrameInfo();

	// Prepare viewport for rendering
	CRenderViewport* viewport = static_cast< CRenderViewport* >( info.m_viewport );	

	// Start rendering
	{
		PC_SCOPE_RENDER_LVL1(BeginRender);
		GpuApi::BeginRender();
#ifndef RED_FINAL_BUILD
		m_gpuProfiler->UpdateFrequency( GpuApi::GetDeviceUsageStats().m_GPUFrequency );
#endif

		Bool canProcessResourceDeletions = true;

#ifdef TEXTURE_MEMORY_DEFRAG_ENABLED
		volatile static Bool defrag = true;
		if ( defrag )
		{
			const Bool defragFinished = m_defragHelper->TryFinishDefrag();
			canProcessResourceDeletions = defragFinished;
		}
#endif

		// Don't process deletions if there is still a defrag in flight. Otherwise, we might free some memory that's
		// still being copied into, and then allocate from that, leading to a lovely stomp.
		if ( canProcessResourceDeletions )
		{
			GpuApi::UpdateQueuedResourceDeletions();
		}

#ifdef TEXTURE_MEMORY_DEFRAG_ENABLED
		if ( defrag )
		{
			m_defragHelper->Tick( m_lastTickDelta, viewport && viewport->IsSceneRenderingSuppressed() );
		}
#endif

		if ( scene )
		{
			scene->BeginRendering();
		}
	}

	// Get rendering surfaces to use
	CRenderSurfaces* surfaces = GetSurfaces();
	ASSERT( surfaces );


	// Update scene tickables
	if ( scene )
	{
		if ( frame->GetFrameInfo().m_enableFPSDisplay 
#ifdef USE_ANSEL
			&& !isAnselCaptureActive 
#endif // USE_ANSEL
			)
		{
			static Float lastTime = 0.0f;
			static Float lastFPS = 0.0f;
			static Int32 numFrames = 0;
			static Float longestFrame = 0.0f;
			static Float lastLongestFrameMs = 0.0f;
			static Float currentElapsedTime = 0.0f;

			longestFrame = Max( m_lastTickDelta, longestFrame );
			currentElapsedTime += m_lastTickDelta;
			const Float timeElapsed = currentElapsedTime - lastTime;
			++numFrames;
			if ( timeElapsed >= 0.5f )
			{
				lastFPS = numFrames / timeElapsed;
				lastTime = currentElapsedTime;
				numFrames = 0;
				lastLongestFrameMs = longestFrame * 1000.0f;
				longestFrame = 0.0f;
			}
			String fpsDisplayString = String::Printf(TXT("FPS: %1.3f (longest frame: %1.3f ms) VSync: %hs VSyncThreshold: %d"), lastFPS, lastLongestFrameMs, Config::cvVSync.Get() ? "ON" : "OFF", Config::cvVSyncThreshold.Get() );
			frame->AddDebugScreenText( 20, 20, fpsDisplayString, Color::WHITE, nullptr, false, Color::BLACK, true );
#if MICROSOFT_ATG_DYNAMIC_SCALING
			String dynamicResolutionString = String::Printf(TXT("Res %ix%i"), GpuApi::g_DynamicScaleWidthFullRes, GpuApi::g_DynamicScaleHeightFullRes);
			frame->AddDebugScreenText( 20, 40, dynamicResolutionString, Color::WHITE, nullptr, false, Color::BLACK, true );
#endif
		}

#ifndef RED_FINAL_BUILD
		if ( info.IsShowFlagOn( SHOW_RenderGPUProfiler ) )
		{
			Uint32 counter = 0;
			for ( TDynArray< SceneRenderingStats::GpuTimesStat >::const_iterator it = GRenderingStats.m_gpuTimeStats.Begin(); it != GRenderingStats.m_gpuTimeStats.End(); ++it )
			{
				String profilerString = String::Printf(TXT("%1.3f - %ls : %ls"), it->m_time, it->m_parent.AsChar(), it->m_name.AsChar() );
				frame->AddDebugScreenText( 20, 50 + 20*counter, profilerString, Color::WHITE, nullptr, true, Color::BLACK, true );
				counter++;
			}
		}
#endif

		PC_SCOPE_RENDER_LVL1( UpdateSceneTickables );

		if ( !SScreenshotSystem::GetInstance().IsTakingScreenshot() )
		{
			const Float frameTime = GetRenderer()->GetLastTickDelta();
			scene->TickProxies( frameTime );
		}

		scene->SetTickCollectionEnabled( !SScreenshotSystem::GetInstance().IsTakingScreenshot() );
	}

	Bool supressSceneRendering = m_renderThread->GetVideoPlayer()->IsPlayingVideo();

	if ( scene && !supressSceneRendering )
	{
		// Spawn dynamic decals queued on a worker thread during particles simulation
		scene->SpawnQueuedDynamicDecals();
	}

	if ( scene )
	{
		if ( scene->GetCoherentTracker( CRenderSceneEx::ECFT_UpdateSkinningData ).Check( info.m_allowSkinningUpdate ) )
		{
			m_skinManager->StartFlushChanges();
		}
	}

#ifdef USE_SPEED_TREE
	// Update speedtree
	if ( scene && info.IsShowFlagOn( SHOW_Foliage ) )
	{
		PC_SCOPE_RENDER_LVL0( UpdateSpeedTree );

		if ( scene->GetSpeedTree() )
		{
			scene->GetSpeedTree()->FrameUpdate( info, scene );
		}
	}
#endif

	// Collect things to render
	m_collector.Setup( frame, scene, &GRenderCollectorData, supressSceneRendering );

	// The shadow culling can add to the collected tick list so we have to wait till that finishes
	// Cascades also must be initialized for the speed tree to start processing its shadow data
	m_collector.FinishShadowCulling();

	if ( scene && scene->IsWorldScene() )
	{
		PC_SCOPE_RENDER_LVL1( UpdateSpeedTreeShadows );
		scene->PrepareSpeedTreeCascadeShadows( m_collector );
	}

	// If the scene has changed OR last frame camera became invalidated, mark any scene coherent systems as dirty
	{	
		// Ignore persistent surfaces last frame data in some cases.
		// Note that we want this to happen only in case we're dealing with world scenes, since in editor
		// there might be multiple scenes open (with world scene being just one of them, and the rest would be
		// some preview scenes, like scenes editor, mesh preview etc).
		Bool makePersistentSurfacesDirty = false;
		{
			static void* oldScene = nullptr;
			static Bool oldSceneIsWorld = false;

			void* currentScene = scene;
			Bool currentSceneIsWorld = scene && scene->IsWorldScene();

			// Test if world scene changed.
			// This that this test is not perfect because it won't catch a scenario where we render worldSceneA, then previewScene and then worldSceneB.
			// We would like a reset in this case, but we won't get one (this might happen in editor only, and the implication are not that bad, so I'll
			// keep it this way).
			if ( oldScene != currentScene && (!(currentScene && oldScene) || ( oldSceneIsWorld && currentSceneIsWorld ) ) )
			{
				makePersistentSurfacesDirty = true;
			}

			// Test if there is a camera jump in the world scene
			if ( currentSceneIsWorld && !info.m_camera.GetLastFrameData().m_isValid )
			{
				makePersistentSurfacesDirty = true;
			}			

			//
			oldScene = currentScene;
			oldSceneIsWorld = currentSceneIsWorld;
		}

		if ( makePersistentSurfacesDirty )
		{
			surfaces->SetAllPersistentSurfacesDirty( true );			
		}
	}

	// Apply temporal AA related subpixel offset
	Bool hasTemporalAASubpixelOffset = false;
	const Float origTemporalAASubpixelOffsetX = info.m_camera.GetSubpixelOffsetX();
	const Float origTemporalAASubpixelOffsetY = info.m_camera.GetSubpixelOffsetY();

#ifdef USE_ANSEL
	if( !isAnselCaptureActive )
#endif // USE_ANSEL
	{
		if ( GetPostProcess()->IsPostFxTemporalAAEnabled( m_collector, true ) && info.IsViewportPresent() )
		{
			Float offX = 0.f;
			Float offY = 0.f;
			switch ( m_collector.m_frameIndex % 2 )
			{
			case 0:		offX = -0.25f; offY = -0.25f; break;
			case 1:		offX = 0.25f; offY = 0.25f; break;
			};

			info.SetSubpixelOffset( offX, offY, info.m_width, info.m_height );
			hasTemporalAASubpixelOffset = true;
		}
	}

	// viewport can be NULL - eg. generating thumbnails
	if ( viewport )
	{
		// Prepare viewport - if device lost, do not render
		if ( !viewport->PrepareViewport() || IsDeviceLost() )
		{
			return;
		}

		// Check if rendering is suppressed for current viewport
		supressSceneRendering =  supressSceneRendering || viewport->IsSceneRenderingSuppressed() || info.m_isLastFrameForced;
	}


	// Update 3D stereo
	// HACK DX10 no stereo
	//GpuApi::UpdateStereoTexture( info.m_envParametersDayPoint.m_convergence );


	// Reset render states to a known state
	m_stateManager->Reset();

	// Reset texture bindings
	GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );

	// Upload texture atlas mapping data
	//m_atlasManager->FlushChanges();

	if ( scene != nullptr )
	{
		if ( scene->GetTerrain() )
		{
			// Update terrain. For PS4-friendliness, this has to happen after BeginRender.
			scene->GetTerrain()->FlushTerrainUpdates();
		}

		// Upload skin texture
		m_skinManager->FinishFlushChanges();

#ifdef USE_NVIDIA_FUR
		if ( m_collector.GetRenderFrameInfo().IsShowFlagOn( SHOW_HairAndFur ) && scene->m_furProxies.Size() )
		{
			UpdateFurSimulation( m_collector );
		}
#endif

	}

	// Texture streaming manager may have an update task running. The first part of the task involves collecting textures and
	// updating their last-drawn-distances. Make sure that the task is done with that before proceeding with rendering, so we
	// don't try and set new distances while it's busy changing them too.
	m_textureStreamingManager->EnsureUpdateTaskIsSafe();


	// If there are any pending frame prefetches, start one. We'll only do one per frame.
	CRenderFramePrefetch* activePrefetch = StartNextFramePrefetch();
	if ( scene )
	{
		// inform scene about prefetch going on
		scene->SetCurrentFramePrefetch( activePrefetch );
	}


	// Hit proxy
	if ( info.m_renderingMode == RM_HitProxies )
	{
		// Set the viewport back
		{
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_FinalColor ) );
			rtSetup.depthTarget = surfaces->GetDepthBufferTex();
			rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );
		}

		// Make sure any culling tasks are done
		m_collector.FinishSceneCulling();

		// Render hit proxies only
		RenderHitProxyFrame( m_collector );
	}
	else if ( !supressSceneRendering )
	{
		PC_SCOPE_RENDER_LVL0(RenderScene);


		PreRenderNormalFrame( m_collector );


		m_gameplayFX->SetGameTime( info.m_engineTime );

		Uint32 numUberSamples = 1;
		
#ifdef USE_ANSEL
		const Bool isDebugZoomAllowed = isAnselCaptureActive ? false : m_collector.IsWorldScene();
#else
		const Bool isDebugZoomAllowed = m_collector.IsWorldScene();
#endif // USE_ANSEL

#ifdef USE_ANSEL
		if( !isAnselCaptureActive )
#endif // USE_ANSEL
		{
			if ( !GetPostProcess()->IsDebugOverlayActivated( info, isDebugZoomAllowed ) )
			{
				if ( (Config::cvUberSampling.Get() > 1) && !GIsRendererTakingUberScreenshot )
				{
					numUberSamples = Clamp<Uint32>( Config::cvUberSampling.Get(), 2, 16 );
				}
			}
		}

		if ( numUberSamples > 1 )
		{
			// Read samples from cached config
			CRenderCamera& camera = const_cast< CRenderCamera& >( info.m_camera );
			const Float origSubpixelOffsetX = camera.GetSubpixelOffsetX();
			const Float origSubpixelOffsetY = camera.GetSubpixelOffsetY();

			// Setup uber quality render settings 
			const GpuApi::RenderSettings prevRenderSettings = GpuApi::GetRenderSettings();
			{		
				GpuApi::RenderSettings newSettings = prevRenderSettings;

				// Calculate mipmap bias
				static const Float log2f = logf( 2.0f );
				const Float samplesPerPixel = static_cast<Float>(numUberSamples);
				newSettings.mipMapBias = -( logf( samplesPerPixel ) / log2f );		

				if ( newSettings != prevRenderSettings )
				{
					GpuApi::SetRenderSettings( newSettings );
				}
			}

			// Multisample the whole scene
			for ( Uint32 i = 0; i < numUberSamples; ++i )
			{
				for ( Uint32 j = 0; j < numUberSamples; ++j )
				{
					const Bool isFirst = (i == 0) && (j == 0);
					const Bool isLast  = (i + 1 == numUberSamples) && (j + 1 == numUberSamples);
					GIsDuringUberSampleIsFirst = isFirst;

					// Calculate sub pixel offset for this iteration
					const Float subPixelOffsetX = static_cast<Float>( numUberSamples - 1 - i ) / static_cast<Float>( numUberSamples );
					const Float subPixelOffsetY = static_cast<Float>( numUberSamples - 1 - j ) / static_cast<Float>( numUberSamples );

					// HACK: Update the sub pixel offset for the camera
					info.SetSubpixelOffset( subPixelOffsetX, subPixelOffsetY, info.m_width, info.m_height );
					ASSERT( !hasTemporalAASubpixelOffset );

					// Render frame content
					RenderNormalFrame( m_collector );

					// Postprocess
					ERenderTargetName rtnColor = RTN_Color;
					if ( info.m_allowPostSceneRender )
					{
						rtnColor = GetPostProcess()->DrawFinal( m_collector, info, surfaces, m_collector.m_postProcess, isFirst, isLast );
					}
					else if ( IsMSAAEnabled( info ) )
					{
						VERIFY( m_postProcess->ResolveMSAAColorSimple( info, surfaces, RTN_Color ) );
					}

					// Copy to temp surface - set for first sample, add for next ones
					ASSERT( RTN_UbersampleAccum != rtnColor );
					const GpuApi::eDrawContext context = isFirst ? GpuApi::DRAWCONTEXT_PostProcSet : GpuApi::DRAWCONTEXT_PostProcAdd;
					const Float invNumSamples = 1.f / (Float)(numUberSamples * numUberSamples);
					GetPostProcess()->CopyWithScale( info, surfaces, rtnColor, RTN_UbersampleAccum, invNumSamples, context );				
				}
			}

			// Finalize
			if ( info.m_allowPostSceneRender )
			{
				GetPostProcess()->DrawFinalizationAndAA( m_collector, info, surfaces, m_collector.m_postProcess, RTN_UbersampleAccum );
			}

			// Restore some states
			info.SetSubpixelOffset( origSubpixelOffsetX, origSubpixelOffsetY, info.m_width, info.m_height );
			GpuApi::SetRenderSettings( prevRenderSettings );
		}
		else
		{
			GIsDuringUberSampleIsFirst = true;

			// If rendering profile changed, switch mip bias to 0 - but not if taking uber screenshot
			GpuApi::RenderSettings prevRenderSettings = GpuApi::GetRenderSettings();
			if ( (prevRenderSettings.mipMapBias != 0.0f) && !GIsRendererTakingUberScreenshot )
			{
				prevRenderSettings.mipMapBias = 0.0f;
				GpuApi::SetRenderSettings( prevRenderSettings );
			}

			// Save original renderFeaturesFlags
			ASSERT( &m_collector.m_frame->GetFrameInfo() == &m_collector.GetRenderFrameInfo() && &info == &m_collector.GetRenderFrameInfo() );
			const Uint32 prevRenderFeaturesFlags = info.m_renderFeaturesFlags;

			// Determine feedback render feature flags
			Uint32 feedbackFeaturesFlags = prevRenderFeaturesFlags;
			if ( GetPostProcess()->IsDebugOverlayActivated( info, isDebugZoomAllowed ) )
			{
				switch ( info.m_envParametersGame.m_displaySettings.m_displayMode )
				{
				case EMM_DecomposedAmbient:				feedbackFeaturesFlags = (feedbackFeaturesFlags & ~DMFF_MASK_LIGHTING) | DMFF_LightingAmbient; break;
				case EMM_DecomposedDiffuse:				feedbackFeaturesFlags = (feedbackFeaturesFlags & ~DMFF_MASK_LIGHTING) | DMFF_LightingDiffuse; break;
				case EMM_DecomposedSpecular:			feedbackFeaturesFlags = (feedbackFeaturesFlags & ~DMFF_MASK_LIGHTING) | DMFF_LightingSpecular; break;
				case EMM_DecomposedReflection:			feedbackFeaturesFlags = (feedbackFeaturesFlags & ~DMFF_MASK_LIGHTING) | DMFF_LightingReflection; break;
				}
			}

			// Render finalized frame content
			RenderNormalFrame( m_collector );

			// Postprocess and finalization
			ERenderTargetName rtnColor = RTN_Color;
			if ( info.m_allowPostSceneRender )
			{
				rtnColor = GetPostProcess()->DrawFinal( m_collector, info, surfaces, m_collector.m_postProcess, true, true );
				GetPostProcess()->DrawFinalizationAndAA( m_collector, info, surfaces, m_collector.m_postProcess, rtnColor );
			}
			else
			{
				if ( IsMSAAEnabled( info ) 
#ifdef USE_ANSEL
					&& !isAnselCaptureActive
#endif // USE_ANSEL
					)
				{
					VERIFY( m_postProcess->ResolveMSAAColorSimple( info, surfaces, RTN_Color ) );
				}

				// ace_ibl_hack: make some better access for perspectiveDepth buffer.
				{
					CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

					Rect rect( 0, info.m_width, 0, info.m_height );
					StretchRect( surfaces->GetDepthBufferTex(), rect, surfaces->GetRenderTargetTex( RTN_GBuffer3Depth ), rect );
				}
			}

			// Display pigment map
			if ( info.IsShowFlagOn( SHOW_PigmentMap ) )
			{
				if ( scene && scene->GetTerrain() )
				{
					CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

					const GpuApi::TextureRef tex = scene->GetTerrain()->GetPigmentData().m_texture;
					const PostProcessUtilities::TexelArea fromArea ( GpuApi::GetTextureLevelDesc(tex,0).width, GpuApi::GetTextureLevelDesc(tex,0).height );
					const PostProcessUtilities::TexelArea toArea ( Min( 512, fromArea.m_width ), Min( 512, fromArea.m_height ) );
					GetPostProcess()->CopyWithScale( tex, fromArea, surfaces->GetRenderTargetTex(RTN_FinalColor), toArea, 1, GpuApi::DRAWCONTEXT_PostProcSet );
				}
			}

			// Render debug overlay
			if ( GetPostProcess()->IsDebugOverlayActivated( info, isDebugZoomAllowed ) )
			{
				RED_ASSERT( info.m_allowPostSceneRender, TXT("Shouldn't this this codepath be excluded?") );

				// Process differenceBased displayModes
				if ( feedbackFeaturesFlags != prevRenderFeaturesFlags )
				{
					// Some displayModes need additional because it may be easier to extract some features this way. 
					// Another reason is that we want the same tonemapping when displaying debug info.
					feedbackFeaturesFlags &= ~DMFF_ToneMappingLuminanceUpdate;
					feedbackFeaturesFlags &= ~DMFF_EnvProbeUpdate;

					//
					ASSERT( &m_collector.m_frame->GetFrameInfo() == &m_collector.GetRenderFrameInfo() && &info == &m_collector.GetRenderFrameInfo() );
					info.m_renderFeaturesFlags = feedbackFeaturesFlags;

					// Copy to feedback buffer
					//Rect rect( 0, info.GetWidth(), 0, info.GetHeight() );
					//StretchRect( surfaces->GetRenderTargetTex( rtnColor ), rect, surfaces->GetRenderTargetTex( RTN_UbersampleAccum ), rect );

					// Render finalized frame content
					RenderNormalFrame( m_collector );
					const ERenderTargetName rtnColor = GetPostProcess()->DrawFinal( m_collector, info, surfaces, m_collector.m_postProcess, true, true );
					GetPostProcess()->DrawFinalizationAndAA( m_collector, info, surfaces, m_collector.m_postProcess, rtnColor );

					// Copy to second feedback buffer
					if ( rtnColor != RTN_Color )
					{
						Rect rect( 0, info.m_width, 0, info.m_height );
						StretchRect( surfaces->GetRenderTargetTex( rtnColor ), rect, surfaces->GetRenderTargetTex( RTN_Color ), rect );
					}
				}

				// Render debug overlay
				{
					BindForwardConsts( info, GetGlobalCascadesShadowResources(), GetSurfaces(), true, GpuApi::PixelShader );
					BindForwardConsts( info, GetGlobalCascadesShadowResources(), GetSurfaces(), true, GpuApi::VertexShader );
					BindSharedConstants( GpuApi::PixelShader );
					GetPostProcess()->DrawDebugOverlay( m_collector, info, surfaces, isDebugZoomAllowed );
					UnbindSharedConstants( GpuApi::PixelShader );
					UnbindForwardConsts( info, GpuApi::PixelShader );
					UnbindForwardConsts( info, GpuApi::VertexShader );
				}
			}

			// Restore some settings
			ASSERT( &m_collector.m_frame->GetFrameInfo() == &m_collector.GetRenderFrameInfo() && &info == &m_collector.GetRenderFrameInfo() );
			info.m_renderFeaturesFlags = prevRenderFeaturesFlags;
			GpuApi::SetRenderSettings( prevRenderSettings );
		}

		// Render debug overlay
		if ( info.m_allowPostSceneRender )
		{
			RenderDebugOverlay( m_collector, frame, surfaces, viewport );
		}

		if ( info.m_allowPostSceneRender && info.m_multiplanePresent )
		{
			if ( viewport )
			{
				GpuApi::Rect sourceRect, targetRect;

				sourceRect.left = 0;
				sourceRect.top  = 0;
				sourceRect.right  = info.m_width;
				sourceRect.bottom = info.m_height;

				if( Config::cvForcedRendererBackBufferResolution.Get() )
				{
					targetRect.left = 0;
					targetRect.top  = 0;
					targetRect.right = Config::cvForcedRendererResolutionWidth.Get();
					targetRect.bottom = Config::cvForcedRendererResolutionHeight.Get();
				}
				else
				{
					targetRect.left = (viewport->GetFullWidth() - viewport->GetWidth()) / 2;
					targetRect.top = (viewport->GetFullHeight() - viewport->GetHeight()) / 2;
					targetRect.right = viewport->GetFullWidth() - ((viewport->GetFullWidth() - viewport->GetWidth()) / 2);
					targetRect.bottom = viewport->GetFullHeight() - ((viewport->GetFullHeight() - viewport->GetHeight()) / 2);
				}

#if MICROSOFT_ATG_DYNAMIC_SCALING
				//HACK make sure no rescaling happens
				targetRect.left = 0;
				targetRect.top = 0;
				targetRect.right = sourceRect.right;
				targetRect.bottom = sourceRect.bottom;
#endif
				m_postProcess->PresentCopy( info, surfaces, RTN_FinalColor, sourceRect, targetRect );
			}
		}
	}

	// Restore subpixel offset
	if ( hasTemporalAASubpixelOffset )
	{
		info.SetSubpixelOffset( origTemporalAASubpixelOffsetX, origTemporalAASubpixelOffsetY, info.m_width, info.m_height );
	}

	// Create a thumbnail of the texture for game saves
	RenderThumbnailScreenshot( info );

	RenderFinal2D( frame, info, supressSceneRendering, rescalingAllowedByLoadingBlur );

	if ( info.m_allowPostSceneRender && !info.m_multiplanePresent )
	{
		if ( viewport )
		{
			GpuApi::Rect sourceRect, targetRect;

			sourceRect.left = 0;
			sourceRect.top  = 0;
			sourceRect.right  = info.m_width;
			sourceRect.bottom = info.m_height;

			if( Config::cvForcedRendererBackBufferResolution.Get() )
			{
				targetRect.left = 0;
				targetRect.top  = 0;
				targetRect.right = Config::cvForcedRendererResolutionWidth.Get();
				targetRect.bottom = Config::cvForcedRendererResolutionHeight.Get();
			}
			else
			{
				targetRect.left = (viewport->GetFullWidth() - viewport->GetWidth()) / 2;
				targetRect.top = (viewport->GetFullHeight() - viewport->GetHeight()) / 2;
				targetRect.right = viewport->GetFullWidth() - ((viewport->GetFullWidth() - viewport->GetWidth()) / 2);
				targetRect.bottom = viewport->GetFullHeight() - ((viewport->GetFullHeight() - viewport->GetHeight()) / 2);
			}


			m_postProcess->PresentCopy( info, surfaces, RTN_FinalColor, sourceRect, targetRect );
		}
	}


	// If we started a prefetch this frame, finish the query. This should be before we end rendering on
	// the scene, in case we still rely on anything in the scene.
	if ( activePrefetch != nullptr )
	{
		FinishFramePrefetch( activePrefetch );
		activePrefetch = nullptr;
	}
	if ( scene )
	{
		// clear active prefetch from scene
		scene->SetCurrentFramePrefetch( nullptr );
	}


	// Make sure onscreen/offscreen particles simulation is finished as in m_collector.Reset we will reset particles array
	// Do this before scene->EndRendering, because that may result it destruction of some objects.
	GetParticleBatcher()->FinishSimulateParticles();
	m_collector.Reset();

	// Handle render to texture frames.
	if ( viewport == nullptr && frame->GetRenderTarget() && info.m_present && !m_dropOneFrame )
	{
		const GpuApi::TextureDesc& desc = GpuApi::GetTextureDesc( frame->GetRenderTarget()->GetGpuTexture() );
		::Rect  sourceRect;
		sourceRect.m_left = 0;
		sourceRect.m_top = 0;
		sourceRect.m_right = desc.width;
		sourceRect.m_bottom = desc.height;

		frame->GetRenderTarget()->CopyFromRenderTarget( RTN_FinalColor , sourceRect );
	}

	// End rendering
	{
		PC_SCOPE_RENDER_LVL1(EndRender);

		if ( scene )
		{
			// Just in case, tick anything that might still be sitting in the collected tick list.
			scene->TickCollectedProxies();

			scene->EndRendering();
		}


		if ( m_textureStreamingManager )
		{
			// If we haven't suppressed rendering, we update streaming as normal.
			// Otherwise, we'll update streaming without automatically timing out textures.
			m_textureStreamingManager->UpdateTextureStreaming( m_suppressRendering == 0 );
		}


		GpuApi::EndRender();
	}


	// Present on viewport (if it is not a render to texture scene), if texture streaming flush was requested do not show
	if ( viewport && info.m_present && !m_dropOneFrame )
	{
		viewport->Present( info.m_multiplanePresent );
#if MICROSOFT_ATG_DYNAMIC_SCALING
		if( !supressSceneRendering && rescalingAllowedByLoadingBlur && GpuApi::UpdateDynamicScaling( rescalingAllowed ) )
		{
			#ifdef REALTIME_REFLECTIONS_FIXED_SIZE
			# if REALTIME_REFLECTIONS_FIXED_SIZE
				// empty
			# else
				GetRenderer()->GetSurfaces()->SetPersistentSurfaceDirty( CRenderSurfaces::PS_RLRHistory, true );
			# endif
			#else
			# error Expected definition
			#endif

			GetRenderer()->GetSurfaces()->SetPersistentSurfaceDirty( CRenderSurfaces::PS_TemporalAntialias, true );
		}
#endif
	}
}



#ifndef RED_FINAL_BUILD
void CRenderInterface::UpdateRenderingStats( CRenderCollector& collector )
{
	// ----------------------
	// Push the stats to scene
	// ----------------------
	if ( collector.m_scene )
	{
		// copy general scene data coutn
		GRenderingStats.m_numSceneChunks				+= collector.m_sceneStats.m_numChunks;
		GRenderingStats.m_numSceneTriangles				+= collector.m_sceneStats.m_numTriangles;
		GRenderingStats.m_numSceneVerts					+= collector.m_sceneStats.m_numVertices;

		GRenderingStats.m_numSceneChunksStatic			+= collector.m_sceneStats.m_numChunksStatic;
		GRenderingStats.m_numSceneTrianglesStatic		+= collector.m_sceneStats.m_numTrianglesStatic;
		GRenderingStats.m_numSceneVertsStatic			+= collector.m_sceneStats.m_numVerticesStatic;

		GRenderingStats.m_numSceneChunksSkinned			+= collector.m_sceneStats.m_numChunksSkinned;
		GRenderingStats.m_numSceneTrianglesSkinned		+= collector.m_sceneStats.m_numTrianglesSkinned;
		GRenderingStats.m_numSceneVertsSkinned			+= collector.m_sceneStats.m_numVerticesSkinned;

		GRenderingStats.m_numSceneBatches				+= collector.m_sceneStats.m_numBatches;
		GRenderingStats.m_numSceneInstancedBatches		+= collector.m_sceneStats.m_numInstancedBatches;
		GRenderingStats.m_biggestSceneBatch				 = collector.m_sceneStats.m_biggestBatch;
		GRenderingStats.m_biggestSceneInstancedBatch	 = collector.m_sceneStats.m_biggestInstancedBatch;
		GRenderingStats.m_smallestSceneBatch			 = collector.m_sceneStats.m_smallestBatch;
		GRenderingStats.m_smallestSceneInstancedBatch	 = collector.m_sceneStats.m_smallestInstancedBatch;

#ifdef RED_PLATFORM_CONSOLE
		GRenderingStats.m_numConstantBufferUpdates		+= GpuApi::GetDeviceUsageStats().m_NumConstantBufferUpdates;
		GRenderingStats.m_constantMemoryLoad			+= GpuApi::GetDeviceUsageStats().m_constantMemoryLoad;
		GRenderingStats.m_isConstantBufferSafe			= GpuApi::GetDeviceUsageStats().m_isConstantBufferMemoryWithinBounds;
#endif

		// update occlusion data
		GRenderingStats.m_visibleObjects						= collector.m_sceneStats.m_visibleObjects;
		GRenderingStats.m_occlusionTimeDynamicObjects			= collector.m_sceneStats.m_occlusionTimeDynamicObjects;
		GRenderingStats.m_occlusionTimeVisibilityByDistance		= collector.m_sceneStats.m_occlusionTimeVisibilityByDistance;

		GRenderingStats.m_furthestProxiesTime					= collector.m_sceneStats.m_furthestProxiesTime;
		GRenderingStats.m_furthestProxiesOcclusionTime			= collector.m_sceneStats.m_furthestProxiesOcclusionTime;
		GRenderingStats.m_furthestProxiesDistanceTime			= collector.m_sceneStats.m_furthestProxiesDistanceTime;
		GRenderingStats.m_furthestProxiesCollectionTime			= collector.m_sceneStats.m_furthestProxiesCollectionTime;

		GRenderingStats.m_renderedShadowStaticProxies			= collector.m_sceneStats.m_renderedShadowStaticProxies;
		GRenderingStats.m_renderedShadowDynamicProxies			= collector.m_sceneStats.m_renderedShadowDynamicProxies;

		GRenderingStats.m_renderedStaticProxies					= collector.m_sceneStats.m_renderedStaticProxies;
		GRenderingStats.m_renderedDynamicProxies				= collector.m_sceneStats.m_renderedDynamicProxies;
		GRenderingStats.m_renderedFurthestProxies				= collector.m_sceneStats.m_renderedFurthestProxies;
		GRenderingStats.m_renderedDynamicDecals					= collector.m_sceneStats.m_renderedDynamicDecals;
		GRenderingStats.m_renderedDynamicDecalsCount			= collector.m_sceneStats.m_renderedDynamicDecalsCount;

		GRenderingStats.m_occludedDynamicProxies				= collector.m_sceneStats.m_occludedDynamicProxies;
		GRenderingStats.m_occludedDynamicProxies				= collector.m_sceneStats.m_occludedFurthestProxies;
		GRenderingStats.m_occludedDynamicDecals					= collector.m_sceneStats.m_occludedDynamicDecals;

		//shadows
		GRenderingStats.m_shadowQueryTime						= collector.m_sceneStats.m_shadowQueryTime;
		GRenderingStats.m_stStatic								= collector.m_sceneStats.m_stStatic;
		GRenderingStats.m_stStaticDistance						= collector.m_sceneStats.m_stStaticDistance;
		GRenderingStats.m_stStaticCollection					= collector.m_sceneStats.m_stStaticCollection;
		GRenderingStats.m_stStaticCulledByDistance				= collector.m_sceneStats.m_stStaticCulledByDistance;

		GRenderingStats.m_stDynamic								= collector.m_sceneStats.m_stDynamic;
		GRenderingStats.m_stDynamicDistance						= collector.m_sceneStats.m_stDynamicDistance;
		GRenderingStats.m_stDynamicUmbra						= collector.m_sceneStats.m_stDynamicUmbra;
		GRenderingStats.m_stDynamicCollection					= collector.m_sceneStats.m_stDynamicCollection;

		if ( collector.m_scene->m_renderElementMap )
		{
			GRenderingStats.m_registeredStaticProxies			= collector.m_scene->m_renderElementMap->GetStaticProxiesCount();
			GRenderingStats.m_registeredDynamicDecals			= collector.m_scene->m_renderElementMap->GetDynamicDecalsCount();

			const SRenderElementMapStats& reMapStats			= collector.m_scene->m_renderElementMap->GetStats();
			GRenderingStats.m_reMapStatsStaticMeshes			= reMapStats.m_staticMeshes;
			GRenderingStats.m_reMapStatsDynamicMeshes			= reMapStats.m_dynamicMeshes;
			GRenderingStats.m_reMapStatsMeshesNotInObjectCache	= reMapStats.m_meshesNotInObjectCache;
			GRenderingStats.m_reMapStatsApex					= reMapStats.m_apex;
			GRenderingStats.m_reMapStatsBakedDecals				= reMapStats.m_bakedDecals;
			GRenderingStats.m_reMapStatsNonBakedDecals			= reMapStats.m_nonBakedDecals;
			GRenderingStats.m_reMapStatsBakedDimmers			= reMapStats.m_bakedDimmers;
			GRenderingStats.m_reMapStatsNonBakedDimmers			= reMapStats.m_nonBakedDimmers;
			GRenderingStats.m_reMapStatsBakedStripes			= reMapStats.m_bakedStripes;
			GRenderingStats.m_reMapStatsNonBakedStripes			= reMapStats.m_nonBakedStripes;
			GRenderingStats.m_reMapStatsFlares					= reMapStats.m_flares;
			GRenderingStats.m_reMapStatsFur						= reMapStats.m_fur;
			GRenderingStats.m_reMapStatsParticles				= reMapStats.m_particles;
			GRenderingStats.m_reMapStatsBakedPointLights		= reMapStats.m_bakedPointLights;
			GRenderingStats.m_reMapStatsNonBakedPointLights		= reMapStats.m_nonBakedPointLights;
			GRenderingStats.m_reMapStatsBakedSpotLights			= reMapStats.m_bakedSpotLights;
			GRenderingStats.m_reMapStatsNonBakedSpotLights		= reMapStats.m_nonBakedSpotLights;
		}

		// update scene data
		collector.m_scene->UpdateSceneStats( GRenderingStats );
	}

	GDumpMeshStats = false;
}
#endif