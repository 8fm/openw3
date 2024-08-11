#pragma once

#include "pool.h"
#include "hash.h"

/**
 *	Hash set
 *
 *	Notes on implementation:
 *	- fast insert/erase/find
 *	- fast iteration (as fast as iterating an array)
 *	- overhead per element: sizeof( TBucket ) + sizeof( TElement ) = 24 bytes
 */
template <
	typename K,
	typename HashFunc = DefaultHashFunc< K >,
	typename EqualFunc = DefaultEqualFunc< K >,
	EMemoryClass memoryClass = MC_HashSet,
	RED_CONTAINER_POOL_TYPE memoryPool = MemoryPool_Default >
class THashSet
{
private:
	static const Uint32 UNUSED_ELEMENT_INDEX = 0xFFFFFFFF;

	struct TElement
	{
		Uint32	m_hash;			// Cached hash of the key
		Uint32	m_index;		// Index of the key in m_keys; UNUSED_ELEMENT_INDEX if element is unused
		Uint32	m_nextId;		// Id of the next element in the bucket; CPool::INVALID_INDEX if there's no next element
	};

	// Each bucket stores fixed number of in-place elements; every next element is on the linked list
	struct TBucket : TElement
	{
		RED_FORCE_INLINE Uint32 IsUsed() const { return TElement::m_index != UNUSED_ELEMENT_INDEX; }
		RED_FORCE_INLINE void SetUnused() { TElement::m_index = UNUSED_ELEMENT_INDEX; }
	};

	Uint32		m_capacity;		// Capacity
	Uint32		m_size;			// Number of elements stored
	TBucket*	m_buckets;		// Buckets with lists of elements with matching hashes modulo m_capacity
	K*			m_keys;			// An array of keys
	CPool		m_elementsPool;	// Pool of TElements for use in linked lists
	Bool		m_autoDownsize;	// If set automatically downsizes internal buffers on Erase(); NOTE: Downsizes to 150% of the size only if exceeding 200% of the size

public:

	// Iterator
	class iterator
	{
		friend class THashSet;
	private:
		K* m_current;
		RED_FORCE_INLINE iterator( K* current ) : m_current( current ) {}
		RED_FORCE_INLINE iterator( THashSet* set ) : m_current( set->m_keys ) {}
	public:
		RED_FORCE_INLINE iterator() : m_current( nullptr ) {}
		RED_FORCE_INLINE iterator( const iterator& it ) : m_current( it.m_current ) {}
		RED_FORCE_INLINE iterator& operator = ( const iterator it ) { m_current = it.m_current; return *this; }
		RED_FORCE_INLINE K& operator*() const { return *m_current; }
		RED_FORCE_INLINE K* operator->() const { return m_current; }
		RED_FORCE_INLINE Bool operator == ( const iterator it ) const { return m_current == it.m_current; }
		RED_FORCE_INLINE Bool operator != ( const iterator it ) const { return m_current != it.m_current; }
		RED_FORCE_INLINE iterator& operator ++ () { ++m_current; return *this; }
	};

	// Const iterator
	class const_iterator
	{
		friend class THashSet;
	private:
		const K* m_current;
		RED_FORCE_INLINE const_iterator( const K* current ) : m_current( current ) {}
		RED_FORCE_INLINE const_iterator( const THashSet* set ) : m_current( set->m_keys ) {}
	public:
		RED_FORCE_INLINE const_iterator() : m_current( nullptr ) {}
		RED_FORCE_INLINE const_iterator( const const_iterator& it ) { m_current = it.m_current; }
		RED_FORCE_INLINE const_iterator( const iterator it ) { m_current = it.m_current; }
		RED_FORCE_INLINE const_iterator& operator = ( const const_iterator it ) { m_current = it.m_current; return *this; }
		RED_FORCE_INLINE const_iterator& operator = ( const iterator it ) { m_current = it.m_current; return *this; }
		RED_FORCE_INLINE const K& operator*() const { return *m_current; }
		RED_FORCE_INLINE const K* operator->() const { return m_current; }
		RED_FORCE_INLINE Bool operator == ( const iterator it ) const { return m_current == it.m_current; }
		RED_FORCE_INLINE Bool operator != ( const iterator it ) const { return m_current != it.m_current; }
		RED_FORCE_INLINE Bool operator == ( const const_iterator it ) const { return m_current == it.m_current; }
		RED_FORCE_INLINE Bool operator != ( const const_iterator it ) const { return m_current != it.m_current; }
		RED_FORCE_INLINE const_iterator& operator ++ () { ++m_current; return *this; }
	};

