#pragma once

#include "pair.h"
#include "pool.h"
#include "hash.h"

/**
 *	Hash map
 *
 *	Notes on performance:
 *	- expect FAST insert()/erase()/find() of O(1) on average (average bucket size across various hash maps used in the engine was between 1.5 and 2)
 *	- per element memory overhead of sizeof(void*) * 2 + sizeof(Uint32) = 20 bytes on 64 bit architecture, 12 bytes on 32 bit architecture
 *	- fully supports Move() semantics
 *
 *	Notes on implementation:
 *	- only uses single memory allocation for all of its internal data
 *	- only reallocates its memory on rehash
 *	- attempts to maintain bucket count such as to make insert/erase/find operations fast:
 *		* when inserting: no less than N and no more than 1.5N capacity
 *		* when erasing (only if "auto-downsize" enabled): no more than 2N and no less than 1.5N capacity
 *  - caches hash value per element to speed up look ups
 */
template < typename K, typename V, typename HashFunc = DefaultHashFunc< K >, typename EqualFunc = DefaultEqualFunc< K >, EMemoryClass MC_Type = MC_HashMap, RED_CONTAINER_POOL_TYPE memoryPool = MemoryPool_Default >
class THashMap
{
private:
	struct TElement : TPair< K, V >
	{
		Uint32 m_hash;		// Cached hash of the key
		TElement* m_next;	// Pointer to the next element in the bucket

		RED_FORCE_INLINE TElement( const K& key )
			: TPair< K, V >( key, V() )
		{}
		RED_FORCE_INLINE TElement( const K& key, const V& value )
			: TPair< K, V >( key, value )
		{}

		RED_FORCE_INLINE TElement( const K& key, V&& value )
			: TPair< K, V >( key, Move( value ) )
		{}

		RED_FORCE_INLINE TElement( const TElement& element )
			: TPair< K, V >( element )
		{}

		RED_FORCE_INLINE TElement( TElement&& element )
			: TPair< K, V >( Move( element ) )
		{}
	};

	Uint32		m_capacity;		// Capacity
	Uint32		m_size;			// Number of elements stored
	CPool		m_pool;			// Fixed size memory block pool
	TElement**	m_buckets;		// Buckets with lists of elements with matching hashes
	Bool		m_autoDownsize;	// If set automatically downsizes internal buffers on Erase(); NOTE: Downsizes to 150% of the size only if exceeding 200% of the size

public:

	// Iterator
	class iterator
	{
	private:
		friend class THashMap;

		THashMap*	m_map;
		Uint32		m_bucketIndex;
		TElement*	m_element;

		RED_FORCE_INLINE iterator( THashMap* map, Uint32 bucketIndex, TElement* element )
			: m_map( map )
			, m_bucketIndex( bucketIndex )
			, m_element ( element )
		{
		}

		RED_FORCE_INLINE iterator( THashMap* map )
			: m_map( map )
			, m_bucketIndex( 0 )
			, m_element ( nullptr )
		{
			// Move to the first element

			if ( !m_map->m_size )
			{
				m_bucketIndex = m_map->m_capacity;
			}
			else while ( m_bucketIndex < m_map->m_capacity )
			{
				if ( m_map->m_buckets[ m_bucketIndex ] )
				{
					m_element = m_map->m_buckets[ m_bucketIndex ];
					break;
				}

				++m_bucketIndex;
			}
		}

	public:

		RED_FORCE_INLINE iterator()
			: m_map( nullptr )
		{
		}

		RED_FORCE_INLINE iterator( const iterator & it )
		{
			*this = it;
		}

		RED_FORCE_INLINE iterator& operator = ( const iterator & it )
		{
			m_map			= it.m_map;
			m_bucketIndex	= it.m_bucketIndex;
			m_element		= it.m_element;
			return *this;
		}

		RED_FORCE_INLINE const K& Key() const
		{
			RED_FATAL_ASSERT( m_element, "" );
			return m_element->m_first;
		}

		RED_FORCE_INLINE V& Value() const
		{
			RED_FATAL_ASSERT( m_element, "" );
			return m_element->m_second;
		}

		RED_FORCE_INLINE TPair<K, V>& operator*() const
		{
			RED_FATAL_ASSERT( m_element, "" );
			return *m_element;
		}

