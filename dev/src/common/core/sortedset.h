/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "dynarray.h"
#include "compare.h"

template < typename K, typename CompareFunc = DefaultCompareFunc< K >, EMemoryClass MC_Type = MC_SortedSet >
class TSortedSet
{
public:
	typedef K			key_type;
	typedef CompareFunc compare_func;

	typedef TSortedSet< K, CompareFunc, MC_Type > this_class;

public:
	//! Set element iterator
	class iterator
	{
	public:
		TSortedSet< K, CompareFunc, MC_Type >*			m_set;			//!< Source set
#ifdef _DEBUG
		Uint32											m_version;		//!< Version	
#endif
		typename TDynArray< K, MC_Type >::iterator		m_iterator;		//!< Internal array iterator

	public:
		//! Default constructor
		RED_INLINE iterator()
			: m_set( NULL )
#ifdef _DEBUG
			, m_version( 0 )
#endif
		{};

		//! Constructor
		RED_INLINE iterator( const iterator& other )
			: m_set( other.m_set )
#ifdef _DEBUG
			, m_version( other.m_version )
#endif
			, m_iterator( other.m_iterator )
		{};

		//! Initialize
		RED_INLINE iterator( const typename TDynArray< K, MC_Type >::iterator& it, TSortedSet< K, CompareFunc, MC_Type >* set
#ifdef _DEBUG
			, Uint32 version 
#endif
			)
			: m_set( set )
#ifdef _DEBUG
			, m_version( version )
#endif
			, m_iterator( it )
		{};

		//! Compare iterator
		RED_INLINE Bool operator==( const iterator& other ) const
		{
			return m_iterator == other.m_iterator;
		}

		//! Compare iterator
		RED_INLINE Bool operator!=( const iterator& other ) const
		{
			return m_iterator != other.m_iterator;
		}

		//! Iterate to next
		RED_INLINE void operator++()
		{
			++m_iterator;
		}

		//! Iterate to next
		RED_INLINE void operator++( Int32 )
		{
			++m_iterator;
		}

		//! Get element
		RED_INLINE K* operator->()
		{
#ifdef _DEBUG
			RED_FATAL_ASSERT( m_set->m_version == m_version, "Iterator used after it was invalidated" );
#endif
			return &(*m_iterator);
		}

		//! Get element
		RED_INLINE K& operator*()
		{
#ifdef _DEBUG
			RED_FATAL_ASSERT( m_set->m_version == m_version, "Iterator used after it was invalidated" );
#endif
			return *m_iterator;
		}
	};

	//! Const element iterator
	class const_iterator
	{
	public:
		const TSortedSet< K, CompareFunc, MC_Type >*			m_set;			//!< Source set
#ifdef _DEBUG
		Uint32													m_version;		//!< Version	
#endif
		typename TDynArray< K, MC_Type >::const_iterator		m_iterator;		//!< Internal array iterator

	public:
		//! Default constructor
		RED_INLINE const_iterator()
			: m_set( NULL )
#ifdef _DEBUG
			, m_version( 0 )
#endif
		{};

		//! Constructor
		RED_INLINE const_iterator( const const_iterator& other )
			: m_set( other.m_set )
#ifdef _DEBUG
			, m_version( other.m_version )
#endif
			, m_iterator( other.m_iterator )
		{};

		//! Construct from non const iterator
		RED_INLINE const_iterator( const iterator& other )
			: m_set( other.m_set )
#ifdef _DEBUG
			, m_version( other.m_version )
#endif
			, m_iterator( other.m_iterator )
		{};

		//! Initialize
#ifdef _DEBUG
		RED_INLINE const_iterator( const typename TDynArray< K, MC_Type >::const_iterator& it, const TSortedSet< K, CompareFunc, MC_Type >* set, Uint32 version )
#else
		RED_INLINE const_iterator( const typename TDynArray< K, MC_Type >::const_iterator& it, const TSortedSet< K, CompareFunc, MC_Type >* set )
#endif
			: m_set( set )
#ifdef _DEBUG
			, m_version( version )
#endif
			, m_iterator( it )
		{};

		//! Compare iterator
		RED_INLINE Bool operator==( const const_iterator& other ) const
		{
			return m_iterator == other.m_iterator;
		}

		//! Compare iterator
		RED_INLINE Bool operator!=( const const_iterator& other ) const
		{
			return m_iterator != other.m_iterator;
		}

		//! Iterate to next
		RED_INLINE void operator++()
		{
			++m_iterator;
		}

		//! Iterate to next
		RED_INLINE void operator++( Int32 )
		{
			++m_iterator;
		}

