/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "../core/configVar.h"
#include "../core/fileSystemProfilerWrapper.h"
#include "../engine/flashValue.h"
#include "../engine/renderFrame.h"
#include "../engine/viewport.h"
#include "../engine/flashValueObject.h"
#include "../engine/flashMovie.h"
#include "../engine/renderSettings.h"
#include "renderLoadingScreen.h"
#include "renderLoadingScreenFence.h"
#include "renderVideo.h"
#include "renderThread.h"
#include "renderVideoPlayer.h"

namespace Config
{
	// Whether to show a progress bar during the loading screen. Required on the PS4.
	TConfigVar< Bool > cvLoadingScreenShowProgress( "LoadingScreen/TCR", "ShowProgress", true, eConsoleVarFlag_Save );
}

namespace Config
{
	extern TConfigVar< Bool > cvLoadingScreenDebugDisableVideos;
	extern TConfigVar< Bool > cvSubtitles;
}

namespace Flash
{
	// Needs to match PlatformType.as
	const Uint32 PLATFORM_PC = 0;
	const Uint32 PLATFORM_XBOX1 = 1;
	const Uint32 PLATFORM_PS4 = 2;
	const Uint32 PLATFORM_UNKNOWN = 3;
}

namespace Hacks
{
	extern void RenderAudioForLoadingScreen();
	extern void RenderAudioWhileBlockingGame();
}

//////////////////////////////////////////////////////////////////////////
// CRenderLoadingScreen
//////////////////////////////////////////////////////////////////////////
CRenderLoadingScreen::CRenderLoadingScreen()
	: m_loadingDuration( 0. )
	, m_loadingProgress( 0.f )
	, m_renderFence( nullptr )
	, m_renderVideo( nullptr )
	, m_flashMovie( nullptr )
	, m_fadeInTime( 0.f )
	, m_showLoadingLatch( eVisiblityLatch_None )
	, m_unregisterLoadingScreenLatch( false )
	, m_videoStartedLatch( false )
	, m_isFlashRegistered( false )
	, m_shouldForceLoadingSpinner( false )
	, m_loadingVisible( false )
{
}

CRenderLoadingScreen::~CRenderLoadingScreen()
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

//////////////////////////////////////////////////////////////////////////

void CRenderLoadingScreen::OnVideoStopped()
{
	m_showLoadingLatch = eVisiblityLatch_SetVisible;
	m_shouldForceLoadingSpinner = true; // don't wait for the timeout
}

void CRenderLoadingScreen::OnVideoStarted()
{
	m_showLoadingLatch = eVisiblityLatch_SetInvisible;
	m_shouldForceLoadingSpinner = false;
}

void CRenderLoadingScreen::OnVideoSubtitles( const String& subtitles )
{
	if ( !Config::cvSubtitles.Get() )
	{
		return;
	}

	if ( m_flashHooks.m_setVideoSubtitles.IsFlashClosure() )
	{
		RED_FATAL_ASSERT( m_flashMovie, "No flash movie!");
		CFlashString* flashString = m_flashMovie->CreateString( subtitles );
		const CFlashValue arg = flashString->AsFlashValue();
		flashString->Release();
		m_flashHooks.m_setVideoSubtitles.InvokeSelf( arg );
	}
}

void CRenderLoadingScreen::OnVideoCuePoint( const String& cuePoint )
{
	RED_FATAL_ASSERT( m_renderFence, "Null renderFence");
	if ( m_renderFence->GetState() != eLoadingScreenFenceState_Shown )
	{
		WARN_RENDERER(TXT("CRenderLoadingScreen::OnVideoCuePoint: Too late for video fadeout in renderFence state %u"), (Uint32)m_renderFence->GetState() );
		return;
	}

	// The video is doing its own fade out, don't double fade out. It's expected the USM will then send the cue point "blackscreen" so we put up the loading screens
	// own blackscreen to cover the loading screen image when the video ends so there's no sudden transition
	
	if ( cuePoint.EqualsNC(TXT("OnFadeout")) ) // The video is just starting to do its own fade out
	{
		HideVideoSkipFlash();
		m_renderFence->SetState( eLoadingScreenFenceState_FadeInProgress );
	}
	else if ( cuePoint.EqualsNC(TXT("OnBlackscreen")) ) // The video has finished fading out and is now a blackscreen
	{
		HideVideoSkipFlash();
		m_renderFence->SetState( eLoadingScreenFenceState_FadeInProgress );
		FadeOutFlash( 0.f );
	}
}