	// Initializes hash set with 'initialCapacity' empty slots
	THashSet( Uint32 initialCapacity = 0 );
	// Copy constructor
	THashSet( const THashSet& other );
	// Move constructor
	THashSet( THashSet&& other );
	// Removes all elements and deallocates memory
	~THashSet();

	// Removes all elements and deallocates memory
	void Clear();
	// Removes all elements and calls delete on all of them and then deallocates memory
	void ClearPtr();
	// Quickly removes all elements; does not deallocate memory
	void ClearFast();
	// Reallocates memory so as to ensure 'newCapacity' elements can be stored without memory reallocation
	RED_FORCE_INLINE void Reserve( Uint32 newCapacity );
	// Reallocates memory to exactly given capacity
	void Rehash( Uint32 newCapacity );
	// Minimizes memory usage (makes capacity match the size)
	RED_FORCE_INLINE void Shrink();

	RED_FORCE_INLINE Uint32 Size() const;
	RED_FORCE_INLINE Bool Empty() const;
	RED_FORCE_INLINE Uint32 Capacity() const;
	// Enables automatic downsize of the buffer on Erase()
	RED_FORCE_INLINE void EnableAutoDownsize( Bool enable );
	RED_FORCE_INLINE Uint32 DataSize() const;
	// Gets total internal memory size used by the hash set
	RED_FORCE_INLINE TMemSize GetInternalMemSize() const;

	// Inserts new element; returns true on success
	RED_FORCE_INLINE Bool Insert( const K& key );
	// Erases key; returns true on success
	template < typename KEY_COMPATIBLE >
	Bool Erase( const KEY_COMPATIBLE& key );
	// Erases the key after it was modified (so regular erase wouldn't find it); returns true on success
	template < typename KEY_COMPATIBLE >
	Bool EraseOldKey( const KEY_COMPATIBLE& oldKey, const K& key );
	// Erases key by iterator; returns true on success
	Bool Erase( const iterator& it );
	// Finds key; returns iterator to key or End() if key is not found
	template < typename KEY_COMPATIBLE >
	RED_FORCE_INLINE const_iterator Find( const KEY_COMPATIBLE& key ) const;
	// Finds key; returns iterator to key or End() if key is not found
	template < typename KEY_COMPATIBLE >
	RED_FORCE_INLINE iterator Find( const KEY_COMPATIBLE& key );
	// Finds key; returns pointer to stored key or nullptr if not found
	template < typename KEY_COMPATIBLE >
	RED_FORCE_INLINE K* FindPtr( const KEY_COMPATIBLE& key );
	// Gets reference to key; if doesn't exist, entry gets created
	template < typename KEY_COMPATIBLE >
	RED_FORCE_INLINE K& GetRef( const KEY_COMPATIBLE& key, const K& defaultKey = K() );
	// Checks if key exists in set
	template < typename KEY_COMPATIBLE >
	RED_FORCE_INLINE Bool Exist( const KEY_COMPATIBLE& key ) const;

	// Copy operator
	THashSet& operator = ( const THashSet& other );
	// Move operator
	THashSet& operator = ( THashSet&& other );
	// Swaps container contents with the other container
	void Swap( THashSet& other );
	RED_FORCE_INLINE Bool operator == ( const THashSet& other ) const;
	RED_FORCE_INLINE Bool operator != ( const THashSet& other ) const;
	// Adds elements from given set to this set
	RED_FORCE_INLINE void Add( const THashSet& other );
	// Removes given elements from this set
	RED_FORCE_INLINE void Subtract( const THashSet& other );
	// Gets all keys into provided array
	RED_FORCE_INLINE void GetKeys( TDynArray< K >& dst ) const;