		//! Get element
		RED_INLINE const K* operator->() const
		{
#ifdef _DEBUG
			RED_FATAL_ASSERT( m_set->m_version == m_version, "Iterator used after it was invalidated" );
#endif
			return &(*m_iterator);
		}

		//! Get element
		RED_INLINE const K& operator*() const
		{
#ifdef _DEBUG
			RED_FATAL_ASSERT( m_set->m_version == m_version, "Iterator used after it was invalidated" );
#endif
			return *m_iterator;
		}
	};

protected:
	TDynArray< K, MC_Type >				m_keyArray;
#ifdef _DEBUG
	Uint32								m_version;
#endif

public:
	//! Default constructor
	RED_INLINE TSortedSet()
#ifdef _DEBUG
		: m_version( 666 )
#endif
	{
	}

	//! Copy constructor
	RED_INLINE TSortedSet( const TSortedSet& set )
		: m_keyArray( set.m_keyArray )
#ifdef _DEBUG
		, m_version( set.m_version + 1 )
#endif
	{}

	RED_INLINE TSortedSet( TSortedSet && value )
		: m_keyArray( std::move( value.m_keyArray ) )
#ifdef _DEBUG
		, m_version( value.m_version + 1 )
#endif
	{
	}

	//! Assignment operator
	RED_INLINE const TSortedSet& operator=( const TSortedSet& set )
	{
#ifdef _DEBUG
		m_version = set.m_version + 1;
#endif
		TSortedSet( set ).Swap( *this );
		return *this;
	}

	//! Move assignment of an array
	RED_INLINE TSortedSet & operator=( TSortedSet && value )
	{
#ifdef _DEBUG
		// Added
		++m_version;
#endif
		TSortedSet( std::move( value ) ).Swap( *this );
		return *this;
	}

	//! Destroy this crap
	RED_INLINE ~TSortedSet()
	{
		Clear();
	}

public:
	//! Get the size of the set
	RED_INLINE Uint32 Size() const
	{
		return static_cast< Uint32 >(m_keyArray.Size());
	}

	//! Get size of the data
	RED_INLINE Uint32 DataSize() const
	{
		return static_cast< Uint32 >( m_keyArray.DataSize() );
	}

	RED_FORCE_INLINE TMemSize GetInternalMemSize() const
	{
		return ( TMemSize ) m_keyArray.SizeOfAllElements();
	}

	//! Get internal capacity
	RED_INLINE Uint32 Capacity() const
	{
		return m_keyArray.Capacity();
	}

	//! Is the set empty
	RED_INLINE bool Empty() const
	{
		return m_keyArray.Size() == 0;
	}

private:
	RED_INLINE Bool FindInsertIndex( Int32& index, const K& key ) const
	{
		Bool keyLess;
		Int32 begin = 0;
		Int32 end = Size() - 1;

		// We have some data already, so we're looking for this key 
		// If key doesn't exist we find the place where we want put new data
		do
		{
			index = (begin + end) / 2;
			if ( CompareFunc::Less( key, m_keyArray[index] ) )
			{
				keyLess = true;
				end = index - 1;
			}
			else if ( CompareFunc::Less( m_keyArray[index], key ) )
			{
				keyLess = false;
				begin = index + 1;
			}
			else
			{
				// Found this key in set, so we do not update data
				return false;
			}
		}
		while( begin <= end );

		const Int32 offset = ( keyLess == true ) ? 0 : 1;
		index += offset;

		return true;
	}

public:
	// Insert key only if key doesn't not exist in map already
	RED_INLINE Bool Insert( const K& key )
	{
		// Array is empty, so we insert first data to the set
		if ( m_keyArray.Size() == 0 )
		{
#ifdef _DEBUG
			++m_version;
#endif
			m_keyArray.PushBack( key );
			return true;
		}

		Int32 index = 0;
		if( FindInsertIndex( index, key ) )
		{
#ifdef _DEBUG
			// Added
			++m_version;
#endif
			return m_keyArray.Insert( index, key );
		}

		return false;
	}

	// Insert key only if key doesn't not exist in map already
	RED_INLINE Bool Insert( const K&& key )
	{
		// Array is empty, so we insert first data to the set
		if ( m_keyArray.Size() == 0 )
		{
#ifdef _DEBUG
			++m_version;
#endif
			m_keyArray.PushBack( std::move( key ) );
			return true;
		}

		Int32 index = 0;
		if( FindInsertIndex( index, key ) )
		{
#ifdef _DEBUG
			// Added
			++m_version;
#endif
			return m_keyArray.Insert( index, std::move( key ) );
		}

		return false;
	}

