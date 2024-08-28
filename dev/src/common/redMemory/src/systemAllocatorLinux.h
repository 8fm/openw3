/**
* Copyright (c) 2018 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_SYSTEM_ALLOCATOR_LINUX_H_
#define _RED_MEMORY_SYSTEM_ALLOCATOR_LINUX_H_

#include "systemAllocator.h"

namespace red
{
namespace memory
{
	class RED_MEMORY_API SystemAllocatorLinux : public SystemAllocator
	{
	public:

		SystemAllocatorLinux();
		virtual ~SystemAllocatorLinux();

	private:

		virtual void OnInitialize() override final;
		virtual u64 OnReleaseVirtualRange( const VirtualRange & block ) override final;
		virtual SystemBlock OnCommit( const SystemBlock & block, u32 flags ) override final;
		virtual void OnDecommit( const SystemBlock & block ) override final;
		virtual u64 OnGetTotalPhysicalMemoryAvailable() const override final;

		u64 m_pageSize;
		u64 m_totalPhysicalMemoryAvailable;
	};
}
}

#endif
