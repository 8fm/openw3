/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "dynarray.h"

/// Non intrusive linked list
template < typename K >
class TList
{
public:
	typedef K			key_type;

	// Link info
	struct Link
	{
		K				m_key;
		Link*			m_next;

		// Construct link object
		RED_INLINE Link( const K& data )
			: m_key( data )
			, m_next( NULL )
		{};
	};

	// Non const iterator
	class iterator
	{
		friend class TList< K >;

	private:
		Link*		m_link;

	public:
		RED_INLINE iterator()
			: m_link( NULL )
		{
		}

		RED_INLINE iterator( const iterator& it )
		{
			*this = it;
		}

		RED_INLINE const iterator& operator=( const iterator& it )
		{
			m_link = it.m_link;
			return *this;
		}

		RED_INLINE ~iterator()
		{
		}

		RED_INLINE K& operator*()
		{
			return m_link->m_key;
		}

		RED_INLINE K* operator->()
		{
			return &(m_link->m_key);
		}

		RED_INLINE K* NextPtr()
		{
			if ( m_link->m_next )
			{
				return &(m_link->m_next->m_key);
			}
			else
			{
				return NULL;
			}
		}

		RED_INLINE iterator operator++()
		{
			m_link = m_link->m_next;
			return *this;
		}

		RED_INLINE iterator operator++( Int32 )
		{
			iterator oldIterator = *this;;
			++(*this);
			return oldIterator;
		}

		RED_INLINE Bool operator==( const iterator& it ) const
		{
			return (m_link == it.m_link);
		}

		RED_INLINE Bool operator!=( const iterator& it ) const
		{
			return (m_link != it.m_link);
		}
	};

	/// Const iterator
	class const_iterator
	{
		friend class TList< K >;

	private:
		Link*		m_link;

	public:
		RED_INLINE const_iterator()
			: m_link( NULL )
		{
		}

		RED_INLINE const_iterator( const const_iterator& it )
		{
			*this = it;
		}

		RED_INLINE const_iterator& operator=( const const_iterator& it )
		{
			m_link = it.m_link;
			return *this;
		}

		RED_INLINE ~const_iterator()
		{
		}

		RED_INLINE const K& operator*()
		{
			return m_link->m_key;
		}

		RED_INLINE const K* operator->()
		{
			return &(m_link->m_key);
		}

		RED_INLINE const_iterator operator++() 
		{
			m_link = m_link->m_next;
			return *this;
		}

		RED_INLINE const_iterator operator++( Int32 )
		{
			const_iterator oldIterator = *this;
			++(*this);
			return oldIterator;
		}

		RED_INLINE Bool operator==( const const_iterator& it ) const
		{
			return m_link == it.m_link;
		}

		RED_INLINE Bool operator!=( const const_iterator& it ) const
		{
			return m_link != it.m_link;
		}
	};

protected:
	Link*	m_head;		//!< List head

public:
	RED_INLINE TList()
		: m_head( NULL )
	{
	}

	RED_INLINE TList( const TList& list )
		: m_head( NULL )
	{
		*this = list;
	}

	RED_INLINE const TList& operator=( const TList& list )
	{
		Clear();

		for( const_iterator it = list.Begin(); it != list.End(); ++it )
		{
			PushBack( *it );
		}

		return *this;
	}

	RED_INLINE ~TList()
	{
		Clear();
	}

	RED_INLINE Uint32 Size() const
	{
		int size = 0;
		for( const_iterator it = Begin(); it != End(); ++it )
		{
			++size;
		}
		return size;
	}

	RED_INLINE Uint32 DataSize() const
	{
		return Size() * sizeof(K);
	}

	RED_INLINE bool Empty() const
	{
		return m_head == NULL;
	}

	RED_INLINE void PushFront( const TDynArray<K>& keys )
	{
		for ( Uint32 i=0; i<keys.Size(); i++ )
		{
			PushFront( keys[i] );
		}
	}

	RED_INLINE void PushFront( const TList<K>& values )
	{
		for( auto valuesIt = values.Begin(); valuesIt != values.End(); valuesIt++ )
		{
			PushFront( *valuesIt );
		}
	}

