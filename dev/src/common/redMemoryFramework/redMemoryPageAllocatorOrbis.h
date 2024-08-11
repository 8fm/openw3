/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_PAGE_ALLOCATOR_ORBIS_H
#define _RED_MEMORY_PAGE_ALLOCATOR_ORBIS_H
#pragma once

#include "redMemoryFrameworkTypes.h"

namespace Red { namespace MemoryFramework { namespace OrbisAPI {

class ExtendedMemoryStats
{
public:
	Red::System::MemSize m_directMemoryUsed;
	Red::System::MemSize m_directMemoryFree;
	Red::System::MemSize m_approxFlexibleMemoryUsed;
	Red::System::MemSize m_approxFlexibleMemoryFree;
};

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

	// Label memory. Name = 32 characters max
	void AnnotateMemoryRegion( Red::System::MemUint address, Red::System::MemSize size, const Red::System::AnsiChar* name );

	// Initialise 
	Red::System::Bool Initialise();

	// Platform-specific Metrics
	Red::System::MemSize GetDirectMemoryAllocated();
	Red::System::MemSize GetFlexibleMemoryAllocated();

	// Returns true if there is > 5gb memory available 
	Red::System::Bool IsExtraDebugMemoryAvailable();

	void ProtectMemoryRegion( Red::System::MemUint address, Red::System::MemSize regionSize, Red::System::Uint32 poolFlags, Red::System::Bool allowCpuAccess );

private:
	// Validate parameters to ensure there are no dodgy flags
	Red::System::Bool ValidateFlags( Red::System::Uint32 flags );

	// Allocate direct memory and map it to virtual address space
	void* AllocateDirectMemory( Red::System::MemSize size, Red::System::Uint32 flags, Red::System::MemSize& actualSize );

	// Allocate flexible memory and map it to virtual address space
	void* AllocateFlexibleMemory( Red::System::MemSize size, Red::System::Uint32 flags, Red::System::MemSize& actualSize );

	// Release direct memory + unmap address space
	void ReleaseDirectMemory( void* ptr, Red::System::MemSize size );
	void ReleaseUnmappedDirectMemory( Red::System::MemUint ptr, Red::System::MemSize size );

	// Release flexible memory + unmap address space
	void ReleaseFlexibleMemory( void* ptr, Red::System::MemSize size );

	Red::System::MemSize m_directMemoryPagesAllocated;
	Red::System::MemSize m_flexibleMemoryPagesAllocated;
};

} } }

#endif
