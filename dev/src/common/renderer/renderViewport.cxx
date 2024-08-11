/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderViewport.h"
#include "renderViewportWindow.h"
#include "../engine/renderCommands.h"
#include "../core/feedback.h"
#include "../engine/renderSettings.h"
#include "../engine/renderFence.h"
#include "../core/configVar.h"

//FIXME2<<< included for hack below
#include "renderViewportWindowRoot.h"

#ifdef USE_SCALEFORM
RED_MESSAGE("HACK: scaleform should reacquire the backbuffer when needed instead of having to be notified here (by CRenderCommand_HandleResizeEvent command)")
#include "../engine/renderScaleformCommands.h"
#include "guiRenderSystemScaleform.h"
#endif
#include "../engine/inGameConfig.h"

namespace Config
{
	// Some users don't want to use gamma setting in game, so we don't want to reset their settings by our SetGammaForSwapChain
	TConfigVar<Bool> cvPreserveSystemGamma( "Rendering", "PreserveSystemGamma", false, eConsoleVarFlag_Save );
}

Bool CRenderViewport::s_preserveSystemGamma = false;

void AdjustPlatformResolution( Uint32& width, Uint32& height )
{
	if( Config::cvForcedRendererResolution.Get() )
	{
		width  = Config::cvForcedRendererResolutionWidth.Get();
		height = Config::cvForcedRendererResolutionHeight.Get();	
	}
}

CRenderViewport::CRenderViewport( Uint32 width, Uint32 height, EViewportWindowMode windowMode, CViewportWindow* window, Bool useVsync, Uint32 renderWidth, Uint32 renderHeight )
#ifdef RED_PLATFORM_CONSOLE
	: IViewport( width, height, windowMode )
#else
	: IViewport( width, height, VWM_Windowed )		// For PC we always initialize with windowed mode and put a request for proper mode
#endif
	, m_suppressSceneRendering( 0 )
	, m_window( window )
	, m_useVsync( useVsync )
	, m_updateGamma( true )
	, m_currentGamma( 0.f )
#ifdef RED_ASSERTS_ENABLED
	, m_disableActivationHandling( false )
#endif
	, m_forceMouseModeRefresh( false )
	, m_forceWindowRefresh( true )
	, m_isInTemporaryWindowMode( false )
{
	if( renderWidth == 0 )
	{
		renderWidth = width;
	}
	if( renderHeight == 0 )
	{
		renderHeight = height;
	}

	m_rendererWidth = renderWidth;
	m_rendererHeight = renderHeight;

	// Initialize expected values to match the constructor values
	m_expectedProperties.width = m_rendererWidth;
	m_expectedProperties.height = m_rendererHeight;
	m_expectedProperties.fullscreen = false;
	m_expectedProperties.outputMonitor = 0;
	m_expectedProperties.windowMode = VWM_Windowed;

	RequestWindowMode( windowMode );
	RequestWindowSize( m_rendererWidth, m_rendererHeight );
	RequestOutputMonitor( 0 );

	// Bind window to viewport
	m_window->Bind( this );

	GpuApi::SwapChainDesc scDesc;
	scDesc.width = m_rendererWidth;
	scDesc.height = m_rendererHeight;

	scDesc.fullscreen = false;			// Always create swap chain in windowed mode and change fullscreen state later, otherwise DXGI has troubles with switching between window modes
	scDesc.overlay = false;

#ifndef RED_PLATFORM_ORBIS
	m_capturedCursorPosition.x = 0;
	m_capturedCursorPosition.y = 0;

	HWND windowHandle = window->GetWindowHandle();
	scDesc.windowHandle = &windowHandle;
#endif

#if MICROSOFT_ATG_DYNAMIC_SCALING
	// create with full HD backbuffer to allow for dynamic scaling
	scDesc.width = 1920;
	scDesc.height = 1080;
#endif

	m_swapChain = GpuApi::CreateSwapChainWithBackBuffer( scDesc );
	GpuApi::ToggleFullscreen( m_swapChain, scDesc.fullscreen );

	LOG_RENDERER( TXT("SwapChain created with width: %d, height: %d in %hs mode"), scDesc.width, scDesc.height, scDesc.fullscreen?"fullscreen":"windowed" );

	// on other platforms the overlay swapchain is the same as the regular one
	m_swapChainOverlay = m_swapChain;
	GpuApi::AddRef( m_swapChainOverlay ); // addref so we can release safely
#if defined( RED_PLATFORM_DURANGO )

	GpuApi::SwapChainDesc scDescOverlay;
	if( Config::cvForcedRendererOverlayResolution.Get() )
	{
		scDescOverlay.width = m_rendererWidth;
		scDescOverlay.height = m_rendererHeight;
	}
	else
	{
		scDescOverlay.width = width;
		scDescOverlay.height = height;
	}
	scDescOverlay.fullscreen = windowMode == VWM_Fullscreen;
	scDescOverlay.overlay = true;
	scDescOverlay.windowHandle = &windowHandle;

	// on Durango we have to create another swapchain for the multiplane present
	m_swapChainOverlay = GpuApi::CreateSwapChainWithBackBuffer( scDescOverlay );
#endif

	if ( m_swapChain.isNull() )
	{
		GFeedback->ShowError( TXT("Can't create new swapchain, too many 3D viewports open at the same time") );
	}

	s_preserveSystemGamma = Config::cvPreserveSystemGamma.Get();

	if( s_preserveSystemGamma == true )
	{
		GInGameConfig::GetInstance().ActivateTag(CNAME(PreserveSystemGamma));
	}
	else if( s_preserveSystemGamma == false )
	{
		GInGameConfig::GetInstance().DeactivateTag(CNAME(PreserveSystemGamma));
	}
}

