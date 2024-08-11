/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "gameplayFxFocusMode.h"

#include "renderCollector.h"
#include "renderShaderPair.h"
#include "renderRenderSurfaces.h"
#include "renderProxyDecal.h"

#include "../../common/core/gatheredResource.h"
#include "../core/2darray.h"
#include "../engine/renderFragment.h"

// TODO: I don't really like that the effect itself manages its settings file... would be nice to have some sort of generalized
// settings mechanism for this kind of stuff maybe? Might also make a mess if/when we get to having per-component groups -- where
// does a mesh get the highlight color from, the effect? That just makes more dependencies on something that shouldn't be in the
// engine to begin with...
extern CGatheredResource resGameplayEffectsFocusModeGroups;

// TODO: Ultimately, groups should be specified per-component or something, so we wouldn't need any global names like this
RED_DEFINE_STATIC_NAME( questGroup );
RED_DEFINE_STATIC_NAME( interactiveGroup );

//////////////////////////////////////////////////////////////////////////
///
/// CFocusModeEffectPostFX
///

CFocusModeEffectPostFX::CFocusModeEffectPostFX( CGameplayEffects* parent )
	: m_timeSinceStart( 0.0f )
	, m_fadeInTime( 0.5f )	
	, m_isFadingOut( false )
	, m_desaturation( 1.0f )
	, m_highlightBoost( 0.0f )
	, m_effectZoom( 1.0f )
	, m_fadeRange( 1000.0f )
	, m_fadeCenterExponent( 1.0f, 1.0f )
	, m_fadeCenterStartRange( 0.0f, 0.0f, 1.0f, 1.0f )
	, m_dimmingFactor( 0.0f )
	, m_dimmingTime( 6.0f )
	, m_isFadingOutInteractive(false)
	, m_isFadingInInteractive(false)
	, m_currentDimmingRate( 1.0f )
	, m_isEnabledExtended( false )
	, m_effectDistanceShift( 0.0f )
	, m_dimmingSpeed( 6.0f )
	, m_playerPosition( Vector::ZEROS )
	, m_diffusion()
	, IGameplayEffect( parent , EPE_FOCUS_MODE , EPO_POST_BLOOM )
{

}

CFocusModeEffectPostFX::~CFocusModeEffectPostFX()
{
}

void CFocusModeEffectPostFX::Init()
{
	ReloadFocusModeSettings();
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().RegisterListener( CNAME( CSVFileSaved ), this );
#endif
	// NO DEVICE YET
	//m_diffusion.Initialize( 256 );
}

void CFocusModeEffectPostFX::Shutdown()
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().UnregisterListener( CNAME( CSVFileSaved ), this );
#endif
	m_fmGroups.ClearFast();
}

Uint8 CFocusModeEffectPostFX::GetUsedStencilBits()
{
	return LC_Custom0 | LC_Interactive;
}


#ifndef NO_EDITOR_EVENT_SYSTEM
void CFocusModeEffectPostFX::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( CSVFileSaved ) )
	{
		if( resGameplayEffectsFocusModeGroups.GetPath().ToString().ContainsSubstring( GetEventData< String >( data ) ) )
		{
			ReloadFocusModeSettings();
		}
	}
}
#endif


void CFocusModeEffectPostFX::ReloadFocusModeSettings()
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	// Lock mutex, since this happens on main thread but render thread accesses also.
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_groupsMutex );
#endif

	C2dArray* groupsArray = resGameplayEffectsFocusModeGroups.LoadAndGet< C2dArray >();

	RED_ASSERT( groupsArray, TXT("Unable to open '%ls'"), resGameplayEffectsFocusModeGroups.GetPath().ToString().AsChar() );
	if ( !groupsArray )
	{
		m_fmGroups.ClearFast();
		return;
	}

	Uint32 numGroups = ( Uint32 )groupsArray->GetNumberOfRows();

	m_fmGroups.Resize( numGroups );
	for ( Uint32 i = 0; i < numGroups; ++i )
	{
		String groupName = groupsArray->GetValue( TXT( "GroupName" ), i );		
		m_fmGroups[i].m_name = CName( groupName );

		Vector color;
		C2dArray::ConvertValue( groupsArray->GetValue( TXT( "Color" ), i ), color );

		m_fmGroups[i].m_color = Vector::Clamp4( color, 0.0f, 1.0f );
		// Pre-multiply RGB by the unclamped (but non-negative) A value. Use unclamped to allow extra brightening.
		m_fmGroups[i].m_color.Mul3( Max( color.W, 0.0f ) );
	}
}


