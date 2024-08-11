#pragma once

#include "../../common/core/pair.h"
#include "../../common/core/hash.h"
#include "../../common/core/pool.h"

/**
 *	Hash map
 *
 *	Notes on performance:
 *	- expect FAST insert()/erase()/find() of O(1) on average
 *	- optimized for caches: first bucket element is stored in-place (no pointer jumping to check the hash)
 *	- fully supports Move() semantics
 *
 *	Notes on implementation:
 *	- only uses single memory allocation for all of its internal data
 *	- only reallocates its memory on rehash
 *	- attempts to maintain bucket count such as to make insert/erase/find operations fast:
 *		* when inserting: no less than N and no more than 1.5N capacity
 *		* when erasing (only if "auto-downsize" enabled): no more than 2N and no less than 1.5N capacity
 *  - caches hash value per element to speed up look ups
 *	- overhead per element: sizeof( TBucket ) + sizeof( TElement ) = 24 bytes
 */
template <
	typename K,
	typename V,
	typename HashFunc = DefaultHashFunc< K >,
	typename EqualFunc = DefaultEqualFunc< K >,
	EMemoryClass memoryClass = MC_HashSet,
	RED_CONTAINER_POOL_TYPE memoryPool = MemoryPool_Default >
class THashMap_continuous
{
private:
	static const Uint32 UNUSED_ELEMENT_INDEX = 0xFFFFFFFF;

	struct TBaseElement
	{
		Uint32	m_hash;			// Cached hash of the key
		Uint32	m_index;		// Index of the pair in m_pairs; UNUSED_ELEMENT_INDEX if element is unused
	};

	struct TElement : TBaseElement
	{
		Uint32	m_nextId;		// Id of the next element in the bucket; CPool::INVALID_INDEX if there's no next element
	};

	// Each bucket stores first element in-place; every next element is on the linked list
	struct TBucket : TElement
	{
		RED_FORCE_INLINE Uint32 IsUsed() const { return TBaseElement::m_index != UNUSED_ELEMENT_INDEX; }
		RED_FORCE_INLINE void SetUnused() { TBaseElement::m_index = UNUSED_ELEMENT_INDEX; }
	};

	Uint32			m_capacity;		// Capacity
	Uint32			m_size;			// Number of elements stored
	TBucket*		m_buckets;		// Buckets with lists of elements with matching hashes modulo m_capacity
	TPair< K, V >*	m_pairs;		// An array of key value pairs
	CPool			m_elementsPool;	// Pool of TElements for use in linked lists
	Bool			m_autoDownsize;	// If map automatically downsizes internal buffers on Erase(); NOTE: Downsizes to 150% of the size only if exceeding 200% of the size

public:

	// Iterator
	class iterator
	{
		friend class THashMap_continuous;
	private:
		TPair< K, V >* m_current;
		RED_FORCE_INLINE iterator( TPair< K, V >* current ) : m_current( current ) {}
		RED_FORCE_INLINE iterator( THashMap_continuous* map ) : m_current( map->m_pairs ) {}
	public:
		RED_FORCE_INLINE iterator() : m_current( nullptr ) {}
		RED_FORCE_INLINE iterator( const iterator& it ) : m_current( it.m_current ) {}
		RED_FORCE_INLINE iterator& operator = ( const iterator it ) { m_current = it.m_current; return *this; }
		RED_FORCE_INLINE TPair< K, V >& operator*() const { return *m_current; }
		RED_FORCE_INLINE TPair< K, V >* operator->() const { return m_current; }
		RED_FORCE_INLINE const K& Key() const { return m_current->m_first; }
		RED_FORCE_INLINE V& Value() { return m_current->m_second; }
		RED_FORCE_INLINE Bool operator == ( const iterator it ) const { return m_current == it.m_current; }
		RED_FORCE_INLINE Bool operator != ( const iterator it ) const { return m_current != it.m_current; }
		RED_FORCE_INLINE iterator& operator ++ () { ++m_current; return *this; }
	};

