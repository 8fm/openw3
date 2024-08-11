#include "redMemorySystemMemoryStats.h"
#include "redMemoryLog.h"
#include "redMemoryPageAllocator.h"

#include <kernel.h>

namespace Red { namespace MemoryFramework { 

///////////////////////////////////////////////////////////////////
// System-wide memory constants
//

// As of SDK 1.000.091, 448mb of flexible memory can be used. Flexible memory is used for executable, data section, stack, etc.
// Since polling entire address space to get real memory available is too slow, we can only approximate it
const Red::System::MemSize c_approxAdditionalFlexibleMemoryUsed = ( 32 * 1024 * 1024 ) + ( 64 * 1024 * 1024 );		// .exe + stack + statics
const Red::System::MemSize c_maximumFlexibleMemoryAvailable = ( 448ul * 1024ul * 1024ul );

///////////////////////////////////////////////////////////////////
// RequestStatistics
//	Physical = direct memory used
//	Virtual = flexible memory used (still technically physical)
SystemMemoryInfo SystemMemoryStats::RequestStatistics()
{
	SystemMemoryInfo results;
	Red::System::MemoryZero( &results, sizeof( results ) );
	OSAPI::PageAllocatorImpl& platformImpl = PageAllocator::GetInstance().GetPlatformImplementation();
	size_t totalDirectAvailable = sceKernelGetDirectMemorySize();

	// Use the system page allocator to provide these metrics (everything should be going through our memory stuff on PS4)
	results.m_totalPhysicalBytes = totalDirectAvailable + c_maximumFlexibleMemoryAvailable;
	results.m_freePhysicalBytes = results.m_totalPhysicalBytes - ( platformImpl.GetDirectMemoryAllocated() + platformImpl.GetFlexibleMemoryAllocated() );
	results.m_totalVirtualBytes = 0;
	results.m_freeVirtualBytes = 0;

	results.m_platformStats.m_directMemoryUsed = platformImpl.GetDirectMemoryAllocated();
	results.m_platformStats.m_directMemoryFree = totalDirectAvailable - platformImpl.GetDirectMemoryAllocated();
	results.m_platformStats.m_approxFlexibleMemoryUsed = platformImpl.GetFlexibleMemoryAllocated() + c_approxAdditionalFlexibleMemoryUsed;
	results.m_platformStats.m_approxFlexibleMemoryFree = c_maximumFlexibleMemoryAvailable - ( platformImpl.GetFlexibleMemoryAllocated() + c_approxAdditionalFlexibleMemoryUsed );

	return results;
}

///////////////////////////////////////////////////////////////////
// LargestAllocationPossible
//	Since pools will not be resizing on Orbis, there is no need to find the largest
//	physical block. Instead, we just return the free amount of direct memory
Red::System::MemSize SystemMemoryStats::LargestAllocationPossible()
{
	size_t totalDirectAvailable = sceKernelGetDirectMemorySize();
	return totalDirectAvailable - PageAllocator::GetInstance().GetPlatformImplementation().GetDirectMemoryAllocated();
}

///////////////////////////////////////////////////////////////////
// LogTotalProcessMemory
//	Output total memory used by the process
void SystemMemoryStats::LogTotalProcessMemory()
{
#ifdef RED_LOGGING_ENABLED
	SystemMemoryInfo systemStats = RequestStatistics();
	Red::System::MemSize directUsed = static_cast< Red::System::MemSize >( systemStats.m_totalPhysicalBytes - systemStats.m_freePhysicalBytes );
	Red::System::MemSize flexibleUsed = static_cast< Red::System::MemSize >( systemStats.m_totalVirtualBytes - systemStats.m_freeVirtualBytes );
	RED_MEMORY_LOG( TXT( "Total Virtual Memory Used by Process: %" ) RED_PRIWsize_t TXT( "Kb direct memory, %" ) RED_PRIWsize_t TXT( "Kb flexible memory." ), directUsed / 1024, flexibleUsed / 1024 );
#endif
}

///////////////////////////////////////////////////////////////////
// WarnOnLowMemory
//	Kick out a warning to the debugger if available memory < the amount passed in
//	Returns true if a warning was output
//	Realistically this won't be needed on Orbis since we will be reserving almost all system memory anyway
Red::System::Bool SystemMemoryStats::WarnOnLowMemory( Red::System::Uint64 lowMemoryAmount )
{
	RED_UNUSED( lowMemoryAmount );
	return false;
}

} }