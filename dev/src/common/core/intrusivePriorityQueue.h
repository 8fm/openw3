#pragma once

#if 0
	// Enables heap structure verification after each heap operation
	#define DEBUG_VERIFY_PRIORITY_QUEUE() Debug_Verify()
#else
	#define DEBUG_VERIFY_PRIORITY_QUEUE()
#endif

// Base intrusive priority queue element; to be derived from
struct TIntrusivePriorityQueueElement
{
	static const Uint32 InvalidHeapIndex = 0xFFFFFFFF;
	Uint32 m_heapIndex;	// Element index in heap; needed for random element removal
	TIntrusivePriorityQueueElement() : m_heapIndex( InvalidHeapIndex ) {}
};

/**
 *	Priority queue implemented as a standard heap.
 *
 *	Apart from standard push, get-min and pop-min also supports random element removal.
 *
 *	Stored elements must be derived from TIntrusivePriorityQueueElement.
 */
template <
	typename T,
	typename CompareFunc = DefaultCompareFunc< T >,
	EMemoryClass memoryClass = MC_DynArray,
	RED_CONTAINER_POOL_TYPE memoryPool = MemoryPool_Default >
class TIntrusivePriorityQueue
{
private:
	TDynArray< T*, memoryClass, memoryPool > m_heap;	// Heap (elements stored in a balanced binary tree)

public:
	~TIntrusivePriorityQueue();
	// Gets queue size
	Uint32 Size() const;
	// Clears the queue
	void Clear();
	// Clears the queue without memory deallocation
	void ClearFast();
	// Pushes an element onto the queue; cost: O(logN)
	void Push( T* elem );
	// Gets minimum element; cost: O(1)
	T* Front();
	// Pops minimum element; cost: O(logN)
	T* Pop();
	// Removes an element; cost: O(logN)
	void Remove( T* elem );
	// Updates an element after its been changed (wrt. to used CompareFunc); cost: O(logN)
	void Update( T* elem );

	// Verifies heap structure
	void Debug_Verify() const;
private:
	RED_FORCE_INLINE void SetElement( const Uint32 index, T* element );
	Bool MoveUp( const Uint32 index );
	Bool MoveDown( const Uint32 index );
};

template < typename T, typename CompareFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
TIntrusivePriorityQueue< T, CompareFunc, memoryClass, memoryPool>::~TIntrusivePriorityQueue()
{
	Clear();
}

template < typename T, typename CompareFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
Uint32 TIntrusivePriorityQueue< T, CompareFunc, memoryClass, memoryPool>::Size() const
{
	return m_heap.Size();
}

template < typename T, typename CompareFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void TIntrusivePriorityQueue< T, CompareFunc, memoryClass, memoryPool>::Clear()
{
	for ( T* elem : m_heap )
	{
		elem->m_heapIndex = TIntrusivePriorityQueueElement::InvalidHeapIndex;
	}
	m_heap.Clear();
}

template < typename T, typename CompareFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void TIntrusivePriorityQueue< T, CompareFunc, memoryClass, memoryPool>::ClearFast()
{
	for ( T* elem : m_heap )
	{
		elem->m_heapIndex = TIntrusivePriorityQueueElement::InvalidHeapIndex;
	}
	m_heap.ClearFast();
}

template < typename T, typename CompareFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
T* TIntrusivePriorityQueue< T, CompareFunc, memoryClass, memoryPool>::Front()
{
	return !m_heap.Empty() ? *m_heap.Begin() : nullptr;
}

template < typename T, typename CompareFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void TIntrusivePriorityQueue< T, CompareFunc, memoryClass, memoryPool>::Push( T* elem )
{
	RED_ASSERT( elem->m_heapIndex == TIntrusivePriorityQueueElement::InvalidHeapIndex );

	const Uint32 newHeapIndex = m_heap.Size();

	static_cast< TIntrusivePriorityQueueElement* >( elem )->m_heapIndex = newHeapIndex;
	m_heap.PushBack( elem );
	MoveUp( newHeapIndex );

	DEBUG_VERIFY_PRIORITY_QUEUE();
}