		RED_FORCE_INLINE TPair<K,V>* operator->() const
		{
			RED_FATAL_ASSERT( m_element, "" );
			return m_element;
		}

		RED_FORCE_INLINE Bool operator == ( const iterator& it ) const
		{
			return m_element == it.m_element;
		}

		RED_FORCE_INLINE Bool operator != ( const iterator& it ) const
		{
			return m_element != it.m_element;
		}

		RED_FORCE_INLINE iterator& operator ++ ()
		{
			// Reached the end

			if ( m_bucketIndex >= m_map->m_capacity)
			{
				return *this;
			}

			// Check next element on the list

			if ( m_element->m_next )
			{
				m_element = m_element->m_next;
				return *this;
			}

			// Check next non-empty bucket

			while ( ++m_bucketIndex < m_map->m_capacity )
			{
				if ( m_map->m_buckets[ m_bucketIndex ] )
				{
					m_element = m_map->m_buckets[ m_bucketIndex ];
					return *this;
				}
			}

			// Reached the end

			m_element = nullptr;
			return *this;
		}
	};

	// Const iterator
	class const_iterator
	{
	private:
		friend class THashMap;

		const THashMap*	m_map;
		Uint32			m_bucketIndex;
		const TElement*	m_element;

		RED_FORCE_INLINE const_iterator( const THashMap* map, Uint32 bucketIndex, const TElement* element )
			: m_map( map )
			, m_bucketIndex( bucketIndex )
			, m_element ( element )
		{
		}

		RED_FORCE_INLINE const_iterator( const THashMap* map )
			: m_map( map )
			, m_bucketIndex( 0 )
			, m_element ( nullptr )
		{
			// Move to the first element

			if ( !m_map->m_size )
			{
				m_bucketIndex = m_map->m_capacity;
			}
			else while ( m_bucketIndex < m_map->m_capacity )
			{
				if ( m_map->m_buckets[ m_bucketIndex ] )
				{
					m_element = m_map->m_buckets[ m_bucketIndex ];
					break;
				}

				++m_bucketIndex;
			}
		}

	public:

		RED_FORCE_INLINE const_iterator()
			: m_map( nullptr )
		{
		}

		RED_FORCE_INLINE const_iterator( const const_iterator& it )
		{
			*this = it;
		}

		RED_FORCE_INLINE const_iterator( const iterator& it )
		{
			*this = it;
		}

		RED_FORCE_INLINE const_iterator& operator = ( const const_iterator& it )
		{
			m_map			= it.m_map;
			m_bucketIndex	= it.m_bucketIndex;
			m_element		= it.m_element;
			return *this;
		}

		RED_FORCE_INLINE const_iterator& operator = ( const iterator& it )
		{
			m_map			= it.m_map;
			m_bucketIndex	= it.m_bucketIndex;
			m_element		= it.m_element;
			return *this;
		}

		RED_FORCE_INLINE const K& Key() const
		{
			RED_FATAL_ASSERT( m_element, "" );
			return m_element->m_first;
		}

		RED_FORCE_INLINE const V& Value() const
		{
			RED_FATAL_ASSERT( m_element, "" );
			return m_element->m_second;
		}

		RED_FORCE_INLINE const TPair<K, V>& operator * () const
		{
			RED_FATAL_ASSERT( m_element, "" );
			return *m_element;
		}

		RED_FORCE_INLINE const TPair<K,V>* operator -> () const
		{
			RED_FATAL_ASSERT( m_element, "" );
			return m_element;
		}

		RED_FORCE_INLINE Bool operator == ( const const_iterator& it ) const
		{
			return m_element == it.m_element;
		}

		RED_FORCE_INLINE Bool operator != ( const const_iterator& it ) const
		{
			return m_element != it.m_element;
		}

		RED_FORCE_INLINE const_iterator& operator ++ ()
		{
			// Reached the end

			if ( m_bucketIndex >= m_map->m_capacity )
			{
				return *this;
			}

			// Check next element on the list

			if ( m_element->m_next )
			{
				m_element = m_element->m_next;
				return *this;
			}

			// Check next non-empty bucket

			while ( ++m_bucketIndex < m_map->m_capacity )
			{
				if ( m_map->m_buckets[ m_bucketIndex ] )
				{
					m_element = m_map->m_buckets[ m_bucketIndex ];
					return *this;
				}
			}

			// Reached the end

			m_element = nullptr;
			return *this;
		}
	};

