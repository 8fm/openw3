/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_ARCHITECTURE_H_
#define _RED_ARCHITECTURE_H_

//////////////////////////////////////////////////////////////////////////
// Compiler
#if defined( _MSC_VER )
#	define RED_COMPILER_MSC
#elif defined( __clang__ )
#	define RED_COMPILER_CLANG
#else
#	error Unsupported compiler
#endif

//////////////////////////////////////////////////////////////////////////
// Instruction set

#if defined ( __AVX__ )
#define RED_USE_AVX
#define RED_USE_SSE2
#define RED_USE_SSE
#elif defined ( __SSE2__ ) || defined( _M_X64 ) || defined( __x86_64__ ) || _M_IX86_FP > 1
#define RED_USE_SSE2
#define RED_USE_SSE
#elif defined ( __SSE__ ) || defined( _M_X64 ) || defined( __x86_64__ )  || _M_IX86_FP > 0
#define RED_USE_SSE
#error Instructionset below the minimum requirements
#else
RED_MESSAGE( "SIMD instruction set switched off" );
#endif

//////////////////////////////////////////////////////////////////////////
// Operating System
#if defined( _DURANGO )
#	define RED_PLATFORM_DURANGO
#elif defined( _WIN64 )
#	define RED_PLATFORM_WIN64
#elif defined( _WIN32 )
#	define RED_PLATFORM_WIN32
#elif defined( __ORBIS__ )
#	define RED_PLATFORM_ORBIS
#else
#	error Undefined operating system
#endif

#if defined( RED_PLATFORM_DURANGO ) || defined( RED_PLATFORM_ORBIS )
#	define RED_PLATFORM_CONSOLE
#endif

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 )
#	define RED_PLATFORM_WINPC
#endif

//////////////////////////////////////////////////////////////////////////
// Architecture
#if defined( _M_X64 ) || defined( __x86_64__ )
#	define RED_ARCH_X64
#	define RED_ENDIAN_LITTLE
#elif defined( _M_IA64 )
#	define RED_ARCH_IA64
#elif defined( _M_IX86 ) || defined( __i386__ ) || defined( __i386 )
#	define RED_ARCH_X86
#	define RED_ENDIAN_LITTLE
#else
#	error Undefined processor architecture
#endif

// All byteswapping is ifdefed out with this at the moment
// this should only happen in the cooker (if ever) and never in the game
// #define RED_ENDIAN_SWAP_SUPPORT_DEPRECATED

#endif //_RED_ARCHITECTURE_H_
