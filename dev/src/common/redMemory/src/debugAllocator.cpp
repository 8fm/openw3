/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "debugAllocator.h"
#include "debugAllocatorConstant.h"
#include "utils.h"
#include "../../redSystem/unitTestMode.h"
#include <xutility>

namespace red
{
namespace memory
{
namespace 
{
	void MarkAllocatedBlock( const Block & block )
	{
#ifdef RED_MEMORY_UNIT_TEST
		if( UnitTestMode() )
		{
			MemsetBlock( block, c_debugAllocatorUnitTestAllocFiller );
		}
#endif
		RED_UNUSED( block );
	}

	void MarkFreeBlock( const Block & block )
	{
#ifdef RED_MEMORY_UNIT_TEST
		if( UnitTestMode() )
		{
			MemsetBlock( block, c_debugAllocatorUnitTestFreeFiller );
		}
#endif
		RED_UNUSED( block );
	} 
}

	DebugAllocator::DebugAllocator()
	{}

	DebugAllocator::~DebugAllocator()
	{}

	Block DebugAllocator::Allocate( u32 size )
	{
		return AllocateAligned( size, c_debugAllocatorDefaultAlignment );
	}
	
	Block DebugAllocator::AllocateAligned( u32 size, u32 alignment )
	{
		alignment = std::max( c_debugAllocatorDefaultAlignment, alignment ); 
		void * ptr = _aligned_malloc( size, alignment );
		Block block =
		{
			AddressOf( ptr ),
			static_cast< u32 >( _aligned_msize( ptr, alignment, 0 ) )
		};

		MarkAllocatedBlock( block );

		return block;
	}
	
	Block DebugAllocator::Reallocate( Block & block, u32 size )
	{
		if( block.address == 0 )
		{
			return size != 0 ? AllocateAligned( size, c_debugAllocatorDefaultAlignment ) : NullBlock();
		}

		if( size == 0 )
		{
			Free( block );
			return NullBlock();
		}

		void * ptr = reinterpret_cast< void * >( block.address );

		u64 oldSize = _aligned_msize( ptr, c_debugAllocatorDefaultAlignment, 0 );
		block.size = oldSize;

		Block newBlock = AllocateAligned( size, c_debugAllocatorDefaultAlignment );
		if( newBlock.address )
		{
			MemcpyBlock( newBlock, block );
		}

		Free( block );

		return newBlock;
	}
	
	void DebugAllocator::Free( Block & block )
	{
		if( block.address )
		{
			void * ptr = reinterpret_cast< void* >( block.address );
			block.size = static_cast< u32 >( _aligned_msize( ptr, c_debugAllocatorDefaultAlignment, 0 ) );
			
			MarkFreeBlock( block );
			
			_aligned_free( ptr );
		}	
	}

	bool DebugAllocator::OwnBlock( u64 block ) const
	{
		// NOTE I can't know for sure I'm not owning the block except if it's null.
		if( block )
		{
			return true;
		}
		
		return false;
	}

	u64 DebugAllocator::GetBlockSize( u64 address ) const
	{
		void * ptr = reinterpret_cast< void* >( address );
		return _aligned_msize( ptr, c_debugAllocatorDefaultAlignment, 0 );
	}

	DebugAllocator & AcquireDebugAllocator()
	{
		static DebugAllocator allocator;
		return allocator;
	}
}
}