	RED_FORCE_INLINE const_iterator Begin() const
	{
		return const_iterator( this );
	}

	RED_FORCE_INLINE const_iterator End() const
	{
		return const_iterator( this, m_capacity, nullptr );
	}

	RED_FORCE_INLINE iterator Begin()
	{
		return iterator( this );
	}

	RED_FORCE_INLINE iterator End()
	{
		return iterator( this, m_capacity, nullptr );
	}

	RED_FORCE_INLINE THashMap( Uint32 initialCapacity = 0 )
		: m_buckets( nullptr )
		, m_size( 0 )
		, m_capacity( 0 )
		, m_autoDownsize( false )
	{
		Rehash( initialCapacity );
	}

	RED_FORCE_INLINE THashMap( const THashMap& other )
		: m_buckets( nullptr )
		, m_size( 0 )
		, m_capacity( 0 )
		, m_autoDownsize( other.m_autoDownsize )
	{
		Rehash( other.Size() );

		for ( auto it = other.Begin(), end = other.End(); it != end; ++it )
		{
			Insert( it->m_first, it->m_second );
		}
	}

	RED_FORCE_INLINE THashMap( THashMap&& other )
		: m_buckets( std::move( other.m_buckets ) )
		, m_size( std::move( other.m_size ) )
		, m_pool( std::move( other.m_pool ) )
		, m_capacity( std::move( other.m_capacity ) )
		, m_autoDownsize( std::move( other.m_autoDownsize ) )
	{
		other.m_buckets = nullptr;
		other.m_size = 0;
		other.m_capacity = 0;
		other.m_autoDownsize = false;
	}

	THashMap& operator = ( const THashMap& other )
	{
		THashMap( other ).Swap( *this );
		return *this;
	}

	THashMap& operator = ( THashMap&& other )
	{
		THashMap( std::move( other ) ).Swap( *this );
		return *this;
	}

	RED_FORCE_INLINE ~THashMap()
	{
		Clear();
	}

	// Enables automatic downsize of the buffer on Erase()
	RED_FORCE_INLINE void EnableAutoDownsize( Bool enable )
	{
		m_autoDownsize = enable;
	}

	RED_FORCE_INLINE Uint32 Size() const
	{
		return m_size;
	}

	RED_FORCE_INLINE Uint32 DataSize() const
	{
		return m_size * sizeof( TPair< K, V > );
	}

	RED_FORCE_INLINE TMemSize GetInternalMemSize() const
	{
		return m_capacity * ( sizeof( TElement ) + sizeof( TElement* ) );
	}

	RED_FORCE_INLINE Uint32 Capacity() const
	{
		return m_capacity;
	}

	RED_FORCE_INLINE Bool Empty() const
	{
		return !m_size;
	}

	// Insertion; fails if key already exists
	Bool Insert( const K& key, const V& value )
	{
		// Get hash

		const Uint32 hash = HashFunc::GetHash( key );

		// Check if key already exists

		Uint32 bucketIndex = 0;
		if ( m_size )
		{
			bucketIndex = hash % m_capacity;

			// Already exists? -> Fail

			TElement* element = m_buckets[ bucketIndex ];
			while ( element )
			{
				if ( element->m_hash == hash && EqualFunc::Equal( element->m_first, key ) )
				{
					return false;
				}
				element = element->m_next;
			}
		}

		// Make sure there's enough room for new element

		if ( PreInsert() || !m_size )
		{
			bucketIndex = hash % m_capacity; // Recalculate bucket index as it might change after rehash
		}

		// Insert

		TElement* newElement = new( m_pool ) TElement( key, value );
		RED_FATAL_ASSERT( newElement, "" );
		newElement->m_hash = hash;

		newElement->m_next = m_buckets[ bucketIndex ];
		m_buckets[ bucketIndex ] = newElement;

		++m_size;

		return true;
	}