	RED_INLINE void PushFront( const K& key )
	{
		Link* link = CreateLink( key );
		link->m_next = m_head;
		m_head = link;
	}

	RED_INLINE void PopFront()
	{
		RED_FATAL_ASSERT( m_head != NULL, "" );
		Link* next = m_head->m_next;
		DestroyLink( m_head );
		m_head = next;
	}

	RED_INLINE void PushBack( const TList<K>& values )
	{
		for( auto valuesIt = values.Begin(); valuesIt != values.End(); valuesIt++ )
		{
			PushBack( *valuesIt );
		}
	}

	RED_INLINE void PushBack( const TDynArray<K>& keys )
	{
		for ( Uint32 i=0; i<keys.Size(); i++ )
		{
			PushBack( keys[i] );
		}
	}

	RED_INLINE void PushBack( const K& key )
	{
		Link* link = CreateLink( key );

		if( m_head == NULL )
		{
			m_head = link;
		}
		else
		{
			for( iterator it = Begin(); it != End(); ++it )
			{
				if( it.m_link->m_next == NULL )
				{
					it.m_link->m_next = link;
					break;
				}
			}
		}
	}

	RED_INLINE void PopBack()
	{
		RED_FATAL_ASSERT( m_head != NULL, "" );

		if ( m_head->m_next == NULL )
		{
			DestroyLink( m_head );
			m_head = NULL;
		}
		else
		{
			for( iterator it = Begin(); it != End(); ++it )
			{
				RED_FATAL_ASSERT( it.m_link->m_next != NULL, "" );
				if ( it.m_link->m_next->m_next == NULL )
				{
					DestroyLink( it.m_link->m_next );
					it.m_link->m_next = NULL;
					break;
				}
			}
		}
	}

	// Return reference to element on front of list
	RED_INLINE const K& Front() const
	{
		RED_FATAL_ASSERT( m_head != NULL, "" );
		return m_head->m_key;
	}

	// Return reference to element on front of list
	RED_INLINE K& Front()
	{
		RED_FATAL_ASSERT( m_head != NULL, "" );
		return m_head->m_key;
	}

	// Return reference to element on back of list
	RED_INLINE const K& Back() const
	{
		RED_FATAL_ASSERT( m_head != NULL, "" );
		for( const_iterator it = Begin(); it != End(); ++it )
		{
			if( it->m_next == NULL )
			{
				return it->m_key;
			}
		}

		// Should never be here but we want to shut up compiler
		RED_FATAL( "Unexpected control flow" );
		return m_head->m_key;
	}
	// Return reference to element on back of list
	RED_INLINE K& Back()
	{
		RED_FATAL_ASSERT( m_head != NULL, "" );
		for( iterator it = Begin(); it != End(); ++it )
		{
			if( it.m_link->m_next == NULL )
			{
				return it.m_link->m_key;
			}
		}

		// Should never be here but we want to shut up compiler
		RED_FATAL( "Unexpected control flow" );
		return m_head->m_key;
	}

	// Insert key before position
	RED_INLINE Bool Insert( iterator& position, const K& key )
	{
		if( position == Begin() )
		{
			PushFront( key );
			return true;
		}
		else
		{
			for( iterator it = Begin(); it != End(); ++it )
			{
				if( it.m_link->m_next == position.m_link )
				{
					Link* link = CreateLink( key );
					link->m_next = position.m_link;
					it.m_link->m_next = link;
					return true;
				}
			}
		}
		return false;
	}

	RED_INLINE iterator Erase( iterator& it )
	{
		iterator ret;
		ret.m_link = it.m_link->m_next;
		DestroyLink( it.m_link );

		if ( it.m_link == m_head )
		{
			m_head = ret.m_link;
			return ret;
		}
		
		for( iterator it2 = Begin(); it2 != End(); ++it2 )
		{
			if( it2.m_link->m_next == it.m_link )
			{
				it2.m_link->m_next = ret.m_link;
				return ret;
			}
		}

		RED_FATAL( "Erase called with broken iterator (from another list?)" );
		return iterator();
	}