void CRenderLoadingScreen::OnFlashExternalInterface( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args, CFlashValue& outRetval )
{
	const Uint32 argCount = args.Size();

	if ( methodName.EqualsNC( TXT("registerLoadingScreen") ) )
	{
		if ( argCount != 1 )
		{
			ERR_ENGINE(TXT("CRenderLoadingScreen::OnExternalInterface: Expected argCount 1"));
			return;
		}
		if ( ! args[0].IsFlashDisplayObject() )
		{
			ERR_ENGINE(TXT("CRenderLoadingScreen::OnExternalInterface: Expected arg[0] to be a display object"));
			return;
		}
		CFlashValue flashSprite = args[0];

		RegisterLoadingScreen( flashSprite );
	}
	else if ( methodName.EqualsNC( TXT("fadeOutCompleted") ) )
	{
		if ( argCount != 1 )
		{
			ERR_ENGINE(TXT("CRenderLoadingScreen::OnExternalInterface: Expected argCount 1"));
			return;
		}
		if ( ! args[0].IsFlashDisplayObject() )
		{
			ERR_ENGINE(TXT("CRenderLoadingScreen::OnExternalInterface: Expected arg[0] to be a display object"));
			return;
		}

		LOG_RENDERER(TXT("CRenderLoadingScreen fadeOutCompleted"));
		m_unregisterLoadingScreenLatch = true; // Set the latch so not destroying the movie mid-advance or while video visible
	}
	else if ( methodName.EqualsNC( TXT("initString") ) )
	{
		if ( argCount != 0 )
		{
			ERR_ENGINE(TXT("CRenderLoadingScreen::OnExternalInterface: Expected argCount 0"));
			return;
		}

		RED_FATAL_ASSERT( m_renderFence, "No render fence!");
		const String& initString = m_renderFence->GetInitString();
		CFlashString* flashString = flashMovie->CreateString( initString );
		outRetval = flashString->AsFlashValue();
		flashString->Release();
		flashString = nullptr;

		LOG_RENDERER(TXT("CRenderLoadingScreen initString: %ls"), initString.AsChar() );
	}
	else
	{
		ERR_ENGINE(TXT("Unknown external interface %ls"), methodName.AsChar() );
	}
}

Bool CRenderLoadingScreen::TickFlash( Float timeDelta, Bool force /*=false*/ )
{
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

	if ( (viewportRect.Width() < 2 || viewportRect.Height() < 2) && !force )
	{
		return false;
	}

	if ( m_flashMovie )
	{
		m_flashMovie->SetViewport( viewportRect );
		m_flashMovie->Tick( timeDelta );
	}

	return true;
}

