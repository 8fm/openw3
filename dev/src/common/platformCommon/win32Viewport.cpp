/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "win32Viewport.h"

// Game message pump
Bool CWin32GameViewport::PumpMessages()
{
	return true;
}

// Editor message pump
Bool CWin32EditorViewport::PumpMessages()
{
	MSG Msg;
	while ( ::PeekMessageW( &Msg,NULL,0,0,PM_REMOVE ) )
	{
		if ( Msg.message == WM_QUIT )
		{
			return false;
		}

		::TranslateMessage(&Msg);
		::DispatchMessageW(&Msg);
	}

	return true;
}