/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifdef _DEBUG
	extern void WriteDebugLog( const char* format, ... );
	bool SetCallback( void (*DebugLogFunc)( const char* message ) );

#	define DDI_LOG( format, ... ) WriteDebugLog( format, ##__VA_ARGS__ )
#	define DDI_LOG_SETCALLBACK( func ) SetCallback( func )

#else
#	define DDI_LOG( format, ... )
#	define DDI_LOG_SETCALLBACK( func ) false
#endif