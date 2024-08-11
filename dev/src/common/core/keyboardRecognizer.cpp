/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "keyboardRecognizer.h"

#ifdef RED_PLATFORM_WINPC
	#include <winuser.h>
#endif

namespace HardwareInstrumentation
{
	String GetCurrentKeyboardLocaleID()
	{
#ifndef RED_PLATFORM_WINPC
		return 0;
#else
		TCHAR layoutName[KL_NAMELENGTH];
		BOOL result = GetKeyboardLayoutName( layoutName );

		if( result != 0 )
		{
			return String( layoutName );
		}

		return String();
#endif
	}
}
