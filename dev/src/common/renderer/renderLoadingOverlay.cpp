/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "../engine/flashValue.h"
#include "../engine/renderFrame.h"
#include "../engine/viewport.h"
#include "../engine/flashValueObject.h"
#include "../engine/flashMovie.h"
#include "../engine/renderSettings.h"
#include "renderLoadingOverlay.h"
#include "../engine/renderer.h"
#include "renderThread.h"
#include "renderVideoPlayer.h"
#include "renderLoadingScreen.h"
#include "../core/configVar.h"
#include "renderLoadingBlur.h"

//////////////////////////////////////////////////////////////////////////
// CRenderLoadingOverlay
//////////////////////////////////////////////////////////////////////////
namespace Config
{
	//! Time to wait for the blackscreen to end before considering it really ended, in order to avoid possible spinner fluctuations
	TConfigVar< Float, Validation::FloatRange<0,5,1> > cvNoninteractiveEndCooldown( "LoadingOverlay/TCR", "NoninteractiveEndCooldown", 1.f, eConsoleVarFlag_Save );
	
	//! Time to wait before showing the loading/blackscreen/blur spinner. TCR requirements vary across platforms. Not too short, but not constantly reminding of loading either.
	TConfigVar< Float, Validation::FloatRange<0,30,1> > cvNoninteractiveSpinnerTime( "LoadingOverlay/TCR", "NoninteractiveSpinnerTime", 3.f, eConsoleVarFlag_Save );

	//! Time it takes the spinner to fade in
	TConfigVar< Float, Validation::FloatRange<0,5,1> > cvFadeInSpinnerTime( "LoadingOverlay/TCR", "FadeInSpinnerTime", 1.f, eConsoleVarFlag_Save );

	//! Time it takes the spinner to fade out
	TConfigVar< Float, Validation::FloatRange<0,5,1> > cvFadeOutSpinnerTime( "LoadingOverlay/TCR", "FadeOutSpinnerTime", 1.f, eConsoleVarFlag_Save );

	//! Spinner can only fade out after cvFadeInSpinnerTime + cvFadeoutCooldownExtraTime seconds.
	TConfigVar< Float, Validation::FloatRange<0,5,1> > cvFadeOutCooldownExtraTime( "LoadingOverlay/TCR", "FadeOutCooldownExtraTime", 1.f, eConsoleVarFlag_Save );

	TConfigVar< Float > cvHeightAdjust( "LoadingOverlay", "HeightAdjust", -140.f );
}

namespace Config
{
	extern TConfigVar< Bool > cvLoadingScreenEditorDisabled;
}

CRenderLoadingOverlay::CRenderLoadingOverlay()
	: m_flashMovie( nullptr )
	, m_noninteractiveDuration( 0. )
	, m_noninteractiveCooldown( 0. )
	, m_fadeOutCooldown( 0. )
	, m_origY(0.f)
	, m_visiblityLatch( eVisiblityLatch_None )
	, m_forceVisible( false )
	, m_isVisible( false )
	, m_latchedVisible( false )
	, m_wasNoninteractive( false )
	, m_hackPendingResetPosition( false )
{
}

CRenderLoadingOverlay::~CRenderLoadingOverlay()
{
	m_flashHooks = SFlashHooks();
	m_flashSprite.Clear();
	if ( m_flashMovie )
	{
		m_flashMovie->UpdateCaptureThread();
		m_flashMovie->Release();
		m_flashMovie = nullptr;
	}
}

void CRenderLoadingOverlay::OnFlashExternalInterface( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args, CFlashValue& outRetval )
{
	const Uint32 argCount = args.Size();
	if ( argCount != 1 )
	{
		ERR_ENGINE(TXT("CRenderLoadingOverlay::OnExternalInterface: Expected argCount 1"));
		return;
	}

	if ( ! args[0].IsFlashDisplayObject() )
	{
		ERR_ENGINE(TXT("CRenderLoadingOverlay::OnExternalInterface: Expected arg[0] to be a display object"));
		return;
	}

	CFlashValue flashSprite = args[0];
	if ( methodName.EqualsNC( TXT("registerLoadingOverlay") ) )
	{
		RegisterLoadingOverlay( flashSprite );
	}
	else
	{
		ERR_ENGINE(TXT("Unknown external interface %ls"), methodName.AsChar() );
	}
}