	//! Erase single element by key
	RED_INLINE Bool Erase( const K& key )
	{
		iterator it = Find( key );
		if( it != End() )
		{
#ifdef _DEBUG
			++m_version;
#endif
			return Erase( it );
		}
		return false;
	}

	//! Erase at position
	RED_INLINE Bool Erase( iterator& it )
	{
#ifdef _DEBUG
		++m_version;
#endif
		m_keyArray.Erase( it.m_iterator );
		return true;
	}

	//! Clear set
	RED_INLINE void Clear()
	{
#ifdef _DEBUG
		++m_version;
#endif
		m_keyArray.Resize(0);
	}

	//! Clear set
	RED_INLINE void ClearFast()
	{
#ifdef _DEBUG
		++m_version;
#endif
		m_keyArray.ClearFast();
	}

	//! Clear elements in the set
	RED_INLINE void ClearPtr()
	{
		for ( Uint32 i = 0; i < m_keyArray.Size(); ++i )
		{
			delete m_keyArray[i];
		}

#ifdef _DEBUG
		// Clear the set itself
		++m_version;
#endif
		m_keyArray.Resize(0);
	}

	//! Reserve memory in the key array
	RED_INLINE void Reserve( Uint32 size )
	{
		m_keyArray.Reserve( size );
	}

