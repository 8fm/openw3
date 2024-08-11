/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_PAGE_ALLOCATOR_H
#define _RED_MEMORY_PAGE_ALLOCATOR_H
#pragma once

#include "redMemoryFrameworkPlatform.h"
#include "redMemoryThreads.h"

#ifdef RED_MEMORY_FRAMEWORK_PLATFORM_WINDOWS_API
	#include "redMemoryPageAllocatorWin.h"
#elif defined( RED_MEMORY_FRAMEWORK_PLATFORM_ORBIS_API )
	#include "redMemoryPageAllocatorOrbis.h"
#elif defined( RED_MEMORY_FRAMEWORK_PLATFORM_DURANGO_API )
	#include "redMemoryPageAllocatorDurango.h"
#endif

#include "../redSystem/types.h"

namespace Red { namespace MemoryFramework {

///////////////////////////////////////////////////////////////////
// PageAllocator is used to allocate system pages
// This is just a wrapper around the platform-specific implementation
class PageAllocator
{
public:
	// Initialise and reserve physical memory on the device
	RED_INLINE Red::System::Bool Initialise()
	{
		CMutex::TScopedLock lock( &m_mutex );
		return m_pageAllocator.Initialise();
	}

	// Get the size of a single page
	RED_INLINE Red::System::MemSize GetPageSize()
	{
		CMutex::TScopedLock lock( &m_mutex );
		return m_pageAllocator.GetPageSize();
	}

	// Get the number of pages allocated
	RED_INLINE Red::System::MemSize GetAllocatedPages()
	{
		CMutex::TScopedLock lock( &m_mutex );
		return m_pageAllocator.GetAllocatedPages();
	}

	// Request a single page of memory
	RED_INLINE void* GetNewPage( Red::System::Uint32 flags )
	{
		CMutex::TScopedLock lock( &m_mutex );
		return m_pageAllocator.GetNewPage( flags );
	}

	// Request a bunch of memory (rounded up and aligned to the page size)
	RED_INLINE void* GetPagedMemory( Red::System::MemSize size, Red::System::MemSize& actualSize, Red::System::Uint32 flags )
	{
		CMutex::TScopedLock lock( &m_mutex );
		return m_pageAllocator.GetPagedMemory( size, actualSize, flags );
	}

	// Free the allocated memory
	RED_INLINE void FreePagedMemory( void* ptr, Red::System::MemSize size, Red::System::Uint32 flags )
	{
		CMutex::TScopedLock lock( &m_mutex );
		return m_pageAllocator.FreePagedMemory( ptr, size, flags );
	}

	// 'Debug' memory stuff (for platform-specific memory stuff)
	RED_INLINE Bool IsExtraDebugMemoryAvailable()
	{
		CMutex::TScopedLock lock( &m_mutex );
		return m_pageAllocator.IsExtraDebugMemoryAvailable();
	}

	// Annotate memory region on platforms that support it
	RED_INLINE void AnnotateMemoryRegion( Red::System::MemUint address, Red::System::MemSize size, const AnsiChar* name )
	{
		CMutex::TScopedLock lock( &m_mutex );
		return m_pageAllocator.AnnotateMemoryRegion( address, size, name );
	}

	// Get the instance of the page allocator
	RED_INLINE static PageAllocator& GetInstance()
	{
		static PageAllocator s_instance;
		return s_instance;
	}

	// Used to get platform-specific info from the page allocator
	RED_INLINE OSAPI::PageAllocatorImpl& GetPlatformImplementation()
	{
		return m_pageAllocator;
	}

	RED_INLINE void ProtectMemoryRegion( Red::System::MemUint address, Red::System::MemSize regionSize, Red::System::Uint32 poolFlags, Red::System::Bool allowCpuAccess )
	{
		CMutex::TScopedLock lock( &m_mutex );
		return m_pageAllocator.ProtectMemoryRegion( address, regionSize, poolFlags, allowCpuAccess );
	}

private:
	PageAllocator()
	{
		Initialise();
	}

	~PageAllocator()
	{
	}

	OSAPI::PageAllocatorImpl m_pageAllocator;
	mutable CMutex m_mutex;
};

} }

#endif
