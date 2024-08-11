/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "redMemoryPageAllocatorDurango.h"
#include "redMemoryFrameworkTypes.h"
#include "../redSystem/error.h"
#include <intrin.h>
#pragma	intrinsic( __popcnt )

#ifdef ENABLE_MEMORY_MANAGER_OVERFLOW_POOL
	// Sigh. This is required in order to use DebugMemGetRegion
	#include <pix.h>
	#pragma comment( lib, "pixEvt.lib" )
#endif

namespace Red { namespace MemoryFramework { namespace DurangoAPI {

// We will use a fixed 4mb page size for all pools. This is recommended by MS (2 x 2mb TLB records)
const Red::System::MemSize c_pageSize = 4 * 1024 * 1024;
const DWORD c_cXMemAllocPageSize = XALLOC_PAGESIZE_4MB;

// Using a 4k alignment ( minimum required by all our APIs )
const Red::System::MemSize c_pageAlignment = 4 * 1024;
const DWORD c_xMemAllocPageAlignment = XALLOC_ALIGNMENT_4K;

// Allocator ID min / max
const Red::System::Uint64 c_allocIDMinimum = eXALLOCAllocatorId_GameMin;
const Red::System::Uint64 c_allocIDMaximum = eXALLOCAllocatorId_GameMax; 

/////////////////////////////////////////////////////////////////////
// Flags testing macros
//	Test flags set, not set, mutual exclusion
#define TEST_FLAGS_INCLUSION( includeFlags, errorText )				\
	if( !( flags & ( includeFlags ) ) )	{							\
	RED_MEMORY_HALT( errorText );								\
	return false;												\
	}

#define TEST_FLAGS_EXCLUSION( excludeFlags, errorText )				\
	if( flags & ( excludeFlags ) )	{								\
	RED_MEMORY_HALT( errorText );								\
	return false;												\
	}

#define TEST_FLAGS_MUTEX( testflags, errorText )					\
	if( __popcnt( flags & ( testflags ) ) != 1 ) {					\
	RED_MEMORY_HALT( errorText );								\
	return false;												\
	}

/////////////////////////////////////////////////////////////////////
// XMemAlloc Stats Tracking
//	Since there is no API to get size of an allocation from XMemAlloc, we need to manually track pointers and sizes
//  This stuff is slow!
class XMemAllocStatsTracking
{
public:
	XMemAllocStatsTracking()
		: m_ptrCount ( 0 )
	{
	}

	void PushEntry( void* ptr, Uint64 size )
	{
		if( m_ptrCount < c_maxAllocationsTracked )
		{
			m_activePtrs[ m_ptrCount ][ 0 ] = reinterpret_cast< Uint64 >( ptr );
			m_activePtrs[ m_ptrCount ][ 1 ] = size;
			++m_ptrCount;
		}
		else
		{
			RED_HALT( "Ran out of slots to track XMemAlloc" );
		}
	}

	// Returns size of the ptr
	Uint64 PopEntry( void* ptr )
	{
		if ( m_ptrCount == 0 )
		{
			return 0u;
		}

		// Run back to front - newer allocs should generally be freed first
		Uint64 ptr64 = reinterpret_cast< Uint64 >( ptr );
		Uint32 i = m_ptrCount - 1;
		while( true )
		{
			if( m_activePtrs[ i ][ 0 ] == ptr64 )
			{
				Uint64 result = m_activePtrs[ i ][ 1 ];
				m_activePtrs[ i ][ 0 ] = m_activePtrs[ m_ptrCount - 1 ][ 0 ];
				m_activePtrs[ i ][ 1 ] = m_activePtrs[ m_ptrCount - 1 ][ 1 ];
				m_ptrCount--;
				return result;
			}

			if( i == 0 )
			{
				return 0u;
			}

			--i;
		}

		return 0u;
	}

private:
	static const Uint32 c_maxAllocationsTracked = 1024 * 1024;
	Uint64 m_activePtrs[ c_maxAllocationsTracked ][ 2 ];			// PTR / SIZE Pairs
	Uint32 m_ptrCount;
};

#ifdef ENABLE_EXTENDED_XMEMALLOC_TRACKING
	XMemAllocStatsTracking gXMemAllocStatsTracking;
#endif

/////////////////////////////////////////////////////////////////////
// XMemAlloc Hooks
class XMemAllocHooks
{
public:
	static void Initialise( XMemAllocStatistics* allocStatsPtr )
	{
		RED_MEMORY_ASSERT( XMemAllocHooks::m_allocStats == nullptr,  "XAllocHooks already initialised!" );
		XMemAllocHooks::m_allocStats = allocStatsPtr;
		XMemSetAllocationHooks( XMemAllocHook, XMemFreeHook );
	}

