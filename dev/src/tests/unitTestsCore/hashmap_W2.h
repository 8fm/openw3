#pragma once

/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

//!< Used to count number of hash map instances with additional stats info for each instance
// #define EXPOSE_HASH_MAP_STATISTICS_API

//!< Iterators expose API identical to the old one
#define OLD_API_ADAPTER

//!< Exposes a few debugging methods (ie. used with tests)
// #define EXPOSE_DEBUG_API

#include "../../common/core/hashmapstatsmixin.h"
#include "../../common/core/pair.h"
#include "../../common/core/dynarray.h"
#include "../../common/core/hash.h"

//FIXME: Implement hashing of general POD types
//FIXME: Implement CONSTRAINT_MUST_BE_POD, CONSTRAINT_MUST_BE_DECAYABLE_POINTER
//FIXME: Implement proper TDynArray push back (which is independent of used memory class - templatized method)

namespace
{
	// hash function with fake partial specialization
	template< typename K >
	Uint32 genericHashFunction_W2( const K & key )
	{
		return GetHash( key );
	}

	const Uint32 GMaxAverageEltsInBucket = 31;
	const Uint32 GInitialEltsInBucket		= 3;
}

//** **********************************
template < typename K, typename V, EMemoryClass MC_Type = MC_HashMap >
class THashMap_W2 : private CHashMapStatsMixin
{
public:

	typedef K							key_type;
	typedef V							value_type;

private:

	typedef THashMap_W2< K, V, MC_Type >	ThisClass;
	typedef TDynArray< Uint32, MC_Type >	TBucket;

private:

#ifndef OLD_API_ADAPTER
	TDynArray< K, MC_Type >		m_orderedKeySet;	//!< keys
	TDynArray< V, MC_Type >		m_orderedValueSet;	//!< values
#else
	TDynArray< TPair< K, V >, MC_Type > m_proxy;	//!< TPair< K, V >
#endif

	TDynArray< TBucket >		m_bucketArray;		//!< m_bucketArray[ i ] == mapping in specified bucket

#ifdef _DEBUG
	Uint32						m_version;			//!< incremented after ALMOST every change 
#endif

public:

	// Iterator
	class iterator
	{
	public:

		THashMap_W2 *  m_pMap;		//!< owner
		Uint32		m_index;	//!< current linear index pointing to ordered sets (may point to mapping if needed)
#ifdef _DEBUG
		Uint32		m_version;	//!< version used to ensure data integrity
#endif

	public:

		RED_INLINE iterator()
			: m_index( 0 )
#ifdef _DEBUG
			, m_version( 0 )
#endif
		{
		}

	public:

		RED_INLINE iterator( const iterator & it )
		{
			*this = it;
		}

		RED_INLINE iterator & operator=( const iterator & it )
		{
			if( this != &it )
			{
				m_pMap		= it.m_pMap;
				m_index		= it.m_index;
#ifdef _DEBUG
				m_version	= it.m_version;
#endif
			}

			return *this;
		}

		RED_INLINE ~iterator()
		{
		}

		RED_INLINE const K & Key	() const
		{
#ifdef _DEBUG
			ASSERT( m_pMap->m_version == m_version, TXT( "Iterator used after it was invalidated" ) );
#endif
			ASSERT( m_index < m_pMap->Size() );

#ifndef OLD_API_ADAPTER
			return m_pMap->m_OrderedKeySet[ m_index ];
#else
			return m_pMap->m_proxy[ m_index ].m_first;
#endif
		}

