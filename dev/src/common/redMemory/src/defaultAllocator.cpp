/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "defaultAllocator.h"
#include "assert.h"
#include "slabHeader.h"
#include "systemAllocator.h"
#include "threadMonitor.h"
#include "tlsfBlock.h"
#include "vault.h"
#include "flags.h"

namespace red
{
namespace memory
{
	const u64 c_defaultAllocatorSlabVirtualRange = RED_GIGA_BYTE( 8 );
	const u64 c_defaultAllocatorTLSFVirtualRange = RED_GIGA_BYTE( 32 );
	const u32 c_defaultAllocatorTLSFInitialAllocSize =  RED_GIGA_BYTE( 1 ) + RED_MEGA_BYTE( 576 );
	const u32 c_defaultAllocatorTLSFChunkSize = RED_MEGA_BYTE( 4 );

	DefaultAllocator::DefaultAllocator()
		: m_systemAllocator( nullptr )
	{}

	DefaultAllocator::~DefaultAllocator()
	{}

	void DefaultAllocator::Initialize( const DefaultAllocatorParameter & parameter )
	{
		m_systemAllocator = parameter.systemAllocator;

		RED_MEMORY_ASSERT( m_systemAllocator, "DefaultAllocator cannot be initialize. Need access to SytemAllocator." );
	
		LocklessSlabAllocatorParameter slabParameter = 
		{
			c_defaultAllocatorSlabVirtualRange,
			m_systemAllocator,
			parameter.threadMonitor
		};

		m_slabAllocator.Initialize( slabParameter );
	
		DynamicTLSFAllocatorParameter tlsfParameter = 
		{
			m_systemAllocator,
			c_defaultAllocatorTLSFVirtualRange,
			c_defaultAllocatorTLSFInitialAllocSize,
			c_defaultAllocatorTLSFChunkSize,
			Flags_CPU_Read_Write
		};

		m_tlsfAllocator.Initialize( tlsfParameter );
	}

	void DefaultAllocator::Uninitialize()
	{
		m_tlsfAllocator.Uninitialize();
		m_slabAllocator.Uninitialize();
	}

	Block DefaultAllocator::Allocate( u32 size )
	{
		RED_MEMORY_ASSERT( IsInitialized(), "DefaultAllocator was not initialized." );

		Block block = NullBlock();
		
		if( size <= c_slabMaxAllocSize )
		{
			block = m_slabAllocator.Allocate( size );
		}
		
		if( !block.address )
		{
			block = m_tlsfAllocator.Allocate( size );
		}

		return block;
	}

	Block DefaultAllocator::AllocateAligned( u32 size, u32 alignment )
	{
		RED_MEMORY_ASSERT( IsInitialized(), "DefaultAllocator was not initialized." );

		Block block = NullBlock(); 

		if( size <= c_slabMaxAllocSize && alignment <= 16 )
		{
			block = m_slabAllocator.AllocateAligned( size, alignment );
		}

		if( !block.address )
		{
			block = m_tlsfAllocator.AllocateAligned( size, alignment );
		}

		return block;
	}

	Block DefaultAllocator::Reallocate( Block & block, u32 size )
	{
		if( size == 0 )
		{
			Free( block );
			return NullBlock();
		}

		if( block.address == 0 )
		{
			return Allocate( size );
		}

		RED_MEMORY_ASSERT( OwnBlock( block.address ), "DefaultAllocator do not own memory block." );

		return m_slabAllocator.OwnBlock( block.address ) ?
			ReallocateFromSlabAllocator( block, size ) :
			ReallocateFromTLSFAllocator( block, size );
	}

	Block DefaultAllocator::ReallocateFromSlabAllocator( Block & block, u32 size )
	{
		Block newBlock = NullBlock();

		if( size <= c_slabMaxAllocSize )
		{
			newBlock = m_slabAllocator.Reallocate( block, size );
		}

		if( !newBlock.address )
		{	
			newBlock = m_tlsfAllocator.Allocate( size );
			if( newBlock.address )
			{
				MemcpyBlock( newBlock.address, block.address, GetSlabBlockSize( block.address ) );
			}

			m_slabAllocator.Free( block );
		}

		return newBlock;
	}

	Block DefaultAllocator::ReallocateFromTLSFAllocator( Block & block, u32 size )
	{
		Block newBlock = NullBlock();

		if( size <= c_slabMaxAllocSize )
		{
			newBlock = Allocate( size );
			if( newBlock.address )
			{
				MemcpyBlock( newBlock.address, block.address, std::min( size, GetTLSFBlockSize( block.address ) ) );
			}

			m_tlsfAllocator.Free( block );
		}
		else
		{
			newBlock = m_tlsfAllocator.Reallocate( block, size );
		}

		return newBlock;
	}

	void DefaultAllocator::Free( Block & block )
	{
		RED_MEMORY_ASSERT( !block.address || OwnBlock( block.address ), "Memory Block is not own by DefaultAllocator." );
		if( m_slabAllocator.OwnBlock( block.address ) )
		{
			m_slabAllocator.Free( block );
		}
		else
		{
			m_tlsfAllocator.Free( block );
		}
	}

	void DefaultAllocator::RegisterCurrentThread()
	{
		m_slabAllocator.RegisterCurrentThread();
	}

	void DefaultAllocator::BuildMetrics( DefaultAllocatorMetrics & metrics )
	{
		AllocatorMetrics & defaultMetric = metrics.metrics;
		LocklessSlabAllocatorMetrics & slabMetric = metrics.locklessSlabAllocatorMetrics;
		TLSFAllocatorMetrics & tlsfMetric = metrics.tlsfAllocatorMetrics;

		m_slabAllocator.BuildMetrics( slabMetric );
		m_tlsfAllocator.BuildMetrics( metrics.tlsfAllocatorMetrics );
		
		defaultMetric.consumedSystemMemoryBytes = slabMetric.metrics.consumedSystemMemoryBytes + tlsfMetric.metrics.consumedSystemMemoryBytes;
		defaultMetric.consumedMemoryBytes = slabMetric.metrics.consumedMemoryBytes + tlsfMetric.metrics.consumedMemoryBytes;
		defaultMetric.bookKeepingBytes = slabMetric.metrics.bookKeepingBytes + tlsfMetric.metrics.bookKeepingBytes;
	}

	bool DefaultAllocator::IsInitialized() const
	{
		return m_systemAllocator != nullptr;
	}

	bool DefaultAllocator::InternalSlabAllocatorOwnBlock( const Block & block ) const
	{
		return m_slabAllocator.OwnBlock( block.address );
	}

	bool DefaultAllocator::InternalTLSFAllocatorOwnBlock( const Block & block ) const
	{
		return m_tlsfAllocator.OwnBlock( block.address );
	}
	
	void DefaultAllocator::InternalMarkSlabAllocatorAsFull()
	{
		// No local slab allocator available is same behavior. I.E. it will return nullptr
		m_slabAllocator.InternalMarkAllLocalSlabAllocatorAsTaken();
	}

	DefaultAllocator & AcquireDefaultAllocator()
	{
		return AcquireVault().GetDefaultAllocator();
	}
}
}
