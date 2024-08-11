/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "slabHeader.h"
#include "slabConstant.h"
#include "block.h"
#include "utils.h"

namespace red
{
namespace memory
{
	void PushBlockToFreeList( u64 block, SlabHeader * header )
	{
		// TODO Is this safe enough ?! 
		IntrusiveSingleLinkedList * node = reinterpret_cast< IntrusiveSingleLinkedList *>( block );
		node->SetNext( nullptr );
		IntrusiveSingleLinkedList ** slabFreeListTail = header->freeListTail;
		
		atomic::TAtomicPtr atomicTail = atomic::ExchangePtr( 
			reinterpret_cast< atomic::TAtomicPtr* >( slabFreeListTail ),  
			reinterpret_cast< atomic::TAtomicPtr >( block ) );

		IntrusiveSingleLinkedList * tail = static_cast< IntrusiveSingleLinkedList* >( atomicTail );
		
		tail->SetNext( node );
	}

	void PushBlockToFreeList( u64 block )
	{
		SlabHeader * blockHeader = GetSlabHeader( block );
		PushBlockToFreeList( block, blockHeader );
	}

	void PushBlockToFreeList( Block & block )
	{
		SlabHeader * blockHeader = GetSlabHeader( block );
		block.size = blockHeader->blockSize;
		PushBlockToFreeList( block.address, blockHeader );
	}
}
}