		RED_INLINE V & Value		() const
		{
#ifdef _DEBUG
			ASSERT( m_pMap->m_version == m_version, TXT( "Iterator used after it was invalidated" ) );
#endif
			ASSERT( m_index < m_pMap->Size() );

#ifndef OLD_API_ADAPTER
			return m_pMap->m_orderedValueSet[ m_index ];
#else
			return m_pMap->m_proxy[ m_index ].m_second;
#endif
		}

#ifndef OLD_API_ADAPTER
		RED_INLINE V & operator*() const
		{
			return Value();
		}
#else
		RED_INLINE TPair<K,V> & operator*() const
		{
			ASSERT( m_index < m_pMap->Size() );
#ifdef _DEBUG
			ASSERT( m_pMap->m_version == m_version, TXT( "Iterator used after it was invalidated" ) );
#endif

			TPair<K,V> & pair = m_pMap->m_proxy[ m_index ];
			return pair;
		}
#endif

#ifndef OLD_API_ADAPTER
		RED_INLINE V * operator->() const
		{
			return &Value();
		}
#else
		RED_INLINE TPair<K,V> * operator->() const
		{
			ASSERT( m_index < m_pMap->Size() );
#ifdef _DEBUG
			ASSERT( m_pMap->m_version == m_version, TXT( "Iterator used after it was invalidated" ) );
#endif

			TPair<K,V> & pair = m_pMap->m_proxy[ m_index ];
			return &pair;
		}
#endif

		RED_INLINE Bool operator==( const iterator & it ) const
		{
			return ( m_pMap == it.m_pMap ) && ( m_index == it.m_index );
		}

		RED_INLINE Bool operator!=( const iterator & it ) const
		{
			return !( *this == it );
		}

		RED_INLINE iterator & operator+=( Uint32 delta )
		{
			m_index += delta;

			return *this;
		}

		// const used because it will be a temporary, and it is wise not to allow programmers to change temporaries
		RED_INLINE const iterator operator +( Uint32 delta )
		{
			iterator it = *this;

			it += delta;

			return it;
		}

		// Previous implementation returned copy, not a reference
		// Move by one
		RED_INLINE iterator & operator++()
		{
			++m_index;

			return *this;
		}

		// const used because returned type is a temporary, and it is wise not to allow programmers to change temporaries
		RED_INLINE const iterator operator++( Int32 )
		{
			iterator copy = *this;

			++*this;

			return copy;
		}

		static RED_INLINE iterator Create( Uint32 index, THashMap_W2 * pMap )
		{
			iterator it;

#ifdef _DEBUG
			it.m_version	= pMap->m_version;
#endif
			it.m_index		= index;
			it.m_pMap		= pMap;

			return it;
		}

	};

	class const_iterator
	{
	public:

		const THashMap_W2 *	m_pMap;		//!< owner
		Uint32				m_index;	//!< current linear index pointing to ordered sets (may point to mapping if needed)
#ifdef _DEBUG
		Uint32				m_version;	//!< version used to ensure data integrity
#endif

	public:

		RED_INLINE const_iterator()
			: m_index( 0 )
#ifdef _DEBUG
			, m_version( 0 )
#endif
		{
		}

	public:

		RED_INLINE const_iterator( const const_iterator & it )
		{
			*this = it;
		}

		RED_INLINE const_iterator( const iterator & it )
		{
			*this = it;
		}

		RED_INLINE const_iterator & operator=( const const_iterator & it )
		{
			if( this != &it )
			{
				m_pMap		= it.m_pMap;
				m_index		= it.m_index;
#ifdef _DEBUG
				m_version	= it.m_version;
#endif
			}

			return *this;
		}

		RED_INLINE const_iterator & operator=( const iterator & it )
		{
			m_pMap		= it.m_pMap;
			m_index		= it.m_index;
#ifdef _DEBUG
			m_version	= it.m_version;
#endif

			return *this;
		}

		RED_INLINE ~const_iterator()
		{
		}

		RED_INLINE const K & Key	() const
		{
#ifdef _DEBUG
			ASSERT( m_pMap->m_version == m_version, TXT( "Iterator used after it was invalidated" ) );
#endif
			ASSERT( m_index < m_pMap->Size() );

#ifndef OLD_API_ADAPTER
			return m_pMap->m_OrderedKeySet[ m_index ];
#else
			return m_pMap->m_proxy[ m_index ].m_first;
#endif
		}