const SFocusModeGroup& CFocusModeEffectPostFX::FindFocusModeGroup( const CName& name ) const
{
	static const SFocusModeGroup defaultGroup = { CNAME( default ), Vector( 1.0f, 1.0f, 1.0f, 1.0f ) };

	for ( Uint32 i = 0; i < m_fmGroups.Size(); ++i )
	{
		if ( m_fmGroups[i].m_name == name )
		{
			return m_fmGroups[i];
		}
	}

	return defaultGroup;
}


Bool CFocusModeEffectPostFX::Apply( CRenderCollector &collector, const CRenderFrameInfo &frameInfo, ERenderTargetName rtnColor, ERenderTargetName rtnTarget )
{
	PC_SCOPE_RENDER_LVL1( RenderFocusMode );

	RED_ASSERT( IsEnabled() );

	ERenderTargetName rtnHelper = RTN_PostProcessTempFull;

	RenderingContext rc( frameInfo.m_camera );
	rc.m_pass = RP_GBuffer;
	GetRenderer()->GetStateManager().SetCamera( frameInfo.m_camera );	

	const Bool origReversedProjection = GpuApi::IsReversedProjectionState();
	GpuApi::SetReversedProjectionState( frameInfo.m_camera.IsReversedProjection() );

	// Add stencil mask for the outside elements - this is for the desaturation to happen only on not-focused screen parts
	CRenderSurfaces* surfaces = GetRenderer()->GetSurfaces();
	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );

	// Bind target
	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnHelper ) );
	rtSetup.SetDepthStencilTarget( surfaces->GetDepthBufferTex(), -1, true );
	rtSetup.SetViewport( frameInfo.m_width, frameInfo.m_height, 0, 0 );
	GpuApi::SetupRenderTargets( rtSetup );
	
	GetRenderer()->ClearColorTarget( Vector::ZEROS );

	// bind depth (for clear shader also needed)
	GpuApi::TextureRef sceneDepth = surfaces->GetDepthBufferTex();
	GpuApi::BindTextures( PSSMP_SceneDepth, 1, &sceneDepth, GpuApi::PixelShader );	
	GpuApi::SetSamplerStatePreset( PSSMP_SceneDepth, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );
		
	{
#ifndef NO_EDITOR_EVENT_SYSTEM
		// Lock mutex. We could get notification of CSV file changed on main thread, which replaces the groups.
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_groupsMutex );
#endif
		
		ELightChannel lightChannel = LC_Custom0;	//< Remember to removed this from GetUsedStencilBits() in case of removal from here !
		RED_ASSERT( 0 != (lightChannel & GetUsedStencilBits()) );

		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet_StencilMatchAny, lightChannel );		
		GetRenderer()->m_postFXClear_FocusMode->Bind();

		// clear pass needs these
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_6, Vector( m_factor, m_effectDistanceShift, m_highlightBoost, m_dimmingFactor ) );		
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_7, m_playerPosition );

		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector(0.0f,1.0f,0.0f,0.0f) );
		GetRenderer()->GetDebugDrawer().DrawQuad( Vector( -1, -1, 0, 0 ), Vector( 1, 1, 0, 0 ), 0.5f );

	}

	{

		// Draw the focusmode group (interactive)
		ELightChannel lightChannel = LC_Interactive;

		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet_StencilMatchAny, lightChannel );		
		GetRenderer()->m_postFXClear_FocusMode->Bind();

		// clear pass needs these
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_6, Vector( m_factor, m_effectDistanceShift, m_highlightBoost, m_dimmingFactor ) );		
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_7, m_playerPosition );

		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector(1.0f,0.0f,0.0f,0.0f) );
		GetRenderer()->GetDebugDrawer().DrawQuad( Vector( -1, -1, 0, 0 ), Vector( 1, 1, 0, 0 ), 0.5f );
	}
	
	if ( collector.m_scene && collector.GetRenderFrameInfo().IsShowFlagOn(SHOW_Decals) )
	{
		PC_SCOPE_PIX(RenderDecalsFocusMode);		
		{
#ifndef NO_EDITOR_EVENT_SYSTEM
			// Lock mutex. We could get notification of CSV file changed on main thread, which replaces the groups.
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_groupsMutex );
#endif
			// Draw quest decals
			rc.m_lightChannelFilter = LCF_AllBitsSet;
			rc.m_lightChannelFilterMask = LC_Custom0;
						
			CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_DecalsFocusMode, 0 );
		
			GpuApi::TextureRef gbuffer1 = surfaces->GetRenderTargetTex( GetRenderer()->IsMSAAEnabled( frameInfo ) ? RTN_GBuffer1MSAA : RTN_GBuffer1 );
			GpuApi::BindTextures( 2, 1, &gbuffer1, GpuApi::PixelShader );

			//Decals power are multiplied by 3 to increase their visibility
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector(0.0f,3.0f,0.0f,0.0f) ); 
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector( m_playerPosition.X, m_playerPosition.Y, m_playerPosition.Z, m_effectDistanceShift ) ); 			

			for( Uint32 i = 0; i < EDDI_MAX; ++i )
			{
				collector.RenderDecals( (EDynamicDecalRenderIndex)(i), rc );
			}
		}

		{
			// Draw interactive decals
			rc.m_lightChannelFilter = LCF_AllBitsSet;
			rc.m_lightChannelFilterMask = LC_Interactive;
			
			CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_DecalsFocusMode, 0 );
			
			GpuApi::TextureRef gbuffer1 = surfaces->GetRenderTargetTex( GetRenderer()->IsMSAAEnabled( frameInfo ) ? RTN_GBuffer1MSAA : RTN_GBuffer1 );
			GpuApi::BindTextures( 2, 1, &gbuffer1, GpuApi::PixelShader );

			//Decals power are multiplied by 3 to increase their visibility
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector(3.0f,0.0f,0.0f,0.0f) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector( m_playerPosition.X, m_playerPosition.Y, m_playerPosition.Z, m_effectDistanceShift ) ); 			

			for( Uint32 i = 0; i < EDDI_MAX; ++i )
			{
				collector.RenderDecals( (EDynamicDecalRenderIndex)(i), rc );
			}			
		}

		GetRenderer()->GetStateManager().SetCamera2D();
	}

	GpuApi::BindTextures( PSSMP_SceneDepth, 1, nullptr, GpuApi::PixelShader );
	GpuApi::BindTextures( 2, 1, nullptr, GpuApi::PixelShader );

	// simulate diffusion on the mask	
	m_diffusion.Initialize();	
	GpuApi::TextureRef dynamic = m_diffusion.Calculate( surfaces->GetRenderTargetTex( rtnHelper ), frameInfo.m_frameTime, frameInfo.m_camera.GetCameraForward() );

	// Set new rendertarget
	GpuApi::RenderTargetSetup rtSetupMain;
	rtSetupMain.SetColorTarget( 0, surfaces->GetRenderTargetTex(rtnTarget) );
	rtSetupMain.SetDepthStencilTarget( GpuApi::TextureRef::Null() );
	rtSetupMain.SetViewport( frameInfo.m_width, frameInfo.m_height, 0, 0 );
	GpuApi::SetupRenderTargets( rtSetupMain );

	// Bind textures
	RED_ASSERT( rtnTarget != rtnColor );
	RED_ASSERT( rtnTarget != rtnHelper );

	GpuApi::TextureRef focusModeRefs[4] = { surfaces->GetRenderTargetTex( rtnColor ), surfaces->GetDepthBufferTex(), dynamic, surfaces->GetRenderTargetTex( rtnHelper ) };
	GpuApi::BindTextures( 0, 4, &(focusModeRefs[0]), GpuApi::PixelShader );

	const GpuApi::SamplerStateRef &stateRef = GpuApi::GetSamplerStatePreset( GpuApi::SAMPSTATEPRESET_WrapLinearNoMip );
	GpuApi::SetSamplerState( 3, stateRef, GpuApi::PixelShader );

	GpuApi::SetSamplerStateCommon( 0, 3, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );
	
	// Setup draw context
	CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

	// 4 colors by now, 2 original, 2 alternatives (colorblind option)
	RED_ASSERT( m_fmGroups.Size() >= 4 );
	{
		Vector col0 = Config::cvColorblindFocusMode.Get() ? m_fmGroups[2].m_color : m_fmGroups[0].m_color;
		Vector col1 = Config::cvColorblindFocusMode.Get() ? m_fmGroups[3].m_color : m_fmGroups[1].m_color;

		// Set parameters	
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector( m_effectZoom, m_fadeRange, m_fadeCenterExponent.X, m_fadeCenterExponent.Y ) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, m_fadeCenterStartRange );			
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_6, Vector( m_factor, m_effectDistanceShift, m_highlightBoost, m_dimmingFactor ) );		
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_7, m_playerPosition );

		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_4, col0 );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_5, col1 );
	}

	GetRenderer()->m_gameplayPostFXFocus->Bind();

	// Blit
	GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 0), Vector (1, 1, 0, 0), 0.5f );

	// Unbind textures
	GpuApi::BindTextures( 0, 2, nullptr, GpuApi::PixelShader );
	GpuApi::BindTextures( 3, 1, nullptr, GpuApi::PixelShader );

	// Restore reversed projection
	GpuApi::SetReversedProjectionState( origReversedProjection );

	return true;
}