template < typename T, typename CompareFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
T* TIntrusivePriorityQueue< T, CompareFunc, memoryClass, memoryPool>::Pop()
{
	if ( m_heap.Empty() )
	{
		return nullptr;
	}

	T* popped = *m_heap.Begin();
	popped->m_heapIndex = TIntrusivePriorityQueueElement::InvalidHeapIndex;

	T* last = m_heap.PopBack();
	if ( !m_heap.Empty() )
	{
		SetElement( 0, last );
		MoveDown( 0 );
	}

	DEBUG_VERIFY_PRIORITY_QUEUE();
	return popped;
}

template < typename T, typename CompareFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void TIntrusivePriorityQueue< T, CompareFunc, memoryClass, memoryPool>::Remove( T* elem )
{
	RED_ASSERT( elem->m_heapIndex != TIntrusivePriorityQueueElement::InvalidHeapIndex );

	const Uint32 heapIndex = elem->m_heapIndex;
	elem->m_heapIndex = TIntrusivePriorityQueueElement::InvalidHeapIndex;

	T* last = m_heap.PopBack();
	if ( elem != last )
	{
		SetElement( heapIndex, last );
		MoveDown( heapIndex ) || MoveUp( heapIndex );
	}

	DEBUG_VERIFY_PRIORITY_QUEUE();
}

template < typename T, typename CompareFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
Bool TIntrusivePriorityQueue< T, CompareFunc, memoryClass, memoryPool>::MoveUp( const Uint32 index )
{
	T* elem = m_heap[ index ];

	Uint32 childIndex = index;
	Uint32 parentIndex = ( index - 1 ) >> 1;
	while ( childIndex > parentIndex )
	{
		T* parent = m_heap[ parentIndex ];
		if ( !CompareFunc::Less( elem, parent ) )
		{
			break;
		}

		SetElement( childIndex, parent );

		childIndex = parentIndex;
		parentIndex = ( parentIndex - 1 ) >> 1;
	}

	SetElement( childIndex, elem );

	return index != childIndex;
}

template < typename T, typename CompareFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
Bool TIntrusivePriorityQueue< T, CompareFunc, memoryClass, memoryPool>::MoveDown( const Uint32 index )
{
	T* elem = m_heap[ index ];
	const Uint32 heapSize = m_heap.Size();

	Uint32 parentIndex = index;
	Uint32 lessChildIndex = ( index << 1 ) + 1;
	while ( lessChildIndex < heapSize )
	{
		if ( lessChildIndex + 1 < heapSize && CompareFunc::Less( m_heap[ lessChildIndex + 1 ], m_heap[ lessChildIndex ] ) )
		{
			++lessChildIndex;
		}

		T* lessChild = m_heap[ lessChildIndex ];
		if ( !CompareFunc::Less( lessChild, elem ) )
		{
			break;
		}

		SetElement( parentIndex, lessChild );

		parentIndex = lessChildIndex;
		lessChildIndex = ( lessChildIndex << 1 ) + 1;
	}

	SetElement( parentIndex, elem );

	return index != parentIndex;
}

template < typename T, typename CompareFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void TIntrusivePriorityQueue< T, CompareFunc, memoryClass, memoryPool>::Update( T* elem )
{
	RED_ASSERT( elem->m_heapIndex != TIntrusivePriorityQueueElement::InvalidHeapIndex );
	RED_ASSERT( elem == m_heap[ elem->m_heapIndex ] );
	MoveDown( elem->m_heapIndex ) || MoveUp( elem->m_heapIndex );
	DEBUG_VERIFY_PRIORITY_QUEUE();
}

template < typename T, typename CompareFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void TIntrusivePriorityQueue< T, CompareFunc, memoryClass, memoryPool>::Debug_Verify() const
{
	for ( Uint32 index = 0; index < m_heap.Size(); ++index )
	{
		const T* elem = m_heap[ index ];
		RED_ASSERT( static_cast< const TIntrusivePriorityQueueElement* >( elem )->m_heapIndex == index );

		if ( index )
		{
			const Uint32 parentIndex = ( index - 1 ) >> 1;
			const T* parent = m_heap[ parentIndex ];

			RED_ASSERT( !CompareFunc::Less( elem, parent ) );
		}
	}
}

template < typename T, typename CompareFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void TIntrusivePriorityQueue< T, CompareFunc, memoryClass, memoryPool>::SetElement( const Uint32 index, T* element )
{
	static_cast< TIntrusivePriorityQueueElement* >( element )->m_heapIndex = index;
	m_heap[ index ] = element;
}