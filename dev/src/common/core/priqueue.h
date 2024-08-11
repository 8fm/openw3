/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

template <
		   typename K,
		   typename CompareFunc = DefaultCompareFunc< K >
		 >
class TPriQueue
{
public:
	typedef K			key_type;
	typedef CompareFunc compare_func;

protected:
	TDynArray< K >		m_keyArray;

public:
	RED_INLINE TPriQueue()
	{
	}
	RED_INLINE TPriQueue( const TPriQueue& queue )
	{
		*this = queue;
	}
	RED_INLINE const TPriQueue& operator=( const TPriQueue& queue )
	{
		m_keyArray = queue.m_keyArray;
		return *this;
	}
	RED_INLINE ~TPriQueue()
	{
	}

	RED_INLINE Uint32 Size() const
	{
		return m_keyArray.Size();
	}
	RED_INLINE Uint32 DataSize() const
	{
		return m_keyArray.DataSize();
	}
	RED_INLINE Uint32 Capacity() const
	{
		return m_keyArray.Capacity();
	}
	RED_INLINE bool Empty() const
	{
		return m_keyArray.Size() == 0;
	}
	RED_INLINE void Shrink()
	{
		m_keyArray.Shrink();
	}
	RED_INLINE void Clear()
	{
		m_keyArray.Clear();
	}
	RED_INLINE void ClearFast()
	{
		m_keyArray.ClearFast();
	}

	// Push new element
	RED_INLINE void Push( const K& key )
	{
		// Add key as a last element of heap
		m_keyArray.PushBack( key );

		// Restore heap property
		Uint32 index = m_keyArray.Size() - 1;
		Uint32 parentIndex = ( index - 1 ) >> 1;
		while ( index && CompareFunc::Less( m_keyArray[ parentIndex ], m_keyArray[ index ] ) )
		{
			m_keyArray.Swap( index, parentIndex );
			index = parentIndex;
			parentIndex = ( index - 1 ) >> 1;
		}
	}

	// Pop element with highest priority
	RED_INLINE void Pop()
	{
		RED_FATAL_ASSERT( m_keyArray.Size() > 0, "" );

		// Move last element of the heap to the top
		// Self assignment may happened here
		m_keyArray[0] = m_keyArray[m_keyArray.Size()-1];

		// Erase last element
		// We use bidirectional iterator, right now we know it's just a T* so it works fine
		typename TDynArray< K >::iterator it = m_keyArray.End();
		m_keyArray.Erase(--it);

		// Restore heap property
		Uint32 index = 0;
		while ( (2*index+1) < m_keyArray.Size() )
		{
			Uint32 offset = 1;

			// If this node has also right child, find which one is greater (left or right)
			if ( (2*index+2) < m_keyArray.Size() && CompareFunc::Less(m_keyArray[2*index+1], m_keyArray[2*index+2]) )
			{
					offset = 2;
			}

			// If greater children (left or right) is greater then this node, swap them
			if( CompareFunc::Less(m_keyArray[index], m_keyArray[2*index+offset]) )
			{
				m_keyArray.Swap( index, 2*index+offset );
				index = 2*index+offset;
			}
			else
			{
				break;
			}
		}
	}

	// Return element with highest priority
	// We don't want to have non-const Top method because we could invalidate heap property by using it
	RED_INLINE const K& Top() const
	{
		RED_FATAL_ASSERT( m_keyArray.Size() > 0, "" );
		return m_keyArray[0];
	}

	// Compare
	RED_INLINE Bool operator==( const TPriQueue& queue ) const
	{
		return m_keyArray == queue.m_keyArray;
	}
	RED_INLINE Bool operator!=( const TPriQueue& queue ) const
	{
		return !(*this == queue);
	}

	// Get array element
	RED_INLINE K& operator[]( Uint32 index )
	{
		return m_keyArray[ index ];
	}

	// Get array element
	RED_INLINE const K& operator[]( Uint32 index ) const
	{
		return m_keyArray[ index ];
	}

	// Serialization
	friend IFile& operator<<( IFile& file, TPriQueue< K, CompareFunc >& priqueue )
	{
		file << priqueue.m_keyArray;
		return file;
	}
};
