/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "packageTool.h"
#include "processRunner.h"

CPackageTool::CPackageTool(const String& exeAbsolutePath )
	: m_exeAbsolutePath( exeAbsolutePath )
{
}

Bool CPackageTool::RunExe( const String& commandLine, const String& workingDirectory /*=String(TXT("."))*/ ) const
{
	CProcessRunner runner;
	if ( ! runner.Run( GetExeAbsolutePath(), commandLine, workingDirectory ) )
	{
		ERR_WCC(TXT("Failed to run '%ls' %ls"), GetExeAbsolutePath().AsChar(), commandLine.AsChar() );
		return false;
	}

	if ( ! runner.WaitForFinish( INFINITE ) )
	{
		ERR_WCC(TXT("Failed to wait for '%ls' %ls"), GetExeAbsolutePath().AsChar(), commandLine.AsChar() );
		return false;
	}

	SYSTEMTIME time;
	::GetSystemTime( &time );

	const String logFile = String::Printf( TXT(".\\packageToolPS4_%04d-%02d-%02d_%02d-%02d-%02d.log"),
		(Int32)time.wDay, (Int32)time.wMonth, (Int32)time.wYear,
		(Int32)time.wHour, (Int32)time.wMinute, (Int32)time.wSecond );

	runner.LogOutput( logFile );
	runner.LogOutput(); // output through logging system

	if ( runner.GetExitCode() != 0 )
	{
		ERR_WCC(TXT("Exit code '%u' for '%ls' %ls"), runner.GetExitCode(), GetExeAbsolutePath().AsChar(), commandLine.AsChar() );

		return false;
	}
	
	return true;
}

