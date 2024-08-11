/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FRAMEWORK_DEFS_H
#define _RED_MEMORY_FRAMEWORK_DEFS_H
#pragma once

#include "../redSystem/architecture.h"

//////////////////////////////////////////////////////////////////////////
// Platform defines
#if defined( RED_PLATFORM_DURANGO )
	#define RED_MEMORY_FRAMEWORK_PLATFORM_DURANGO_API
#elif defined( RED_PLATFORM_WIN32 ) || defined ( RED_PLATFORM_WIN64 )
	#define RED_MEMORY_FRAMEWORK_PLATFORM_WINDOWS_API
#elif defined( RED_PLATFORM_ORBIS )
	#define RED_MEMORY_FRAMEWORK_PLATFORM_ORBIS_API
#endif

//////////////////////////////////////////////////////////////////////////
// OS Namespaces for switching between platform-specific implementations
#ifdef RED_MEMORY_FRAMEWORK_PLATFORM_WINDOWS_API

	namespace Red { namespace MemoryFramework { namespace WinAPI {
	} } }

	namespace Red { namespace MemoryFramework {
		namespace OSAPI = WinAPI;
	} }

#elif defined( RED_MEMORY_FRAMEWORK_PLATFORM_ORBIS_API )

	namespace Red { namespace MemoryFramework { namespace OrbisAPI {
	} } } 

	namespace Red { namespace MemoryFramework {
		namespace OSAPI = OrbisAPI;
	} }

#elif defined( RED_MEMORY_FRAMEWORK_PLATFORM_DURANGO_API )

	namespace Red { namespace MemoryFramework { namespace DurangoAPI {
	} } } 

	namespace Red { namespace MemoryFramework {
		namespace OSAPI = DurangoAPI;
	} }

#endif

//////////////////////////////////////////////////////////////////////////



#endif // RED_MEMORY_FRAMEWORK_DEFS_H