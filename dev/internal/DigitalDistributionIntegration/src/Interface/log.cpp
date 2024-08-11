/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifdef _DEBUG

#include "red.h"

typedef void (*DebugLogFunc)( const char* message );
static DebugLogFunc logFunc = nullptr;

void WriteDebugLog( const char* format, ... )
{
	if( logFunc )
	{
		const int maxLength = 1024;
		char buffer[ maxLength ];

		va_list arglist;
		va_start( arglist, format );
		Red::System::VSNPrintF( buffer, maxLength, format, arglist );
		va_end( arglist );

		logFunc( buffer );
	}
}

bool SetCallback( DebugLogFunc func )
{
	logFunc = func;
	return true;
}

#endif
