/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "hookPool.h"
#include "operators.h"
#include "hook.h"

namespace red
{
namespace memory
{
namespace 
{
	class HookImpl : public Hook
	{};
}

	HookPool::HookPool()
	{
		StaticFixedSizeAllocatorParameter param = 
		{
			m_buffer,
			sizeof( m_buffer ),
			sizeof( HookImpl ),
			__alignof( HookImpl ) 
		};

		m_allocator.Initialize( param );
	}

	Hook * HookPool::TakeHook()
	{
		Block block = m_allocator.Allocate( sizeof( HookImpl ) );
		HookImpl * hook = new( reinterpret_cast< void* >( block.address ) ) HookImpl; 
		RED_MEMORY_ASSERT( hook, "No more hook available." );
		return hook;
	}
	
	void HookPool::GiveHook( Hook * hook )
	{
		static_cast< HookImpl* >( hook )->~HookImpl();
		Block block = { AddressOf( hook ), 0 };
		m_allocator.Free( block );
	}

	u32 HookPool::GetTotalHookCount() const
	{
		return m_allocator.GetTotalBlockCount();
	}
}
}