	// Move-enabled insertion; fails if key already exists
	Bool Insert( const K& key, V&& value )
	{
		// Get hash

		const Uint32 hash = HashFunc::GetHash( key );

		// Check if key already exists

		Uint32 bucketIndex = 0;
		if ( m_size )
		{
			bucketIndex = hash % m_capacity;

			// Already exists? -> Fail

			TElement* element = m_buckets[ bucketIndex ];
			while ( element )
			{
				if ( element->m_hash == hash && EqualFunc::Equal( element->m_first, key ) )
				{
					return false;
				}
				element = element->m_next;
			}
		}

		// Make sure there's enough room for new element

		if ( PreInsert() || !m_size )
		{
			bucketIndex = hash % m_capacity; // Recalculate bucket index as it might change after rehash
		}

		// Insert

		TElement* newElement = new( m_pool ) TElement( key, Move( value ) );
		RED_FATAL_ASSERT( newElement, "" );
		newElement->m_hash = hash;

		newElement->m_next = m_buckets[ bucketIndex ];
		m_buckets[ bucketIndex ] = newElement;

		++m_size;

		return true;
	}

	// Sets value for a key; inserts new element if needed
	Bool Set( const K& key, const V& value )
	{
		// Get hash

		const Uint32 hash = HashFunc::GetHash( key );

		// Check if key already exists

		Uint32 bucketIndex = 0;
		if ( m_size )
		{
			bucketIndex = hash % m_capacity;

			// Already exists? -> Fail

			TElement* element = m_buckets[ bucketIndex ];
			while ( element )
			{
				if ( element->m_hash == hash && EqualFunc::Equal( element->m_first, key ) )
				{
					element->m_second = value;
					return true;
				}
				element = element->m_next;
			}
		}

		// Make sure there's enough room for new element

		if ( PreInsert() || !m_size )
		{
			bucketIndex = hash % m_capacity; // Recalculate bucket index as it might change after rehash
		}

		// Insert

		TElement* newElement = new( m_pool ) TElement( key, value );
		RED_FATAL_ASSERT( newElement, "" );
		newElement->m_hash = hash;

		newElement->m_next = m_buckets[ bucketIndex ];
		m_buckets[ bucketIndex ] = newElement;

		++m_size;

		return true;
	}

	// Move-enabled value setter; inserts new element if needed
	Bool Set( const K& key, V&& value )
	{
		// Get hash

		const Uint32 hash = HashFunc::GetHash( key );

		// Check if key already exists

		Uint32 bucketIndex = 0;
		if ( m_size )
		{
			bucketIndex = hash % m_capacity;

			// Already exists? -> Fail

			TElement* element = m_buckets[ bucketIndex ];
			while ( element )
			{
				if ( element->m_hash == hash && EqualFunc::Equal( element->m_first, key ) )
				{
					element->m_second = Move( value );
					return true;
				}
				element = element->m_next;
			}
		}

		// Make sure there's enough room for new element

		if ( PreInsert() || !m_size )
		{
			bucketIndex = hash % m_capacity; // Recalculate bucket index as it might change after rehash
		}

		// Insert

		TElement* newElement = new( m_pool ) TElement( key, Move( value ) );
		RED_FATAL_ASSERT( newElement, "" );
		newElement->m_hash = hash;

		newElement->m_next = m_buckets[ bucketIndex ];
		m_buckets[ bucketIndex ] = newElement;

		++m_size;

		return true;
	}

	// Erases key
	// Returns true if key was found and erased else returns false
	Bool Erase( const K& key )
	{
		if ( !m_size )
		{
			return false;
		}

		const Uint32 hash = HashFunc::GetHash( key );
		const Uint32 bucketIndex = hash % m_capacity;

		TElement** current = &m_buckets[ bucketIndex ];
		while ( *current )
		{
			if ( ( *current )->m_hash == hash && EqualFunc::Equal( ( *current )->m_first, key ) )
			{
				TElement* elementToRemove = *current;
				*current = elementToRemove->m_next;
				static_cast< TPair<K, V>* >( elementToRemove )->~TPair<K, V>();
				m_pool.FreeBlock( elementToRemove );
				--m_size;
				PostErase();
				return true;
			}
			current = &( *current )->m_next;
		}

		return false;
	}

