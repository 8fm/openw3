/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "systemAllocatorOrbis.h"
#include "systemAllocatorOrbisHelper.h"
#include "utils.h"
#include "assert.h"
#include "flags.h"

namespace red
{
namespace memory
{
	SystemAllocatorOrbis::SystemAllocatorOrbis()
		: m_totalPhysicalMemoryAvailable( 0 )
	{}

	SystemAllocatorOrbis::~SystemAllocatorOrbis()
	{}

	void SystemAllocatorOrbis::OnInitialize()
	{
		off_t offset = 0;
		u64 size;

		i32 result = sceKernelAvailableDirectMemorySize(
			0,
			SCE_KERNEL_MAIN_DMEM_SIZE,
			0,
			&offset,
			&size
		);

		RED_FATAL_ASSERT( result == SCE_OK, "SYSTEM ERROR cannot fetch Orbis available memory." );
		RED_UNUSED( result );

		m_totalPhysicalMemoryAvailable = size;
	}

	u64 SystemAllocatorOrbis::OnReleaseVirtualRange( const VirtualRange & range )
	{
		return internal::DecommitRange( range );
	}
	
	SystemBlock SystemAllocatorOrbis::OnCommit( const SystemBlock & block, u32 flags )
	{
		return internal::CommitBlockToDirectMemory( block, flags );
	}
	
	void SystemAllocatorOrbis::OnDecommit( const SystemBlock & block )
	{
		internal::DecommitBlock( block );
	}

	u64 SystemAllocatorOrbis::OnGetTotalPhysicalMemoryAvailable() const
	{
		return m_totalPhysicalMemoryAvailable;
	}

}
}