		RED_INLINE const V & Value() const
		{
#ifdef _DEBUG
			ASSERT( m_pMap->m_version == m_version, TXT( "Iterator used after it was invalidated" ) );
#endif
			ASSERT( m_index < m_pMap->Size() );

#ifndef OLD_API_ADAPTER
			return m_pMap->m_orderedValueSet[ m_index ];
#else
			return m_pMap->m_proxy[ m_index ].m_second;
#endif
		}

#ifndef OLD_API_ADAPTER
		RED_INLINE const V & operator*() const
		{
			return Value();
		}
#else
		RED_INLINE const TPair<K,V> & operator*() const
		{
			ASSERT( m_index < m_pMap->Size() );
#ifdef _DEBUG
			ASSERT( m_pMap->m_version == m_version, TXT( "Iterator used after it was invalidated" ) );
#endif

			const TPair<K,V> & pair = m_pMap->m_proxy[ m_index ];
			return pair;
		}
#endif

#ifndef OLD_API_ADAPTER
		RED_INLINE const V * operator->() const
		{
			return &Value();
		}
#else
		RED_INLINE const TPair<K,V> * operator->() const
		{
			ASSERT( m_index < m_pMap->Size() );
#ifdef _DEBUG
			ASSERT( m_pMap->m_version == m_version, TXT( "Iterator used after it was invalidated" ) );
#endif

			const TPair<K,V> & pair = m_pMap->m_proxy[ m_index ];
			return &pair;
		}
#endif

		RED_INLINE Bool operator==( const const_iterator & it ) const
		{
			return ( m_pMap == it.m_pMap ) && ( m_index == it.m_index );
		}

		RED_INLINE Bool operator!=( const const_iterator & it ) const
		{
			return !( *this == it );
		}

		RED_INLINE const_iterator operator+= ( Uint32 delta )
		{
			m_index += delta;

			return *this;
		}

		// const used because it will be a temporary, and it is wise not to allow programmers to change temporaries
		RED_INLINE const const_iterator operator +( Uint32 delta )
		{
			const_iterator it = *this;

			it += delta;

			return it;
		}

		//previous implementation returned copy, not a reference
		RED_INLINE const_iterator & operator++()
		{
			++m_index;

			return *this;
		}

		// const used because returned type is a temporary, and it is wise not to allow programmers to change temporaries
		RED_INLINE const const_iterator operator++( Int32 )
		{
			const_iterator copy = *this;

			++*this;

			return copy;
		}

		static RED_INLINE const_iterator Create( Uint32 index, const THashMap_W2 * pMap )
		{
			const_iterator it;

#ifdef _DEBUG
			it.m_version	= pMap->m_version;
#endif
			it.m_index		= index;
			it.m_pMap		= pMap;

			return it;
		}

	};

private:

	RED_INLINE	iterator		CreateIterator		( Uint32 index, THashMap_W2 * pMap )
	{
		return iterator::Create( index, pMap );
	}

	RED_INLINE	const_iterator	CreateConstIterator	( Uint32 index, const THashMap_W2 * pMap ) const
	{
		return const_iterator::Create( index, pMap );
	}

	RED_INLINE const_iterator		AsConstIterator		( const iterator & it ) const
	{
		const_iterator iter;

		iter.m_pMap		= it.m_pMap;
		iter.m_index	= it.m_index;
#ifdef _DEBUG
		iter.m_version	= it.m_version;
#endif

		return iter;
	}

	RED_INLINE iterator			AsIterator			( const const_iterator & it ) const
	{
		iterator iter;

		iter.m_pMap		= const_cast< THashMap_W2 * >( it.m_pMap );
		iter.m_index	= it.m_index;
#ifdef _DEBUG
		iter.m_version	= it.m_version;
#endif

		return iter;
	}

public:

	// Iterators
	RED_INLINE iterator Begin()
	{
		return CreateIterator( 0, this );
	}

	RED_INLINE iterator End()
	{
		return CreateIterator( Size(), this );
	}

	RED_INLINE const_iterator Begin() const
	{
		return CreateConstIterator( 0, this );
	}

	RED_INLINE const_iterator End() const
	{
		return CreateConstIterator( Size(), this );
	}

public:

	//** FIXME: default bucket size will have to be tweaked according to the final usage of this class
	RED_INLINE THashMap_W2					( Uint32 initialBucketCount = 0 )
#ifdef _DEBUG
		: m_version( 127 )
#endif
	{
		if( initialBucketCount > 0 )
			m_bucketArray.Resize( initialBucketCount );
	}

