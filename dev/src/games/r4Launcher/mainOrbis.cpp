/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#ifdef RED_LOGGING_ENABLED
# include "../../common/core/feedback.h"
#endif
# include "../../common/core/loadingProfiler.h"

#include "gameApplicationOrbis.h"

#include "states.h"
#include "activateState.h"
#include "initializationState.h"
#include "gameRunningState.h"
#include "shutdownState.h"
#include "gameContrainedState.h"

// #ifdef RED_LOGGING_ENABLED
// class COrbisSplashScreen : public ISplashScreen
// {
// public:
// 	//! Update splash text
// 	virtual void UpdateProgress( const Char* info, ... ) override
// 	{
// 		va_list arglist;
// 		va_start( arglist, info );
// 		RED_LOG( SplashScreen, info, arglist );
// 		va_end(arglist);
// 	}
// };
// #endif // RED_LOGGING_ENABLED

#ifdef RED_LOGGING_ENABLED
	// Blech, opens as admin only access on /data using stdlib IO. So back to hostapp with the log.
	Red::System::Log::File fileLogger( TXT( "/hostapp/bin/r4launcher_orbis.log" ), true );
#endif

namespace MemoryDump
{
	static const Uint32 MAX_VIRTUAL_MEMORY_BLOCKS = 4096;

	typedef TStaticArray< SceKernelVirtualQueryInfo, MAX_VIRTUAL_MEMORY_BLOCKS > TVirtualQueryInfos;

	static void WalkVirtualMemory(	TVirtualQueryInfos*	virtualQueryInfos, const void* virtualAddress = 0 )
	{
		SceKernelVirtualQueryInfo queryInfo;
		int ret = sceKernelVirtualQuery( virtualAddress, SCE_KERNEL_VQ_FIND_NEXT, &queryInfo, sizeof(SceKernelVirtualQueryInfo) );
		if (0 == ret)
		{
			if ( !virtualQueryInfos->Full() )
			{
				virtualQueryInfos->PushBack( queryInfo );

				void* nextVirtualAddress = queryInfo.end;
				WalkVirtualMemory( virtualQueryInfos, nextVirtualAddress );
			}
		}
	}


	static void GetNumBytesMapped( TVirtualQueryInfos& rQueryInfos, Uint64* pNumBytesDirectMemoryMapped, Uint64* pNumBytesFlexibleMemoryMapped )
	{
		Uint64 totalDirectBytesMapped = 0;
		Uint64 totalFlexibleBytesMapped = 0;

		const Uint32 numItems = rQueryInfos.Size();
		for ( Uint32 i=0; i<numItems; ++i )
		{
			const Uint64 numBytesMappedInThisAllocation = ((Uint64)rQueryInfos[i].end - (Uint64)rQueryInfos[i].start);
			if ( rQueryInfos[i].isDirectMemory)
			{
				totalDirectBytesMapped += numBytesMappedInThisAllocation;
			}
			else if (rQueryInfos[i].isFlexibleMemory)
			{
				totalFlexibleBytesMapped += numBytesMappedInThisAllocation;
			}
		}

		*pNumBytesDirectMemoryMapped = totalDirectBytesMapped;
		*pNumBytesFlexibleMemoryMapped = totalFlexibleBytesMapped;
	}

	struct MemtypeToStr
	{
		SceKernelMemoryType m_type;
		const AnsiChar*		m_str;
	};

	const MemtypeToStr memoryTypeStrs[] =
	{
		{SCE_KERNEL_WC_GARLIC, "WC_GARLIC"},
		{SCE_KERNEL_WB_ONION, "WB_ONION"},
	};

	static const AnsiChar* FindMemTypeStr( SceKernelMemoryType memType )
	{
		for ( Uint32 i=0; i<ARRAY_COUNT_U32(memoryTypeStrs); ++i )
		{
			const MemtypeToStr& info = memoryTypeStrs[i];
			if ( info.m_type == memType )
			{
				return info.m_str;
			}
		}

		return "UNKNOWN";
	}

