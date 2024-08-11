/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_TYPES_H_
#define _RED_TYPES_H_

#include "os.h"
#include "architecture.h"
#include "compilerExtensions.h"

// For size_t, ptrdiff_t
#include <cstddef>

// For intptr_t, uintptr_t
#include <cstdint>

// For _SCE_BREAK
#if defined( RED_PLATFORM_ORBIS )
	#include <libdbg.h>
#endif

// Temporary namespace
namespace Red
{
	namespace System
	{
		//////////////////////////////////////////////////////////////////////////
		// Type definitions

		typedef bool				Bool;

		typedef char				Int8;
		typedef unsigned char		Uint8;

		typedef std::int16_t		Int16;
		typedef std::uint16_t		Uint16;

		typedef std::int32_t		Int32;
		typedef std::uint32_t		Uint32;

		typedef std::int64_t		Int64;
		typedef std::uint64_t		Uint64;

		typedef float				Float;
		typedef double				Double;

		typedef wchar_t				UniChar;
		typedef char				AnsiChar;

		typedef size_t				MemSize;
		typedef ptrdiff_t			MemDiff;
		typedef intptr_t			MemInt;
		typedef	uintptr_t			MemUint;

		namespace Random
		{
			typedef Uint32			SeedValue;
		}

		//////////////////////////////////////////////////////////////////////////
		// Unicode
		#if defined( UNICODE )

			typedef UniChar				Char;
		#	define TXT(s)				L##s

		#else
		// Prevent mix and matching across projects.
		#error Should be using Unicode but not defined
			typedef AnsiChar			Char;
		#	define TXT(s)				s

		#endif
	}
}

#endif //_RED_TYPES_H_
