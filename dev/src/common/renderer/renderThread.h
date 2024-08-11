/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../engine/renderThread.h"

class CRenderLoadingScreen;
class CRenderVideoPlayer;
class CRenderLoadingOverlay;
class CLoadingScreenBlur;

/// Rendering thread
class CRenderThread : public IRenderThread
{
protected:
	CRenderInterface*		m_renderer;

	CRenderLoadingOverlay*	m_loadingOverlay;
	CRenderLoadingScreen*	m_loadingScreen;
	CRenderVideoPlayer*		m_videoPlayer;
	CLoadingScreenBlur*		m_loadingScreenBlur;

	Bool					m_hasMessagePump;
	Bool					m_suspended;

public:
	//! Do we have non interactive rendering enabled ?
	Bool IsNonInteractiveRenderingEnabled() const;

public:
	CRenderThread( CRenderInterface* renderer, Bool hasMessagePump );
	~CRenderThread();

	//! Request thread exit
	void RequestExit();

	CRenderVideoPlayer* GetVideoPlayer() const { return m_videoPlayer; }
	CRenderLoadingOverlay* GetLoadingOverlay() const { return m_loadingOverlay; }
	CRenderLoadingScreen* GetLoadingScreen() const { return m_loadingScreen; }
	CLoadingScreenBlur*	 GetLoadingScreenBlur() const { return m_loadingScreenBlur; }

	Bool IsSuspended() const { return m_suspended; }
	void SetSuspended( Bool suspended ){ m_suspended = suspended; }

	//! Thread function
	virtual void ThreadFunc();

#ifdef USE_ANSEL
	virtual Bool IsPlayingVideo() const;
#endif // USE_ANSEL

	void Shutdown();

private:
	Bool ProcessCommandBatch( const class RenderThreadTime& currentTime );

	//! Process non interactive rendering
	void ProcessNonInteractiveRendering( Float timeDelta, Float unclampedTimeDelta );

	void ProcessLoadingScreenBlur( Float timeDelta );
};

/// Render thread
extern CRenderThread* GRenderThread;