void CRenderLoadingScreen::Tick( Float timeDelta, Float unclampedTimeDelta )
{
	PC_SCOPE_PIX( CRenderLoadingScreen_Render );

	// Just render the audio shit. Give CSoundSystem a helping hand when prefetch blackscreens etc come up.
	Hacks::RenderAudioWhileBlockingGame();

	m_loadingDuration += unclampedTimeDelta;

	UpdateLoadingVisiblity();

	if ( m_prevLoadingProgress < m_loadingProgress )
	{
		SetProgressFlash( m_loadingProgress );
		m_prevLoadingProgress = m_loadingProgress;
	}

	// Hack: If the progress bar is visible, make sure the spinner is too without waiting for the timeout
	// Should refactor the fade-in logic, but also need to refactor it during streaming prefetch to not show a spinner
	// unnecessarily often. Can't just force it on if not playing a video, since might be a blackscreen
	// or could also just generally then look bad when loading a save from the pause-menu (loading-screen blackscreen) and then transitioning
	// to the loading screen proper.
	m_shouldForceLoadingSpinner |= m_loadingVisible && m_loadingProgress > 0.f;

	if ( ! TickFlash( timeDelta ) )
	{
		return;
	}

	// Keep loading screen Flash while video is playing, for the loading screens own blackscreen
	if ( !m_renderVideo && m_unregisterLoadingScreenLatch )
	{
		UnregisterLoadingScreen();
	}

	if ( m_renderVideo && !m_renderVideo->IsValid() )
	{
#ifndef RED_FINAL_BUILD
		m_debugInfo.m_videoInfo.m_videoFinished = true;
		m_debugInfo.m_videoInfo.m_videoFailed = m_renderVideo->GetErrorFlag();
#endif
		m_renderVideo->Release();
		m_renderVideo = nullptr;
		m_videoStartedLatch = false;
		OnVideoStopped();
	}

	if ( m_renderVideo )
	{
		// Check if video actually started. Somebody could have specified a non-existent video
		if ( m_videoStartedLatch && m_renderVideo->GetRenderVideoState() == eRenderVideoState_Playing )
		{
			m_videoStartedLatch = false;
			OnVideoStarted();
		}

		String subtitles;
		if ( m_renderVideo->FlushSubtitles( subtitles ) )
		{
			OnVideoSubtitles( subtitles );
		}

		String cuePoint;
		if ( m_renderVideo->FlushCuePoint( cuePoint ) )
		{
			OnVideoCuePoint( cuePoint );
		}
	}

	// Check for fade out after validity and don't release video yet, to give the video texture a tick to be cleared
	// so no flashing
	if ( IsFadedOut() && m_renderVideo )
	{
		CRenderVideoPlayer* videoPlayer = GRenderThread->GetVideoPlayer();
		videoPlayer->CancelVideo( m_renderVideo );
	}

	if ( !m_renderVideo && !m_flashMovie )
	{
		m_shouldForceLoadingSpinner = false;
		m_loadingVisible = false;
		m_renderFence->SetState( eLoadingScreenFenceState_Finished );
		m_renderFence->Release();
		m_renderFence = nullptr;
		CRenderVideoPlayer* videoPlayer = GRenderThread->GetVideoPlayer();
		videoPlayer->LockVideoThread( false, eVideoThreadIndex_GameThread );

#ifdef RED_PROFILE_FILE_SYSTEM
RedIOProfiler::ProfileSignalHideLoadingScreen();
#endif
	}
}

