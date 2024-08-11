/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

template < typename K >
class TQueue
{
public:
	typedef K			key_type;

protected:
	Uint32				m_numKeys;
	Uint32				m_backIndex;
	Uint32				m_frontIndex;
	TDynArray< K >		m_keyArray;

public:
	RED_INLINE TQueue()
	{
		m_numKeys = m_frontIndex = m_backIndex = 0;
	}
	RED_INLINE TQueue( const TQueue& queue )
	{
		*this = queue;
	}
	RED_INLINE const TQueue& operator=( const TQueue& queue )
	{
		m_numKeys    = queue.m_numKeys;
		m_backIndex  = queue.m_backIndex;
		m_frontIndex = queue.m_frontIndex;
		m_keyArray   = queue.m_keyArray;
		return *this;
	}
	RED_INLINE ~TQueue()
	{
	}

	RED_INLINE Uint32 Size() const
	{
		return m_numKeys;		
	}
	RED_INLINE Uint32 DataSize() const
	{
		return m_numKeys*sizeof(K);
	}
	RED_INLINE Uint32 Capacity() const
	{
		return m_keyArray.Capacity();
	}
	RED_INLINE void Reserve( Uint32 size ) 
	{
		return m_keyArray.Reserve( size );
	}
	RED_INLINE bool Empty() const
	{
		return m_numKeys == 0;
	}
	RED_INLINE void Clear()
	{
		m_numKeys = m_frontIndex = m_backIndex = 0;
		m_keyArray.Clear();
	}
	RED_INLINE void ClearPtr()
	{
		m_numKeys = m_frontIndex = m_backIndex = 0;
		m_keyArray.ClearPtr();
	}
	// Push new element on back of queue
	RED_INLINE void Push( const K& key )
	{
		if( m_numKeys == 0 )
		{
			if( m_keyArray.Size() == 0 )
			{
				m_keyArray.PushBack( key );
			}
			else
			{
				m_keyArray[m_backIndex] = key;
			}
		}
		else
		{
			if( m_backIndex == m_frontIndex )
			{
				// Grow array, because we are out of space also increment frontIndex because we moved elements
				m_keyArray.Insert( m_backIndex, key );
				m_frontIndex++;
			}
			else
			{
				m_keyArray[m_backIndex] = key;
			}
		}
		m_backIndex = (m_backIndex+1) % m_keyArray.Size();
		m_numKeys++;
	}

	// Pop element from front of queue
	RED_INLINE void Pop()
	{
		RED_FATAL_ASSERT( m_numKeys > 0, "" );
		m_frontIndex = (m_frontIndex+1) % m_keyArray.Size();
		m_numKeys--;
	}

	// Return reference to element on front of queue
	RED_INLINE const K& Front() const
	{
		RED_FATAL_ASSERT( m_numKeys > 0, "" );
		return m_keyArray[m_frontIndex];
	}
	// Return reference to element on front of queue
	RED_INLINE K& Front()
	{
		RED_FATAL_ASSERT( m_numKeys > 0, "" );
		return m_keyArray[m_frontIndex];
	}

	// Return reference to element on back of queue
	RED_INLINE const K& Back() const
	{
		RED_FATAL_ASSERT( m_numKeys > 0, "" );
		Uint32 index = (m_keyArray.Size()+m_backIndex-1)%m_keyArray.Size();
		return m_keyArray[index];
	}
	// Return reference to element on back of queue
	RED_INLINE K& Back()
	{
		RED_FATAL_ASSERT( m_numKeys > 0, "" );
		Uint32 index = (m_keyArray.Size()+m_backIndex-1)%m_keyArray.Size();
		return m_keyArray[index];
	}

	// Compare
	RED_INLINE Bool operator==( const TQueue& queue ) const
	{
		if( m_numKeys == queue.m_numKeys )
		{
			for( Uint32 i = 0; i < m_numKeys; ++i )
			{
				if ( m_keyArray[(m_frontIndex+i)%m_keyArray.Size()] != queue.m_keyArray[(queue.m_frontIndex+i)%queue.m_keyArray.Size()] )
				{
					return false;
				}
			}
			return true;
		}
		return false;
	}
	RED_INLINE Bool operator!=( const TQueue& queue ) const
	{
		return !(*this == queue);
	}

public:
	// Serialization
	friend IFile& operator<<( IFile& file, TQueue< K >& queue )
	{
		file << queue.m_numKeys;
		file << queue.m_backIndex;
		file << queue.m_frontIndex;
		file << queue.m_keyArray;
		return file;
	}
};