	// Erases entry pointed via an iterator
	Bool Erase( const iterator& it )
	{
		if ( !m_size )
		{
			return false;
		}

		TElement** current = &m_buckets[ it.m_bucketIndex ];
		while ( *current )
		{
			if ( *current == it.m_element )
			{
				TElement* elementToRemove = *current;
				*current = elementToRemove->m_next;
				static_cast< TPair<K, V>* >( elementToRemove )->~TPair<K, V>();
				m_pool.FreeBlock( elementToRemove );
				--m_size;
				PostErase();
				return true;
			}
			current = &( *current )->m_next;
		}

		return false;
	}

	Bool EraseByValue( const V& value )
	{
		for ( iterator it = Begin(), end = End(); it != end; ++it )
		{
			if ( it->m_second == value )
			{
				return Erase( it );
			}
		}

		return false;
	}

	void Clear()
	{
		if ( m_capacity )
		{
			if ( m_size )
			{
				for ( auto it = Begin(), end = End(); it != end; ++it )
				{
					static_cast< TPair<K, V>* >( it.m_element )->~TPair<K, V>();
				}
				m_size = 0;
			}

			RED_MEMORY_FREE( memoryPool, MC_Type, m_pool.GetMemory() );
			m_capacity = 0;
		}
	}

	void ClearFast()
	{
		if ( !m_size )
		{
			return;
		}

		for ( auto it = Begin(), end = End(); it != end; ++it )
		{
			static_cast< TPair<K, V>* >( it.m_element )->~TPair<K, V>();
		}

		m_pool.Clear();
		Red::System::MemoryZero( m_buckets, m_capacity * sizeof( TElement* ) );
		m_size = 0;
	}

	void ClearPtr()
	{
		if ( m_size )
		{
			for ( auto it = Begin(), end = End(); it != end; ++it )
			{
				delete it->m_second;
			}
		}

		Clear();
	}

	void ClearPtrFast()
	{
		if ( !m_size )
		{
			return;
		}

		for ( auto it = Begin(), end = End(); it != end; ++it )
		{
			delete it->m_second;
		}

		ClearFast();
	}

	void Reserve( Uint32 newCapacity )
	{
		Rehash( newCapacity );
	}

	void Rehash( Uint32 newCapacity )
	{
		RED_FATAL_ASSERT( newCapacity >= m_size, "" );

		if ( newCapacity == m_capacity )
		{
			return;
		}

		if ( !newCapacity )
		{
			RED_MEMORY_FREE( memoryPool, MC_Type, m_pool.GetMemory() );
			m_capacity = 0;
			return;
		}

		// Allocate single memory block for the whole hashmap

		const Uint32 newPoolSize = newCapacity * sizeof( TElement );
		const Uint32 newBucketsSize = newCapacity * sizeof( TElement* );
		const Uint32 newMemorySize = newPoolSize + newBucketsSize;
		void* newMemory = RED_MEMORY_ALLOCATE( memoryPool, MC_Type, newMemorySize );
		RED_FATAL_ASSERT( newMemory, "" );

		// Initialize new internal containers
		CPool newPool;
		newPool.Init( newMemory, newPoolSize, sizeof( TElement ) );
		TElement** newBuckets = (TElement**) ( (Uint8*) newMemory + newPoolSize );
		Red::System::MemoryZero( newBuckets, newBucketsSize );
		
		if ( m_size )
		{
			// Move all elements to new containers

			for ( auto it = Begin(), end = End(); it != end; ++it )
			{
				const Uint32 hash = it.m_element->m_hash;
				const Uint32 bucketIndex = hash % newCapacity;

				// Copy into new element

				TElement* newElement = new( newPool ) TElement( Move( *it.m_element ) );
				RED_FATAL_ASSERT( newElement, "" );
				newElement->m_hash = hash;

				newElement->m_next = newBuckets[ bucketIndex ];
				newBuckets[ bucketIndex ] = newElement;

				// Destroy old element

				static_cast< TPair<K, V>* >( it.m_element )->~TPair<K, V>();
			}
		}

		// Delete old containers

		if ( m_capacity )
		{
			RED_MEMORY_FREE( memoryPool, MC_Type, m_pool.GetMemory() );
		}

		// Set new containers

		m_capacity = newCapacity;
		m_buckets = newBuckets;
		m_pool = Move( newPool );
	}

	RED_FORCE_INLINE void Shrink()
	{
		Rehash( m_size );
	}

