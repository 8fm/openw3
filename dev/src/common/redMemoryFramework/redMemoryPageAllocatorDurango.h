/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_PAGE_ALLOCATOR_DURANGO_H
#define _RED_MEMORY_PAGE_ALLOCATOR_DURANGO_H
#pragma once

#include "redMemoryFrameworkTypes.h"

namespace Red { namespace MemoryFramework { namespace DurangoAPI {

class ExtendedMemoryStats
{
public:
	// These are taken from our internal XMemAlloc tracking
	Red::System::MemSize m_staticsAllocated;
	Red::System::MemSize m_overflowAllocated;
	Red::System::MemSize m_enginePoolsAllocated;
	Red::System::MemSize m_d3dAllocated;
	Red::System::MemSize m_audioAllocated;
	Red::System::MemSize m_platformAllocated;
	Red::System::MemSize m_middlewareAllocated;

	// Legacy memory = stuff outside XMemAlloc. Mainly MS libraries, and CRT.
	Red::System::MemSize m_legacyUsed;
	Red::System::MemSize m_legacyAvailable;
	Red::System::MemSize m_legacyPeak;

	// This memory includes graphics + cpu memory, as well as anything allocated for page tables
	Red::System::MemSize m_titleUsed;
	Red::System::MemSize m_titleAvailable;
};

// We will collect the calls through XMem* in this class
class XMemAllocStatistics
{
public:
	XMemAllocStatistics()	{ }
	~XMemAllocStatistics()  { }

	void OnAlloc( Red::System::MemSize size, Red::System::Uint32 allocID );
	void OnFree( Red::System::MemSize size, Red::System::Uint32 allocID );
	Red::System::Uint64 GetTotalAllocated() const;

	// Keep stats for each allocation ID
	struct AllocStats
	{
		AllocStats()
			: m_bytesAllocated( 0ull )
		{
		}

		volatile Red::System::Uint64 m_bytesAllocated;
	};

	static const Red::System::Uint32 c_maximumAllocIds = 256;
	AllocStats m_stats[ c_maximumAllocIds ];
};
	
// On Durango, the page allocator will go through XMemAlloc. Stats will be collected via XMemAlloc / Free callbacks
class PageAllocatorImpl
{
public:
	PageAllocatorImpl();
	~PageAllocatorImpl();

	// Get the size of a single page
	Red::System::MemSize GetPageSize();

	// Get the number of pages allocated
	Red::System::MemSize GetAllocatedPages();

	// Request a single page of memory
	void* GetNewPage( Red::System::Uint32 flags );

	// Request a bunch of memory (rounded up and aligned to the page size)
	void* GetPagedMemory( Red::System::MemSize size, Red::System::MemSize& actualSize, Red::System::Uint32 flags );

	// Free the allocated memory
	void FreePagedMemory( void* ptr, Red::System::MemSize size, Red::System::Uint32 flags );

	// Initialise 
	Red::System::Bool Initialise();

	// Get XMemAlloc tracked statistics
	const XMemAllocStatistics& GetXMemAllocStats() const;

	// Returns true if console profile mode is enabled and debug memory mode is set to PIX_Tool or PGI_Tool
	Red::System::Bool IsExtraDebugMemoryAvailable();

	// AnnotateMemoryRegion Does nothing on Xbox
	void AnnotateMemoryRegion( Red::System::MemUint, Red::System::MemSize, const Red::System::AnsiChar* ) { }

	// ProtectMemoryRegion does nothing since we hit XMemAlloc directly, rather than handling virtual memory
	void ProtectMemoryRegion( Red::System::MemUint, Red::System::MemSize, Red::System::Uint32, Red::System::Bool )	{ }

private:
	// Allocate from debug memory 
	const void* DebugMemoryAllocate( Red::System::MemSize size );

	// Round size up to internal page size
	Red::System::MemSize RoundUpSize( Red::System::MemSize size, Red::System::MemSize roundUpTo );

	// Validate parameters to ensure there are no dodgy flags
	Red::System::Bool ValidateFlags( Red::System::Uint32 flags );

	// Gets the XMemAlloc attributes based on our internal flags. Assumes flags are already valid
	Red::System::Uint64 GetXMemAllocAttribs( Red::System::Uint32 flags );

	// Track anything going through XMemAlloc
	XMemAllocStatistics m_xMemAllocStats;

#ifdef ENABLE_MEMORY_MANAGER_OVERFLOW_POOL
	// Debug memory allocation bookkeeping. Just a simple linear buffer allocator. No free!
	const void* m_debugMemoryBuffer;
	Red::System::MemSize m_debugMemoryBufferSize;
	const void* m_debugMemoryHeadPtr;		// Head of circular buffer
#endif
};

} } }

#endif
