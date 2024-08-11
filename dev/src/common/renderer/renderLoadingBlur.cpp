/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
* Kamil Nowakowski
*/

#include "build.h"

#include "../engine/renderCommands.h"
#include "../engine/loadingScreen.h"
#include "../engine/renderer.h"

#include "renderLoadingBlur.h"

#include "renderPostProcess.h"
using namespace PostProcessUtilities;

#include "renderPostFx.h"
#include "renderRenderSurfaces.h"

#include "../engine/viewport.h"

///////////////////////////////////////////////////////////////////////////////////////
//
//	Render commands and another spooky stuff
//
///////////////////////////////////////////////////////////////////////////////////////


CLoadingScreenBlur::CLoadingScreenBlur ( )
	: m_blurProgress( -0.2f )	// We need give 200ms latency before perform any blur
	, m_fadeScale( 1.0f )
	, m_fadeTime( 1.0f )
	, m_maxBluringTime( 2.0f )
	, m_flipSide( 0 )
	, m_blurScaleTempo( 2.0f )
	, m_blurSourceTempo( 4.0f )
	, m_sizeDiv( 8 )
	, m_viewport( nullptr )
	, m_fadingOut( false ) 
	, m_enabled( false )
	, m_refcount( 0 )
{
	m_temporalBuffer[0] = NULL;
	m_temporalBuffer[1] = NULL;
}


CLoadingScreenBlur::~CLoadingScreenBlur ()
{
	ReleaseTemporalBuffer();
}

void CLoadingScreenBlur::ResetBlurParametersToDefaults()
{
	m_blurProgress		= -0.2f;
	m_fadeScale			= 1.0f;
	m_fadeTime			= 1.0f;
	m_maxBluringTime	= 2.0f;
	m_flipSide			= 0;
	m_blurScaleTempo	= 2.0f;
	m_blurSourceTempo	= 4.0f;
	m_sizeDiv			= 8;
	m_viewport			= nullptr;
	m_fadingOut			= false;
	m_enabled			= false;
}

void CLoadingScreenBlur::Init( IViewport* viewport , Float blurScale , Float timeScale, Bool useFallback )
{
	m_refcount++;
	if ( m_refcount > 1 )
	{
		return;
	}

	// Release old buffers just for caution
	ReleaseTemporalBuffer();

	// Clear all values to default
	ResetBlurParametersToDefaults();

	m_viewport			= viewport;
	m_enabled			= true;
	m_blurScaleTempo	= blurScale;
	m_blurSourceTempo	= timeScale;
	m_useFallback		= useFallback;
}	

void CLoadingScreenBlur::Deinit( Float fadeTime )
{
	// If it was never launched, just pass off
	RED_ASSERT( m_refcount > 0 );
	m_refcount--;
	if ( m_refcount > 0 )
	{
		return;
	}

	// Is fading allowed
	m_fadingOut = fadeTime > 0.0f;

	// If there is no fade
	if( !m_fadingOut )
	{
		ReleaseTemporalBuffer();
	}

	m_enabled = false;
}

///////////////////////////////////////////////////////////////////////////////////////
//
//	Rendering stuff
//
///////////////////////////////////////////////////////////////////////////////////////

