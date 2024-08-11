/************************************************************************/
/* Red Memory Framework Setup											*/
/************************************************************************/

#ifndef __ID_ALLOCATOR_H_INL__
#define __ID_ALLOCATOR_H_INL__

//--------------------------

template< Uint32 MaxObjects >
RED_INLINE IDAllocator< MaxObjects >::IDAllocator()
{
	Reset();
}

template< Uint32 MaxObjects >
RED_INLINE void IDAllocator< MaxObjects >::Reset()
{
	m_numAllocated = 1;
	m_freeIndices.SetAll();
	m_firstFreeIndex = 1;
	m_searchIndex = 1;
}

template< Uint32 MaxObjects >
Uint32 IDAllocator< MaxObjects >::Alloc()
{
	// try fast allocation
	if ( m_firstFreeIndex < MaxObjects )
	{
		const Uint32 id = m_firstFreeIndex++;
		RED_FATAL_ASSERT( m_freeIndices.Get( id ), "Free index is not so free. Possible memory corruption. Stoping for safety." );
		m_freeIndices.Clear( id );
		m_numAllocated += 1;
		return id;
	}

	// full - don't waste time looking for the index
	if ( IsFull() )
		return 0;

	// wrapper around - restart the search
	if ( m_searchIndex == MaxObjects )
		m_searchIndex = 0;

	// search in a linear fashion starting from last allocated slot
	// NOTE: this is fast due to the CPU prefetcher
	Uint32 firstFreeIndex = m_freeIndices.FindNextSet( m_searchIndex );
	if ( firstFreeIndex == MaxObjects )
	{
		RED_FATAL_ASSERT( IsFull(), "Bit set is not full and yet we cannot allocate stuff. Capacity=%d, Allocated=%d, SerachStart=%d", GetCapacity(), GetNumAllocated(), m_searchIndex );
		return 0;
	}

	// mark the index found as allocated
	m_searchIndex = firstFreeIndex + 1;
	m_freeIndices.Clear( firstFreeIndex );
	m_numAllocated += 1;

	// return allocated free index
	return firstFreeIndex;
}

template< Uint32 MaxObjects >
void IDAllocator< MaxObjects >::Release( const Uint32 id )
{
	// zero should not be release
	if ( !id )
		return;

	// trying to free index that's already freed
	RED_FATAL_ASSERT( id < m_firstFreeIndex, "Trying to free index that was not yet allocated. Possible memory corruption. Stoping for safety." );
	RED_FATAL_ASSERT( !m_freeIndices.Get( id ), "Trying to free index that is already freed. Possible memory corruption. Stoping for safety." );

	// mark index as freed
	m_freeIndices.Set( id );
	m_numAllocated -= 1;

	// extend the search region
	if ( id < m_searchIndex )
		m_searchIndex = id;
}

//--------------------------

#endif