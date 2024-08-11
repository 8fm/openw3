/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "fixedSizeAllocator.h"
#include "assert.h"
#include "utils.h"

namespace red
{
namespace memory
{

	FixedSizeAllocator::FixedSizeAllocator()
		:	m_firstFree( 0 ),
			m_endAddress( 0 ),
			m_blockSize( 0 ),
			m_blockAlignment( 0 ),
			m_range()
	{}

	FixedSizeAllocator::~FixedSizeAllocator()
	{}

	void FixedSizeAllocator::Initialize( const FixedSizeAllocatorParameter & param )
	{
		RED_MEMORY_ASSERT( IsPowerOf2( param.blockAlignment ), "Alignment must be power of two." );
		RED_MEMORY_ASSERT( param.block.address, "No buffer provided." );
		RED_MEMORY_ASSERT( param.block.size, "Buffer size can't be 0." );

		m_range = param.range;
		m_blockAlignment = param.blockAlignment;

#ifdef RED_MEMORY_ENABLE_HOOKS
		// Overrun Hooks will need 8 more bytes.
		m_blockSize = RoundUp( param.blockSize + 8, m_blockAlignment );
#else
		m_blockSize = RoundUp( param.blockSize, m_blockAlignment );
#endif
		
		m_firstFree = RoundUp( param.block.address, static_cast< u64 >( m_blockAlignment ) );
		m_firstFree += 1;  // Magic bit to know if memory was previously allocate or not.
		m_endAddress = param.block.address + param.block.size;
	}
	
	void FixedSizeAllocator::Uninitialize()
	{}

	Block FixedSizeAllocator::Allocate( u32 size )
	{
		RED_MEMORY_ASSERT( size <= m_blockSize, "Cannot allocate %d byte. Fix size is %d.", size, m_blockSize );
		RED_UNUSED( size );

		u64 value = m_firstFree;
		if( value & 1 )
		{
			value -= 1;
			if( value + m_blockSize > m_endAddress )
			{
				return NullBlock();
			}

			m_firstFree += m_blockSize;
		}
		else
		{
			m_firstFree = *reinterpret_cast< u64 * >( value );
		}

		Block block = { value, m_blockSize };

		return block;
	}

	Block FixedSizeAllocator::AllocateAligned( u32 size, u32 alignment )
	{
		RED_MEMORY_ASSERT( alignment <= m_blockAlignment, "Alignment can't be more than default one." );
		RED_UNUSED( alignment );
		return Allocate( size );
	}

	void FixedSizeAllocator::Free( Block & block )
	{
		if( block.address )
		{
			RED_MEMORY_ASSERT( OwnBlock( block.address ), "Block is owned by this FixedSizeAllocator." );

			block.size = m_blockSize;
			*reinterpret_cast< u64* >( block.address ) = m_firstFree;
			m_firstFree = block.address;
		}
	}

	Block FixedSizeAllocator::Reallocate( Block & block, u32 size )
	{
		RED_MEMORY_ASSERT( size <= m_blockSize, "Cannot allocate %d byte. Fix size is %d.", size, m_blockSize );
	
		if( size == 0 )
		{
			Free( block );
			return NullBlock();
		}

		if( !block.address )
		{
			return Allocate( size );
		}
		
		block.size = m_blockSize;
		return block;
	}

	bool FixedSizeAllocator::OwnBlock( u64 block ) const
	{
		RED_MEMORY_ASSERT( IsInitialized(), "FixedSizeAllocator was not initialized." );
		return block - m_range.start < GetVirtualRangeSize( m_range ); 
	}

	bool FixedSizeAllocator::IsInitialized() const
	{
		return m_firstFree != 0;
	}

	u32 FixedSizeAllocator::GetBlockSize() const
	{
		return m_blockSize;
	}

	u32 FixedSizeAllocator::GetTotalBlockCount() const
	{
		return m_blockSize ? static_cast< u32 >( GetVirtualRangeSize( m_range ) / m_blockSize ) : 0; 
	}

	void FixedSizeAllocator::UpdateEndAddress( u64 endAddress )
	{
		m_endAddress = endAddress;
	}
}
}
