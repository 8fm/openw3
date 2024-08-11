/**
* Copyright © 2007 CDProjekt Red, Inc. All Rights Reserved.
*
* This file contains ANSI C++ run time functions
*/

#pragma once

/************************************************************************/
/* String functions                                                     */
/************************************************************************/

#ifdef RED_PLATFORM_ORBIS
#	include <ctype.h>
#endif

// just a quick implementation...  just returns nonzero if strings differ
RED_INLINE Int32 AUniAnsiStrCmp( const UniChar* uniStr, const AnsiChar* ansiStr )
{
	UniChar a, u;
	for ( ;; )
	{
		a = *ansiStr++;
		u = *uniStr++;												 
		if ( a == 0 )
		{	
			return u;
		}
		else if ( u == 0 || u != a )
		{
			return -1;
		}
	}
}

// just a quick implementation...  just returns nonzero if strings differ
RED_INLINE Int32 AUniAnsiStrICmp( const UniChar* uniStr, const AnsiChar* ansiStr )
{
	int a, u;
	for ( ;; )
	{
		a = tolower( (int) *ansiStr++ );
		u = tolower( (int) *uniStr++ );
		if ( a == 0 )
		{	
			return u;
		}
		else if ( u == 0 || u != a )
		{
			return -1;
		}
	}
}

// small crap for fixing NANs
RED_INLINE Float AFixNAN( Float possibleNan ) { return ( possibleNan != possibleNan ) ? 0.f : possibleNan; }

static const Uint32   HASH32_BASE = 0x811C9DC5; // FNV32_Prime
static const Uint64 HASH64_BASE = 0xCBF29CE484222325; // FNV64_Prime

// Calculate Hash of buffer
extern Uint32 ACalcBufferHash32Merge( const void* buffer, size_t sizeInBytes, Uint32 previousHash );

// Calculate Hash of buffer
extern Uint64 ACalcBufferHash64Merge( const void* buffer, size_t sizeInBytes, Uint64 previousHash );

// it's insanely stupid... but since Win32 and PowerPC have different Unicodes (wchar_t), then calculating hashes from String will give different results
extern Uint64 ACalcBufferHash64Merge( const String& string, Uint64 previousHash );

// Calculate hash of Ansi string
extern Uint64 ACalcBufferHash64Merge( const StringAnsi& string, Uint64 previousHash );

// ...aaand the same for Char*
extern Uint64 ACalcBufferHash64MergeWithRespectToEndians( const Char* string, size_t length, Uint64 previousHash );

// ...aaand the 32-bit version
extern Uint32 ACalcBufferHash32MergeWithRespectToEndians( const Char* string, size_t length, Uint32 previousHash );

// Replaces non-ascii characters with their nearest ascii substitutes (like 'ê' with 'e')
extern void ReplaceUnicodeCharsWithAscii( Char buffer[], size_t length );
