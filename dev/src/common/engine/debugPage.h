/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_DEBUG_PAGES

enum EInputKey : Int32;
enum EInputAction : Int32;
class IViewport;
class CRenderFrame;
class CRenderCamera;

/// Debug pages
class IDebugPage
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Debug );

protected:
	String		m_pageName;			//!< Name of the page

public:
	//! Get page caption
	RED_INLINE const String& GetPageName() const { return m_pageName; }

public:
	IDebugPage( const String& name );
	virtual ~IDebugPage();

	//! This debug page was shown
	virtual void OnPageShown();

	//! This debug page was hidden
	virtual void OnPageHidden();

	//! External viewport tick
	virtual void OnTick( Float timeDelta );

	//! Generalized input event
	virtual Bool OnViewportInput( IViewport* view, EInputKey key, EInputAction action, Float data );
	
	//! Generalized mouse input event
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

	//! Override camera
	virtual Bool OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );

    virtual Bool FullScreen() const;

public:
	//! Draw helper for progress bars
	static void DrawProgressBar( CRenderFrame *frame, Int32 x, Int32 y, Int32 width, Int32 height, Float prc, const Color& color, const Color& colorFrame = Color::WHITE );
};

#endif
