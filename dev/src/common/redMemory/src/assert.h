/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_ASSERT_H
#define _RED_MEMORY_ASSERT_H

#include "../../redSystem/types.h"
#include "../../redSystem/error.h"
#include "../../redSystem/log.h"

#ifdef RED_PLATFORM_CONSOLE
	#define ENABLE_SIMPLE_ASSERTS
#endif

#ifdef RED_MEMORY_ENABLE_ASSERTS

#ifdef ENABLE_SIMPLE_ASSERTS

#ifdef RED_PLATFORM_DURANGO

inline void MemoryPrintErrorToConsole( char* format, ... )
{
	char buffer[ 256 ] = {};

	va_list arglist;
	va_start( arglist, format );
	vsnprintf_s( buffer, 256, format, arglist );
	va_end( arglist );
	::OutputDebugStringA( buffer );
};

	#define RED_MEMORY_ASSERT( condition, txt, ... )	do { if( !(condition) ) { MemoryPrintErrorToConsole( txt, ## __VA_ARGS__ ); __debugbreak(); } }	while( 0,0 )
	#define RED_MEMORY_HALT( txt, ... )					do { MemoryPrintErrorToConsole( txt, ## __VA_ARGS__ ); __debugbreak(); } while( 0, 0 )

#else

	#define RED_MEMORY_ASSERT( condition, txt, ... )	do { if( !(condition) ) { printf( txt, ## __VA_ARGS__); __debugbreak(); } } while( 0, 0 )
	#define RED_MEMORY_HALT( txt, ... )					do { printf( txt, ## __VA_ARGS__ ); __debugbreak(); } while( 0, 0 )

#endif

#else
	// Asserts for red::MemoryFramework
	// We have to disable some error handler features as they may allocate memory themselves 
	#define RED_MEMORY_ASSERT( condition, txt, ... )	if( !(condition) ) { \
														  red::Log::Manager::GetInstance().SetCrashModeActive( true ); \
														  RED_FATAL_ASSERT( false, txt, ## __VA_ARGS__ );	\
														  red::Log::Manager::GetInstance().SetCrashModeActive( false ); \
														}

	#define RED_MEMORY_HALT( txt, ... )					{ \
														  red::Log::Manager::GetInstance().SetCrashModeActive( true ); \
														  RED_FATAL_ASSERT( !txt, txt, ## __VA_ARGS__ );	\
														  red::Log::Manager::GetInstance().SetCrashModeActive( false ); \
														}
#endif

#else

	#define RED_MEMORY_ASSERT( ... )	do { } while ( (void)0,0 )
	#define RED_MEMORY_HALT( ... )		do { } while ( (void)0,0 )

#endif

#endif