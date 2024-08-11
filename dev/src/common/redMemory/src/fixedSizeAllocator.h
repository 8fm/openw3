/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_FIXED_SIZE_ALLOCATOR_H_
#define _RED_MEMORY_FIXED_SIZE_ALLOCATOR_H_

#include "virtualRange.h"
#include "systemBlock.h"
#include "block.h"

namespace red
{
namespace memory
{
	struct FixedSizeAllocatorParameter
	{
		u32 blockSize;
		u32 blockAlignment;
		VirtualRange range;
		SystemBlock block;
	};

	class RED_MEMORY_API FixedSizeAllocator
	{
	public:

		FixedSizeAllocator();
		~FixedSizeAllocator();

		void Initialize( const FixedSizeAllocatorParameter & param );
		void Uninitialize();

		Block Allocate( u32 size );
		Block AllocateAligned( u32 size, u32 alignment );
		void Free( Block & block );
		Block Reallocate( Block & block, u32 size );

		bool OwnBlock( u64 block ) const;

		u32 GetBlockSize() const;
		u32 GetTotalBlockCount() const;

		void UpdateEndAddress( u64 endAddress );

	private:

		FixedSizeAllocator( const FixedSizeAllocator& );
		FixedSizeAllocator & operator=( const FixedSizeAllocator& );

		bool IsInitialized() const;

		u64 m_firstFree;
		u64 m_endAddress;
		u32 m_blockSize;
		u32 m_blockAlignment;
		VirtualRange m_range;
	};
}
}

#endif
