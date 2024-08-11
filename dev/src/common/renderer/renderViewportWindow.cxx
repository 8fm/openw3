/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../core/tokenizer.h"
#include "../engine/inputDeviceTablet.h"
#include "../engine/inputDeviceManager.h"
#include "../engine/inputEditorInterface.h"

#include "renderViewportWindow.h"
#include "renderViewportWindowRoot.h"
#include "renderViewport.h"

#ifndef NO_RED_GUI
#	include "../core/algorithms.h"
#	include "../engine/redGuiManager.h"
#	include "../engine/renderSettings.h"
#endif	// NO_RED_GUI

#include "../engine/baseEngine.h"
#include "../engine/videoPlayer.h"

#if defined(USE_RAW_INPUT_FOR_KEYBOARD_DEVICE)
#include "../../win32/platform/inputDeviceManagerWin32.h"
#endif

#ifndef RED_PLATFORM_ORBIS

// This is defined in r4game's resource.h and game.rc
#define WIN32_ICON_RESOURCE_NUMBER 106

CViewportWindow::CViewportWindow( CViewportWindowRoot* viewportWindowRoot, HWND topLevelWindow, HWND parent, const String& caption, Uint32 width, Uint32 height, Uint32 windowMode, Bool isOwnedByRenderThread )
	: m_viewportWindowRoot( viewportWindowRoot )
	, m_localMousePos()
	, m_parentHandle( parent )
	, m_windowHandle( NULL )
	, m_topLevelHandle( topLevelWindow )
	, m_viewport( NULL )
	, m_width( width )
	, m_height( height )
	, m_isOwnedByRenderThread( isOwnedByRenderThread )
	, m_windowMode( windowMode )
{
	// Setup styles
	Uint32 exStyle, mainStyle;

	// Calculate optimal window placement
	RECT windowRect;
	if ( parent )
	{
		// Child window, no fancy placement
		windowRect.left = 0;
		windowRect.top = 0;
		windowRect.right = width;
		windowRect.bottom = height;

		// No fancy styles
		exStyle = WS_EX_TRANSPARENT;
		mainStyle = WS_CHILD;
	}
	else if ( windowMode == VWM_Fullscreen )
	{
		// Take the full screen
		windowRect.left = 0;
		windowRect.top = 0;
		windowRect.right = width;
		windowRect.bottom = height;

		// Fullscreen window
		exStyle = WS_EX_TOPMOST;
		mainStyle = WS_POPUP;
	}

#ifndef RED_PLATFORM_CONSOLE
	else
	{
		// Get size of desktop
		Int32 desktopX = ::GetSystemMetrics( SM_CXSCREEN );
		Int32 desktopY = ::GetSystemMetrics( SM_CYSCREEN );

		if ( windowMode == VWM_Windowed )
		{
			// Try to plane in center
			windowRect.left = Max<Int32>( 0, desktopX/2 - width/2 );
			windowRect.top = Max<Int32>( 0, desktopY/2 - height/2 );
			windowRect.right = windowRect.left + width;
			windowRect.bottom = windowRect.top + height;

			// Normal floating window
			exStyle = WS_EX_APPWINDOW;
			mainStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
		}
		else if ( windowMode == VWM_Borderless )
		{
			// Default position is at 0,0
			windowRect.left = 0;
			windowRect.top = 0;

			// Check for position override
			CTokenizer tokenizer( SGetCommandLine(), TXT(" ") );
			for ( Uint32 i=0; i < tokenizer.GetNumTokens(); ++i )
			{
				if ( tokenizer.GetToken(i) == TXT("-position") && i + 1 < tokenizer.GetNumTokens() )
				{
					String left, right;
					if ( tokenizer.GetToken( i + 1 ).Split( TXT(","), &left, &right ) )
					{
						Uint32 x, y;
						if ( ::FromString( left, x ) && ::FromString( right, y ) )
						{
							windowRect.left = x;
							windowRect.top = y;
						}
					}				
				}
			}

			// Calculate right/bottom
			windowRect.right = windowRect.left + width;
			windowRect.bottom = windowRect.top + height;

			// Fullscreen window
			exStyle = 0;
			mainStyle = WS_POPUP;
		}
	}

	// Adjust window rectangle with border size
	::AdjustWindowRect( &windowRect, mainStyle, false );

	// Register window class
	static Bool sRegisterWindowClass = true;
	if ( sRegisterWindowClass )
	{
		WNDCLASSEX info;
		Red::System::MemorySet( &info, 0, sizeof( WNDCLASSEX ) );
		info.cbSize = sizeof( WNDCLASSEX );

		// Assemble class info
		info.cbWndExtra = 8;
		info.hbrBackground = (HBRUSH)GetStockObject( NULL_BRUSH );
		info.hCursor = GIsEditor ? ::LoadCursor( NULL, IDC_CROSS ) : ::LoadCursor( GetModuleHandle(NULL), MAKEINTRESOURCEW(102) ); // IDC_GAMECURSOR
		info.hInstance = ::GetModuleHandle( NULL );
		info.hIcon = parent ? NULL : ::LoadIconW( info.hInstance, MAKEINTRESOURCEW( WIN32_ICON_RESOURCE_NUMBER ) );
		info.lpfnWndProc = reinterpret_cast< WNDPROC >( &CViewportWindow::StaticWndProc );
		info.lpszClassName = TXT("W2ViewportClass");
		info.lpszMenuName = NULL;
		info.style = CS_VREDRAW | CS_HREDRAW;

		// Register class
		VERIFY( ::RegisterClassEx( &info ) );
		sRegisterWindowClass = false;
	}

	// Create window
	HWND handle = ::CreateWindowEx( 
		exStyle,
		TXT("W2ViewportClass"), 
		caption.AsChar(), 
		mainStyle | WS_CLIPCHILDREN, 
		windowRect.left,
		windowRect.top, 
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		m_parentHandle, 
		NULL, 
		::GetModuleHandle( NULL ),
		this
	);

	// Make sure window has been bound
	VERIFY( handle && m_windowHandle == handle );

	// Bring to front 
	::SetForegroundWindow( m_windowHandle );
	::SetFocus( m_windowHandle );
	::UpdateWindow( m_windowHandle );
	::ShowWindow( m_windowHandle, SW_SHOW );
	::SendMessage( m_windowHandle, WM_ERASEBKGND, 0, 0 );
#endif
}

