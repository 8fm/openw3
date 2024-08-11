/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CRenderViewport;

#ifndef RED_PLATFORM_ORBIS

// Mouse hack for debug pages
#ifdef RED_PLATFORM_DURANGO
# include "../engine/rawInputGamepadReading.h"
#endif

class CViewportWindowRoot;

/// WinAPI viewport window
class CViewportWindow
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

#ifndef RED_PLATFORM_CONSOLE
private:
	struct WndProcMsg
	{
		UINT	m_uMsg;
		WPARAM	m_wParam;
		LPARAM	m_lParam;
	};

	enum { DELAYED_MESSAGE_RESERVE = 8 };
	TDynArray< WndProcMsg > m_delayedMessages;
	TDynArray< WndProcMsg > m_delayedMessagesInUse;

	Bool DelayWndProc( UINT uMsg, WPARAM wParam, LPARAM lParam, LONG& outReturnValue );
	void ProcessDelayedMessages();
#endif // RED_PLATFORM_CONSOLE

private:
	CViewportWindowRoot*	m_viewportWindowRoot;

private:
#ifdef RED_PLATFORM_DURANGO
		SRawMouseReading		m_localMousePos;
#else
		POINT					m_localMousePos;
#endif
	Uint32						m_windowMode;

protected:
	HWND					m_windowHandle;
	HWND					m_parentHandle;
	HWND					m_topLevelHandle;
	CRenderViewport*		m_viewport;
	Uint32					m_width;
	Uint32					m_height;
	Bool					m_isOwnedByRenderThread;

public:
	CViewportWindow( CViewportWindowRoot* viewportWindowRoot, HWND topLevelWindow, HWND parent, const String& caption, Uint32 width, Uint32 height, Uint32 windowMode, Bool isOwnedByRenderThread );
	virtual ~CViewportWindow();

	void Bind( CRenderViewport* viewport );
	void AdjustRect( const Rect& newRect );
	void OnExternalDeactivateEvent();
	
	//Bool HandleMouseMove( Int32 x, Int32 y, Float pressure );
	Bool HandleMouseMoveEditorPacket( Int32 x, Int32 y, Float pressure );
	Bool HandleMouseEvent( Int32 button, Bool state, Int32 x, Int32 y );
	Bool HandleInputEvent( EInputKey key, EInputAction action, Float data );
	void HandleKillFocusEvent();
	void HandleSetFocusEvent();
	void HandleGeometryChangeEvent();
	void SendViewportActivatedEvent( Bool activated );

	// Input processing
	Bool ProcessInput( const BufferedInput& input );

	// Get the shit
	RED_INLINE Bool IsChildWindow() const { return m_parentHandle != NULL; } 
	RED_INLINE HWND GetTopLevelHandle() const { return m_topLevelHandle; }
	RED_INLINE HWND GetWindowHandle() const { return m_windowHandle; }
	RED_INLINE HWND GetParentHandle() const { return m_parentHandle; }
	RED_INLINE CRenderViewport* GetViewport() const { return m_viewport; }
	RED_INLINE CViewportWindowRoot* GetViewportWindowRoot() const { return m_viewportWindowRoot; }
	RED_INLINE Uint32 GetViewportWindowMode() const { return m_windowMode; }

	// Set the shit
	RED_INLINE void SetTopLevelWindow( HWND handle ) { m_topLevelHandle = handle; }

	void Activate();
	void Deactivate();

#ifndef RED_PLATFORM_CONSOLE
	// When switching between window modes, we need to change window style
	void AdjustStyleToWindowMode( EViewportWindowMode windowMode );
#endif

private:
#ifndef RED_PLATFORM_CONSOLE
	// Set the window as top-most
	void SetZOrderAsTopMost();
	
	// Set the window as not top-most (when alt-tabbing for instance)
	void SetZOrderAsNonTopMost();
#endif

	// Activate / deactivate tablet input
	void ActivateTabletInput( Bool isActivated );

	//! Message processing function
	virtual LONG WndProc( UINT uMsg, WPARAM wParam, LPARAM lParam );

	//! Message processing function
	static LONG APIENTRY StaticWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

	void HandleActivation( Bool isActivated );
};

#else

class CViewportWindowRoot;

class CViewportWindow
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

private:
	CViewportWindowRoot*	m_viewportWindowRoot;

protected:
	CRenderViewport*		m_viewport;
	Uint32					m_width;
	Uint32					m_height;
	Bool					m_isOwnedByRenderThread;

public:
	CViewportWindow( CViewportWindowRoot* viewportWindowRoot, const String& caption, Uint32 width, Uint32 height, Uint32 windowMode, Bool isOwnedByRenderThread );
	~CViewportWindow();

	void Bind( CRenderViewport* viewport );
	Bool HandleInputEvent( EInputKey key, EInputAction action, Float data );

	// Input processing
	Bool ProcessInput( const BufferedInput& input );

	RED_INLINE CRenderViewport* GetViewport() const { return m_viewport; }
	RED_INLINE CViewportWindowRoot* GetViewportWindowRoot() const { return m_viewportWindowRoot; }

	void Activate();
	void Deactivate();
};

#endif
