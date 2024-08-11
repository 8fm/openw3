
#include "renderCollector.h"
#include "../engine/environmentManager.h"
#include "../engine/mesh.h"
#include "renderMesh.h"
#include "renderScene.h"
#include "renderElementMap.h"
#include "renderProxyMesh.h"
#include "../engine/renderFragment.h"

namespace Debug
{
	extern Int32			GRenderTargetZoomShift;
	extern Int32			GRenderTargetOffsetX;
	extern Int32			GRenderTargetOffsetY;
}

/// Don't use this class to anything beyond debug renders. Implementation may be suboptimal in many cases.
class CPostFxDebug
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );

private:
	struct SZoomState
	{
		SZoomState ()
		{
			div			= 1;
			offsetX		= 0;
			offsetY		= 0;
		}

		bool operator!=( const SZoomState &other ) const { return !operator==( other ); }
		bool operator==( const SZoomState &other ) const
		{
			return div == other.div && offsetX == other.offsetX && offsetY == other.offsetY;
		}

		Float div; //< div instead of mul for better pixel perfectness
		Float offsetX;
		Float offsetY;
	};

	struct SZoomData
	{
		SZoomData ()
		{
			lastChangeTime = -1;
		}

		Bool IsActive() const
		{
			return currState.div > 1 || 0 != currState.offsetX || 0 != currState.offsetY || Debug::GRenderTargetZoomShift > 0;
		}

		void Update( const CRenderFrameInfo &info )
		{
			SZoomState newEndState;
			newEndState.div = (Float) (1 << Debug::GRenderTargetZoomShift);
			newEndState.offsetX = (Float) Debug::GRenderTargetOffsetX;
			newEndState.offsetY = (Float) Debug::GRenderTargetOffsetY;

			const Float currTime = info.m_engineTime;
			if ( newEndState != endState )
			{
				startState = currState;
				endState = newEndState;
				lastChangeTime = currTime;
			}
			else if ( -1 != lastChangeTime )
			{
				const Float t = Clamp( (currTime - lastChangeTime) / 0.075f, 0.f, 1.f );
				if ( t < 1.f )
				{
					currState.div = 1.f / Lerp( t, 1.f / startState.div, 1.f / endState.div ); 
					currState.offsetX = Lerp( t, startState.offsetX, endState.offsetX ); 
					currState.offsetY = Lerp( t, startState.offsetY, endState.offsetY ); 
				}
				else
				{
					// make exact (Lerp doesn't guarantee this)
					currState = endState;
				}
			}
		}

		Float lastChangeTime;
		SZoomState startState;
		SZoomState currState;
		SZoomState endState;
	};

	static SZoomData& RefZoomData()
	{
		static SZoomData data;
		return data;
	}

private:
	static Vector BuildChannelMask( Int32 channelIndex )
	{
		return Vector ( 0 == channelIndex ? 1.f : 0.f, 1 == channelIndex ? 1.f : 0.f, 2 == channelIndex ? 1.f : 0.f, 3 == channelIndex  ? 1.f : 0.f );
	}

	static Vector BuildZoomParams( const CRenderFrameInfo &info, Bool allowZoom )
	{
		// Always update the zoom to stabilize the zooming, 
		// in case zoom was switched off during transition etc.
		SZoomData &zoom = RefZoomData();
		zoom.Update( info );

		if ( allowZoom )
		{
			return Vector ( Max( 1.f, zoom.currState.div ), zoom.currState.offsetX, zoom.currState.offsetY, 0 );
		}

		return Vector ( 1, 0, 0, 0 );
	}

public:
	static Bool IsActivated( const CRenderFrameInfo &info, Bool allowZoom )
	{
	#ifdef RED_FINAL_BUILD
		{
			return false;
		}
	#else
		{
			const Bool needsSpecialDisplay = info.IsShowFlagOn( SHOW_Wireframe ) ||
				( EMM_None		!= info.m_envParametersGame.m_displaySettings.m_displayMode &&
				  EMM_LinearAll	!= info.m_envParametersGame.m_displaySettings.m_displayMode );

			return (needsSpecialDisplay || (allowZoom && RefZoomData().IsActive())) && info.m_allowPostSceneRender;
		}
	#endif
	}

