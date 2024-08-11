/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_SYSTEM_MEMORY_STATS_H
#define _RED_MEMORY_SYSTEM_MEMORY_STATS_H
#pragma once

#include "redMemoryFrameworkTypes.h"

#ifdef RED_MEMORY_FRAMEWORK_PLATFORM_WINDOWS_API
	#include "redMemoryPageAllocatorWin.h"
#elif defined( RED_MEMORY_FRAMEWORK_PLATFORM_ORBIS_API )
	#include "redMemoryPageAllocatorOrbis.h"
#elif defined( RED_MEMORY_FRAMEWORK_PLATFORM_DURANGO_API )
	#include "redMemoryPageAllocatorDurango.h"
#endif

namespace Red { namespace MemoryFramework { 

struct SystemMemoryInfo
{
	Red::System::Uint64 m_totalPhysicalBytes;
	Red::System::Uint64 m_freePhysicalBytes;
	Red::System::Uint64 m_totalVirtualBytes;
	Red::System::Uint64 m_freeVirtualBytes;
	OSAPI::ExtendedMemoryStats m_platformStats;		// Platform-specific data
};

///////////////////////////////////////////////////////////
// Use this class to get statistics about system memory available
// All functions should be static, no need to instantiate this as will be just wrapping system calls
class SystemMemoryStats
{
public:
	// Request latest memory stats from the system
	static SystemMemoryInfo RequestStatistics();

	// Kick out a warning to the debugger if available memory < the amount passed in
	// Returns true if a warning was output
	static Red::System::Bool WarnOnLowMemory( Red::System::Uint64 lowMemoryAmount );

	// Get largest system allocation we can handle
	static Red::System::MemSize LargestAllocationPossible();

	// Output total memory used by the process
	static void LogTotalProcessMemory();
};

} }

#endif