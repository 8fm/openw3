/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "ttyWriter.h"

Red::System::Log::TTYWriter::TTYWriter()
:	CommonOutputDevice()
{
#ifdef RED_PLATFORM_ORBIS
	// You tell me. Log file doesn't work when when we OOM - nice to see the stats.
	SetSafeToCallOnCrash();
#endif
}

Red::System::Log::TTYWriter::~TTYWriter()
{

}

void Red::System::Log::TTYWriter::WriteFormatted( const Char* message )
{
#if defined(RED_PLATFORM_ORBIS) && defined(UNICODE)
	// WChar tty output seems flaky so convert to char
	Red::System::AnsiChar charBuffer[ 2048 ];
	Red::System::WideCharToStdChar( charBuffer, message, sizeof(charBuffer));
	Internal::FilePrint( stdout, charBuffer );
#else
	Internal::FilePrint( stdout, message );
#endif
}