	static void Release()
	{
		XMemAllocHooks::m_allocStats = nullptr;
	}

	static PVOID XMemAllocHook( SIZE_T size, ULONGLONG attributes )
	{
		void* thePtr = ::XMemAllocDefault( size, attributes );

		RED_MEMORY_ASSERT( thePtr != nullptr,  "Allocation failed with size: %ld", size );
#ifdef ENABLE_EXTENDED_XMEMALLOC_TRACKING
		if( m_allocStats )
		{
			_XALLOC_ATTRIBUTES attribs;
			attribs.dwAttributes = attributes;
			gXMemAllocStatsTracking.PushEntry( thePtr, size );
			m_allocStats->OnAlloc( size, attribs.s.dwAllocatorId );
		}
#endif

		return thePtr;
	}

	static void XMemFreeHook( PVOID address, ULONGLONG attributes )
	{
		::XMemFreeDefault( address, attributes );

#ifdef ENABLE_EXTENDED_XMEMALLOC_TRACKING
		if( m_allocStats )
		{
			_XALLOC_ATTRIBUTES attribs;
			attribs.dwAttributes = attributes;
			Uint64 size = gXMemAllocStatsTracking.PopEntry( address );
			m_allocStats->OnFree( size, attribs.s.dwAllocatorId );
		}
#endif
	}

private:
	static XMemAllocStatistics* m_allocStats;
};
XMemAllocStatistics* XMemAllocHooks::m_allocStats = nullptr;

/////////////////////////////////////////////////////////////////////
// XMem* stats
//
Red::System::Uint64 XMemAllocStatistics::GetTotalAllocated() const
{
	Red::System::Uint64 total = 0;
	for( Red::System::Uint32 i=0; i < c_maximumAllocIds; ++i )
	{
		total += m_stats[i].m_bytesAllocated;
	}
	return total;
}

/////////////////////////////////////////////////////////////////////
// XMem* stats
//	
void XMemAllocStatistics::OnAlloc( Red::System::MemSize size, Red::System::Uint32 allocID )
{
	m_stats[ allocID ].m_bytesAllocated += size;
}

/////////////////////////////////////////////////////////////////////
// XMem* stats
//
void XMemAllocStatistics::OnFree( Red::System::MemSize size, Red::System::Uint32 allocID )
{
	m_stats[ allocID ].m_bytesAllocated -= size;
}

/////////////////////////////////////////////////////////////////////
// CTor
//	
PageAllocatorImpl::PageAllocatorImpl()
#ifdef ENABLE_MEMORY_MANAGER_OVERFLOW_POOL
	: m_debugMemoryBuffer( nullptr )
	, m_debugMemoryBufferSize( 0 )
	, m_debugMemoryHeadPtr( nullptr )
#endif
{
	XMemAllocHooks::Initialise( &m_xMemAllocStats );
}

/////////////////////////////////////////////////////////////////////
// DTor
//
PageAllocatorImpl::~PageAllocatorImpl()
{
	XMemAllocHooks::Release();
}

/////////////////////////////////////////////////////////////////////
// GetXMemAllocStats
//
const XMemAllocStatistics& PageAllocatorImpl::GetXMemAllocStats() const
{
	return m_xMemAllocStats;
}

/////////////////////////////////////////////////////////////////////
// IsExtraDebugMemoryAvailable
//	Returns true if console profile mode is enabled and debug memory mode is set to PIX_Tool or PGI_Tool
Red::System::Bool PageAllocatorImpl::IsExtraDebugMemoryAvailable()
{
#ifdef ENABLE_MEMORY_MANAGER_OVERFLOW_POOL
	Red::System::MemSize debugRegionSize = 0;
	if( DebugMemGetRegion( &debugRegionSize ) != nullptr )
	{
		return debugRegionSize > 0;
	}
#endif
	return false;
}

/////////////////////////////////////////////////////////////////////
// DebugMemoryAllocate
//	Allocate from debug memory buffer
const void* PageAllocatorImpl::DebugMemoryAllocate( Red::System::MemSize size )
{
	const void* result = nullptr;

#ifdef ENABLE_MEMORY_MANAGER_OVERFLOW_POOL
	Red::System::MemSize allocEnd = reinterpret_cast< Red::System::MemUint >( m_debugMemoryHeadPtr ) + size;
	if( allocEnd <= ( reinterpret_cast< Red::System::MemUint >( m_debugMemoryBuffer ) + m_debugMemoryBufferSize ) )
	{
		result = m_debugMemoryHeadPtr;
		m_debugMemoryHeadPtr = reinterpret_cast< const void* >( reinterpret_cast< Red::System::MemUint >( m_debugMemoryHeadPtr ) + size );
	}
#endif

	return result;
}

/////////////////////////////////////////////////////////////////////
// Initialise 
Red::System::Bool PageAllocatorImpl::Initialise()
{	
#ifdef ENABLE_MEMORY_MANAGER_OVERFLOW_POOL
	m_debugMemoryBuffer = DebugMemGetRegion( &m_debugMemoryBufferSize );
	m_debugMemoryHeadPtr = m_debugMemoryBuffer;
#endif
	return true;
}

/////////////////////////////////////////////////////////////////////
// GetPageSize
//	Get the size of a single page
Red::System::MemSize PageAllocatorImpl::GetPageSize()
{
	return c_pageSize;
}

/////////////////////////////////////////////////////////////////////
// GetAllocatedPages
//	Returns the num. pages allocated by our stuff
Red::System::MemSize PageAllocatorImpl::GetAllocatedPages()
{
	return m_xMemAllocStats.GetTotalAllocated() / c_pageSize;
}

/////////////////////////////////////////////////////////////////////
// RoundUpSize
//	Round size up to page size
Red::System::MemSize PageAllocatorImpl::RoundUpSize( Red::System::MemSize size, Red::System::MemSize roundUpTo )
{
	return (size + (roundUpTo-1)) & ~(roundUpTo-1);
}

Red::System::Bool PageAllocatorImpl::ValidateFlags( Red::System::Uint32 flags )
{
	if( flags & Allocator_UseDebugMemory )
	{
		return true;		// All other flags are ignored, so it doesn't matter if they are 'invalid'
	}

	TEST_FLAGS_INCLUSION( Allocator_AccessCpu | Allocator_AccessCpuGpu,  "AccessCpu or AccessCpuGpu must be specified" );
	TEST_FLAGS_MUTEX ( Allocator_AccessCpu | Allocator_AccessCpuGpu, "Specify EITHER Cpu or CpuGpu access" );

	if( flags & Allocator_AccessCpu )
	{
		TEST_FLAGS_EXCLUSION( Allocator_GpuReadOnly | Allocator_GpuNonCoherent | Allocator_CommandBuffer, "Cannot specify GPU flags for CPU-only memory" );
		TEST_FLAGS_INCLUSION( Allocator_Cached | Allocator_WriteCombine | Allocator_Uncached, "Cpu memory must either be cached, write-combined, or uncached" );
		TEST_FLAGS_MUTEX( Allocator_Cached | Allocator_WriteCombine | Allocator_Uncached, "Specify only one access type (cached/ uncached / write combined)" );
	}

	if( flags & Allocator_AccessCpuGpu )
	{
		TEST_FLAGS_INCLUSION( Allocator_Cached | Allocator_WriteCombine, "Gpu-visible memory must either be cached or write-combined" );
		TEST_FLAGS_MUTEX( Allocator_Cached | Allocator_WriteCombine, "Specify only one access type (cached / write combined)" );
		TEST_FLAGS_EXCLUSION( Allocator_Uncached, "Gpu-visible memory cannot be uncached" );

		if( flags & Allocator_GpuReadOnly )
		{
			TEST_FLAGS_INCLUSION( Allocator_WriteCombine, "GPU-Ready-Only memory must be write-combined" );
		}
		
		if( flags & Allocator_GpuNonCoherent )
		{
			TEST_FLAGS_INCLUSION( Allocator_Cached, "GPU-non-coherent memory must be cached" );
		}
	}

	return true;
}

/////////////////////////////////////////////////////////////////////
// GetXMemAllocAttribs
//	Gets the XMemAlloc attributes based on our internal flags. Assumes flags are already valid
Red::System::Uint64 PageAllocatorImpl::GetXMemAllocAttribs( Red::System::Uint32 flags )
{
	Red::System::Int64 allocatorID = eXALLOCAllocatorId_GameMin + ( RED_MEMORY_DURANGO_GET_ALLOCID( flags ) );
	Red::System::Int64 memoryType = 0;

	if( flags & Allocator_AccessCpu )
	{
		if( flags & Allocator_Cached )
			memoryType = XALLOC_MEMTYPE_PHYSICAL_CACHEABLE;
		else if( flags & Allocator_WriteCombine )
			memoryType = XALLOC_MEMTYPE_PHYSICAL_WRITECOMBINE;
		else if( flags & Allocator_Uncached )
			memoryType = XALLOC_MEMTYPE_PHYSICAL_UNCACHED;
	}
	else if( flags & Allocator_AccessCpuGpu )
	{
		if( flags & Allocator_CommandBuffer )
		{
			if( flags & Allocator_Cached )
				memoryType = XALLOC_MEMTYPE_GRAPHICS_COMMAND_BUFFER_CACHEABLE;
			else if( flags & Allocator_WriteCombine )
				memoryType = XALLOC_MEMTYPE_GRAPHICS_COMMAND_BUFFER_WRITECOMBINE;
		}
		else
		{
			if( flags & Allocator_Cached )
			{
				if( flags & Allocator_GpuNonCoherent )
					memoryType = XALLOC_MEMTYPE_GRAPHICS_CACHEABLE_NONCOHERENT_GPU_READONLY;
				else 
					memoryType = XALLOC_MEMTYPE_GRAPHICS_CACHEABLE;
			}
			else if( flags & Allocator_WriteCombine )
			{
				if( flags & Allocator_GpuReadOnly )
					memoryType = XALLOC_MEMTYPE_GRAPHICS_WRITECOMBINE_GPU_READONLY;
				else
					memoryType = XALLOC_MEMTYPE_GRAPHICS_WRITECOMBINE;
			}
		}
	}

	Red::System::Int64 pageSize = ( flags & Allocator_Use64kPages ) ? c_cXMemAllocPageSize :  XALLOC_PAGESIZE_64KB;
	Red::System::Int64 pageAlignment = ( flags & Allocator_Use64kPages ) ? c_xMemAllocPageAlignment :   XALLOC_ALIGNMENT_64K;

	return MAKE_XALLOC_ATTRIBUTES( allocatorID, 0, memoryType, pageSize, pageAlignment );
}

/////////////////////////////////////////////////////////////////////
// GetNewPage
//	Request a single page of memory
void* PageAllocatorImpl::GetNewPage( Red::System::Uint32 flags )
{
	MemSize sizeAllocated = 0;
	return GetPagedMemory( c_pageSize, sizeAllocated, flags );
}

/////////////////////////////////////////////////////////////////////
// GetPagedMemory
//	Request some continuous paged memory
void* PageAllocatorImpl::GetPagedMemory( Red::System::MemSize size, Red::System::MemSize& actualSize, Red::System::Uint32 flags )
{
	const Uint32 c_specialCaseNonCoherent = Allocator_GpuNonCoherent | Allocator_AccessCpuGpu | Allocator_Cached;

	if( !ValidateFlags( flags ) )
	{
		return nullptr;
	}

	Red::System::MemSize sizeRounded = RoundUpSize( size, flags & Allocator_Use64kPages ? ( 64 * 1024 ) : c_pageSize );
	if( flags & Allocator_UseDebugMemory )
	{
		void* result = const_cast< void* >( DebugMemoryAllocate( sizeRounded ) );
		if( result )
		{
			actualSize = sizeRounded;
		}
		return result;
	}
	else if( ( flags & c_specialCaseNonCoherent ) == c_specialCaseNonCoherent && !(flags & Allocator_GpuReadOnly) )
	{
		// HACK! XMemAlloc cannot support noncoherant with GPU write, so we have to go through VirtualAlloc
		void* thePtr = VirtualAlloc( nullptr, sizeRounded, MEM_RESERVE | MEM_COMMIT | MEM_GRAPHICS | MEM_4MB_PAGES, PAGE_READWRITE );
		if( thePtr )
		{
			actualSize = sizeRounded;
		}
		return thePtr;
	}
	else
	{
		ULONGLONG xMemAllocAttribs = GetXMemAllocAttribs( flags );
		void* thePtr = ::XMemAlloc( sizeRounded, xMemAllocAttribs );
		if( thePtr )
		{
			actualSize = sizeRounded;
		}
		return thePtr;
	}
}

/////////////////////////////////////////////////////////////////////
// FreePagedMemory
//	Free the allocated memory
void PageAllocatorImpl::FreePagedMemory( void* ptr, Red::System::MemSize size, Red::System::Uint32 flags )
{
	RED_UNUSED( size );		// THis basically implies that a sub-region cannot be freed, you must use the base address
	if( !( flags & Allocator_UseDebugMemory ) )		// Debug memory is never freed, its handled by the OS
	{
		ULONGLONG xMemAllocAttribs = GetXMemAllocAttribs( flags );
		::XMemFree( ptr, xMemAllocAttribs );
	}
}

} } }
