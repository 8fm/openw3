/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "processRunner.h"
#include "../../common/redIO/redIO.h"
#include "../../common/redIO/redIOCommon.h"
#include "../../common/redSystem/crt.h"

CProcessRunner::CProcessRunner()
	: _stdoutRead( nullptr )
	, _stdoutWrite( nullptr )
{
	// Setup security attributes needed by the pipe
	SECURITY_ATTRIBUTES securityAttribs; 

	// Set the bInheritHandle flag so pipe handles are inherited. 
	securityAttribs.nLength = sizeof(SECURITY_ATTRIBUTES); 
	securityAttribs.bInheritHandle = TRUE; 
	securityAttribs.lpSecurityDescriptor = nullptr; 

	// Create a pipe for the orbis-psslc.exe process's STDOUT. 
	if ( !CreatePipe( &_stdoutRead, &_stdoutWrite, &securityAttribs, 1024 * 1024 ) ) 
	{
		WARN_WCC( TXT( "Unable to create process STDOUT pipe" ) );
	}

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if ( _stdoutRead && !SetHandleInformation( _stdoutRead, HANDLE_FLAG_INHERIT, 0 ) )
	{
		WARN_WCC( TXT( "Read handle process's STDOUT is inherited!" ) );
	}

	Red::System::MemoryZero( _fullCommandLine, sizeof( _fullCommandLine ) );
}


CProcessRunner::~CProcessRunner()
{
	if ( !CloseHandle( _stdoutWrite ) )
	{
		WARN_WCC( TXT("Couldn't close process's STDOUT write handle") );
	}
	if ( !CloseHandle( _stdoutRead ) )
	{
		WARN_WCC( TXT( "Couldn't close process's STDOUT read handle" ) );
	}
}


Bool CProcessRunner::Run( const String& appPath, const String& arguments, const String& workingDirectory )
{
	// Initialize process startup info
	STARTUPINFO startupInfo;
	ZeroMemory( &startupInfo, sizeof( STARTUPINFO ) );
	startupInfo.cb = sizeof( STARTUPINFO );

	// Specify redirection handles
	startupInfo.hStdError = _stdoutWrite;
	startupInfo.hStdOutput = _stdoutWrite;
	startupInfo.dwFlags |= STARTF_USESTDHANDLES;

	// Process information
	ZeroMemory( &_processInformation, sizeof( PROCESS_INFORMATION ) );

	// Combine appPath and arguments into one command line.
	Red::System::SNPrintF( _fullCommandLine, ARRAY_COUNT(_fullCommandLine), TXT("\"%ls\" %ls"), appPath.AsChar(), arguments.AsChar() );

	// Spawn shader compiler
	// Give null for app path, so it'll use the commandLine, and we get proper argv
	if ( !CreateProcess( nullptr, _fullCommandLine, nullptr, nullptr, true, 0, nullptr, workingDirectory.AsChar(), &startupInfo, &_processInformation ) )
	{
		LOG_WCC( TXT("%ls"), _fullCommandLine );

		LPVOID lpMsgBuf;
		DWORD dw = GetLastError();
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, nullptr );
		String error = String::Printf( TXT("%s"), (LPCTSTR)lpMsgBuf );
		WARN_WCC( TXT("%ls"), error.AsChar() );
		LocalFree(lpMsgBuf);

		return false;
	}


	return true;
}

Bool CProcessRunner::WaitForFinish( Uint32 timeoutMS )
{
	DWORD ret = WaitForSingleObject( _processInformation.hProcess, timeoutMS );
	if ( ret )
	{
		LOG_WCC( _fullCommandLine );
		if ( ret == WAIT_TIMEOUT )
		{
			WARN_WCC( TXT("Operation timed out.") );
		}
		else
		{
			LPVOID lpMsgBuf;
			DWORD dw = GetLastError();
			FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, nullptr );
			String error = String::Printf( TXT("%ls"), (LPCTSTR)lpMsgBuf );
			WARN_WCC( TXT("Error '%ls'"), error.AsChar() );
			LocalFree(lpMsgBuf);
		}
		return false;
	}

	return true;
}

Bool CProcessRunner::Terminate( Uint32 exitCode /*=0*/, Bool closeHandles /*=true*/ )
{
	if ( !TerminateProcess( _processInformation.hProcess, exitCode ) )
	{
		return false;
	}

	if ( closeHandles )
	{
		if ( !CloseHandle( _processInformation.hThread ) )
		{
			WARN_WCC( TXT( "Couldn't close thread: %d" ), _processInformation.dwThreadId );
			return false;
		}
		if ( !CloseHandle( _processInformation.hProcess ) )
		{
			WARN_WCC( TXT( "Couldn't close process: %d" ), _processInformation.dwProcessId );
			return false;
		}
	}	

	return true;
}

Uint32 CProcessRunner::GetExitCode() const
{
	DWORD exitCode;
	GetExitCodeProcess( _processInformation.hProcess, &exitCode );

	return exitCode;
}

void CProcessRunner::LogOutput( const String& filePath /*=String::EMPTY*/ )
{
	// Read the error report
	DWORD numBytesRead;
	Bool success;
	const Uint32 s_BufferSize = 8192;
	AnsiChar logBuf[ s_BufferSize + 1 ];

	Bool logToFile = !filePath.Empty();
	Red::IO::CNativeFileHandle errorFileHandle;
	Uint32 dummyOut;

	if ( logToFile )
	{
		if ( !errorFileHandle.Open( filePath.AsChar(), Red::IO::eOpenFlag_Write | Red::IO::eOpenFlag_Truncate | Red::IO::eOpenFlag_Create ) )
		{
			WARN_WCC( TXT("Unable to create error file '%ls', logging to stdout"), filePath.AsChar() );
			logToFile = false;
		}
	}

	while ( true )
	{
		DWORD bytesAvailable = 0;
		// We don't have a named pipe, but this function is useable for read end of anonymous pipe.
		if ( !PeekNamedPipe( _stdoutRead, nullptr, 0, nullptr, &bytesAvailable, nullptr ) )
		{
			WARN_WCC( TXT("PeekNamedPipe failed. Can't dump process output.") );
			break;
		}
		// If there's nothing to read, we're done here.
		if ( bytesAvailable == 0 )
		{
			break;
		}

		DWORD bytesToRead = Min( s_BufferSize, bytesAvailable );

		success = ( 0 != ReadFile( _stdoutRead, logBuf, bytesToRead, &numBytesRead, NULL ) );
		if ( !success || numBytesRead == 0 )
		{
			break;
		}
		logBuf[bytesToRead] = '\0';

		if ( logToFile )
		{
			RED_VERIFY( errorFileHandle.Write( logBuf, bytesToRead, dummyOut ) );
		}
		else
		{
			LOG_WCC( TXT("%ls"), ANSI_TO_UNICODE( logBuf ) );
		}
	}

	if ( logToFile )
	{
		RED_VERIFY( errorFileHandle.Close() );
	}
}