	// If key already exists returns reference to associated value
	// If key doesn't exist create new mapping and return reference to value associated with that key
	V& operator [] ( const K& key )
	{
		// Get hash

		const Uint32 hash = HashFunc::GetHash( key );

		// Check if key already exists

		Uint32 bucketIndex = 0;
		if ( m_size )
		{
			bucketIndex = hash % m_capacity;

			// Already exists? -> return value

			TElement* element = m_buckets[ bucketIndex ];
			while ( element )
			{
				if ( element->m_hash == hash && EqualFunc::Equal( element->m_first, key ) )
				{
					return element->m_second;
				}
				element = element->m_next;
			}
		}

		// Make sure there's enough room for new element

		if ( PreInsert() || !m_size )
		{
			bucketIndex = hash % m_capacity; // Recalculate bucket index as it might change after rehash
		}

		// Insert

		TElement* newElement = new( m_pool ) TElement( key );
		RED_FATAL_ASSERT( newElement, "" );
		newElement->m_hash = hash;

		newElement->m_next = m_buckets[ bucketIndex ];
		m_buckets[ bucketIndex ] = newElement;

		++m_size;

		// Return new value

		return newElement->m_second;
	}

	// Find given key or add it ( with default value )
	RED_INLINE V& GetRef( const K& key, const V& defaultValue = V() )
	{
		// Get hash

		const Uint32 hash = HashFunc::GetHash( key );

		// Check if key already exists

		Uint32 bucketIndex = 0;
		if ( m_size )
		{
			bucketIndex = hash % m_capacity;

			// Already exists? -> return value

			TElement* element = m_buckets[ bucketIndex ];
			while ( element )
			{
				if ( element->m_hash == hash && EqualFunc::Equal( element->m_first, key ) )
				{
					return element->m_second;
				}
				element = element->m_next;
			}
		}

		// Make sure there's enough room for new element

		if ( PreInsert() || !m_size )
		{
			bucketIndex = hash % m_capacity; // Recalculate bucket index as it might change after rehash
		}

		// Insert

		TElement* newElement = new( m_pool ) TElement( key, defaultValue );
		RED_FATAL_ASSERT( newElement, "" );
		newElement->m_hash = hash;

		newElement->m_next = m_buckets[ bucketIndex ];
		m_buckets[ bucketIndex ] = newElement;

		++m_size;

		// Return new value

		return newElement->m_second;
	}

	// If key already exists returns reference to associated value, otherwise crash
	RED_FORCE_INLINE const V& operator [] ( const K& key ) const
	{
		Uint32 bucketIndex;
		const TElement* element = nullptr;
		RED_VERIFY( FindInternal( key, bucketIndex, element ) );
		RED_FATAL_ASSERT( element, "Key does not exist in hashmap. Cannot continue." );
		return element->m_second;
	}

	// Finds key
	// Returns iterator to key or End() if key is not found
	RED_FORCE_INLINE const_iterator Find( const K& key ) const
	{
		Uint32 bucketIndex;
		const TElement* element;
		return FindInternal( key, bucketIndex, element ) ? const_iterator( this, bucketIndex, element ) : End();
	}

	// Finds key
	// Returns iterator to key or End() if key is not found
	RED_FORCE_INLINE iterator Find( const K& key )
	{
		Uint32 bucketIndex;
		TElement* element;
		return FindInternal( key, bucketIndex, element ) ? iterator( this, bucketIndex, element ) : End();
	}

	// Finds key
	// Returns pointer to value if key was found
	// Returns nullptr if key was not found
	RED_FORCE_INLINE const V* FindPtr( const K& key ) const
	{
		Uint32 bucketIndex;
		const TElement* element;
		return FindInternal( key, bucketIndex, element ) ? &element->m_second : nullptr;
	}

	// Finds key
	// Returns pointer to value if key was found
	// Returns nullptr if key was not found
	RED_FORCE_INLINE V* FindPtr( const K& key )
	{
		Uint32 bucketIndex;
		TElement* element;
		return FindInternal( key, bucketIndex, element ) ? &element->m_second : nullptr;
	}

	// Find key
	// Returns true if key is found and set associated value to value variable
	// Returns false if key is not found, value variable is not changed
	RED_FORCE_INLINE Bool Find( const K& key, V& value ) const
	{
		Uint32 bucketIndex;
		const TElement* element;
		if ( FindInternal( key, bucketIndex, element ) )
		{
			value = element->m_second;
			return true;
		}
		return false;
	}