CViewportWindow::~CViewportWindow()
{
	if ( m_viewportWindowRoot->IsViewportActive( this ) )
	{
		Deactivate();
	}

	// Close window
	if ( m_windowHandle )
	{
#ifndef RED_PLATFORM_CONSOLE
		DestroyWindow( m_windowHandle );
#endif
		m_windowHandle = NULL;
	}
}

#ifndef RED_PLATFORM_CONSOLE
void CViewportWindow::ProcessDelayedMessages()
{
	// Processing a message can have the WndProc called again by the system
	static Bool recursionCheck = false;
	RED_FATAL_ASSERT( ::SIsMainThread(), "Must process delayed messages on main thread!" );

	extern Bool GPumpingMessagesOutsideMainLoop;
	if ( !GPumpingMessagesOutsideMainLoop && !recursionCheck )
	{
		Red::System::ScopedFlag<Bool> flag( recursionCheck = true, false );

		// Swap so can still delay any new messages generated by the WndProc without
		// altering the array being iterated over
		::Swap( m_delayedMessagesInUse, m_delayedMessages );
		for ( const WndProcMsg& msg : m_delayedMessagesInUse )
		{
			WndProc( msg.m_uMsg, msg.m_wParam, msg.m_lParam );
		}

		m_delayedMessagesInUse.ResizeFast( DELAYED_MESSAGE_RESERVE );
		m_delayedMessagesInUse.ClearFast();
	}
}
#endif

#ifndef RED_PLATFORM_CONSOLE
Bool CViewportWindow::DelayWndProc( UINT uMsg, WPARAM wParam, LPARAM lParam, LONG& outReturnValue )
{
	Bool delay = false;
	extern Bool GPumpingMessagesOutsideMainLoop;
	if ( GPumpingMessagesOutsideMainLoop )
	{
		switch (uMsg)
		{
			/* fall through game and editor events */
		case WM_CANCELMODE:
		case WM_ACTIVATE:
		case WM_MOVE:
		case WM_SIZE:
			delay = true;
			break;

			/* fall through editor events */
		case WM_KILLFOCUS:
		case WM_SETFOCUS:
		case WM_MOUSELEAVE:
			delay = true;
			break;

		default:
			break;
		}
	}

	if ( delay )
	{
		const WndProcMsg delayedMsg = { uMsg, wParam, lParam };
		m_delayedMessages.PushBack( delayedMsg );
		outReturnValue = 0; // the above events all return zero to indicate we handled the message
	}
	
	return delay;
}

