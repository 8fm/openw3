/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

//////////////////////////////////////////////////////////////////////////
// CLoadingOverlay
//////////////////////////////////////////////////////////////////////////
class CLoadingOverlay
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
			CLoadingOverlay();
			~CLoadingOverlay();
	Bool	DelayedInit();

public:
	void	ToggleVisible( Bool visible, const String& reason );
};