/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_PLATFORM_H
#define RED_THREADS_PLATFORM_H
#pragma once

#include "../redSystem/os.h"
#include "../redSystem/crt.h"

#include "../redSystem/error.h"
#include "../redSystem/types.h"
#include "../redSystem/utility.h"

#define REDTHR_NOCOPY_CLASS(cls)	\
	private:						\
		cls(const cls&);			\
		void operator=(const cls&);	\

#define REDTHR_NOCOPY_STRUCT(s)		\
	REDTHR_NOCOPY_CLASS(s)			\
	public:

// FIXME redSystem on ORBIS
// #ifndef RED_ASSERT
// //#define RED_ASSERT(x, message, ...) do{ if( !(x) ){fprintf(stderr, (message), ## __VA_ARGS__ ); __debugbreak();} }while(false)
// # define RED_ASSERT(x, ...) do{ (void)(x); }while(false)
// #endif
// #ifndef RED_VERIFY
// # define RED_VERIFY(x, ...) do{ (void)(x); }while(false)
// #endif
// #ifndef RED_HALT
// # define RED_VERIFY(x, ...) do{ (void)(x); }while(false)
// #endif

#define REDTHR_ASSERT(x) RED_ASSERT( (x), TXT("") )

//////////////////////////////////////////////////////////////////////////
// Platform defines
#if defined( RED_PLATFORM_WIN32 ) || defined ( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
#define RED_THREADS_PLATFORM_WINDOWS_API
#elif defined( RED_PLATFORM_ORBIS )
#define RED_THREADS_PLATFORM_ORBIS_API
#endif

//////////////////////////////////////////////////////////////////////////
// OS Namespaces for switching between platform-specific implementations
#if defined( RED_THREADS_PLATFORM_WINDOWS_API )

namespace Red { namespace Threads { namespace WinAPI {
} } } // Red { namespace Threads { namespace WinAPI {

namespace Red { namespace Threads {
	namespace OSAPI = WinAPI;
} } // namespace Red { namespace Threads {

#elif defined( RED_THREADS_PLATFORM_ORBIS_API )

namespace Red { namespace Threads { namespace OrbisAPI {
} } } // Red { namespace Threads { namespace WinAPI {

namespace Red { namespace Threads {
	namespace OSAPI = OrbisAPI;
} } 

#endif

//////////////////////////////////////////////////////////////////////////
// OS specific types
#include "redThreadsTypes.h"

//////////////////////////////////////////////////////////////////////////
// OS specific error check wrappers
#if defined( RED_PLATFORM_ORBIS )
#	ifdef RED_ASSERTS_ENABLED
#		define REDTHR_SCE_CHECK( expression ) do{ int sceret_ = (expression); (void)sceret_; RED_ASSERT( sceret_ == SCE_OK, MACRO_TXT( #expression ) TXT("\nReturn value: 0x%08X"), sceret_ ); }while(false)
#	else
#		define REDTHR_SCE_CHECK( expression ) RED_VERIFY( (expression), TXT("") )
#	endif
#else
#	define REDTHR_SCE_CHECK( expression ) (void)(expression)
#endif

#ifdef RED_COMPILER_MSC
#pragma warning( disable : 4127 ) // conditional expression is constant: from do{}while(false)
#endif // RED_COMPILER_MSC

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
#	ifdef RED_ASSERTS_ENABLED
// Speculatively GetLastError() in case assert checking changes it. Compare with != 0 so can check pointers, numbers, or bools without "performance" warnings.
// Technically relies on "expression" not doing some conversion that could clobber last error...
#		define REDTHR_WIN_CHECK( expression ) do{ Red::Threads::Bool winret_ = ( (expression) != 0 ); DWORD lastError_ = ::GetLastError(); (void)lastError_; RED_ASSERT( winret_, MACRO_TXT( #expression ) TXT("\nGetLastError() result: 0x%08X"), lastError_ ); RED_UNUSED(winret_); }while(false)
#	else
#		define REDTHR_WIN_CHECK( expression ) RED_VERIFY( (expression), TXT("") )
#	endif
#else
#	define REDTHR_WIN_CHECK( expression ) (void)(expression)
#endif

#endif // RED_THREADS_PLATFORM_H