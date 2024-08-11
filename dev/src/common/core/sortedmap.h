/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "sortedset.h"
#include "pair.h"

/**
 *	Sorted, array based (!), map.
 *
 *	Only use if:
 *	- you really need elements to be sorted or
 *	- if per-element memory overhead being low (zero overhead here) is of high importance
 *	Otherwise use THashMap which is much faster.
 *
 *	Notes on performance:
 *	- SLOW insert()/erase() of O(n) - potentially moves all other elements in memory
 *	- find() of O(logn) - binary search
 *	- zero per element memory overhead - the main advantage of this container
 *
 *	Notes on implementation:
 *	- uses TDynArray internally to store all elements sorted, so changing its content requires massive move operations
 *	- typically sorted map is implemented via some sort of balanced tree (e.g. red black tree), so be careful when using it (slow insert/erase)
 */
template < 
		   typename K,
		   typename V,
		   typename CompareFunc = DefaultCompareFunc< K >,
		   EMemoryClass MC_Type = MC_SortedSet
		 >
class TSortedMap : public TSortedSet< TPair< K, V >, ComparePairFunc< K, V, CompareFunc >, MC_Type >
{
public:
	typedef K			key_type;
	typedef V			value_type;
	typedef CompareFunc compare_func;

	typedef TSortedSet< TPair< K, V >, ComparePairFunc< K, V, CompareFunc >, MC_Type >	base_class;
	typedef TSortedMap< K, V, CompareFunc, MC_Type >									this_class;

	typedef typename base_class::iterator iterator;
	typedef typename base_class::const_iterator const_iterator;

public:
	using base_class::Size;
	using base_class::End;

protected:
	using base_class::m_keyArray;

private:
#ifdef _DEBUG
	using base_class::m_version;
#endif

public:

	TSortedMap()
	{}

	TSortedMap( const TSortedMap & value )
		: base_class( value )	
	{}

	TSortedMap( TSortedMap && value )
		: base_class( std::forward< TSortedMap >( value ) )
	{}

	//! Erase entry from map by matching key
	RED_INLINE Bool Erase( const K& key )
	{
		iterator it = Find( key );
		if( it != this->End() )
		{
			return base_class::Erase( it );
		}
		return false;
	}

	//! Erase entry at given iteration point
	RED_INLINE Bool Erase( iterator& it )
	{
		return base_class::Erase( it );
	}

	//! Erase entry from map by matching value (O(n))
	RED_INLINE Bool EraseByValue( const V& val )
	{
		// Delete item by value, slow, linear time
		for ( iterator it = this->Begin(); it != this->End(); ++it )
		{
			if ( it->m_second == val )
			{
				return base_class::Erase( it );
			}
		}
		return false;
	}

	//! Find entry in map by matching value (O(n))
	RED_INLINE iterator FindByValue( const V& val )
	{
		// Find item by value, slow, linear time
		for ( iterator it = this->Begin(); it != this->End(); ++it )
		{
			if ( it->m_second == val )
			{
				return it;
			}
		}
		return this->End();
	}

	//! Clear array and call delete on the "value" element of the entry
	RED_INLINE void ClearPtr()
	{
#ifdef _DEBUG
		++m_version;
#endif
		for ( Uint32 i=0; i<this->m_keyArray.Size(); i++ )
		{
			delete this->m_keyArray[i].m_second;
		}
		this->m_keyArray.Resize(0);
	}

	//! Find key, returns iterator to key or End() if key is not found
	RED_INLINE iterator Find( const K& key )
	{
		if ( Size() > 0 )
		{
			Int32 begin = 0;
			Int32 end   = Size()-1;

			// Bisection search because m_keyArray is sorted
			do
			{
				Int32 index = (begin + end) / 2;
				if ( CompareFunc::Less(key, m_keyArray[index].m_first) )
				{
					end = index - 1;
				}
				else if ( CompareFunc::Less(m_keyArray[index].m_first, key) )
				{
					begin = index + 1;
				}
				else
				{
#ifdef _DEBUG
					return iterator( m_keyArray.Begin() + index, this, m_version );
#else
					return iterator( m_keyArray.Begin() + index, this );
#endif
				}
			}
			while( begin <= end );
		}

		// Not found
		return End();
	}

	//! Find key, returns iterator to key or End() if key is not found
	RED_INLINE const_iterator Find( const K& key ) const
	{
		if ( Size() > 0 )
		{
			Int32 begin = 0;
			Int32 end   = Size()-1;

			// Bisection search because m_keyArray is sorted
			do
			{
				Int32 index = (begin + end) / 2;
				if ( CompareFunc::Less(key, m_keyArray[index].m_first) )
				{
					end = index - 1;
				}
				else if ( CompareFunc::Less(m_keyArray[index].m_first, key) )
				{
					begin = index + 1;
				}
				else
				{
#ifdef _DEBUG
					return const_iterator( m_keyArray.Begin() + index, this, m_version );
#else
					return const_iterator( m_keyArray.Begin() + index, this );
#endif
				}
			}
			while( begin <= end );
		}

		// Not found
		return End();
	}