void CViewportWindow::AdjustStyleToWindowMode(EViewportWindowMode windowMode)
{
	if( GIsEditor == true )
	{
		return;
	}

	Uint64 exStyle = ::GetWindowLongPtr( GetWindowHandle(), GWL_EXSTYLE );
	Uint64 mainStyle = ::GetWindowLongPtr( GetWindowHandle(), GWL_STYLE );
	Uint32 windowFlags = SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOREPOSITION | SWP_NOSENDCHANGING | SWP_NOSIZE | SWP_NOZORDER;

	if( windowMode == VWM_Windowed )
	{
		exStyle = WS_EX_APPWINDOW;
		mainStyle |= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
		mainStyle &= ~WS_POPUP;
	}
	else if( windowMode == VWM_Borderless)
	{
		exStyle = 0;
		mainStyle &= ~(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
		mainStyle |= WS_POPUP;
	}
	else	// if( windowMode == VWM_Fullscreen )
	{
		exStyle = 0;
		mainStyle &= ~(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
		mainStyle |= WS_POPUP;
	}

	::SetWindowLongPtr( GetWindowHandle(), GWL_EXSTYLE, exStyle );
	::SetWindowLongPtr( GetWindowHandle(), GWL_STYLE, mainStyle );
	::SetWindowPos( GetWindowHandle(), 0, 0, 0, 0, 0, windowFlags );
}

void CViewportWindow::SetZOrderAsTopMost()
{
	// Change z-order only
	Uint32 windowFlags = SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOREPOSITION | SWP_NOSENDCHANGING | SWP_NOSIZE;
	::SetWindowPos( GetWindowHandle(), HWND_TOPMOST, 0, 0, 0, 0, windowFlags );
}

void CViewportWindow::SetZOrderAsNonTopMost()
{
	// Change z-order only
	Uint32 windowFlags = SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOREPOSITION | SWP_NOSENDCHANGING | SWP_NOSIZE;
	::SetWindowPos( GetWindowHandle(), HWND_NOTOPMOST, 0, 0, 0, 0, windowFlags );
}
#endif  // RED_PLATFORM_CONSOLE

LONG CViewportWindow::WndProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	PC_SCOPE_PIX( WndProc );
	// Process message
	switch ( uMsg )
	{
		// Window is about to be destroyed
		case WM_DESTROY:
		{
#ifndef RED_PLATFORM_CONSOLE
			// Unlink
			SetWindowLongPtr( m_windowHandle, GWLP_USERDATA, NULL );
#endif
			break;
		}

		// Window is closing
		case WM_CLOSE:
		{
			GEngine->RequestExit();
			break;
		}

#ifndef RED_PLATFORM_CONSOLE
		case WM_CANCELMODE:
		{
#ifdef RED_ASSERTS_ENABLED
			// Activation handling is disabled
			if ( GetViewport() != nullptr && GetViewport()->IsActivationHandlingDisabled() )
			{
				break;
			}
#endif

			if ( GetViewport() )
			{
				GetViewport()->SetMouseMode( MM_Normal );
			}

			return 0;
		}
#endif

#if defined(USE_RAW_INPUT_FOR_KEYBOARD_DEVICE)
		case WM_INPUT:
			{
				if( GEngine != nullptr )
				{
					CInputDeviceManagerWin32* inputDeviceManager = static_cast< CInputDeviceManagerWin32* >( GEngine->GetInputDeviceManager() );
					if( inputDeviceManager != nullptr )
					{
						inputDeviceManager->OnWindowsInput( wParam, lParam );
						return 0;
					}
				}
				break;
			}
#endif

		// Window was (de)activated
		case WM_ACTIVATE:
		{
			// if activated by clicking, don't process
			if ( LOWORD( wParam ) == WA_CLICKACTIVE )
			{
				return 0;
			}

#ifdef RED_ASSERTS_ENABLED
			// Activation handling is disabled
			if ( GetViewport() != nullptr && GetViewport()->IsActivationHandlingDisabled() )
			{
				break;
			}
#endif
			const Bool isActivated = LOWORD( wParam ) != 0;
			HandleActivation( isActivated );

			return 0;
		}

		// Window has changed size
		case WM_SIZE:
		{
		#if defined(RED_PLATFORM_WINPC) && !defined(RED_FINAL_BUILD)
			// When switching from fullscreen to other modes, DXGI sends WM_SIZE event and changes out resolution to whatever was set before getting into fullscreen
			// so we want to ignore that event when VWM_Fullscreen
			if( GIsEditor && GetViewport() != nullptr && GetViewport()->GetViewportWindowMode() != VWM_Fullscreen )
			{
				RECT winRect;
				::GetClientRect( m_windowHandle, &winRect );
				GetViewport()->RequestWindowSize(winRect.right - winRect.left, winRect.bottom - winRect.top);
			}
		#endif

			break;
		}

#ifdef RED_PLATFORM_WINPC
		case WM_GETMINMAXINFO:
			{
				if( m_width > 0 && m_height > 0 )
				{
					// By default windows will prevent a window from being larger than the resolution of the monitor it's displayed on
					// So we will calculate the size of the border decorations and make sure the window size will be the correct size
					// for our requested client size (1080p or whatever)
					PMINMAXINFO info = reinterpret_cast< PMINMAXINFO >( lParam );

					RECT clientSize, windowSize;
					GetClientRect( m_windowHandle, &clientSize );
					GetWindowRect( m_windowHandle, &windowSize );
					Int32 borderWidth  = ( windowSize.right - windowSize.left ) - ( clientSize.right - clientSize.left );
					Int32 borderHeight = ( windowSize.bottom - windowSize.top ) - ( clientSize.bottom - clientSize.top );



					Int32 maxX = borderWidth + m_width;
					Int32 maxY = borderHeight + m_height;

					info->ptMaxSize.x = maxX;
					info->ptMaxSize.y = maxY;
					info->ptMaxTrackSize.x = maxX;
					info->ptMaxTrackSize.y = maxY;
				}
			}

			break;
#endif

		// Set cursor
		case WM_SETCURSOR:
		{
			break;
		}

		// Erase tylko kaszet
		case WM_ERASEBKGND:
		{
#ifndef RED_PLATFORM_CONSOLE
			if( !m_viewport )
			{
				// We can erase everything, no viewport
				PAINTSTRUCT ps;

				HDC dc = ::BeginPaint( m_windowHandle, &ps );

				RECT rc;
				rc.left = 0;
				rc.top = 0;
				rc.bottom = m_height;
				rc.right = m_width;

				::FillRect( dc, &rc, (HBRUSH)::GetStockObject( BLACK_BRUSH ) );

				// Done painting
				::EndPaint( m_windowHandle, &ps );
			}
#endif
			return 1;
		}

		// Paint message
		case WM_PAINT:
		{
#ifndef RED_PLATFORM_CONSOLE
			::ValidateRect( m_windowHandle, nullptr );
#endif
			return 0;
		}

		// Kill focus
		case WM_KILLFOCUS:
		{
			HandleKillFocusEvent();
			break;
		}

		// Set focus
		case WM_SETFOCUS:
		{
			HandleSetFocusEvent();
			break;
		}

		case WM_MOUSEACTIVATE:
		{
#ifndef RED_PLATFORM_CONSOLE

			// Reset mouse position
			::GetCursorPos( &m_localMousePos );
			::ScreenToClient( m_windowHandle, &m_localMousePos );

			// If the input is already captured, do not recapture it, some other system apparently requires it

			//FIXME2<<< Unconditionally track

			// If we enter here it means that neither mouse nor keyboard is acquired
			// Equivalent of WM_MOUSEENTER which doesn't exist
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = m_windowHandle;

			::TrackMouseEvent( &tme );

			::SetFocus( m_topLevelHandle ? m_topLevelHandle : m_windowHandle );		
#endif

			Int32 titleBarClicked = LOWORD( lParam );
			if ( titleBarClicked == HTCAPTION )
			{
				return MA_NOACTIVATE;
			}

			HandleActivation( true );			
			break;
		}

		case WM_EXITSIZEMOVE:
		{
			// Skip if fullscreen and borderless
			if( GetViewport() != nullptr && GetViewport()->GetViewportWindowMode() != VWM_Windowed )
			{
				break;
			}

			HandleActivation( true );
			return 0;
		}

		case WM_MOUSELEAVE:
		{
			if ( ! m_isOwnedByRenderThread )
			{
				// HACK: For ForegroundNonExclusive mouse mode so the game stops when you mouse out of the window
				// It doesn't get the WM_KILLFOCUS at this level
				HandleKillFocusEvent();

				// Don't deactivate in r4Launcher when leaving the viewport. Also, can introduce input loss when you deactivate
				// the window by clicking on another app window or desktop, then click on the game's titlebar to reactivate it
				// but we can't WM_MOUSEACTIVATE it now

				Deactivate();
			}
			
			break;
		}

			// We want to handle all keys (beep fix)
		case WM_GETDLGCODE:
		{
			return DLGC_WANTALLKEYS;
		}
	}
#ifndef RED_PLATFORM_CONSOLE
	// Default processing
	return static_cast< LONG >( DefWindowProc( m_windowHandle, uMsg, (WPARAM)wParam, (LPARAM)lParam ) );
#else
	return 0;
#endif
}

void CViewportWindow::ActivateTabletInput( Bool isActivated )
{
#ifndef NO_TABLET_INPUT_SUPPORT

	/* This call puts your context (active area) on top the */
	/* context overlap order. */
	IInputDeviceManager* inputDeviceManager = GEngine ? GEngine->GetInputDeviceManager() : nullptr;
	if ( inputDeviceManager && inputDeviceManager->GetEditorInterface() )
	{
		IInputDeviceTablet* inputDeviceTablet = inputDeviceManager->GetEditorInterface()->GetTablet();
		if ( inputDeviceTablet )
		{
			inputDeviceTablet->SetEnabled( isActivated );
		}
	}			
#endif // !NO_TABLET_INPUT_SUPPORT
}

LONG APIENTRY CViewportWindow::StaticWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
#ifndef RED_PLATFORM_CONSOLE
	// Window creation
	if ( uMsg == WM_CREATE )
	{
		LPCREATESTRUCT data = (LPCREATESTRUCT)lParam;
		::SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)data->lpCreateParams );
		CViewportWindow *window = (CViewportWindow *)data->lpCreateParams;
		window->m_windowHandle = hWnd;
	}

	// Process messages by window message function
	CViewportWindow *window = (CViewportWindow *) ::GetWindowLongPtr( hWnd, GWLP_USERDATA );
	if ( window )
	{
		ASSERT( window->m_windowHandle == hWnd );

		window->ProcessDelayedMessages();

		LONG retval = 0;
		if ( !window->DelayWndProc( uMsg, wParam, lParam, retval ) )
		{
			retval = window->WndProc( uMsg, wParam, lParam );
		}
		return retval;
	}
	else
	{
		return static_cast< LONG >( DefWindowProc( hWnd, uMsg, wParam, lParam ) );
	}
