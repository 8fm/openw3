/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../engine/viewport.h"
#include "../engine/inputBufferedInputEvent.h"

class CRenderSurfaces;

void AdjustPlatformResolution( Uint32& width, Uint32& height );

/// Viewport for Direct3D 9 based renderer
class CRenderViewport : public IViewport
{
protected:
	struct ExpectedViewportProperties
	{
		Bool fullscreen;	//! Note: when alt-tabbing, swap chain fullscreen state is changed by DXGI, so we need to handle that case also
		Uint32 width;
		Uint32 height;
		EViewportWindowMode windowMode;
		Uint32 outputMonitor;
	};

protected:
	class CViewportWindow*				m_window;					//!< Internal window
	Uint32								m_suppressSceneRendering;	//!< Suppress scene rendering
#ifndef RED_PLATFORM_ORBIS
	POINT								m_capturedCursorPosition;	//!< Mouse position when captured
#endif
#ifdef RED_ASSERTS_ENABLED
	Bool								m_disableActivationHandling;
#endif
	GpuApi::SwapChainRef				m_swapChain;
	GpuApi::SwapChainRef				m_swapChainOverlay;
	Bool								m_useVsync;
	Bool								m_updateGamma;
	Float								m_currentGamma;				//! HACK moradin - there is no callback from the config system so we have to check this every frame if it changed

	Uint32								m_rendererWidth;
	Uint32								m_rendererHeight;

	Int32								m_horizontalCachet;			//! HACK for ratios bigger than 16:9
	Int32								m_verticalCachet;			//! HACK for ratios bigger than 16:9

	Bool								m_fullscreenMode;
	Uint32								m_outputMonitor;

	ExpectedViewportProperties			m_expectedProperties;

	Bool								m_windowDeactivated;			//! We set this to true when for example alt-tabbing
	EViewportWindowMode					m_windowModeForRestore;			//! When alt-tabbing, then we want to switch to windowed mode temporarily, so we need to store previous mode
	Bool								m_isInTemporaryWindowMode;		//! Keep track of TemporalWindowModeBegin/End, so we won't begin it twice
	Bool								m_forceMouseModeRefresh;		//! When changing window size we need to refresh mouse mode, so it's captured and clipped to proper window
	Bool								m_forceWindowRefresh;			//! Forces window refresh on initialization
	static Bool							s_preserveSystemGamma;			//! If true, then we are not going to call GpuApi::SetGammaForSwapChain in any RenderViewport

public:
	// Get window
	RED_INLINE CViewportWindow* GetWindow() const { return m_window; }

	// Is the rendering scene suppressed
	RED_INLINE Bool IsSceneRenderingSuppressed() const { return m_suppressSceneRendering > 0; }

	// Request to suppress/allow normal scene rendering. Works on a refcount, so each call to suppress should have a matching call to allow.
	void SuppressSceneRendering( Bool state );
	
	RED_INLINE Int32 GetHorizontalCachet(){ return m_horizontalCachet; }
	RED_INLINE Int32 GetVerticalCachet(){ return m_verticalCachet; }

	virtual Uint32 GetRendererWidth() const override;
	virtual Uint32 GetRendererHeight() const override;

	RED_INLINE EViewportWindowMode GetViewportWindowModeForRestore() const { return m_windowModeForRestore; }

#ifdef RED_ASSERTS_ENABLED
	RED_INLINE void DisableActivationHandling() { m_disableActivationHandling = true; }
	RED_INLINE void EnableActivationHandling() { m_disableActivationHandling = false; }
	RED_INLINE Bool IsActivationHandlingDisabled() const { return m_disableActivationHandling; }
#endif

public:
	CRenderViewport( Uint32 width, Uint32 height, EViewportWindowMode windowMode, CViewportWindow* window, Bool useVsync, Uint32 renderWidth = 0, Uint32 renderHeight = 0 );
	~CRenderViewport();

	// Interface
	virtual Bool AdjustSize( Uint32 x, Uint32 y, Uint32 width, Uint32 height );
	virtual Bool AdjustSize( Uint32 width, Uint32 height );
	virtual Bool ToggleFullscreen( Bool fullscreen );
	virtual Bool ProcessInput( const BufferedInput& input );
	virtual Bool GetPixels( Uint32 x, Uint32 y, Uint32 width, Uint32 height, TDynArray< Color >& pixels );
	virtual void SetMouseMode( EMouseMode mode, Bool notRestoreMousePosition = false );
	virtual void AdjustSizeWithCachets( EAspectRatio cachetAspectRatio );
	virtual void RestoreSize() override;

	virtual void SetFocus();
#ifndef RED_PLATFORM_ORBIS
	virtual void SetTopLevelWindow( HWND handle );
#endif
	virtual void ResizeBackBuffer( Uint32 width, Uint32 height );

	virtual void SetCursorVisibility( Bool state, Bool notRestoreMousePosition = false );

	virtual void Activate();
	virtual void Deactivate();

	virtual void NotifyGammaChanged() override
	{
		m_updateGamma = true;
	}

public:
	// Update resolution, window mode, output monitor
	void UpdateViewportProperties();

	// Tick viewport
	void Tick( Float timeDelta );

	// Prepare viewport for rendering
	Bool PrepareViewport();

	// Prepare viewport for front rendering
	Bool PrepareViewportOverlay();

	//! Reset viewport
	void Reset();
	
	// Present viewport contents
	void Present( Bool multiplane );

	// Used for screenshot generation - do not use in normal rendering or anything, totally unsafe
	void SetCheatedSize( Uint32 rendererWidth, Uint32 rendererHeight, Uint32 width, Uint32 height, Uint32 fullWidth, Uint32 fullHeight );

	// Resize viewport and all related buffers
	virtual void RequestWindowSize( Uint32 width, Uint32 height ) override;

	// Change output monitor (triggers resize)
	virtual void RequestOutputMonitor( Uint32 outputMonitorIndex ) override;

	// Change window mode (windowed, fullscreen, borderless)
	virtual void RequestWindowMode( EViewportWindowMode windowMode ) override;

public:
	// Switch to windowed mode and store previous mode
	void TemporalWindowModeBegin();

	// Switch from windowed mode to stored previous mode
	void TemporalWindowModeEnd();

private:
	// Handle fullscreen mode changes triggered by DXGI (some alt+tab cases, external application notifications, etc.)
	void MinimizeOnExternalApplicationEvent();

	// Checks whether any viewport action must be done
	Bool NeedToResize();
	Bool NeedToToggleFullscreen();
	Bool NeedToChangeWindowMode();
	Bool NeedToChangeOutputMonitor();
	RED_INLINE Bool NeedToRefresh() { return m_forceWindowRefresh; }

	// Actions to be performed on viewport when changing size, window mode or output monitor
	void PerformResize();
	void PerformToggleFullscreen();
	void PerformChangeWindowMode();
};
