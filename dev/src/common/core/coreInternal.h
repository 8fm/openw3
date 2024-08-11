/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _CORE_INTERNAL_H_
#define _CORE_INTERNAL_H_

//Wtf? Durango pch settings must be messed up somewhere...
#ifdef _MSC_VER
#pragma warning( disable: 4651 )
#endif

// There's an inverse dependency from engine->core that's not necessarily clean to remove at this pont
// Used to get core linking, so we can work on getting the engine ported to Orbis
#if defined( __ORBIS__ ) || defined( _DURANGO )
#define TMP_NO_ENTITY_HANDLE_FOR_CORE_LINKAGE
#endif

#define USE_REDMATH
//#define USE_RED_RESOURCEMANAGER

// Temporary compatibility defines (Required now only for things that are soon to be replaced:
// String.h
// dynarray.h
// singleton.h
#define _CRT_SECURE_NO_WARNINGS

// Turn off security checks in iterators and stl containers
#define _SECURE_SCL 0
#define _SCL_SECURE_NO_WARNINGS

// Turn off exceptions in stl
#define _HAS_EXCEPTIONS 0

#include <math.h>
#include <functional>

#define USE_RED_THREADS

//////////////////////////////////////////////////////////////////////////
// Red System Integration

#include "../redSystem/settings.h"
#include "../redSystem/architecture.h"
#include "../redSystem/compilerExtensions.h"
#include "../redSystem/os.h"
#include "../redSystem/types.h"
#include "../redSystem/crt.h"
#include "../redSystem/typetraits.h"
#include "../redSystem/utility.h"
#include "../redSystem/utilityTimers.h"
#include "../redSystem/guid.h"
#include "../redSystem/log.h"
#include "../redSystem/logFile.h"
#include "../redSystem/error.h"

#include "../redMath/random/random.h"
#include "../redMath/random/standardRand.h"

#include "types.h"

// As things improve, these warning suppressors should be disabled
RED_DISABLE_WARNING_CLANG( "-Wunknown-pragmas" );
RED_DISABLE_WARNING_CLANG( "-W#pragma-messages" );
RED_DISABLE_WARNING_CLANG( "-Wswitch" );					// Don't report about unhandled conditions in switch statements
RED_DISABLE_WARNING_CLANG( "-Wreorder" );					// Don't report initializer lists being in the wrong order
RED_DISABLE_WARNING_CLANG( "-Wunused-function" );			// Don't report unused functions
RED_DISABLE_WARNING_CLANG( "-Wunused-private-field" );		// Don't report unused private members
RED_DISABLE_WARNING_CLANG( "-Wunused-variable" );			// Don't report declared variables that are unused
RED_DISABLE_WARNING_CLANG( "-Woverloaded-virtual" );		// Don't report virtual function overloads (bad, but its used in a bunch of places)

RED_DISABLE_WARNING_MSC( 4503 )	// 'function': decorated name length exceeded, name was truncated
#ifdef RED_PLATFORM_DURANGO
	RED_DISABLE_WARNING_MSC( 4291 ) // no matching operator delete found; memory will not be freed if initialization throws an exception
#endif

// Prevent direct use of standard library functions by deprecating them here
// RED_DEPRECATED( rand ); // Unfortunately this is blocked by standard library headers (<algorithm>) that call rand() (even if we don't use those particular functions)
RED_DEPRECATED( srand );

#ifndef RED_LOGGING_ENABLED
#	define NO_LOG
#endif

#ifndef RED_ASSERTS_ENABLED
#	define NO_ASSERTS
#endif

// Temporary compatibility macros
#define TXT2 MACRO_TXT
#define RESTRICT_RETURN	RED_RESTRICT_RETURN
#define RESTRICT RED_RESTRICT_LOCAL
#define STDCALL RED_STDCALL
#define SYS_DebugBreak RED_BREAKPOINT();

// Log
#define LOG_CORE( format, ... )						RED_LOG( Core, format, ##__VA_ARGS__ )
#define WARN_CORE( format, ... )					RED_LOG_WARNING( Core, format, ##__VA_ARGS__ )
#define WARN_CORE_ONCE( format, ... )				RED_LOG_WARNING_ONCE( Core, format, ##__VA_ARGS__  )
#define ERR_CORE( format, ... )						RED_LOG_ERROR( Core, format, ##__VA_ARGS__ )

// Asserts
#define ASSERT RED_ASSERT
#define HALT RED_HALT
#define VERIFY RED_VERIFY

#define ASSUME RED_ASSUME

#include "settings.h"

#include "explatformos.h"

#include "util.h"
#include "system.h"
#include "../redThreads/redThreadsThread.h"
#include "../redThreads/redThreadsAtomic.h"
#include "../redThreads/redThreadsRedSystem.h"

#include "fileVersionList.h"
#include "crt.h"
#include "singleton.h"
#include "memory.h"
#include "memoryHelpers.h"
#include "algorithms.h"
#include "mathForward.h"
#include "stringLocale.h"
#include "numericLimits.h"
#include "namesRegistry.h"
#include "coreTypeRegistry.h"

#endif 
