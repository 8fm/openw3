/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "systemAllocatorWin.h"
#include "assert.h"
#include "utils.h"
#include "flags.h"

namespace red
{
namespace memory
{
namespace 
{
	u32 ComputePageProtectionFlags( u32 flags )
	{
		u32 result = 0;
		if( flags & Flags_CPU_Write )
		{
			result |= PAGE_READWRITE;
		}
		else if( flags & Flags_CPU_Read )
		{
			result |= PAGE_READONLY;
		}	
		else
		{
			result |= PAGE_NOACCESS;
		}

		return result;
	}

}
	SystemAllocatorWin::SystemAllocatorWin()
		:	m_pageSize( 0 ),
			m_totalPhysicalMemoryAvailable( 0 )
	{
		SYSTEM_INFO sysInfo;
		::GetSystemInfo( &sysInfo );

		// On Windows machines, allocations are aligned to dwAllocationGranularity, but page size is dwPageSize
		// and is generally smaller
		// In order to reduce virtual memory fragmentation, we will allocate in chunks of dwAllocationGranularity
		// otherwise, we will introduce holes
		m_pageSize = sysInfo.dwAllocationGranularity;
	}

	SystemAllocatorWin::~SystemAllocatorWin()
	{}

	void SystemAllocatorWin::OnInitialize()
	{
		MEMORYSTATUS status;
		status.dwLength = sizeof( status );
		GlobalMemoryStatus( &status );
		m_totalPhysicalMemoryAvailable = status.dwAvailPhys;
	}

	u64 SystemAllocatorWin::OnReleaseVirtualRange( const VirtualRange & range )
	{
		u64 commitedMemory = 0;
		u64 currentAddress = range.start;
		u64 queryResult = 0;
		
		do
		{
			const void * address = reinterpret_cast< const void * >( currentAddress );
			MEMORY_BASIC_INFORMATION info;
			queryResult = VirtualQuery( address, &info, sizeof( info ) );
			if( queryResult )
			{
				if( info.State & MEM_COMMIT )
				{
					commitedMemory += info.RegionSize;
				}

				currentAddress = AddressOf( info.BaseAddress ) + info.RegionSize;
			}
		}
		while( queryResult && currentAddress < range.end );

		return commitedMemory;
	}
	
	SystemBlock SystemAllocatorWin::OnCommit( const SystemBlock & block, u32 flags )
	{
		void * ptr = reinterpret_cast< void* >( block.address );
		const u64 size = RoundUp( block.size, m_pageSize );
		void * result = ::VirtualAlloc( ptr, size, MEM_COMMIT, ComputePageProtectionFlags( flags )  );
		
		RED_MEMORY_ASSERT( result != nullptr, "Failed to commit system pages (%d bytes)", block.size );

		if( result )
		{
			SystemBlock resultBlock = { reinterpret_cast< u64 >( ptr ), size };
			return resultBlock;
		}
		
		return NullSystemBlock();
	}
	
	void SystemAllocatorWin::OnDecommit( const SystemBlock & block )
	{
		void * ptr = reinterpret_cast< void* >( block.address );
		::VirtualFree( ptr, block.size, MEM_DECOMMIT );
	}

	u64 SystemAllocatorWin::OnGetTotalPhysicalMemoryAvailable() const
	{
		return m_totalPhysicalMemoryAvailable;
	}
}
}