#else
	return 0;
#endif
}

void CViewportWindow::HandleActivation( Bool isActivated )
{
#ifdef RED_ASSERTS_ENABLED
	// Activation handling is disabled
	if ( GetViewport() != nullptr && GetViewport()->IsActivationHandlingDisabled() )
	{
		return;
	}
#endif

	ActivateTabletInput( isActivated );

#ifndef RED_PLATFORM_CONSOLE
	// Deactivated
	if ( isActivated == false )
	{
		CRenderViewport* viewport = GetViewport();

		// Temporarily switch to windowed mode
		if ( viewport != nullptr )
		{
			viewport->TemporalWindowModeBegin();
			if( viewport->GetViewportWindowMode() == VWM_Fullscreen )
			{
				SetZOrderAsNonTopMost();

				// Minimize it
				::ShowWindow( m_windowHandle, SW_MINIMIZE );
			}
		}

		Deactivate();
	}
	else // Activated
	{
		CRenderViewport* viewport = GetViewport();

		// Restore window mode
		if ( viewport != nullptr )
		{
			viewport->TemporalWindowModeEnd();

			if( viewport->GetViewportWindowMode() == VWM_Fullscreen )
			{
				SetZOrderAsTopMost();
			}
			::ShowWindow( m_windowHandle, SW_RESTORE );
		}

		Activate();
	}

	// Inform the game
	SendViewportActivatedEvent( isActivated );
#endif // !RED_PLATFORM_CONSOLE
}

