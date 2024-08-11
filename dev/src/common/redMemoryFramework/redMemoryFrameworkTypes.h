/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FRAMEWORK_TYPES_H
#define _RED_MEMORY_FRAMEWORK_TYPES_H
#pragma once

#include "../redSystem/os.h"
#include "../redSystem/utility.h"
#include "../redSystem/bitUtils.h"
#include "redMemoryFrameworkPlatform.h"
#include "redMemoryMetricsNamePool.h"

//////////////////////////////////////////////////////////////////////////
// This shouldn't really be here, but its the only truly common header in the memory system
#ifndef RED_FINAL_BUILD
	#define ENABLE_MEMORY_MANAGER_OVERFLOW_POOL			// Enable overflow pool in dev builds
	//#define ENABLE_EXTENDED_XMEMALLOC_TRACKING	 	// Enables extended XMemAlloc stats. Its too slow to have on all the time
#endif

#if !defined( RED_FINAL_BUILD ) || defined( LOG_IN_FINAL )
	#define ENABLE_EXTENDED_MEMORY_METRICS				// Enables detailed metrics storage	
#endif

//////////////////////////////////////////////////////////////////////////
// Memory Framework Pragma-disable for compilers that support C++11 correctly
//
#ifdef RED_COMPILER_MSC
	#define RED_MEMORY_PUSH_WARNING_DISABLE(warningNum)		\
		__pragma( warning( push ) )							\
		__pragma( warning( disable : warningNum ) )			

	#define RED_MEMORY_POP_WARNING_DISABLE					\
		__pragma( warning( pop ) )
#else
	#define RED_MEMORY_PUSH_WARNING_DISABLE(warningNum)
	#define RED_MEMORY_POP_WARNING_DISABLE
#endif

//////////////////////////////////////////////////////////////////////////
// Memory Framework Types / Enums
namespace Red { namespace MemoryFramework {

// Maximum number of pools supported by the Memory Manager
const Red::System::Uint32	k_MaximumPools = 128;

// Maximum number of memory classes supported by the Memory Manager (only used for metrics)
const Red::System::Uint32	k_MaximumMemoryClasses = 256;

// Maximum number of memory class metric groups we want to handle
const Red::System::Uint32	k_MaximumMemoryClassGroups = 24;

///////////////////////////////////////////////////////////////
// Memory framework Typedefs
typedef MetricsNamePool<k_MaximumPools, 64> PoolNamesList;
typedef MetricsNamePool<k_MaximumMemoryClasses, 64> MemoryClassNamesList;

typedef Red::System::Uint32		PoolLabel;			// Used to identify a memory pool
typedef Red::System::Uint32		MemoryClass;		// Used to identify a type of allocation (used for metrics)
typedef Red::System::Int64		AllocationID;		// Used to identify allocations when using the memory leak tracker

///////////////////////////////////////////////////////////////
// Enums

// VS2010 kicks out a warning about strongly typed enums, even though it does implement them
// Disable the warning for this enum (ToDo! Remove this when we move to VS2012)
RED_MEMORY_PUSH_WARNING_DISABLE( 4480 )
enum EAllocatorFlags : Red::System::Uint32		// Flags that can be attached to any pool
{
	Allocator_StaticSize = FLAG( 0 ),			// The pool is a static size
	Allocator_NoBreakOnOOM = FLAG( 1 ),			// Don't assert / break / crash on OOM, just report

	// Platform-specific pool flags
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 )
	Allocator_NoPhysicalReserved = FLAG( 2 ),	// Do not explicitly reserve physical memory for this allocator (when used with virtual memory)
#elif defined RED_PLATFORM_ORBIS
	Allocator_DirectMemory = FLAG( 2 ),			// Use direct (i.e. physical) memory - 4.5gb max
	Allocator_FlexibleMemory = FLAG( 3 ),		// Use flexible memory - 512mb max, 448mb max for gpu access
	Allocator_UseOnionBus = FLAG( 4 ),			// Read / write using the Onion bus (uses CPU L2 cache, gpu will snoop CPU caches), write-back
	Allocator_UseGarlicBus = FLAG( 5 ),			// Read / write using the Garlic bus (uses GPU cache), CPU reads are uncached, writes are write-combined, 
	Allocator_AccessCpuRead = FLAG( 6 ),		// CPU Read access
	Allocator_AccessCpuWrite = FLAG( 7 ),		// CPU Write access
	Allocator_AccessGpuRead = FLAG( 8 ),		// GPU Read access
	Allocator_AccessGpuWrite = FLAG( 9 ),		// GPU Write access
	Allocator_TextureMemory = FLAG( 10 ),		// 64k align