Bool CRenderLoadingScreen::RegisterLoadingScreen( CFlashValue& flashSprite )
{
	m_unregisterLoadingScreenLatch = false;

	if ( ! flashSprite.IsFlashDisplayObject() )
	{
		ERR_RENDERER(TXT("RegisterLoadingScreen: Not a flash sprite"));
		return false;
	}

	m_flashSprite = flashSprite;

	Bool retval = true;
	retval &= m_flashSprite.GetMember( TXT("setVideoSubtitles"), m_flashHooks.m_setVideoSubtitles );
	retval &= m_flashSprite.GetMember( TXT("setTipText"), m_flashHooks.m_setTipText );
	retval &= m_flashSprite.GetMember( TXT("showVideoSkip"), m_flashHooks.m_showVideoSkip );
	retval &= m_flashSprite.GetMember( TXT("hideVideoSkip"), m_flashHooks.m_hideVideoSkip );
	retval &= m_flashSprite.GetMember( TXT("hideImage"), m_flashHooks.m_hideImage );
	retval &= m_flashSprite.GetMember( TXT("showImage"), m_flashHooks.m_showImage );
	retval &= m_flashSprite.GetMember( TXT("fadeIn"), m_flashHooks.m_fadeIn );
	retval &= m_flashSprite.GetMember( TXT("fadeOut"), m_flashHooks.m_fadeOut );
	retval &= m_flashSprite.GetMember( TXT("setPlatform"), m_flashHooks.m_setPlatform );
	retval &= m_flashSprite.GetMember( TXT("showProgressBar"), m_flashHooks.m_showProgressBar );
	retval &= m_flashSprite.GetMember( TXT("setProgressValue"), m_flashHooks.m_setProgressValue );
	retval &= m_flashSprite.GetMember( TXT("setPCInput"), m_flashHooks.m_setPCInput );
	retval &= m_flashSprite.GetMember( TXT("setExpansionsAvailable"), m_flashHooks.m_setExpansionsAvailable );

	if ( ! retval )
	{
		ERR_RENDERER(TXT("RegisterLoadingScreen failed to get all Flash hooks!"));
	}

	if ( m_flashHooks.m_setPlatform.IsFlashClosure() )
	{
		const Config::EPlatform platform = Config::GetPlatform();
		Uint32 flashPlatform = Flash::PLATFORM_UNKNOWN;
		switch( platform )
		{
		case Config::ePlatform_PC:
			flashPlatform = Flash::PLATFORM_PC;
			break;
		case Config::ePlatform_PS4:
			flashPlatform = Flash::PLATFORM_PS4;
			break;
		case Config::ePlatform_XB1:
			flashPlatform = Flash::PLATFORM_XBOX1;
			break;
		default:
			flashPlatform = Flash::PLATFORM_UNKNOWN;
			break;
		}

		m_flashHooks.m_setPlatform.InvokeSelf( CFlashValue( flashPlatform ) );
	}

	if ( m_flashHooks.m_fadeIn.IsFlashClosure() )
	{
		m_flashHooks.m_fadeIn.InvokeSelf( CFlashValue(m_fadeInTime) );
	}

	m_isFlashRegistered = true;

	return retval;
}

void CRenderLoadingScreen::UnregisterLoadingScreen()
{
	m_fadeInTime = 0.f;
	m_unregisterLoadingScreenLatch = false;

	m_flashSprite.Clear();
	m_flashHooks = SFlashHooks();

	if ( m_flashMovie )
	{
		RED_VERIFY( m_flashMovie->Release() == 0, TXT("Movie should have been fully released") );
		m_flashMovie = nullptr;
	}

	m_isFlashRegistered = false;
}

void CRenderLoadingScreen::SetLoadingScreenMovie( CFlashMovie* flashMovie )
{
	// We haven't advanced the new movie yet, so no race in clearing any old flashSprite value here.
	UnregisterLoadingScreen();

	m_flashMovie = flashMovie;
	if ( m_flashMovie )
	{
		m_flashMovie->UpdateCaptureThread();
		m_flashMovie->SetExternalInterfaceOverride( this );
		m_flashMovie->Attach();
		
		// Make good and sure it's registered in case used quickly
		TickFlash( 0.05f, true );
		RED_FATAL_ASSERT( m_isFlashRegistered, "Loading screen movie not registered!" );
	}
}

void CRenderLoadingScreen::FadeOutFlash( Float fadeOutTime )
{
	if ( m_flashHooks.m_fadeOut.IsFlashClosure() )
	{
		if ( ! m_flashHooks.m_fadeOut.InvokeSelf( CFlashValue( fadeOutTime ) ) )
		{
			ERR_RENDERER(TXT("CRenderLoadingScreen::FadeOut failed to invoke Flash function!"));
		}
	}
}

void CRenderLoadingScreen::ShowVideoSkipFlash()
{
	RED_FATAL_ASSERT( m_renderFence, "Null renderFence");
	if ( m_renderFence->GetState() != eLoadingScreenFenceState_Shown )
	{
		WARN_RENDERER(TXT("Too late to show video skip in render fence state %u"),  (Uint32)m_renderFence->GetState() );
		return;
	}

	if ( m_flashHooks.m_showVideoSkip.IsFlashClosure() )
	{
		if ( ! m_flashHooks.m_showVideoSkip.InvokeSelf() )
		{
			ERR_RENDERER(TXT("CRenderLoadingScreen::ShowVideoSkip failed to invoke Flash function!"));
		}
	}
}