	// Const iterator
	class const_iterator
	{
		friend class THashMap_continuous;
	private:
		const TPair< K, V >* m_current;
		RED_FORCE_INLINE const_iterator( const TPair< K, V >* current ) : m_current( current ) {}
		RED_FORCE_INLINE const_iterator( const THashMap_continuous* map ) : m_current( map->m_pairs ) {}
	public:
		RED_FORCE_INLINE const_iterator() : m_current( nullptr ) {}
		RED_FORCE_INLINE const_iterator( const const_iterator& it ) { m_current = it.m_current; }
		RED_FORCE_INLINE const_iterator( const iterator it ) { m_current = it.m_current; }
		RED_FORCE_INLINE const_iterator& operator = ( const const_iterator it ) { m_current = it.m_current; return *this; }
		RED_FORCE_INLINE const_iterator& operator = ( const iterator it ) { m_current = it.m_current; return *this; }
		RED_FORCE_INLINE const TPair< K, V >& operator*() const { return *m_current; }
		RED_FORCE_INLINE const TPair< K, V >* operator->() const { return m_current; }
		RED_FORCE_INLINE const K& Key() const { return m_current->m_first; }
		RED_FORCE_INLINE const V& Value() const { return m_current->m_second; }
		RED_FORCE_INLINE Bool operator == ( const iterator it ) const { return m_current == it.m_current; }
		RED_FORCE_INLINE Bool operator != ( const iterator it ) const { return m_current != it.m_current; }
		RED_FORCE_INLINE Bool operator == ( const const_iterator it ) const { return m_current == it.m_current; }
		RED_FORCE_INLINE Bool operator != ( const const_iterator it ) const { return m_current != it.m_current; }
		RED_FORCE_INLINE const_iterator& operator ++ () { ++m_current; return *this; }
	};

	// Initializes hash map with 'initialCapacity' empty slots
	THashMap_continuous( Uint32 initialCapacity = 0 );
	// Copy constructor
	THashMap_continuous( const THashMap_continuous& other );
	// Move constructor
	THashMap_continuous( THashMap_continuous&& other );
	// Removes all elements and deallocates memory
	~THashMap_continuous();

	// Removes all elements and deallocates memory
	void Clear();
	// Quickly removes all elements; does not deallocate memory
	void ClearFast();
	// Removes all elements, deletes all values and deallocates memory
	void ClearPtr();
	// Quickly removes all elements and deletes all values; does not deallocate memory
	void ClearPtrFast();
	// Reallocates memory so as to ensure 'newCapacity' elements can be stored without memory reallocation
	RED_FORCE_INLINE void Reserve( Uint32 newCapacity );
	// Minimizes memory usage (changes capacity to match the size)
	RED_FORCE_INLINE void Shrink();

	RED_FORCE_INLINE Uint32 Size() const;
	RED_FORCE_INLINE Bool Empty() const;
	RED_FORCE_INLINE Uint32 Capacity() const;
	// Enables automatic downsize of the buffer on Erase()
	RED_FORCE_INLINE void EnableAutoDownsize( Bool enable );
	RED_FORCE_INLINE Uint32 DataSize() const;
	// Gets total internal memory size used by the hash map
	RED_FORCE_INLINE TMemSize GetInternalMemSize() const;

	// Inserts new element, fails if key exists unless 'failIfExists' is set to false; returns true on success
	Bool Insert( const K& key, const V& value, Bool failIfExists = true );
	// Inserts new element, fails if key exists unless 'failIfExists' is set to false; returns true on success
	Bool Insert( const K& key, V&& value, Bool failIfExists = true );
	// Inserts new element; returns true on success
	RED_FORCE_INLINE void Set( const K& key, const V& value );
	// Returns reference to value at given key; if entry didn't exist, it gets added
	V& operator [] ( const K& key );
	// Returns const reference to value at given key; if entry didn't exist, it gets added
	const V& operator [] ( const K& key ) const;
	// Find given key or add it ( with default value )
	V& GetRef( const K& key, const V& defaultValue = V() );
	// Erases key; returns true on success
	Bool Erase( const K& key );
	// Erases key by iterator; returns true on success
	Bool Erase( const iterator& it );
	// Erases key by value; returns true on success
	Bool EraseByValue( const V& value );
	// Finds key; returns iterator to key or End() if key is not found
	RED_FORCE_INLINE const_iterator Find( const K& key ) const;
	// Finds key; returns iterator to key or End() if key is not found
	RED_FORCE_INLINE iterator Find( const K& key );
	// Finds key; returns pointer to key or nullptr if key is not found
	RED_FORCE_INLINE const V* FindPtr( const K& key ) const;
	// Finds key; returns pointer to key or nullptr if key is not found
	RED_FORCE_INLINE V* FindPtr( const K& key );
	// Gets value at given key; returns true on success
	RED_FORCE_INLINE Bool Find( const K& key, V& value ) const;
	// Finds an element by value
	RED_INLINE iterator FindByValue( const V& value );
	// Checks if key exists in map
	RED_FORCE_INLINE Bool KeyExist( const K& key ) const;

