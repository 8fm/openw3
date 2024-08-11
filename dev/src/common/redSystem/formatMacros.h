/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_FORMAT_MACROS_H
#define _RED_FORMAT_MACROS_H

#include "architecture.h"

//////////////////////////////////////////////////////////////////////
// This file provides printf formatting macros for non-c89 types (particularly 16 / 64 bit integers and size_t / ptrs)
// These should mirror the c99 formatting macros 
// We need to wrap these since some compilers (ahem, MSVC) only support c89 printf formats (+ a few non-standard ones)

#ifdef RED_COMPILER_MSC

// 16-bit integers
#define RED_PRId16		"hi"
#define RED_PRIx16		"hx"
#define RED_PRIu16		"hu"

// 64-bit integers
#define RED_PRId64		"I64d"
#define RED_PRIu64		"I64u"
#define RED_PRIx64		"I64x"

// size_t
#define RED_PRIsize_t	"Iu"
#define RED_PRIxsize_t	"Ix"

// Strings
#define RED_PRIas		"hs"	//!< AnsiChar*
#define RED_PRIus		"ls"	//!< UniChar*

// Char*
#ifdef UNICODE
#	define RED_PRIs	RED_PRIus	//!< Char*
#else
#	define RED_PRIs	RED_PRIas	//!< Char*
#endif

#else

// Assuming anything not made by Microsoft actually conforms to c99
#include <inttypes.h>

// 16-bit integers
#define RED_PRId16		PRId16
#define RED_PRIu16		PRIu16
#define RED_PRIx16		PRIx16

// 64-bit integers
#define RED_PRId64		PRId64
#define RED_PRIu64		PRIu64
#define RED_PRIx64		PRIx64

// size_t
#define RED_PRIsize_t	"zd"
#define RED_PRIxsize_t	"zx"

// Strings
#define RED_PRIas		"s"		//!< AnsiChar*
#define RED_PRIus		"ls"	//!< UniChar*

// Char*
#ifdef UNICODE
#	define RED_PRIs	RED_PRIus	//!< Char*
#else
#	define RED_PRIs	RED_PRIas	//!< Char*
#endif

#endif

// Wide versions
#define RED_PRIWd16			MACRO_TXT( RED_PRId16 )
#define RED_PRIWu16			MACRO_TXT( RED_PRIu16 )
#define RED_PRIWx16			MACRO_TXT( RED_PRIx16 )
#define RED_PRIWd64			MACRO_TXT( RED_PRId64 )
#define RED_PRIWu64			MACRO_TXT( RED_PRIu64 )
#define RED_PRIWx64			MACRO_TXT( RED_PRIx64 )
#define RED_PRIWsize_t		MACRO_TXT( RED_PRIsize_t )
#define RED_PRIWxsize_t		MACRO_TXT( RED_PRIxsize_t )
#define RED_PRIWas			MACRO_TXT( RED_PRIas )
#define RED_PRIWus			MACRO_TXT( RED_PRIus )
#ifdef UNICODE
#	define RED_PRIWs		RED_PRIWus
#else
#	define RED_PRIWs		RED_PRIWas
#endif

#endif