void CRenderLoadingScreen::HideVideoSkipFlash()
{
	if ( m_flashHooks.m_hideVideoSkip.IsFlashClosure() )
	{
		if ( ! m_flashHooks.m_hideVideoSkip.InvokeSelf() )
		{
			ERR_RENDERER(TXT("CRenderLoadingScreen::HideVideoSkip failed to invoke Flash function!"));
		}
	}
}

void CRenderLoadingScreen::SetPCInputFlash( Bool enable )
{
	if ( m_flashHooks.m_setPCInput.IsFlashClosure() )
	{
		if ( ! m_flashHooks.m_setPCInput.InvokeSelf( CFlashValue( enable ) ) )
		{
			ERR_RENDERER(TXT("CRenderLoadingScreen::SetPCInput failed to invoke Flash function!"));
		}
	}
}

void CRenderLoadingScreen::SetExpansionsAvailableFlash( Bool ep1, Bool ep2 )
{
	if ( m_flashHooks.m_setExpansionsAvailable.IsFlashClosure() )
	{
		if ( ! m_flashHooks.m_setExpansionsAvailable.InvokeSelf( CFlashValue( ep1 ), CFlashValue( ep2 ) ) )
		{
			ERR_RENDERER(TXT("CRenderLoadingScreen::SetExpansionsAvailable failed to invoke Flash function!"));
		}
	}
}

void CRenderLoadingScreen::SetFence( CRenderLoadingScreenFence* fence, Float fadeInTime, Bool hideAtStart )
{
	// Allow null fence to force clean it on shutdown

#ifndef RED_FINAL_BUILD
	m_debugInfo = SDebugInfo();
	m_debugInfo.m_videoInfo.m_videoToPlay = Config::cvLoadingScreenDebugDisableVideos.Get() ? TXT("<disabled>") : TXT("<none>");

	m_debugInfo.m_loadingStartBytes = 0;//CDependencyLoader::GetNumBytesLoadedSoFar();
	m_debugInfo.m_caption = fence ? fence->GetCaption() : String::EMPTY;
#endif

	m_loadingDuration = 0.;
	m_loadingProgress = 0.f;
	m_loadingVisible = false;
	m_shouldForceLoadingSpinner = false;
	m_prevLoadingProgress = 0.f;
	m_showLoadingLatch = hideAtStart ? eVisiblityLatch_SetInvisible : eVisiblityLatch_SetVisible;

	m_fadeInTime = fadeInTime;

	SetLoadingScreenMovie( nullptr );

	if ( m_renderFence )
	{
		m_renderFence->Release();
		m_renderFence = nullptr;
	}
	m_renderFence = fence;

	if ( m_renderVideo )
	{
		CRenderVideoPlayer* videoPlayer = GRenderThread->GetVideoPlayer();
		videoPlayer->CancelVideo( m_renderVideo );
		m_renderVideo->Release();
		m_renderVideo = nullptr;
		m_videoStartedLatch = false;
	}

	// If going to show the loading screen, prevent any new videos from the game.
	CRenderVideoPlayer* videoPlayer = GRenderThread->GetVideoPlayer();
	videoPlayer->LockVideoThread( m_renderFence != nullptr, eVideoThreadIndex_GameThread );

	if ( ! m_renderFence )
	{
		return;
	}

	// Stop any rogue videos from other systems now that we'll show the loading screen.
	videoPlayer->StopAllVideos();

	m_renderFence->AddRef();
	IViewport* const viewport = m_renderFence->GetViewport();
	RED_FATAL_ASSERT( viewport, "Loading screen fence has no viewport!" );

	SetLoadingScreenMovie( m_renderFence->GetFlashMovie() );

#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileSignalShowLoadingScreen();
#endif
}