void CViewportWindow::Bind( CRenderViewport* viewport )
{
	ASSERT( viewport );
	ASSERT( !m_viewport );
	m_viewport = viewport;
}

void CViewportWindow::AdjustRect( const Rect& newRect )
{
	Uint32 flags = 0;
	flags |= SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER;  
	flags |= SWP_NOCOPYBITS;

	m_width = newRect.m_right - newRect.m_left;
	m_height = newRect.m_bottom - newRect.m_top;
#ifndef RED_PLATFORM_CONSOLE
	// Update window
	MemUint mainStyle = ::GetWindowLongPtr( GetWindowHandle(), GWL_STYLE );

	RECT adjustedSize;
	adjustedSize.left = newRect.m_left;
	adjustedSize.right = newRect.m_right;
	adjustedSize.top = newRect.m_top;
	adjustedSize.bottom = newRect.m_bottom;

	// Take into account the window borders
	::AdjustWindowRect( &adjustedSize, static_cast< DWORD >( mainStyle ), false );

	// AdjustWindowRect() will expand the rect size in all 4 directions to keep the client area in the same position
	// So we'll make sure the window title bar stays put instead (to avoid it going off the top of the screen)
	Int32 windowLeft = adjustedSize.left;
	Int32 windowTop = adjustedSize.top;
	Int32 windowWidth = adjustedSize.right - adjustedSize.left;
	Int32 windowHeight = adjustedSize.bottom - adjustedSize.top;

	::SetWindowPos( m_windowHandle, 0, windowLeft, windowTop, windowWidth, windowHeight, flags );
	::SendMessage( m_windowHandle, WM_ERASEBKGND, 0, 0 );
	::InvalidateRect( m_windowHandle, NULL, true );
#endif
}

void CViewportWindow::OnExternalDeactivateEvent()
{
	HandleActivation( false );
}

