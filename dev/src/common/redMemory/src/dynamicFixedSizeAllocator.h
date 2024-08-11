/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_DYNAMIC_FIXED_SIZE_ALLOCATOR_H_
#define _RED_MEMORY_DYNAMIC_FIXED_SIZE_ALLOCATOR_H_

#include "../include/utils.h"

#include "allocator.h"
#include "fixedSizeAllocator.h"
#include "virtualRange.h"

namespace red
{
namespace memory
{
	class SystemAllocator;

	struct DynamicFixedSizeAllocatorParameter
	{
		SystemAllocator * systemAllocator;
		u32 blockSize;
		u32 blockAlignment;
		u32 initialBlockCount;
		u32 maxBlockCount;
		u32 flags;
	};

	class RED_MEMORY_API DynamicFixedSizeAllocator
	{
	public:

		RED_MEMORY_DECLARE_ALLOCATOR( DynamicFixedSizeAllocator, 0xE8AF4A35, 8 );

		DynamicFixedSizeAllocator();
		~DynamicFixedSizeAllocator();

		void Initialize( const DynamicFixedSizeAllocatorParameter & parameter );
		void Uninitialize();	

		Block Allocate( u32 size );
		Block AllocateAligned( u32 size, u32 alignment );
		Block Reallocate( Block & block, u32 size );
		void Free( Block & block );

		bool OwnBlock( u64 block ) const;
		u64 GetBlockSize( u64 ) const;

	private:

		bool CanCreateMoreBlock() const;
		void CreateBlocks();

		RED_ALIGN( 64 ) FixedSizeAllocator m_allocator;
		VirtualRange m_virtualRange;
		u64 m_nextVirtualAddress;
		SystemAllocator * m_systemAllocator;
		u32 m_flags;
	};
}
}

#endif