void CRenderLoadingOverlay::Tick( Float timeDelta, Float unclampedTimeDelta )
{
	PC_SCOPE_PIX( CRenderLoadingScreen_Render );
	Rect viewportRect = Rect::EMPTY;

	IViewport* viewport = GGame ? GGame->GetViewport() : nullptr;
	if ( viewport )
	{
		viewportRect.m_left = viewport->GetX();
		viewportRect.m_right = viewportRect.m_left + viewport->GetWidth();
		viewportRect.m_top = viewport->GetY();
		viewportRect.m_bottom = viewportRect.m_top + viewport->GetHeight();
		if( Config::cvForcedRendererOverlayResolution.Get() )
		{
			viewportRect.m_left = 0;
			viewportRect.m_right = Config::cvForcedRendererResolutionWidth.Get();
			viewportRect.m_top = 0;
			viewportRect.m_bottom = Config::cvForcedRendererResolutionHeight.Get();	
		}
	}

	if ( m_fadeOutCooldown > 0. )
	{
		m_fadeOutCooldown -= unclampedTimeDelta;
		if ( m_fadeOutCooldown < 0. )
		{
			m_fadeOutCooldown = 0.;
		}
	}

	UpdateForceVisible( unclampedTimeDelta );

	if ( viewportRect.Width() < 2 || viewportRect.Height() < 2 )
	{
		return;
	}

	// Don't process the latches yet if no movie to not get the visibility states out of sync between code and Flash
	if ( ! m_flashMovie )
	{
		return;
	}

	Bool makeVisible = false;
	Bool resetLatch = true;
	// Don't process the latch if forced, since it's an override state
	if ( !m_forceVisible )
	{
		switch ( m_visiblityLatch )
		{
		case eVisiblityLatch_None:
			break;
		case eVisiblityLatch_SetVisible:
			m_latchedVisible = true;
			break;
		case eVisiblityLatch_SetInvisible:
			if ( m_fadeOutCooldown <= 0. )
			{
				m_latchedVisible = false;
			}
			else
			{
				resetLatch = false;
			}
			break;
		default:
			HALT( "Unhandled visiblityLatch %u", (Uint32)m_visiblityLatch );
			break;
		}

		makeVisible = m_latchedVisible;
	}
	else
	{
		resetLatch = false;
		makeVisible = true;

		// If forced visible, just reset to normal position...
		if ( m_hackPendingResetPosition )
		{
			HACK_ResetPosition();
			m_hackPendingResetPosition = false;
		}
	}

	if ( resetLatch )
	{
		m_visiblityLatch = eVisiblityLatch_None;
	}

	// Should change visibility state
	if ( ( m_isVisible ^ makeVisible ) )
	{
		if ( makeVisible )
		{
			if ( !GIsEditor || !Config::cvLoadingScreenEditorDisabled.Get() )
			{
				const Float fadeInTime = Config::cvFadeInSpinnerTime.Get();
				m_fadeOutCooldown = fadeInTime + Config::cvFadeOutCooldownExtraTime.Get();
				DoFadeInFlash( fadeInTime );
			}
		}
		else
		{
			const Float fadeOutTime = Config::cvFadeOutSpinnerTime.Get();
			DoFadeOutFlash( fadeOutTime );
		}
		m_isVisible = makeVisible;
	}

	m_flashMovie->SetViewport( viewportRect );
	m_flashMovie->Tick( timeDelta );
}

void CRenderLoadingOverlay::UpdateForceVisible( Float timeDelta )
{
	CLoadingScreenBlur* loadingBlur = GRenderThread->GetLoadingScreenBlur();

	// This counts as loading, although if we switch to a blackscreen and show the spinner,
	// then it seems like an allowable edge case because it's a blackscreen and not necessarily loading anymore
	CRenderLoadingScreen* loadingScreen = GRenderThread->GetLoadingScreen();

	const Bool isBlurOrBlackscreen = static_cast< CRenderInterface* >( GRender )->IsBlackscreen() || loadingBlur->IsActive();
	
	// Count the loading screen as non-interactive and part of the spinner duration/cooldown. This way we don't show the spinner
	// when showing the loading screen just to hide it almost immediately when playing a loading screen video
	const Bool isNoninteractive = loadingScreen->IsActive() || isBlurOrBlackscreen;

	// We have any video, loading screen or not. It should count as an animation and also not count as loading
	// as long as we don't show a loading indicator. This also helps in the case that scene rendering is suppressed
	// while playing a video.
	// Blackscreen isn't visible during non-interactive rendering and the blur had better not be!
	CRenderVideoPlayer* videoPlayer = GRenderThread->GetVideoPlayer();
	if ( videoPlayer && videoPlayer->IsPlayingVideo() )
	{
		m_wasNoninteractive = false; // Don't necessarily force a spinner as soon as video stops
		m_noninteractiveDuration = m_noninteractiveCooldown = 0.;
		m_forceVisible = false;
		return;
	}

	// Noninteractive state changed. Acknowledge after enough time has elapsed
	// Clear m_wasNoninteractive once enough time has elapsed so not resetting the start
	// time if the blackscreen/blur is off for too little time
	if ( (m_wasNoninteractive ^ isNoninteractive ) )
	{
		if ( isNoninteractive )
		{
			m_noninteractiveDuration = m_noninteractiveCooldown = 0.;
			m_wasNoninteractive = true;
		}
		else
		{
			m_noninteractiveCooldown += timeDelta;
			const Float noninteractiveEndCooldown = Config::cvNoninteractiveEndCooldown.Get();
			if ( m_noninteractiveCooldown > noninteractiveEndCooldown )
			{
				m_noninteractiveDuration = m_noninteractiveCooldown = 0.;
				m_wasNoninteractive = false;
			}
		}
	}

	if ( isNoninteractive )
	{
		m_noninteractiveDuration += timeDelta;
	}

	RED_FATAL_ASSERT( loadingScreen, "Loading screen null");
	const Float noninteractiveSpinnerTime = Config::cvNoninteractiveSpinnerTime.Get();
	m_forceVisible = (loadingScreen->IsActive() && loadingScreen->ShouldForceLoadingSpinner()) || m_noninteractiveDuration >= noninteractiveSpinnerTime;
}

