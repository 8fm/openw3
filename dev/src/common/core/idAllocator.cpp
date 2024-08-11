#include "build.h"
#include "idAllocator.h"

IDAllocatorDynamic::IDAllocatorDynamic()
	: m_firstFreeIndex( 0 ) 
	, m_searchIndex( 0 )
	, m_numAllocated( 0 )
{
}

void IDAllocatorDynamic::Resize( const Uint32 maxObjects )
{
	if ( maxObjects > m_freeIndices.GetNumEntries() )
	{
		const Uint32 prevSize = m_freeIndices.GetNumEntries();
		m_freeIndices.Resize( maxObjects );

		// set the new entries to 1
		const Uint32 curSize = m_freeIndices.GetNumEntries();
		for ( Uint32 i=prevSize; i<curSize; ++i )
		{
			m_freeIndices.Set(i);
		}
	}
}

void IDAllocatorDynamic::Reset()
{
	m_numAllocated = 1;
	m_freeIndices.SetAll();
	m_firstFreeIndex = 1;
	m_searchIndex = 1;
}

Uint32 IDAllocatorDynamic::Alloc()
{
	// try fast allocation
	const Uint32 maxIndex = m_freeIndices.GetNumEntries();
	if ( m_firstFreeIndex < maxIndex )
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
	if ( m_searchIndex ==  maxIndex )
		m_searchIndex = 0;

	// search in a linear fashion starting from last allocated slot
	// NOTE: this is fast due to the CPU prefetcher
	Uint32 firstFreeIndex = m_freeIndices.FindNextSet( m_searchIndex );
	if ( firstFreeIndex == maxIndex )
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

void IDAllocatorDynamic::Release( const Uint32 id )
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
