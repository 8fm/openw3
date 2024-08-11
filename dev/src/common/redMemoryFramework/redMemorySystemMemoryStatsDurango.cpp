#include "redMemorySystemMemoryStats.h"
#include "redMemoryPageAllocator.h"
#include "redMemoryPageAllocatorDurango.h"
#include "redMemoryLog.h"

namespace Red { namespace MemoryFramework { 

///////////////////////////////////////////////////////////////////
// RequestStatistics
//
SystemMemoryInfo SystemMemoryStats::RequestStatistics()
{
	SystemMemoryInfo results;
	Red::System::MemoryZero( &results, sizeof( results ) );

	// GlobalMemoryStatus allows us to track stuff outside XMemAlloc
	TITLEMEMORYSTATUS memStatus;
	memStatus.dwLength = sizeof( memStatus );
	::TitleMemoryStatus( &memStatus );

	results.m_totalPhysicalBytes = results.m_totalVirtualBytes = memStatus.ullTotalMem + memStatus.ullAvailMem;
	results.m_freePhysicalBytes = results.m_freeVirtualBytes = memStatus.ullAvailMem;
	results.m_platformStats.m_legacyUsed = memStatus.ullLegacyUsed;
	results.m_platformStats.m_legacyAvailable = memStatus.ullLegacyAvail;
	results.m_platformStats.m_legacyPeak = memStatus.ullLegacyPeak;
	results.m_platformStats.m_titleUsed = memStatus.ullTitleUsed;
	results.m_platformStats.m_titleAvailable = memStatus.ullTitleAvail;

	const DurangoAPI::XMemAllocStatistics& allocStats = PageAllocator::GetInstance().GetPlatformImplementation().GetXMemAllocStats();
	for( Uint32 allocId=0; allocId < DurangoAPI::XMemAllocStatistics::c_maximumAllocIds; ++allocId )
	{
		switch( allocId )
		{
		case 0:
			results.m_platformStats.m_staticsAllocated += allocStats.m_stats[ allocId ].m_bytesAllocated;
			break;
		case 1:
			results.m_platformStats.m_overflowAllocated += allocStats.m_stats[ allocId ].m_bytesAllocated;
			break;
		case eXALLOCAllocatorId_D3D:
			results.m_platformStats.m_d3dAllocated += allocStats.m_stats[ allocId ].m_bytesAllocated;
			break;
		case eXALLOCAllocatorId_Audio:
			results.m_platformStats.m_audioAllocated += allocStats.m_stats[ allocId ].m_bytesAllocated;
			break;
		default:
			if( allocId < eXALLOCAllocatorId_GameMax )
			{
				results.m_platformStats.m_enginePoolsAllocated += allocStats.m_stats[ allocId ].m_bytesAllocated;
			}
			else if( allocId < eXALLOCAllocatorId_PlatformReservedMax )
			{
				results.m_platformStats.m_platformAllocated += allocStats.m_stats[ allocId ].m_bytesAllocated;
			}
			else
			{
				results.m_platformStats.m_middlewareAllocated += allocStats.m_stats[ allocId ].m_bytesAllocated;
			}
			break;
		}
	}

	return results;
}

///////////////////////////////////////////////////////////////////
// LargestAllocationPossible
//
Red::System::MemSize SystemMemoryStats::LargestAllocationPossible()
{
	return 0;
}

///////////////////////////////////////////////////////////////////
// LogTotalProcessMemory
//	Output total memory used by the process
void SystemMemoryStats::LogTotalProcessMemory()
{
#ifdef RED_LOGGING_ENABLED
	const Uint32 c_allocColumnWidth = 20;

	const DurangoAPI::XMemAllocStatistics& allocStats = PageAllocator::GetInstance().GetPlatformImplementation().GetXMemAllocStats();
	SystemMemoryInfo systemStats = RequestStatistics();
	RED_MEMORY_LOG( TXT( "Total Memory Allocated via XMemAlloc: %" ) RED_PRIWsize_t TXT( "Kb" ), ( systemStats.m_totalPhysicalBytes - systemStats.m_freePhysicalBytes ) / 1024 );

	if( systemStats.m_totalPhysicalBytes > 0 )
	{
		RED_MEMORY_LOG( TXT( "\t%-*s| Kb Allocated" ), c_allocColumnWidth, TXT( "XMemAlloc ID" ) );
		RED_MEMORY_LOG( TXT( "\t---------------------------------------" ) );
	}

	Char *allocatorNameBuffer = nullptr;
	for( Uint32 allocId=0; allocId < DurangoAPI::XMemAllocStatistics::c_maximumAllocIds; ++allocId )
	{
		if( allocStats.m_stats[ allocId ].m_bytesAllocated > 0 )
		{
			switch( allocId )
			{
			case 0:
				allocatorNameBuffer = TXT( "Statics / Globals" );
				break;
			case 1:
				allocatorNameBuffer = TXT( "Overflow" );
				break;
			case eXALLOCAllocatorId_D3D:
				allocatorNameBuffer = TXT( "D3D" );
				break;
			case eXALLOCAllocatorId_Audio:
				allocatorNameBuffer = TXT( "Audio" );
				break;
			default:
				if( allocId < eXALLOCAllocatorId_GameMax )
				{
					allocatorNameBuffer =  TXT( "Engine Pool" );
				}
				else if( allocId < eXALLOCAllocatorId_PlatformReservedMax )
				{
					allocatorNameBuffer =  TXT( "OS / Platform" );
				}
				else
				{
					allocatorNameBuffer =  TXT( "Middleware" );
				}
				break;
			}
			RED_MEMORY_LOG( TXT( "\t%2d: %-*s| %" ) RED_PRIWsize_t, allocId, c_allocColumnWidth - 4, allocatorNameBuffer, allocStats.m_stats[ allocId ].m_bytesAllocated / 1024 );
		}
	}
#endif
}

///////////////////////////////////////////////////////////////////
// WarnOnLowMemory
//	Kick out a warning to the debugger if available memory < the amount passed in
//	Returns true if a warning was output
Red::System::Bool SystemMemoryStats::WarnOnLowMemory( Red::System::Uint64 lowMemoryAmount )
{
#ifdef RED_LOGGING_ENABLED
	SystemMemoryInfo systemStats = RequestStatistics();

	if( systemStats.m_freePhysicalBytes < lowMemoryAmount )
	{
		RED_MEMORY_LOG( TXT( "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! LOW MEMORY DETECTED !!!!!!!!!!!!!!!!!!!!!!!!!!!!" ) );
		RED_MEMORY_LOG( TXT( "\tWarning! %" ) RED_PRIWsize_t TXT( "Kb available" ) , systemStats.m_freePhysicalBytes / 1024 );
		LogTotalProcessMemory();
		RED_MEMORY_LOG( TXT( "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! LOW MEMORY DETECTED !!!!!!!!!!!!!!!!!!!!!!!!!!!!" ) );
	}
#endif

	return false;
}

} }