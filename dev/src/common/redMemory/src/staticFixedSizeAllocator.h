/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_STATIC_FIXED_SIZE_ALLOCATOR_H_
#define _RED_MEMORY_STATIC_FIXED_SIZE_ALLOCATOR_H_

#include "../include/utils.h"
#include "allocator.h"
#include "fixedSizeAllocator.h"

namespace red
{
namespace memory
{
	struct StaticFixedSizeAllocatorParameter
	{
		void * buffer;
		u32 bufferSize;
		u32 blockSize;
		u32 blockAlignment;
	};

	class RED_MEMORY_API StaticFixedSizeAllocator
	{
	public:

		RED_MEMORY_DECLARE_ALLOCATOR( StaticFixedSizeAllocator, 0x2528264D, 8 );

		StaticFixedSizeAllocator();
		~StaticFixedSizeAllocator();

		void Initialize( const  StaticFixedSizeAllocatorParameter & param );
		void Uninitialize();

		Block Allocate( u32 size );
		Block AllocateAligned( u32 size, u32 alignment );
		void Free( Block & block );
		Block Reallocate( Block & block, u32 size );

		bool OwnBlock( u64 block ) const; 
		u64 GetBlockSize( u64 block ) const;
		u32 GetTotalBlockCount() const;

	private:

		RED_ALIGN( 64 ) FixedSizeAllocator m_allocator;
	};

}
}

#endif
