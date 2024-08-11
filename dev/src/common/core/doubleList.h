/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

/// Non intrusive double linked list
template < typename K >
class TDoubleList
{
public:
	typedef K			key_type;

	// Link info
	struct Link
	{
		K				m_key;
		Link*			m_next;
		Link*			m_prev;

		// Construct link object
		RED_INLINE Link( const K& data )
			: m_key( data )
			, m_next( NULL )
			, m_prev( NULL )
		{};
	};

	// Non const iterator
	class iterator
	{
		friend class TDoubleList< K >;

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

		RED_INLINE iterator operator++()
		{
			m_link = m_link->m_next;
			return *this;
		}

		RED_INLINE iterator operator++( Int32 Temp )
		{
			iterator oldIterator = *this;
			++(*this);
			return oldIterator;
		}

		RED_INLINE iterator operator--()
		{
			m_link = m_link->m_prev;
			return *this;
		}

		RED_INLINE iterator operator--( Int32 Temp )
		{
			iterator oldIterator = *this;
			--(*this);
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
		friend class TDoubleList< K >;

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

		RED_INLINE const_iterator( const iterator& it )
		{
			m_link = it.m_link;
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

		RED_INLINE const_iterator operator++( Int32 Temp )
		{
			const_iterator oldIterator = *this;;
			++(*this);
			return oldIterator;
		}

		RED_INLINE iterator operator--()
		{
			m_link = m_link->m_prev;
			return *this;
		}

		RED_INLINE iterator operator--( Int32 Temp )
		{
			const_iterator oldIterator = *this;
			--(*this);
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


private:
	Link*	m_head;		//!< List head
	Link*	m_tail;		//!< List tail
	Uint32	m_size;		//!< List size

public:
	RED_INLINE TDoubleList()
		: m_head( NULL )
		, m_tail( NULL )
		, m_size( 0 )
	{
	}

	RED_INLINE TDoubleList( const TDoubleList& list )
		: m_head( NULL )
		, m_tail( NULL )
		, m_size( NULL )
	{
		*this = list;
	}

	RED_INLINE const TDoubleList& operator=( const TDoubleList& list )
	{
		Clear();

		for( const_iterator it = list.Begin(); it != list.End(); ++it )
		{
			PushBack( *it );
		}

		return *this;
	}

	RED_INLINE ~TDoubleList()
	{
		Clear();
	}

	RED_INLINE Uint32 Size() const
	{
		return m_size;
	}

	RED_INLINE Bool Empty() const
	{
		return Size() == 0;
	}

	RED_INLINE void PushFront( const TDynArray<K>& keys )
	{
		for ( Uint32 i=0; i<keys.Size(); i++ )
		{
			PushFront( keys[i] );
		}
	}

	RED_INLINE void PushFront( const K& key )
	{
		Link* link = CreateLink( key );
		link->m_prev = NULL;
		link->m_next = m_head;

		if( m_head != NULL )
		{
			m_head->m_prev = link;
		}
		else
		{
			// It means also m_tail == NULL
			m_tail = link;
		}
	
		m_head = link;

		++m_size;
	}

	RED_INLINE void PopFront()
	{
		RED_FATAL_ASSERT( m_head != NULL, "" );
		RED_FATAL_ASSERT( m_tail != NULL, "" );
		RED_FATAL_ASSERT( m_size > 0, "" );

		Link* next = m_head->m_next;

		DestroyLink( m_head );

		if( next != NULL )
		{
			next->m_prev = NULL;
			m_head = next;
		}
		else
		{
			m_head = NULL;
			m_tail = NULL;
		}

		--m_size;
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
		link->m_prev = m_tail;
		link->m_next = NULL;

		if( m_tail != NULL )
		{
			m_tail->m_next = link;
		}
		else
		{
			// It means also m_head == NULL
			m_head = link;
		}

		m_tail = link;
		++m_size;
	}

	RED_INLINE void PopBack()
	{
		RED_FATAL_ASSERT( m_head != NULL, "" );
		RED_FATAL_ASSERT( m_tail != NULL, "" );
		RED_FATAL_ASSERT( m_size > 0, "" );

		Link* prev = m_tail->m_prev;

		DestroyLink( m_tail );

		if( prev != NULL )
		{
			prev->m_next = NULL;
			m_tail = prev;
		}
		else
		{
			m_head = NULL;
			m_tail = NULL;
		}

		--m_size;
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
		return m_tail->m_key;
	}
	// Return reference to element on back of list
	RED_INLINE K& Back()
	{
		RED_FATAL_ASSERT( m_head != NULL, "" );
		return m_tail->m_key;
	}

	// Insert key before position
	RED_INLINE Bool Insert( iterator& position, const K& key )
	{
		if( position == Begin() )
		{
			PushFront( key );
			return true;
		}
		else if( position == End() )
		{
			PushBack( key );
			return true;
		}
		else
		{
			for( iterator it = Begin(); it != End(); ++it )
			{
				if( it.m_link->m_next == position.m_link )
				{
					Link* link = CreateLink( key );
					link->m_prev = it.m_link;
					link->m_next = position.m_link;
					it.m_link->m_next = link;
					position.m_link->m_prev = link;
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

		if ( it.m_link == m_head )
		{
			PopFront();
			return ret;
		}
		else if ( it.m_link == m_tail )
		{
			PopBack();
			return ret;
		}

		for( iterator it2 = Begin(); it2 != End(); ++it2 )
		{
			if( it2.m_link == it.m_link )
			{
				it2.m_link->m_prev->m_next = it2.m_link->m_next;
				it2.m_link->m_next->m_prev = it2.m_link->m_prev;

				DestroyLink( it.m_link );
				--m_size;
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
		m_tail = NULL;
		m_size = 0;
	}

	RED_INLINE void ClearPtr()
	{
		Clear();
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
	RED_INLINE Bool operator==( const TDoubleList& list ) const
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

	RED_INLINE Bool operator!=( const TDoubleList& list ) const
	{
		return !(*this == list);
	}

	// Iterators
	RED_INLINE const_iterator Begin() const
	{
		const_iterator it;
		it.m_link = m_head;
		return it;
	}

	RED_INLINE const_iterator End() const
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
		void* mem =	RED_MEMORY_ALLOCATE( MemoryPool_SmallObjects, MC_List, sizeof(Link) );
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
			for ( typename TList<K>::iterator i=l.Begin(); i!=l.End(); i++ )
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
			for ( typename TList<K>::iterator i=l.Begin(); i!=l.End(); i++ )
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
