///////////////////////////////////////////////////////////////////////
//  MyRenderFunctions.cpp
//
//	All source files prefixed with "My" indicate an example implementation, 
//	meant to detail what an application might (and, in most cases, should)
//	do when interfacing with the SpeedTree SDK.  These files constitute 
//	the bulk of the SpeedTree Reference Application and are not part
//	of the official SDK, but merely example or "reference" implementations.
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////
//  Preprocessor

#include "MyApplication.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////
//  CMyApplication::Render

void CMyApplication::Render(void)
{
    ScopeTrace("MyApp::Render");

	// lighting
	UpdateTimeOfDay( );

	// take deferred- or forward-rendering path
    if (m_cConfigFile.m_sDeferredRender.m_bEnabled)
    {
		DeferredRender( );
    }
    else
    {
		if (m_cConfigFile.m_sLighting.m_sBloom.m_bEnabled && m_nBloomDisplayMode != BLOOM_DISPLAY_MODE_NONE)
		{
			ForwardRenderWithBloom( );
		}
		else if (m_bExplicitMsaaResolveEnabled)
		{
			if (m_cConfigFile.m_sForwardRender.m_bDepthOnlyPrepass)
				ForwardRenderWithManualMsaaResolveWithDepthPrepass( );
			else
				ForwardRenderWithManualMsaaResolve( );
		}
		else
		{
			if (m_cConfigFile.m_sForwardRender.m_bDepthOnlyPrepass)
				ForwardRenderWithDepthPrepass( );
			else
				ForwardRender( );
		}
    }

	if (m_bRenderOverlays)
		(void) m_cOverlays.Render( );

	// frame's over; update a few things
	++m_nFrameIndex;
	m_cForest.GetRenderStats( ).m_nFrameCount += 1;
	m_bCameraChanged = false;
	
	// did we get any errors from SpeedTree or the graphics API?
	{
		PrintSpeedTreeErrors("End of CMyApplication::Render");

		#ifdef SPEEDTREE_OPENGL
			PrintOpenGLErrors("End of CMyApplication::Render");
		#endif
	}
}


///////////////////////////////////////////////////////////////////////
//  CMyApplication::DeferredRender

