/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pageAllocatorOrbis.h"
#include "utils.h"

namespace red
{
namespace memory
{
namespace  
{
	// TODO. Orbis sceKernelReserveVirtualRange doesn't work. and I'm too lazy to write a full fledge page allocator.
	// However we have a terabyte of address range.... 
	// Also, sceKernelMapDirectMemory will search from starting address, so it "should" skip mapped address safely.

	//const u64 c_systemReserveMemoryMapRange = SCE_KERNEL_APP_MAP_AREA_START_ADDR;
	const u64 c_orbisSystemPageSize = 16 * 1024;
	static u64 s_reservedMemoryMapRange = SCE_KERNEL_APP_MAP_AREA_START_ADDR;
}

	PageAllocatorOrbis::PageAllocatorOrbis()
	{}

	PageAllocatorOrbis::~PageAllocatorOrbis()
	{}

	void PageAllocatorOrbis::OnInitialize()
	{
		SetPageSize( c_orbisSystemPageSize );
	}

	VirtualRange PageAllocatorOrbis::OnReserveRange( u64 size, u32 pageSize, u32  ) 
	{
		const u64 sizeRoundedToPageSize = RoundUp( size, c_orbisSystemPageSize );
		u64 reservedAddress = 0;
		{
			CScopedLock< CMutex > scopedLock( m_lock );
			reservedAddress = s_reservedMemoryMapRange;
			s_reservedMemoryMapRange += sizeRoundedToPageSize;
		}

		VirtualRange range = { reservedAddress, reservedAddress + sizeRoundedToPageSize };
		return range;
	}

	void PageAllocatorOrbis::OnReleaseRange( const VirtualRange & )
	{/* See comments at the beginning of file */}
}
}
