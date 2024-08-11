/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_FLEXIBLE_SYSTEM_ALLOCATOR_H_
#define _RED_MEMORY_FLEXIBLE_SYSTEM_ALLOCATOR_H_

#include "systemAllocator.h"

namespace red
{
namespace memory
{
	class FlexibleSystemAllocator : public SystemAllocator
	{
	public:
		FlexibleSystemAllocator();
		virtual ~FlexibleSystemAllocator();

	private:

		virtual void OnInitialize() override final;
		virtual u64 OnReleaseVirtualRange( const VirtualRange & block ) override final;
		virtual SystemBlock OnCommit( const SystemBlock & block, u32 flags ) override final;
		virtual void OnDecommit( const SystemBlock & block ) override final;
		virtual u64 OnGetTotalPhysicalMemoryAvailable() const override final;
		
		u64 m_totalFlexibleMemoryAvailable;
	};
}
}

#endif
