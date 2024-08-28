/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "debugAllocator.h"
#include "debugAllocatorConstant.h"
#include "utils.h"
#include "../../redSystem/unitTestMode.h"
#ifdef RED_PLATFORM_LINUX
#include <malloc.h> // for malloc_usable_size()
#else
#include <xutility>
#endif

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

	const u32 c_offset = sizeof( void* );

	void* AlignedAlloc( u32 size, u32 alignment )
	{
		RED_MEMORY_ASSERT( IsPowerOf2( alignment ), "Alignment must be power of 2." );

		size += alignment - 1 + c_offset;
		void* ptr = std::malloc( size );
		if ( ptr )
		{
			u64 userPtr = red::memory::AlignAddress( reinterpret_cast< u64 >( ptr ) + c_offset, alignment );
			RED_MEMORY_ASSERT( IsAligned( userPtr, alignment ), "Allocated memory is not properly aligned" );

			// store the address of the actual allocation
			*reinterpret_cast< void** >( userPtr - c_offset ) = ptr;

			return reinterpret_cast< void* >( userPtr );
		}

		return nullptr;
	}

	void AlignedFree( u64 address )
	{
		RED_MEMORY_ASSERT( address > c_offset, "Given address is invalid." );
		void* ptr = *reinterpret_cast< void** >( address - c_offset );
		std::free( ptr );
	}

	u32 GetAllocatedMemorySize( u64 address )
	{
		RED_MEMORY_ASSERT( address > c_offset, "Given address is invalid." );

		void* ptr = *reinterpret_cast< void** >( address - c_offset );
		const u32 diff = static_cast< u32 >( address - reinterpret_cast< u64 >( ptr ) );
#if defined( RED_COMPILER_CLANG )
		size_t blockSize = malloc_usable_size( ptr );
#else
		size_t blockSize = _msize( ptr );
#endif
		return static_cast< u32 >( blockSize ) - diff;
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
		void * ptr = AlignedAlloc( size, alignment );
		const u64 address = AddressOf( ptr );
		Block block =
		{
			address,
			GetBlockSize( address )
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

		u64 oldSize = GetBlockSize( block.address );
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
			block.size = GetBlockSize( block.address );
			
			MarkFreeBlock( block );
			
			AlignedFree( block.address );
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
		return GetAllocatedMemorySize( address );
	}

	DebugAllocator & AcquireDebugAllocator()
	{
		static DebugAllocator allocator;
		return allocator;
	}
}
}
