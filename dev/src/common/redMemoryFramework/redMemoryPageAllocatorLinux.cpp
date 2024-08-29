/**
* Copyright (c) 2013 CD Projekt Red. All Rights Reserved.
*/

#include "redMemoryPageAllocatorLinux.h"
#include "redMemoryAssert.h"
#include "../redSystem/os.h"

#include <sys/mman.h>

namespace Red { namespace MemoryFramework { namespace LinuxAPI {

/////////////////////////////////////////////////////////////////////
// CTor
//	Query the OS for the virtual memory page size
PageAllocatorImpl::PageAllocatorImpl()
	: m_allocatedPages( 0 )
{
	m_pageSize = ::sysconf(_SC_PAGESIZE);
}

/////////////////////////////////////////////////////////////////////
// DTor
//
PageAllocatorImpl::~PageAllocatorImpl()
{
}

/////////////////////////////////////////////////////////////////////
// GetPageSize
//	Get the size of a single page
Red::System::MemSize PageAllocatorImpl::GetPageSize()
{
	return m_pageSize;
}

/////////////////////////////////////////////////////////////////////
// GetAllocatedPages
//	Returns the num. pages allocated
Red::System::MemSize PageAllocatorImpl::GetAllocatedPages()
{
	return m_allocatedPages;
}

/////////////////////////////////////////////////////////////////////
// GetNewPage
//	Request a single page of memory
void* PageAllocatorImpl::GetNewPage( Red::System::Uint32 flags )
{
	void* newPage = ::mmap( nullptr, m_pageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
	RED_MEMORY_ASSERT( newPage != MAP_FAILED,  "Failed to allocate a new system page" );

	if( newPage != MAP_FAILED )
	{
		if( !(flags & ::Red::MemoryFramework::Allocator_NoPhysicalReserved) )
		{
			// Try to lock the page into physical memory. This ensure it will never page-fault
			::mlock( newPage, m_pageSize );
		}

		m_allocatedPages++;
	}

	return newPage;
}

/////////////////////////////////////////////////////////////////////
// GetPagedMemory
//	Request some continuous paged memory
void* PageAllocatorImpl::GetPagedMemory( Red::System::MemSize size, Red::System::MemSize& actualSize, Red::System::Uint32 flags )
{
	Red::System::MemSize sizeRoundedToPageSize = (size + (m_pageSize-1)) & ~(m_pageSize-1);
	void* newPages = ::mmap( nullptr, sizeRoundedToPageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
	RED_MEMORY_ASSERT( newPages != MAP_FAILED,  "Failed to allocate %d system pages (%d bytes)", sizeRoundedToPageSize / m_pageSize, sizeRoundedToPageSize );

	if( newPages != MAP_FAILED )
	{
		if( !(flags & ::Red::MemoryFramework::Allocator_NoPhysicalReserved) )
		{
			// Try to lock the entire region into physical memory
			::mlock( newPages, sizeRoundedToPageSize );
		}

		m_allocatedPages += (sizeRoundedToPageSize / m_pageSize);
	}

	actualSize = sizeRoundedToPageSize;
	return newPages;
}

/////////////////////////////////////////////////////////////////////
// FreePagedMemory
//	Free the allocated memory
void PageAllocatorImpl::FreePagedMemory( void* ptr, Red::System::MemSize size, Red::System::Uint32 flags )
{
	RED_UNUSED( flags );
	RED_MEMORY_ASSERT( ( size & (m_pageSize-1) ) == 0, "Trying to free memory with size that is not a multiple of page size" );

	// Release the address space
	if( size > 0 )
	{
		::munlock( ptr, size );
	}

	// Free the memory
	const int result = ::munmap( ptr, size );
	RED_MEMORY_ASSERT( result == 0, "Failed to free system memory. VirtualFree requires that only base addresses from VirtualAlloc can be freed" );

	// Update metrics
	if( result == 0 )
	{
		m_allocatedPages -= ( size / m_pageSize );
	}
}

/////////////////////////////////////////////////////////////////////
// Initialise
Red::System::Bool PageAllocatorImpl::Initialise()
{
	return true;
}

/////////////////////////////////////////////////////////////////////
// ProtectMemoryRegion
//	Enable / disable CPU read/write access
void PageAllocatorImpl::ProtectMemoryRegion( Red::System::MemUint address, Red::System::MemSize regionSize, Red::System::Uint32 poolFlags, Red::System::Bool allowCpuAccess )
{
	RED_UNUSED( poolFlags );

	int protectionFlags = allowCpuAccess ? (PROT_READ | PROT_WRITE) : PROT_NONE;
	::mprotect( reinterpret_cast< void* >( address ), regionSize, protectionFlags );
}

} } }
