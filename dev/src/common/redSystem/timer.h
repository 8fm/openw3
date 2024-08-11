/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_TIMER_H_
#define _RED_TIMER_H_

//////////////////////////////////////////////////////////////////////////
// System Timer
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
	#include "timerWindows.h"
#elif defined( RED_PLATFORM_ORBIS )
	#include "timerOrbis.h"
#endif

#endif // _RED_TIMER_H_
