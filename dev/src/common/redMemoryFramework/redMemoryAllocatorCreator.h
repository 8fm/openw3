/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_ALLOCATOR_CREATOR_H
#define _RED_MEMORY_ALLOCATOR_CREATOR_H

#include "redMemoryFrameworkTypes.h"

namespace Red { namespace MemoryFramework {

//////////////////////////////////////////////////////////////////////
// This class acts as both an allocator factory, as well as a very simple linear allocator
class AllocatorCreator
{
public:
	AllocatorCreator();
	~AllocatorCreator();

	template< class ALLOCATOR_TYPE >
	ALLOCATOR_TYPE* CreateAllocator();

	template< class ALLOCATOR_TYPE >
	void DestroyAllocator( ALLOCATOR_TYPE* allocator );

private:
	static const Red::System::MemSize c_maximumAllocatorBufferSize = 1024 * 2;		// 2k buffer for allocator objects
	Red::System::Int8 m_allocatorBuffer[ c_maximumAllocatorBufferSize ];
	Red::System::Int8* m_head;
	Red::System::Int8* m_tail;
};

} }

#include "redMemoryAllocatorCreator.inl"

#endif