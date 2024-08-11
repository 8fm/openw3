/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_LINEAR_ALLOCATOR_H_
#define _RED_MEMORY_LINEAR_ALLOCATOR_H_

#include "allocator.h"
#include "block.h"
#include "virtualRange.h"
#include "systemBlock.h"

namespace red
{
namespace memory
{
	class SystemAllocator;

	struct LinearAllocatorParameter
	{
		u32 chunkSize;
		u32 flags;
	};

	class RED_MEMORY_API LinearAllocator
	{
	public:

		RED_MEMORY_DECLARE_ALLOCATOR( LinearAllocator, 0x4784F1A2, 4 );

		LinearAllocator();
		~LinearAllocator();

		void Initialize( const LinearAllocatorParameter & param );

		Block Allocate( u32 size );
		Block AllocateAligned( u32 size, u32 alignment );
		void Free( Block & block );
		Block Reallocate( Block & block, u32 size );

		bool OwnBlock( u64 block ) const; 

		void Reset();

		void InternalSetSystemAllocator( SystemAllocator * allocator );

	private:

		void CreateNewArea( u32 minimumSize, u32 alignment );

		// ctremblay TEMP SOLUTION. For now it takes Chunk from SystemAllocator. 
		// Another Allocator need to be used. One solution could be using a FixedSizeAllocator.
		// FixedSizeAllocator could act as a PageAllocator.
		SystemBlock AllocateBlock( u32 size );

		u64 m_marker;;
		u32 m_chunkSize;
		u32 m_flags;
		u64 m_nextVirtualAddress;
		SystemAllocator * m_systemAllocator;
		VirtualRange m_virtualRange;
	};
}
}

#endif
