#include "redMemorySystemMemoryStats.h"
#include "redMemoryLog.h"

namespace Red { namespace MemoryFramework { 

///////////////////////////////////////////////////////////////////
// RequestStatistics
//	
SystemMemoryInfo SystemMemoryStats::RequestStatistics()
{
	SystemMemoryInfo results;
	MEMORYSTATUSEX memoryStatusStruct;

	Red::System::MemoryZero( &results, sizeof( results ) );
	Red::System::MemoryZero( &memoryStatusStruct, sizeof( memoryStatusStruct ) );
	memoryStatusStruct.dwLength = sizeof( memoryStatusStruct );

	if( GlobalMemoryStatusEx( &memoryStatusStruct ) )
	{
		results.m_totalVirtualBytes = memoryStatusStruct.ullTotalVirtual;
		results.m_freeVirtualBytes = memoryStatusStruct.ullAvailVirtual;
		results.m_totalPhysicalBytes =  memoryStatusStruct.ullTotalPhys;
		results.m_freePhysicalBytes = memoryStatusStruct.ullAvailPhys;
	}

	return results;
}

///////////////////////////////////////////////////////////////////
// LargestAllocationPossible
//	Get largest system allocation we can handle by walking the process
//  virtual memory
Red::System::MemSize SystemMemoryStats::LargestAllocationPossible()
{
	// First, we find the address space of this process
	SYSTEM_INFO systemInfo;
	ZeroMemory(&systemInfo, sizeof(systemInfo));
	GetSystemInfo( &systemInfo );	
	Red::System::MemUint baseAddress = reinterpret_cast< Red::System::MemUint >( systemInfo.lpMinimumApplicationAddress );
	Red::System::MemUint highAddress = reinterpret_cast< Red::System::MemUint >( systemInfo.lpMaximumApplicationAddress );
	Red::System::MemSize largestFree = 0;

	// Now walk every area of virtual memory, keeping track of the largest free block
	while( baseAddress < highAddress )
	{
		MEMORY_BASIC_INFORMATION memoryBlockInfo;
		if( VirtualQuery( reinterpret_cast< void* >( baseAddress ), &memoryBlockInfo, sizeof( memoryBlockInfo ) ) )
		{
			if( memoryBlockInfo.State & MEM_FREE )
			{
				largestFree = Red::Math::NumericalUtils::Max( largestFree, static_cast< Red::System::MemSize >( memoryBlockInfo.RegionSize ) );
			}

			baseAddress += memoryBlockInfo.RegionSize;
		}
		else
		{
			baseAddress += systemInfo.dwPageSize;	// No info for this address, go to the next page of virtual memory
		}
	}

	return largestFree;
}

///////////////////////////////////////////////////////////////////
// LogTotalProcessMemory
//	Output total memory used by the process
void SystemMemoryStats::LogTotalProcessMemory()
{
#ifdef RED_LOGGING_ENABLED
	SystemMemoryInfo systemStats = RequestStatistics();
	Red::System::MemSize memoryUsed = static_cast< Red::System::MemSize >( systemStats.m_totalVirtualBytes - systemStats.m_freeVirtualBytes );
	RED_MEMORY_LOG( TXT( "Total Virtual Memory Used by Process: %" ) RED_PRIWsize_t TXT( "Kb" ), memoryUsed / 1024 );
	RED_UNUSED( memoryUsed );
#endif
}

///////////////////////////////////////////////////////////////////
// WarnOnLowMemory
//	Kick out a warning to the debugger if available memory < the amount passed in
//	Returns true if a warning was output
Red::System::Bool SystemMemoryStats::WarnOnLowMemory( Red::System::Uint64 lowMemoryAmount )
{
	const Red::System::Uint64 c_oneMb = 1024 * 1024;
	Red::System::AnsiChar debugOutput[256] = {'\0'};
	Red::System::Bool displayWarning = false;

	SystemMemoryInfo systemMemory = RequestStatistics();
	if( systemMemory.m_totalPhysicalBytes < systemMemory.m_totalVirtualBytes )
	{
		// System RAM is less than virtual address space. Either we are on a 64 bit machine (and the address space is HUGE),
		// or we are on a system with a small amount of physical memory ( <4 gig ). Either way, spit out a warning as running out of physical
		// is bad enough anyway
		if( systemMemory.m_freePhysicalBytes < lowMemoryAmount )
		{
			Red::System::AnsiChar format[] = "!! WARNING: System physical memory available < %" RED_PRIu64 "MB (%" RED_PRIu64 "MB Remaining of %" RED_PRIu64 "MB Total) Largest available block = %" RED_PRIsize_t "!!\n";
			Red::System::SNPrintF( debugOutput, ARRAY_COUNT( debugOutput ), format,	lowMemoryAmount / c_oneMb, 
																					systemMemory.m_freePhysicalBytes / c_oneMb, 
																					systemMemory.m_totalPhysicalBytes / c_oneMb, 
																					LargestAllocationPossible() );
			displayWarning = true;
		}
	}
	else
	{
		// Virtual address space is < physical ram. We are most likely running on a 32 bit machine (or the machine has a massive amount of ram)
		// If we run out of virtual address space, crashes will most likely occur
		if( systemMemory.m_freeVirtualBytes < lowMemoryAmount )
		{
			Red::System::AnsiChar format[] = "!! WARNING: Virtual memory available < %" RED_PRIu64 "MB (%" RED_PRIu64 "MB Remaining of %"RED_PRIu64 "MB Total) Largest available block = %" RED_PRIsize_t "!!\n";
			Red::System::SNPrintF( debugOutput, ARRAY_COUNT( debugOutput ), format, lowMemoryAmount / c_oneMb, 
																					systemMemory.m_freeVirtualBytes / c_oneMb, 
																					systemMemory.m_totalVirtualBytes / c_oneMb,
																					LargestAllocationPossible() );
			displayWarning = true;
		}
	}

	if( displayWarning )
	{
		RED_MEMORY_LOG( TXT( "%" ) RED_PRIWas, debugOutput );

		// Make sure it is output, even in builds with logging disabled
		OutputDebugStringA( debugOutput );
	}

	return displayWarning;
}

} }