	RED_INLINE void Clear()
	{
		for( iterator it = Begin(); it != End(); )
		{
			iterator next = it;
			++next;
			DestroyLink( it.m_link );
			it = next;
		}
		m_head = NULL;
	}

	RED_INLINE void ClearPtr()
	{
		for( iterator it = Begin(); it != End(); )
		{
			iterator next = it;
			++next;
			delete it.m_link->m_key;
			DestroyLink( it.m_link );
			it = next;
		}
		m_head = NULL;
	}

	// Check if element exists in list
	RED_INLINE Bool Exist( const K& elem ) const
	{
		for( const_iterator it = Begin(); it != End(); ++it )
		{
			if ( *it == elem )
			{
				return true;
			}
		}

		return false;
	}

	// Remove element from list
	RED_INLINE Bool Remove( const K& elem )
	{
		for( iterator it = Begin(); it != End(); ++it )
		{
			if ( *it == elem )
			{
				Erase( it );
				return true;
			}
		}

		return false;
	}

	// Compare
	RED_INLINE Bool operator==( const TList& list ) const
	{
		const_iterator it1, it2;

		// Compare elements on each list
		for( it1 = Begin(), it2 = list.Begin(); it1 != End() && it2 != list.End(); ++it1, ++it2 )
		{
			if ( (*it1) != (*it2) )
			{
				return false;
			}
		}

		// End of one list, but the end of other, so not equal
		if( it1 != End() || it2 != list.End() )
		{
			return false;
		}

		return true;
	}

	RED_INLINE Bool operator!=( const TList& list ) const
	{
		return !(*this == list);
	}

	// Iterators
	RED_INLINE const const_iterator Begin() const
	{
		const_iterator it;
		it.m_link = m_head;
		return it;
	}

	RED_INLINE const const_iterator End() const
	{
		const_iterator it;
		it.m_link = NULL;
		return it;
	}

	RED_INLINE iterator Begin()
	{
		iterator it;
		it.m_link = m_head;
		return it;
	}
	RED_INLINE iterator End()
	{
		iterator it;
		it.m_link = NULL;
		return it;
	}

protected:
	//! Create link item
	RED_INLINE Link* CreateLink( const K& data )
	{
		void* mem = RED_MEMORY_ALLOCATE( MemoryPool_SmallObjects, MC_List, sizeof(Link) );
		return new ( mem ) Link( data );
	}

	//! Destroy link item
	RED_INLINE void DestroyLink( Link* link )
	{
		if ( link )
		{
			link->~Link();
			RED_MEMORY_FREE( MemoryPool_SmallObjects, MC_List, link );
		}
	}

public:
	//! Serialization
	friend IFile& operator<<( IFile& file, TList<K>& l )
	{
		// GC only
		if ( file.IsGarbageCollector() )
		{
			for ( TList<K>::iterator i=l.Begin(); i!=l.End(); i++ )
			{
				file << (*i);
			}
		}

		// Writing
		else if ( file.IsWriter() )
		{
			// Save count
			Uint32 size = l.Size();
			file << size;

			// Save elements
			for ( TList<K>::iterator i=l.Begin(); i!=l.End(); i++ )
			{
				file << (*i);
			}
		}

		// Reading
		else if ( file.IsReader() )
		{
			// Load count
			Uint32 size = 0;
			file << size;

			// Clear current list
			l.Clear();

			// Load elements
			for ( Uint32 i=0; i<size; i++ )
			{
				// Load element
				K element;
				file << element;

				// Add to list
				l.PushBack( element );
			}
		}

		return file;
	}
};

// Enable c++11 range-based for loop

template < typename T >
typename TList< T >::iterator begin( TList< T >& list ) { return list.Begin(); }

template < typename T >
typename TList< T >::iterator end( TList< T >& list ) { return list.End(); }

template < typename T >
typename TList< T >::const_iterator begin( const TList< T >& list ) { return list.Begin(); }

template < typename T >
typename TList< T >::const_iterator end( const TList< T >& list ) { return list.End(); }
