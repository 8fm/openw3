/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef CONTAINERS_COMMON_H
#define CONTAINERS_COMMON_H

#include "../redSystem/types.h"
#include "../redSystem/error.h"

// !Debug code - enables overrun detection in arrays
#define ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION

namespace Red { namespace Containers {

	// This function allows us to safely resolve rvalue references for move semantics
	template< class T > struct RemoveReference      { typedef T Type; };
	template< class T > struct RemoveReference<T&>  { typedef T Type; };
	template< class T > struct RemoveReference<T&&> { typedef T Type; };
	template< class T >
	RED_INLINE typename RemoveReference< T >::Type&& ElementMoveConstruct( T&& v ) { return static_cast< typename RemoveReference<T>::Type&& >( v ); }

	// Default comparators used by various containers
	template< class ElementType >
	class DefaultEqualsComparator
	{
	public:
		RED_INLINE Red::System::Bool operator() ( const ElementType& t0, const ElementType& t1 ) const { return t0 == t1; }
	};

	template< class ElementType >
	class DefaultLessthanComparator
	{
	public:
		RED_INLINE Red::System::Bool operator() ( const ElementType& t0, const ElementType& t1 ) const { return t0 < t1; }
	};

	// Pair comparators only compare first value (assumed to be keys) by default
	// Relies on PairType having public typedef first_type and m_first element accessor
	template< class PairType >
	class DefaultPairEqualsComparator
	{
	public:
		// Compare pair to pair
		RED_INLINE Red::System::Bool operator() ( const PairType& p0, const PairType& p1 ) const
		{
			DefaultPairEqualsComparator< typename PairType::first_type > cmpFirstType;
			return cmpFirstType( p0.m_first, p1.m_first );
		}

		// Compare key to pair
		RED_INLINE Red::System::Bool operator() ( const typename PairType::first_type& key, const PairType& pair ) const
		{
			DefaultPairEqualsComparator< typename PairType::first_type > cmpFirstType;
			return cmpFirstType( key, pair.m_first );
		}

		// Compare pair to key
		RED_INLINE Red::System::Bool operator() ( const PairType& pair, const typename PairType::first_type& key ) const
		{
			DefaultPairEqualsComparator< typename PairType::first_type > cmpFirstType;
			return cmpFirstType( pair.m_first, key );
		}
	};

	template< class PairType >
	class DefaultPairLessthanComparator
	{
	public:
		// Compare pair to pair
		RED_INLINE Red::System::Bool operator() ( const PairType& p0, const PairType& p1 ) const
		{
			DefaultLessthanComparator< typename PairType::first_type > cmpFirstType;
			return cmpFirstType( p0.m_first, p1.m_first );
		}

		// Compare key to pair
		RED_INLINE Red::System::Bool operator() ( const typename PairType::first_type& key, const PairType& pair ) const
		{
			DefaultLessthanComparator< typename PairType::first_type > cmpFirstType;
			return cmpFirstType( key, pair.m_first );
		}

		// Compare pair to key
		RED_INLINE Red::System::Bool operator() ( const PairType& pair, const typename PairType::first_type& key ) const
		{
			DefaultLessthanComparator< typename PairType::first_type > cmpFirstType;
			return cmpFirstType( pair.m_first, key );
		}
	};

} }

#endif