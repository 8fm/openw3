/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once


class CProcessRunner
{
private:
	Char _fullCommandLine[1024];

	HANDLE _stdoutRead;
	HANDLE _stdoutWrite;

	PROCESS_INFORMATION _processInformation;


public:
	CProcessRunner();
	~CProcessRunner();

	Bool Run( const String& appPath, const String& arguments, const String& workingDirectory );
	Bool WaitForFinish( Uint32 timeoutMS );

	Bool Terminate( Uint32 exitCode = 0, Bool closeHandles = true );

	void LogOutput( const String& filePath = String::EMPTY );

	Uint32 GetExitCode() const;

	String GetFullCommandLine() const { return _fullCommandLine; }
};
