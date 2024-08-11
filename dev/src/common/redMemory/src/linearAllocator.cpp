/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "linearAllocator.h"
#include "systemAllocatorType.h"

namespace red
{
namespace memory
{
	const u64 c_linearAllocatorRangeSize = RED_GIGA_BYTE( 4 ); // ctremblay TEMP SOLUTION see linearAllocator.h
	const u32 c_linearAllocatorDefaultAlignment = 4;

	LinearAllocator::LinearAllocator()
		:	m_marker( 0 ),
			m_chunkSize( 0 ),
			m_flags( 0 ),
			m_nextVirtualAddress( 0 ),
			m_systemAllocator( &AcquireSystemAllocator() ),
			m_virtualRange( NullVirtualRange() )
	{}
	
	LinearAllocator::~LinearAllocator()
	{
		m_systemAllocator->ReleaseVirtualRange( m_virtualRange );
	}

	void LinearAllocator::Initialize( const LinearAllocatorParameter & param )
	{
		RED_MEMORY_ASSERT( param.chunkSize, "Chunk size needs to be at least 1." );
		RED_MEMORY_ASSERT( IsPowerOf2( param.chunkSize ), "Chunk size needs to be power of 2." );
		
		m_chunkSize = param.chunkSize;
		m_flags = param.flags;
		m_virtualRange = m_systemAllocator->ReserveVirtualRange( c_linearAllocatorRangeSize, m_flags );
		m_nextVirtualAddress = m_virtualRange.start;
		const SystemBlock block = AllocateBlock( m_chunkSize );
		m_marker = block.address;
	}

	SystemBlock LinearAllocator::AllocateBlock( u32 size )
	{
		SystemBlock block = { m_nextVirtualAddress, RoundUp( size, m_chunkSize ) };
		block = m_systemAllocator->Commit( block, m_flags );

		if( block.address )
		{
			m_nextVirtualAddress = block.address + block.size;
			return block;
		}

		return NullSystemBlock();
	}

	Block LinearAllocator::Allocate( u32 size )
	{
		return AllocateAligned( size, c_linearAllocatorDefaultAlignment );
	}
	
	Block LinearAllocator::AllocateAligned( u32 size, u32 alignment )
	{
		RED_MEMORY_ASSERT( m_marker, "Allocator was not Initialize." );

		alignment = std::max( alignment, c_linearAllocatorDefaultAlignment );
		const u64 address = RoundUp( m_marker, static_cast< u64 >( alignment ) );
		size = RoundUp( size, c_linearAllocatorDefaultAlignment );
		
		m_marker = address + size;
		if( m_nextVirtualAddress < m_marker )
		{
			SystemBlock block = AllocateBlock( size ); 
			if( block == NullSystemBlock() )
			{
				return NullBlock();
			}
		}
		
		const Block result = { address, size };
		return result;
	}

	void LinearAllocator::Free( Block & /*block*/ )
	{
		// Nothing to do. 
		RED_MEMORY_ASSERT( 0, "LinearAllocator do not support Free." );
	}
	
	Block LinearAllocator::Reallocate( Block & /*block*/, u32 size )
	{
		RED_MEMORY_ASSERT( 0, "LinearAllocator do not support Reallocate." );
		return NullBlock();
	}

	bool LinearAllocator::OwnBlock( u64 block ) const
	{
		return block - m_virtualRange.start < GetVirtualRangeSize( m_virtualRange ); 
	}

	void LinearAllocator::Reset()
	{
		SystemBlock decommitBlock = { m_virtualRange.start, m_nextVirtualAddress - m_virtualRange.start };
		m_systemAllocator->Decommit( decommitBlock );

		m_nextVirtualAddress = m_virtualRange.start;
		const SystemBlock block = AllocateBlock( m_chunkSize );
		m_marker = block.address;
	}

	void LinearAllocator::InternalSetSystemAllocator( SystemAllocator * allocator )
	{
		m_systemAllocator = allocator;
	}
}
}