CRenderViewport::~CRenderViewport()
{
	// Flush pending stuff from rendering thread
	GetRenderer()->Flush();

	// If we had rendering suppressed, stop suppressing in the renderer.
	RED_ASSERT( m_suppressSceneRendering == 0, TXT("Destroying CRenderViewport with %u outstanding 'SuppressSceneRendering' requests. Someone missed re-enabling it somewhere? I'll clear them out now, but it could lead to problems elsewhere"), m_suppressSceneRendering );
	while ( m_suppressSceneRendering > 0 )
	{
		SuppressSceneRendering( false );
	}

	// Reset all viewport shit
	Reset();

	// Close window
	if ( m_window )
	{
		delete m_window;
		m_window = NULL;
	}

	// Notify that viewport has been close
	GetRenderer()->ViewportClosed( this );

	GpuApi::ToggleFullscreen( m_swapChain, false );

	GpuApi::SafeRelease( m_swapChain );
	GpuApi::SafeRelease( m_swapChainOverlay );
}


void CRenderViewport::SuppressSceneRendering( Bool state )
{
	if ( state )
	{
		++m_suppressSceneRendering;
	}
	else
	{
		RED_ASSERT( m_suppressSceneRendering > 0, TXT("Unsuppressing scene rendering too many times!") );
		if ( m_suppressSceneRendering > 0 )
		{
			--m_suppressSceneRendering;
		}
	}
	GetRenderer()->SuppressSceneRendering( state );
}


void CRenderViewport::SetFocus()
{
#ifndef RED_PLATFORM_CONSOLE
	::SetFocus( m_window->GetWindowHandle() );
#endif
}

void CRenderViewport::Reset()
{
	// empty
}

Uint32 CRenderViewport::GetRendererWidth() const
{ 
	RED_FATAL_ASSERT( m_rendererWidth <= GetFullWidth(), "Invalid width" ); 
	return Min( GetFullWidth(), m_rendererWidth ); 
}

Uint32 CRenderViewport::GetRendererHeight() const 
{ 
	RED_FATAL_ASSERT( m_rendererHeight <= GetFullHeight(), "Invalid height" ); 
	return Min( GetFullHeight(), m_rendererHeight ); 
}

Bool CRenderViewport::AdjustSize( Uint32 x, Uint32 y, Uint32 width, Uint32 height )
{
	// RequestWindowSize( width, height );
	return true;
}

Bool CRenderViewport::AdjustSize( Uint32 width, Uint32 height )
{
	RequestWindowSize( width, height );
	return true;
}

void CRenderViewport::AdjustSizeWithCachets( EAspectRatio cachetAspectRatio )
{
	IViewport::AdjustSizeWithCachets( cachetAspectRatio );
	m_forceWindowRefresh = true;
}

void CRenderViewport::RestoreSize()
{
	IViewport::RestoreSize();
	m_forceWindowRefresh = true;
}

void CRenderViewport::ResizeBackBuffer( Uint32 width, Uint32 height )
{
	RED_ASSERT( ! ::SIsMainThread() );
}

