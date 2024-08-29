/**
* Copyright (c) 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_PAGE_ALLOCATOR_LINUX_H
#define _RED_MEMORY_PAGE_ALLOCATOR_LINUX_H
#pragma once

#include "redMemoryFrameworkTypes.h"

namespace Red { namespace MemoryFramework { namespace LinuxAPI {

class ExtendedMemoryStats
{
public:
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

	// Initialise
	Red::System::Bool Initialise();

	// Linux always has extra memory available for debug stuff
	Red::System::Bool IsExtraDebugMemoryAvailable() { return true; }

	// AnnotateMemoryRegion Does nothing on Linux
	void AnnotateMemoryRegion( Red::System::MemUint, Red::System::MemSize, const Red::System::AnsiChar* ) { }

	void ProtectMemoryRegion( Red::System::MemUint address, Red::System::MemSize regionSize, Red::System::Uint32 poolFlags, Red::System::Bool allowCpuAccess );

private:
	Red::System::MemSize m_pageSize;
	Red::System::MemSize m_allocatedPages;
};

} } }

#endif