	// Find key, returns iterator to key or End() if key is not found
	template< class _L > RED_INLINE iterator Find( const _L& key )
	{
		if ( Size() > 0 )
		{
			Int32 begin = 0;
			Int32 end   = Size()-1;

			// Bisection search because m_keyArray is sorted
			do
			{
				Int32 index = (begin + end) / 2;
				if ( CompareFunc::Less(key, m_keyArray[index]) )
				{
					end = index - 1;
				}
				else if ( CompareFunc::Less(m_keyArray[index], key) )
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

	// Find key, returns iterator to key or End() if key is not found
	template< class _L >
	RED_INLINE const_iterator Find( const _L& key ) const
	{
		if ( Size() > 0 )
		{
			Int32 begin = 0;
			Int32 end   = Size()-1;

			// Bisection search because m_keyArray is sorted
			do
			{
				Int32 index = (begin + end) / 2;
				if ( CompareFunc::Less(key, m_keyArray[index]) )
				{
					end = index - 1;
				}
				else if ( CompareFunc::Less(m_keyArray[index], key) )
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

	// Check if key exists in map
	template< class _L >
	RED_INLINE Bool Exist( const _L& key ) const
	{
		return ( Find( key ) != End() );
	}

	// Compare
	RED_INLINE Bool operator==( const TSortedSet& map ) const
	{
		return m_keyArray == map.m_keyArray;
	}

	// Compare
	RED_INLINE Bool operator!=( const TSortedSet& map ) const
	{
		return (m_keyArray != map.m_keyArray);
	}

	// Get start
	RED_INLINE const_iterator Begin() const
	{
#ifdef _DEBUG
		return const_iterator( m_keyArray.Begin(), this, m_version );
#else
		return const_iterator( m_keyArray.Begin(), this );
#endif
	}

	// Get end
	RED_INLINE const_iterator End() const
	{
#ifdef _DEBUG
		return const_iterator( m_keyArray.End(), this, m_version );
#else
		return const_iterator( m_keyArray.End(), this );
#endif

	}

	// Get start
	RED_INLINE iterator Begin()
	{
#ifdef _DEBUG
		return iterator( m_keyArray.Begin(), this, m_version );
#else
		return iterator( m_keyArray.Begin(), this );
#endif
	}

	// Get end
	RED_INLINE iterator End()
	{
#ifdef _DEBUG
		return iterator( m_keyArray.End(), this, m_version );
#else
		return iterator( m_keyArray.End(), this );
#endif

	}

	// Remove
	RED_INLINE void Shrink()
	{
#ifdef _DEBUG
		++m_version;
#endif
		m_keyArray.Shrink();
	}

	// Insert all
	RED_INLINE void InsertSorted( const TDynArray< K >& data )
	{
		const Uint32 count = data.Size();
		m_keyArray.Resize( count );

		for ( Uint32 i=0; i<count; ++i )
		{
			m_keyArray[i] = data[i];
		}
	}

	// (HACKERY) Bulk insert an element - inserts it at the end of the element array.
	// IMPORTANT: you need to use Resort() after using this - if you don't you risk having a broken container
	RED_INLINE void BulkInsert( const K& data )
	{
		m_keyArray.PushBack( data );
	}

	// (HACKERY) Resort array - usually done after group of "BulkInsert"
	RED_INLINE void Resort()
	{
		::Sort( m_keyArray.Begin(), m_keyArray.End(), []( const K& a, const K& b ) -> bool { return CompareFunc::Less( a, b ); } );
	}

public:
	// Serialization
	friend IFile& operator<<( IFile& file, TSortedSet< K, CompareFunc, MC_Type > &ar )
	{
		file << ar.m_keyArray;
		return file;
	}

	RED_INLINE void Merge( const this_class& other )
	{
		this_class** sources = static_cast< this_class* >( RED_ALLOCA( sizeof( this_class* ) * 2 ) );

		sources[ 0 ] = this;
		sources[ 1 ] = &other;

		this_class out;
		Merge( out, sources, 2 );

		*this = std::move( out );
	}

	RED_INLINE static void Merge( this_class& destination, this_class** sources, Uint32 numSources )
	{
		Uint32* indicies = static_cast< Uint32* >( RED_ALLOCA( sizeof( Uint32 ) * numSources ) );

		Uint32 totalSize = 0;
		for( Uint32 i = 0; i < numSources; ++i )
		{
			// Count up the total number of elements to be added to the new array
			totalSize += sources[ i ]->Size();

			// Initialisation
			indicies[ i ] = 0;
		}

		destination.m_keyArray.Reserve( totalSize );

		Uint32 destIndex = 0;
		for( Uint32 iElement = 0; iElement < totalSize; ++iElement )
		{
			// Find the next element from amongst all of these sorted sources
			Uint32 lowestSource = 0;
			for( Uint32 iSource = 1; iSource < numSources; ++iSource )
			{
				const TDynArray< K, MC_Type >& a = sources[ iSource ]->m_keyArray;
				const TDynArray< K, MC_Type >& b = sources[ lowestSource ]->m_keyArray;

				if( CompareFunc::Less( a[ indicies[ iSource ] ], b[ indicies[ lowestSource ] ] ) )
				{
					lowestSource = iSource;
				}
			}

			const K& source = sources[ lowestSource ]->m_keyArray[ indicies[ lowestSource ] ];

			// Ignore identical keys
			//if( destIndex == 0 || !( destination.m_keyArray[ destIndex - 1 ].m_first == source.m_first ) )
			if( destIndex == 0 || CompareFunc::Less( destination.m_keyArray[ destIndex - 1 ], source ) )
			{
				// Copy element from source to destination
				destination.m_keyArray.PushBack( source );

				// 
				++destIndex;
			}

			// Only increment the index to the array we just copied from
			++indicies[ lowestSource ];

			// Check to see if we have reached the end of the source we just copied from
			// And if we have, remove it from future comparisons
			if( indicies[ lowestSource ] == sources[ lowestSource ]->Size() )
			{
				for( Uint32 i = lowestSource + 1; i < numSources; ++i )
				{
					sources[ i - 1 ] = sources[ i ];
					indicies[ i - 1 ] = indicies[ i ];
				}

				--numSources;
			}
		}
	}

	RED_INLINE void Swap( TSortedSet & value )
	{
		m_keyArray.SwapWith( value.m_keyArray );
#ifdef _DEBUG
		::Swap( m_version, value.m_version );
#endif
	}

	// (HACKERY) Save as bulk data
	RED_INLINE void SerializeBulk( IFile& file )
	{
		m_keyArray.SerializeBulk(file);
	}

};

// Enable c++11 range-based for loop

template < typename K, typename CompareFunc, EMemoryClass MC_Type >
typename TSortedSet< K, CompareFunc, MC_Type >::iterator begin( TSortedSet< K, CompareFunc, MC_Type >& set ) { return set.Begin(); }

template < typename K, typename CompareFunc, EMemoryClass MC_Type >
typename TSortedSet< K, CompareFunc, MC_Type >::iterator end( TSortedSet< K, CompareFunc, MC_Type >& set ) { return set.End(); }

template < typename K, typename CompareFunc, EMemoryClass MC_Type >
typename TSortedSet< K, CompareFunc, MC_Type >::const_iterator begin( const TSortedSet< K, CompareFunc, MC_Type >& set ) { return set.Begin(); }

template < typename K, typename CompareFunc, EMemoryClass MC_Type >
typename TSortedSet< K, CompareFunc, MC_Type >::const_iterator end( const TSortedSet< K, CompareFunc, MC_Type >& set ) { return set.End(); }
