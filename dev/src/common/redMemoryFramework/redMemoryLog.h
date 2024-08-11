/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_LOG_H
#define _RED_MEMORY_LOG_H

#include "../redSystem/log.h"

// Logging for Red::MemoryFramework

#if !defined( RED_FINAL_BUILD ) || defined( LOG_IN_FINAL )

	#define RED_MEMORY_LOG( format, ... )	Red::System::Log::Manager::GetInstance().SetCrashModeActive( true );	\
											RED_LOG( MemoryFramework, format, ##__VA_ARGS__ );	\
											Red::System::Log::Manager::GetInstance().SetCrashModeActive( false );

	#define RED_MEMORY_LOG_ONCE( format, ... )	{ \
		static Red::System::Bool s_hasLogged = false;	\
		if( !s_hasLogged )	\
		{    \
			RED_MEMORY_LOG( format, ##__VA_ARGS__ );	\
			s_hasLogged = true;	\
		}    \
	}

#else

	#define RED_MEMORY_LOG( ... )			do { } while ( (void)0,0 )
	#define RED_MEMORY_LOG_ONCE( ... )		do { } while ( (void)0,0 )

#endif

#endif
