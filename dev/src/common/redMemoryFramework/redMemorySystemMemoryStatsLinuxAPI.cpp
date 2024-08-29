#include "redMemorySystemMemoryStats.h"
#include "redMemoryLog.h"

namespace Red { namespace MemoryFramework {

///////////////////////////////////////////////////////////////////
// RequestStatistics
//
SystemMemoryInfo SystemMemoryStats::RequestStatistics()
{
	SystemMemoryInfo results;
	Red::System::MemoryZero( &results, sizeof( results ) );

	RED_LOG_ERROR(SystemMemoryStats, TXT("FIX_LINUX SystemMemoryStats::RequestStatistics"));

	return results;
}

///////////////////////////////////////////////////////////////////
// LargestAllocationPossible
//
Red::System::MemSize SystemMemoryStats::LargestAllocationPossible()
{
	RED_LOG_ERROR( SystemMemoryStats, TXT("FIX_LINUX SystemMemoryStats::LargestAllocationPossible"));
	return 0;
}

///////////////////////////////////////////////////////////////////
// LogTotalProcessMemory
//
void SystemMemoryStats::LogTotalProcessMemory()
{
	RED_LOG_ERROR(SystemMemoryStats, TXT("FIX_LINUX SystemMemoryStats::LogTotalProcessMemory"));
}

///////////////////////////////////////////////////////////////////
// WarnOnLowMemory
//
Red::System::Bool SystemMemoryStats::WarnOnLowMemory( Red::System::Uint64 lowMemoryAmount )
{
	RED_UNUSED( lowMemoryAmount );
	RED_LOG_ERROR(SystemMemoryStats, TXT("FIX_LINUX SystemMemoryStats::WarnOnLowMemory"));
	return false;
}

} }