	RED_INLINE THashMap_W2					( const THashMap_W2 & map )
		: m_bucketArray( map.m_bucketArray )
#ifdef _DEBUG
		, m_version( 127 )
#endif
#ifndef OLD_API_ADAPTER
		, m_orderedKeySet( map.m_orderedKeySet )
		, m_orderedValueSet( map.m_orderedValueSet )
#else
		, m_proxy( map.m_proxy )
#endif
	{
		OnCopy();
	}

	RED_INLINE THashMap_W2 & operator =		( const THashMap_W2 & map )
	{
		if( this != &map )
		{
#ifdef _DEBUG
			++m_version;
#endif

			m_bucketArray		= map.m_bucketArray;

#ifndef OLD_API_ADAPTER
			m_orderedKeySet		= map.m_orderedKeySet;
			m_orderedValueSet	= map.m_orderedValueSet;
#else
			m_proxy = map.m_proxy;
#endif
			OnCopy();
		}

		return *this;
	}

	RED_INLINE ~THashMap_W2					()
	{
#ifndef OLD_API_ADAPTER
		Uint32 size = sizeof( K ) + sizeof( V );
#else
		Uint32 size = sizeof( TPair< K, V > );
#endif

		OnDestroy( *this, size );
	}

	RED_INLINE Uint32 Size					() const
	{
#ifndef OLD_API_ADAPTER
		return m_orderedValueSet.Size();
#else
		return m_proxy.Size();
#endif
	}

	RED_INLINE Uint32 DataSize				() const
	{
#ifndef OLD_API_ADAPTER
		return m_orderedKeySet.DataSize() + m_orderedValueSet.DataSize();
#else
		return m_proxy.Size();
#endif
	}

	RED_INLINE Uint32 Capacity				() const
	{
#ifndef OLD_API_ADAPTER
		return m_orderedKeySet.Capacity();
#else
		return m_proxy.Size();
#endif
	}

	RED_INLINE Bool Empty					() const
	{
#ifndef OLD_API_ADAPTER		
		return m_orderedKeySet.Empty();
#else
		return m_proxy.Empty();
#endif
	}

	RED_INLINE Uint32 NumBuckets			() const
	{
		return m_bucketArray.Size();
	}

	// Statistics
#ifdef EXPOSE_HASH_MAP_STATISTICS_API

	RED_INLINE Uint32 BucketMinCount() const
	{
		if( m_bucketArray.Size() == 0 )
			return 0;

		Uint32 minBucketCount = m_bucketArray[ 0 ].Size();

		for( Uint32 i = 1; i < m_bucketArray.Size(); ++i )
		{
			if( m_bucketArray[ i ].Size() < minBucketCount  )
			{
				minBucketCount = m_bucketArray[ i ].Size();
			}
		}

		return minBucketCount;
	}

	RED_INLINE Uint32 BucketMaxCount() const
	{
		if( m_bucketArray.Size() == 0 )
			return 0;

		Uint32 maxBucketCount = m_bucketArray[ 0 ].Size();

		for( Uint32 i = 1; i < m_bucketArray.Size(); ++i )
		{
			if( m_bucketArray[ i ].Size() > maxBucketCount  )
			{
				maxBucketCount = m_bucketArray[ i ].Size();
			}
		}

		return maxBucketCount;
	}

	RED_INLINE Uint32 BucketZeroCount() const
	{
		Uint32 zeroBucketCount = 0;

		for( Uint32 i = 0; i < m_bucketArray.Size(); ++i )
		{
			if( m_bucketArray[ i ].Size() == 0 )
			{
				++zeroBucketCount;
			}
		}

		return zeroBucketCount;
	}

#endif

private:

	RED_INLINE TBucket & GetBucket		( Uint32 bucket )
	{
		ASSERT( bucket < m_bucketArray.Size() );

		return m_bucketArray[ bucket ];
	}

	RED_INLINE const TBucket & GetBucket	( Uint32 bucket ) const
	{
		ASSERT( bucket < m_bucketArray.Size() );

		return m_bucketArray[ bucket ];
	}

#ifdef EXPOSE_DEBUG_API
public:
#endif

