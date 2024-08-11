/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "..\..\common\core\version.h"

#include <psapi.h>
#include <dbghelp.h>

#pragma comment( lib, "dbghelp.lib" )
#pragma comment( lib, "psapi.lib" )

CResourceVerifierErrorSystem GResVerifierErrorSystem;


CResourceVerifierErrorSystem::CResourceVerifierErrorSystem()
{
	CSystem::SystemTime systemTime;
	GSystem.GetSystemTime( &systemTime );

	String logHeader = String::Printf( TXT("Lava Engine [ Build: %s ] Resource verification started at %02i:%02i:%02i - %02i.%02i.%04i \n")
		, APP_VERSION_NUMBER
		, systemTime.hour
		, systemTime.minute
		, systemTime.second 
		, systemTime.day
		, systemTime.month
		, systemTime.year);

	RESV_LOG( logHeader.AsChar() );	
}

EAssertAction CResourceVerifierErrorSystem::AssertImp( const Char *file, Uint line, const Char *message )
{
	Char callStackMessage[2048*4];
	GetCallStackStringStatic( callStackMessage, ARRAY_COUNT(callStackMessage ), 3 );

	Char assertionMessage[4096];
	swprintf_s(assertionMessage, 
		TXT("\n=================")
		TXT("\nASSERTION FAILED:") TXT(" \"%s\"")
		TXT("\nfile:") TXT(" \"%s\"")
		TXT("\nline:") TXT(" \"%d\"")
		TXT("\n=================")
		TXT("\nCall Stack:")
		TXT("\n%s")
		, message
		, file
		, line
		, callStackMessage);
	
	// Writing to logfile
	RESV_ERR( assertionMessage );

	RaiseException( 0, EXCEPTION_NONCONTINUABLE, NULL, NULL );

	//__asm int 3
	return AA_Continue;
}

void CResourceVerifierErrorSystem::ErrorImp( EErrorType type, const Char *message )
{
	Char callStackMessage[2048*4];
	GetCallStackStringStatic( callStackMessage, ARRAY_COUNT(callStackMessage ), 3 );

	Char errorMessage[4096];
	swprintf_s(errorMessage, 
		TXT("\n================")
		TXT("\nERROR OCCURED:  ") TXT(" \"%s\"")
		TXT("\n================")	
		TXT("\nCall Stack:")
		TXT("\n%s")
		, message
		, callStackMessage );

	// Writing to logfile
	RESV_ERR( errorMessage );

	RaiseException( 0, EXCEPTION_NONCONTINUABLE, NULL, NULL );
}

void CResourceVerifierErrorSystem::GetCallStackStringStatic( Char* buffer, Uint size, Uint framesToSkip, EXCEPTION_POINTERS* pExcPtrs /*= NULL*/, const Char *indent /*= String::EMPTY*/, const Char *newLine /*= TXT("\r\n")*/ )
{
	extern CResourceVerifierErrorSystem GResVerifierErrorSystem;
	GResVerifierErrorSystem.GetCallStackString(buffer, size, framesToSkip, pExcPtrs, indent, newLine);
}