	RED_FORCE_INLINE const_iterator Begin() const { return const_iterator( m_keys ); }
	RED_FORCE_INLINE const_iterator End() const { return const_iterator( m_keys + m_size ); }
	RED_FORCE_INLINE iterator Begin() { return iterator( m_keys ); }
	RED_FORCE_INLINE iterator End() { return iterator( m_keys + m_size ); }

	// Gets hash set stats
	void GetStats( THashContainerStats& stats );
	// Gets user friendly type name
	static const CName& GetTypeName();

	// Serialization
	friend IFile& operator << ( IFile& file, THashSet& set )
	{
		if ( file.IsReader() )
		{
			set.Clear();
			if ( file.GetVersion() >= VER_NEW_HASHSET )
			{
				Uint32 size;
				file << size;
				set.Reserve( size );

				K key;
				for ( Uint32 i = 0; i < size; ++i )
				{
					file << key;
					set.Insert( key );
				}
			}
			else
			{
				// Read in from old format

				typedef TDynArray< K, MC_HashSet > TBucket;
				TDynArray< TBucket, MC_HashSet > hashArray;

				file << hashArray;

				// Determine number of elements stored

				Uint32 size = 0;
				for ( const TBucket& bucket : hashArray )
				{
					size += bucket.Size();
				}

				// Convert to new format

				set.Rehash( size );
				for ( const TBucket& bucket : hashArray )
				{
					for ( auto it = bucket.Begin(), end = bucket.End(); it != end; ++it )
					{
						set.Insert( *it );
					}
				}
			}
		}
		else if ( file.IsWriter() && !file.IsGarbageCollector() )
		{
			// Make sure hash set serialization is deterministic by maintaining deterministic (depending on hash set size) number of buckets
			// Note: Shrink() will do nothing if size equals capacity already

			set.Shrink();

			// Serialize the hash set

			file << set.m_size;
			const Uint32 size = set.m_size;
			for ( Uint32 i = 0; i < size; ++i )
			{
				file << set.m_keys[ i ];
			}
		}
		else
		{
			file << set.m_size;
			const Uint32 size = set.m_size;
			for ( Uint32 i = 0; i < size; ++i )
			{
				file << set.m_keys[ i ];
			}
		}

		return file;
	}

private:
	RED_FORCE_INLINE void Upsize();
	RED_FORCE_INLINE void PostErase();
	void InsertNoFail( K&& key, Uint32 hash );
	RED_FORCE_INLINE void EraseKeyAt( Uint32 index );
	template < typename KEY_COMPATIBLE >
	RED_FORCE_INLINE K* FindInternal( const KEY_COMPATIBLE& key );
	template < typename KEY_COMPATIBLE >
	const K* FindInternal( const KEY_COMPATIBLE& key ) const;
};