	RED_INLINE Uint32	BucketEltCount		( Uint32 bucket ) const
	{
		return GetBucket( bucket ).Size();
	}

#ifdef EXPOSE_DEBUG_API
public: 
#endif

#ifdef EXPOSE_DEBUG_API
	void	ReadRawDataFromIndex			( Uint32 index, K & key, V & val ) const
	{
		ASSERT( index < Size() );

#ifndef OLD_API_ADAPTER
		key = m_orderedKeySet[ i ];
		val = m_orderedValueSet[ i ];
#else
		key = m_proxy[ i ].m_first;
		val = m_proxy[ i ].m_second;
#endif
	}
	
	TDynArray< V > BucketVals			( Uint32 nBucket ) const
	{
		TDynArray< V > arr;

		const TBucket & bucket = GetBucket( nBucket );

		for( Uint32 i = 0; i < bucket.Size(); ++i )
		{
#ifndef OLD_API_ADAPTER
			arr.PushBack( m_orderedValueSet[ bucket[ i ] ] );
#else
			arr.PushBack( m_proxy[ bucket[ i ] ].m_second );
#endif
		}

		return arr;
	}

	TDynArray< K > BucketKeys			( Uint32 nBucket ) const
	{
		TDynArray< K > arr;

		const TBucket & bucket = GetBucket( nBucket );

		for( Uint32 i = 0; i < bucket.Size(); ++i )
		{
#ifndef OLD_API_ADAPTER
			arr.PushBack( m_orderedKeySet[ bucket[ i ] ] );
#else
			arr.PushBack( m_proxy[ bucket[ i ] ].m_first );
#endif
		}

		return arr;
	}
#endif //EXPOSE_DEBUG_API

#ifdef EXPOSE_DEBUG_API
private:
#endif

	RED_INLINE void	StorePair			( const K & key, const V & val, Uint32 bucket )
	{
#ifndef OLD_API_ADAPTER
		m_orderedKeySet.PushBack( key );
		m_orderedValueSet.PushBack( val );
#else
		new ( m_proxy ) TPair< K, V >( key, val );
//		m_proxy.PushBack( TPair< K,V >( key, val ) );
#endif
		// This entry is currently the last one in raw data collection
		AddBucketMapping( bucket, Size() - 1 );
	}

	RED_INLINE void AddBucketMapping( Uint32 bucket, Uint32 mapping )
	{
		m_bucketArray[ bucket ].PushBack( mapping );
	}

	// Removes raw data entry. Bucket mappings are not modified
	RED_INLINE void	RemoveRawDataEntry			( Uint32 index )
	{
#ifndef OLD_API_ADAPTER
		// Remove key entry
		m_orderedKeySet.EraseFast( m_orderedKeySet.Begin() + index );

		// Remove value entry
		m_orderedValueSet.EraseFast( m_orderedValueSet.Begin() + index );
#else
		m_proxy.EraseFast( m_proxy.Begin() + index );
#endif
	}

	// Removes raw data entry. Bucket mappings are not modified
	RED_INLINE void	RemoveBucketEntry	( TBucket & bucket, Uint32 index )
	{
		bucket.EraseFast( bucket.Begin() + index );
	}

	RED_INLINE Uint32	CalcHashIndex		( const K & key ) const
	{
		OnHash();

		return genericHashFunction_W2( key ) % m_bucketArray.Size();
	}

public:

	// Insert key only if key doesn't not exist in map already
	RED_INLINE Bool Insert				( const K & key, const V & value )
	{
		// Check whether current key is not in use
		iterator it = Find( key );

		if( it != End() )
		{
			return false;
		}

		// FIXME: Find calculates hash for current key so this data can be reused
		Uint32 bucket = CalcHashIndex( key );

#ifdef _DEBUG
		// Add new entry	
		++m_version;
#endif

		// Insert data (key, value) and map to proper bucket
		StorePair( key, value, bucket );

		OnInsert( *this );

		if( Size() > ( GMaxAverageEltsInBucket * NumBuckets() ) )
		{
			Rehash( Size() / GInitialEltsInBucket );
		}

		return true;
	}

	// Erase key
	// Returns true if key was found and erased else returns false
	RED_INLINE Bool Erase					( const K & key )
	{
		//FIXME: can be optimized -> find generates intermediate info that can be used during next erase
		iterator it = Find( key );

		if( it != End() )
		{
			return Erase( it );
		}

		return false;
	}