	// Find key
	// Returns true if key is found and set associated value to value variable
	// Returns false if key is not found, value variable is not changed
	RED_INLINE Bool Find( const K& key, V& value )
	{
		iterator it = Find( key );
		if ( it != this->End() )
		{
			value = (*it).m_second;
			return true;
		}
		return false;
	}

	// Find key
	// Returns true if key is found and set associated value to value variable
	// Returns false if key is not found, value variable is not changed
	RED_INLINE Bool Find( const K& key, V& value ) const
	{
		const_iterator it = Find( key );
		if ( it != this->End() )
		{
			value = (*it).m_second;
			return true;
		}
		return false;
	}

	// Find key
	// Returns pointer to pair value if key was found
	// Returns NULL if key was not found
	RED_INLINE V* FindPtr( const K& key )
	{
		iterator it = Find( key );
		if ( it != this->End() )
		{
			return &it->m_second;
		}
		return NULL;
	}

	// Find key
	// Returns pointer to pair value if key was found
	// Returns NULL if key was not found
	RED_INLINE const V* FindPtr( const K& key ) const
	{
		const_iterator it = Find( key );
		if ( it != this->End() )
		{
			return &it->m_second;
		}
		return NULL;
	}

	// Check if key exists in map
	RED_INLINE Bool KeyExist( const K& key ) const
	{
		return Find( key ) != this->End();
	}

	// Insert key, value pair, only if key doesn't exist in map already
	RED_INLINE Bool Insert( const K& key, const V& value )
	{
		TPair<K,V> pair( key, value );
		return base_class::Insert( pair );
	}

	RED_INLINE Bool Insert( const K& key, V&& value )
	{
		TPair<K,V> pair( key, std::move( value ) );
		return base_class::Insert( std::move( pair ) );
	}

	// Set key value, if key does not exist it's added
	RED_INLINE Bool Set( const K& key, const V& value )
	{
		iterator it = Find( key );
		if ( it != End() )
		{
			// Change value
			it->m_second = value;
			return true;
		}
		else
		{
			// Insert new
			TPair<K,V> pair( key, value );
			return base_class::Insert( pair );
		}
	}

	// Gets or creates element at given key
	RED_INLINE V& operator [] ( const K& key )
	{
		return GetRef( key );
	}

	// Find given key or add it ( with default value )
	RED_INLINE V& GetRef( const K& key, const V& defaultValue = V() )
	{
		iterator it = Find( key );
		if ( it != End() )
		{
			return it->m_second;
		}
		else
		{
			// Insert new
			TPair<K,V> pair( key, defaultValue );
			if ( !base_class::Insert( pair ) )
			{
				HALT( "Failed to GetRef on map" );
				return *(V*) 0x00000000;
			}
			else
			{
				it = Find( key );
				if ( it != End() )
				{
					return it->m_second;
				}
				else
				{
					HALT( "Failed to GetRef on map" );
					return *(V*) 0x00000000;
				}
			}
		}
	}

	RED_INLINE void Merge( const this_class& other )
	{
		base_class::Merge( static_cast< const base_class& >( other ) );
	}

	RED_INLINE static void Merge( this_class& destination, this_class** sources, Uint32 numSources )
	{
		base_class::Merge( static_cast< base_class& >( destination ), reinterpret_cast< base_class** >( sources ), numSources );
	}

	RED_INLINE TSortedMap operator=( TSortedMap && keyArray )
	{
		base_class::operator=( std::forward< TSortedMap >( keyArray ) );
		return *this;
	}
	
	RED_INLINE TSortedMap operator=( const TSortedMap & keyArray )
	{
		base_class::operator=(keyArray);
		return *this;
	}
	
	// (HACKERY) Bulk insert an entry into the map
	// IMPORTANT: you need to use Resort() after using this - if you don't you risk having a broken container
	RED_INLINE void BulkInsert( const K& key, const V& value )
	{
		base_class::BulkInsert( TPair<K,V>( key, value ) );
	}

	// (HACKERY) Resort map - usually done after group of "BulkInsert"
	RED_INLINE void Resort()
	{
		base_class::Resort();
	}

	// (HACKERY) Save as bulk data
	RED_INLINE void SerializeBulk( IFile& file )
	{
		base_class::SerializeBulk( file );
	}
};

// Enable c++11 range-based for loop

template < typename K, typename V, typename CompareFunc, EMemoryClass MC_Type >
typename TSortedMap< K, V, CompareFunc, MC_Type >::iterator begin( TSortedMap< K, V, CompareFunc, MC_Type >& map ) { return map.Begin(); }

template < typename K, typename V, typename CompareFunc, EMemoryClass MC_Type >
typename TSortedMap< K, V, CompareFunc, MC_Type >::iterator end( TSortedMap< K, V, CompareFunc, MC_Type >& map ) { return map.End(); }

template < typename K, typename V, typename CompareFunc, EMemoryClass MC_Type >
typename TSortedMap< K, V, CompareFunc, MC_Type >::const_iterator begin( const TSortedMap< K, V, CompareFunc, MC_Type >& map ) { return map.Begin(); }

template < typename K, typename V, typename CompareFunc, EMemoryClass MC_Type >
typename TSortedMap< K, V, CompareFunc, MC_Type >::const_iterator end( const TSortedMap< K, V, CompareFunc, MC_Type >& map ) { return map.End(); }

