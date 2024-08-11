/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "gameplayFxSepia.h"

#include "renderShaderPair.h"
#include "renderRenderSurfaces.h"

//////////////////////////////////////////////////////////////////////////
///
/// CSepiaEffectPostFX
///

CSepiaEffectPostFX::CSepiaEffectPostFX( CGameplayEffects* parent )
	: m_timeSinceStart( 0.0f )
	, m_fadeInTime( 0.0f )
	, m_fadeOutTime( 0.0f )
	, m_isFadingOut( false )
	, IGameplayEffect( parent, EPE_SEPIA )
{

}

CSepiaEffectPostFX::~CSepiaEffectPostFX()
{
}

void CSepiaEffectPostFX::Init()
{
}

void CSepiaEffectPostFX::Shutdown()
{
}

Bool CSepiaEffectPostFX::Apply( CRenderCollector &collector, const CRenderFrameInfo& frameInfo, ERenderTargetName rtnColorSource , ERenderTargetName rtnColorTarget )
{
	RED_ASSERT( IsEnabled() );
	
	CRenderSurfaces* surfaces = GetRenderer()->GetSurfaces();

	// Set new rendertarget
	GpuApi::RenderTargetSetup rtSetupMain;
	rtSetupMain.SetColorTarget( 0, surfaces->GetRenderTargetTex(rtnColorTarget) );
	rtSetupMain.SetViewport( frameInfo.m_width, frameInfo.m_height, 0, 0 );
	GpuApi::SetupRenderTargets( rtSetupMain );

	// Bind textures
	GpuApi::TextureRef colorTarget = GetRenderer()->GetSurfaces()->GetRenderTargetTex( rtnColorSource );
	GpuApi::BindTextures( 0, 1, &colorTarget, GpuApi::PixelShader );
	GpuApi::SetSamplerStateCommon( 0, 2, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );

	// Set parameters	
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0 + 0, Vector ( m_factor, 0.0f, 0.0f, 0.0f ) );


	// Setup draw context
	CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

	GetRenderer()->m_gameplayPostFXSepia->Bind();

	// Blit
	GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 0), Vector (1, 1, 0, 0), 0.5f );

	// Unbind textures
	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );

	return true;
}

void CSepiaEffectPostFX::Tick( Float time )
{
	m_timeSinceStart += time;

	if ( m_isFadingOut )
	{
		m_factor = 1.0f - (Clamp<Float>( m_timeSinceStart, 0.0f, Max<Float>( m_fadeOutTime, 0.0001f ) ) / Max<Float>( m_fadeOutTime, 0.0001f ));
	}
	else
	{
		m_factor = Clamp<Float>( m_timeSinceStart, 0.0f, Max<Float>( m_fadeInTime, 0.0001f ) ) / Max<Float>( m_fadeInTime, 0.0001f );
	}

	if ( m_isFadingOut && (m_timeSinceStart > m_fadeOutTime) )
	{
		m_isFadingOut = false;
		SetEnabled( false );
	}
}

void CSepiaEffectPostFX::Enable( Float fadeInTime )
{
	SetEnabled( true );
	m_timeSinceStart = 0.0f;
	m_isFadingOut = false;
	m_fadeInTime = fadeInTime;
}

void CSepiaEffectPostFX::Disable( Float fadeOutTime )
{
	m_fadeOutTime = fadeOutTime;
	m_timeSinceStart = 0.0f;
	m_isFadingOut = true;
}