	// Erase entry pointed by an iterator
	RED_INLINE Bool Erase					( const iterator & it )
	{
		ASSERT( it != End() );

		const K & key = it.Key();

		TBucket & bucket = GetBucket( CalcHashIndex( key ) );
		
		bool bErased = false;

		for( Uint32 i = 0; i < bucket.Size(); ++i )
		{
#ifndef OLD_API_ADAPTER
			if( m_orderedKeySet[ bucket[ i ] ] == key )
#else
			if( m_proxy[ bucket[ i ] ].m_first == key )
#endif 
			{
				// Update mapping for the last element
#ifndef OLD_API_ADAPTER
				const K & lastKey = m_orderedKeySet[ Size() - 1 ];
#else
				const K & lastKey = m_proxy[ Size() - 1 ].m_first;
#endif
				TBucket & lastEltBucket = GetBucket( CalcHashIndex( lastKey ) );

				bool bRemapped = false;

				// Change mapping due to EraseFast semantics (used in RemoveRawDataEntry)
				for( Uint32 j = 0; j < lastEltBucket.Size(); ++j )
				{
					if( lastEltBucket[ j ] == ( Size() - 1 ) )
					{
						lastEltBucket[ j ] = it.m_index;

						bRemapped = true;

						break;
					}
				}

				ASSERT( bRemapped );

				// Remove raw data entry
				RemoveRawDataEntry( it.m_index );
				
				// Remove bucket entry
				RemoveBucketEntry ( bucket, i );

				bErased = true;

				break;
			}
		}

		ASSERT( bErased );

		OnRemove();

		return bErased;
	}

	//Semantics: erases the first found entry with this value (if found)
	//This is exactly the semantics of previous implementation
	RED_INLINE Bool EraseByValue			( const V & val )
	{
		iterator end = End();

		for( iterator it = Begin(); it != end; ++it )
		{
			if( it.Value() == val )
			{
				return Erase( it );
			}
		}

		return false;
	}

	RED_INLINE void Clear()
	{
#ifdef _DEBUG
		++m_version;
#endif

		// Clear dynamic collections
#ifndef OLD_API_ADAPTER
		m_orderedKeySet.Clear();
		m_orderedValueSet.Clear();
#else
		m_proxy.Clear();
#endif
		for( Uint32 i = 0; i < m_bucketArray.Size(); ++i )
		{
			m_bucketArray[ i ].Clear();
		}

		OnErase();
	}

	void ClearFast()
	{
#ifdef _DEBUG
		++m_version;
#endif

#ifndef OLD_API_ADAPTER
		// Clear dynamic collections
		m_orderedKeySet.ClearFast();
		m_orderedValueSet.ClearFast();
#else
		m_proxy.ClearFast();
#endif
		for( Uint32 i = 0; i < m_bucketArray.Size(); ++i )
		{
			m_bucketArray[ i ].ClearFast();
		}

		OnErase();
	}

	//FIXME: add CONSTRAINT_MUST_BE_PTR
	RED_INLINE void ClearPtr()
	{
#ifdef _DEBUG
		++m_version;
#endif

		for( Uint32 i = 0; i < Size(); ++i )
		{
#ifndef OLD_API_ADAPTER
			delete m_orderedValueSet[ i ];
#else
			delete m_proxy[ i ].m_second;
#endif
		}

		Clear();
	}

	//FIXME: add CONSTRAINT_MUST_BE_PTR
	RED_INLINE void ClearPtrFast()
	{
#ifdef _DEBUG
		++m_version;
#endif

		for( Uint32 i = 0; i < Size(); ++i )
		{
#ifndef OLD_API_ADAPTER
			delete m_orderedValueSet[ i ];
#else
			delete m_proxy[ i ].m_second;
#endif
		}

		ClearFast();
	}

	void Reserve( Uint32 capacity )
	{
		Rehash( capacity );
	}

