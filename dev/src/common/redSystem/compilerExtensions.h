/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_COMPILER_EXTENSIONS_H_
#define _RED_COMPILER_EXTENSIONS_H_

#include "architecture.h"

// Used when TXT() is required by a macro
#define MACRO_TXT( x )	TXT( x )

// Used when you need to turn a macro parameter into a string literal
#define RED_STRINGIFY( x ) #x

// Used when you want to expand x macro first before turning it into string literal
#define RED_EXPAND_AND_STRINGIFY( x ) RED_STRINGIFY( x )

// For when a parameter is defined, but not yet used (If this particular version of the function does not require the variable, remove the identifier from the function signature instead)
#define RED_UNUSED( x )	(void)x

//////////////////////////////////////////////////////////////////////////
// Non standard extensions
#if defined( RED_COMPILER_MSC )

#	define RED_ALIGNED_CLASS( type, alignment )		__declspec( align( alignment ) ) class type
#	define RED_ALIGNED_STRUCT( type, alignment )	__declspec( align( alignment ) ) struct type
#	define RED_ALIGNED_VAR( type, alignment )		__declspec( align( alignment ) ) type

// Compiler Pointer optimisation hints
#	define RED_RESTRICT_RETURN					__declspec( restrict )
#	define RED_RESTRICT_PARAMS					__declspec( noalias )
#	define RED_RESTRICT_LOCAL					__restrict

// Miscellaneous optimisation hints
#	define RED_ASSUME( expression )				__assume( expression )

// Function call conventions
#	define RED_CDECL							__cdecl					// Default C++ calling convention - required for variadic functions
#	define RED_STDCALL							__stdcall				// Windows API function calling convention
#	define RED_FASTCALL							__fastcall

#	define RED_INLINE							inline
#	define RED_FORCE_INLINE						__forceinline

#	define NOEXCEPT
#	define FUNC_NOOPTS

#	define RED_FINAL							final

// Class extensions
#	define RED_PURE_INTERFACE(cls_)				__declspec(novtable) cls_ abstract

// Warning suppression
#	define RED_DISABLE_WARNING_MSC( n )			__pragma( warning( disable : n ) )
#	define RED_DISABLE_WARNING_CLANG( a )

#	define RED_WARNING_PUSH()					__pragma( warning( push ) )
#	define RED_WARNING_POP()					__pragma( warning( pop ) )

// Mark something (function, type, identifier, etc) as deprecated
#	define RED_DEPRECATED( identifier )			__pragma( deprecated( identifier ) )

//	Compile time Messages
#	define RED_MESSAGE_INTERNAL( text, line )	__pragma( message( __FILE__ ##"(" RED_STRINGIFY( line ) ##") : " ##text ) )
#	define RED_MESSAGE( text )					RED_MESSAGE_INTERNAL( text, __LINE__ )

#	define RED_PLATFORM_LIBRARY_EXT				lib

#elif defined( RED_COMPILER_CLANG )

// Struct / variable alignment
#	define RED_ALIGNED_CLASS( type, alignment )		class alignas( alignment ) type
#	define RED_ALIGNED_STRUCT( type, alignment )	struct alignas( alignment ) type
#	define RED_ALIGNED_VAR( type, alignment )		alignas( alignment ) type

// Compiler Pointer optimisation hints
#	define RED_RESTRICT_RETURN
#	define RED_RESTRICT_PARAMS
#	define RED_RESTRICT_LOCAL

// Miscellaneous optimisation hints
#	define RED_ASSUME( expression )

// Function call conventions
// FIXME redSystem. Supports e.g., __attribute__((cdecl)), but this is probably preferable for the game
#	define RED_CDECL			
#	define RED_STDCALL			
#	define RED_FASTCALL			 

#	define RED_INLINE						inline
// FIXME redSystem: attribute placement
//#	define RED_FORCE_INLINE					__attribute__((always_inline)) 
#	define RED_FORCE_INLINE					inline 

#	define NOEXCEPT							noexcept
#	define FUNC_NOOPTS						__attribute__((optnone))

#	define RED_FINAL

// Class extensions
#	define RED_PURE_INTERFACE(cls_)			cls_

#	define RED_CLANG_PRAGMA_INTERNAL( a )	_Pragma( #a )

// Warning suppression
#	define RED_DISABLE_WARNING_MSC( n )
#	define RED_DISABLE_WARNING_CLANG( a )	RED_CLANG_PRAGMA_INTERNAL( clang diagnostic ignored a )

#	define RED_WARNING_PUSH()				RED_CLANG_PRAGMA_INTERNAL( clang diagnostic push )
#	define RED_WARNING_POP()				RED_CLANG_PRAGMA_INTERNAL( clang diagnostic pop )

// Clang only supports deprecating a feature when it's declared (as an __attribute__)
#	define RED_DEPRECATED( identifier )

//	Compile time Messages
#	define RED_MESSAGE( text )				RED_CLANG_PRAGMA_INTERNAL( message text )

#	define RED_PLATFORM_LIBRARY_EXT			a

#endif

#ifndef RED_FINAL_BUILD
#	define RED_MOCKABLE virtual
#else
#	define RED_MOCKABLE 
#endif


//////////////////////////////////////////////////////////////////////////
// Debugging
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
#	define RED_BREAKPOINT() do{ if( ::IsDebuggerPresent() ){ __debugbreak(); } } while(0,0)
#elif defined( RED_PLATFORM_ORBIS )
# ifndef RED_FINAL_BUILD
#	define RED_BREAKPOINT() do{ if( ::sceDbgIsDebuggerAttached() ) { _SCE_BREAK(); } } while((void)0,0)
# else
#   define RED_BREAKPOINT()
# endif
#endif

/////////////////////////////////////////////////////////////////
// Compiler Intrinsics
#ifdef RED_COMPILER_MSC
// We must do the switch between the debug / normal malloc here as intrin pulls in malloc.h somewhere down the line
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif
#include <intrin.h>
#pragma	intrinsic( _BitScanForward )
#pragma	intrinsic( _BitScanReverse )
#else
#include <x86intrin.h>
#endif

#endif // _RED_COMPILER_EXTENSIONS_H_
