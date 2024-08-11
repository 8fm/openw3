/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/
#include "errorWindows.h"
#include "nameHash.h"
#include "clock.h"
#include "utility.h"
#include "crt.h"
#include "log.h"

#pragma comment( lib, "dbghelp.lib" )
#pragma comment( lib, "psapi.lib" )

#ifdef UNICODE
#	define DBGHELP_TRANSLATE_TCHAR
#endif

#include <dbghelp.h>
#include <TlHelp32.h>
#include <direct.h>

#define RED_SYMBOL_PATHS_MAX_LENGTH ( 2048 )

#define RED_PATH_SHRINK				TXT( "src\\" )
#define RED_EXCEPTION_MAPFILE_NAME	TXT( "red_exception_info" )


// This must not be defined when running without a debugger as it will also receive "Handled" exceptions
// Such as DBG_PRINTEXCEPTION_C which happens every time someone prints to the log
// #define	RED_DEBUG_EXCEPTION_HANDLER

//////////////////////////////////////////////////////////////////////////
// 

namespace
{
	class ScopedLock
	{
	public:

		ScopedLock( CRITICAL_SECTION& criticalSection )
			: m_criticalSection( &criticalSection )
		{
			::EnterCriticalSection( m_criticalSection );
		}

		~ScopedLock()
		{
			::LeaveCriticalSection( m_criticalSection );
		}

	private:

		CRITICAL_SECTION* m_criticalSection;
	};

}