	// This implementation avoids key, value pairs copying altogether
	RED_INLINE void Rehash( Uint32 newSize )
	{
		ASSERT( newSize > 0 );

#ifdef _DEBUG
		++m_version;
#endif

		// Resize and clear all buckets
		m_bucketArray.ClearFast();
		m_bucketArray.Resize( newSize );

		for( Uint32 i = 0; i < m_bucketArray.Size(); ++i )
		{
			m_bucketArray[ i ].ClearFast();
		}

		// Update bucket information for new number of buckets (bucket mapping)
		for( Uint32 i = 0; i < Size(); ++i )
		{			
#ifndef OLD_API_ADAPTER
			Uint32 bucket = CalcHashIndex( m_orderedKeySet[ i ] );
#else
			Uint32 bucket = CalcHashIndex( m_proxy[ i ].m_first );
#endif
			AddBucketMapping( bucket, i );
		}

		OnRehash();
	}

	RED_INLINE void Shrink()
	{
#ifdef _DEBUG
		++m_version;
#endif

#ifndef OLD_API_ADAPTER
		m_orderedKeySet.Shrink();
		m_orderedValueSet.Shrink();
#else
		m_proxy.Shrink();
#endif
		m_bucketArray.Shrink(); //FIXME: this one is questionable 
	}

private:

	RED_INLINE const_iterator FindGeneric			( const K & key ) const
	{
		// Initially just one bucket as there are lots of objects storing single entry only
		// CalcHashIndex uses number of buckets to divide modulo (so we cannot let 0 there)
		if( m_bucketArray.Size() == 0 )
		{
			THashMap_W2<K,V,MC_Type> * pThis = const_cast< THashMap_W2<K,V,MC_Type>* >( this );

			pThis->m_bucketArray.Resize( 1 );
		}

		const TBucket & bucket = GetBucket( CalcHashIndex( key ) );

		// Linear search within bucket
		for( Uint32 i = 0; i < bucket.Size(); ++i )
		{
#ifndef OLD_API_ADAPTER
			if( m_orderedKeySet[ bucket[ i ] ] == key )
			{
				return CreateConstIterator( bucket[ i ], this );
			}
#else
			if( m_proxy[ bucket[ i ] ].m_first == key )
			{
				return CreateConstIterator( bucket[ i ], this );
			}
#endif
		}

		// Entry not found in hashed bucket
		return End();
	}

public:

	// Find key
	// Returns iterator to key or End() if key is not found
	RED_INLINE iterator		Find		( const K & key )
	{
		return AsIterator( FindGeneric( key ) );
	}

	// Find key
	// Returns iterator to key or End() if key is not found
	RED_INLINE const_iterator Find		( const K & key ) const
	{
		return FindGeneric( key );
	}

	// If key already exists returns reference to associated value
	// If key doesn't exist create new mapping and return reference to value associated with that key
	RED_INLINE V & operator[]	( const K & key )
	{
		iterator it = Find( key );

		if( it == End() )
		{
			Uint32 oldSize = Size();

			Insert( key, V() );

			it = CreateIterator( oldSize, this );
		}

		OnAssign();

		return it.Value();
	}

	// If key already exists returns reference to associated value
	RED_INLINE const V & operator[]	( const K & key ) const
	{
		const_iterator it = Find( key );

		ASSERT( it != End() );

		OnRead();

		return it.Value();
	}

	// Find key
	// Returns true if key is found and set associated value to value variable
	// Returns false if key is not found, value variable is not changed
	RED_INLINE Bool Find		( const K & key, V & val )
	{
		iterator it = Find( key );

		if( it != End() )
		{
			val = it.Value();

			return true;
		}

		return false;
	}

	// Find key
	// Returns true if key is found and set associated value to value variable
	// Returns false if key is not found, value variable is not changed
	RED_INLINE Bool Find		( const K & key, V & val ) const
	{
		const_iterator it = Find( key );

		if( it != End() )
		{
			val = it.Value();

			return true;
		}

		return false;
	}

	// Find key
	// Returns pointer to pair value if key was found
	// Returns NULL if key was not found
	RED_INLINE V * FindPtr	( const K & key )
	{
		iterator it = Find( key );

		if( it != End() )
		{
			return &it.Value();
		}

		return NULL;
	}

	// Find key
	// Returns pointer to pair value if key was found
	// Returns NULL if key was not found
	RED_INLINE const V * FindPtr( const K & key ) const
	{
		const_iterator it = Find( key );

		if( it != End() )
		{
			return &it.Value();
		}

		return NULL;
	}