	RED_INLINE iterator FindByValue( const V& value )
	{
		for ( iterator it = Begin(), end = End(); it != end; ++it )
		{
			if ( it->m_second == value )
			{
				return it;
			}
		}
		return End();
	}

	// Checks if key exists in map
	RED_FORCE_INLINE Bool KeyExist( const K& key ) const
	{
		Uint32 bucketIndex;
		const TElement* element;
		return FindInternal( key, bucketIndex, element );
	}

// Left for posterity: Don't use this please, gets confused with "Find" and people think "value" will be initialized
// Makes for weird bugs on the PS4 esp. with uninitialized "value"
//	// Checks if key and value pair exists in map
// 	RED_FORCE_INLINE Bool	Exist( const K& key, const V& value ) const
// 	{
// 		Uint32 bucketIndex;
// 		const TElement* element;
// 		return FindInternal( key, bucketIndex, element ) && element->m_second == value;
// 	}

	RED_FORCE_INLINE void GetKeys( TDynArray< K >& dst ) const
	{
		dst.ClearFast();
		dst.Reserve( Size() );
		for ( auto it = Begin(), end = End(); it != end; ++it )
		{
			dst.PushBack( it->m_first );
		}
	}

	RED_FORCE_INLINE void GetValues( TDynArray< V >& dst ) const
	{
		dst.ClearFast();
		dst.Reserve( Size() );
		for ( auto it = Begin(), end = End(); it != end; ++it )
		{
			dst.PushBack( it->m_second );
		}
	}