Bool CViewportWindow::HandleMouseMoveEditorPacket( Int32 x, Int32 y, Float pressure )
{
	Bool processed = false;

	// Handle mouse events only if we have viewport set
	if ( GetViewport() )
	{
#if !defined( RED_FINAL_BUILD ) || !defined( NO_RED_GUI )
		IInputDeviceManager* inputDeviceManager = GEngine ? GEngine->GetInputDeviceManager() : nullptr;

		// Calculate mouse delta
		const Float dx = Float(x - m_localMousePos.x);
		const Float dy = Float(y - m_localMousePos.y);

		// No mouse movement
		if ( dx || dy )
		{
			//FIXME2<<< no editor...
# ifndef NO_RED_GUI
			if( GRedGui::GetInstance().GetEnabled() == true )
			{
				Float mousePosScaleX = 1.0f;
				Float mousePosScaleY = 1.0f;
				if( Config::cvForcedRendererOverlayResolution.Get() )
				{
					mousePosScaleX =(Float)Config::cvForcedRendererResolutionWidth.Get()/(Float)m_width;
					mousePosScaleY =(Float)Config::cvForcedRendererResolutionHeight.Get()/(Float)m_height;
				}
				CMousePacket packet( GetViewport(), (Int32)(x*mousePosScaleX), (Int32)(y*mousePosScaleY), (Int32)dx, (Int32)dy, pressure );
				if( GRedGui::GetInstance().OnViewportTrack( packet ) == true )
				{
					return true;
				}
			}
# endif	// NO_RED_GUI

# ifndef RED_FINAL_BUILD
			// We do not support track and move events in windows owned by render thread
			if ( !m_isOwnedByRenderThread )
			{
				// Mouse packet
				CMousePacket packet( GetViewport(), x, y, (Int32)dx, (Int32)dy, pressure );

				// Track event
				if ( GetViewport()->GetMouseMode() == MM_Normal )
				{
					// Send raw input event to viewport hook
					IViewportHook* hook = GetViewport()->GetViewportHook();
					if ( hook )
					{
						processed = hook->OnViewportTrack( packet );
					}
				}
				else 
				{
					// Send raw input event to viewport hook
					IViewportHook* hook = GetViewport()->GetViewportHook();
					if ( hook )
					{
						processed = hook->OnViewportMouseMove( packet );
					}
				}
			}
# endif // !RED_FINAL_BUILD
		}
#endif // !defined( RED_FINAL_BUILD ) || !defined( NO_RED_GUI )

		// Send delta movements
// 		processed |= HandleInputEvent( IK_MouseX, IACT_Axis, dx );
// 		processed |= HandleInputEvent( IK_MouseY, IACT_Axis, dy );
	}

	return processed;
}

Bool CViewportWindow::HandleMouseEvent( Int32 button, Bool state, Int32 x, Int32 y )
{
	Bool processed = false;

	// Send click event to viewport hook. We do not support track and move events in windows owned by render thread

	//HACK E3 DEMO m_isOwnedByRenderThread is not checked to enable teleport
	if ( /*!m_isOwnedByRenderThread && */GetViewport() )
	{
		IViewportHook* hook = GetViewport()->GetViewportHook();
		if (  hook )
		{
			processed = hook->OnViewportClick( GetViewport(), button, state, x, y );
		}
	}

	// Send input event
	EInputKey mouseButtons[] = { IK_LeftMouse, IK_RightMouse, IK_MiddleMouse };
	processed |= HandleInputEvent( mouseButtons[ button ], state ? IACT_Press : IACT_Release, state ? 1.0f : 0.0f );

	return processed;
}

Bool CViewportWindow::HandleInputEvent( EInputKey key, EInputAction action, Float data )
{
	// Send raw input event to viewport hook
	IViewport* vp = GetViewport();
	if ( vp )
	{
		IViewportHook* hook = vp->GetViewportHook();
		if (  hook )
		{
			return hook->OnViewportInput( vp, key, action, data );
		}
	}

	return false;
}

void CViewportWindow::HandleKillFocusEvent()
{
	// Send event to viewport hook. We do not support track and move events in windows owned by render thread
	if ( !m_isOwnedByRenderThread && GetViewport() )
	{
		IViewportHook* hook = GetViewport()->GetViewportHook();
		if (  hook )
		{
			hook->OnViewportKillFocus( GetViewport() );
		}
	}
}

void CViewportWindow::HandleSetFocusEvent()
{
	// Send event to viewport hook. We do not support track and move events in windows owned by render thread
	if ( !m_isOwnedByRenderThread && GetViewport() )
	{
		IViewportHook* hook = GetViewport()->GetViewportHook();
		if (  hook )
		{
			hook->OnViewportSetFocus( GetViewport() );
		}
	}
}

void CViewportWindow::HandleGeometryChangeEvent()
{
#ifndef RED_PLATFORM_CONSOLE
	RECT winRect;
	::GetClientRect( m_windowHandle, &winRect );

	// Send raw event to viewport hook
	if ( GetViewport() )
	{
		GetViewport()->AdjustSize(winRect.right - winRect.left, 
			winRect.bottom - winRect.top);
	}
#endif
}

