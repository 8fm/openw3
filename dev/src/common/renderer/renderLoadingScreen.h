/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../engine/videoPlayer.h"
#include "../engine/loadingScreen.h"
#include "../engine/flashValue.h"
#include "../engine/flashMovie.h"
#include "../engine/flashPlayer.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CRenderVideo;
class CRenderLoadingScreenFence;

//////////////////////////////////////////////////////////////////////////
// CRenderLoadingScreen
//////////////////////////////////////////////////////////////////////////
class CRenderLoadingScreen : public IFlashExternalInterfaceHandler
{
public:
#ifndef RED_FINAL_BUILD
	struct SDebugInfo
	{
		Uint64					m_loadingStartBytes;
		String					m_caption;
		struct SVideoInfo
		{
			String				m_videoToPlay;
			Bool				m_videoFailed:1;
			Bool				m_videoFinished:1;

			SVideoInfo()
				: m_videoFailed(false)
				, m_videoFinished(false)
			{}
		};

		SVideoInfo				m_videoInfo;

		SDebugInfo()
			: m_loadingStartBytes( 0 )
		{}
	};

	SDebugInfo m_debugInfo;

public:
	const SDebugInfo& GetDebugInfo() const { return m_debugInfo; }
	SDebugInfo& GetDebugInfo() { return m_debugInfo; }
#endif

private:
	enum EVisiblityLatch : Uint8
	{
		eVisiblityLatch_None,
		eVisiblityLatch_SetVisible,
		eVisiblityLatch_SetInvisible,
	};

private:
	Double						m_loadingDuration;
	Float						m_loadingProgress;
	Float						m_prevLoadingProgress;

private:
	CRenderLoadingScreenFence*	m_renderFence;
	CRenderVideo*				m_renderVideo;
	CFlashMovie*				m_flashMovie;
	CFlashValue					m_flashSprite;
	Float						m_fadeInTime;
	EVisiblityLatch				m_showLoadingLatch;
	Bool						m_unregisterLoadingScreenLatch:1;
	Bool						m_videoStartedLatch:1;
	Bool						m_isFlashRegistered:1;
	Bool						m_shouldForceLoadingSpinner:1;
	Bool						m_loadingVisible:1;

private:
	struct SFlashHooks
	{
		CFlashValue				m_setVideoSubtitles;
		CFlashValue				m_setTipText;
		CFlashValue				m_showVideoSkip;
		CFlashValue				m_hideVideoSkip;
		CFlashValue				m_hideImage;
		CFlashValue				m_showImage;
		CFlashValue				m_fadeIn;
		CFlashValue				m_fadeOut;
		CFlashValue				m_setPlatform;
		CFlashValue				m_showProgressBar;
		CFlashValue				m_setProgressValue;
		CFlashValue				m_setPCInput;
		CFlashValue				m_setExpansionsAvailable;
	};

	SFlashHooks m_flashHooks;

public:
	//! IFlashExternalInterfaceHandler functions
	virtual void OnFlashExternalInterface( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args, CFlashValue& outRetval ) override;

public:
	CRenderLoadingScreen();
	~CRenderLoadingScreen();

public:
	void SetFence( CRenderLoadingScreenFence* fence, Float fadeInTime, Bool hideAtStart );

	//! Fade out the loading screen to black
	void FadeOut( CRenderLoadingScreenFence* fence, Float fadeOutTime );

	void PlayVideo( CRenderVideo* renderVideo, CRenderLoadingScreenFence* fence );

	//! Toggle the skip indicator for the loading video
	void ToggleVideoSkip( CRenderLoadingScreenFence* fence, Bool enabled );

	//! Change input feedback scheme (keyboard + mouse / game pad)
	void SetPCInput( CRenderLoadingScreenFence* fence, Bool enabled );

	void SetExpansionsAvailable( CRenderLoadingScreenFence* fence, Bool ep1, Bool ep2 );

	void SetProgress( Float value );

	Double GetLoadingDuration() const { return m_loadingDuration; }

	Bool ShouldForceLoadingSpinner() const { return m_shouldForceLoadingSpinner; }

public:
	IViewport* GetViewport() const;

public:
	void Tick( Float timeDelta, Float unclampedTimeDelta );

public:
	Bool IsActive() const { return m_renderFence != nullptr; }
	Bool IsFadedOut() const { return m_unregisterLoadingScreenLatch; }

private:
	void SetLoadingScreenMovie( CFlashMovie* flashMovie );

private:
	Bool RegisterLoadingScreen( CFlashValue& flashSprite );
	void UnregisterLoadingScreen();

private:
	void ShowVideoSkipFlash();
	void HideVideoSkipFlash();
	void FadeOutFlash( Float fadeOutTime );
	void SetProgressFlash( Float value );
	void ShowProgressFlash();
	void HideProgressFlash();
	void ShowImageFlash();
	void HideImageFlash();
	void SetPCInputFlash( Bool enable );
	void SetExpansionsAvailableFlash( Bool ep1, Bool ep2 );

private:
	Bool TickFlash( Float timeDelta, Bool force = false );

private:
	void UpdateLoadingVisiblity();

private:
	void OnVideoStopped();
	void OnVideoStarted();
	void OnVideoSubtitles( const String& subtitles );
	void OnVideoCuePoint( const String& cuePoint );
};