	static void PrintQueryInfos( const TVirtualQueryInfos& rQueryInfos )
	{
		fprintf( stderr, "Memory map:\n" );
		fprintf( stderr, "F D %-10s %s\t%s\t%-10s\t\t%-10s\t\t%-23s\t%s\t\t\t%s\n",
			"Size KiB",
			"Memory",
			"IsStack",
			"VA Start",
			"VA End",
			"Protection",
			"Memory Type",
			"Name");

		const Uint32 numInfos = rQueryInfos.Size();
		for ( Uint32 ii = 0; ii < numInfos; ++ii)
		{
			SceKernelVirtualQueryInfo info = rQueryInfos[ii];

			AnsiChar infoStr[33] = {0};
			Red::MemoryCopy( infoStr, info.name, 32 );

			const AnsiChar* stackStr = "     ";
			if (1 == info.isStack)
			{
				stackStr = "STACK";
			}

			AnsiChar protStr[24] = {0};
			if (0 != (info.protection & SCE_KERNEL_PROT_CPU_READ))
			{
				strcat(protStr, "CPU_R");
			}

			if (0 != (info.protection & SCE_KERNEL_PROT_CPU_WRITE))
			{
				if (strlen(protStr) > 0 )
				{
					strcat(protStr, "|");
				}
				strcat(protStr, "CPU_W");
			}

			if (0 != (info.protection & SCE_KERNEL_PROT_CPU_EXEC))
			{
				if (strlen(protStr) > 0 )
				{
					strcat(protStr, "|");
				}
				strcat(protStr, "CPU_E");
			}

			if (0 != (info.protection & SCE_KERNEL_PROT_GPU_READ))
			{
				if (strlen(protStr) > 0 )
				{
					strcat(protStr, "|");
				}
				strcat(protStr, "GPU_R");
			}

			if (0 != (info.protection & SCE_KERNEL_PROT_GPU_WRITE))
			{
				if (strlen(protStr) > 0 )
				{
					strcat(protStr, "|");
				}
				strcat(protStr, "GPU_W");
			}

			const AnsiChar * memtypeStr = FindMemTypeStr( (SceKernelMemoryType)info.memoryType );

			const Uint64 areaSizeInBytes = (Uint64)info.end - (Uint64)info.start;
			const Uint64 areaSizeInKiB = areaSizeInBytes / 1024;

			fprintf( stderr, "%d %d %-10lu\t%s\t0x%010lX\t0x%010lX\t%-23s\t%s\t%s\n", info.isFlexibleMemory, info.isDirectMemory, areaSizeInKiB, stackStr, (Uint64)info.start, (Uint64)info.end, protStr, memtypeStr, infoStr);
			fflush( stderr );
		}
	}

	static Bool DumpOrbisMemoryMap( Red::MemoryFramework::PoolLabel label )
	{
		TVirtualQueryInfos queryInfos;
		WalkVirtualMemory( &queryInfos );

		Uint64 totalDirectBytesMapped = 0;
		Uint64 totalFlexibleBytesMapped = 0;
		GetNumBytesMapped( queryInfos, &totalDirectBytesMapped, &totalFlexibleBytesMapped );

		PrintQueryInfos(queryInfos);

		const Uint64 directMemorySizeInBytes = sceKernelGetDirectMemorySize();
		const Uint64 directMemorySizeMiB = directMemorySizeInBytes >> 20;

		const Uint64 kFlexibleMemorySizeMiB = 448;
		const Float totalDirectMiBMapped = ((Float)totalDirectBytesMapped / (1024.0f * 1024.0f));
		const Float totalFlexibleMiBMapped = ((Float)totalFlexibleBytesMapped / (1024.0f * 1024.0f));

		fprintf( stderr, "Total Direct bytes mapped = %lu (%f MiB), Total direct memory: %lu MiB, Remaining: %f MiB\n",
			totalDirectBytesMapped, totalDirectMiBMapped,
			directMemorySizeMiB, ((float)directMemorySizeMiB - totalDirectMiBMapped) );


		fprintf( stderr, "Total Flexible bytes mapped = %lu (%f MiB), Total flexible memory: %lu MiB, Remaining: %f MiB\n", 
			totalFlexibleBytesMapped, totalFlexibleMiBMapped,
			kFlexibleMemorySizeMiB, ((float)kFlexibleMemorySizeMiB - totalFlexibleMiBMapped) );

		fflush( stderr );
		return false;
	}
} // MemoryDump

void BindNativeOOMHandlerForAllocator( Red::MemoryFramework::MemoryManager* pool )
{
	pool->RegisterOOMCallback( &MemoryDump::DumpOrbisMemoryMap );
}

int mainOrbis( const Char* commandLine )
{
	CGameApplicationOrbis theGame;

	CActivateState activateState( commandLine );
	theGame.RegisterState( GameActivate, &activateState );

	CInitializationSate initState;
	theGame.RegisterState( GameInitialize, &initState );

	CGameRunningState gameRunningState;
	theGame.RegisterState( GameRunning, &gameRunningState );

	CGameConstrainedState gameConstrainedState;
	theGame.RegisterState( GameConstrained, &gameConstrainedState );

	CShutdownState shutodwnState;
	theGame.RegisterState( GameShutdown, &shutodwnState );

	theGame.RequestState( GameActivate );

	return theGame.Run();
}

int main( int argc, char** argv )
{
	GLoadingProfiler.Start();

	// Disabled OOM handler by default since it causes stack overflow on some threads
	//BindNativeOOMHandlerForAllocator( &SRedMemory::GetInstance() );

	// Increase this if, for whatever reason, you need a longer one
	const int c_maxCommandLineLength = 2048;

	// Build a single string containing the command line arguments
	char commandLineConcat[ c_maxCommandLineLength ] = {'\0'};

	for( Int32 i = 1; i < argc; ++i )	// The first one appears to always by " ", so well ...
	{
		Red::System::StringConcatenate( commandLineConcat, argv[ i ], c_maxCommandLineLength );
		Red::System::StringConcatenate( commandLineConcat, " ", c_maxCommandLineLength );
	}

	wchar_t wCommandLineConcat[ c_maxCommandLineLength ] = {'\0'};
	Red::System::StringConvert( wCommandLineConcat, commandLineConcat, c_maxCommandLineLength );

//#ifdef RED_LOGGING_ENABLED
// 	COrbisSplashScreen splash;
// 	GSplash = &splash;
//#endif // RED_LOGGING_ENABLED
	return mainOrbis( wCommandLineConcat );
}