void CViewportWindow::SendViewportActivatedEvent( Bool activated )
{
	RED_ASSERT( ::SIsMainThread(), TXT("Tried to send viewport activation event from another thread!") );

	if ( GetViewport() != nullptr && GetViewport()->GetViewportHook() != nullptr )
	{
		if ( activated )
		{
			GetViewport()->GetViewportHook()->OnViewportActivated( GetViewport() );
		}
		else
		{
			GetViewport()->GetViewportHook()->OnViewportDeactivated( GetViewport() );
		}
	}
}

#include "../engine/hwCursorManager.h"

#ifdef RED_PLATFORM_WINPC
static void UpdateHWCursor( CViewportWindow* window )
{
	POINT mousePos;
	if ( GHardwareCursorManager.GetRequestMouseMove().IsValid() )
	{
		const HWND hwnd = window->GetWindowHandle();
		const SCursorPoint& requestMouseMove = GHardwareCursorManager.GetRequestMouseMove();
		RECT clientRect;
		::GetClientRect(hwnd, &clientRect);
		mousePos.x = static_cast<LONG>((clientRect.left + clientRect.right) * requestMouseMove.m_x);
		mousePos.y = static_cast<LONG>((clientRect.top + clientRect.bottom) * requestMouseMove.m_y);
		GHardwareCursorManager.ClearRequestMouseMove();
		::ClientToScreen( hwnd, &mousePos );
		::SetCursorPos( mousePos.x, mousePos.y );
		const IViewport* const vp = window->GetViewport();
		GHardwareCursorManager.OnMouseMove( Float( mousePos.x ) / Float( vp->GetFullWidth() ), Float( mousePos.y ) / Float( vp->GetFullHeight() ) );
	}

	GHardwareCursorManager.Update();
}
#endif // RED_PLATFORM_WINPC

Bool CViewportWindow::ProcessInput( const BufferedInput& input )
{
	// Update regardless of pending input
#ifdef RED_PLATFORM_WINPC
	UpdateHWCursor( this );
#endif

	Bool mouseMoveProcessed = false;
	Bool inputProcessed = false;

	//FIXME2<<< Such a mess. Editor and game input intermixed here...

	for( Uint32 inputIndex = 0; inputIndex < input.Size(); ++inputIndex )
	{
		SBufferedInputEvent it = input[ inputIndex ];

		const EInputKey key = it.key;
		const EInputAction action = it.action;
		const Bool state = action == IACT_Press ? true : false;

#ifdef RED_PLATFORM_WINPC
		POINT mousePos;
		::GetCursorPos( &mousePos );
		::ScreenToClient( m_windowHandle, &mousePos ); 

		if( key == IK_MouseX || key == IK_MouseY )
		{
			GHardwareCursorManager.OnMouseMove( Float( mousePos.x ) / Float( GetViewport()->GetFullWidth() ), Float( mousePos.y ) / Float( GetViewport()->GetFullHeight() ) );
		}
#elif defined( RED_PLATFORM_DURANGO )
		extern SRawMouseReading DurangoHackGetMouseReading();
		SRawMouseReading mousePos = DurangoHackGetMouseReading();
#endif

		if( key == IK_MouseX || key == IK_MouseY )
		{
			if( !mouseMoveProcessed )
			{
				mouseMoveProcessed = true;
				const Float pressure = it.data;

#if defined( RED_PLATFORM_WINPC ) || ( ! defined( RED_FINAL_BUILD ) && defined( RED_PLATFORM_DURANGO ) )
				inputProcessed = HandleMouseMoveEditorPacket( (Int32)mousePos.x, (Int32)mousePos.y, pressure );
				m_localMousePos = mousePos;
#endif
			}

			// Handle mouse events only if we have viewport set
			if ( GetViewport() )
			{
				// Send delta movements
				inputProcessed |= HandleInputEvent( key, IACT_Axis, it.data );
				inputProcessed |= HandleInputEvent( key, IACT_Axis, it.data );
			}
		}
		else if( key == IK_LeftMouse )
		{
#if defined( RED_PLATFORM_WINPC ) || ( ! defined( RED_FINAL_BUILD ) && defined( RED_PLATFORM_DURANGO ) )
			inputProcessed = HandleMouseEvent( 0, state, (Int32)mousePos.x, (Int32)mousePos.y );
#endif
		}
		else if( key == IK_MiddleMouse )
		{
#if defined( RED_PLATFORM_WINPC ) || ( ! defined( RED_FINAL_BUILD ) && defined( RED_PLATFORM_DURANGO ) )
			inputProcessed = HandleMouseEvent( 2, state, (Int32)mousePos.x, (Int32)mousePos.y );
#endif
		}
		else if( key == IK_RightMouse )
		{
#if defined( RED_PLATFORM_WINPC ) || ( ! defined( RED_FINAL_BUILD ) && defined( RED_PLATFORM_DURANGO ) )
			inputProcessed = HandleMouseEvent( 1, state, (Int32)mousePos.x, (Int32)mousePos.y );
#endif
		}
		else
		{
			inputProcessed = HandleInputEvent( key, action, it.data );
		}
	}

	return inputProcessed;
}

