/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dynamicTlsfAllocator.h"
#include "systemAllocator.h"
#include "assert.h"
#include "utils.h"
#include "tlsfConstant.h"
#include "tlsfBlock.h"

#include <xutility>

namespace red
{
namespace memory
{
	const u32 c_sizeFactor = RED_KILO_BYTE( 64 );

	DynamicTLSFAllocator::DynamicTLSFAllocator()
		:	m_nextVirtualAddress( 0 ),
			m_memoryConsumed( 0 )
	{}

	DynamicTLSFAllocator::~DynamicTLSFAllocator()
	{}

	void DynamicTLSFAllocator::Initialize( const DynamicTLSFAllocatorParameter & parameter )
	{
		m_parameter = parameter;
		
		ValidateParameter();
		
		m_virtualRange = m_parameter.systemAllocator->ReserveVirtualRange( m_parameter.virtualRangeSize, m_parameter.flags );
		m_nextVirtualAddress = m_virtualRange.start;
		
		SystemBlock firstBlock = AllocateBlock( m_parameter.defaultSize );
	
		TLSFAllocatorParameter implementationParameter = 
		{
			m_virtualRange,
			firstBlock
		};
		
		m_allocator.Initialize( implementationParameter );
		m_memoryConsumed += static_cast< u32 >( firstBlock.size );
	}

	SystemBlock DynamicTLSFAllocator::AllocateBlock( u32 size )
	{
		SystemBlock block = { m_nextVirtualAddress, std::min( static_cast< u64 >( size ), m_virtualRange.end - m_nextVirtualAddress ) };
		block = m_parameter.systemAllocator->Commit( block, m_parameter.flags );

		if( block.address )
		{
			m_nextVirtualAddress = block.address + block.size;
		}

		return block;
	}

	void DynamicTLSFAllocator::Uninitialize()
	{
		m_parameter.systemAllocator->ReleaseVirtualRange( m_virtualRange );
	}

	Block DynamicTLSFAllocator::Allocate( u32 size )
	{		 
		return AllocateAligned( size, c_tlsfDefaultAlignment );
	}

	Block DynamicTLSFAllocator::AllocateAligned( u32 size, u32 alignment )
	{
		Block block = m_allocator.AllocateAligned( size, alignment );

		if( !block.address && CanCreateNewArea() )
		{
			CreateNewArea( size, alignment );
			block = m_allocator.AllocateAligned( size, alignment );
		}

		return block;
	}

	Block DynamicTLSFAllocator::Reallocate( Block & block, u32 size )
	{
		if( size != 0 )
		{
			Block reallocatedBlock = m_allocator.Reallocate( block, size );
			if( !reallocatedBlock.address && CanCreateNewArea() )
			{
				CreateNewArea( size, c_tlsfDefaultAlignment );
				reallocatedBlock = m_allocator.Reallocate( block, size );
			}

			return reallocatedBlock;
		}
		else
		{
			m_allocator.Free( block );
			return NullBlock();
		}
	}

	bool DynamicTLSFAllocator::CanCreateNewArea() const
	{
		return m_nextVirtualAddress != m_virtualRange.end;
	}

	void DynamicTLSFAllocator::CreateNewArea( u32 minimumSize, u32 alignment )
	{
		u32 areaSize = m_allocator.ComputeMinimumAreaSize( minimumSize, alignment );
		areaSize = RoundUp( areaSize, m_parameter.chunkSize );

		SystemBlock areaBlock = AllocateBlock( areaSize );
		if( areaBlock != NullSystemBlock() )
		{
			m_allocator.AddArea( areaBlock );
			m_memoryConsumed += areaSize;
		}
	}

	void DynamicTLSFAllocator::Free( Block & block )
	{
		m_allocator.Free( block );
	}

	bool DynamicTLSFAllocator::OwnBlock( u64 block ) const
	{
		return m_allocator.OwnBlock( block );
	}

	u64 DynamicTLSFAllocator::GetBlockSize( u64 address ) const
	{
		RED_MEMORY_ASSERT( OwnBlock( address ), "Block is not own by this allocator." );
		return GetTLSFBlockSize( address );
	}

	u64 DynamicTLSFAllocator::GetSystemMemoryConsumed() const
	{
		return m_memoryConsumed;
	}

	void DynamicTLSFAllocator::BuildMetrics( TLSFAllocatorMetrics & metrics )
	{
		metrics.metrics.consumedSystemMemoryBytes = m_memoryConsumed;
		m_allocator.BuildMetrics( metrics );
	}

	void DynamicTLSFAllocator::ValidateParameter()
	{
		RED_MEMORY_ASSERT( m_parameter.systemAllocator, "DynamicTLSFAllocator require access to System Allocator." );
		RED_MEMORY_ASSERT( IsAligned( m_parameter.chunkSize, c_sizeFactor), "ChunkSize must be a factor of 64k," );
		RED_MEMORY_ASSERT( IsAligned( m_parameter.defaultSize, c_sizeFactor ), "Default size must be a factor of 64k." );
		RED_UNUSED( c_sizeFactor );
	}
}
}