	RED_FORCE_INLINE Bool operator == ( const THashMap& other ) const
	{
		if ( Size() != other.Size() )
		{
			return false;
		}

		for ( auto it = Begin(), end = End(); it != end; ++it )
		{
			if ( const V* otherValue = other.FindPtr( it->m_first ) )
			{
				if ( it->m_second != *otherValue )
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}

		return true;
	}

	RED_FORCE_INLINE Bool operator != ( const THashMap& other ) const
	{
		return !( *this == other );
	}

	// Serialization
	friend IFile& operator << ( IFile& file, THashMap& map )
	{
		if ( file.IsReader() )
		{
			map.Clear();
			if ( file.GetVersion() >= VER_NEW_HASHMAP )
			{
				Uint32 size;
				file << size;
				map.Rehash( size );

				K key;
				V value;
				for ( Uint32 i = 0; i < size; ++i )
				{
					file << key;
					file << value;
					map.Insert( key, value );
				}
			}
			else
			{
				// Read in from old format

				typedef TDynArray< Uint32, MC_Type > TBucket;
				TDynArray< TPair< K, V >, MC_Type > proxy;
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
			// Make sure hashmap serialization is deterministic by maintaining deterministic (depending on hashmap size) number of buckets
			// Note: Shrink() will do nothing if size equals capacity already

			map.Shrink();

			// Serialize the hashmap

			ASSERT( map.m_size == map.m_capacity );

			file << map.m_size;

			for ( Uint32 i = 0; i < map.m_capacity; ++i )
			{
				if ( TElement* list = map.m_buckets[ i ] )
				{
					if ( !list->m_next ) // Common case: single element list
					{
						file << list->m_first;
						file << list->m_second;
					}
					else // More than one element on the list
					{
						// Reverse the list before saving it out (to maintain identical binary format of the hashmap between saves)

						TElement* reversed = nullptr;
						while ( list )
						{
							TElement* next = list->m_next;
							list->m_next = reversed;
							reversed = list;
							list = next;
						}

						// Serialize and reverse back

						list = nullptr;
						while ( reversed )
						{
							file << reversed->m_first;
							file << reversed->m_second;

							TElement* next = reversed->m_next;
							reversed->m_next = list;
							list = reversed;
							reversed = next;
						}
					}
				}
			}
		}
		else
		{
			file << map.m_size;
			for ( auto it = map.Begin(), end = map.End(); it != end; ++it )
			{
				file << it->m_first;
				file << it->m_second;
			}
		}

		return file;
	}

	void GetStats( THashContainerStats& stats )
	{
		stats.m_size = m_size;
		stats.m_numBuckets = m_capacity;
		stats.m_numUsedBuckets = 0;
		stats.m_largestBucketSize = 0;

		for ( Uint32 i = 0; i < m_capacity; i++ )
		{
			if ( !m_buckets[ i ] )
			{
				continue;
			}

			stats.m_numUsedBuckets++;

			Uint32 bucketSize = 0;
			TElement* element = m_buckets[ i ];
			while ( element )
			{
				bucketSize++;
				element = element->m_next;
			}

			if ( bucketSize > stats.m_largestBucketSize )
			{
				stats.m_largestBucketSize = bucketSize;
			}
		}

		stats.m_averageBucketSize = (Float) m_size / (Float) stats.m_numUsedBuckets;
	}

	static const CName& GetTypeName();

private:
	RED_FORCE_INLINE Bool PreInsert()
	{
		if ( m_size + 1 > m_capacity )
		{
			// Grow to 150 % of the new size (and the minimum of 4)
			const Uint32 newCapacity = Max( (Uint32) 4, m_size + ( m_size >> 1 ) );
			Rehash( newCapacity );
			return true;
		}
		return false;
	}

	RED_FORCE_INLINE void PostErase()
	{
		// Only if "auto-downsize" option is enabled (which it isn't by default):
		// If capacity is over twice as big as size, then downsize to 150 % of the size

		if ( m_autoDownsize && m_size && ( m_size << 1 ) < m_capacity )
		{
			const Uint32 newCapacity = Max( (Uint32) 4, m_size + ( m_size >> 1 ) );
			Rehash( newCapacity );
		}
	}

	RED_FORCE_INLINE Bool FindInternal( const K& key, Uint32& bucketIndexOut, TElement*& elementOut )
	{
		return const_cast< const THashMap< K, V, HashFunc, EqualFunc, MC_Type >* >( this )->FindInternal( key, bucketIndexOut, const_cast< const TElement*& >( elementOut ) );
	}

	Bool FindInternal( const K& key, Uint32& bucketIndexOut, const TElement*& elementOut ) const
	{
		if ( !m_size )
		{
			return false;
		}

		const Uint32 hash = HashFunc::GetHash( key );
		const Uint32 bucketIndex = hash % m_capacity;

		TElement* element = m_buckets[ bucketIndex ];
		while ( element )
		{
			if ( element->m_hash == hash && EqualFunc::Equal( element->m_first, key ) )
			{
				bucketIndexOut = bucketIndex;
				elementOut = element;
				return true;
			}
			element = element->m_next;
		}

		// Not found

		return false;
	}

	void Swap( THashMap & value )
	{
		::Swap( m_capacity, value.m_capacity );
		::Swap( m_size, value.m_size );
		m_pool.Swap( value.m_pool );
		::Swap( m_buckets, value.m_buckets );
		::Swap( m_autoDownsize, value.m_autoDownsize );
	}

};

// Enable c++11 range-based for loop

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass MC_Type, RED_CONTAINER_POOL_TYPE memoryPool >
typename THashMap< K, V, HashFunc, EqualFunc, MC_Type, memoryPool >::iterator begin( THashMap< K, V, HashFunc, EqualFunc, MC_Type, memoryPool >& map ) { return map.Begin(); }

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass MC_Type, RED_CONTAINER_POOL_TYPE memoryPool >
typename THashMap< K, V, HashFunc, EqualFunc, MC_Type, memoryPool >::iterator end( THashMap< K, V, HashFunc, EqualFunc, MC_Type, memoryPool >& map ) { return map.End(); }

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass MC_Type, RED_CONTAINER_POOL_TYPE memoryPool >
typename THashMap< K, V, HashFunc, EqualFunc, MC_Type, memoryPool >::const_iterator begin( const THashMap< K, V, HashFunc, EqualFunc, MC_Type, memoryPool >& map ) { return map.Begin(); }

template < typename K, typename V, typename HashFunc, typename EqualFunc, EMemoryClass MC_Type, RED_CONTAINER_POOL_TYPE memoryPool >
typename THashMap< K, V, HashFunc, EqualFunc, MC_Type, memoryPool >::const_iterator end( const THashMap< K, V, HashFunc, EqualFunc, MC_Type, memoryPool >& map ) { return map.End(); }
