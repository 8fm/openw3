/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_DYNAMIC_TLSF_ALLOCATOR_H_
#define _RED_MEMORY_DYNAMIC_TLSF_ALLOCATOR_H_

#include "../include/utils.h"

#include "allocator.h"
#include "tlsfAllocator.h"
#include "block.h"

namespace red
{
namespace memory
{
	class SystemAllocator;

	struct DynamicTLSFAllocatorParameter
	{
		SystemAllocator * systemAllocator;
		u64 virtualRangeSize;
		u32 defaultSize;
		u32 chunkSize;
		u32 flags;
	};

	class RED_MEMORY_API DynamicTLSFAllocator
	{
	public:

		RED_MEMORY_DECLARE_ALLOCATOR( DynamicTLSFAllocator, 0x07A64635, 16 );

		DynamicTLSFAllocator();
		~DynamicTLSFAllocator();

		void Initialize( const DynamicTLSFAllocatorParameter & parameter );
		void Uninitialize();	

		Block Allocate( u32 size );
		Block AllocateAligned( u32 size, u32 alignment );
		Block Reallocate( Block & block, u32 size );
		void Free( Block & block );

		bool OwnBlock( u64 block ) const;
		u64 GetBlockSize( u64 address ) const;

		u64 GetSystemMemoryConsumed() const;

		void BuildMetrics( TLSFAllocatorMetrics & metrics );

	private:

		void ValidateParameter();

		void CreateNewArea( u32 minimumSize, u32 alignmemt );
		bool CanCreateNewArea() const;
		SystemBlock AllocateBlock( u32 size );

		RED_ALIGN( 64 ) TLSFAllocator m_allocator;
		
		DynamicTLSFAllocatorParameter m_parameter;
		VirtualRange m_virtualRange;
		u64 m_nextVirtualAddress;
		u64 m_memoryConsumed;
	};
}
}

#endif
