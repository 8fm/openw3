/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

template <
	Int32 size,
	typename K,
	typename HashFunc = DefaultHashFunc< K >,
	typename EqualFunc = DefaultEqualFunc< K >
>
class THashSetPreallocated
{
public:
	typedef K			key_type;
	typedef HashFunc	hash_func;
	typedef EqualFunc   equal_func;

protected:
	struct HashLink
	{
		K				m_key;
		HashLink*		m_next;
	};
	
	TBitSet< size >		m_bitset;
	HashLink			m_head[size];

public:
	class iterator
	{
	public:
		THashSetPreallocated< size, K, HashFunc, EqualFunc >*	m_set;
		Int32														m_bucketIndex;
		HashLink*												m_bucketIterator;

	public:
		RED_INLINE iterator()
		{
		}
		RED_INLINE iterator( const iterator& it )
		{
			*this = it;
		}
		RED_INLINE iterator( THashSetPreallocated< size, K, HashFunc, EqualFunc >* set, Int32 bucketIndex,	HashLink* bucketIterator )
		{
			m_set            = set;
			m_bucketIndex    = bucketIndex;
			m_bucketIterator = bucketIterator;
		}
		RED_INLINE const iterator& operator=( const iterator& it )
		{
			m_set            = it.m_set;
			m_bucketIndex    = it.m_bucketIndex;
			m_bucketIterator = it.m_bucketIterator;
			return *this;
		}
		RED_INLINE ~iterator()
		{
		}
		RED_INLINE K& operator*() const
		{
			return (m_bucketIterator->m_key);
		}
		RED_INLINE K* operator->() const
		{
			return &(m_bucketIterator->m_key);
		}
		RED_INLINE Bool operator==( const iterator& it )
		{
			return (m_bucketIterator == it.m_bucketIterator) && (m_bucketIndex == it.m_bucketIndex) && (m_set == it.m_set);
		}
		RED_INLINE Bool operator!=( const iterator& it )
		{
			return !(*this == it);
		}
		RED_INLINE iterator operator++()
		{
			m_bucketIterator = m_bucketIterator->m_next;
			if( m_bucketIterator == NULL )
			{
				//while( ( ++m_bucketIndex < size ) && ( m_set->m_bitset.Get( m_bucketIndex ) == false ) ) {};
				m_bucketIndex = m_bitset.FindNextSet( m_bucketIndex + 1 );
				m_bucketIterator = ( m_bucketIndex == size ) ? NULL : &m_set->m_head[m_bucketIndex];
			}
			return *this;
		}
		RED_INLINE iterator operator++( Int32 Temp )
		{
			iterator oldIterator = *this;;
			++(*this);
			return oldIterator;
		}
	};

	class const_iterator
	{
	public:
		const THashSetPreallocated< size, K, HashFunc, EqualFunc >*	m_set;
		Int32															m_bucketIndex;
		const HashLink*												m_bucketIterator;

	public:
		RED_INLINE const_iterator()
		{
		}
		RED_INLINE const_iterator( const const_iterator& it )
		{
			*this = it;
		}
		RED_INLINE const_iterator( const THashSetPreallocated< size, K, HashFunc, EqualFunc >* set, Int32 bucketIndex, const HashLink* bucketIterator )
		{
			m_set            = set;
			m_bucketIndex    = bucketIndex;
			m_bucketIterator = bucketIterator;
		}
		RED_INLINE const const_iterator& operator=( const const_iterator& it )
		{
			m_set            = it.m_set;
			m_bucketIndex    = it.m_bucketIndex;
			m_bucketIterator = it.m_bucketIterator;
			return *this;
		}
		RED_INLINE ~const_iterator()
		{
		}
		RED_INLINE const K& operator*() const
		{
			return (m_bucketIterator->m_key);
		}
		RED_INLINE const K* operator->() const
		{
			return &(m_bucketIterator->m_key);
		}
		RED_INLINE Bool operator==( const const_iterator& it )
		{
			return (m_bucketIterator == it.m_bucketIterator) && (m_bucketIndex == it.m_bucketIndex) && (m_set == it.m_set);
		}
		RED_INLINE Bool operator!=( const const_iterator& it )
		{
			return !(*this == it);
		}
		RED_INLINE const_iterator operator++()
		{
			m_bucketIterator = m_bucketIterator->m_next;
			if( m_bucketIterator == NULL )
			{
				//while( ( ++m_bucketIndex < size ) && ( m_set->m_bitset.Get( m_bucketIndex ) == false ) ) {};
				m_bucketIndex = m_bitset.FindNextSet( m_bucketIndex + 1 );
				m_bucketIterator = ( m_bucketIndex == size ) ? NULL : &m_set->m_head[m_bucketIndex];
			}
			return *this;
		}
		RED_INLINE const_iterator operator++( Int32 Temp )
		{
			const_iterator oldIterator = *this;;
			++(*this);
			return oldIterator;
		}
	};

	RED_INLINE THashSetPreallocated()
	{
	}
	RED_INLINE THashSetPreallocated( const THashSetPreallocated& set )
	{
		RED_FATAL( "" );
	}
	RED_INLINE const THashSetPreallocated& operator=( const THashSetPreallocated& set )
	{
		RED_FATAL( "" );
		return *this;
	}
	RED_INLINE ~THashSetPreallocated()
	{
		Clear();
	}

