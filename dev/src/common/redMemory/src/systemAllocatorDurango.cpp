/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "systemAllocatorDurango.h"
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

	u32 ComputePageProtectionFlags( u32 flags )
	{
		u32 result = 0;
		if( flags & Flags_GPU_Read_Write ) // CPU/GPU visibility
		{
			if( !( flags & Flags_GPU_Write ) )
			{
				result |= PAGE_GPU_READONLY; // Read Only GPU memory, not visible by CPU
			}
			
			if( flags & Flags_GPU_Coherent )
			{
				result |= PAGE_GPU_COHERENT;
			}
		}
		
		if( flags & Flags_CPU_Write )
		{
			result |= PAGE_READWRITE;
		}
		else if( flags & Flags_CPU_Read )
		{
			result |= PAGE_READONLY;
		}

		return result ? result : PAGE_NOACCESS;
	}

	u32 ComputeCommitFlags( u32 flags )
	{
		u32 result = MEM_COMMIT | MEM_LARGE_PAGES;

		if( flags & Flags_GPU_Read_Write )
		{
			result |= MEM_GRAPHICS;
		}

		return result;
	}
}

	SystemAllocatorDurango::SystemAllocatorDurango()
		: m_totalPhysicalMemoryAvailable( 0 )
	{}

	SystemAllocatorDurango::~SystemAllocatorDurango()
	{}

	void SystemAllocatorDurango::OnInitialize()
	{
		TITLEMEMORYSTATUS titleMemoryStatus;
		std::memset( &titleMemoryStatus, 0, sizeof( TITLEMEMORYSTATUS ) );
		titleMemoryStatus.dwLength = sizeof( titleMemoryStatus );
		BOOL result = TitleMemoryStatus( &titleMemoryStatus );
	
		RED_FATAL_ASSERT( result != 0, "SYSTEM ERROR cannot fetch Durango available memory." );
		RED_UNUSED( result );

		m_totalPhysicalMemoryAvailable = titleMemoryStatus.ullAvailMem;
	}

	u64 SystemAllocatorDurango::OnReleaseVirtualRange( const VirtualRange & range )
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
	
	SystemBlock SystemAllocatorDurango::OnCommit( const SystemBlock & block, u32 flags )
	{
		void * ptr = reinterpret_cast< void* >( block.address );
		const u32 protectFlags = ComputePageProtectionFlags( flags );
		const u32 commitFlags = ComputeCommitFlags( flags );
		const u64 size = RoundUp( block.size, c_durangoSystemPageSize );

		void * result = ::VirtualAlloc( ptr, size, commitFlags, protectFlags );

		RED_MEMORY_ASSERT( result != nullptr, "Failed to commit system pages (%d bytes), err code: %d", size, GetLastError() );

		if( result )
		{	
			SystemBlock resultBlock = { reinterpret_cast< u64 >( ptr ), size };
			return resultBlock;
		}
	
		return NullSystemBlock();
	}
	
	void SystemAllocatorDurango::OnDecommit( const SystemBlock & block )
	{
		void * ptr = reinterpret_cast< void* >( block.address );
		::VirtualFree( ptr, RoundUp( block.size, c_durangoSystemPageSize ), MEM_DECOMMIT );
	}

	u64 SystemAllocatorDurango::OnGetTotalPhysicalMemoryAvailable() const
	{
		return m_totalPhysicalMemoryAvailable;
	}
}
}
