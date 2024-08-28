/**
* Copyright (c) 2018 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pageAllocatorLinux.h"
#include "assert.h"
#include "utils.h"

#include <sys/mman.h>

namespace red
{
namespace memory
{
	PageAllocatorLinux::PageAllocatorLinux()
	{}
	
	PageAllocatorLinux::~PageAllocatorLinux()
	{}

	void PageAllocatorLinux::OnInitialize()
	{
		// has to use real page size as this is the allocation granularity on Linux
		const long pageSize = sysconf( _SC_PAGESIZE );
		if ( pageSize < 0 )
		{
			const Int32 err = errno;
			RED_MEMORY_HALT( "Failed to query _SC_PAGESIZE errno=0x%08X", err );
		}

		// On Linux, mmap page size and allocation granularity should be the same
		SetPageSize( pageSize );
	}

	VirtualRange PageAllocatorLinux::OnReserveRange( u64 size, u32 pageSize, u32 )
	{
		const u64 sizeRoundedToPageSize = RoundUp( size, static_cast< u64 >( pageSize ) );
		void* pages = ::mmap( nullptr, sizeRoundedToPageSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
		if ( pages == MAP_FAILED )
		{
			const Int32 err = errno;
			RED_MEMORY_HALT( "Failed to reserve %d system pages (%d bytes) (errno=0x%08X)", sizeRoundedToPageSize / pageSize, sizeRoundedToPageSize, err );
		}
		
		const u64 start = AddressOf( pages );
		RED_MEMORY_ASSERT( IsAligned( start, pageSize ), "" );
		const u64 end = start + sizeRoundedToPageSize;
		VirtualRange range = { start, end };
		return range;
	}

	void PageAllocatorLinux::OnReleaseRange( const VirtualRange & range )
	{
		void * ptr = reinterpret_cast< void* >( range.start );

		if ( ::munmap( ptr, GetVirtualRangeSize( range ) ) < 0 )
		{
			const Int32 err = errno;
			RED_MEMORY_HALT( "Failed to call munmap for ptr=0x%pm, size=%llu, errno=0x%08X", ptr, ( range.end - range.start ), err );
			return;
		}

	}
}
}