Bool CRenderViewport::ToggleFullscreen( Bool fullscreen )
{
	return true;
}

void CRenderViewport::UpdateViewportProperties()
{
#ifdef RED_PLATFORM_WINPC
	if( NeedToRefresh() || NeedToToggleFullscreen() || NeedToResize() || NeedToChangeWindowMode() || NeedToChangeOutputMonitor() )
	{
		// Make sure renderer does not execute any commands
		GetRenderer()->Flush();

		PerformChangeWindowMode();
		PerformToggleFullscreen();
		PerformResize();
		m_forceMouseModeRefresh = true;
		m_forceWindowRefresh = false;

		// Inform renderer that render surfaces and scaleform must be updated
		(new CRenderCommand_HandleResizeEvent( m_rendererWidth, m_rendererHeight ))->Commit();
	}

	MinimizeOnExternalApplicationEvent();
#endif
}

void CRenderViewport::Tick( Float timeDelta )
{
	// Tick by hook
	if ( m_hook )
	{
		// Do not draw user windows if game is active
		if ( GGame->IsActive() && !GGame->IsPaused() )
		{
			return;
		}

		// Tick it
		m_hook->OnViewportTick( this, timeDelta );

		// Auto redraw
		if ( IsAutoRedraw() )
		{
			CRenderFrame* frame = m_hook->OnViewportCreateFrame( this );
			if ( frame )
			{
				//m_hook->OnViewportGenerateFragments( this, frame );
				m_hook->OnViewportRenderFrame( this, frame );
				frame->Release();
			}
		}
	}
}

Bool CRenderViewport::ProcessInput(  const BufferedInput& input  )
{
	// Process internal events
	return m_window->ProcessInput( input );
}

Bool CRenderViewport::PrepareViewport()
{
	GpuApi::SetBackBufferFromSwapChain( m_swapChain );

	// Viewport prepared
	return true;
}

Bool CRenderViewport::PrepareViewportOverlay()
{
	GpuApi::SetBackBufferFromSwapChain( m_swapChainOverlay );

	// Viewport prepared
	return true;
}

#if MICROSOFT_ATG_DYNAMIC_SCALING
namespace GpuApi
{
	extern Uint32 g_DynamicScaleWidthFullRes;
	extern Uint32 g_DynamicScaleHeightFullRes;
}
#endif

void CRenderViewport::Present( Bool multiplane )
{
	PC_SCOPE_RENDER_LVL0(Present);

	GpuApi::Rect sourceRect, destRect;

	sourceRect.left = 0;
	sourceRect.top = 0;
	sourceRect.bottom = m_rendererHeight;
	sourceRect.right  = m_rendererWidth;

	destRect.left = 0;
	destRect.top = 0;
	if( Config::cvForcedRendererBackBufferResolution.Get() )
	{
		destRect.bottom  = Config::cvForcedRendererResolutionWidth.Get();
		destRect.right	 = Config::cvForcedRendererResolutionHeight.Get();
	}
	else
	{
		destRect.bottom  = m_fullHeight;
		destRect.right	 = m_fullWidth;
	}

	if( s_preserveSystemGamma == false )
	{
		Float gammaValue = 1.0f;
	#ifdef RED_PLATFORM_ORBIS
		gammaValue = Clamp<Float>( 1.f - (Config::cvGamma.Get() - 1.f), 0.1f, 1.9f ); // [0.1, 1.9]
	#else
		gammaValue = 1.f - (Config::cvGamma.Get() - 1.f) / 4.0f; // [0.75, 1.25]
	#endif
		if ( (m_updateGamma || gammaValue != m_currentGamma) && ( m_windowMode == VWM_Fullscreen ) )
		{
			GpuApi::SetGammaForSwapChain( m_swapChain, gammaValue );
			m_updateGamma = false;
			m_currentGamma = gammaValue;
		}
	}

#if defined( RED_PLATFORM_DURANGO )
#if MICROSOFT_ATG_DYNAMIC_SCALING
	//HACK use the dynamically rescaled resolution
	sourceRect.right = GpuApi::g_DynamicScaleWidthFullRes;
	sourceRect.bottom  = GpuApi::g_DynamicScaleHeightFullRes;
#endif
	if (multiplane)
	{
		GpuApi::PresentMultiplane( &sourceRect, &destRect, m_swapChain, m_swapChainOverlay, m_useVsync, (Uint32)Config::cvVSyncThreshold.Get() );
	}
	else
#endif
	{

#if defined( RED_PLATFORM_WINPC )
		// Allow vsync toggling at runtime on PC only
		if( m_useVsync != Config::cvVSync.Get() )
		{
			m_useVsync = Config::cvVSync.Get();
		}
#endif

		GpuApi::Present( &sourceRect, &destRect, m_swapChain, m_useVsync, (Uint32)Config::cvVSyncThreshold.Get() );
	}

#ifndef RED_PLATFORM_CONSOLE
	// Refresh window
	::InvalidateRect( m_window->GetWindowHandle(), nullptr, true );
#endif
}

