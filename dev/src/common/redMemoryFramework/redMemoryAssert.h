/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_ASSERT_H
#define _RED_MEMORY_ASSERT_H

#include "../redSystem/types.h"
#include "../redSystem/error.h"
#include "../redSystem/log.h"

#ifdef RED_PLATFORM_CONSOLE
	#define ENABLE_SIMPLE_ASSERTS
#endif

#ifndef RED_FINAL_BUILD

#ifdef ENABLE_SIMPLE_ASSERTS
	#define RED_MEMORY_ASSERT( condition, txt, ... )	if( !(condition) ) { printf( txt, ## __VA_ARGS__); __debugbreak(); }
	#define RED_MEMORY_HALT( txt, ... )					{ printf( txt, ## __VA_ARGS__ ); __debugbreak(); }
#else
	// Asserts for Red::MemoryFramework
	// We have to disable some error handler features as they may allocate memory themselves 
	#define RED_MEMORY_ASSERT( condition, txt, ... )	if( !(condition) ) { \
														  Red::System::Log::Manager::GetInstance().SetCrashModeActive( true ); \
														  RED_FATAL_ASSERT( false, txt, ## __VA_ARGS__ );	\
														  Red::System::Log::Manager::GetInstance().SetCrashModeActive( false ); \
														}

	#define RED_MEMORY_HALT( txt )						{ \
														  Red::System::Log::Manager::GetInstance().SetCrashModeActive( true ); \
														  RED_FATAL_ASSERT( !txt, txt );	\
														  Red::System::Log::Manager::GetInstance().SetCrashModeActive( false ); \
														}
#endif

#else

	#define RED_MEMORY_ASSERT( ... )	do { } while ( (void)0,0 )
	#define RED_MEMORY_HALT( ... )		do { } while ( (void)0,0 )

#endif

#endif