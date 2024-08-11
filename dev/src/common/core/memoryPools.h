//////////////////////////////////////////////////////////////////////////
// Memory Pool Definitions
//	This file should not be included directly unless you plan on defining the 
//	DECLARE_MEMORY_POOL macro to do something interesting!

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 )
	#include "memoryPoolsWindows.h"
#elif defined( RED_PLATFORM_DURANGO )
	#include "memoryPoolsDurango.h"
#elif defined( RED_PLATFORM_ORBIS )
	#include "memoryPoolsOrbis.h"
#endif