/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pageAllocatorDurango.h"
#include "assert.h"
#include "utils.h"
#include "flags.h"

namespace red
{
namespace memory
{
namespace
{
	const u64 c_durangoSystemPageSize = 64 * 1024; 
	const u64 c_durangoPageAlignment = 4 * 1024;
}
	
	PageAllocatorDurango::PageAllocatorDurango()
	{}

	PageAllocatorDurango::~PageAllocatorDurango()
	{}

	void PageAllocatorDurango::OnInitialize()
	{
		SetPageSize( c_durangoSystemPageSize );
	}

	VirtualRange PageAllocatorDurango::OnReserveRange( u64 size, u32 /*pageSize*/, u32 flags ) 
	{
		const u64 sizeRoundedToPageSize = RoundUp( size, c_durangoSystemPageSize );
		u32 reserveFlags = flags & Flags_GPU_Read_Write ? MEM_RESERVE | MEM_LARGE_PAGES | MEM_GRAPHICS : MEM_RESERVE | MEM_LARGE_PAGES;
		void * pages = ::VirtualAlloc( nullptr, sizeRoundedToPageSize, reserveFlags, PAGE_READONLY );
		RED_MEMORY_ASSERT( pages != nullptr,  "Failed to reserve %d system pages (%d bytes)", sizeRoundedToPageSize / c_durangoSystemPageSize, sizeRoundedToPageSize );
		const u64 start = AddressOf( pages );
		const u64 end = start + sizeRoundedToPageSize;
		VirtualRange range = { start, end };
		return range;
	}

	void PageAllocatorDurango::OnReleaseRange( const VirtualRange & range )
	{
		void * ptr = reinterpret_cast< void* >( range.start );
		::VirtualFree( ptr, 0, MEM_RELEASE );
	}
}
}
