/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "staticFixedSizeAllocator.h"

namespace red
{
namespace memory
{
	StaticFixedSizeAllocator::StaticFixedSizeAllocator()
	{}

	StaticFixedSizeAllocator::~StaticFixedSizeAllocator()
	{}

	void StaticFixedSizeAllocator::Initialize( const  StaticFixedSizeAllocatorParameter & param )
	{
		const u64 bufferAddress = reinterpret_cast< u64 >( param.buffer );
		VirtualRange range = { bufferAddress, bufferAddress + param.bufferSize };
		SystemBlock block = { bufferAddress, param.bufferSize };

		FixedSizeAllocatorParameter implementationParam = 
		{
			param.blockSize,
			param.blockAlignment,
			range,
			block
		};

		m_allocator.Initialize( implementationParam );
	}

	void StaticFixedSizeAllocator::Uninitialize()
	{}

	Block StaticFixedSizeAllocator::Allocate( u32 size )
	{
		return m_allocator.Allocate( size );
	}
	
	Block StaticFixedSizeAllocator::AllocateAligned( u32 size, u32 alignment )
	{
		return m_allocator.AllocateAligned( size, alignment );
	}
	
	void StaticFixedSizeAllocator::Free( Block & block )
	{
		m_allocator.Free( block );
	}
	
	Block StaticFixedSizeAllocator::Reallocate( Block & block, u32 size )
	{
		return m_allocator.Reallocate( block, size );
	}

	bool StaticFixedSizeAllocator::OwnBlock( u64 block ) const
	{
		return m_allocator.OwnBlock( block );
	}

	u64 StaticFixedSizeAllocator::GetBlockSize( u64 address ) const
	{
		RED_MEMORY_ASSERT( OwnBlock( address ), "Block is not own by this allocator." );
		return m_allocator.GetBlockSize();
	}

	u32 StaticFixedSizeAllocator::GetTotalBlockCount() const
	{
		return m_allocator.GetTotalBlockCount();
	}
}
}
