/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "flexibleSystemAllocator.h"
#include "systemAllocatorOrbisHelper.h"
#include "utils.h"

namespace red
{
namespace memory
{

	FlexibleSystemAllocator::FlexibleSystemAllocator()
		: m_totalFlexibleMemoryAvailable( 0 )
	{}

	FlexibleSystemAllocator::~FlexibleSystemAllocator()
	{}
	
	void FlexibleSystemAllocator::OnInitialize()
	{
		u64 size = 0;
		i32 result = sceKernelAvailableFlexibleMemorySize( &size );

		RED_FATAL_ASSERT( result == SCE_OK, "SYSTEM ERROR cannot fetch Orbis available memory." );
		RED_UNUSED( result );
	
		m_totalFlexibleMemoryAvailable = size;
	}

	u64 FlexibleSystemAllocator::OnReleaseVirtualRange( const VirtualRange & range )
	{
		return internal::DecommitRange( range );
	}
	
	SystemBlock FlexibleSystemAllocator::OnCommit( const SystemBlock & block, u32 flags )
	{
		return internal::CommitBlockToFlexibleMemory( block, flags );
	}
	
	void FlexibleSystemAllocator::OnDecommit( const SystemBlock & block )
	{
		internal::DecommitBlock( block );
	}

	u64 FlexibleSystemAllocator::OnGetTotalPhysicalMemoryAvailable() const
	{
		return m_totalFlexibleMemoryAvailable;
	}
}
}
