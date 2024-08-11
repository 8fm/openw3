/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_DEBUG_PAGES

#include "debugPageManagerBase.h"

/// Debug pages
class CDebugPageManagerTabbed : public IDebugPageManagerBase
{
public:
	CDebugPageManagerTabbed( CGatheredResource& fontResource );
	virtual ~CDebugPageManagerTabbed();

	//! External viewport tick
	virtual void OnTick( Float timeDelta );

	//! Generalized input event
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

private:
};

#endif
