/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_HOOK_POOL_H_
#define _RED_MEMORY_HOOK_POOL_H_

#include "staticFixedSizeAllocator.h"

namespace red
{
namespace memory
{
	class Hook;

	class RED_MEMORY_API HookPool
	{
	public:

		HookPool();
		Hook * TakeHook();
		void GiveHook( Hook * hook );
		
		u32 GetTotalHookCount() const;

	private:

		StaticFixedSizeAllocator m_allocator;
		u8 m_buffer[ 512 ];			
	};
}
}

#endif
