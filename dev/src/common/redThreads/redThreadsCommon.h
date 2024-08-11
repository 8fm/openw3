/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_CONSTS_H
#define RED_THREADS_CONSTS_H
#pragma once

#include "redThreadsPlatform.h"

namespace Red { namespace Threads {

//////////////////////////////////////////////////////////////////////////
// Enumerations
//////////////////////////////////////////////////////////////////////////
enum EThreadPriority
{
	TP_Idle,			//!< Indicates that the thread is idle. 
	TP_Lowest,			//!< Indicates the lowest scheduling priority for an active thread. 
	TP_BelowNormal,		//!< Indicates the second-lowest scheduling priority for an active thread. 
	TP_Normal,			//!< Indicates the default scheduling priority for an active thread.	
	TP_AboveNormal,		//!< Indicates the second-highest priority for an active thread that can be rescheduled. 
	TP_Highest,			//!< Indicates the highest priority for an active thread that can be rescheduled. 
	TP_TimeCritical,	//!< Indicates the true highest priority level for an active thread, one which cannot be rescheduled.
};

} } // namespace Red { namespace Threads {

#if defined( RED_THREADS_PLATFORM_WINDOWS_API )

namespace Red { namespace Threads { namespace WinAPI {

#ifdef RED_ARCH_X64
	const TStackSize			g_kDefaultSpawnedThreadStackSize = 2 * 1024 * 1024;
#else
	const TStackSize			g_kDefaultSpawnedThreadStackSize = 1024 * 1024;
#endif

} } } // namespace Red { namespace Threads { namespace WinAPI {

#elif defined( RED_THREADS_PLATFORM_ORBIS_API )

namespace Red { namespace Threads { namespace OrbisAPI {

const TStackSize				g_kDefaultSpawnedThreadStackSize = 2 * 1024 * 1024;

} } } // namespace Red { namespace Threads { namespace OrbisAPI {

#endif

#endif // RED_THREADS_CONSTS_H