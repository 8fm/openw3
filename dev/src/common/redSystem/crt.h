/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_CRT_H_
#define _RED_CRT_H_

#include "architecture.h"

//////////////////////////////////////////////////////////////////////////
// 
#include "formatMacros.h"

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
	#include "crtWindows.h"
#elif defined( RED_PLATFORM_ORBIS )
	#include "crtOrbis.h"
#endif

namespace Red
{
	namespace System
	{
		// These functions are for converting to "Char" from UniChar or AnsiChar or any custom typedef
		RED_FORCE_INLINE Bool StringConvert( UniChar* dest, const AnsiChar* source, size_t destSize )		{ return StdCharToWideChar( dest, source, destSize ) > 0; }
		RED_FORCE_INLINE Bool StringConvert( AnsiChar* dest, const UniChar* source, size_t destSize )		{ return WideCharToStdChar( dest, source, destSize ) > 0; }

		// These are "dummy" functions that provide the same appearance of functionality as the above functions
		// Allowing you to use StringConvert when some platform configurations have differing choices on encoding
		RED_FORCE_INLINE Bool StringConvert( UniChar* dest, const UniChar* source, size_t destSize )		{ return StringCopy( dest, source, destSize ); }
		RED_FORCE_INLINE Bool StringConvert( AnsiChar* dest, const AnsiChar* source, size_t destSize )		{ return StringCopy( dest, source, destSize ); }
	}
}

#endif