namespace Red { namespace System { namespace Error {

WindowsHandler::ThreadHandle::ThreadHandle()
:	opened( false )
,	handle( NULL )
{

}

WindowsHandler::ScopedThreadHandle::~ScopedThreadHandle()
{
	if( opened )
	{
		CloseHandle( handle );
	}
}

//////////////////////////////////////////////////////////////////////////
// 

WindowsHandler::WindowsHandler()
#ifdef RED_CRASH_HANDLER_CREATES_DUMP
#ifdef RED_DEBUG_EXCEPTION_HANDLER
:	m_exceptionInfoMapFile( NULL )
,	m_exceptionInfoBuffer( nullptr )
#endif
#endif
{
	::InitializeCriticalSection( &m_criticalSection );

	StringCopy( m_logFilepath, TXT( "Not Set" ), ARRAY_COUNT( m_logFilepath ) );

	::SymSetOptions( SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_DEBUG );
	//! PLEASE DO NOT ADD USER SEARCH PATHS, IF YOU NEED ADD PDB FOR NEW PROJECT CONTACT WITH BUILD MASTER
	::SymInitialize( GetCurrentProcess(), NULL, TRUE );


#ifdef RED_DEBUG_EXCEPTION_HANDLER
	AddVectoredExceptionHandler( TRUE, &WindowsHandler::DebuggableHandleException );
#endif

	SetUnhandledExceptionFilter( &WindowsHandler::HandleException );
	_set_purecall_handler( &WindowsHandler::PureCallHandler );

#ifdef RED_CRASH_HANDLER_CREATES_DUMP
	m_exceptionInfoMapFile = CreateFileMapping
	(
		INVALID_HANDLE_VALUE,        // use paging file
		NULL,                        // default security
		PAGE_READWRITE,              // read/write access
		0,                           // maximum object size (high-order DWORD)
		sizeof( CrashInfo ),         // maximum object size (low-order DWORD)
		RED_EXCEPTION_MAPFILE_NAME   // name of mapping object
	);

	if( m_exceptionInfoMapFile )
	{
		m_exceptionInfoBuffer = MapViewOfFile
		(
			m_exceptionInfoMapFile,
			FILE_MAP_ALL_ACCESS,
			0,
			0,
			sizeof( CrashInfo )
		);
	}

#endif
}

WindowsHandler::~WindowsHandler()
{
#ifdef RED_CRASH_HANDLER_CREATES_DUMP
	if( m_exceptionInfoBuffer )
	{
		UnmapViewOfFile( m_exceptionInfoBuffer );
		CloseHandle( m_exceptionInfoMapFile );
	}
#endif

	::SymCleanup( GetCurrentProcess() );

	::DeleteCriticalSection( &m_criticalSection );
}

//////////////////////////////////////////////////////////////////////////
// Internal data grabbing functions

Uint32 WindowsHandler::WalkStack( StackInfo* stack, Uint32 size, ThreadHandle& threadHandle, PCONTEXT threadContext, Uint32 framesToSkip )
{
	ScopedLock lock( m_criticalSection );

	if( threadHandle.handle == GetCurrentThread() )
	{
		PVOID addresses[ RED_MAX_STACK_FRAMES ];
		Uint32 stackSize = ( size > RED_MAX_STACK_FRAMES ) ? RED_MAX_STACK_FRAMES : size;

		USHORT frames = RtlCaptureStackBackTrace( framesToSkip, stackSize, addresses, NULL );

		for( Uint32 i = 0; i < frames; ++i )
		{
			MemUint address = reinterpret_cast< MemUint >( addresses[ i ] );
			stack[ i ].frameAddress = static_cast< DWORD64 >( address );
		}

		return frames;
	}

	STACKFRAME64 stackFrame;
	ZeroMemory( &stackFrame, sizeof( STACKFRAME64 ) );

	stackFrame.AddrPC.Mode			= AddrModeFlat;
	stackFrame.AddrFrame.Mode		= AddrModeFlat;
	stackFrame.AddrStack.Mode		= AddrModeFlat;

#if defined( RED_ARCH_X86 )
	DWORD machineType				= IMAGE_FILE_MACHINE_I386;
	stackFrame.AddrPC.Offset		= threadContext->Eip;
	stackFrame.AddrFrame.Offset		= threadContext->Ebp;
	stackFrame.AddrStack.Offset		= threadContext->Esp;
#elif defined( RED_ARCH_X64 )
	DWORD machineType				= IMAGE_FILE_MACHINE_AMD64;
	stackFrame.AddrPC.Offset		= threadContext->Rip;
	stackFrame.AddrFrame.Offset		= threadContext->Rbp;
	stackFrame.AddrStack.Offset		= threadContext->Rsp;
#elif defined( RED_ARCH_IA64 )
	DWORD machineType				= IMAGE_FILE_MACHINE_IA64;
	stackFrame.AddrPC.Offset		= threadContext->StIIP;
	stackFrame.AddrFrame.Offset		= threadContext->RsBSP;
	stackFrame.AddrStack.Offset		= threadContext->IntSp;
	stackFrame.AddrBStore.Offset	= threadContext->RsBSP;
	stackFrame.AddrBStore.Mode		= AddrModeFlat;
#else
#	error Cannot Unwind stack for undefined architechture
#endif

	Uint32 totalFrames = framesToSkip + size;
	Uint32 frameIndex = 0;
	for( Uint32 i = 0; i < totalFrames; ++i )
	{
		if
		(
			StackWalk64
			(
				machineType,
				::GetCurrentProcess(),
				threadHandle.handle,
				&stackFrame,
				threadContext,
				NULL,
				SymFunctionTableAccess64,
				SymGetModuleBase64,
				NULL
			)
		)
		{
			if ( stackFrame.AddrFrame.Offset != 0 )
			{
				if( i >= framesToSkip )
				{
					stack[ frameIndex ].frameAddress = stackFrame.AddrPC.Offset;

					++frameIndex;
				}
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	return frameIndex;
}

void WindowsHandler::GetStackSymbols( StackInfo* stack, Uint32 numValidStackFrames )
{
	// Alignment still can be off, but if even the MSDN example does that...
	const size_t buffSize = sizeof( SYMBOL_INFO ) + RED_SYMBOL_MAX_LENGTH * sizeof(Char);
	Uint8 buffer[ buffSize ];

	for( Uint32 i = 0; i < numValidStackFrames; ++i )
	{
		MemorySet( buffer, 0, ARRAY_COUNT( buffer ) );

		PSYMBOL_INFO symbol = reinterpret_cast< PSYMBOL_INFO >( buffer );

		// Initialise data struct.
		symbol->SizeOfStruct	= sizeof( SYMBOL_INFO );
		symbol->MaxNameLen		= RED_SYMBOL_MAX_LENGTH;

		if( SymFromAddr( GetCurrentProcess(), stack[ i ].frameAddress, &stack[ i ].symbolDisplacement, symbol ) )
		{
			MemoryCopy( stack[ i ].symbol, symbol->Name, sizeof( Char ) * ( symbol->NameLen + 1 ) );
			stack[ i ].symbol[ symbol->NameLen + 1 ] = '\0';
		}
		else
		{
			stack[ i ].symbolDisplacement = 0;
			StringCopy( stack[ i ].symbol, TXT( "<Unknown Symbol>" ), RED_SYMBOL_MAX_LENGTH );
		}

		IMAGEHLP_LINE64 line;
		line.SizeOfStruct = sizeof( IMAGEHLP_LINE64 );
		if( SymGetLineFromAddr64( GetCurrentProcess(), stack[ i ].frameAddress, &stack[ i ].lineDisplacement, &line ) )
		{
			Char* filename = StringSearch( line.FileName, RED_PATH_SHRINK );

			if( filename )
			{
				// Remove RED_PATH_SHRINK from front of path as well
				filename = &filename[ StringLengthCompileTime( RED_PATH_SHRINK ) ];
			}
			else
			{
				// Display full path
				filename = line.FileName;
			}

			stack[ i ].lineNumber = static_cast< Uint32 >( line.LineNumber );
			StringCopy( stack[ i ].filename, filename, RED_SYMBOL_MAX_LENGTH );
		}
		else
		{
			stack[ i ].lineNumber = 0;
			stack[ i ].lineDisplacement = 0;
			StringCopy( stack[ i ].filename, TXT( "<Unknown File>" ), RED_SYMBOL_MAX_LENGTH );
		}
	}
}

Uint32 WindowsHandler::PrintStack( StackInfo* stack, Uint32 numFrames, const Internal::ThreadId& threadId, Char* output, Uint32 outputSize )
{
	Uint32 bufferUsed = 0;

	RED_APPEND_ERROR_STRING( output, outputSize, bufferUsed, TXT( "Thread ID: %u\n\n" ), threadId.id );

	// For the test track reporter parser
	RED_APPEND_ERROR_STRING( output, outputSize, bufferUsed, TXT( "callstack:\n" ) );

	for( Uint32 i = 0; i < numFrames; ++i )
	{
		RED_APPEND_ERROR_STRING( output, outputSize, bufferUsed, TXT( "%.*" ) MACRO_TXT( RED_PRIs ) TXT( "() + 0x%llx" ), RED_SYMBOL_MAX_LENGTH, stack[ i ].symbol, stack[ i ].symbolDisplacement );
		RED_APPEND_ERROR_STRING( output, outputSize, bufferUsed, TXT( " - " ) );
		RED_APPEND_ERROR_STRING( output, outputSize, bufferUsed, TXT( "%" ) MACRO_TXT( RED_PRIs ) TXT( "(%d)" ), stack[ i ].filename, stack[ i ].lineNumber );
		RED_APPEND_ERROR_STRING( output, outputSize, bufferUsed, TXT( "\n" ) );
	}

	return bufferUsed;
}

Bool WindowsHandler::GetThreadContext( const Internal::ThreadId& threadId, ThreadHandle& thread, CONTEXT& context )
{
	if( threadId.id == GetCurrentThreadId() )
	{
		thread.handle = GetCurrentThread();
		GetThreadContext( thread, context );
		return true;
	}

	thread.handle = OpenThread( THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, threadId.id );

	if( thread.handle )
	{
		thread.opened = true;
		GetThreadContext( thread, context );

		return true;
	}

	return false;
}

void WindowsHandler::GetThreadContext( ThreadHandle& thread, CONTEXT& context )
{
	MemorySet( &context, 0, sizeof( CONTEXT ) );
	context.ContextFlags = CONTEXT_ALL;

	if( thread.handle == GetCurrentThread() )
	{
		RtlCaptureContext( &context );
	}
	else
	{
		Uint32 suspendCount = SuspendThread( thread.handle );
		if( suspendCount != -1 )
		{
			::GetThreadContext( thread.handle, &context );
			ResumeThread( thread.handle );
		}
	}
}

Uint32 WindowsHandler::GetLastError( Char* output, Uint32 outputSize )
{
	DWORD errorCode = ::GetLastError();

	DWORD outputUsed = ::FormatMessage
	(
		(
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS
		),
		NULL,
		errorCode,
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
		output,
		outputSize,
		NULL
	);

	return static_cast< Uint32 >( outputUsed );
}

Uint32 WindowsHandler::ExceptionAsString( PEXCEPTION_POINTERS exceptionPointers, Char* output, Uint32 outputSize )
{
	Uint32 outputUsed = 0;

	const Char* exceptionStr = ExceptionAsString( exceptionPointers->ExceptionRecord->ExceptionCode );

	switch( exceptionPointers->ExceptionRecord->ExceptionCode )
	{
	case EXCEPTION_ACCESS_VIOLATION:
		if ( exceptionPointers->ExceptionRecord->NumberParameters >= 2 )
		{
			const Char* rwStr = ( exceptionPointers->ExceptionRecord->ExceptionInformation[0] == 0 ) ? TXT( "reading" ) : TXT( "writing" );
			outputUsed = SNPrintF
			(
				output,
				outputSize,
				TXT( "%" ) MACRO_TXT( RED_PRIs ) TXT( " (%u), Error %s location 0x%08x" ),
				exceptionStr,
				exceptionPointers->ExceptionRecord->ExceptionCode,
				rwStr,
				exceptionPointers->ExceptionRecord->ExceptionInformation[1]
			);
			break;
		}

	default:
		outputUsed = SNPrintF( output, outputSize, TXT( "%" ) MACRO_TXT( RED_PRIs ) TXT( " (%u)" ), exceptionStr, exceptionPointers->ExceptionRecord->ExceptionCode );
	}

	return outputUsed;
}

const Char* WindowsHandler::ExceptionAsString( DWORD code )
{
	switch( code )
	{
		case EXCEPTION_ACCESS_VIOLATION:         return TXT( "EXCEPTION_ACCESS_VIOLATION" )         ;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return TXT( "EXCEPTION_ARRAY_BOUNDS_EXCEEDED" )    ;
		case EXCEPTION_BREAKPOINT:               return TXT( "EXCEPTION_BREAKPOINT" )               ;
		case EXCEPTION_DATATYPE_MISALIGNMENT:    return TXT( "EXCEPTION_DATATYPE_MISALIGNMENT" )    ;
		case EXCEPTION_FLT_DENORMAL_OPERAND:     return TXT( "EXCEPTION_FLT_DENORMAL_OPERAND" )     ;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return TXT( "EXCEPTION_FLT_DIVIDE_BY_ZERO" )       ;
		case EXCEPTION_FLT_INEXACT_RESULT:       return TXT( "EXCEPTION_FLT_INEXACT_RESULT" )       ;
		case EXCEPTION_FLT_INVALID_OPERATION:    return TXT( "EXCEPTION_FLT_INVALID_OPERATION" )    ;
		case EXCEPTION_FLT_OVERFLOW:             return TXT( "EXCEPTION_FLT_OVERFLOW" )             ;
		case EXCEPTION_FLT_STACK_CHECK:          return TXT( "EXCEPTION_FLT_STACK_CHECK" )          ;
		case EXCEPTION_FLT_UNDERFLOW:            return TXT( "EXCEPTION_FLT_UNDERFLOW" )            ;
		case EXCEPTION_ILLEGAL_INSTRUCTION:      return TXT( "EXCEPTION_ILLEGAL_INSTRUCTION" )      ;
		case EXCEPTION_IN_PAGE_ERROR:            return TXT( "EXCEPTION_IN_PAGE_ERROR" )            ;
		case EXCEPTION_INT_DIVIDE_BY_ZERO:       return TXT( "EXCEPTION_INT_DIVIDE_BY_ZERO" )       ;
		case EXCEPTION_INT_OVERFLOW:             return TXT( "EXCEPTION_INT_OVERFLOW" )             ;
		case EXCEPTION_INVALID_DISPOSITION:      return TXT( "EXCEPTION_INVALID_DISPOSITION" )      ;
		case EXCEPTION_NONCONTINUABLE_EXCEPTION: return TXT( "EXCEPTION_NONCONTINUABLE_EXCEPTION" ) ;
		case EXCEPTION_PRIV_INSTRUCTION:         return TXT( "EXCEPTION_PRIV_INSTRUCTION" )         ;
		case EXCEPTION_SINGLE_STEP:              return TXT( "EXCEPTION_SINGLE_STEP" )              ;
		case EXCEPTION_STACK_OVERFLOW:           return TXT( "EXCEPTION_STACK_OVERFLOW" )           ;
		default:                                 return TXT( "UNKNOWN EXCEPTION" )                  ;
	}
}

Bool WindowsHandler::WriteDump( PEXCEPTION_POINTERS exceptionPointers, DWORD threadId, const Char* filename )
{
	Bool success = false;

	HANDLE dumpFile = CreateFile
	(
		filename,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if( dumpFile != INVALID_HANDLE_VALUE )
	{
		MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;

		exceptionInfo.ThreadId = threadId;
		exceptionInfo.ExceptionPointers = exceptionPointers;
		exceptionInfo.ClientPointers = FALSE;
		
		OutputDebugStringA( "!-- Writing mini dump crash file...\n" );

		success = MiniDumpWriteDump
		(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			dumpFile,
			static_cast< MINIDUMP_TYPE >( MiniDumpWithDataSegs | MiniDumpWithThreadInfo ),
			&exceptionInfo,
			NULL,
			NULL
		) != FALSE;

		OutputDebugStringA( "!-- Mini dump crash file written\n" );

		CloseHandle( dumpFile );
	}

	return success;
}

Bool WindowsHandler::GenerateExceptionPointers( const Internal::ThreadId& threadId, EXCEPTION_POINTERS& exceptionInfo, EXCEPTION_RECORD& exceptionRecord, CONTEXT& context )
{
	MemorySet( &exceptionInfo, 0, sizeof( EXCEPTION_POINTERS ) );
	MemorySet( &exceptionRecord, 0, sizeof( EXCEPTION_RECORD ) );
	MemorySet( &context, 0, sizeof( CONTEXT ) );

	ScopedThreadHandle thread;
	if( GetThreadContext( threadId, thread, context ) )
	{
		exceptionRecord.ExceptionCode		= EXCEPTION_BREAKPOINT;
		exceptionRecord.ExceptionFlags		= 0;
		exceptionRecord.ExceptionRecord		= NULL;
#ifdef RED_ARCH_X86
		exceptionRecord.ExceptionAddress	= reinterpret_cast< PVOID >( context.Eip );
#endif

		exceptionInfo.ContextRecord			= &context;
		exceptionInfo.ExceptionRecord		= &exceptionRecord;

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
// Public interface implementations

Bool WindowsHandler::IsDebuggerPresent() const
{
	return ( ::IsDebuggerPresent() )? true : false;
}

Bool WindowsHandler::WriteDump( const Char* filename )
{
	ScopedLock lock( m_criticalSection );

	Internal::ThreadId thread;
	thread.id = GetCurrentThreadId();

	return WriteDump( filename, thread );
}

Bool WindowsHandler::WriteDump( const Char* filename, const Internal::ThreadId& threadId )
{
	ScopedLock lock( m_criticalSection );

	EXCEPTION_POINTERS exceptionPointers;
	EXCEPTION_RECORD exceptionRecord;
	CONTEXT context;

	if( GenerateExceptionPointers( threadId, exceptionPointers, exceptionRecord, context ) )
	{
		return WriteDump( &exceptionPointers, threadId.id, filename );
	}

	return false;
}


Uint32 WindowsHandler::GetCallstack( Char* output, Uint32 outputSize, Uint32 skipFrames )
{
	ScopedLock lock( m_criticalSection );

	Internal::ThreadId threadId;
	threadId.id = GetCurrentThreadId();

	Uint32 outputUsed = 0;

	outputUsed += GetCallstack( output, outputSize, threadId, ++skipFrames );

	if ( MessageStack::HasStackMessages() )
	{
		outputUsed += RED_APPEND_ERROR_STRING( output, outputSize, outputUsed, TXT("\nError stack messages:\n") );
		outputUsed += MessageStack::CompileReport( output + outputUsed, outputSize - outputUsed );
	}

	return outputUsed;
}


Uint32 WindowsHandler::GetCallstack( Char* output, Uint32 outputSize, const Internal::ThreadId& threadId, Uint32 skipFrames )
{
	ScopedLock lock( m_criticalSection );

	Uint32 outputUsed = 0;

	StackInfo stack[ RED_MAX_STACK_FRAMES ];

	ScopedThreadHandle thread;
	CONTEXT context;
	if( GetThreadContext( threadId, thread, context ) )
	{
		// Remove GetCallstack() and WalkStack()
		skipFrames += 2;

		Uint32 numFrames = WalkStack( stack, RED_MAX_STACK_FRAMES, thread, &context, skipFrames );
		GetStackSymbols( stack, numFrames );

		outputUsed = PrintStack( stack, numFrames, threadId, output, outputSize );
	}

	return outputUsed;
}


void WindowsHandler::GetCallstack( Callstack& stackInformation, Uint32 skipFrames )
{
	ScopedLock lock( m_criticalSection );

	Internal::ThreadId threadId;
	threadId.id = ::GetCurrentThreadId();

	++skipFrames;

	GetCallstack( stackInformation, threadId, skipFrames );
}

void WindowsHandler::GetCallstack( Callstack& stackInformation, const Internal::ThreadId& threadId, Uint32 skipFrames )
{
	ScopedLock lock( m_criticalSection );

	StackInfo stack[ RED_MAX_STACK_FRAMES ];

	ScopedThreadHandle thread;
	CONTEXT context;
	if( GetThreadContext( threadId, thread, context ) )
	{
		// Remove GetCallstack() and WalkStack()
		skipFrames += 2;

		Uint32 numFrames = WalkStack( stack, RED_MAX_STACK_FRAMES, thread, &context, skipFrames );
		GetStackSymbols( stack, numFrames );

		stackInformation.numFrames = numFrames;

		for( Uint32 i = 0; i < numFrames; ++i )
		{
			StringCopy( stackInformation.frame[ i ].file, stack[ i ].filename, ARRAY_COUNT( stackInformation.frame[ i ].file ) );
			StringCopy( stackInformation.frame[ i ].symbol, stack[ i ].symbol, ARRAY_COUNT( stackInformation.frame[ i ].symbol ) );

			stackInformation.frame[ i ].line = stack[ i ].lineNumber;
		}
	}
}

Uint32 WindowsHandler::EnumerateThreads( Internal::ThreadId* threadIds, Uint32 maxThreads )
{
	ScopedLock lock( m_criticalSection );

	Uint32 numberOfThreads = 0;

	HANDLE snapshotHandle = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, GetCurrentProcessId() );

	if( snapshotHandle != INVALID_HANDLE_VALUE )
	{
		THREADENTRY32 threadEntry;
		threadEntry.dwSize = sizeof( THREADENTRY32 );

		if( Thread32First( snapshotHandle, &threadEntry ) ) 
		{
			do
			{
				// Make sure the thread belongs to our process (CreateToolhelp32Snapshot() gets all threads system wide!)
				if( threadEntry.th32OwnerProcessID == GetCurrentProcessId() )
				{
					threadIds[ numberOfThreads ].id = threadEntry.th32ThreadID;

					++numberOfThreads;

					// Make sure we don't overrun the supplied array
					if( numberOfThreads >= maxThreads )
					{
						break;
					}
				}
			} while( Thread32Next( snapshotHandle, &threadEntry ) );
		}

		CloseHandle( snapshotHandle );
	}

	return numberOfThreads;
}

const Char* WindowsHandler::GetCommandline() const
{
	return GetCommandLine();
}

void WindowsHandler::HandleUserChoice( EAssertAction chosenAction, const Char*, Uint32, const Char* expression, const Char* details )
{
	switch( chosenAction )
	{
	case AA_Continue:
	case AA_Stop:
		{
			EXCEPTION_POINTERS exceptionInfo;
			EXCEPTION_RECORD exceptionRecord;
			CONTEXT context;

			Internal::ThreadId threadId;
			threadId.id = GetCurrentThreadId();

			if( GenerateExceptionPointers( threadId, exceptionInfo, exceptionRecord, context ) )
			{
				//Filename
				DateTime crashtime;
				Clock::GetInstance().GetUTCTime( crashtime );

				Char executableFileName[ MAX_PATH ];
				GetModuleFileName( NULL, executableFileName, ARRAY_COUNT( executableFileName ) );

				Char filename[ MAX_PATH ];
				SNPrintF
				(
					filename,
					ARRAY_COUNT( filename ),
					TXT( "%s_assert_%i%02i%02i_%02i%02i%02i%03i" ),
					executableFileName,
					crashtime.GetYear(),
					crashtime.GetMonth(),
					crashtime.GetDay(),
					crashtime.GetHour(),
					crashtime.GetMinute(),
					crashtime.GetSecond(),
					crashtime.GetMilliSeconds()
				);

				Char dumpFileName[ MAX_PATH ];
#ifdef RED_CRASH_HANDLER_CREATES_DUMP
				if( !m_exceptionInfoBuffer )
				{
#endif
					// Crash Dump
					SNPrintF( dumpFileName, ARRAY_COUNT( dumpFileName ), TXT( "%" ) MACRO_TXT( RED_PRIs ) TXT( ".dmp" ), filename );
					WriteDump( &exceptionInfo, GetCurrentThreadId(), dumpFileName );
#ifdef RED_CRASH_HANDLER_CREATES_DUMP
				}
#endif

				// Write information to "crash info" file
				Char crashdataFilename[ MAX_PATH ];
				SNPrintF( crashdataFilename, ARRAY_COUNT( crashdataFilename ), TXT( "%" ) MACRO_TXT( RED_PRIs ) TXT( ".crashinfo" ), filename );

				FILE* crashdataFile;
				Internal::FileOpen( &crashdataFile, crashdataFilename, TXT( "w" ) );
				Internal::FilePrint( crashdataFile, TXT( "Red Engine Assert Log\n" ) );
				Internal::FilePrint( crashdataFile, TXT( "--------------------------------------\n" ) );
				Internal::FilePrintF( crashdataFile, TXT( "Build: %" ) MACRO_TXT( RED_PRIs ) TXT( " [ Compiled %" ) MACRO_TXT( RED_PRIs ) TXT( " ]\n\n" ), GetVersion(), MACRO_TXT( __DATE__ ) );
				
				Char lpszUsername[ 255 ];
				DWORD dUsername = sizeof( lpszUsername );
				if( GetUserName( lpszUsername, &dUsername ) )
				{
					Internal::FilePrintF( crashdataFile, TXT ( "User: %" ) RED_PRIWs TXT ( "\n\n" ), lpszUsername );
				}
				else
				{
					Internal::FilePrintF( crashdataFile, TXT ( "Unknown user\n\n" ) );
				}

				Internal::FilePrint( crashdataFile, details );
				Internal::FileClose( crashdataFile );

				Char crashReporterCommandLine[ 2048 ];

				Uint32 crCmdUsed = 
#ifdef RED_CRASH_HANDLER_CREATES_DUMP
				( !m_exceptionInfoBuffer )?
#endif
					SNPrintF
					(
						crashReporterCommandLine,
						ARRAY_COUNT( crashReporterCommandLine ),
							TXT( "TestTrackReporter.exe -c \"%" ) MACRO_TXT( RED_PRIs ) TXT( "\"" )
							TXT( " -d \"%" ) MACRO_TXT( RED_PRIs ) TXT( "\"" )
							TXT( " -l \"%" ) MACRO_TXT( RED_PRIs ) TXT( "\"" )
							TXT( " -exe \"%" ) MACRO_TXT( RED_PRIs ) TXT( "\"" ),
						crashdataFilename,
						dumpFileName,
						m_logFilepath,
						executableFileName
					)

#ifdef RED_CRASH_HANDLER_CREATES_DUMP
					:
					SNPrintF
					(
						crashReporterCommandLine,
						ARRAY_COUNT( crashReporterCommandLine ),
							TXT( "TestTrackReporter.exe -c \"%" ) MACRO_TXT( RED_PRIs ) TXT( "\"" )
							TXT( " -pid \"%u\" -tid \"%u\" -exp \"%" ) MACRO_TXT( RED_PRIs ) TXT( "\"" )
							TXT( " -l \"%" ) MACRO_TXT( RED_PRIs ) TXT( "\"" )
							TXT( " -exe \"%" ) MACRO_TXT( RED_PRIs ) TXT( "\"" ),
						crashdataFilename,
						GetCurrentProcessId(),
						GetCurrentThreadId(),
						RED_EXCEPTION_MAPFILE_NAME,
						m_logFilepath,
						executableFileName
					)
#endif
					;

				RED_APPEND_ERROR_STRING( crashReporterCommandLine, ARRAY_COUNT( crashReporterCommandLine ), crCmdUsed, TXT( " -u \"[ASSERT] %" ) MACRO_TXT( RED_PRIs ) TXT( "\"" ), expression );

				if( HasAssertFlag( AC_SilentCrashHook ) )
				{
					RED_APPEND_ERROR_STRING( crashReporterCommandLine, ARRAY_COUNT( crashReporterCommandLine ), crCmdUsed, TXT( " -nowindow" ) );
				}

				RED_LOG_FLUSH();

				STARTUPINFO startupInfo;
				MemorySet( &startupInfo, 0, sizeof( startupInfo ) );

				startupInfo.cb = sizeof( startupInfo );
				startupInfo.dwFlags = STARTF_USESHOWWINDOW;
				startupInfo.wShowWindow = SW_SHOW;

				PROCESS_INFORMATION processInfo;
				MemorySet( &processInfo, 0, sizeof( processInfo ) );

				BOOL success = CreateProcess
				(
					NULL,
					crashReporterCommandLine,
					NULL,
					NULL,
					FALSE,
					NORMAL_PRIORITY_CLASS,
					NULL,
					NULL,
					&startupInfo,
					&processInfo
				);


#ifdef RED_CRASH_HANDLER_CREATES_DUMP
				if( success && m_exceptionInfoBuffer )
				{
					WaitForSingleObject( processInfo.hProcess, INFINITE );
				}
#else
				RED_UNUSED( success );
#endif
			}
		}
		break;
	}
}


//////////////////////////////////////////////////////////////////////////
// Windows interface

#ifdef RED_DEBUG_EXCEPTION_HANDLER
LONG CALLBACK WindowsHandler::DebuggableHandleException( PEXCEPTION_POINTERS exceptionInfo )
{
	WindowsHandler* handler = static_cast< WindowsHandler* >( Handler::GetInstance() );

	handler->DoHandleException( exceptionInfo );

	return EXCEPTION_CONTINUE_SEARCH;
}
#endif

LONG WINAPI WindowsHandler::HandleException( EXCEPTION_POINTERS* exceptionInfo )
{
	WindowsHandler* handler = static_cast< WindowsHandler* >( Handler::GetInstance() );

	handler->DoHandleException( exceptionInfo );

	return EXCEPTION_EXECUTE_HANDLER;
}

void WindowsHandler::DoHandleException( EXCEPTION_POINTERS* exceptionInfo )
{
	ScopedLock lock( m_criticalSection );

	Internal::ThreadId threads[ RED_MAX_THREAD_CALLSTACKS ];
	const Uint32 numThreads = EnumerateThreads( threads, ARRAY_COUNT( threads ) );

	Internal::ThreadId thisTid;
	thisTid.InitWithCurrentThread();

	// Bit of a race here, but good enough for our purposes, which is to stop people from continuing the game!
	for ( Uint32 i = 0; i < numThreads; ++i )
	{
		const Internal::ThreadId tid = threads[ i ];
		if ( tid.id == thisTid.id )
		{
			continue;
		}
		
		HANDLE hThread = ::OpenThread( THREAD_SUSPEND_RESUME, FALSE, tid.id );

		if ( hThread )
		{
			::SuspendThread( hThread ); // Overly Attached Exception Handler is never going to let you go...
		}
	}

	//Filename
	DateTime crashtime;
	Clock::GetInstance().GetUTCTime( crashtime );

	Char executableFileName[ MAX_PATH ];
	GetModuleFileName( NULL, executableFileName, ARRAY_COUNT( executableFileName ) );

	Char filename[ MAX_PATH ];
	SNPrintF
	(
		filename,
		ARRAY_COUNT( filename ),
		TXT( "%s_crash_%i%02i%02i_%02i%02i%02i%03i" ),
		executableFileName,
		crashtime.GetYear(),
		crashtime.GetMonth(),
		crashtime.GetDay(),
		crashtime.GetHour(),
		crashtime.GetMinute(),
		crashtime.GetSecond(),
		crashtime.GetMilliSeconds()
	);

	Char dumpFileName[ MAX_PATH ];

#ifdef RED_CRASH_HANDLER_CREATES_DUMP
	if( m_exceptionInfoBuffer )
	{
		CrashInfo* info = reinterpret_cast< CrashInfo* >( m_exceptionInfoBuffer );
		MemoryCopy( &info->record, exceptionInfo->ExceptionRecord, sizeof( EXCEPTION_RECORD ) );
		MemoryCopy( &info->context, exceptionInfo->ContextRecord, sizeof( CONTEXT ) );

		FlushViewOfFile( m_exceptionInfoBuffer, sizeof( CrashInfo ) );
	}
	else
	{
#endif
		// Crash Dump
		SNPrintF( dumpFileName, ARRAY_COUNT( dumpFileName ), TXT( "%" ) MACRO_TXT( RED_PRIs ) TXT( ".dmp" ), filename );
		WriteDump( exceptionInfo, GetCurrentThreadId(), dumpFileName );
#ifdef RED_CRASH_HANDLER_CREATES_DUMP
	}
#endif

	// Log
	static const Uint32 debugStringSize = 1024 * 5 * 16;
	Char debugString[ debugStringSize ];
	Uint32 debugStringUsed = 0;

	RED_APPEND_ERROR_STRING( debugString, debugStringSize, debugStringUsed, TXT( "Red Engine Crash Log\n" ) );
	RED_APPEND_ERROR_STRING( debugString, debugStringSize, debugStringUsed, TXT( "--------------------------------------\n" ) );
	RED_APPEND_ERROR_STRING( debugString, debugStringSize, debugStringUsed, TXT( "Build: %" ) MACRO_TXT( RED_PRIs ) TXT( " [ Compiled %" ) MACRO_TXT( RED_PRIs ) TXT( " ]\n\n" ), GetVersion(), MACRO_TXT( __DATE__ ) );
	
	Char lpszUsername[ 255 ];
	DWORD dUsername = sizeof( lpszUsername );
	if( GetUserName( lpszUsername, &dUsername ) )
	{
		RED_APPEND_ERROR_STRING( debugString, debugStringSize, debugStringUsed, TXT ( "User: %" ) RED_PRIWs TXT ( "\n\n" ), lpszUsername );
	}
	else
	{
		RED_APPEND_ERROR_STRING( debugString, debugStringSize, debugStringUsed, TXT ( "Unknown user\n\n" ) );
	}

	RED_APPEND_ERROR_STRING( debugString, debugStringSize, debugStringUsed, TXT( "Crash reason: " ) );
	debugStringUsed += ExceptionAsString( exceptionInfo, &debugString[ debugStringUsed ], debugStringSize - debugStringUsed );

	// stack messages from  crash site - may be usefull for debugging
	if ( MessageStack::HasStackMessages() )
	{
		RED_APPEND_ERROR_STRING( debugString, debugStringSize, debugStringUsed, TXT( "\n\nCrash stack messages:\n" ) );
		debugStringUsed += MessageStack::CompileReport( &debugString[ debugStringUsed ], debugStringSize - debugStringUsed );
	}

	RED_APPEND_ERROR_STRING( debugString, debugStringSize, debugStringUsed, TXT( "\n\nCrashed Thread:\n\n" ) );

	::SymRefreshModuleList( GetCurrentProcess() );
	// The callstack for the thread that threw the exception (this thread)
	StackInfo stack[ RED_MAX_STACK_FRAMES ];

	Internal::ThreadId threadId;
	threadId.id = GetCurrentThreadId();

	ThreadHandle thread;
	thread.handle = NULL;

	Uint32 numFrames = WalkStack( stack, RED_MAX_STACK_FRAMES, thread, exceptionInfo->ContextRecord );
	GetStackSymbols( stack, numFrames );
	debugStringUsed += PrintStack( stack, numFrames, threadId, &debugString[ debugStringUsed ], debugStringSize - debugStringUsed );
	RED_APPEND_ERROR_STRING( debugString, debugStringSize, debugStringUsed, TXT( "\n" ) );

	// All other callstacks and debug information
	debugStringUsed += GetDebugInformation
	(
		&debugString[ debugStringUsed ],
		debugStringSize - debugStringUsed,
		DIF_ExcludeThreadCallstack | DIF_AllCallBackInformation,
		0,
		CNameHash(),
		threadId
	);

	// MattH: If the debug string contains 'Out of Memory', we add a 'OOM' summary string to the command line
	Bool isOutOfMemory = Red::System::StringSearch( debugString, TXT( "OUT OF MEMORY" ) ) != nullptr ? true : false;

	// Write information to "crash info" file
	Char crashdataFilename[ MAX_PATH ];
	SNPrintF( crashdataFilename, ARRAY_COUNT( crashdataFilename ), TXT( "%" ) MACRO_TXT( RED_PRIs ) TXT( ".crashinfo" ), filename );
	
	FILE* crashdataFile;
	Internal::FileOpen( &crashdataFile, crashdataFilename, TXT( "w" ) );
	Internal::FilePrint( crashdataFile, debugString );
	Internal::FileClose( crashdataFile );

	// Add the crash info to the log file in silent mode (typically if we're running on the build servers)
#ifdef RED_LOGGING_ENABLED
	if( HasAssertFlag( AC_SilentCrashHook ) && Log::Manager::GetInstance().IsEnabled() )
	{
		Log::Manager::GetInstance().SetCrashModeActive( true );
		RED_LOG_ERROR( RED_LOG_CHANNEL( Exception ), debugString );
		Log::Manager::GetInstance().SetCrashModeActive( false );
	}
#endif

	// Time to invoke the crash handler
	Char crashReporterCommandLine[ 2048 ];
	Uint32 crCmdUsed = 

#ifdef RED_CRASH_HANDLER_CREATES_DUMP
	( !m_exceptionInfoBuffer )?
#endif

	SNPrintF
	(
		crashReporterCommandLine,
		ARRAY_COUNT( crashReporterCommandLine ),
			TXT( "TestTrackReporter.exe -c \"%" ) MACRO_TXT( RED_PRIs ) TXT( "\"" )
			TXT( " -d \"%" ) MACRO_TXT( RED_PRIs ) TXT( "\"" )
			TXT( " -l \"%" ) MACRO_TXT( RED_PRIs ) TXT( "\"" )
			TXT( " -exe \"%" ) MACRO_TXT( RED_PRIs ) TXT( "\"" )
			TXT( "%" ) MACRO_TXT( RED_PRIs ),
		crashdataFilename,
		dumpFileName,
		m_logFilepath,
		executableFileName,
		isOutOfMemory ? TXT( " -u \"[OOM]\"" ) : TXT( "" )
	)

#ifdef RED_CRASH_HANDLER_CREATES_DUMP
	:

	SNPrintF
	(
		crashReporterCommandLine,
		ARRAY_COUNT( crashReporterCommandLine ),
			TXT( "TestTrackReporter.exe -c \"%" ) MACRO_TXT( RED_PRIs ) TXT( "\"" )
			TXT( " -pid \"%u\" -tid \"%u\" -cb \"%" ) MACRO_TXT( RED_PRIs ) TXT( "\"" )
			TXT( " -l \"%" ) MACRO_TXT( RED_PRIs ) TXT( "\"" )
			TXT( " -exe \"%" ) MACRO_TXT( RED_PRIs ) TXT( "\"" )
			TXT( "%" ) MACRO_TXT( RED_PRIs ),
		crashdataFilename,
		GetCurrentProcessId(),
		GetCurrentThreadId(),
		RED_EXCEPTION_MAPFILE_NAME,
		m_logFilepath,
		executableFileName,
		isOutOfMemory ? TXT( " -u \"[OOM]\"" ) : TXT( "" )
	)

#endif
	;

	if( HasAssertFlag( AC_SilentCrashHook  ) )
	{
		RED_APPEND_ERROR_STRING( crashReporterCommandLine, ARRAY_COUNT( crashReporterCommandLine ), crCmdUsed, TXT( " -nowindow" ) );
	}

	STARTUPINFO startupInfo;
	MemorySet( &startupInfo, 0, sizeof( startupInfo ) );

	startupInfo.cb = sizeof( startupInfo );
	startupInfo.dwFlags = STARTF_USESHOWWINDOW;
	startupInfo.wShowWindow = SW_SHOW;

	PROCESS_INFORMATION processInfo;
	MemorySet( &processInfo, 0, sizeof( processInfo ) );
	
	BOOL success = CreateProcess
	(
		NULL,
		crashReporterCommandLine,
		NULL,
		NULL,
		FALSE,
		NORMAL_PRIORITY_CLASS,
		NULL,
		NULL,
		&startupInfo,
		&processInfo
	);

#ifdef RED_CRASH_HANDLER_CREATES_DUMP
	if( success && m_exceptionInfoBuffer )
	{
		// Need to keep the process alive so the crash handler can get our call stack
		WaitForSingleObject( processInfo.hProcess, INFINITE );
	}
#else
	RED_UNUSED( success );
#endif
}

void WindowsHandler::PureCallHandler()
{
	RED_WARNING_PUSH()
	RED_DISABLE_WARNING_MSC( 4127 ) // C4127: conditional expression is constant
	RED_FATAL_ASSERT( false, "Pure virtual function call inside constructor" );
	RED_WARNING_POP()
}

void WindowsHandler::SetLogFile( const Char* filepath, Log::File* )
{
	GetCurrentDirectory( ARRAY_COUNT( m_logFilepath ), m_logFilepath );
	StringConcatenate( m_logFilepath, TXT( "\\" ), ARRAY_COUNT( m_logFilepath ) );
	StringConcatenate( m_logFilepath, filepath, ARRAY_COUNT( m_logFilepath ) );
}

Handler* Handler::GetInstance()
{
	if( m_handlerInstance == nullptr )
	{
		static WindowsHandler instance;
		Handler::SetInternalInstance( &instance );
	}
	
	return m_handlerInstance;
}

} } } // namespace Red { namespace System { namespace Error {
