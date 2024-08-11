#pragma once

#include "bitset.h"

/// Dynamic allocator for IDs
///  - Not thread safe - wrap in something if you want to use it from threads
///  - Very fast if not fragmented
///  - ID 0 is assumed to have special meaning (no ID) and is never returned
template< Uint32 MaxObjects >
class IDAllocator
{
public:
	IDAllocator();

	// reset the allocator - release all IDs
	void Reset();

	// allocate new ID, will return 0 when full
	Uint32 Alloc();

	// release allocated ID
	void Release( const Uint32 id );

	// get number of allocated IDs
	RED_FORCE_INLINE  const Uint32 GetNumAllocated() const { return m_numAllocated; }

	// get the capacity of the allocator
	RED_FORCE_INLINE  const Uint32 GetCapacity() const { return MaxObjects; }

	// is full ?
	RED_FORCE_INLINE  const Bool IsFull() const { return (m_numAllocated == MaxObjects); }

private:
	TBitSet64< MaxObjects >		m_freeIndices;
	Uint32						m_firstFreeIndex;
	Uint32						m_searchIndex;
	Uint32						m_numAllocated;
};

/// Dynamic allocator for IDs
///  - Not thread safe - wrap in something if you want to use it from threads
///  - Very fast if not fragmented
///  - ID 0 is assumed to have special meaning (no ID) and is never returned
class IDAllocatorDynamic
{
public:
	IDAllocatorDynamic();
	
	// prepare cache for use with given maximum object count
	void Resize( const Uint32 maxObjects );

	// reset the allocator - release all IDs
	void Reset();

	// allocate new ID, will return 0 when full
	Uint32 Alloc();

	// release allocated ID
	void Release( const Uint32 id );

	// get number of allocated IDs
	RED_FORCE_INLINE const Uint32 GetNumAllocated() const { return m_numAllocated; }

	// get the capacity of the allocator
	RED_FORCE_INLINE const Uint32 GetCapacity() const { return m_freeIndices.GetNumEntries(); }

	// is full ?
	RED_FORCE_INLINE const Bool IsFull() const { return (m_numAllocated == m_freeIndices.GetNumEntries()); }

private:
	BitSet64Dynamic		m_freeIndices;
	Uint32				m_firstFreeIndex;
	Uint32				m_searchIndex;
	Uint32				m_numAllocated;
};

#include "idAllocator.inl"