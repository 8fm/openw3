/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef RED_CONTAINERS_PAIR_H
#define RED_CONTAINERS_PAIR_H

namespace Red { namespace Containers {

	// Pair of elements used in other containers  
	template< class ElementType1, class ElementType2, 
			  class LessThanCompareType1 = DefaultLessthanComparator< ElementType1 >, class LessThanCompareType2 = DefaultLessthanComparator< ElementType2 >,
		      class EqualsCompareType1 = DefaultEqualsComparator< ElementType1 >, class EqualsCompareType2 = DefaultEqualsComparator< ElementType2 > >
	class Pair
	{
	public:
		RED_INLINE Pair()
		{
		}

		RED_INLINE Pair( const ElementType1& t1, const ElementType2& t2 )
			: m_first( t1 )
			, m_second( t2 )
		{
		}

		RED_INLINE Pair( const Pair& p )
			: m_first( p.m_first )
			, m_second( p.m_second )
		{
		}

		RED_INLINE ~Pair()
		{
		}

		RED_INLINE Pair& operator=( const Pair& p )
		{
			m_first = p.m_first;
			m_second = p.m_second;
			return *this;
		}
		
		RED_INLINE Red::System::Bool operator==( const Pair& p )
		{
			return EqualsCompareType1( m_first, p.m_first ) && EqualsCompareType2( m_second, p.m_second );
		}

		RED_INLINE Red::System::Bool operator!=( const Pair& p )
		{
			return !( *this == p );
		}

		RED_INLINE Red::System::Bool operator<( const Pair& p )
		{
			return LessThanCompareType1( m_first, p.m_first ) || ( EqualsCompareType1( m_first, p.m_first ) && LessThanCompareType2( m_second, p.m_second ) );
		}

		RED_INLINE Red::System::Bool operator<=( const Pair& p )
		{
			return LessThanCompareType1( m_first, p.m_first ) || ( EqualsCompareType1( m_first, p.m_first ) && !LessThanCompareType2( p.m_second, m_second ) );
		}

		RED_INLINE Red::System::Bool operator>( const Pair& p )
		{
			return LessThanCompareType1( p.m_first, m_first ) || ( EqualsCompareType1( m_first, p.m_first ) && LessThanCompareType2( p.m_second, m_second ) );
		}

		RED_INLINE Red::System::Bool operator>=( const Pair& p )
		{
			return LessThanCompareType1( p.m_first, m_first ) || ( EqualsCompareType1( m_first, p.m_first ) && !LessThanCompareType2( m_second, p.m_second ) );
		}

		ElementType1 m_first;
		ElementType2 m_second;
		typedef ElementType1 first_type;
		typedef ElementType2 second_type;
	};

} }

#endif