Bool CRenderViewport::GetPixels( Uint32 x, Uint32 y, Uint32 width, Uint32 height, TDynArray< Color >& pixels )
{
	if ( IsCachet() )
	{
		x += m_x;
		y += m_y;
	}

	// Flush rendering thread so we have up to date data
	GRender->Flush();

	// Get backbuffer
	GpuApi::TextureRef backBuff = GpuApi::GetBackBufferTexture();
	if ( !backBuff )
	{
		ERR_RENDERER(TXT("Render buffer is not created."));
		return false;
	}

	// Test whether we have a valid pixels area
	GpuApi::TextureDesc backBuffDesc = GpuApi::GetTextureDesc( backBuff );
	if ( width < 1 || height < 1			||
		 x >= backBuffDesc.width			||	// Checking for the top left side avoids misses
		 y >= backBuffDesc.height			||  // caused from integer overflow when adding the
		 x + width > backBuffDesc.width		||  // width and height
		 y + height > backBuffDesc.height )
	{
		ERR_RENDERER(TXT("Invalid area. x = %u, y = %u, rtDim = %u x %u "), x, y, backBuffDesc.width, backBuffDesc.height);
		return false;
	}

	// Flush rendering thread so we have up to date data
	GRender->Flush();

	// Allocate pixels
	const Uint32 prevPixelsSize = pixels.Size();
	pixels.Resize( width * height );

	// Get pixels
	if ( !GpuApi::GrabTexturePixels( backBuff, x, y, width, height, pixels[0].RGBA, sizeof(Color), true ) )
	{
		pixels.Resize( prevPixelsSize ); // let's at least try to be fair :)
		return false;
	}

	// We're done :)
	return true;
}

void CRenderViewport::SetCheatedSize( Uint32 rendererWidth, Uint32 rendererHeight, Uint32 width, Uint32 height, Uint32 fullWidth, Uint32 fullHeight )
{
	m_rendererWidth = rendererWidth;
	m_rendererHeight = rendererHeight;
	m_width = width;
	m_height = height;
	m_fullWidth = fullWidth;
	m_fullHeight = fullHeight;
}

void CRenderViewport::SetMouseMode( EMouseMode mode, Bool notRestoreMousePosition /*= false*/ )
{
#ifndef RED_PLATFORM_CONSOLE
	if( mode == m_mouseMode && m_forceMouseModeRefresh == false )
	{
		return;
	}

	m_forceMouseModeRefresh = false;

	if( mode == MM_Capture  )
	{
		// Capture mouse and hide cursor
		::GetCursorPos( &m_capturedCursorPosition );
		::SetCapture( m_window->GetWindowHandle() );
		::ClipCursor( nullptr );
		SetCursorVisibility( false );
	}
	else if( mode == MM_Clip )
	{
		RECT rect;
		::GetWindowRect( m_window->GetWindowHandle(), &rect );
		::SetCapture( m_window->GetWindowHandle() );
		::ClipCursor( &rect );
	}
	else if ( mode == MM_ClipAndCapture )
	{
		RECT rect;
		// Capture mouse and hide cursor
		::GetCursorPos( &m_capturedCursorPosition );
		::GetWindowRect( m_window->GetWindowHandle(), &rect );
		::SetCapture( m_window->GetWindowHandle() );
		::ClipCursor( &rect );
		SetCursorVisibility( false );
	}
	else if( mode == MM_Normal )
	{
		if ( m_mouseMode == MM_Capture  )
		{
			// Uncapture mouse and restore cursor position
			::ReleaseCapture();
		} 
		else if ( m_mouseMode == MM_Clip )
		{
			// Release clipping window
			::ReleaseCapture();
			::ClipCursor( nullptr );
		}
		else if ( m_mouseMode == MM_ClipAndCapture )
		{
			// Uncapture mouse and restore cursor position
			::ReleaseCapture();

			// unclip
			::ClipCursor( nullptr );
		}

		SetCursorVisibility( true, notRestoreMousePosition );
	}
#endif

	m_mouseMode = mode;
}
#ifndef RED_PLATFORM_ORBIS
void CRenderViewport::SetTopLevelWindow( HWND handle )
{
	m_window->SetTopLevelWindow( handle );
}
#endif

