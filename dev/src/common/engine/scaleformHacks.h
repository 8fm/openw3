/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */
#pragma once

//////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

namespace Scaleform
{
	// otherwise conflicts in the SF headers themselves
	// with our core type ::StringBuffer
	class StringBuffer;
}

// From GFx\GFx_DrawText.h, but adapted for the macro defines in our game.
// a special case for Windows: windows.h renames DrawText to 
// either DrawTextA or DrawTextW depending on UNICODE macro.
// Avoiding the effect on our DrawText class.
#ifdef W2_PLATFORM_WIN32
#	ifdef DrawText
#		undef DrawText
#	endif // DrawText
#endif

//#include <GFx/GFx_DrawText.h>
// Restore DrawTextA/DrawTextW macros. Note, to use Scaleform's
// DrawText you'll need to use fully qualified name (GFx::DrawText)
// if you uncomment the lines below.
// #ifdef _WINDOWS
// typedef DrawText DrawTextA;
// typedef DrawText DrawTextW;
// #ifdef UNICODE
// #define DrawText DrawTextW
// #else
// #define DrawText DrawTextA
// #endif
// #endif //_WINDOWS

//////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////