	// Check if key exists in map
	RED_INLINE Bool KeyExist( const K & key ) const
	{
		return ( Find( key ) != End() );
	}

	RED_INLINE Bool	Exist( const K & key, const V & val ) const
	{
		const_iterator it = Find( key );

		if( it != End() )
		{
			return val == it.Value();
		}

		return false;
	}

	RED_INLINE void GetKeys( TDynArray< K > & dst ) const
	{
		//FIXME: implement generic (regarding mem_class) interface in TDynArray and use push back from that implementation
		for( Uint32 i = 0; i < Size(); ++i )
		{
#ifndef OLD_API_ADAPTER
			dst.PushBack( m_orderedKeySet[ i ] );
#else
			dst.PushBack( m_proxy[ i ].m_first );
#endif
		}
	}

	RED_INLINE void GetValues( TDynArray< V > & dst ) const
	{
		//FIXME: implement generic (regarding mem_class) interface in TDynArray and use push back from that implementation
		for( Uint32 i = 0; i < Size(); ++i )
		{
#ifndef OLD_API_ADAPTER
			dst.PushBack( m_orderedValueSet[ i ] );
#else
			dst.PushBack( m_proxy[ i ].m_second );
#endif
		}
	}

#ifndef OLD_API_ADAPTER
	RED_INLINE const TDynArray< K, MC_Type > GetKeys() const
	{
		//FIXME: implement generic (regarding mem_class) interface in TDynArray and use push back from that implementation
		return m_orderedKeySet;
	}

	RED_INLINE const TDynArray< V, MC_Type > & GetValues() const
	{
		//FIXME: implement generic (regarding mem_class) interface in TDynArray and use push back from that implementation
		return m_orderedValueSet;
	}
#endif

	// Previous implementation was adding TPair sizeof as well, but sizeof hash structures were not added
	RED_INLINE TMemSize SizeOfAllElements	() const
	{
#ifndef OLD_API_ADAPTER
		return Capacity() * ( sizeof( K ) + sizeof( V ) );
#else
		return Capacity() * sizeof( TPair< K, V > );
#endif 
	}

	// It should work somehow
	RED_INLINE TMemSize GetInternalMemSize() const
	{
		TMemSize totalMemSize = 0;
		totalMemSize += ( TMemSize ) m_proxy.SizeOfAllElements();
		totalMemSize += ( TMemSize ) m_bucketArray.SizeOfAllElements();
		for ( const TBucket& bucket : m_bucketArray )
		{
			totalMemSize += ( TMemSize ) bucket.SizeOfAllElements();
		}

		return totalMemSize;
	}

	// Compare
	// This works as content comparator (if stored pairs are equal in both maps then they are treated as
	// identical. No underlying bucket structure is checked in this method
	RED_INLINE Bool operator==			( const THashMap_W2 & map ) const
	{
		if( Size() != map.Size() )
		{
			return false;
		}

		for( Uint32 i = 0; i < Size(); ++i )
		{
#ifndef OLD_API_ADAPTER
			if( m_orderedKeySet[ i ] != map.m_orderedKeySet[ i ] )
				return false;

			if( m_orderedValueSet[ i ] != map.m_orderedValueSet[ i ] )
				return false;
#else
			if( m_proxy[ i ] != map.m_proxy[ i ] )
				return false;
#endif
		}

		return true;
	}

	RED_INLINE Bool operator!=	( const THashMap_W2& map ) const
	{
		return !( *this == map );
	}

	// Serialization
	friend IFile& operator<<( IFile & file, THashMap_W2 & map )
	{
		Uint32 ver		= file.GetVersion();
		bool bIsReader	= file.IsReader();

		RED_UNUSED( ver );
		RED_UNUSED( bIsReader );

#ifndef OLD_API_ADAPTER
		file << map.m_orderedKeySet;
		file << map.m_orderedValueSet;
#else
		file << map.m_proxy;
#endif 
		file << map.m_bucketArray;

		if( file.IsReader() && map.m_bucketArray.Size() > 0 )
		{
			// FIXME: minor performance bottleneck - there is no need to read bucket array
			map.Rehash( map.m_bucketArray.Size() );
		}

		return file;
	}
};
