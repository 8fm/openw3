/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "windowsDebuggerWriter.h"

Red::System::Log::WindowsDebuggerWriter::WindowsDebuggerWriter()
:	CommonOutputDevice()
{
	SetSafeToCallOnCrash();
}

Red::System::Log::WindowsDebuggerWriter::~WindowsDebuggerWriter()
{

}

void Red::System::Log::WindowsDebuggerWriter::WriteFormatted( const Char* message )
{
	OutputDebugString( message );
}