void CMyApplication::DeferredRender(void)
{
	// render into shadow maps
	if (m_cConfigFile.m_sShadows.m_bEnabled && (m_bCameraChanged || m_cForest.WindIsEnabled( )))
	{
		m_cForest.UpdateFrameConstantBuffer(m_cView, m_sCmdLine.m_nWindowWidth, m_sCmdLine.m_nWindowHeight);

		m_cForest.StartRender( );
		{
			// the shadow maps update at different intervals, but some events, like the camera
			// moving, should override the interval timing and update immediately
			const st_bool c_bForceRender = m_bCameraChanged;

			m_cShadowSystem.RenderIntoMaps(m_cConfigFile, 
										   m_cForest,
										   m_aGrassLayers,
										   m_cTerrain,
										   m_nFrameIndex, 
										   m_bRenderTrees, 
										   c_bForceRender);
		}
		m_cForest.EndRender( );
	}
	else if (!m_cConfigFile.m_sShadows.m_bEnabled)
	{
		m_cShadowSystem.BindBlankShadowsAsSourceTextures( );
	}

	// clear & bind lighting render targets
	if (m_cDeferredTarget.SetAsTarget( ))
	{
		m_cDeferredTarget.Clear(Vec4(m_cConfigFile.m_sFog.m_vColor, 1.0f));

		// set the shader view parameters for the main view
		m_cForest.UpdateFrameConstantBuffer(m_cView, m_sCmdLine.m_nWindowWidth, m_sCmdLine.m_nWindowHeight);

		// start the forest render
		m_cForest.StartRender( );
		{
			// terrain
			m_cTerrain.Render(RENDER_PASS_MAIN,
							  m_sTerrainCullResults,
							  m_cForest.GetRenderInfo( ).m_sLightMaterial, 
							  m_cForest.GetRenderStats( ));

			// render speedtree geometry
			EnableTextureAlphaMode( );
			{
				// 3d trees
				if (m_bRenderTrees)
					(void) m_cForest.Render3dTrees(RENDER_PASS_MAIN, m_cVisibleTreesFromCamera);

				// billboards
				if (m_bRenderBillboards)
					(void) m_cForest.RenderBillboards(RENDER_PASS_MAIN, m_cVisibleTreesFromCamera);

				// grass
				if (m_bRenderGrass)
					for (st_int32 nLayer = 0; nLayer < st_int32(m_aGrassLayers.size( )); ++nLayer)
						(void) m_cForest.RenderGrass(RENDER_PASS_MAIN, m_aGrassLayers[nLayer].GetBaseGrass( ), m_aGrassLayers[nLayer].GetVisibleFromMainCamera( ));
			}
		}
		m_cForest.EndRender( );
		m_cDeferredTarget.ReleaseAsTarget( );

		bool bContinue = true;
		if (m_cConfigFile.m_sDeferredRender.m_bDistanceBlur && m_bDepthBlur)
		{
			bContinue &= m_cDepthBlurTarget.SetAsTarget( );
			if (bContinue)
				m_cDepthBlurTarget.Clear(Vec4(0.0f, 0.0f, 0.0f, 1.0f));
		}

		bContinue &= m_cDeferredTarget.BindAsTextures( );

		// render lighting pass now that render targets are complete
		if (bContinue)
		{
			// bind shadow maps and setup comparison samplers
			if (m_cConfigFile.m_sShadows.m_bEnabled)
				(void) m_cShadowSystem.BindAsSourceTextures(m_cConfigFile, TEXTURE_REGISTER_SHADOW_MAP_0);
			else
				m_cShadowSystem.BindBlankShadowsAsSourceTextures( );

			(void) m_cDeferredRenderResolveQuad.Render(m_cDeferredState);

			(void) m_cDeferredTarget.UnBindAsTextures( );

			if (m_cConfigFile.m_sShadows.m_bEnabled)
				m_cShadowSystem.UnBindAsSourceTextures(m_cConfigFile, TEXTURE_REGISTER_SHADOW_MAP_0);

			if (m_cConfigFile.m_sDeferredRender.m_bDistanceBlur && m_bDepthBlur)
			{
				m_cDepthBlurTarget.ReleaseAsTarget( );
				if (m_cDeferredTarget.BindAsTextures( ) && m_cDepthBlurTarget.BindAsTexture(0))
				{
					(void) m_cDepthBlurQuad.Render(m_cDepthBlurState);

					(void) m_cDepthBlurTarget.UnBindAsTexture(0);
					(void) m_cDeferredTarget.UnBindAsTextures( );
				}
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////
//  CMyApplication::ForwardRender

void CMyApplication::ForwardRender(void)
{
	m_cForest.StartRender( );

	// render into shadow maps
	if (m_cConfigFile.m_sShadows.m_bEnabled && (m_bCameraChanged || m_cForest.WindIsEnabled( )))
	{
		// the shadow maps update at different intervals, but some events, like the camera
		// moving, should override the interval timing and update immediately
		const st_bool c_bForceRender = m_bCameraChanged;

		m_cShadowSystem.RenderIntoMaps(m_cConfigFile,
										m_cForest,
										m_aGrassLayers,
										m_cTerrain,
										m_nFrameIndex,
										m_bRenderTrees,
										c_bForceRender);
	}

	// set the shader view parameters for the main view
	m_cForest.UpdateFrameConstantBuffer(m_cView, m_sCmdLine.m_nWindowWidth, m_sCmdLine.m_nWindowHeight);

	// bind shadow maps and setup comparison samplers
	if (m_cConfigFile.m_sShadows.m_bEnabled)
		(void) m_cShadowSystem.BindAsSourceTextures(m_cConfigFile, TEXTURE_REGISTER_SHADOW_MAP_0);
	else
		m_cShadowSystem.BindBlankShadowsAsSourceTextures( );
        
	// terrain
	m_cTerrain.Render(RENDER_PASS_MAIN,
					  m_sTerrainCullResults,
					  m_cForest.GetRenderInfo( ).m_sLightMaterial, 
					  m_cForest.GetRenderStats( ));

	// render speedtree geometry
	EnableTextureAlphaMode( );
	{
		// 3d trees
		if (m_bRenderTrees)
			(void) m_cForest.Render3dTrees(RENDER_PASS_MAIN, m_cVisibleTreesFromCamera);

		// billboards
		if (m_bRenderBillboards)
			(void) m_cForest.RenderBillboards(RENDER_PASS_MAIN, m_cVisibleTreesFromCamera);

		// grass
		if (m_bRenderGrass)
			for (st_int32 nLayer = 0; nLayer < st_int32(m_aGrassLayers.size( )); ++nLayer)
				(void) m_cForest.RenderGrass(RENDER_PASS_MAIN, m_aGrassLayers[nLayer].GetBaseGrass( ), m_aGrassLayers[nLayer].GetVisibleFromMainCamera( ));
	}

	if (m_cConfigFile.m_sShadows.m_bEnabled)
		m_cShadowSystem.UnBindAsSourceTextures(m_cConfigFile, TEXTURE_REGISTER_SHADOW_MAP_0);

	m_cSky.Render(m_cForest.GetRenderStats( ));

	m_cForest.EndRender( );
}


///////////////////////////////////////////////////////////////////////
//  CMyApplication::ForwardRenderWithBloom

void CMyApplication::ForwardRenderWithBloom(void)
{
	m_cForest.StartRender( );

	// render into shadow maps
	if (m_cConfigFile.m_sShadows.m_bEnabled && (m_bCameraChanged || m_cForest.WindIsEnabled( )))
	{
		// the shadow maps update at different intervals, but some events, like the camera
		// moving, should override the interval timing and update immediately
		const st_bool c_bForceRender = m_bCameraChanged;

		m_cShadowSystem.RenderIntoMaps(m_cConfigFile, 
										m_cForest,
										m_aGrassLayers,
										m_cTerrain,
										m_nFrameIndex, 
										m_bRenderTrees, 
										c_bForceRender);
	}

	// set the main render target
	m_cBloomForwardTarget.SetAsTarget( );
	m_cBloomForwardTarget.Clear(Vec4(0.0f, 0.0f, 0.0f, 1.0f));

	// set the shader view parameters for the main view
	m_cForest.UpdateFrameConstantBuffer(m_cView, m_sCmdLine.m_nWindowWidth, m_sCmdLine.m_nWindowHeight);

	// bind shadow maps and setup comparison samplers
	if (m_cConfigFile.m_sShadows.m_bEnabled)
		(void) m_cShadowSystem.BindAsSourceTextures(m_cConfigFile, TEXTURE_REGISTER_SHADOW_MAP_0);
	else
		m_cShadowSystem.BindBlankShadowsAsSourceTextures( );

	// render speedtree geometry
	EnableTextureAlphaMode( );
	{
		// 3d trees
		if (m_bRenderTrees)
			(void) m_cForest.Render3dTrees(RENDER_PASS_MAIN, m_cVisibleTreesFromCamera);

		// billboards
		if (m_bRenderBillboards)
			(void) m_cForest.RenderBillboards(RENDER_PASS_MAIN, m_cVisibleTreesFromCamera);

		// grass
		if (m_bRenderGrass)
			for (st_int32 nLayer = 0; nLayer < st_int32(m_aGrassLayers.size( )); ++nLayer)
				(void) m_cForest.RenderGrass(RENDER_PASS_MAIN, m_aGrassLayers[nLayer].GetBaseGrass( ), m_aGrassLayers[nLayer].GetVisibleFromMainCamera( ));
	}

	// it's a bit of a hack, but we use the alpha channel to control how the sky interacts with the bloom effect;
	// because the alpha channel plays a role, we have to wipe out the alpha channel used by the vegetation before
	// the sky is rendered; this call will update the view matrices to flat/fullscreen
	(void) m_cFullscreenAlpaQuad.Render(m_cFullscreenAlpaState);

	// restore the 3D view matrices before rendering the sky
	m_cForest.UpdateFrameConstantBuffer(m_cView, m_sCmdLine.m_nWindowWidth, m_sCmdLine.m_nWindowHeight);

	// render terrain
	m_cTerrain.Render(RENDER_PASS_MAIN,
					  m_sTerrainCullResults,
					  m_cForest.GetRenderInfo( ).m_sLightMaterial, 
					  m_cForest.GetRenderStats( ));

	// don't need the shadow maps as read textures any longer
	if (m_cConfigFile.m_sShadows.m_bEnabled)
		m_cShadowSystem.UnBindAsSourceTextures(m_cConfigFile, TEXTURE_REGISTER_SHADOW_MAP_0);

	// render sky -- alpha channel is important to bloom approach
	m_cSky.Render(m_cForest.GetRenderStats( ));

	// main pass complete
	m_cForest.EndRender( );

	// release forward render target (color & depth targets)
	m_cBloomForwardTarget.ReleaseAsTarget( );

	// necessary for MSAA render target to resolve to a non-MSAA texture
	(void) m_cBloomForwardTarget.ResolveToTexture( );

	StartBloomPostPass( );
	{
		// render bloom high-pass
		{
			// set single color target
			m_cBloomAuxTargetA.SetAsTarget( );
			m_cBloomAuxTargetA.Clear(Vec4(0.0f, 0.0f, 1.0f, 1.0f)); // todo: black or not at all?

			// bind forward target as textures to be read by high-pass shader
			m_cBloomForwardTarget.GetNonMsaaTarget( ).BindAsTexture(0, false);

			// set view parameters (smaller target)
			m_cForest.UpdateFrameConstantBuffer(m_cView, m_sCmdLine.m_nWindowWidth / m_cConfigFile.m_sLighting.m_sBloom.m_nDownsample, 
														  m_sCmdLine.m_nWindowHeight / m_cConfigFile.m_sLighting.m_sBloom.m_nDownsample);

			// render fullscreen quad with high pass bloom shader
			(void) m_cBloomHiPassQuad.Render(m_cBloomHiPassState);

			// release render target
			m_cBloomAuxTargetA.ReleaseAsTarget( );
		}

		// render bloom blur pass
		{
			// set single color target
			if (m_nBloomDisplayMode == BLOOM_DISPLAY_MODE_FULL)
			{
				m_cBloomAuxTargetB.SetAsTarget( );
				// todo: doesn't setastarget clear if requested? should probably separate them
				m_cBloomAuxTargetB.Clear(Vec4(0.0f, 0.0f, 1.0f, 1.0f)); // todo: black or not at all?
			}

			// bind forward target as textures to be read by blur shader
			m_cBloomAuxTargetA.BindAsTexture(0, false); // false = use linear filtering in DX9

			// set view parameters (smaller target)
			m_cForest.UpdateFrameConstantBuffer(m_cView, m_sCmdLine.m_nWindowWidth / m_cConfigFile.m_sLighting.m_sBloom.m_nDownsample, 
														  m_sCmdLine.m_nWindowHeight / m_cConfigFile.m_sLighting.m_sBloom.m_nDownsample);

			// render fullscreen quad with blur bloom shader
			(void) m_cBloomBlurQuad.Render(m_cBloomBlurState);

			// release render target
			if (m_nBloomDisplayMode == BLOOM_DISPLAY_MODE_FULL)
				m_cBloomAuxTargetB.ReleaseAsTarget( );
		}

		// bloom final/combine pass to screen; screen should be the current render target after the last target release
		if (m_nBloomDisplayMode == BLOOM_DISPLAY_MODE_FULL)
		{
			// set previous targets as textures
			m_cBloomForwardTarget.GetNonMsaaTarget( ).BindAsTexture(0);
			m_cBloomAuxTargetB.BindAsTexture(1, false); // false = use linear filtering in DX9

			// make sure shader constants are set
			m_cForest.UpdateFrameConstantBuffer(m_cView, m_sCmdLine.m_nWindowWidth, m_sCmdLine.m_nWindowHeight);

			// render fullscreen quad to screen device
			(void) m_cBloomFinalQuad.Render(m_cBloomFinalState);
		}
	}
	EndBloomPostPass( );
}


///////////////////////////////////////////////////////////////////////
//  CMyApplication::StartBloomPostPass

void CMyApplication::StartBloomPostPass(void)
{
	// platform-specific settings
	#ifdef SPEEDTREE_DIRECTX9
		for (st_int32 i = 0; i < 3; ++i)
		{
			DX9::Device( )->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
			DX9::Device( )->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		}
	#endif
}


///////////////////////////////////////////////////////////////////////
//  CMyApplication::EndBloomPostPass

void CMyApplication::EndBloomPostPass(void)
{
	// platform-specific settings
	#ifdef SPEEDTREE_DIRECTX9
		for (st_int32 i = 0; i < 3; ++i)
		{
			DX9::Device( )->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
			DX9::Device( )->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		}
	#endif
}


///////////////////////////////////////////////////////////////////////
//  CMyApplication::ForwardRenderWithDepthPrepass

void CMyApplication::ForwardRenderWithDepthPrepass(void)
{
	m_cForest.StartRender( );

	// render into shadow maps
	if (m_cConfigFile.m_sShadows.m_bEnabled && (m_bCameraChanged || m_cForest.WindIsEnabled( )))
	{
		// the shadow maps update at different intervals, but some events, like the camera
		// moving, should override the interval timing and update immediately
		const st_bool c_bForceRender = m_bCameraChanged;

		m_cShadowSystem.RenderIntoMaps(m_cConfigFile, 
										m_cForest,
										m_aGrassLayers,
										m_cTerrain,
										m_nFrameIndex, 
										m_bRenderTrees, 
										c_bForceRender);
	}

	// set the shader view parameters for the main view
	m_cForest.UpdateFrameConstantBuffer(m_cView, m_sCmdLine.m_nWindowWidth, m_sCmdLine.m_nWindowHeight);

	// depth-only prepass
	{
		// terrain
		m_cTerrain.Render(RENDER_PASS_DEPTH_PREPASS, 
						  m_sTerrainCullResults,
						  m_cForest.GetRenderInfo( ).m_sLightMaterial, 
						  m_cForest.GetRenderStats( ));

		// render speedtree geometry
		EnableTextureAlphaMode( );
		{
			// 3d trees
			if (m_bRenderTrees)
				(void) m_cForest.Render3dTrees(RENDER_PASS_DEPTH_PREPASS, m_cVisibleTreesFromCamera, true);

			// billboards
			if (m_bRenderBillboards)
				(void) m_cForest.RenderBillboards(RENDER_PASS_DEPTH_PREPASS, m_cVisibleTreesFromCamera);

			// grass
			if (m_bRenderGrass)
				for (st_int32 nLayer = 0; nLayer < st_int32(m_aGrassLayers.size( )); ++nLayer)
					(void) m_cForest.RenderGrass(RENDER_PASS_DEPTH_PREPASS, m_aGrassLayers[nLayer].GetBaseGrass( ), m_aGrassLayers[nLayer].GetVisibleFromMainCamera( ));
		}
	}

	// main lighting pass
	{
		// bind shadow maps and setup comparison samplers
		if (m_cConfigFile.m_sShadows.m_bEnabled)
			(void) m_cShadowSystem.BindAsSourceTextures(m_cConfigFile, TEXTURE_REGISTER_SHADOW_MAP_0);
		else
			m_cShadowSystem.BindBlankShadowsAsSourceTextures( );

		// terrain
		m_cTerrain.Render(RENDER_PASS_MAIN,
						  m_sTerrainCullResults,
						  m_cForest.GetRenderInfo( ).m_sLightMaterial, 
						  m_cForest.GetRenderStats( ));

		// render speedtree geometry
		EnableTextureAlphaMode( );
		{
			// 3d trees
			if (m_bRenderTrees)
				(void) m_cForest.Render3dTrees(RENDER_PASS_MAIN, m_cVisibleTreesFromCamera);

			// billboards
			if (m_bRenderBillboards)
				(void) m_cForest.RenderBillboards(RENDER_PASS_MAIN, m_cVisibleTreesFromCamera);

			// grass
			if (m_bRenderGrass)
				for (st_int32 nLayer = 0; nLayer < st_int32(m_aGrassLayers.size( )); ++nLayer)
					(void) m_cForest.RenderGrass(RENDER_PASS_MAIN, m_aGrassLayers[nLayer].GetBaseGrass( ), m_aGrassLayers[nLayer].GetVisibleFromMainCamera( ));
		}

		if (m_cConfigFile.m_sShadows.m_bEnabled)
			m_cShadowSystem.UnBindAsSourceTextures(m_cConfigFile, TEXTURE_REGISTER_SHADOW_MAP_0);

		m_cSky.Render(m_cForest.GetRenderStats( ));
	}

	m_cForest.EndRender( );
}


///////////////////////////////////////////////////////////////////////
//  CMyApplication::ForwardRenderWithManualMsaaResolve

void CMyApplication::ForwardRenderWithManualMsaaResolve(void)
{
	m_cForest.StartRender( );

	// render into shadow maps
	if (m_cConfigFile.m_sShadows.m_bEnabled && (m_bCameraChanged || m_cForest.WindIsEnabled( )))
	{
		// the shadow maps update at different intervals, but some events, like the camera
		// moving, should override the interval timing and update immediately
		const st_bool c_bForceRender = m_bCameraChanged;

		m_cShadowSystem.RenderIntoMaps(m_cConfigFile, 
										m_cForest,
										m_aGrassLayers,
										m_cTerrain,
										m_nFrameIndex, 
										m_bRenderTrees, 
										c_bForceRender);
	}

	// set the main render target
	m_cExplicitMsaaResolveTarget.SetAsTarget( );
	m_cExplicitMsaaResolveTarget.Clear(Vec4(0.0f, 0.0f, 0.0f, 1.0f));

	// set the shader view parameters for the main view
	m_cForest.UpdateFrameConstantBuffer(m_cView, m_sCmdLine.m_nWindowWidth, m_sCmdLine.m_nWindowHeight);

	// bind shadow maps and setup comparison samplers
	if (m_cConfigFile.m_sShadows.m_bEnabled)
		(void) m_cShadowSystem.BindAsSourceTextures(m_cConfigFile, TEXTURE_REGISTER_SHADOW_MAP_0);
	else
		m_cShadowSystem.BindBlankShadowsAsSourceTextures( );

	// terrain
	m_cTerrain.Render(RENDER_PASS_MAIN,
					  m_sTerrainCullResults,
					  m_cForest.GetRenderInfo( ).m_sLightMaterial, 
					  m_cForest.GetRenderStats( ));

	// render speedtree geometry
	EnableTextureAlphaMode( );
	{
		// 3d trees
		if (m_bRenderTrees)
			(void) m_cForest.Render3dTrees(RENDER_PASS_MAIN, m_cVisibleTreesFromCamera);

		// billboards
		if (m_bRenderBillboards)
			(void) m_cForest.RenderBillboards(RENDER_PASS_MAIN, m_cVisibleTreesFromCamera);

		// grass
		if (m_bRenderGrass)
			for (st_int32 nLayer = 0; nLayer < st_int32(m_aGrassLayers.size( )); ++nLayer)
				(void) m_cForest.RenderGrass(RENDER_PASS_MAIN, m_aGrassLayers[nLayer].GetBaseGrass( ), m_aGrassLayers[nLayer].GetVisibleFromMainCamera( ));
	}

	if (m_cConfigFile.m_sShadows.m_bEnabled)
		m_cShadowSystem.UnBindAsSourceTextures(m_cConfigFile, TEXTURE_REGISTER_SHADOW_MAP_0);

	m_cSky.Render(m_cForest.GetRenderStats( ));

	m_cForest.EndRender( );

	// set render back to device now
	m_cExplicitMsaaResolveTarget.ReleaseAsTarget( );

	// resolve MSAA render to a single non-MSAA texture
	if (m_cExplicitMsaaResolveTarget.ResolveToTexture( ))
	{
		// bind non-MSAA texture
		if (m_cExplicitMsaaResolveTarget.GetNonMsaaTarget( ).BindAsTexture(0))
		{
			// make sure shader constants are set
			m_cForest.UpdateFrameConstantBuffer(m_cView, m_sCmdLine.m_nWindowWidth, m_sCmdLine.m_nWindowHeight);

			// render non-MSAA texture as fullscreen quad
			(void) m_cExplicitMsaaResolveQuad.Render(m_cExplicitMsaaResolveState);

			// complete
			m_cExplicitMsaaResolveTarget.GetNonMsaaTarget( ).UnBindAsTexture(0);
		}
	}
}


///////////////////////////////////////////////////////////////////////
//  CMyApplication::ForwardRenderWithManualMsaaResolveWithDepthPrepass

void CMyApplication::ForwardRenderWithManualMsaaResolveWithDepthPrepass(void)
{
	m_cForest.StartRender( );

	// render into shadow maps
	if (m_cConfigFile.m_sShadows.m_bEnabled && (m_bCameraChanged || m_cForest.WindIsEnabled( )))
	{
		// the shadow maps update at different intervals, but some events, like the camera
		// moving, should override the interval timing and update immediately
		const st_bool c_bForceRender = m_bCameraChanged;

		m_cShadowSystem.RenderIntoMaps(m_cConfigFile, 
										m_cForest,
										m_aGrassLayers,
										m_cTerrain,
										m_nFrameIndex, 
										m_bRenderTrees, 
										c_bForceRender);
	}

	// set the main render target
	m_cExplicitMsaaResolveTarget.SetAsTarget( );
	m_cExplicitMsaaResolveTarget.Clear(Vec4(0.0f, 0.0f, 0.0f, 1.0f));
	
	// set the shader view parameters for the main view
	m_cForest.UpdateFrameConstantBuffer(m_cView, m_sCmdLine.m_nWindowWidth, m_sCmdLine.m_nWindowHeight);

	// depth-only prepass
	{
		// terrain
		m_cTerrain.Render(RENDER_PASS_DEPTH_PREPASS, 
						  m_sTerrainCullResults,
						  m_cForest.GetRenderInfo( ).m_sLightMaterial, 
						  m_cForest.GetRenderStats( ));

		// render speedtree geometry
		EnableTextureAlphaMode( );
		{
			// 3d trees
			if (m_bRenderTrees)
				(void) m_cForest.Render3dTrees(RENDER_PASS_DEPTH_PREPASS, m_cVisibleTreesFromCamera, true);

			// billboards
			if (m_bRenderBillboards)
				(void) m_cForest.RenderBillboards(RENDER_PASS_DEPTH_PREPASS, m_cVisibleTreesFromCamera);

			// grass
			if (m_bRenderGrass)
				for (st_int32 nLayer = 0; nLayer < st_int32(m_aGrassLayers.size( )); ++nLayer)
					(void) m_cForest.RenderGrass(RENDER_PASS_DEPTH_PREPASS, m_aGrassLayers[nLayer].GetBaseGrass( ), m_aGrassLayers[nLayer].GetVisibleFromMainCamera( ));
		}
	}
	
	// main lighting pass
	{
		// bind shadow maps and setup comparison samplers
		if (m_cConfigFile.m_sShadows.m_bEnabled)
			(void) m_cShadowSystem.BindAsSourceTextures(m_cConfigFile, TEXTURE_REGISTER_SHADOW_MAP_0);
		else
			m_cShadowSystem.BindBlankShadowsAsSourceTextures( );

		// terrain
		m_cTerrain.Render(RENDER_PASS_MAIN,
						  m_sTerrainCullResults,
						  m_cForest.GetRenderInfo( ).m_sLightMaterial, 
						  m_cForest.GetRenderStats( ));

		// render speedtree geometry
		EnableTextureAlphaMode( );
		{
			// 3d trees
			if (m_bRenderTrees)
				(void) m_cForest.Render3dTrees(RENDER_PASS_MAIN, m_cVisibleTreesFromCamera);

			// billboards
			if (m_bRenderBillboards)
				(void) m_cForest.RenderBillboards(RENDER_PASS_MAIN, m_cVisibleTreesFromCamera);

			// grass
			if (m_bRenderGrass)
				for (st_int32 nLayer = 0; nLayer < st_int32(m_aGrassLayers.size( )); ++nLayer)
					(void) m_cForest.RenderGrass(RENDER_PASS_MAIN, m_aGrassLayers[nLayer].GetBaseGrass( ), m_aGrassLayers[nLayer].GetVisibleFromMainCamera( ));
		}

		if (m_cConfigFile.m_sShadows.m_bEnabled)
			m_cShadowSystem.UnBindAsSourceTextures(m_cConfigFile, TEXTURE_REGISTER_SHADOW_MAP_0);

		m_cSky.Render(m_cForest.GetRenderStats( ));
	}
	
	m_cForest.EndRender( );

	// set render back to device now
	m_cExplicitMsaaResolveTarget.ReleaseAsTarget( );

	// resolve MSAA render to a single non-MSAA texture
	if (m_cExplicitMsaaResolveTarget.ResolveToTexture( ))
	{
		// bind non-MSAA texture
		if (m_cExplicitMsaaResolveTarget.GetNonMsaaTarget( ).BindAsTexture(0))
		{
			// make sure shader constants are set
			m_cForest.UpdateFrameConstantBuffer(m_cView, m_sCmdLine.m_nWindowWidth, m_sCmdLine.m_nWindowHeight);

			// render non-MSAA texture as fullscreen quad
			(void) m_cExplicitMsaaResolveQuad.Render(m_cExplicitMsaaResolveState);

			// complete
			m_cExplicitMsaaResolveTarget.GetNonMsaaTarget( ).UnBindAsTexture(0);
		}
	}
}


///////////////////////////////////////////////////////////////////////
//  CMyApplication::EnableTextureAlphaMode

void CMyApplication::EnableTextureAlphaMode(void)
{
	if (m_eTransparentTextureRenderMode == TRANS_TEXTURE_ALPHA_TO_COVERAGE)
		SetTextureAlphaScalars(1.0f, 1.0f, 1.0f);
	else
		SetTextureAlphaScalars(m_cConfigFile.m_sWorld.m_fAlphaTestScalar3d,
							   m_cConfigFile.m_sWorld.m_fAlphaTestScalarGrass,
							   m_cConfigFile.m_sWorld.m_fAlphaTestScalarBillboards);
}


///////////////////////////////////////////////////////////////////////
//  CMyApplication::SetTextureAlphaScalars

void CMyApplication::SetTextureAlphaScalars(st_float32 fScalar3d, st_float32 fScalarGrass, st_float32 fScalarBillboards)
{
	SForestRenderInfo sInfo = m_cForest.GetRenderInfo( );
	sInfo.m_fTextureAlphaScalar3d = fScalar3d;
	sInfo.m_fTextureAlphaScalarGrass = fScalarGrass;
	sInfo.m_fTextureAlphaScalarBillboards = fScalarBillboards;
	m_cForest.SetRenderInfo(sInfo);
}