	// Copy operator
	THashMap_continuous& operator = ( const THashMap_continuous& other );
	// Move operator
	THashMap_continuous& operator = ( THashMap_continuous&& other );
	Bool operator == ( const THashMap_continuous& other ) const;
	Bool operator != ( const THashMap_continuous& other ) const;
	// Adds elements from given map to this map
	void Add( const THashMap_continuous& other );
	// Removes given elements from this map
	void Subtract( const THashMap_continuous& other );
	// Merges elements from multiple maps into one
	static void Merge( THashMap_continuous& out, THashMap_continuous** sources, Uint32 numSources );
	// Gets all keys into provided array
	void GetKeys( TDynArray< K >& dst ) const;
	// Gets all values into provided array
	void GetValues( TDynArray< V >& dst ) const;

	RED_FORCE_INLINE const_iterator Begin() const { return const_iterator( m_pairs ); }
	RED_FORCE_INLINE const_iterator End() const { return const_iterator( m_pairs + m_size ); }
	RED_FORCE_INLINE iterator Begin() { return iterator( m_pairs ); }
	RED_FORCE_INLINE iterator End() { return iterator( m_pairs + m_size ); }

	// Gets hash map stats
	void GetStats( THashContainerStats& stats );
	// Gets user friendly type name
	static const CName& GetTypeName();

private:
	RED_FORCE_INLINE void Upsize();
	RED_FORCE_INLINE void PostErase();
	void Rehash( Uint32 newCapacity );
	void InsertNoFail( TPair< K, V >&& pair, Uint32 hash );
	RED_FORCE_INLINE void ErasePairAt( Uint32 index );
	RED_FORCE_INLINE TPair< K, V >* FindInternal( const K& key );
	const TPair< K, V >* FindInternal( const K& key ) const;

public:
	// Serialization
	friend IFile& operator << ( IFile& file, THashMap_continuous& map )
	{
		if ( file.IsReader() )
		{
			map.Clear();
			if ( file.GetVersion() >= VER_NEW_HASHMAP )
			{
				Uint32 size;
				file << size;
				map.Reserve( size );

				TPair< K, V > pair;
				for ( Uint32 i = 0; i < size; ++i )
				{
					file << pair;
					map.InsertNoFail( Move( pair ), HashFunc::GetHash( pair.m_first ) );
				}
			}
			else
			{
				// Read in from old format

				typedef TDynArray< Uint32, memoryClass > TBucket;
				TDynArray< TPair< K, V >, memoryClass > proxy;
				TDynArray< TBucket > bucketArray;

				file << proxy;
				file << bucketArray;

				map.Rehash( proxy.Size() );
				for ( auto it = proxy.Begin(); it != proxy.End(); ++it )
				{
					map.Insert( it->m_first, it->m_second );
				}
			}
		}
		else if ( file.IsWriter() && !file.IsGarbageCollector() )
		{
			// Make sure hash map serialization is deterministic by maintaining deterministic (depending on hash map size) number of buckets
			// Note: Shrink() will do nothing if size equals capacity already

			map.Shrink();

			// Serialize the hash map

			file << map.m_size;
			for ( Uint32 i = 0; i < map.m_size; ++i )
			{
				file << map.m_pairs[ i ];
			}
		}
		else
		{
			file << map.m_size;
			for ( Uint32 i = 0; i < map.m_size; ++i )
			{
				file << map.m_pairs[ i ];
			}
		}

		return file;
	}
};