public:
	CPostFxDebug ()
	{
	}

	~CPostFxDebug ()
	{
	}
		
	void ApplyDecodeChannel( 
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		GpuApi::TextureRef sampleTexture, 
		Int32 channelIndex, Float gamma, Bool allowZoom,
		GpuApi::eDrawContext drawContext = GpuApi::DRAWCONTEXT_PostProcSet )
	{
		ApplyDecodeColor( info, drawer, sampleTexture, channelIndex, channelIndex, channelIndex, gamma, allowZoom, drawContext );
	}

	void ApplyDecodeRGB( 
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		GpuApi::TextureRef sampleTexture, 
		Float gamma, Bool allowZoom,
		GpuApi::eDrawContext drawContext = GpuApi::DRAWCONTEXT_PostProcSet )
	{
		ApplyDecodeColor( info, drawer, sampleTexture, 0, 1, 2, gamma, allowZoom, drawContext );
	}

	void ApplyDecodeColor( 
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		GpuApi::TextureRef sampleTexture, 
		Int32 channelRed, Int32 channelGreen, Int32 channelBlue, Float gamma, Bool allowZoom,
		GpuApi::eDrawContext drawContext = GpuApi::DRAWCONTEXT_PostProcSet )
	{	
		CRenderShaderPair* shader = GetRenderer()->m_postfxDebug;
		CGpuApiScopedDrawContext scopedDrawContext( drawContext );

		// Set textures
		ASSERT ( sampleTexture );
		GpuApi::BindTextures( 0, 1, &sampleTexture, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Build some helper constant values
		const Int32 mode = 0;
		
		// Render
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0,	Vector ( (Float)mode, gamma, 0.f, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_1,	BuildZoomParams( info, allowZoom ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_2,	BuildChannelMask( channelRed ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_3,	BuildChannelMask( channelGreen ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_4,	BuildChannelMask( channelBlue ) );

		drawer.DrawQuad( shader );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}

	void ApplyDecodeGBufferInterleavedYCbCr( 
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		GpuApi::TextureRef sampleTexture, 
		Float gamma, Bool allowZoom,
		GpuApi::eDrawContext drawContext = GpuApi::DRAWCONTEXT_PostProcSet )
	{
		CRenderShaderPair* shader = GetRenderer()->m_postfxDebug;
		CGpuApiScopedDrawContext scopedDrawContext( drawContext );

		// Set textures
		ASSERT ( sampleTexture );
		GpuApi::BindTextures( 0, 1, &sampleTexture, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Build some helper constant values
		const Int32 mode = 10;
		
		// Render
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0,	Vector ( (Float)mode, gamma, 0.f, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_1,	BuildZoomParams( info, allowZoom ) );
		
		drawer.DrawQuad( shader );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}

	void ApplyNormalizedAlbedo( 
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		GpuApi::TextureRef sampleTextureAlbedo, 
		GpuApi::TextureRef sampleTextureSpecularity, 
		Float gamma, Bool allowZoom,
		GpuApi::eDrawContext drawContext = GpuApi::DRAWCONTEXT_PostProcSet )
	{
		CRenderShaderPair* shader = GetRenderer()->m_postfxDebug;
		CGpuApiScopedDrawContext scopedDrawContext( drawContext );

		// Set textures
		ASSERT ( sampleTextureAlbedo );
		ASSERT ( sampleTextureSpecularity );
		GpuApi::BindTextures( 0, 1, &sampleTextureAlbedo, GpuApi::PixelShader );
		GpuApi::BindTextures( 1, 1, &sampleTextureSpecularity, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Build some helper constant values
		const Int32 mode = 11;

		// Render
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0,	Vector ( (Float)mode, gamma, 0.f, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_1,	BuildZoomParams( info, allowZoom ) );

		drawer.DrawQuad( shader );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}

	void ApplyLightsOverlay( 
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		GpuApi::TextureRef sampleTexture, 
		Float gamma, Bool allowZoom,
		GpuApi::eDrawContext drawContext = GpuApi::DRAWCONTEXT_PostProcSet )
	{
		CRenderShaderPair* shader = GetRenderer()->m_postfxDebug;
		CGpuApiScopedDrawContext scopedDrawContext( drawContext );

		// Set textures
		GpuApi::TextureRef sampleTextures[3] = { GetRenderer()->GetSurfaces()->GetDepthBufferTex(), GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_GBuffer0 ), GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_GBuffer1 ) };
		GpuApi::BindTextures( 0, 3, &(sampleTextures[0]), GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 2, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Build some helper constant values
		const Int32 mode = 111;

		// Render
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0,	Vector ( (Float)mode, gamma, 0.f, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_1,	BuildZoomParams( info, allowZoom ) );

		drawer.DrawQuad( shader );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}
	void ApplyLightsOverlayDensity( 
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		GpuApi::TextureRef sampleTexture, 
		Float gamma, Bool allowZoom,
		GpuApi::eDrawContext drawContext = GpuApi::DRAWCONTEXT_PostProcSet )
	{
		CRenderShaderPair* shader = GetRenderer()->m_postfxDebug;
		CGpuApiScopedDrawContext scopedDrawContext( drawContext );

		// Set textures
		ASSERT ( sampleTexture );
		
		GpuApi::TextureRef sampleTextures[3] = { GetRenderer()->GetSurfaces()->GetDepthBufferTex(), GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_GBuffer0 ), GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_GBuffer1 ) };
		GpuApi::BindTextures( 0, 3, &(sampleTextures[0]), GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 2, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Build some helper constant values
		const Int32 mode = 112;

		// Render
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0,	Vector ( (Float)mode, gamma, 0.f, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_1,	BuildZoomParams( info, allowZoom ) );

		drawer.DrawQuad( shader );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}

	void ApplyDecodeNormals( 
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		GpuApi::TextureRef sampleTexture, 
		Float gamma, Bool allowZoom,
		Bool showIsViewSpace /*otherwise in worldSpace*/,
		GpuApi::eDrawContext drawContext = GpuApi::DRAWCONTEXT_PostProcSet )
	{
		CRenderShaderPair* shader = GetRenderer()->m_postfxDebug;
		CGpuApiScopedDrawContext scopedDrawContext( drawContext );

		// Set textures
		ASSERT ( sampleTexture );
		GpuApi::BindTextures( 0, 1, &sampleTexture, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Build some helper constant values
		const Int32 mode = showIsViewSpace ? 21 : 20;

		// Render
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0,	Vector ( (Float)mode, gamma, 0.f, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_1,	BuildZoomParams( info, allowZoom ) );

		drawer.DrawQuad( shader );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}

	void ApplyDifference( 
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		GpuApi::TextureRef sampleTextureLeft, GpuApi::TextureRef sampleTextureRight,
		Float texturesGamma, Float gamma, Bool allowZoom,
		GpuApi::eDrawContext drawContext = GpuApi::DRAWCONTEXT_PostProcSet )
	{	
		CRenderShaderPair* shader = GetRenderer()->m_postfxDebug;
		CGpuApiScopedDrawContext scopedDrawContext( drawContext );

		// Set textures
		ASSERT ( sampleTextureLeft );

		GpuApi::TextureRef sampleTextures[2] = { sampleTextureLeft, sampleTextureRight };

		GpuApi::BindTextures( 0, 2, &(sampleTextures[0]), GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Build some helper constant values
		const Int32 mode = 30;

		// Render
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0,	Vector ( (Float)mode, gamma, 0.f, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_1,	BuildZoomParams( info, allowZoom ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_2,	Vector ( texturesGamma, 0.f, 0.f, 0.f ) );

		drawer.DrawQuad( shader );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}

	void ApplyDimmers( 
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		Float gamma, Bool allowZoom,
		Bool isVolumeDisplay )
	{
		CRenderShaderPair* shader = GetRenderer()->m_postfxDebug;
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Set textures

		GpuApi::TextureRef sampleTextures[3] = { GetRenderer()->GetSurfaces()->GetDepthBufferTex(), GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_GBuffer0 ), GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_GBuffer1 ) };
		GpuApi::BindTextures( 0, 3, &(sampleTextures[0]), GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 2, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Build some helper constant values
		const Int32 mode = 40 + (isVolumeDisplay ? 1 : 0);

		// Render
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0,	Vector ( (Float)mode, gamma, 0.f, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_1,	BuildZoomParams( info, allowZoom ) );

		drawer.DrawQuad( shader );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}

	void ApplyComplexityEnvProbes( 
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		Bool allowZoom )
	{
		CRenderShaderPair* shader = GetRenderer()->m_postfxDebug;
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Build some helper constant values
		const Int32 mode = 50;

		// Bind textures
		GpuApi::TextureRef sampleTextures[2] = { GetRenderer()->GetSurfaces()->GetDepthBufferTex(), GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_GBuffer0 ) };
		GpuApi::BindTextures( 0, 2, &(sampleTextures[0]), GpuApi::PixelShader );

		// Render
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0,	Vector ( (Float)mode, 1, 0.f, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_1,	BuildZoomParams( info, allowZoom ) );

		//
		GetRenderer()->BindForwardConsts( info, GetRenderer()->GetGlobalCascadesShadowResources(), GetRenderer()->GetSurfaces(), true, GpuApi::PixelShader );
		// BindForwardConsts for VertexShader not needed here!
		
		drawer.DrawQuad( shader );

		GetRenderer()->UnbindForwardConsts( info, GpuApi::PixelShader );
		
		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}

	void ApplyShowStencilBits( 
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		const GpuApi::TextureRef &sampleTexture, 
		Uint32 stencilBitMask, 
		Bool allowZoom )
	{
		CRenderShaderPair* shader = GetRenderer()->m_postfxDebug;
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Set textures
		ASSERT ( sampleTexture );
		GpuApi::BindTextureStencil( 3, sampleTexture, GpuApi::PixelShader );
		
		// Build some helper constant values
		const Int32 mode = 60;

		// Render
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0,	Vector ( (Float)mode, 1.f, 0.f, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_1,	BuildZoomParams( info, allowZoom ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_2,	Vector ( (Float)stencilBitMask, 0, 0, 0 ) );
		
		drawer.DrawQuad( shader );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}

	void ApplyShowVolumeRendering( 
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		const GpuApi::TextureRef sampleTexture, CRenderCollector& collector,
		Bool isInteriorFactor )//, GpuApi::RenderTargetSetup postRtSetup )
	{	
		GpuApi::TextureRef depthTexture = GetRenderer()->GetSurfaces()->GetDepthBufferTex();
		GpuApi::TextureRef targetRef = GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_InteriorVolume );
		CRenderShaderPair* shader = GetRenderer()->m_postfxDebug;
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Set textures
		ASSERT ( sampleTexture );
		GpuApi::BindTextures( 0, 1, &sampleTexture, GpuApi::PixelShader );
		GpuApi::BindTextures( 1, 1, &targetRef, GpuApi::PixelShader );
		GpuApi::BindTextures( 2, 1, &depthTexture, GpuApi::PixelShader );

		// Build some helper constant values
		const Int32 mode = isInteriorFactor ? 71 : 70;

		// Render
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0,	Vector ( (Float)mode, 1.f, 0.f, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_1,	BuildZoomParams( info, true ) );		

		drawer.DrawQuad( shader );
		GpuApi::BindTextures( 0, 3, nullptr, GpuApi::PixelShader );
	}

	void ApplyShowShadowLods( 
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		CRenderCollector& collector
		)
	{	
#ifndef NO_EDITOR
		CRenderCollector::SRenderElementContainer elementCollector;// = new CRenderCollector::SRenderElementContainer;

// FIXME : The collection stuff was commented out. So this is really just a whole lot of wasted work right now.
// If someone wants to make the commented out bit work again, this can be re-enabled.
#if 0
		TDynArray< IRenderProxyDrawable* > allProxies;
		
		auto solidMeshes = collector.m_renderCollectorData->m_elements[ RPl_Scene ].m_elements[ RSG_LitOpaque ].m_solidMeshes;
		for ( auto chunk : solidMeshes )
		{
			allProxies.PushBackUnique( chunk->GetProxy() );
		}
		auto discardMeshes = collector.m_renderCollectorData->m_elements[ RPl_Scene ].m_elements[ RSG_LitOpaque ].m_discardMeshes;
		for ( auto chunk : discardMeshes )
		{
			allProxies.PushBackUnique( chunk->GetProxy() );
		}

		for ( auto proxy : allProxies )
		{
			RED_ASSERT( proxy == nullptr || proxy->GetType() == RPT_Mesh, TXT("Proxy is not a mesh") );
			if ( CRenderProxy_Mesh* s = static_cast<CRenderProxy_Mesh*>( proxy ) )
			{
				/*Uint8 bias = s->GetShadowLodBias();
				Uint8 lodIndex = s->GetLodIndex();
				lodIndex += bias;
				if ( lodIndex >= s->GetLodGroups().Size() ) lodIndex = s->GetLodGroups().Size() - 1;
				if ( lodIndex >= 0 && lodIndex < s->GetLodGroups().Size() )
				{
				s->CollectShadowLodsForced(lodIndex, elementCollector);
				}*/
			}
		}
#endif

		RenderingContext rc( info.m_camera );		
		rc.m_constantHitProxyID = Color( 255, 0, 0 );
		rc.m_useConstantHitProxyID = true;
		rc.m_pass = RP_HitProxies;
		rc.m_forceNoDissolves = true;
		GetRenderer()->GetStateManager().SetCamera( info.m_camera );		
				
		if ( !elementCollector.Empty() )
		{			
			GpuApi::SetCustomDrawContext(GpuApi::DSSM_NoStencilDepthTestLess, GpuApi::RASTERIZERMODE_DefaultCullCCW, GpuApi::BLENDMODE_Set);		
			elementCollector.Draw(collector, rc, RECG_ALL);
		}		

		//GetRenderer()->ClearColorTarget( GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_GBuffer0 ), Vector::ZEROS );		

#endif
	}

	void ApplyDecodeLocalReflection( 
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		GpuApi::TextureRef sampleTexture, 
		Float gamma, Bool allowZoom,
		GpuApi::eDrawContext drawContext = GpuApi::DRAWCONTEXT_PostProcSet )
	{	
		CRenderShaderPair* shader = GetRenderer()->m_postfxDebug;
		CGpuApiScopedDrawContext scopedDrawContext( drawContext );

		// Set textures
		ASSERT ( sampleTexture );
		GpuApi::BindTextures( 0, 1, &sampleTexture, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Build some helper constant values
		const Int32 mode = 80;

		Vector downsampleRatio ( 1.f / REALTIME_REFLECTIONS_SIZE_DIV, 1.f / REALTIME_REFLECTIONS_SIZE_DIV, (Float)REALTIME_REFLECTIONS_SIZE_DIV, (Float)REALTIME_REFLECTIONS_SIZE_DIV );
		#ifdef REALTIME_REFLECTIONS_FIXED_SIZE
		{
		# if REALTIME_REFLECTIONS_FIXED_SIZE
			const Uint32 rlrWidth	= REALTIME_REFLECTIONS_SIZE_WIDTH;
			const Uint32 rlrHeight	= REALTIME_REFLECTIONS_SIZE_HEIGHT;
			downsampleRatio.X = rlrWidth / (Float)Max<Uint32>(1,info.m_width);
			downsampleRatio.Y = rlrHeight / (Float)Max<Uint32>(1,info.m_height);
			downsampleRatio.Z = 1.f / Max( 0.0001f, downsampleRatio.X );
			downsampleRatio.W = 1.f / Max( 0.0001f, downsampleRatio.Y );
		# endif
		}
		#else
		# error Expected definition
		#endif
		
		// Render
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0,	Vector ( (Float)mode, gamma, 0.f, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_1,	BuildZoomParams( info, allowZoom ) );		
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_2,	downsampleRatio );		
		
		drawer.DrawQuad( shader );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}

	void ApplyShowDepth( 
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		Bool allowZoom,
		GpuApi::eDrawContext drawContext = GpuApi::DRAWCONTEXT_PostProcSet )
	{	
		CRenderShaderPair* shader = GetRenderer()->m_postfxDebug;
		CGpuApiScopedDrawContext scopedDrawContext( drawContext );

		// Set textures
		GpuApi::TextureRef sceneDepth = GetRenderer()->GetSurfaces()->GetDepthBufferTex();
		GpuApi::BindTextures( 0, 1, &sceneDepth, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Build some helper constant values
		const Int32 mode = 90;

		// Render
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0,	Vector ( (Float)mode, 1.0f, 0.f, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_1,	BuildZoomParams( info, allowZoom ) );		

		drawer.DrawQuad( shader );

		GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
	}

	void ApplyLightingAmbient(
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		Bool allowZoom,
		Float gamma )
	{
		CRenderShaderPair* shader = GetRenderer()->m_postfxDebug;
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Build some helper constant values
		const Int32 mode = 100;

		// Render
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0,	Vector ( (Float)mode, gamma, 0.f, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_1,	BuildZoomParams( info, allowZoom ) );		

		// Set textures
		GpuApi::TextureRef tex[] = { GetRenderer()->GetSurfaces()->GetDepthBufferTex(), GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_GBuffer1 ), GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_GlobalShadowAndSSAO ) };
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );

		drawer.DrawQuad( shader );

		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
	}

	void ApplyLightingReflection(
		const CRenderFrameInfo &info,
		CPostProcessDrawer &drawer,
		Bool allowZoom,
		Float gamma  )
	{
		CRenderShaderPair* shader = GetRenderer()->m_postfxDebug;
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Build some helper constant values
		const Int32 mode = 101;

		// Render
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0,	Vector ( (Float)mode, gamma, 0.f, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_1,	BuildZoomParams( info, allowZoom ) );		

		// Set textures
		GpuApi::TextureRef tex[] = { GetRenderer()->GetSurfaces()->GetDepthBufferTex(), GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_GBuffer1 ), GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_GlobalShadowAndSSAO )  };
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );

		drawer.DrawQuad( shader );

		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
	}
};