IViewport* CRenderLoadingScreen::GetViewport() const
{
	return m_renderFence ? m_renderFence->GetViewport() : nullptr;
}

void CRenderLoadingScreen::FadeOut( CRenderLoadingScreenFence* fence, Float fadeOutTime )
{
	if ( ! m_renderFence )
	{
		ERR_RENDERER(TXT("No loading screen currently active for fade out!"));
	}

	if ( m_renderFence != fence )
	{
		ERR_RENDERER(TXT("Different loading screen fence currently active than requested for fadeout"));
		return;
	}

	HideVideoSkipFlash();

	// If force fading, allow even if fade in progress
	if ( fadeOutTime <= 0.f )
	{
		m_renderFence->SetState( eLoadingScreenFenceState_FadeInProgress );
		FadeOutFlash( 0.f );
		return;
	}

	const ELoadingScreenFenceState state = m_renderFence->GetState();
	if ( state != eLoadingScreenFenceState_Shown )
	{
		WARN_RENDERER(TXT("Loading screen state %u is too late for fade out'"), state );
		return;
	}

	m_renderFence->SetState( eLoadingScreenFenceState_FadeInProgress );
	FadeOutFlash( fadeOutTime );
}

void CRenderLoadingScreen::PlayVideo( CRenderVideo* renderVideo, CRenderLoadingScreenFence* fence )
{
	RED_FATAL_ASSERT( renderVideo, "No video" );
	RED_FATAL_ASSERT( fence, "No fence" );

#ifndef RED_FINAL_BUILD
	m_debugInfo.m_videoInfo.m_videoToPlay = renderVideo->GetVideoParams().m_fileName;
	m_debugInfo.m_videoInfo.m_videoFailed = true; // Change to false at end of function
#endif

	if ( ! m_renderFence )
	{
		renderVideo->Stop();
		ERR_RENDERER(TXT("No loading screen currently active for loading screen video!"));
		return;
	}

	if ( m_renderFence != fence )
	{
		renderVideo->Stop();
		ERR_RENDERER(TXT("Different loading screen fence currently active than requested"));
		return;
	}

	const ELoadingScreenFenceState state = m_renderFence->GetState();
	if ( state != eLoadingScreenFenceState_Shown )
	{
		renderVideo->Stop();
		WARN_RENDERER(TXT("Loading screen state %u is too late for starting loading screen video"), state );
		return;
	}

	if ( m_renderVideo )
	{
		renderVideo->Stop();
		ERR_RENDERER(TXT("Loading screen video already set!"));
		return;
	}

	m_renderVideo = renderVideo;
	m_renderVideo->AddRef();

#ifndef RED_FINAL_BUILD
	m_debugInfo.m_videoInfo.m_videoFailed = false; // Not failed yet anyway
#endif

	CRenderVideoPlayer* videoPlayer = GRenderThread->GetVideoPlayer();
	videoPlayer->StopAllVideos();
	videoPlayer->PlayVideo( m_renderVideo, eVideoThreadIndex_RenderThread );
	m_videoStartedLatch = true;
}

void CRenderLoadingScreen::ToggleVideoSkip( CRenderLoadingScreenFence* fence, Bool enabled )
{
	if ( ! m_renderFence )
	{
		ERR_RENDERER(TXT("No loading screen currently active to toggle the skip for!") );
	}

	if ( m_renderFence != fence )
	{
		ERR_RENDERER(TXT("Different loading screen fence currently active than requested for video skip!"));
		return;
	}

	if ( enabled )
	{
		if ( m_renderVideo && m_renderVideo->IsValid() )
		{
			ShowVideoSkipFlash();
		}
	}
	else
	{
		HideVideoSkipFlash();
	}
}