Bool CRenderLoadingOverlay::RegisterLoadingOverlay( CFlashValue& flashSprite )
{
	if ( ! flashSprite.IsFlashDisplayObject() )
	{
		ERR_RENDERER(TXT("RegisterLoadingScreen: Not a flash sprite"));
		return false;
	}

	m_flashSprite = flashSprite;
	CFlashDisplayInfo dinfo;
	if ( m_flashSprite.GetFlashDisplayInfo(dinfo) )
	{
		m_origY = (Float)dinfo.GetY();
		LOG_RENDERER(TXT("Original spinner Y: %.2f"), m_origY);
	}

	Bool retval = true;
	retval &= m_flashSprite.GetMember( TXT("fadeIn"), m_flashHooks.m_fadeIn );
	retval &= m_flashSprite.GetMember( TXT("fadeOut"), m_flashHooks.m_fadeOut );

	if ( ! retval )
	{
		ERR_RENDERER(TXT("RegisterLoadingOverlay failed to get all Flash hooks!"));
	}

	return retval;
}

void CRenderLoadingOverlay::InitWithFlash( CFlashMovie* flashMovie )
{
	RED_FATAL_ASSERT( ! m_flashMovie, "Flash already initialized!" );

	m_flashMovie = flashMovie;
	m_flashMovie->UpdateCaptureThread();
	m_flashMovie->SetExternalInterfaceOverride( this );
	m_flashMovie->Attach();
}

const Char* const HACK_CAPTION = TXT("Loading Save");

void CRenderLoadingOverlay::FadeIn( const String& caption )
{
	m_caption = caption;
	m_visiblityLatch = eVisiblityLatch_SetVisible;

	// mega hack, maybe make an extra arg
	if ( caption.EqualsNC(HACK_CAPTION) )
	{
		HACK_ChangePosition();
		m_hackPendingResetPosition = false;
	}
}

void CRenderLoadingOverlay::FadeOut( const String& caption )
{
	m_caption = caption;
	m_visiblityLatch = eVisiblityLatch_SetInvisible;
	if ( caption.EqualsNC(HACK_CAPTION) )
	{
		m_hackPendingResetPosition = true;
	}
}

void CRenderLoadingOverlay::DoFadeOutFlash( Float fadeOutTime )
{
	const Float actualFadeOutTime = m_hackPendingResetPosition ? 0.f : fadeOutTime;

	if ( m_flashHooks.m_fadeOut.IsFlashClosure() )
	{
		if ( ! m_flashHooks.m_fadeOut.InvokeSelf( CFlashValue( actualFadeOutTime ) ) )
		{
			ERR_RENDERER(TXT("CRenderLoadingOverlay::FadeOut failed to invoke Flash function!"));
		}
	}

	if ( m_hackPendingResetPosition )
	{
		HACK_ResetPosition();
		m_hackPendingResetPosition = false;
	}
}

void CRenderLoadingOverlay::DoFadeInFlash( Float fadeInTime )
{
	if ( m_flashHooks.m_fadeOut.IsFlashClosure() )
	{
		if ( ! m_flashHooks.m_fadeIn.InvokeSelf( CFlashValue( fadeInTime ) ) )
		{
			ERR_RENDERER(TXT("CRenderLoadingOverlay::FadeIn failed to invoke Flash function!"));
		}
	}
}

void CRenderLoadingOverlay::HACK_ChangePosition()
{
	CFlashDisplayInfo dinfo;
	if ( m_flashSprite.GetFlashDisplayInfo( dinfo ) )
	{
		dinfo.SetY( m_origY + Config::cvHeightAdjust.Get() );
		m_flashSprite.SetFlashDisplayInfo( dinfo );
	}	
}

void CRenderLoadingOverlay::HACK_ResetPosition()
{
	CFlashDisplayInfo dinfo;
	if ( m_flashSprite.GetFlashDisplayInfo( dinfo ) )
	{
		dinfo.SetY( m_origY );
		m_flashSprite.SetFlashDisplayInfo( dinfo );
	}	
}