void CLoadingScreenBlur::PerformBlur( 
	CPostProcessDrawer &drawer,
	CRenderTarget* sourceColor,
	const CRenderFrameInfo& frameInfo
	)
{
	RED_FATAL_ASSERT( sourceColor != nullptr , "" );
	RED_FATAL_ASSERT( sourceColor->GetTexture().isNull() == false , "" );

	if( !IsInitialized() )
	{
		CreateTemporalBuffer( frameInfo );
	}

	CRenderStateManager &stateManager = GetRenderer()->GetStateManager();

	const Uint16 writeIdx = 1 - m_flipSide;
	const Uint16 readIdx = m_flipSide;

	RED_FATAL_ASSERT( writeIdx < ARRAY_COUNT(m_temporalBuffer), "Wrong writeIdx" );
	RED_FATAL_ASSERT( readIdx < ARRAY_COUNT(m_temporalBuffer), "Wrong readIdx" );

	CRenderTarget* target = m_temporalBuffer[ writeIdx ];

	if( target )
	{

		const Uint32 width  = frameInfo.m_width;
		const Uint32 height = frameInfo.m_height;
		const Uint32 sampleFullWidth = target->GetWidth();
		const Uint32 sampleFullHeight = target->GetHeight();

		RED_FATAL_ASSERT( width > 0, "" );
		RED_FATAL_ASSERT( height > 0, "" );
		RED_FATAL_ASSERT( sampleFullWidth > 0, "" );
		RED_FATAL_ASSERT( sampleFullHeight > 0, "" );

		RED_FATAL_ASSERT( m_sizeDiv > 0, "" );

		const Uint32 sampleWidth = width / m_sizeDiv;
		const Uint32 sampleHeight = height / m_sizeDiv;

		TexelArea blurArea( sampleWidth , sampleHeight );

		/////////////////////////////////////////////
		// Shader setup
		{
			// If there is fading. jump quickly to blur one only
			const Float sharpToBlurScale			= m_fadingOut ? 1.0f : ::Clamp( m_blurProgress * m_blurScaleTempo , 0.0f , 1.0f );
			const Float blurForce					= ::Clamp( m_blurProgress * m_blurSourceTempo , 0.1f , 0.5f );
			const Vector calculateTexCoordTransform = CalculateTexCoordTransform( blurArea, blurArea, sampleFullWidth , sampleFullHeight );

			stateManager.SetVertexConst( VSC_Custom_0,	calculateTexCoordTransform );

			stateManager.SetPixelConst( PSC_Custom_0,	Vector( blurForce , sharpToBlurScale , 0.0 , 0.0 ) );

			stateManager.SetPixelConst( PSC_Custom_1,	Vector( (sampleWidth - 0.5f) / sampleFullWidth , (sampleHeight - 0.5f) / sampleFullHeight , 1.0f/sampleFullWidth , 1.0f/sampleFullHeight ) );
		}

		/////////////////////////////////////////////
		// Do blur bullshit
		{
			GpuApi::RenderTargetSetup rtSetup;
			GpuApi::TextureRef	target = m_temporalBuffer[ writeIdx ]->GetTexture();

			RED_FATAL_ASSERT( target.isNull() == false , "" );

			rtSetup.SetColorTarget( 0, target );
			rtSetup.SetViewportFromTarget( target );
			GpuApi::SetupRenderTargets( rtSetup );	

			GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip);
			GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip);

			// Set draw context
			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

			RED_FATAL_ASSERT( sourceColor != nullptr , "" );

			GpuApi::TextureRef colorRefs[2] = { sourceColor->GetTexture() , m_temporalBuffer[ readIdx ]->GetTexture() };
			GpuApi::BindTextures( 0, 2, colorRefs, GpuApi::PixelShader );

			drawer.DrawQuad( GetRenderer()->m_postfxLoadingBlur );		

			GpuApi::BindTextures( 0, 0, nullptr, GpuApi::PixelShader );
			GpuApi::BindTextures( 1, 0, nullptr, GpuApi::PixelShader );
		}

	}

	/////////////////////////////////////////////
	// Post-blur updates

	m_flipSide = 1 - m_flipSide;

}


// We don't need buffer anymore
void CLoadingScreenBlur::ReleaseTemporalBuffer( )
{
	if( m_temporalBuffer[0] )
	{
		m_temporalBuffer[0]->ReleaseResources();
		delete m_temporalBuffer[0];
		m_temporalBuffer[0] = NULL;
	}
	if( m_temporalBuffer[1] )
	{
		m_temporalBuffer[1]->ReleaseResources();
		delete m_temporalBuffer[1];
		m_temporalBuffer[1] = NULL;
	}
}


