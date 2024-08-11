/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "compare.h"
#include "file.h"
#include "hash.h"

template <
		   typename Type1,
		   typename Type2,
		   typename CompareFuncType1 = DefaultCompareFunc< Type1 >,
		   typename CompareFuncType2 = DefaultCompareFunc< Type2 >,
		   typename EqualFuncType1 = DefaultEqualFunc< Type1 >,
		   typename EqualFuncType2 = DefaultEqualFunc< Type2 >
		 >
class TPair
{
public:
	typedef Type1 first_type;
	typedef Type2 second_type;

public:
	Type1 m_first;
	Type2 m_second;

	RED_INLINE TPair()
		:	m_first(),
			m_second()
	{
	}
	RED_INLINE TPair( const Type1& first, const Type2 & second )
		:	m_first( first ),
			m_second( second )
	{
	}

	RED_INLINE TPair( const TPair& pair )
		:	m_first( pair.m_first ), 
			m_second( pair.m_second )
	{
	}

	template< typename Other1, typename Other2 >
	RED_INLINE TPair( Other1 && first, Other2 && second )
		:	m_first( std::forward< Other1 >( first ) ),
			m_second( std::forward< Other2 >( second ) )
	{}

	RED_INLINE TPair( TPair&& pair )
		:	m_first( std::forward< Type1 >( pair.m_first ) ), 
			m_second( std::forward< Type2 >( pair.m_second ) )
	{
	}

	RED_INLINE TPair & operator=( const TPair& pair )
	{
		if( this != &pair )
		{
			m_first = pair.m_first;
			m_second = pair.m_second;
		}
		return *this;
	}

	RED_INLINE TPair & operator=( TPair&& pair )
	{
		TPair( std::move( pair ) ).Swap( *this );
		return *this;
	}

	RED_INLINE Bool operator==( const TPair& pair ) const
	{
		return EqualFuncType1::Equal( m_first, pair.m_first ) && EqualFuncType2::Equal( m_second, pair.m_second );

		// by Dex: Makes no sense in most of the cases, removed
/*		return ( ( !CompareFuncType1::Less( m_first, pair.m_first ) && !CompareFuncType1::Less( pair.m_first, m_first ) ) &&
				 ( !CompareFuncType2::Less( m_second, pair.m_second ) && !CompareFuncType2::Less( pair.m_second, m_second ) ) );*/
	}

	RED_INLINE Bool operator!=( const TPair& pair ) const
	{
		return !EqualFuncType1::Equal( m_first, pair.m_first ) || !EqualFuncType2::Equal( m_second, pair.m_second );

		// by Dex: Makes no sense in most of the cases, removed
/*		return ( ( CompareFuncType1::Less( m_first, pair.m_first ) || CompareFuncType1::Less( pair.m_first, m_first ) ) ||
			     ( CompareFuncType2::Less( m_second, pair.m_second ) || CompareFuncType2::Less( pair.m_second, m_second ) ) );*/
	}

	RED_INLINE Bool operator<( const TPair& pair ) const
	{
		return ( CompareFuncType1::Less( m_first, pair.m_first ) ||
			     ( ( !CompareFuncType1::Less( m_first, pair.m_first ) && !CompareFuncType1::Less( pair.m_first, m_first ) ) && CompareFuncType2::Less( m_second, pair.m_second ) ) );
	}

	RED_INLINE Bool operator>( const TPair& pair ) const
	{
		return ( CompareFuncType1::Less( pair.m_first, m_first ) ||
			     ( ( !CompareFuncType1::Less( m_first, pair.m_first ) && !CompareFuncType1::Less( pair.m_first, m_first ) ) && CompareFuncType2::Less( pair.m_second, m_second ) ) );
	}

	RED_INLINE Bool operator>=( const TPair& pair ) const
	{
		return ( CompareFuncType1::Less( pair.m_first, m_first ) ||
				 ( ( !CompareFuncType1::Less( m_first, pair.m_first ) && !CompareFuncType1::Less( pair.m_first, m_first ) ) && !CompareFuncType2::Less( m_second, pair.m_second ) ) );
	}

	RED_INLINE Bool operator<=( const TPair& pair ) const
	{
		return ( CompareFuncType1::Less( m_first, pair.m_first ) ||
				 ( ( !CompareFuncType1::Less( m_first, pair.m_first ) && !CompareFuncType1::Less( pair.m_first, m_first ) ) && !CompareFuncType2::Less( pair.m_second, m_second ) ) );
	}

	RED_FORCE_INLINE Uint32 CalcHash() const
	{
		return GetHash( m_first ) ^ GetHash( m_second );
	}

public:
	// Serialization
	friend IFile& operator<<( IFile& file, TPair<Type1, Type2>& pair )
	{
		file << pair.m_first;
		file << pair.m_second;
		return file;
	}

	void Swap( TPair & pair )
	{
		::Swap( m_first, pair.m_first );
		::Swap( m_second, pair.m_second );
	}
};

template < typename K, typename V, typename CompareFunc >
struct ComparePairFunc
{
	static RED_INLINE Bool Less( const TPair<K,V>& key1, const TPair<K,V>& key2 ) { return CompareFunc::Less(key1.m_first, key2.m_first); }	
};

template < typename K, typename V, typename CompareFunc >
struct EqualPairFunc
{
	static RED_INLINE Bool Equal( const TPair<K,V>& key1, const TPair<K,V>& key2 ) { return CompareFunc::Equal(key1.m_first, key2.m_first); }
};

template <typename Type1, typename Type2>
RED_INLINE TPair< typename std::decay< Type1 >::type, typename std::decay< Type2 >::type > MakePair( Type1 && first, Type2 && second )
{
	return TPair< typename std::decay< Type1 >::type, typename std::decay< Type2 >::type >( std::forward< Type1 >( first ), std::forward< Type2 >( second ) );
};

template <typename T1, typename T2> struct TCopyableType< TPair< T1, T2 > >			{ enum { Value = TCopyableType<T1>::Value && TCopyableType<T2>::Value }; };

template <typename T1, typename T2> struct TPlainType< TPair< T1, T2 > >
{
	enum { Value = TPlainType<T1>::Value && TPlainType<T2>::Value	};
};