	RED_INLINE Uint32 Size() const
	{
		Uint32 sz = 0;
		for ( typename THashSetPreallocated::const_iterator it = Begin(); it != End(); ++it )
		{
			++sz;
		}
		return sz;
	}

	// Insert key only if key doesn't not exist in map already
	RED_INLINE Bool Insert( const K& key )
	{
		if( Exist( key ) == true )
		{
			return false;
		}
		InsertNoCheck( key );
		return true;
	}

	RED_INLINE void InsertNoCheck( const K& key )
	{
		const Uint32 hash = HashFunc::GetHash( key ) % size;
		if ( m_bitset.Get(hash) == true )
		{
			HashLink* link = new HashLink;
			link->m_key = key;
			link->m_next = m_head[hash].m_next;
			m_head[hash].m_next = link;
		}
		else
		{
			m_head[hash].m_key = key;
			m_head[hash].m_next = NULL;
			m_bitset.Set( hash );
		}
	}

	RED_INLINE Bool Erase( const K& key )
	{
		const Uint32 hash = HashFunc::GetHash( key ) % size;
		if ( m_bitset.Get(hash) == false )
		{
			return false;
		}

		for( HashLink *link = &m_head[hash], *prev = NULL; link; prev = link, link = link->m_next )
		{
			if ( link->m_key == key )
			{
				if ( prev == NULL )
				{
					m_bitset.Clear( hash );
				}
				else
				{
					prev->m_next = link->m_next;
					delete link;
				}
				return true;
			}
		}
		return false;
	}

	RED_INLINE Bool Erase( iterator& it )
	{
		//m_hashArray[it.m_bucketIndex].Erase( it.m_bucketIterator );
		return true;
	}

	RED_INLINE void Clear()
	{
		Int32 index = m_bitset.FindNextSet( 0 );
		while( index < size )
		{
			if ( m_head[index].m_next != NULL )
			{
				ClearLink( m_head[index].m_next );
			}
			index = m_bitset.FindNextSet( index + 1 );
		}
		m_bitset.ClearAll();
	}

	// Find key
	// Returns iterator to key or End() if key is not found
	RED_INLINE iterator Find( const K& key )
	{
		const Uint32 hash = HashFunc::GetHash( key ) % size;
		if ( m_bitset.Get(hash) == true )
		{
			for( HashLink* link = &m_head[hash]; link; link = link->m_next )
			{
				if ( link->m_key == key )
				{
					iterator it;
					it.m_set = this;
					it.m_bucketIndex = hash;
					it.m_bucketIterator = link;
					return it;
				}
			}
		}
		return End();
	}

	// Check if key exists in map
	RED_INLINE Bool Exist( const K& key ) const
	{
		const Uint32 hash = HashFunc::GetHash( key ) % size;
		if ( m_bitset.Get(hash) == true )
		{
			for( const HashLink* link = &m_head[hash]; link; link = link->m_next )
			{
				if ( link->m_key == key )
				{
					return true;
				}
			}
		}
		return false;
	}

	// Compare
	RED_INLINE Bool operator==( const THashSetPreallocated& set ) const
	{
		return false;
	}

	RED_INLINE Bool operator!=( const THashSetPreallocated& map ) const
	{
		return !(*this == map);
	}

	// Iterators
	RED_INLINE const_iterator Begin() const
	{
		const_iterator it( this, -1, NULL );
		// while( ( ++it.m_bucketIndex < size ) && ( m_bitset.Get( it.m_bucketIndex ) == false ) ) {};
		it.m_bucketIndex = m_bitset.FindNextSet( 0 );
		it.m_bucketIterator = ( it.m_bucketIndex == size ) ? NULL : &m_head[it.m_bucketIndex];
		return it;
	}

	RED_INLINE const_iterator End() const
	{
		return const_iterator( this, size, NULL );
	}

	RED_INLINE iterator Begin()
	{
		iterator it( this, -1, NULL );
		it.m_bucketIndex = m_bitset.FindNextSet( 0 );
		it.m_bucketIterator = ( it.m_bucketIndex == size ) ? NULL : &m_head[it.m_bucketIndex];
		return it;
	}

	RED_INLINE iterator End()
	{
		return iterator( this, size, NULL );
	}

	// Serialization
	friend IFile& operator<<( IFile& file, THashSetPreallocated<size, K, HashFunc, EqualFunc >& hashset )
	{
		if ( file.IsReader() || file.IsWriter() )
		{
			//Uint32 size = Size();
			file << size;
		}

		/*
#ifdef RED_ENDIAN_SWAP_SUPPORT_DEPRECATED
		if ( TPlainType<T>::Value && !file.IsByteSwapping() )
#else
		if ( TPlainType<T>::Value )
#endif
		{
			// Serialize whole buffer
			if ( ar.m_size )
			{
				file.Serialize( ar.m_buf, ar.m_size * sizeof(T) );
			}
		}
		else
		{
			// Serialize each element
			for ( Uint32 i=0; i<ar.m_size; i++ )
			{
				file << ar.TypedData()[i];
			}
		}
		*/
		return file;
	}

protected:
	void ClearLink( HashLink* link )
	{
		if ( link->m_next != NULL )
		{
			ClearLink( link->m_next );
		}
		delete link;
	}

};

