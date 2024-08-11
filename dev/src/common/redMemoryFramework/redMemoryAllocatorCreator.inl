/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include <new>

namespace Red { namespace MemoryFramework {

//////////////////////////////////////////////////////////////////
// Default CTor
//
RED_INLINE AllocatorCreator::AllocatorCreator()
{
	m_head = m_allocatorBuffer;
	m_tail = m_allocatorBuffer + c_maximumAllocatorBufferSize;
}

//////////////////////////////////////////////////////////////////
// Default CTor
//
RED_INLINE AllocatorCreator::~AllocatorCreator()
{
	m_tail = m_head;
}

//////////////////////////////////////////////////////////////////
// Create an allocator
//	The allocator is given a bit of static memory
template< class ALLOCATOR_TYPE >
ALLOCATOR_TYPE* AllocatorCreator::CreateAllocator()
{
	ALLOCATOR_TYPE* newAllocator = nullptr;
	if( ( m_tail - m_head ) >= sizeof( ALLOCATOR_TYPE ) )
	{
		newAllocator = new( m_head ) ALLOCATOR_TYPE();
		m_head = m_head + sizeof( ALLOCATOR_TYPE );
	}

	return newAllocator;
}

//////////////////////////////////////////////////////////////////
// Destroy an allocator
//	Since we can't free the memory, just called the destructor
template< class ALLOCATOR_TYPE >
void AllocatorCreator::DestroyAllocator( ALLOCATOR_TYPE* allocator )
{
	allocator->~ALLOCATOR_TYPE();
}

} }