// Template implementation

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::THashMap_continuous( Uint32 initialCapacity )
	: m_buckets( nullptr )
	, m_size( 0 )
	, m_capacity( 0 )
	, m_pairs( nullptr )
	, m_autoDownsize( false )
{
	Rehash( initialCapacity );
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::THashMap_continuous( const THashMap_continuous& other )
	: m_buckets( nullptr )
	, m_size( 0 )
	, m_capacity( 0 )
	, m_pairs( nullptr )
	, m_autoDownsize( false )
{
	*this = other;
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::THashMap_continuous( THashMap_continuous&& other )
	: m_buckets( nullptr )
	, m_size( 0 )
	, m_capacity( 0 )
	, m_pairs( nullptr )
	, m_autoDownsize( false )
{
	*this = Move( other );
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >& THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::operator = ( const THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >& other )
{
	if ( this == &other )
	{
		return *this;
	}

	ClearFast();
	const Uint32 otherSize = other.m_size;
	Rehash( otherSize );
	for ( Uint32 i = 0; i < otherSize; ++i )
	{
		Insert( other.m_pairs[ i ].m_first, other.m_pairs[ i ].m_second );
	}

	return *this;
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >& THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::operator = ( THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >&& other )
{
	if ( this == &other )
	{
		return *this;
	}

	Clear();

	m_elementsPool	= Move( other.m_elementsPool );
	m_pairs			= other.m_pairs;				other.m_pairs = nullptr;
	m_buckets		= other.m_buckets;				other.m_buckets = nullptr;
	m_size			= other.m_size;					other.m_size = 0;
	m_capacity		= other.m_capacity;				other.m_capacity = 0;
	m_autoDownsize	= other.m_autoDownsize;			other.m_autoDownsize = false;

	return *this;
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::~THashMap_continuous()
{
	Clear();
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::EnableAutoDownsize( Bool enable )
{
	m_autoDownsize = enable;
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Uint32 THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Size() const
{
	return m_size;
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Uint32 THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::DataSize() const
{
	return m_size * sizeof( K );
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE TMemSize THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::GetInternalMemSize() const
{
	return m_capacity * ( sizeof( TElement ) + sizeof( TBucket ) + sizeof( K ) );
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Uint32 THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Capacity() const
{
	return m_capacity;
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Bool THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Empty() const
{
	return !m_size;
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE V& THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::operator [] ( const K& key )
{
	return GetRef( key );
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE const V& THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::operator [] ( const K& key ) const
{
	const V* valuePtr = FindPtr( key );
	ASSERT( valuePtr );
	return *valuePtr;
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Bool THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Find( const K& key, V& value ) const
{
	if ( const TPair< K, V >* pair = FindInternal( key ) )
	{
		value = pair->m_second;
		return true;
	}
	return false;
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Set( const K& key, const V& value )
{
	Insert( key, value, false );
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Bool THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Insert( const K& key, V&& value, Bool failIfExists )
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
			Uint32 index;
			if ( bucket->m_hash == hash && EqualFunc::Equal( m_pairs[ index = bucket->m_index ].m_first, key ) )
			{
				if ( !failIfExists )
				{
					m_pairs[ index ].m_second = value;
				}
				return failIfExists;
			}

			// Check linked list

			Uint32 currentId = bucket->m_nextId;
			while ( currentId != CPool::INVALID_INDEX )
			{
				TElement* current = static_cast< TElement* >( m_elementsPool.GetBlock( currentId ) );
				if ( current->m_hash == hash && EqualFunc::Equal( m_pairs[ index = current->m_index ].m_first, key ) )
				{
					if ( !failIfExists )
					{
						m_pairs[ index ].m_second = value;
					}
					return failIfExists;
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

	new ( m_pairs + m_size ) TPair< K, V >( key, value );

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

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Bool THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Insert( const K& key, const V& value, Bool failIfExists )
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
			Uint32 index;
			if ( bucket->m_hash == hash && EqualFunc::Equal( m_pairs[ index = bucket->m_index ].m_first, key ) )
			{
				if ( !failIfExists )
				{
					m_pairs[ index ].m_second = value;
				}
				return failIfExists;
			}

			// Check linked list

			Uint32 currentId = bucket->m_nextId;
			while ( currentId != CPool::INVALID_INDEX )
			{
				TElement* current = static_cast< TElement* >( m_elementsPool.GetBlock( currentId ) );
				if ( current->m_hash == hash && EqualFunc::Equal( m_pairs[ index = current->m_index ].m_first, key ) )
				{
					if ( !failIfExists )
					{
						m_pairs[ index ].m_second = value;
					}
					return failIfExists;
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

	new ( m_pairs + m_size ) TPair< K, V >( key, value );

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

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE V& THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::GetRef( const K& key, const V& defaultValue )
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
			Uint32 index;
			if ( bucket->m_hash == hash && EqualFunc::Equal( m_pairs[ index = bucket->m_index ].m_first, key ) )
			{
				return m_pairs[ index ].m_second;
			}

			// Check linked list

			Uint32 currentId = bucket->m_nextId;
			while ( currentId != CPool::INVALID_INDEX )
			{
				TElement* current = static_cast< TElement* >( m_elementsPool.GetBlock( currentId ) );
				if ( current->m_hash == hash && EqualFunc::Equal( m_pairs[ index = current->m_index ].m_first, key ) )
				{
					return m_pairs[ index ].m_second;
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

	TPair< K, V >* newPair = new ( m_pairs + m_size ) TPair< K, V >( key, defaultValue );

	// Try to insert into bucket head

	TBucket* bucket = m_buckets + bucketIndex;
	if ( !bucket->IsUsed() )
	{
		bucket->m_hash = hash;
		bucket->m_index = m_size;
		++m_size;
		return newPair->m_second;
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
	return newPair->m_second;
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::ErasePairAt( Uint32 index )
{
	--m_size;
	if ( index < m_size )
	{
		// Move the last pair into free slot

		m_pairs[ index ] = Move( m_pairs[ m_size ] );
		m_pairs[ m_size ].~TPair< K, V >();

		// Get moved element's bucket

		const TPair< K, V>& movedPair = m_pairs[ index ];
		const Uint32 hash = HashFunc::GetHash( movedPair.m_first );
		const Uint32 bucketIndex = hash % m_capacity;

		TBucket* bucket = m_buckets + bucketIndex;
		ASSERT( bucket->IsUsed() );

		// Try to update pointer to moved element in head

		if ( bucket->m_index == m_size )
		{
			bucket->m_index = index;
			return;
		}

		// Update pointer to moved element in linked list

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

		ASSERT( !"Element to erase was not found in bucket. Should _never_ happen since the key is there." );
		return;
	}

	m_pairs[ index ].~TPair< K, V >();
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
Bool THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::EraseByValue( const V& value )
{
	for ( Uint32 i = 0; i < m_size; ++i )
	{
		if ( m_pairs[ i ].m_second == value )
		{
			Erase( m_pairs[ i ].m_first );
			return true;
		}
	}
	return false;
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
Bool THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Erase( const K& key )
{
	if ( !m_size )
	{
		return false;
	}

	const Uint32 hash = HashFunc::GetHash( key );
	const Uint32 bucketIndex = hash % m_capacity;

	TBucket* bucket = m_buckets + bucketIndex;
	if ( bucket->IsUsed() )
	{
		Uint32 index;
		if ( bucket->m_hash == hash && EqualFunc::Equal( m_pairs[ index = bucket->m_index ].m_first, key ) )
		{
			// Remove the key

			ErasePairAt( index );

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
			if ( current->m_hash == hash && EqualFunc::Equal( m_pairs[ index = current->m_index ].m_first, key ) )
			{
				// Remove the key

				ErasePairAt( index );

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

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Bool THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Erase( const iterator& it )
{
	return Erase( it.m_current->m_first );
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Clear()
{
	if ( m_capacity )
	{
		if ( m_size )
		{
			for ( Uint32 i = 0; i < m_size; ++i )
			{
				m_pairs[ i ].~TPair< K, V >();
			}
			m_size = 0;
		}

		RED_MEMORY_FREE( memoryPool, memoryClass, m_pairs );
		m_pairs = nullptr;
		m_capacity = 0;
	}
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::ClearFast()
{
	if ( m_size )
	{
		for ( Uint32 i = 0; i < m_size; ++i )
		{
			m_pairs[ i ].~TPair< K, V >();
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

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::ClearPtr()
{
	if ( m_capacity )
	{
		if ( m_size )
		{
			for ( Uint32 i = 0; i < m_size; ++i )
			{
				delete m_pairs[ i ].m_second;
				m_pairs[ i ].~TPair< K, V >();
			}
			m_size = 0;
		}

		RED_MEMORY_FREE( memoryPool, memoryClass, m_pairs );
		m_pairs = nullptr;
		m_capacity = 0;
	}
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::ClearPtrFast()
{
	if ( m_size )
	{
		for ( Uint32 i = 0; i < m_size; ++i )
		{
			delete m_pairs[ i ].m_second;
			m_pairs[ i ].~TPair< K, V >();
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

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Reserve( Uint32 newCapacity )
{
	if ( m_capacity < newCapacity )
	{
		Rehash( newCapacity );
	}
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Shrink()
{
	Rehash( m_size );
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE typename THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::const_iterator THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Find( const K& key ) const
{
	const TPair< K, V >* pair = FindInternal( key );
	return pair ? const_iterator( pair ) : End();
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE typename THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::iterator THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Find( const K& key )
{
	TPair< K, V >* pair = FindInternal( key );
	return pair ? iterator( pair ) : End();
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE const V* THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::FindPtr( const K& key ) const
{
	const TPair< K, V >* pair = FindInternal( key );
	return pair ? &pair->m_second : nullptr;
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE V* THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::FindPtr( const K& key )
{
	TPair< K, V >* pair = FindInternal( key );
	return pair ? &pair->m_second : nullptr;
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
typename THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::iterator THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::FindByValue( const V& value )
{
	for ( Uint32 i = 0; i < m_size; ++i )
	{
		if ( m_pairs[ i ].m_second == value )
		{
			return iterator( m_pairs + i );
		}
	}

	return End();
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Bool THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::KeyExist( const K& key ) const
{
	return FindInternal( key ) != nullptr;
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
Bool THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::operator == ( const THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >& other ) const
{
	if ( Size() != other.Size() )
	{
		return false;
	}

	for ( Uint32 i = 0; i < m_size; ++i )
	{
		const TPair< K, V >* pairInOther = other.FindPtr( m_pairs[ i ] );
		if ( !pairInOther || *pairInOther != m_pairs[ i ] )
		{
			return false;
		}
	}

	return true;
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE Bool THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::operator != ( const THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >& other ) const
{
	return !( *this == other );
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Add( const THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >& other )
{
	const Uint32 otherSize = other.m_size;
	for ( Uint32 i = 0; i < otherSize; ++i )
	{
		Insert( other.m_pairs[ i ].m_first, other.m_pairs[ i ].m_second );
	}
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Subtract( const THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >& other )
{
	const Uint32 otherSize = other.m_size;
	for ( Uint32 i = 0; i < otherSize; ++i )
	{
		Erase( other.m_pairs[ i ].m_first );
	}
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Merge( THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >& out, THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >** sources, Uint32 numSources )
{
	out.ClearFast();
	for ( Uint32 i = 0; i < numSources; ++i )
	{
		out.Add( *sources [ i ] );
	}
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::GetKeys( TDynArray< K >& dst ) const
{
	dst.ClearFast();
	dst.Reserve( m_size );
	for ( Uint32 i = 0; i < m_size; ++i )
	{
		dst.PushBack( m_pairs[ i ].m_first );
	}
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::GetValues( TDynArray< V >& dst ) const
{
	dst.ClearFast();
	dst.Reserve( m_size );
	for ( Uint32 i = 0; i < m_size; ++i )
	{
		dst.PushBack( m_pairs[ i ].m_second );
	}
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::GetStats( THashContainerStats& stats )
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

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Upsize()
{
	ASSERT( m_size == m_capacity );

	// Grow to 150 % of the new size (and the minimum of 4)

	const Uint32 newCapacity = Max( ( Uint32 ) 4, m_size + ( m_size >> 1 ) );
	Rehash( newCapacity );
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::PostErase()
{
	// Only if "auto-downsize" option is enabled (which it isn't by default):
	// If capacity is over twice as big as size, then downsize to 150 % of the size

	if ( m_autoDownsize && m_size && ( m_size << 1 ) < m_capacity )
	{
		const Uint32 newCapacity = Max( ( Uint32 ) 4, m_size + ( m_size >> 1 ) );
		Rehash( newCapacity );
	}
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::Rehash( Uint32 newCapacity )
{
	RED_FATAL_ASSERT( newCapacity >= m_size, "" );

	if ( newCapacity == m_capacity )
	{
		return;
	}

	if ( !newCapacity )
	{
		RED_MEMORY_FREE( memoryPool, memoryClass, m_pairs );
		m_pairs = nullptr;
		m_capacity = 0;
		return;
	}

	// Copy old data aside

	Uint32 oldSize			= m_size;
	Uint32 oldCapacity		= m_capacity;
	CPool oldElementsPool;
	oldElementsPool			= Move( m_elementsPool );
	TBucket* oldBuckets		= m_buckets;
	TPair< K, V >* oldPairs	= m_pairs;

	// Allocate single memory block for the whole hash map

	const Uint32 newPoolSize	= newCapacity * sizeof( TElement );
	const Uint32 newBucketsSize = newCapacity * sizeof( TBucket );
	const Uint32 newPairsSize	= newCapacity * sizeof( TPair< K, V > );
	const Uint32 newMemorySize	= newPoolSize + newBucketsSize + newPairsSize;
	Uint8* newMemory = ( Uint8* ) RED_MEMORY_ALLOCATE( memoryPool, memoryClass, newMemorySize );
	RED_FATAL_ASSERT( newMemory, "" );

	// Initialize new internal containers

	m_capacity	= newCapacity;
	m_size		= 0;
	m_pairs		= ( TPair< K, V >* ) newMemory; newMemory += newPairsSize;
	m_buckets	= ( TBucket* ) newMemory; newMemory += newBucketsSize;
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
				TPair< K, V >* pair = oldPairs + oldBucket->m_index;
				InsertNoFail( Move( *pair ), oldBucket->m_hash );
				pair->~TPair< K, V >();

				// Move linked list

				Uint32 currentId = oldBucket->m_nextId;
				while ( currentId != CPool::INVALID_INDEX )
				{
					TElement* current = static_cast< TElement* >( oldElementsPool.GetBlock( currentId ) );
					TPair< K, V >* pair = oldPairs + current->m_index;
					InsertNoFail( Move( *pair ), current->m_hash );
					pair->~TPair< K, V >();
					currentId = current->m_nextId;
				}
			}
		}
	}

	// Delete old containers

	if ( oldCapacity )
	{
		RED_MEMORY_FREE( memoryPool, memoryClass, oldPairs );
	}
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::InsertNoFail( TPair< K, V >&& pair, Uint32 hash )
{
	const Uint32 bucketIndex = hash % m_capacity;
	TBucket* bucket = m_buckets + bucketIndex;

	// Insert key

	new ( m_pairs + m_size ) TPair< K, V >( pair );

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

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE TPair< K, V >* THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::FindInternal( const K& key )
{
	return const_cast< TPair< K, V >* >( const_cast< const THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >* >( this )->FindInternal( key ) );
}

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
const TPair< K, V >* THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::FindInternal( const K& key ) const
{
	if ( !m_size )
	{
		return nullptr;
	}

	const Uint32 hash = HashFunc::GetHash( key );
	const Uint32 bucketIndex = hash % m_capacity;

	const TBucket* bucket = m_buckets + bucketIndex;
	if ( bucket->IsUsed() )
	{
		Uint32 currentIndex;
		if ( bucket->m_hash == hash && EqualFunc::Equal( m_pairs[ currentIndex = bucket->m_index ].m_first, key ) )
		{
			return m_pairs + currentIndex;
		}

		// Search in linked list

		Uint32 currentId = bucket->m_nextId;
		while ( currentId != CPool::INVALID_INDEX )
		{
			const TElement* current = static_cast< const TElement* >( m_elementsPool.GetBlock( currentId ) );
			Uint32 currentIndex;
			if ( current->m_hash == hash && EqualFunc::Equal( m_pairs[ currentIndex = current->m_index ].m_first, key ) )
			{
				return m_pairs + currentIndex;
			}
			currentId = current->m_nextId;
		}
	}

	// Not found

	return nullptr;
}


// Enable C++11 range-based for loop

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
typename THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::iterator begin( THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >& map ) { return map.Begin(); }

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
typename THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::iterator end( THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >& map ) { return map.End(); }

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
typename THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::const_iterator begin( const THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >& map ) { return map.Begin(); }

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
typename THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >::const_iterator end( const THashMap_continuous< K, V, HashFunc, EqualFunc, memoryClass, memoryPool >& map ) { return map.End(); }