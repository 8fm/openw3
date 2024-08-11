/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderPostProcess.h"
#include "renderShaderPair.h"
#include "renderRenderSurfaces.h"
#include "renderEnvProbe.h"
#include "renderEnvProbeManager.h"
#include "renderScene.h"
#include "renderSkybox.h"
#include "renderShadowManager.h"
#include "renderTextureArray.h"
#include "../engine/baseEngine.h"
#include "../engine/renderFragment.h"


void BuildEnvProbeCamera( CRenderCamera &camera, const Vector &origin, Uint32 faceIndex )
{
	ASSERT( faceIndex < 6 );

	EulerAngles cameraRotations[6] =
	{ 
		EulerAngles( -90.0f, 0.0f, 90.0f ),
		EulerAngles( 90.0f, 0.0f, -90.0f ),
		EulerAngles( 0.0f, 0.0f, 180.0f ),
		EulerAngles( 0.0f, 180.0f, 180.0f ),
		EulerAngles( 0.0f, -90.0f, 180.0f ),
		EulerAngles( 0.0f, 90.0f, 0.0f ),
	};

	const Float fov = 90.f;

	const Float nearPlane = 0.5f;
	const Float farPlane = 4000.f;

	camera.Set( origin, cameraRotations[faceIndex], fov, 1.0f, nearPlane, farPlane, 1.0f );
}

