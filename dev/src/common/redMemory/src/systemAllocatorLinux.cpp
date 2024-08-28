/**
* Copyright (c) 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "systemAllocatorLinux.h"
#include "assert.h"
#include "utils.h"
#include "flags.h"

#include <sys/sysinfo.h>
#include <sys/mman.h>

namespace red
{
	namespace memory
	{
		namespace
		{
			u32 ComputePageProtectionFlags( u32 flags )
			{
				u32 result = 0;
				if ( flags & Flags_CPU_Write )
				{
					result |= PROT_READ | PROT_WRITE;
				}
				else if ( flags & Flags_CPU_Read )
				{
					result |= PROT_READ;
				}
				else
				{
					result |= PROT_NONE;
				}

				return result;
			}

		}

		SystemAllocatorLinux::SystemAllocatorLinux()
			:	m_pageSize( 0 ),
				m_totalPhysicalMemoryAvailable( 0 )
		{
			// has to use real page size as this is the allocation granularity on Linux
			const long pageSize = sysconf( _SC_PAGESIZE );
			if ( pageSize < 0 )
			{
				const Int32 err = errno;
				RED_MEMORY_HALT( "Failed to query _SC_PAGESIZE errno=0x%08X", err );
			}

			// On Linux, mmap page size and allocation granularity should be the same
			m_pageSize = pageSize;
		}

		SystemAllocatorLinux::~SystemAllocatorLinux()
		{}

		void SystemAllocatorLinux::OnInitialize()
		{
			struct sysinfo info;
			if ( sysinfo(&info) < 0 )
			{
				const Int32 err = errno;
				RED_MEMORY_HALT( "Failed to query sysinfo errno=0x%08X", err );
			}

			// not sure if total ram includes swap or not
			m_totalPhysicalMemoryAvailable = info.totalram * info.mem_unit;
		}

		u64 SystemAllocatorLinux::OnReleaseVirtualRange( const VirtualRange & range )
		{
			// TODO no sure about the equivalent of VirtualQuery on Linux

			u64 commitedMemory = 0;
			/*
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
			*/
			return commitedMemory;
		}
	
		SystemBlock SystemAllocatorLinux::OnCommit( const SystemBlock & block, u32 flags )
		{
			RED_MEMORY_ASSERT( IsAligned( block.address, m_pageSize ), " Block needs to be page aligned." );

			void * ptr = reinterpret_cast< void* >( block.address );
			const u64 size = RoundUp( block.size, m_pageSize );
			i32 result = ::mprotect( ptr, size, ComputePageProtectionFlags( flags ) );
		
			if ( result != 0 )
			{
				const Int32 err = errno;
				RED_MEMORY_HALT( "Failed to commit system pages (%d bytes) (errno=0x%08X)", size, err );
			}
			else
			{
				SystemBlock resultBlock = { reinterpret_cast< u64 >( ptr ), size };
				return resultBlock;
			}
		
			return NullSystemBlock();
		}

		SystemBlock SystemAllocatorLinux::OnCommitAligned( const SystemBlock & block, u32 flags, u32 alignment )
		{
			if ( alignment == m_pageSize )
			{
				return OnCommit( block, flags );
			}
			else
			{
				RED_MEMORY_HALT( "OnCommitAligned with alignment different that page size is not available on Linux(PC)" );
				return NullSystemBlock();
			}
		}
	
		void SystemAllocatorLinux::OnDecommit( const SystemBlock & block )
		{
			RED_MEMORY_ASSERT( IsAligned( block.address, m_pageSize ), " Block needs to be page aligned." );
			RED_MEMORY_ASSERT( block.size >= m_pageSize, " Block needs to be at least of page size." );

			void* ptr = reinterpret_cast< void* >( block.address );

			if ( ::mprotect( ptr, block.size, PROT_NONE ) < 0 )
			{
				const Int32 err = errno;
				RED_MEMORY_HALT( "Failed to call mprotect for ptr=0x%pm, size=%llu, errno=0x%08X", ptr, block.size, err );
			}
		}

		void SystemAllocatorLinux::OnPartialDecommit( const SystemBlock & block, const SystemBlock & partialBlock )
		{
			RED_MEMORY_ASSERT( partialBlock.address >= block.address && partialBlock.address + partialBlock.size <= block.address + block.size, "Partial block needs to fit in to given block." );
			(void)block;
			OnDecommit( partialBlock );
		}

		u64 SystemAllocatorLinux::OnGetTotalPhysicalMemoryAvailable() const
		{
			return m_totalPhysicalMemoryAvailable;
		}

		u64 SystemAllocatorLinux::OnGetCurrentPageMemoryAvailable() const
		{
			struct sysinfo info;
			if( sysinfo( &info ) < 0 )
			{
				return 0;
			}

			return info.freeswap;
		}

		u64 SystemAllocatorLinux::OnGetPageSize() const
		{
			return m_pageSize;
		}

		void SystemAllocatorLinux::OnWriteReportToLog() const
		{
		}

		void SystemAllocatorLinux::OnWriteReportToJson( FILE* file ) const
		{
		}
	}
}
