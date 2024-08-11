/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "redMemoryPageAllocatorWin.h"
#include "redMemoryAssert.h"
#include "../redSystem/os.h"

namespace Red { namespace MemoryFramework { namespace WinAPI {

// Process working set defines the maximum amount of virtual memory we can lock into physical ram manually
#ifdef RED_ARCH_X86
	const Red::System::MemSize c_maximumWorkingSetSize = 3u * 1024u * 1024u;
#else
	const Red::System::MemSize c_maximumWorkingSetSize = 8u * 1024u * 1024u;
#endif

/////////////////////////////////////////////////////////////////////
// CTor
//	Query the OS for the virtual memory page size
PageAllocatorImpl::PageAllocatorImpl()
	: m_allocatedPages( 0 )
{
	SYSTEM_INFO sysInfo;
	::GetSystemInfo( &sysInfo );

	// On Windows machines, allocations are aligned to dwAllocationGranularity, but page size is dwPageSize
	// and is generally smaller
	// In order to reduce virtual memory fragmentation, we will allocate in chunks of dwAllocationGranularity
	// otherwise, we will introduce holes
	m_pageSize = sysInfo.dwAllocationGranularity;

	::SetProcessWorkingSetSize( GetCurrentProcess(), c_maximumWorkingSetSize, c_maximumWorkingSetSize );
}

/////////////////////////////////////////////////////////////////////
// DTor
//
PageAllocatorImpl::~PageAllocatorImpl()
{
	// In an ideal world we would test that everything is deallocated.
	// However, since the MSVC++ CRT libs call our global operator new / delete before statics / globals are initialised by CRT,
	// there will be stuff left that we have to trust the OS to release
}

/////////////////////////////////////////////////////////////////////
// QueryRegionSize
//
Red::System::MemSize PageAllocatorImpl::QueryRegionSize( void* basePtr )
{
	MEMORY_BASIC_INFORMATION regionInfo;
	if( VirtualQuery( basePtr, &regionInfo, sizeof( regionInfo ) ) == 0 )
	{
		return 0;
	}

	return regionInfo.RegionSize;
}

/////////////////////////////////////////////////////////////////////
// MakeRegionReadOnly
void PageAllocatorImpl::MakeRegionReadOnly( void* basePtr, Red::System::MemSize size )
{
	DWORD oldProtectionFlags = 0;
	VirtualProtect( basePtr, size, PAGE_READONLY, &oldProtectionFlags );
}

/////////////////////////////////////////////////////////////////////
// Initialise 
Red::System::Bool PageAllocatorImpl::Initialise()
{	
	return true;
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
	// Reserve (map the address space) and commit (allocate physical memory for it) 
	void* newPage = ::VirtualAlloc( NULL, m_pageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
	RED_MEMORY_ASSERT( newPage != nullptr,  "Failed to allocate a new system page" );

	if( newPage != nullptr )
	{
		if( !(flags & ::Red::MemoryFramework::Allocator_NoPhysicalReserved) )
		{
			// Try to lock the page into physical memory. This ensure it will never page-fault
			VirtualLock( newPage, m_pageSize );
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
	void* newPages = ::VirtualAlloc( NULL, sizeRoundedToPageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
	RED_MEMORY_ASSERT( newPages != nullptr,  "Failed to allocate %d system pages (%d bytes)", sizeRoundedToPageSize / m_pageSize, sizeRoundedToPageSize );

	if( newPages != nullptr )
	{
		if( !(flags & ::Red::MemoryFramework::Allocator_NoPhysicalReserved) )
		{
			// Try to lock the entire region into physical memory
			VirtualLock( newPages, sizeRoundedToPageSize );
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
		VirtualUnlock( ptr, size );
	}

	// Free the memory
	BOOL freedSuccess = VirtualFree( ptr, 0, MEM_RELEASE );
	RED_MEMORY_ASSERT( freedSuccess, "Failed to free system memory. VirtualFree requires that only base addresses from VirtualAlloc can be freed" );

	// Update metrics
	if( freedSuccess )
	{
		m_allocatedPages -= ( size / m_pageSize );
	}
}

/////////////////////////////////////////////////////////////////////
// ProtectMemoryRegion
//	Enable / disable CPU read/write access
void PageAllocatorImpl::ProtectMemoryRegion( Red::System::MemUint address, Red::System::MemSize regionSize, Red::System::Uint32 poolFlags, Red::System::Bool allowCpuAccess )
{
	RED_UNUSED( poolFlags );

	DWORD protectionFlags = allowCpuAccess ? PAGE_READWRITE : PAGE_NOACCESS;
	DWORD oldProtectionFlags = 0;

	VirtualProtect( reinterpret_cast< void* >( address ), regionSize, protectionFlags, &oldProtectionFlags );
}

} } }