void CFocusModeEffectPostFX::Tick( Float time )
{	

	if( m_isFadingOutInteractive )
	{
		m_dimmingFactor -= time*m_currentDimmingRate;

		m_effectDistanceShift =  Clamp<Float>( m_effectDistanceShift - ( time/m_dimmingSpeed ), 0.0f, 1.0f );

		if( m_dimmingFactor < 0.0f )
		{
			m_dimmingFactor = 0.0f;
			m_isFadingOutInteractive = false;
			m_isFadingInInteractive = true;
			m_currentDimmingRate = 5.0f;
			m_effectDistanceShift = 0.0f;

			// turning effect completely off				
			if( m_isFadingOut || !m_isEnabledExtended )
			{
				SetEnabled( false );
				m_isFadingOut = false;	
				m_isEnabledExtended = false;										
			}				

			// do not fade in when extended is disabled (user is not holding the LT anymore)
			if( !m_isEnabledExtended )
			{
				m_currentDimmingRate = 1.0f;
				m_isFadingInInteractive = false;
			}			
		}			
	}	
	else if( m_isFadingInInteractive )
	{
		if( m_dimmingFactor < m_dimmingTime )
		{
			m_dimmingFactor += time*m_currentDimmingRate;
			m_effectDistanceShift =  Clamp<Float>( m_effectDistanceShift + ( time/m_dimmingSpeed ), 0.0f, 1.0f );

			if( m_dimmingFactor > (m_dimmingTime-0.001f) )
			{
				// restore dimming										
				m_currentDimmingRate = 1.0f;
				m_dimmingFactor = m_dimmingTime;					
			}
		}
	}

	if( m_isEnabledExtended )
	{
		if ( m_isFadingOut )
		{
			m_timeSinceStart -= time;

			if ( m_timeSinceStart <= 0.0f )
			{
				m_isFadingOut = false;
				m_isEnabledExtended = false;
			}								
		}
		else
		{
			m_timeSinceStart += time;										
		}

		Float fadeInClamp = Max<Float>( m_fadeInTime, 0.0001f );
		m_timeSinceStart = Clamp<Float>( m_timeSinceStart, 0.0f, fadeInClamp );
		m_factor =  Clamp<Float>( m_timeSinceStart / fadeInClamp, 0.0f, 1.0f );

	}
	else
	{
		if( m_timeSinceStart > 0.0f ) 
		{
			m_timeSinceStart -= time;

			Float fadeInClamp = Max<Float>( m_fadeInTime, 0.0001f );
			m_timeSinceStart = Clamp<Float>( m_timeSinceStart, 0.0f, fadeInClamp );
			m_factor =  Clamp<Float>( m_timeSinceStart / fadeInClamp, 0.0f, 1.0f );				
		}
	}

}

