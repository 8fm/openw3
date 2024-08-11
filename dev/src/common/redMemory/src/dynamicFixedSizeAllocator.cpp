/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dynamicFixedSizeAllocator.h"
#include "systemAllocator.h"
#include "utils.h"
#include "flags.h"

namespace red
{
namespace memory
{
	void ValidateInitializationParameter( const DynamicFixedSizeAllocatorParameter & parameter  )
	{
		RED_MEMORY_ASSERT( parameter.systemAllocator, "Allocator need access to SystemAllocator." );
		RED_MEMORY_ASSERT( parameter.blockSize, "Block size can't be 0." );
		RED_MEMORY_ASSERT( parameter.blockAlignment, "Block alignment can't be 0." );
		RED_MEMORY_ASSERT( IsPowerOf2( parameter.blockAlignment ), "Block alignment need to be power of 2." );
		RED_MEMORY_ASSERT( parameter.initialBlockCount <= parameter.maxBlockCount, "Max Block Count can't be bigger than initial block count." );
		RED_UNUSED( parameter );
	}

	DynamicFixedSizeAllocator::DynamicFixedSizeAllocator()
		:	m_nextVirtualAddress( 0 ),
			m_systemAllocator( nullptr ),
			m_flags( Flags_CPU_Read_Write )
	{}

	DynamicFixedSizeAllocator::~DynamicFixedSizeAllocator()
	{}

	void DynamicFixedSizeAllocator::Initialize( const DynamicFixedSizeAllocatorParameter & parameter )
	{
		ValidateInitializationParameter( parameter );

		m_systemAllocator = parameter.systemAllocator;
		m_flags = parameter.flags;
		const u64 rangeSize = parameter.blockSize * parameter.maxBlockCount;
		m_virtualRange = m_systemAllocator->ReserveVirtualRange( RoundUp( rangeSize, c_systemBlockSizeFactor ), m_flags );
		m_nextVirtualAddress = m_virtualRange.start;

		SystemBlock block = { m_nextVirtualAddress, parameter.blockSize * parameter.initialBlockCount };
		block = m_systemAllocator->Commit( block, m_flags );
		m_nextVirtualAddress = block.address + block.size;

		FixedSizeAllocatorParameter fixedSizeParam = 
		{
			parameter.blockSize,
			parameter.blockAlignment,
			m_virtualRange,
			block
		};

		m_allocator.Initialize( fixedSizeParam );
	}

	void DynamicFixedSizeAllocator::Uninitialize()
	{
		m_systemAllocator->ReleaseVirtualRange( m_virtualRange );
	}

	Block DynamicFixedSizeAllocator::Allocate( u32 size )
	{
		Block block = m_allocator.Allocate( size );

		if( !block.address && CanCreateMoreBlock() )
		{
			CreateBlocks();
			block = m_allocator.Allocate( size );
		}

		return block; 
	}
	
	Block DynamicFixedSizeAllocator::AllocateAligned( u32 size, u32 alignment )
	{
		Block block = m_allocator.AllocateAligned( size, alignment );

		if( !block.address && CanCreateMoreBlock() )
		{
			CreateBlocks();
			block = m_allocator.AllocateAligned( size, alignment );
		}

		return block;
	}
	
	Block DynamicFixedSizeAllocator::Reallocate( Block & block, u32 size )
	{
		Block result = m_allocator.Reallocate( block, size );
		
		if( size && result.address == 0 )
		{
			return Allocate( size );
		}

		return result;
	}
	
	void DynamicFixedSizeAllocator::Free( Block & block )
	{
		m_allocator.Free( block );
	}

	bool DynamicFixedSizeAllocator::OwnBlock( u64 block ) const
	{
		return m_allocator.OwnBlock( block );
	}

	u64 DynamicFixedSizeAllocator::GetBlockSize( u64 address ) const
	{
		RED_MEMORY_ASSERT( OwnBlock( address ), "Block is not own by this allocator." );
		return m_allocator.GetBlockSize();
	}

	bool DynamicFixedSizeAllocator::CanCreateMoreBlock() const
	{
		return m_nextVirtualAddress < m_virtualRange.end;
	}

	void DynamicFixedSizeAllocator::CreateBlocks()
	{
		const u32 blockSize = m_allocator.GetBlockSize();
		SystemBlock block = { m_nextVirtualAddress, RoundUp( static_cast< u64 >( blockSize ), c_systemBlockSizeFactor ) };
		block = m_systemAllocator->Commit( block, m_flags );
		if( block.address )
		{
			m_nextVirtualAddress = block.address + block.size;
			m_allocator.UpdateEndAddress( m_nextVirtualAddress );
		}	
	}
}
}
