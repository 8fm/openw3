/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "engineTime.h"
#include "loadingProfiler.h"
#include "fileSystemProfiler.h"
#include "fileSystemProfilerWrapper.h"
#include "version.h"

///----

CLoadingProfiler	GLoadingProfiler;

///----

// IO Stats - hacked in :(

#ifdef RED_ARCH_X64
Red::Threads::AtomicOps::TAtomic64		GSystemIONumBytesRead = 0;
Red::Threads::AtomicOps::TAtomic64		GSystemIONumBytesWritten = 0;
Red::Threads::AtomicOps::TAtomic64		GSystemIONumOps = 0;
Red::Threads::AtomicOps::TAtomic64		GSystemIONumFilesOpened = 0;
#endif

// GPU Memory Stats - hacked in :(
typedef Uint64 (*TGetGPUMemoryFunc)();
TGetGPUMemoryFunc						GSystemGPUMemoryStatFunc = nullptr;

CLoadingProfiler::CLoadingProfiler()
	: m_counter(0)
{
}

void CLoadingProfiler::Start()
{
	// make sure the timer is initialized
	EngineTime::Init();

	// rewind
	m_counter = 0;
	m_start = (Double) EngineTime::GetNow();

	// reset IO counters
#ifdef RED_ARCH_X64
	Red::Threads::AtomicOps::Exchange64( &GSystemIONumBytesRead, 0 );
	Red::Threads::AtomicOps::Exchange64( &GSystemIONumBytesWritten, 0 );
	Red::Threads::AtomicOps::Exchange64( &GSystemIONumOps, 0 );
	Red::Threads::AtomicOps::Exchange64( &GSystemIONumFilesOpened, 0 );
#endif

	// add the start capture
	FinishStage( TXT("<base>") );
}

#if !defined( RED_FINAL_BUILD ) || defined( RED_PROFILE_BUILD )
	#undef LOG_CORE

# ifdef RED_PLATFORM_ORBIS
	#define LOG_CORE(...) { fwprintf( stdout, __VA_ARGS__ ); fwprintf( stdout, L"\n" ); }
# else
	#define LOG_CORE(...) { Char tempArray[1024]; swprintf( tempArray, 1024, __VA_ARGS__ ); OutputDebugString(tempArray); OutputDebugString(TXT("\n")); }
# endif
#endif

void CLoadingProfiler::FinishLoading( const Char* title )
{
	// push dummy end stage
	FinishStage( TXT("END") );

	// push to native profiler
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileEndLoading();
#endif

#ifdef RED_ARCH_X64
	// dump the stats table
	LOG_CORE( TXT("--------------------------------------------------------------------------------------------------"));
	LOG_CORE( TXT("-- LOADING STATS DUMP for '%ls'"), title );
	LOG_CORE( TXT("-- Platform: %ls"), ANSI_TO_UNICODE( RED_EXPAND_AND_STRINGIFY( PROJECT_PLATFORM ) ) );
	LOG_CORE( TXT("-- Configuration: %ls"), ANSI_TO_UNICODE( RED_EXPAND_AND_STRINGIFY( PROJECT_CONFIGURATION ) ) );
	LOG_CORE( TXT("-- Version: %ls"), ANSI_TO_UNICODE( RED_EXPAND_AND_STRINGIFY( APP_VERSION_BUILD ) ) );
	LOG_CORE( TXT("--------------------------------------------------------------------------------------------------------------------------"));
	LOG_CORE( TXT("| Stage                   |  Time [ms] | CPU Mem [MB] | GPU Mem [MB] | IORead [MB] | IOWrite [MB] |  IO KOps  | IO Files |"));
	LOG_CORE( TXT("--------------------------------------------------------------------------------------------------------------------------"));

	// base capture
	CaptureInfo start;
	Red::MemoryZero( &start, sizeof(start) );
	start.m_time = m_start;

	// print base capture
	const Float oneMB = 1024.0f * 1024.0f;
	CaptureInfo base = start;
	const Uint32 count = Red::Threads::AtomicOps::Exchange32( &m_counter, 0 );
	for ( Uint32 i=0; i<count; ++i )
	{
		const CaptureInfo& info = m_captures[i];
		LOG_CORE( TXT("| %23ls | %10.3f | %12.3f | %12.3f | %11.3f | %12.3f | %9.2f | %8d |"),
			info.m_name,
			(info.m_time - base.m_time) * 1000.0f,
			((Int64)info.m_memoryCPU - (Int64)base.m_memoryCPU) / oneMB,
			((Int64)info.m_memoryGPU - (Int64)base.m_memoryGPU) / oneMB,
			((Int64)info.m_bytesRead - (Int64)base.m_bytesRead) / oneMB,
			((Int64)info.m_bytesWritten - (Int64)base.m_bytesWritten) / oneMB,
			((Int64)info.m_fileOps - (Int64)base.m_fileOps) / 1000.0f,
			((Int64)info.m_filesOpened - (Int64)base.m_filesOpened) );

		base = info;
	}

	LOG_CORE( TXT("--------------------------------------------------------------------------------------------------------------------------"));

	{
		const CaptureInfo& start = m_captures[ 0 ];
		const CaptureInfo& info = m_captures[ count-1 ];

		LOG_CORE( TXT("| %23ls | %10.3f | %12.3f | %12.3f | %11.3f | %12.3f | %9.2f | %8d |"),
			TXT("TOTAL"),
			(info.m_time - start.m_time) * 1000.0f,
			((Int64)info.m_memoryCPU - (Int64)start.m_memoryCPU) / oneMB,
			((Int64)info.m_memoryGPU - (Int64)start.m_memoryGPU) / oneMB,
			((Int64)info.m_bytesRead - (Int64)start.m_bytesRead) / oneMB,
			((Int64)info.m_bytesWritten - (Int64)start.m_bytesWritten) / oneMB,
			((Int64)info.m_fileOps - (Int64)start.m_fileOps) / 1000.0f,
			((Int64)info.m_filesOpened - (Int64)start.m_filesOpened) );
	}

	LOG_CORE( TXT("--------------------------------------------------------------------------------------------------------------------------"));
#endif

	// flush IO profiler
	GFileManagerProfiler.Flush();
}

void CLoadingProfiler::FinishStage( const Char* name )
{
	// push to native profiler
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileFinishLoadingPhase( UNICODE_TO_ANSI( name ) );
#endif

#ifdef RED_ARCH_X64
	// disable reporting from another threads for now
	if ( !::SIsMainThread() )
		return;

	// allocate stage index
	const Uint32 index = Red::Threads::AtomicOps::Increment32( &m_counter );
	if ( index >= MAX_CAPTURES )
		return;

	// setup capture
	CaptureInfo& info = m_captures[ index-1 ];
	info.m_name = name;
	info.m_bytesRead = GSystemIONumBytesRead;
	info.m_bytesWritten = GSystemIONumBytesWritten;
	info.m_fileOps = GSystemIONumOps;
	info.m_filesOpened = GSystemIONumFilesOpened;
	info.m_time = EngineTime::GetNow();
	info.m_memoryCPU = Memory::GetTotalBytesAllocated();
	info.m_memoryGPU = GSystemGPUMemoryStatFunc ? GSystemGPUMemoryStatFunc() : 0; 
#endif
}