void RenderEnvProbeFace( const CRenderCollector &mainRenderCollector, MeshDrawingStats &meshDrawingStats, const CRenderFrameInfo &info, const CRenderSceneEx *scene, CRenderInterface &renderer, CRenderEnvProbe &envProbe, const CRenderEnvProbeManager::SCubeSlotData &envProbeSlot, Uint32 faceIndex, const GpuApi::TextureRef &texRenderTarget, const GpuApi::TextureRef &texTempShadowMask, Uint32 renderResolution, const void *tiledConstantBuffer )
{
	ASSERT( renderResolution <= renderer.GetSurfaces()->GetWidth() );
	ASSERT( renderResolution <= renderer.GetSurfaces()->GetHeight() );
	ASSERT( NULL != tiledConstantBuffer );
	
	const CCascadeShadowResources &cascadeShadowResources = GetRenderer()->GetGlobalCascadesShadowResources();
	const Uint32 rtFullWidth = CRenderEnvProbeManager::GetMaxProbeTypeResolution();
	const Uint32 rtFullHeight = CRenderEnvProbeManager::GetMaxProbeTypeResolution();
	
	// Update surfaces guard
	renderer.SetupSurfacesAvailabilityGuard( false );

	// Setup constants
	{
		GetRenderer()->GetStateManager().SetCamera( info.m_camera );

		// Set some global constants
		GPUAPI_ASSERT( -1 != envProbe.GetDynamicData().m_arraySlotIndex );
		GetRenderer()->CalculateSharedConstants( info, rtFullWidth, rtFullHeight, envProbe.GetDynamicData().m_arraySlotIndex, -1 != envProbe.GetDynamicData().m_lastFullUpdateFinishUpdateTime ? 1.f : 0.f );
		GetRenderer()->BindSharedConstants( GpuApi::PixelShader );

		// Setup global camera parameters
		GetRenderer()->GetStateManager().SetLocalToWorld( NULL );
		GetRenderer()->GetStateManager().SetGlobalShaderConstants( info, rtFullWidth, rtFullHeight, GetRenderer()->GetGameplayFX() );
		GetRenderer()->GetStateManager().BindGlobalConstants();
		GetRenderer()->BindGlobalSkyShadowTextures( info, GpuApi::PixelShader );

		{
			GetRenderer()->ImportTiledDeferredConstants( tiledConstantBuffer ); // Various, Lights, Dimmers included in provided buffer
			GetRenderer()->CalculateTiledDeferredConstants_CascadeShadows( info, &mainRenderCollector.m_cascades, cascadeShadowResources );
			GetRenderer()->CalculateTiledDeferredConstants_TerrainShadows( info, scene );
			GetRenderer()->FlushTiledDeferredConstants();
		}
	}

	// Get gbuffers 
	GpuApi::TextureRef texGBuffer0;
	GpuApi::TextureRef texGBuffer1;
	GpuApi::TextureRef texDepthBuffer;
	GpuApi::TextureRef texGlobalShadow;
	{
		GPUAPI_ASSERT( -1 != envProbe.GetDynamicData().m_arraySlotIndex );
		const CRenderEnvProbeManager::SCubeSlotData &sdata = GetRenderer()->GetEnvProbeManager()->GetCubeSlotData( envProbe.GetDynamicData().m_arraySlotIndex  );

		texGBuffer0		= sdata.GetFaceTexture( ENVPROBEBUFFERTEX_Albedo );
		texGBuffer1		= sdata.GetFaceTexture( ENVPROBEBUFFERTEX_Normals );
		texDepthBuffer	= envProbeSlot.GetFaceTexture( ENVPROBEBUFFERTEX_Depth );
		texGlobalShadow	= envProbeSlot.GetFaceTexture( ENVPROBEBUFFERTEX_GlobalShadow );
	}

	// Unpack shadowmask
	{
		GPUAPI_ASSERT( -1 != envProbe.GetDynamicData().m_arraySlotIndex );
		const CRenderEnvProbeManager::SCubeSlotData &sdata = GetRenderer()->GetEnvProbeManager()->GetCubeSlotData( envProbe.GetDynamicData().m_arraySlotIndex  );

		// unpack shadowmask
		{		
			// Build weights vector
			Vector channelWeights0 ( 0, 0, 0, 0 );
			Vector channelWeights1 ( 0, 0, 0, 0 );
			Vector bitMasks ( 0, 0, 0, 0 );
			{
				const Float  timeFactor = Max( 0.f, Min( 1.f, info.m_gameTime / (24.f * 60.f * 60.f) ) );
				const Uint32 numTimepoints = 4 * 8;
				const Uint32 timepointIndex0 = Min( numTimepoints - 1, (Uint32)(timeFactor * numTimepoints) );
				const Uint32 timepointIndex1 = timepointIndex0 + 1 < numTimepoints ? timepointIndex0 + 1 : 0;
				const Float  blendFactor = (timeFactor - timepointIndex0 / (Float)numTimepoints) * numTimepoints;
				const Uint32 channelIndex0 = timepointIndex0 / 8;
				const Uint32 channelIndex1 = timepointIndex1 / 8;

				channelWeights0.Set4( 0 == channelIndex0 ? 255.f : 0.f, 1 == channelIndex0 ? 255.f : 0.f, 2 == channelIndex0 ? 255.f : 0.f, 3 == channelIndex0 ? 255.f : 0.f );
				channelWeights1.Set4( 0 == channelIndex1 ? 255.f : 0.f, 1 == channelIndex1 ? 255.f : 0.f, 2 == channelIndex1 ? 255.f : 0.f, 3 == channelIndex1 ? 255.f : 0.f );
				bitMasks.Set4( (Float)(1 << (timepointIndex0 % 8)), (Float)(1 << (timepointIndex1 % 8)), blendFactor, 0.f );
			}

			// Setup targets
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, texTempShadowMask );
			rtSetup.SetViewport( renderResolution, renderResolution );
			GpuApi::SetupRenderTargets( rtSetup );

			// Bind textures
			GpuApi::TextureRef refs[2] = { texGlobalShadow, texDepthBuffer };
			GpuApi::BindTextures( 0, 2, &(refs[0]), GpuApi::PixelShader );

			// Bind clouds texture from skybox material
			GetRenderer()->BindGlobalSkyShadowTextures( info, GpuApi::PixelShader );

			// Setup draw context
			CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

			//

			GetRenderer()->BindForwardConsts( info, cascadeShadowResources, NULL, false, GpuApi::PixelShader);
			// BindForwardConsts for VertexShader not needed here!

			// Render
			renderer.m_shaderEnvProbeShadowmask->Bind();
			renderer.GetStateManager().SetPixelConst( PSC_Custom_0, channelWeights0 );
			renderer.GetStateManager().SetPixelConst( PSC_Custom_1, channelWeights1 );
			renderer.GetStateManager().SetPixelConst( PSC_Custom_2, bitMasks );
			renderer.GetStateManager().SetPixelConst( PSC_Custom_3, Vector ( (Float)faceIndex, 0, 0, 0 ) );
			renderer.GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );

			//
			GetRenderer()->UnbindForwardConsts( info, GpuApi::PixelShader );
			
			// Unbind textures
			GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
			GpuApi::BindTextures( 1, 1, nullptr, GpuApi::PixelShader );
			GpuApi::BindTextures( 14, 1, nullptr, GpuApi::PixelShader );
		}
	}

	// Render sky
	// (rendered first because we're not using depth buffer, and scene lighting shader writes to scene related pixels only)
	if ( scene && scene->GetSkybox() )
	{
		// Bind main rendertargets
		GpuApi::RenderTargetSetup rtSetupMain;
		rtSetupMain.SetColorTarget( 0, texRenderTarget );
		rtSetupMain.SetViewport( renderResolution, renderResolution );
		GpuApi::SetupRenderTargets( rtSetupMain );

		// Build renderingcontext
		RenderingContext rc( info.m_camera );
		rc.m_terrainToolStampVisible = false;
		rc.m_materialDebugMode = MDM_None;
		rc.m_pass = RP_NoLighting;
		rc.m_forceNoDissolves = true;

		// Render skybox
		scene->GetSkybox()->Render( info, rc, meshDrawingStats, true, true );
	}

	// Render deferred color for given face
	{
		GetRenderer()->RenderTiledDeferredEnvProbe( info, envProbe, texRenderTarget, texGBuffer0, texGBuffer1, texTempShadowMask, texDepthBuffer, faceIndex );
	}

	// Update surfaces guard
	renderer.SetupSurfacesAvailabilityGuard( true );
}