// Template implementation

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::THashSet( Uint32 initialCapacity )
	: m_keys( nullptr )
	, m_buckets( nullptr )
	, m_size( 0 )
	, m_capacity( 0 )
	, m_autoDownsize( false )
{
	Rehash( initialCapacity );
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::THashSet( const THashSet& other )
	: m_keys( nullptr )
	, m_buckets( nullptr )
	, m_size( 0 )
	, m_capacity( 0 )
	, m_autoDownsize( false )
{
	const Uint32 otherSize = other.m_size;
	Rehash( otherSize );
	for ( Uint32 i = 0; i < otherSize; ++i )
	{
		Insert( other.m_keys[ i ] );
	}
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::THashSet( THashSet&& other )
	: m_keys( other.m_keys )
	, m_buckets( other.m_buckets )
	, m_elementsPool( Move( other.m_elementsPool ) )
	, m_size( other.m_size )
	, m_capacity( other.m_capacity )
	, m_autoDownsize( other.m_autoDownsize )
{
	other.m_keys = nullptr;
	other.m_buckets = nullptr;
	other.m_size = 0;
	other.m_capacity = 0;
	other.m_autoDownsize = false;
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Swap( THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >& other )
{
	::Swap( m_keys, other.m_keys );
	::Swap( m_buckets, other.m_buckets );
	m_elementsPool.Swap( other.m_elementsPool );
	::Swap( m_capacity, other.m_capacity );
	::Swap( m_size, other.m_size );
	::Swap( m_autoDownsize, other.m_autoDownsize );
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >& THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::operator = ( const THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >& other )
{
	THashSet( other ).Swap( *this ); 
	return *this;
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >& THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::operator = ( THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >&& other )
{
	THashSet( Move( other ) ).Swap( *this ); 
	return *this;
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::~THashSet()
{
	Clear();
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE void THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::EnableAutoDownsize( Bool enable )
{
	m_autoDownsize = enable;
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Uint32 THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Size() const
{
	return m_size;
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Uint32 THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::DataSize() const
{
	return m_size * sizeof( K );
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE TMemSize THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::GetInternalMemSize() const
{
	return m_capacity * ( sizeof( TElement ) + sizeof( TBucket ) + sizeof( K ) );
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Uint32 THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Capacity() const
{
	return m_capacity;
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Bool THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Empty() const
{
	return !m_size;
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
template < typename KEY_COMPATIBLE >
K& THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::GetRef( const KEY_COMPATIBLE& key, const K& defaultKey )
{
	const Uint32 hash = HashFunc::GetHash( key );

	// Check if key already exists and fail if so

	Uint32 bucketIndex;
	if ( m_capacity )
	{
		bucketIndex = hash % m_capacity;

		TBucket* bucket = m_buckets + bucketIndex;
		if ( bucket->IsUsed() )
		{
			if ( bucket->m_hash == hash && EqualFunc::Equal( m_keys[ bucket->m_index ], key ) )
			{
				return m_keys[ bucket->m_index ];
			}

			// Check linked list

			Uint32 currentId = bucket->m_nextId;
			while ( currentId != CPool::INVALID_INDEX )
			{
				TElement* current = static_cast< TElement* >( m_elementsPool.GetBlock( currentId ) );
				if ( current->m_hash == hash && EqualFunc::Equal( m_keys[ current->m_index ], key ) )
				{
					return m_keys[ current->m_index ];
				}
				currentId = current->m_nextId;
			}
		}
	}

	// Make sure there's enough room for new element

	if ( m_size == m_capacity )
	{
		Upsize();

		// Recalculate bucket index

		bucketIndex = hash % m_capacity;
	}

	// Insert key

	K* newKey = new ( m_keys + m_size ) K( defaultKey );

	// Try to insert into bucket head

	TBucket* bucket = m_buckets + bucketIndex;
	if ( !bucket->IsUsed() )
	{
		bucket->m_hash = hash;
		bucket->m_index = m_size;
		++m_size;
		return *newKey;
	}

	// Insert into linked list

	const Uint32 newElementId = m_elementsPool.AllocateBlockIndex();

	TElement* newElement = static_cast< TElement* >( m_elementsPool.GetBlock( newElementId ) );
	RED_FATAL_ASSERT( newElement, "" );
	newElement->m_hash = hash;
	newElement->m_index = m_size;

	newElement->m_nextId = bucket->m_nextId;
	bucket->m_nextId = newElementId;

	++m_size;
	return *newKey;
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Bool THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Insert( const K& key )
{
	const Uint32 hash = HashFunc::GetHash( key );

	// Check if key already exists and fail if so

	Uint32 bucketIndex;
	if ( m_capacity )
	{
		bucketIndex = hash % m_capacity;

		TBucket* bucket = m_buckets + bucketIndex;
		if ( bucket->IsUsed() )
		{
			if ( bucket->m_hash == hash && EqualFunc::Equal( m_keys[ bucket->m_index ], key ) )
			{
				return false;
			}

			// Check linked list

			Uint32 currentId = bucket->m_nextId;
			while ( currentId != CPool::INVALID_INDEX )
			{
				TElement* current = static_cast< TElement* >( m_elementsPool.GetBlock( currentId ) );
				if ( current->m_hash == hash && EqualFunc::Equal( m_keys[ current->m_index ], key ) )
				{
					return false;
				}
				currentId = current->m_nextId;
			}
		}
	}

	// Make sure there's enough room for new element

	if ( m_size == m_capacity )
	{
		Upsize();

		// Recalculate bucket index

		bucketIndex = hash % m_capacity;
	}

	// Insert key

	new ( m_keys + m_size ) K( key );

	// Try to insert into bucket head

	TBucket* bucket = m_buckets + bucketIndex;
	if ( !bucket->IsUsed() )
	{
		bucket->m_hash = hash;
		bucket->m_index = m_size;
		++m_size;
		return true;
	}

	// Insert into linked list

	const Uint32 newElementId = m_elementsPool.AllocateBlockIndex();

	TElement* newElement = static_cast< TElement* >( m_elementsPool.GetBlock( newElementId ) );
	RED_FATAL_ASSERT( newElement, "" );
	newElement->m_hash = hash;
	newElement->m_index = m_size;

	newElement->m_nextId = bucket->m_nextId;
	bucket->m_nextId = newElementId;

	++m_size;
	return true;
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE void THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::EraseKeyAt( Uint32 index )
{
	--m_size;
	if ( index < m_size )
	{
		// Move the last key into free slot

		m_keys[ index ] = Move( m_keys[ m_size ] );
		m_keys[ m_size ].~K();

		// Update pointer to moved key

		const K& movedKey = m_keys[ index ];
		const Uint32 hash = HashFunc::GetHash( movedKey );
		const Uint32 bucketIndex = hash % m_capacity;

		TBucket* bucket = m_buckets + bucketIndex;
		if ( bucket->IsUsed() )
		{
			if ( bucket->m_index == m_size )
			{
				bucket->m_index = index;
				return;
			}

			Uint32 currentId = bucket->m_nextId;
			while ( currentId != CPool::INVALID_INDEX )
			{
				TElement* current = static_cast< TElement* >( m_elementsPool.GetBlock( currentId ) );
				if ( current->m_index == m_size )
				{
					current->m_index = index;
					return;
				}
				currentId = current->m_nextId;
			}
		}

		ASSERT( !"Element to erase was not found in bucket. Should _never_ happen since the key is there." );
		return;
	}

	m_keys[ index ].~K();
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
template < typename KEY_COMPATIBLE >
Bool THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Erase( const KEY_COMPATIBLE& key )
{
	if ( !m_size )
	{
		return false;
	}

	const Uint32 hash = HashFunc::GetHash( key );
	const Uint32 bucketIndex = hash % m_capacity;

	TBucket* bucket = m_buckets + bucketIndex;

	// Check head

	if ( bucket->IsUsed() )
	{
		Uint32 index;
		if ( bucket->m_hash == hash && EqualFunc::Equal( m_keys[ index = bucket->m_index ], key ) )
		{
			// Remove the key

			EraseKeyAt( index );

			// Update list

			if ( bucket->m_nextId == CPool::INVALID_INDEX )
			{
				bucket->SetUnused();
			}
			else
			{
				TElement* next = static_cast< TElement* >( m_elementsPool.GetBlock( bucket->m_nextId ) );
				*( TElement* ) bucket = *next;
				m_elementsPool.FreeBlock( next );
			}

			PostErase();
			return true;
		}

		// Check linked list

		Uint32* currentId = &bucket->m_nextId;
		while ( *currentId != CPool::INVALID_INDEX )
		{
			TElement* current = static_cast< TElement* >( m_elementsPool.GetBlock( *currentId ) );
			Uint32 index;
			if ( current->m_hash == hash && EqualFunc::Equal( m_keys[ index = current->m_index ], key ) )
			{
				// Remove the key

				EraseKeyAt( index );

				// Update linked list

				*currentId = current->m_nextId;
				m_elementsPool.FreeBlock( current );

				PostErase();
				return true;
			}
			currentId = &current->m_nextId;
		}
	}

	return false;
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
template < typename KEY_COMPATIBLE >
Bool THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::EraseOldKey( const KEY_COMPATIBLE& oldKey, const K& key )
{
	if ( !m_size )
	{
		return false;
	}

	const Uint32 hash = HashFunc::GetHash( oldKey );
	const Uint32 bucketIndex = hash % m_capacity;

	TBucket* bucket = m_buckets + bucketIndex;

	// Check head

	if ( bucket->IsUsed() )
	{
		Uint32 index;
		if ( bucket->m_hash == hash && EqualFunc::Equal( m_keys[ index = bucket->m_index ], key ) )
		{
			// Remove the key

			EraseKeyAt( index );

			// Update list

			if ( bucket->m_nextId == CPool::INVALID_INDEX )
			{
				bucket->SetUnused();
			}
			else
			{
				TElement* next = static_cast< TElement* >( m_elementsPool.GetBlock( bucket->m_nextId ) );
				*( TElement* ) bucket = *next;
				m_elementsPool.FreeBlock( next );
			}

			PostErase();
			return true;
		}

		// Check linked list

		Uint32* currentId = &bucket->m_nextId;
		while ( *currentId != CPool::INVALID_INDEX )
		{
			TElement* current = static_cast< TElement* >( m_elementsPool.GetBlock( *currentId ) );
			Uint32 index;
			if ( current->m_hash == hash && EqualFunc::Equal( m_keys[ index = current->m_index ], key ) )
			{
				// Remove the key

				EraseKeyAt( index );

				// Update linked list

				*currentId = current->m_nextId;
				m_elementsPool.FreeBlock( current );

				PostErase();
				return true;
			}
			currentId = &current->m_nextId;
		}
	}

	return false;
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Bool THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Erase( const iterator& it )
{
	return Erase( *it.m_current );
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Clear()
{
	if ( m_capacity )
	{
		if ( m_size )
		{
			for ( Uint32 i = 0; i < m_size; ++i )
			{
				m_keys[ i ].~K();
			}
			m_size = 0;
		}

		RED_MEMORY_FREE( memoryPool, memoryClass, m_keys );
		m_keys = nullptr;
		m_capacity = 0;
	}
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::ClearPtr()
{
	if ( m_capacity )
	{
		if ( m_size )
		{
			for ( Uint32 i = 0; i < m_size; ++i )
			{
				delete m_keys[ i ];
			}
			m_size = 0;
		}

		RED_MEMORY_FREE( memoryPool, memoryClass, m_keys );
		m_keys = nullptr;
		m_capacity = 0;
	}
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::ClearFast()
{
	if ( m_size )
	{
		for ( Uint32 i = 0; i < m_size; ++i )
		{
			m_keys[ i ].~K();
		}
		m_size = 0;

		m_elementsPool.Clear();
		for ( Uint32 i = 0; i < m_capacity; ++i )
		{
			m_buckets[ i ].SetUnused();
			m_buckets[ i ].m_nextId = CPool::INVALID_INDEX;
		}
	}
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE void THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Reserve( Uint32 newCapacity )
{
	if ( m_capacity < newCapacity )
	{
		Rehash( newCapacity );
	}
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE void THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Shrink()
{
	Rehash( m_size );
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
template < typename KEY_COMPATIBLE >
RED_FORCE_INLINE typename THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::const_iterator THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Find( const KEY_COMPATIBLE& key ) const
{
	const K* found = FindInternal( key );
	return found ? const_iterator( found ) : End();
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
template < typename KEY_COMPATIBLE >
RED_FORCE_INLINE K* THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::FindPtr( const KEY_COMPATIBLE& key )
{
	return FindInternal( key );
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
template < typename KEY_COMPATIBLE >
RED_FORCE_INLINE typename THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::iterator THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Find( const KEY_COMPATIBLE& key )
{
	K* found = FindInternal( key );
	return found ? iterator( found ) : End();
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
template < typename KEY_COMPATIBLE >
RED_FORCE_INLINE Bool THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Exist( const KEY_COMPATIBLE& key ) const
{
	return FindInternal( key ) != nullptr;
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
Bool THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::operator == ( const THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >& other ) const
{
	if ( Size() != other.Size() )
	{
		return false;
	}

	for ( Uint32 i = 0; i < m_size; ++i )
	{
		if ( !other.FindInternal( m_keys[ i ] ) )
		{
			return false;
		}
	}

	return true;
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Bool THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::operator != ( const THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >& other ) const
{
	return !( *this == other );
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Add( const THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >& other )
{
	const Uint32 otherSize = other.m_size;
	for ( Uint32 i = 0; i < otherSize; ++i )
	{
		Insert( other.m_keys[ i ] );
	}
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Subtract( const THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >& other )
{
	const Uint32 otherSize = other.m_size;
	for ( Uint32 i = 0; i < otherSize; ++i )
	{
		Erase( other.m_keys[ i ] );
	}
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::GetKeys( TDynArray< K >& dst ) const
{
	dst.ClearFast();
	dst.Reserve( m_size );
	for ( Uint32 i = 0; i < m_size; ++i )
	{
		dst.PushBack( m_keys[ i ] );
	}
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::GetStats( THashContainerStats& stats )
{
	stats.m_size = m_size;
	stats.m_numBuckets = m_capacity;
	stats.m_numUsedBuckets = 0;
	stats.m_largestBucketSize = 0;

	for ( Uint32 i = 0; i < m_capacity; i++ )
	{
		// Count elements in bucket

		Uint32 bucketSize = 0;

		const TBucket* bucket = m_buckets + i;
		if ( bucket->IsUsed() )
		{
			++bucketSize;

			Uint32 currentId = bucket->m_nextId;
			while ( currentId != CPool::INVALID_INDEX )
			{
				const TElement* current = static_cast< TElement* >( m_elementsPool.GetBlock( currentId ) );
				++bucketSize;
				currentId = current->m_nextId;
			}
		}

		// Update stats

		if ( bucketSize )
		{
			++stats.m_numUsedBuckets;
		}

		if ( bucketSize > stats.m_largestBucketSize )
		{
			stats.m_largestBucketSize = bucketSize;
		}
	}

	stats.m_averageBucketSize = (Float) m_size / (Float) stats.m_numUsedBuckets;
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE void THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Upsize()
{
	ASSERT( m_size == m_capacity );

	// Grow to 150 % of the new size (and the minimum of 4)

	const Uint32 newCapacity = Max( ( Uint32 ) 4, m_size + ( m_size >> 1 ) );
	Rehash( newCapacity );
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE void THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::PostErase()
{
	// Only if "auto-downsize" option is enabled (which it isn't by default):
	// If capacity is over twice as big as size, then downsize to 150 % of the size

	if ( m_autoDownsize && m_size && ( m_size << 1 ) < m_capacity )
	{
		const Uint32 newCapacity = Max( ( Uint32 ) 4, m_size + ( m_size >> 1 ) );
		Rehash( newCapacity );
	}
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::Rehash( Uint32 newCapacity )
{
	RED_FATAL_ASSERT( newCapacity >= m_size, "" );

	if ( newCapacity == m_capacity )
	{
		return;
	}

	if ( !newCapacity )
	{
		RED_MEMORY_FREE( memoryPool, memoryClass, m_keys );
		m_keys = nullptr;
		m_capacity = 0;
		return;
	}

	// Copy old data aside

	Uint32 oldSize			= m_size;
	Uint32 oldCapacity		= m_capacity;
	CPool oldElementsPool;
	oldElementsPool			= Move( m_elementsPool );
	TBucket* oldBuckets		= m_buckets;
	K* oldKeys				= m_keys;

	// Allocate single memory block for the whole hash set

	const Uint32 newPoolSize		= newCapacity * sizeof( TElement );
	const Uint32 newBucketsSize		= newCapacity * sizeof( TBucket );
	const Uint32 newKeysSize		= newCapacity * sizeof( K );
	const Uint32 newMemorySize		= newPoolSize + newBucketsSize + newKeysSize;
	Uint8* newMemory = ( Uint8* ) RED_MEMORY_ALLOCATE( memoryPool, memoryClass, newMemorySize );
	RED_FATAL_ASSERT( newMemory, "" );

	// Initialize new internal containers

	m_capacity		= newCapacity;
	m_size			= 0;
	m_keys			= ( K* ) newMemory;			newMemory += newKeysSize;
	m_buckets		= ( TBucket* ) newMemory;	newMemory += newBucketsSize;
	m_elementsPool.Init( newMemory, newPoolSize, sizeof( TElement ) );

	for ( Uint32 i = 0; i < newCapacity; ++i )
	{
		m_buckets[ i ].SetUnused();
		m_buckets[ i ].m_nextId = CPool::INVALID_INDEX;
	}

	if ( oldSize )
	{
		// Move all elements to new containers

		for ( Uint32 i = 0; i < oldCapacity; ++i )
		{
			TBucket* oldBucket = oldBuckets + i;
			if ( oldBucket->IsUsed() )
			{
				K* key = oldKeys + oldBucket->m_index;
				InsertNoFail( Move( *key ), oldBucket->m_hash );
				key->~K();

				// Move linked list

				Uint32 currentId = oldBucket->m_nextId;
				while ( currentId != CPool::INVALID_INDEX )
				{
					TElement* current = static_cast< TElement* >( oldElementsPool.GetBlock( currentId ) );
					K* key = oldKeys + current->m_index;
					InsertNoFail( Move( *key ), current->m_hash );
					key->~K();
					currentId = current->m_nextId;
				}
			}
		}
	}

	// Delete old containers

	if ( oldCapacity )
	{
		RED_MEMORY_FREE( memoryPool, memoryClass, oldKeys );
	}
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::InsertNoFail( K&& key, Uint32 hash )
{
	const Uint32 bucketIndex = hash % m_capacity;
	TBucket* bucket = m_buckets + bucketIndex;

	// Insert key

	new ( m_keys + m_size ) K( key );

	// Try to insert into bucket head

	if ( !bucket->IsUsed() )
	{
		bucket->m_hash = hash;
		bucket->m_index = m_size;
		++m_size;
		return;
	}

	// Insert into linked list

	const Uint32 newElementId = m_elementsPool.AllocateBlockIndex();

	TElement* newElement = static_cast< TElement* >( m_elementsPool.GetBlock( newElementId ) );
	RED_FATAL_ASSERT( newElement, "" );
	newElement->m_hash = hash;
	newElement->m_index = m_size;

	newElement->m_nextId = bucket->m_nextId;
	bucket->m_nextId = newElementId;

	++m_size;
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
template < typename KEY_COMPATIBLE >
RED_FORCE_INLINE K* THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::FindInternal( const KEY_COMPATIBLE& key )
{
	return const_cast< K* >( const_cast< const THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >* >( this )->FindInternal( key ) );
}

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
template < typename KEY_COMPATIBLE >
const K* THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::FindInternal( const KEY_COMPATIBLE& key ) const
{
	if ( !m_size )
	{
		return nullptr;
	}

	const Uint32 hash = HashFunc::GetHash( key );
	const Uint32 bucketIndex = hash % m_capacity;

	// Search

	const TBucket* bucket = m_buckets + bucketIndex;
	if ( bucket->IsUsed() )
	{
		Uint32 currentIndex;
		if ( bucket->m_hash == hash && EqualFunc::Equal( m_keys[ currentIndex = bucket->m_index ], key ) )
		{
			return m_keys + currentIndex;
		}

		// Search in linked list

		Uint32 currentId = bucket->m_nextId;
		while ( currentId != CPool::INVALID_INDEX )
		{
			const TElement* current = static_cast< const TElement* >( m_elementsPool.GetBlock( currentId ) );
			Uint32 currentIndex;
			if ( current->m_hash == hash && EqualFunc::Equal( m_keys[ currentIndex = current->m_index ], key ) )
			{
				return m_keys + currentIndex;
			}
			currentId = current->m_nextId;
		}
	}

	// Not found

	return nullptr;
}


// Enable C++11 range-based for loop

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
typename THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::iterator begin( THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >& set ) { return set.Begin(); }

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
typename THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::iterator end( THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >& set ) { return set.End(); }

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
typename THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::const_iterator begin( const THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >& set ) { return set.Begin(); }

template < typename K, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
typename THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >::const_iterator end( const THashSet< K, HashFunc, EqualFunc, memoryClass, memoryPool >& set ) { return set.End(); }
