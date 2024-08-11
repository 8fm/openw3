/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiWindow.h"

namespace RedGui
{
	class CRedGuiModalWindow : public CRedGuiWindow
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiModalWindow( Uint32 left, Uint32 top, Uint32 width, Uint32 height );
		virtual ~CRedGuiModalWindow();

		virtual void SetVisible( Bool value ) override;
	};
	
}	// namespace RedGui

#endif	// NO_RED_GUI