void CRenderLoadingScreen::SetPCInput( CRenderLoadingScreenFence* fence, Bool enabled )
{
	if ( ! m_renderFence )
	{
		ERR_RENDERER(TXT("No loading screen currently active to toggle the skip for!") );
	}

	if ( m_renderFence != fence )
	{
		ERR_RENDERER(TXT("Different loading screen fence currently active than requested for video skip!"));
		return;
	}

	SetPCInputFlash( enabled );
}

void CRenderLoadingScreen::SetExpansionsAvailable( CRenderLoadingScreenFence* fence, Bool ep1, Bool ep2 )
{
	if ( ! m_renderFence )
	{
		ERR_RENDERER(TXT("No loading screen currently active to toggle the skip for!") );
	}

	if ( m_renderFence != fence )
	{
		ERR_RENDERER(TXT("Different loading screen fence currently active than requested for video skip!"));
		return;
	}

	SetExpansionsAvailableFlash( ep1, ep2 );
}

void CRenderLoadingScreen::UpdateLoadingVisiblity()
{
	if ( ! m_isFlashRegistered )
	{
		return;
	}

	switch ( m_showLoadingLatch )
	{	
	case eVisiblityLatch_SetVisible:
		{
			HideVideoSkipFlash();
			ShowImageFlash();
			ShowProgressFlash();
			m_loadingVisible = true;
		}

		break;
	case eVisiblityLatch_SetInvisible:
		{
			HideVideoSkipFlash();
			HideProgressFlash();
			HideImageFlash();
			m_loadingVisible = false;
		}
		break;
	default:
		break;
	}

	m_showLoadingLatch = eVisiblityLatch_None;
}

void CRenderLoadingScreen::ShowProgressFlash()
{
	if ( !Config::cvLoadingScreenShowProgress.Get() )
	{	
		return;
	}

	if ( m_flashHooks.m_showProgressBar.IsFlashClosure() )
	{
		const CFlashValue arg( true );
		if ( ! m_flashHooks.m_showProgressBar.InvokeSelf( arg ) )
		{
			ERR_RENDERER(TXT("CRenderLoadingScreen::ShowProgressFlash failed to invoke Flash function!"));
		}

		// Set it to initial value
		SetProgressFlash( m_loadingProgress );
	}
}

void CRenderLoadingScreen::HideProgressFlash()
{
	if ( m_flashHooks.m_showProgressBar.IsFlashClosure() )
	{
		const CFlashValue arg( false );
		if ( ! m_flashHooks.m_showProgressBar.InvokeSelf( arg ) )
		{
			ERR_RENDERER(TXT("CRenderLoadingScreen::HideProgressFlash failed to invoke Flash function!"));
		}
	}
}

void CRenderLoadingScreen::ShowImageFlash()
{
	if ( m_flashHooks.m_showImage.IsFlashClosure() )
	{
		if ( ! m_flashHooks.m_showImage.InvokeSelf() )
		{
			ERR_RENDERER(TXT("CRenderLoadingScreen::ShowImageFlash failed to invoke Flash function!"));
		}
	}
}

void CRenderLoadingScreen::HideImageFlash()
{
	if ( m_flashHooks.m_hideImage.IsFlashClosure() )
	{
		if ( ! m_flashHooks.m_hideImage.InvokeSelf() )
		{
			ERR_RENDERER(TXT("CRenderLoadingScreen::HideImageFlash failed to invoke Flash function!"));
		}
	}
}

void CRenderLoadingScreen::SetProgressFlash( Float value )
{
	if ( m_flashHooks.m_showProgressBar.IsFlashClosure() )
	{
		const CFlashValue arg( value );
		if ( ! m_flashHooks.m_setProgressValue.InvokeSelf( arg ) )
		{
			ERR_RENDERER(TXT("CRenderLoadingScreen::SetProgressFlash failed to invoke Flash function!"));
		}
	}
}

void CRenderLoadingScreen::SetProgress( Float value )
{
	m_loadingProgress = Clamp< Float >( value, 0.f, 1.0f );
}
