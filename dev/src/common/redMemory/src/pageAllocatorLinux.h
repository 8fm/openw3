/**
* Copyright (c) 2018 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_PAGE_ALLOCATOR_LINUX_H_
#define _RED_MEMORY_PAGE_ALLOCATOR_LINUX_H_

#include "pageAllocator.h"

namespace red
{
namespace memory
{
	class RED_MEMORY_API PageAllocatorLinux : public PageAllocator
	{
	public:
		PageAllocatorLinux();
		virtual ~PageAllocatorLinux();

	private:

		virtual void OnInitialize() override final;
		virtual VirtualRange OnReserveRange( u64 size, u32 pageSize, u32 flags ) override final; 
		virtual void OnReleaseRange( const VirtualRange & range ) override final;
	};
}
}

#endif
