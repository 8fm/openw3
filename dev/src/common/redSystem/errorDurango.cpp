/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "errorDurango.h"

#include <dbghelp.h>

#ifdef RED_LOGGING_ENABLED
# include "logFile.h"
#endif

namespace Red { namespace System { namespace Error {

CRITICAL_SECTION DurangoHandler::m_miniDumpMutex;
DurangoHandler::DumpInfo DurangoHandler::m_dumpInfo;
Char DurangoHandler::m_miniDumpFileName[MAX_PATH];

#ifdef RED_LOGGING_ENABLED
Char DurangoHandler::m_origLogFileName[MAX_PATH];
Char DurangoHandler::m_crashLogFileName[MAX_PATH];
#endif // RED_LOGGING_ENABLED

Bool DurangoHandler::m_hasDumped;

#define MS_VC_SET_THREAD_NAME_EXCEPTION 0x406D1388
const Uint32 DurangoHandler::NON_FATAL_EXCEPTIONS[] = { MS_VC_SET_THREAD_NAME_EXCEPTION };

//////////////////////////////////////////////////////////////////////////
// Public Interface
DurangoHandler::DurangoHandler()
:	m_logFile( nullptr )
{
	MemoryZero( &m_dumpInfo, sizeof( DurangoHandler::DumpInfo ) );
	InitializeCriticalSection( &m_miniDumpMutex );

	SetUnhandledExceptionFilter( &DurangoHandler::HandleException );

#ifdef RED_DEBUG_EXCEPTION_HANDLER
	AddVectoredExceptionHandler( TRUE, &DurangoHandler::DebuggableHandleException );
#endif

#ifdef RED_LOGGING_ENABLED
	if ( !m_miniDumpFileName[0] )
	{
		Red::System::StringCopy( m_miniDumpFileName, TXT("d:\\RedEngine.dmp"), MAX_PATH );
		Red::System::StringCopy( m_crashLogFileName, TXT("d:\\RedEngineCrash.log"), MAX_PATH );
	}
#endif // RED_LOGGING_ENABLED
}

DurangoHandler::~DurangoHandler()
{
	DeleteCriticalSection( &m_miniDumpMutex );
}

void DurangoHandler::SetMiniDumpFileName( const Char* fileName )
{
	if ( fileName && *fileName )
	{
		Red::System::StringCopy( m_miniDumpFileName, fileName, MAX_PATH );

	}
}

#ifdef RED_LOGGING_ENABLED
void DurangoHandler::SetCrashLogFileName( const Char* crashLogFileName )
{
	if ( crashLogFileName && *crashLogFileName )
	{
		Red::System::StringCopy( m_crashLogFileName, crashLogFileName, MAX_PATH );
	}
}

void DurangoHandler::SetLogFile( const Char* logFilename, Log::File* instance )
{
	Red::System::StringCopy( m_origLogFileName, logFilename, MAX_PATH );
	m_logFile = instance;
}

#else
void DurangoHandler::SetLogFile( const Char*, Log::File* )
{

}
#endif // RED_LOGGING_ENABLED

//////////////////////////////////////////////////////////////////////////
// Internal data grabbing functions
LONG WINAPI DurangoHandler::HandleException( EXCEPTION_POINTERS* exceptionInfo )
{
	OutputDebugStringA( "Exception Handler" );
	DurangoHandler* handler = static_cast< DurangoHandler* >( GetInstance() );
	handler->WriteDump( exceptionInfo, GetCurrentThreadId(), m_miniDumpFileName );

	return EXCEPTION_EXECUTE_HANDLER;
}

LONG CALLBACK DurangoHandler::DebuggableHandleException( PEXCEPTION_POINTERS exceptionInfo )
{
	OutputDebugStringA( "Debuggable Exception Handler" );

	const DWORD exceptionCode = exceptionInfo->ExceptionRecord->ExceptionCode;
	Bool isHarmless = false;
	for ( Uint32 i = 0; i < ARRAY_COUNT( NON_FATAL_EXCEPTIONS ); ++i )
	{
		if ( exceptionCode == NON_FATAL_EXCEPTIONS[ i ] )
		{
			isHarmless = true;
			break;
		}
	}

	if ( ! isHarmless )
	{
		DurangoHandler* handler = static_cast< DurangoHandler* >( GetInstance() );
		handler->WriteDump( exceptionInfo, GetCurrentThreadId(), TXT( "TestDump.dmp" ) );
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

Bool DurangoHandler::WriteDump( PEXCEPTION_POINTERS exceptionPointers, DWORD threadId, const Char* filename )
{
	EnterCriticalSection( &m_miniDumpMutex );
	
	// FIXME: Support incremental filename dumps, but for now log the first crash instead of just clobbering the dump for each thread that blows up
	if ( m_hasDumped )
	{
		return true;
	}

	m_hasDumped = true;

	m_dumpInfo.exceptionPointers = exceptionPointers;
	m_dumpInfo.threadId = threadId;
	m_dumpInfo.filename = filename;

	/*
	https://forums.xboxlive.com/AnswerPage.aspx?qid=6965df61-84a0-4825-939c-685e506d9f00&tgt=1

	There was an issue with the memory manager that caused a hang when attempting to write dumps with full memory in Exclusive titles.
	The fix for that was shipped with the April 2013 XDK.
	
	Next, verify that you aren't writing the dump from the same thread as the thread that crashed -- 
	you'll need to do this in a separate thread (note that you may need to reduce the memory overhead to load the libraries and create a dump [e.g., some unneeded graphics memory]).
	
	Also ensure that the handle passed to MiniDumpWriteDump() is opened for both write and read.
	
	There is a known issue where calls to MiniDumpWriteDump() with full stack will result in an incorrect stack when you call it on the application thread.
	The workaround for this is to call it off thread and pass it the right thread ID. 
	*/
	HANDLE miniDumpThreadHandle = CreateThread( NULL, 0, &DurangoHandler::WriteDumpThread, NULL, 0, NULL );

	WaitForSingleObject( miniDumpThreadHandle, INFINITE );

	DWORD returnVal;
	GetExitCodeThread( miniDumpThreadHandle, &returnVal );

	CloseHandle( miniDumpThreadHandle );
	miniDumpThreadHandle = nullptr;

#ifdef RED_LOGGING_ENABLED
	if ( *m_origLogFileName && *m_crashLogFileName )
	{
		BOOL cancelFlag = FALSE;
		COPYFILE2_EXTENDED_PARAMETERS params;
		Red::System::MemoryZero( &params, sizeof(COPYFILE2_EXTENDED_PARAMETERS) );
		params.dwSize = sizeof(COPYFILE2_EXTENDED_PARAMETERS);
		params.dwCopyFlags = 0; // COPY_FILE_NO_BUFFERING -> ERROR_INVALID_PARAMETER (probably not implemented right on Durango, sector aligned offsets and buffer ptr, or read/write sizes)
		params.pfCancel = &cancelFlag;

		// Force close the log file, otherwise CopyFile2 will return HRESULT 0x20 ( ERROR_SHARING_VIOLATION )
		// despite _wfsopen using _SH_DENYNO. Also, this should flush out the log.
		// Not thread safe, but we've already crashed and trying to lock some mutex in the log system could make us hang if a thread holding the mutex crashed etc
		// Better is we didn't use stdlib, and if the log file name wasn't hardcoded to begin with, but that also requires us not statically creating it as well...
		if( m_logFile )
		{
			m_logFile->Close();

			// This might fail under crash conditions but try anyway.
			::CopyFile2( m_origLogFileName, m_crashLogFileName, &params );
		}
	}
#endif // RED_LOGGING_ENABLED

	LeaveCriticalSection( &m_miniDumpMutex );

	return returnVal == 0;
}

DWORD WINAPI DurangoHandler::WriteDumpThread( LPVOID )
{
	Bool success = false;

	OutputDebugStringA( "WriteDump - load library" );

	HMODULE libraryHandle = LoadLibrary( TXT( "toolhelpx.dll" ) );

	if( libraryHandle )
	{
		typedef BOOL  (WINAPI *MiniDumpWriteDumpFunc)
		(
			_In_ HANDLE hProcess,
			_In_ DWORD ProcessId,
			_In_ HANDLE hFile,
			_In_ MINIDUMP_TYPE DumpType,
			_In_opt_ PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
			_In_opt_ PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
			_Reserved_ PVOID CallbackParam
		);

		OutputDebugStringA( "WriteDump - get function" );

		MiniDumpWriteDumpFunc mdwd = reinterpret_cast< MiniDumpWriteDumpFunc >( GetProcAddress( libraryHandle, "MiniDumpWriteDump" ) );

		if( mdwd )
		{
			OutputDebugStringA( "WriteDump - open file" );

			HANDLE dumpFile = CreateFile
			(
				m_dumpInfo.filename,
				GENERIC_READ | GENERIC_WRITE,
				0,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL
			);

			if( dumpFile != INVALID_HANDLE_VALUE )
			{
				MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;

				exceptionInfo.ThreadId = m_dumpInfo.threadId;
				exceptionInfo.ExceptionPointers = m_dumpInfo.exceptionPointers;
				exceptionInfo.ClientPointers = FALSE;

				OutputDebugStringA( "WriteDump - write dump" );

				success = mdwd
				(
					GetCurrentProcess(),
					GetCurrentProcessId(),
					dumpFile,
					static_cast< MINIDUMP_TYPE >( MiniDumpWithDataSegs | MiniDumpWithThreadInfo ),
					&exceptionInfo,
					NULL,
					NULL
				) != FALSE;

				CloseHandle( dumpFile );
			}
		}
	}

	return ( success )? 0 : 1;
}

//////////////////////////////////////////////////////////////////////////
// Public Interface
// 
Bool DurangoHandler::IsDebuggerPresent() const
{
	return ( ::IsDebuggerPresent() )? true : false;
}

Bool DurangoHandler::WriteDump( const Char* filename )
{
	RED_UNUSED( filename );
	return false;
}

Uint32 DurangoHandler::GetCallstack( Char* output, Uint32 outputSize, Uint32 skipFrames )
{
	RED_UNUSED( output );
	RED_UNUSED( outputSize );
	RED_UNUSED( skipFrames );
	return 0;
}

Uint32 DurangoHandler::GetCallstack( Char* output, Uint32 outputSize, const Internal::ThreadId& threadId, Uint32 skipFrames )
{
	RED_UNUSED( output );
	RED_UNUSED( outputSize );
	RED_UNUSED( threadId );
	RED_UNUSED( skipFrames );

	return 0;
}

void DurangoHandler::GetCallstack( Error::Callstack& stack, Uint32 skipFrames )
{
	RED_UNUSED( skipFrames );

	stack.numFrames = 1;
	stack.frame[ 0 ].line = 0;
	stack.frame[ 0 ].file[ 0 ] = TXT( '\0' );
	StringCopy( stack.frame[ 0 ].symbol, TXT( "Native callstack unavailable on Durango" ), RED_SYMBOL_MAX_LENGTH );
}

void DurangoHandler::GetCallstack( Callstack& stack, const Internal::ThreadId& threadId, Uint32 skipFrames )
{
	GetCallstack( stack, skipFrames );

	RED_UNUSED( threadId );
}

Uint32 DurangoHandler::EnumerateThreads( Internal::ThreadId* threadIds, Uint32 maxThreads )
{
	RED_UNUSED( threadIds );
	RED_UNUSED( maxThreads );
	return 0;
}

const Char* DurangoHandler::GetCommandline() const
{
	return TXT( "" );
}

void DurangoHandler::HandleUserChoice( EAssertAction chosenAction, const Char* cppFile, Uint32 line, const Char* expression, const Char* details )
{
	RED_UNUSED( chosenAction );
	RED_UNUSED( cppFile );
	RED_UNUSED( line );
	RED_UNUSED( expression );
	RED_UNUSED( details );
}

//////////////////////////////////////////////////////////////////////////
// Create Instance
Error::Handler* Error::Handler::GetInstance()
{
	if( m_handlerInstance == nullptr )
	{
		static DurangoHandler instance;
		Handler::SetInternalInstance( &instance );
	}
	
	return m_handlerInstance;
}

}}} // Namespace