#include "../engine/hwCursorManager.h"
void CRenderViewport::SetCursorVisibility( Bool state, Bool notRestoreMousePosition )
{
#ifndef RED_PLATFORM_CONSOLE
	if ( state )
	{
		if ( !notRestoreMousePosition )
		{
			::SetCursorPos( m_capturedCursorPosition.x, m_capturedCursorPosition.y );
			RED_LOG( Cursor, TXT("CRenderViewport SetCursorPos()") );
		}
	}

	GHardwareCursorManager.Viewport_RequestHardwareCursor( state );	

#endif
}

void CRenderViewport::Activate()
{
	if ( m_window && m_window->GetViewportWindowRoot() )
	{
		m_window->Activate();
	}
}

void CRenderViewport::Deactivate()
{
	if ( m_window && m_window->GetViewportWindowRoot() )
	{
		m_window->Deactivate();
	}
}

void CRenderViewport::RequestWindowSize(Uint32 width, Uint32 height)
{
	m_expectedProperties.width = width;
	m_expectedProperties.height = height;
}

void CRenderViewport::RequestOutputMonitor(Uint32 outputMonitorIndex)
{
	m_expectedProperties.outputMonitor = outputMonitorIndex;
}

void CRenderViewport::RequestWindowMode(EViewportWindowMode windowMode)
{
	if( GIsEditor == true )
	{
		windowMode = VWM_Borderless;
	}

	m_expectedProperties.fullscreen = (windowMode == VWM_Fullscreen);
	if( windowMode != m_windowMode )
	{
		m_expectedProperties.windowMode = windowMode;
	}
}

Bool CRenderViewport::NeedToResize()
{
	Bool result =	m_fullWidth != m_expectedProperties.width ||
					m_fullHeight != m_expectedProperties.height;

	return result;
}

void CRenderViewport::PerformResize()
{
#ifdef RED_PLATFORM_WINPC
	// Adjust to best matching resolution if in fullscreen
	if( m_expectedProperties.windowMode == VWM_Fullscreen )
	{
		Int32 matchingWidth;
		Int32 matchingHeight;
		if (Config::Helper::GetBestMatchingResolution( m_expectedProperties.width, m_expectedProperties.height, matchingWidth, matchingHeight ))
		{
			m_expectedProperties.width = matchingWidth;
			m_expectedProperties.height = matchingHeight;
		}
	}

	// Apply new resolution to parameters
	m_rendererWidth = m_expectedProperties.width;
	m_rendererHeight = m_expectedProperties.height;
	m_fullWidth = m_expectedProperties.width;
	m_fullHeight = m_expectedProperties.height;

	// Apply cachet if there is any
	Int32 cachetWidth = m_expectedProperties.width;
	Int32 cachetHeight = m_expectedProperties.height;
	GetSizeAdjustedToCachet( m_cachetAspectRatio, m_fullWidth, m_fullHeight, cachetWidth, cachetHeight );
	
	m_width = cachetWidth;
	m_height = cachetHeight;

	m_x = (Uint32)(ceilf( (m_fullWidth - m_width) / 2.0f ));
	m_y = (Uint32)(ceilf( (m_fullHeight - m_height) / 2.0f ));

	// 21:9 HACK++ 
	GetSizeAdjustedToCachet( EAspectRatio::FR_16_9, m_fullWidth, m_fullHeight, m_horizontalCachet, m_verticalCachet );
	// 21:9 HACK--

	// Resize backbuffer
	GpuApi::ResizeBackbuffer( m_rendererWidth, m_rendererHeight, m_swapChain );

	Rect rect;
#ifndef NO_EDITOR
	if ( GIsEditor )
	{
		rect.m_left		= 0;
		rect.m_top		= 0;
		rect.m_bottom	= m_expectedProperties.height;
		rect.m_right	= m_expectedProperties.width;
	}
	else
#endif
	{
		Int32 top		= 0;
		Int32 left		= 0;
		Int32 bottom	= m_expectedProperties.height;
		Int32 right		= m_expectedProperties.width;
		if ( !GpuApi::GetMonitorCoordinates( m_expectedProperties.outputMonitor, top, left, bottom, right ) )
		{
			WARN_RENDERER( TXT("Could not get monitor %u coordinates, using defaults [left: %d, right: %d, top: %d, bottom: %d]"),
				m_expectedProperties.outputMonitor,	left, right, top, bottom );
		}
		rect.m_left		= left;
		rect.m_top		= top;
		rect.m_right	= m_expectedProperties.width + left;
		rect.m_bottom	= m_expectedProperties.height + top;
		if ( m_expectedProperties.windowMode != VWM_Fullscreen )
		{
			// Try to plane in center
			Int32 offsetX = ( right - left ) / 2 - m_expectedProperties.width / 2;
			Int32 offsetY = ( bottom - top ) / 2 - m_expectedProperties.height / 2;
			rect.m_left		+= offsetX;
			rect.m_right	+= offsetX;
			rect.m_top		+= offsetY;
			rect.m_bottom	+= offsetY;
		}
	}
	m_window->AdjustRect( rect );

	if ( m_hook )
	{
		m_hook->OnViewportSetDimensions( this );
	}
#endif
}

