/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_DEBUG_PAGES

#include "debugPage.h"
#include "../core/gatheredResource.h"

class IViewport;
class CRenderFrame;

/// Debug pages
class IDebugPageManagerBase
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Debug );

public:
	IDebugPageManagerBase( CGatheredResource& fontResource );
	virtual ~IDebugPageManagerBase();

	//! External viewport tick
	virtual void OnTick( Float timeDelta );

	//! Generalized input event
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	//! Generalized mouse event
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

	//! Override camera
	virtual Bool OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );

	void SelectDebugPage( IDebugPage* page );

	//! Register debug page
	void RegisterDebugPage( IDebugPage* page );

	//! Unregister debug page
	void UnregisterDebugPage( IDebugPage* page );

	Bool IsDebugPageActive() const { return m_activePage != NULL; }

	Bool IsDebugPageActive( const String& pageName ) const { return m_activePage && m_activePage->GetPageName() == pageName; }

private:
	Color GetColorForFrameTime( Float time ) const;
	Color GetColorForSyncTime( Float time ) const;

	void DisplayMemorySummary( CRenderFrame* frame, Int32 x, Int32 y );

protected:

	TDynArray< IDebugPage* >		m_debugPages;			//!< Debug pages

	IDebugPage* m_activePage;

	CGatheredResource	m_fontResource;

	// Singleton stuff
public:
	static IDebugPageManagerBase* GetInstance() { return s_instance; }

private:
	static IDebugPageManagerBase* s_instance;
};

#endif
