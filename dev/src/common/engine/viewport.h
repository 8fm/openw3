/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "viewportWindowMode.h"
#include "../core/sharedPtr.h"

enum EMouseMode
{
	MM_Normal,			// Normal mouse mode
	MM_Clip,			// Mouse is clipped within the window bounds
	MM_Capture,			// Mouse is captured by the window and is invisible
	MM_ClipAndCapture,	// Mouse is captured by the window and clipped within the window bounds
};

class IRender;
class IViewportHook;
enum ERenderingMode : Int32;
enum EVisualDebugCommonOptions : Int32;
class IViewport;

typedef Red::TSharedPtr< IViewport > ViewportHandle;

/// Base class for rendering viewport
class IViewport
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );

public:
	enum class EAspectRatio
	{
		FR_NONE,
		FR_21_9,
		FR_16_9,
		FR_16_10,
		FR_4_3,
	};

protected:
	IViewportHook*					m_hook;
	Uint32							m_width;
	Uint32							m_height;
	Uint32							m_x;
	Uint32							m_y;
	Uint32							m_fullWidth;
	Uint32							m_fullHeight;
	EViewportWindowMode				m_windowMode;
	Bool							m_isAutoRedraw;
	ERenderingMode					m_renderingMode;
	Bool							m_renderingMask[ SHOW_MAX_INDEX ];
	TDynArray< CClass* >			m_classRenderingExclusion;
	THashMap< String, Bool >		m_templateRenderingExclusion;
	Bool				m_isTerrainToolStampVisible;
	Bool				m_isGrassMaskPaintMode;
	EMouseMode			m_mouseMode;

	Float				m_visualDebugCommonOptions[ VDCommon__MAX_INDEX ];	

	EAspectRatio	    m_cachetAspectRatio;

public:
	IViewport( Uint32 width, Uint32 height, EViewportWindowMode windowMode );
	virtual ~IViewport();
	// Getters
	RED_INLINE IViewportHook* GetViewportHook() const { return m_hook; }
	RED_INLINE Uint32 GetWidth() const { return m_width; }
	RED_INLINE Uint32 GetHeight() const { return m_height; }
	RED_INLINE Uint32 GetX() const { return m_x; }
	RED_INLINE Uint32 GetY() const { return m_y; }
	RED_INLINE EViewportWindowMode GetViewportWindowMode() const { return m_windowMode; }
	RED_INLINE Bool IsAutoRedraw() const { return m_isAutoRedraw; }
	RED_INLINE Bool IsMinimized() const { return m_width < 1 && m_height < 1; }
	RED_INLINE ERenderingMode GetRenderingMode() const { return m_renderingMode; }
	RED_INLINE const Bool* GetRenderingMask() const { return m_renderingMask; }
	RED_INLINE Bool IsTerrainToolStampVisible() const {return m_isTerrainToolStampVisible;}
	RED_INLINE Bool IsGrassMaskPaintMode() const { return m_isGrassMaskPaintMode; }
	RED_INLINE Uint32 GetFullWidth() const { return m_fullWidth; }
	RED_INLINE Uint32 GetFullHeight() const { return m_fullHeight; }
	RED_INLINE EMouseMode GetMouseMode() const { return m_mouseMode; }
	RED_INLINE Bool IsCachet() const { return m_cachetAspectRatio != EAspectRatio::FR_NONE; }

	// Calculate world space ray going through given pixel
	void CalcRay( Int32 x, Int32 y, Vector& origin, Vector &dir );

	// Change viewport state
	void SetRenderingMode( ERenderingMode mode );
	void SetRenderingMask( const EShowFlags* showFlags );
	void SetRenderingMask( EShowFlags showFlag );
	void SetViewportHook( IViewportHook* hook );
	void SetAutoRedraw( Bool autoRedraw );
	void ClearRenderingMask( const EShowFlags* showFlags );
	void ClearRenderingMask( EShowFlags showFlag );
	void SetTerrainToolStampVisible( Bool visible );
	void SetGrassMaskPaintMode( Bool flag );

	void SetRenderingDebugOptions( EVisualDebugCommonOptions option, Float val );
	Float GetRenderingDebugOption( EVisualDebugCommonOptions option );

	virtual void SetFocus() {}

	void ClearClassRenderingExclusion() { m_classRenderingExclusion.Clear(); }
	void SetClassRenderingExclusion( CClass * type, Bool exclude );
	Bool IsClassRenderingDisabled( CClass * type ) const { return m_classRenderingExclusion.Exist( type ); }

	void SetTemplateRenderingExclusion( const String& entTemplate, Bool exclude );
	Bool IsTemplateRenderingDisabled( const CEntityTemplate * entTemplate, const CClass* componentClass ) const;

	//
	virtual Uint32 GetRendererWidth() const { return m_width; }
	virtual Uint32 GetRendererHeight()  const { return m_height; }

	// interface
	virtual Bool AdjustSize( Uint32 width, Uint32 height );
	virtual Bool AdjustSize( Uint32 x, Uint32 y, Uint32 width, Uint32 height );
	virtual void AdjustSizeWithCachets( EAspectRatio cachetAspectRatio );
	virtual void RestoreSize();
#ifndef NO_WINDOWS_INPUT
	virtual void SetTopLevelWindow( HWND handle )=0;
#endif
	virtual Bool GetPixels( Uint32 x, Uint32 y, Uint32 width, Uint32 height, TDynArray< Color >& pixels )=0;
	virtual void SetMouseMode( EMouseMode mode, Bool notRestoreMousePosition = false )=0;
	virtual void SetCursorVisibility( Bool state, Bool doNotRestoreMousePos = false )=0;

	virtual void ResizeBackBuffer( Uint32 width, Uint32 height )=0; // need separate function because in DX11 this must happen on the render thread

	virtual void Activate() {}
	virtual void Deactivate() {}

	virtual void NotifyGammaChanged()=0;

	virtual void RequestWindowMode( EViewportWindowMode windowMode )=0;
	virtual void RequestWindowSize( Uint32 width, Uint32 height )=0;
	virtual void RequestOutputMonitor( Uint32 outputMonitorIndex)=0;

protected:
	void GetSizeAdjustedToCachet( const EAspectRatio aspectRatio, const Uint32 inFullWidth, const Uint32 inFullHeight, Int32& outWidth, Int32& outHeight ) const;

};