void CRenderViewport::MinimizeOnExternalApplicationEvent()
{
#ifdef RED_PLATFORM_WINPC
	// If fullscreen state change was forced by swap chain itself, this means that some external application forces it.
	// Example is hipchat with notifications turned on, or Windows Color Scheme popup.
	// Alt+tab case is handled by WM_ACTIVATE in window class.
	Bool swapChainFullscreenState = GpuApi::GetFullscreenState( m_swapChain );
	Bool isScreenModeUpToDate = m_fullscreenMode == m_expectedProperties.fullscreen;

	// Minimize window only when non-fullscreen mode is triggered by DXGI (when external application forces it, but not when alt+tab)
	if( isScreenModeUpToDate == true && m_expectedProperties.fullscreen != swapChainFullscreenState && swapChainFullscreenState == false )
	{
		// Minimize window
		GetWindow()->OnExternalDeactivateEvent();
	}
#endif
}

Bool CRenderViewport::NeedToToggleFullscreen()
{
	Bool isScreenModeUpToDate = m_fullscreenMode == m_expectedProperties.fullscreen;

	Bool result = !isScreenModeUpToDate;
	return result;
}

Bool CRenderViewport::NeedToChangeWindowMode()
{
	Bool result = m_windowMode != m_expectedProperties.windowMode;
	return result;
}

Bool CRenderViewport::NeedToChangeOutputMonitor()
{
	Bool result = m_outputMonitor != m_expectedProperties.outputMonitor;
	return result;
}

void CRenderViewport::PerformToggleFullscreen()
{
	if( m_expectedProperties.fullscreen == true )
	{
		GpuApi::ToggleFullscreen( m_swapChain, m_expectedProperties.fullscreen, m_expectedProperties.outputMonitor );
	}
	else
	{
		GpuApi::ToggleFullscreen( m_swapChain, m_expectedProperties.fullscreen );
	}
	m_fullscreenMode = m_expectedProperties.fullscreen;
	m_outputMonitor = m_expectedProperties.outputMonitor;
}

void CRenderViewport::PerformChangeWindowMode()
{
#ifndef RED_PLATFORM_CONSOLE
	m_window->AdjustStyleToWindowMode( m_expectedProperties.windowMode );
	m_windowMode = m_expectedProperties.windowMode;
#endif
}

void CRenderViewport::TemporalWindowModeBegin()
{
	if( m_isInTemporaryWindowMode == false )
	{
		m_windowModeForRestore = m_windowMode;
		if( m_windowMode == VWM_Fullscreen )
		{
			RequestWindowMode( VWM_Windowed );
		}
		m_isInTemporaryWindowMode = true;
	}
}

void CRenderViewport::TemporalWindowModeEnd()
{
	if( m_isInTemporaryWindowMode == true )
	{
		RequestWindowMode( m_windowModeForRestore );
		m_isInTemporaryWindowMode = false;
	}
}