// Create new buffer for bluring
void CLoadingScreenBlur::CreateTemporalBuffer( const CRenderFrameInfo& frameInfo )
{
	ReleaseTemporalBuffer();

	// RTFMT_R11G11B10F
	m_temporalBuffer[0] = new CRenderTarget( frameInfo.m_width / m_sizeDiv , frameInfo.m_height / m_sizeDiv, RTFMT_A16B16G16R16F , true , 0 );
	m_temporalBuffer[1] = new CRenderTarget( frameInfo.m_width / m_sizeDiv , frameInfo.m_height / m_sizeDiv, RTFMT_A16B16G16R16F , true , 0 );

	RED_FATAL_ASSERT( m_temporalBuffer[0] != nullptr, "Coudn't allocate temporal buffer" );
	RED_FATAL_ASSERT( m_temporalBuffer[1] != nullptr, "Coudn't allocate temporal buffer" );

	// Recreate surfaces
	m_temporalBuffer[0]->CreateResources();
	m_temporalBuffer[1]->CreateResources();

	RED_FATAL_ASSERT( m_flipSide < ARRAY_COUNT(m_temporalBuffer) , "Wrong flipIdx" );

	RED_FATAL_ASSERT( m_temporalBuffer[0]->GetTexture().isNull() == false , "Coudn't allocate temporal buffer" );
	RED_FATAL_ASSERT( m_temporalBuffer[1]->GetTexture().isNull() == false , "Coudn't allocate temporal buffer" );

	// ensure the surface is cleared before it is _sampled_ in PerformBlur
	GpuApi::TextureRef target = m_temporalBuffer[ m_flipSide ]->GetTexture();
	GetRenderer()->ClearColorTarget(target, Vector(0.0f, 0.0f, 0.0f, 0.0f));
}

Bool CLoadingScreenBlur::AllowsDynamicRescaling() const
{
	return !IsDrawable();
}