void CFocusModeEffectPostFX::Enable( Float desaturation, Float highlightBoost )
{	
	m_effectZoom = Config::cvMotionSicknessFocusMode.Get() ? 0.1f : 1.0f;
	SetEnabled( true );

	// start with smooth blend	
	m_isFadingInInteractive = true;
	m_isFadingOutInteractive = false;

	m_desaturation = desaturation;
	m_highlightBoost = highlightBoost;
}



void CFocusModeEffectPostFX::Disable( Bool forceDisable )
{
	m_effectZoom = Config::cvMotionSicknessFocusMode.Get() ? 0.1f : 1.0f;

	if( forceDisable )
	{
		SetEnabled( false );
	}
	else
	{
		// end with smooth blend	
		m_isFadingOutInteractive = true;
		m_isFadingInInteractive = false;

		m_isFadingOut = true;				
	}	
}

// Focus mode enhanced effect
void CFocusModeEffectPostFX::EnableExtended( Float fadeInTime )
{
	if( IsEnabled() )
	{
		m_isEnabledExtended = true;

		m_isFadingOut = false;				

		// hardocoded by now
		m_fadeInTime = 1.0f;

		if( m_isFadingOutInteractive ) 
		{
			m_isFadingInInteractive = true;
			m_isFadingOutInteractive = false;
		}
	}	
}

void CFocusModeEffectPostFX::DisableExtended( Float fadeOutTime )
{
	if( IsEnabled() )
	{	
		m_isFadingOut = true;		
	}	
}

void CFocusModeEffectPostFX::SetDimming( Bool dimming )
{
	m_isFadingOutInteractive = dimming;	
}

void CFocusModeEffectPostFX::SetFadeParameters( Float fadeNear, Float fadeFar, Float dimmingTime, Float dimmingSpeed )
{	
	m_fadeRange = fadeFar - fadeNear;	
	m_dimmingTime = dimmingTime > 0.1f ? dimmingTime : 0.1f;
	m_dimmingSpeed = dimmingSpeed;
}

void CFocusModeEffectPostFX::SetPlayerPosition( const Vector& playerPosition )
{
	m_playerPosition = playerPosition;
}