void CViewportWindow::Activate()
{
	if ( m_viewportWindowRoot )
	{
		m_viewportWindowRoot->Activate( this );
	}
}

void CViewportWindow::Deactivate()
{
	if ( m_viewportWindowRoot )
	{
		m_viewportWindowRoot->Deactivate( this );
	}
}

#else // ORBIS
CViewportWindow::CViewportWindow( CViewportWindowRoot* viewportWindowRoot, const String& caption, Uint32 width, Uint32 height, Uint32 windowMode, Bool isOwnedByRenderThread )
	: m_viewportWindowRoot( viewportWindowRoot )
	, m_viewport( NULL )
	, m_width( width )
	, m_height( height )
	, m_isOwnedByRenderThread( isOwnedByRenderThread )
{
	RED_UNUSED( windowMode );
}

CViewportWindow::~CViewportWindow()
{
}

void CViewportWindow::Bind( CRenderViewport* viewport )
{
	ASSERT( viewport );
	ASSERT( !m_viewport );
	m_viewport = viewport;
}

Bool CViewportWindow::HandleInputEvent( EInputKey key, EInputAction action, Float data )
{
	// Send raw input event to viewport hook
	IViewport* vp = GetViewport();
	if ( vp )
	{
		IViewportHook* hook = vp->GetViewportHook();
		if (  hook )
		{
			return hook->OnViewportInput( vp, key, action, data );
		}
	}

	return false;
}

Bool CViewportWindow::ProcessInput( const BufferedInput& input )
{
	Bool inputProcessed = false;

	for( Uint32 inputIndex = 0; inputIndex < input.Size(); ++inputIndex )
	{
		SBufferedInputEvent it = input[ inputIndex ];

		const EInputKey key = it.key;
		const EInputAction action = it.action;
		const Bool state = action == IACT_Press ? true : false;

#if !defined( RED_FINAL_BUILD )
#ifndef NO_RED_GUI
		static Int32 editorMouseX = 0;
		static Int32 editorMouseY = 0;
		Bool guiConsumedInput = false;
		if( GRedGui::GetInstance().GetEnabled() )
		{
			if ( key == IK_MouseX )
			{
				editorMouseX += (Int32)it.data;

#ifdef RED_PLATFORM_ORBIS
				editorMouseX = Clamp( editorMouseX, 0, static_cast< Int32 >( m_width ) );
#endif

				CMousePacket packet( GetViewport(), editorMouseX, editorMouseY, (Int32)it.data, 0, 1.f );
				guiConsumedInput = GRedGui::GetInstance().OnViewportTrack( packet );
			}
			else if ( key == IK_MouseY )
			{
				editorMouseY += (Int32)it.data;

#ifdef RED_PLATFORM_ORBIS
				editorMouseY = Clamp( editorMouseY, 0, static_cast< Int32 >( m_height ) );
#endif

				CMousePacket packet( GetViewport(), editorMouseX, editorMouseY, 0, (Int32)it.data, 1.f );
				guiConsumedInput = GRedGui::GetInstance().OnViewportTrack( packet );
			}
			else if ( key == IK_LeftMouse || key == IK_MiddleMouse || key == IK_RightMouse )
			{
				Int32 button = key - 1;
				if(GRedGui::GetInstance().GetEnabled() == true)
				{
					guiConsumedInput = GRedGui::GetInstance().OnViewportClick( GetViewport(), button, state, editorMouseX, editorMouseY );
				}				
			}
		}	

		inputProcessed |= guiConsumedInput;

		if ( !guiConsumedInput )
#endif // NO_RED_GUI
#endif // !defined( RED_FINAL_BUILD ) || defined( DEMO_BUILD )
		{
			inputProcessed = HandleInputEvent( key, action, it.data );
		}
	}

	return inputProcessed;
}

void CViewportWindow::Activate()
{
	if ( m_viewportWindowRoot )
	{
		m_viewportWindowRoot->Activate( this );
	}
}

void CViewportWindow::Deactivate()
{
	if ( m_viewportWindowRoot )
	{
		m_viewportWindowRoot->Deactivate( this );
	}
}

#endif // ORBIS