// Render arleady blurred image over the screen
void CLoadingScreenBlur::DrawFade( CPostProcessDrawer &drawer, const CRenderFrameInfo& info , Float timeDelta , ERenderTargetName renderSource )
{
	CRenderSurfaces* surfaces = GetRenderer()->GetSurfaces();
	CRenderStateManager &stateManager = GetRenderer()->GetStateManager();

	RED_FATAL_ASSERT( surfaces != nullptr , "" );

	// We need to blur just over some time
	if( false == m_useFallback && ( m_blurProgress < m_maxBluringTime ) )
	{
		PerformBlur( drawer , surfaces->GetRenderTarget( renderSource ) , info );
	}

	const Uint32 width				= info.m_width;
	const Uint32 height				= info.m_height;

	RED_FATAL_ASSERT( width > 0 && height > 0 , "Inocrect frame size" );

	const GpuApi::TextureRef helper			= surfaces->GetRenderTargetTex( RTN_PostProcessTempFull );
	const GpuApi::TextureRef finalColor		= surfaces->GetRenderTargetTex( RTN_FinalColor );
	const GpuApi::TextureRef sourceTexture	= surfaces->GetRenderTargetTex( renderSource );

	const Float blendScale = m_fadeScale * ::Clamp( m_blurProgress * m_blurScaleTempo , 0.0f , 1.0f );
	
	// On XBox when doing multipresent there must be special case
	const Bool useBackbuffer = ( info.IsViewportPresent() && info.m_multiplanePresent );

	// Check if source texture is the same as the output one
	const Bool sourceIsFinal = ( RTN_FinalColor == renderSource );

	// Drawing surface is helper when source is the same as final
	const GpuApi::TextureRef drawSurface = ( sourceIsFinal ? helper : finalColor );

	// Setup render target
	if( useBackbuffer )
	{
		GpuApi::TextureDesc desc = GetTextureDesc( GpuApi::GetBackBufferTexture() );

		Uint32 viewportWidth = desc.width;
		Uint32 viewportHeight = desc.height;
		if ( width > 0 && height > 0 )
		{
			viewportWidth = Min( viewportWidth, width );
			viewportHeight = Min( viewportHeight, height );
		}

		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, GpuApi::GetBackBufferTexture() );
		rtSetup.SetViewport( viewportWidth, viewportHeight );
		GpuApi::SetupRenderTargets( rtSetup );	
	}
	else
	{
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, drawSurface );
		rtSetup.SetViewport( width , height ,0 ,0 );
		GpuApi::SetupRenderTargets( rtSetup );	
	}

	// We have valid blurred surface. Use it
	if( CRenderTarget* target = m_temporalBuffer[ m_flipSide ] )
	{

		const Uint32 samplerFullWidth = target->GetWidth();
		const Uint32 samplerFullHeight = target->GetHeight();

		RED_FATAL_ASSERT( samplerFullWidth > 0 && samplerFullHeight > 0 && m_sizeDiv > 0 , "Incorect screen size" );

		// Set draw context
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		TexelArea blurArea( width / m_sizeDiv , height / m_sizeDiv );
		TexelArea renderArea( width , height );

		/////////////////////////////////////////////
		// Shader setup
		{
			const Vector calculateTexCoordTransform = CalculateTexCoordTransform( renderArea , blurArea, samplerFullWidth , samplerFullHeight );
			stateManager.SetVertexConst( VSC_Custom_0,	calculateTexCoordTransform );

			stateManager.SetPixelConst( PSC_Custom_0,	Vector( 0.0f , 0.0f , blendScale , 0.0f ) );
		}

		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip);
		GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip);
		GpuApi::SetSamplerStatePreset( 2, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip);

		GpuApi::TextureRef colorRefs[3] = { sourceTexture , m_temporalBuffer[ 1 - m_flipSide ]->GetTexture() , m_temporalBuffer[ m_flipSide ]->GetTexture() };
		GpuApi::BindTextures( 0, 3, colorRefs, GpuApi::PixelShader );

		drawer.DrawQuad( GetRenderer()->m_postfxLoadingBlurFinal );		

		GpuApi::BindTextures( 0, 3, nullptr, GpuApi::PixelShader );

		// If using regular buffer, copy it from helper
		if( !useBackbuffer && sourceIsFinal )
		{
			const ::Rect sourceArea( 0, width, 0, height );
			GetRenderer()->StretchRect( drawSurface , sourceArea , finalColor , sourceArea );
		}

	}
	// Blackscreen fallback
	else
	{
		if( blendScale < 1.0f )
		{
			stateManager.SetLocalToWorld( NULL );
			stateManager.SetCamera2D();

			// GpuApi::TextureDesc desc = GpuApi::GetTextureDesc(sourceTexture);
			const Uint32 fullWidth = surfaces->GetRenderTarget(renderSource)->GetWidth();
			const Uint32 fullHeight = surfaces->GetRenderTarget(renderSource)->GetHeight();

			const Vector channelSelector( 1.0f-blendScale, 1.0f-blendScale, 1.0f-blendScale, 0.0f );
			GetRenderer()->GetDebugDrawer().DrawTexturePreviewTile( 0,0, (Float)fullWidth, (Float)fullHeight, sourceTexture, 0, 0, 0.0f, 1.0f, channelSelector );

			// If using regular buffer, copy it from helper
			if( !useBackbuffer && sourceIsFinal )
			{
				const ::Rect sourceArea( 0, width, 0, height );
				GetRenderer()->StretchRect( drawSurface , sourceArea , finalColor , sourceArea );
			}
		}
		else if( useBackbuffer )
		{
			GetRenderer()->ClearColorTarget( GpuApi::GetBackBufferTexture(), Vector(0,0,0,0) );
		}
		else
		{
			GetRenderer()->ClearColorTarget( finalColor, Vector(0,0,0,0) );
		}

	}

	// Do fading timings
	if( m_fadeTime > FLT_EPSILON  )
	{
		if( m_fadingOut )
		{
			m_fadeScale -= timeDelta / m_fadeTime;
			if( m_fadeScale < 0.0f )
			{
				m_fadingOut = false;
				ReleaseTemporalBuffer();
			}
		}
	}
	else
	{
		m_fadingOut = false;
		ReleaseTemporalBuffer();
	}

}


// This function is for testing pursposes and simulatets showing/hiding loading blur 
// NOT FROM RENDER THREAD !!!!!
void PerformLoadingBlurSimulation( float dT )
{
	static float accumTime = 0.0f;
	static bool mode = 0;
	static bool fallback = 0;

	if( ( accumTime += dT ) > 5 )
	{
		accumTime = 0;

		mode = !mode;

		if( mode )
		{
			( new CRenderCommand_ShowLoadingScreenBlur( 2.0 , 4.0 , fallback ) )->Commit();
		}
		else
		{
			( new CRenderCommand_HideLoadingScreenBlur( 1.0f ) )->Commit();
		}

		if( mode == 0 )
		{
			fallback = !fallback;
		}
	}

}