	// Combined flags for convenience
	Allocator_AccessCpuReadWrite = Allocator_AccessCpuRead | Allocator_AccessCpuWrite,
	Allocator_AccessGpuReadWrite = Allocator_AccessGpuRead | Allocator_AccessGpuWrite,

#elif defined RED_PLATFORM_DURANGO
	////////////////////////////////////////////////////////////////////////////////////////
	// The below flags are for our general allocators. 
	// For graphics allocations, use D3DAllocateGraphicsMemory directly (our XMemAlloc hooks will pick up the metrics) 

	// Physical / GPU Memory must be used seperately. GPU implies CPU access as well
	Allocator_AccessCpu = FLAG( 2 ),			// CPU-only visibility
	Allocator_AccessCpuGpu = FLAG( 3 ),			// GPU and CPU visible memory

	// These flags can be combined with the ones above
	Allocator_Cached = FLAG( 4 ),				// Read / write is cached
	Allocator_WriteCombine = FLAG( 5 ),			// Writes are uncached / write combined

	// These flags are specific to GPU memory
	Allocator_GpuReadOnly = FLAG( 6 ),			// Read-only GPU memory. Must be write-combined
	Allocator_GpuNonCoherent = FLAG( 7 ),		// GPU access does NOT snoop CPU caches. 				
	Allocator_CommandBuffer = FLAG( 8 ),		// Memory is used as a command buffer. Must be write-combined or cached

	// These flags are specific to CPU memory
	Allocator_Uncached = FLAG( 9 ),			// Uncached read/write (CPU only)

	// Debug memory allocator (available when ProfilingMode is enabled, and DebugMemoryMode is either PIX_Tool or PGI_Tool. Use this flag alone - no masking with others)
	Allocator_UseDebugMemory = FLAG( 10 ),

	// Use 64k page size and alignment (useful for graphics stuff)
	Allocator_Use64kPages = FLAG( 11 ),

	// Reserving bits for pool / allocator IDs. Useful for debugging in PIX
	Allocator_IdReservedStart = FLAG( 12 ),	
	Allocator_IdReservedEnd = FLAG( 17 ),	
#endif
};
RED_MEMORY_POP_WARNING_DISABLE

#ifdef RED_PLATFORM_DURANGO
	// Bits reserved for allocator IDs (starting at Allocator_IdReserved0)
	const Uint32 c_ReservedAllocIdStartBits = Red::System::BitUtils::Log2( (Uint32)Allocator_IdReservedStart );
	const Uint32 c_ReservedAllocIdEndBits = Red::System::BitUtils::Log2( (Uint32)Allocator_IdReservedEnd );
	const Uint32 c_ReservedAllocIdBits =  c_ReservedAllocIdEndBits - c_ReservedAllocIdStartBits;
	const Uint32 c_ReservedAllocIdMask = ( ( 1 << ( c_ReservedAllocIdBits + 1 ) ) - 1 ) << c_ReservedAllocIdStartBits;

	// Use this macro to pass an allocator id to XMemAlloc. 0 / 1 are reserved for static / overflow
	#define RED_MEMORY_DURANGO_ALLOCID( id )		( ( id + 2 ) << Red::MemoryFramework::c_ReservedAllocIdStartBits ) & Red::MemoryFramework::c_ReservedAllocIdMask
	#define RED_MEMORY_DURANGO_GET_ALLOCID( flags )	( flags & Red::MemoryFramework::c_ReservedAllocIdMask ) >> Red::MemoryFramework::c_ReservedAllocIdStartBits

	#define RED_MEMORY_DURANGO_STATIC_ALLOCID		0
	#define RED_MEMORY_DURANGO_OVERFLOW_ALLOCID		Allocator_IdReservedStart
#endif

// Allocator initialisation result
enum EAllocatorInitResults
{
	AllocInit_OK,
	AllocInit_OutOfMemory,
	AllocInit_BadParameters
};

// Allocator memory free result
enum EAllocatorFreeResults
{
	Free_OK,
	Free_NotOwned,		// Returned if the pointer passed in was not managed by the current allocator
	Free_AlreadyFree	// The address is already free. will cause a message in the crash reports
};

} }

#endif
