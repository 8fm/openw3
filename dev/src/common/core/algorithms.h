/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#ifndef _ALGORITHMS_H
#define _ALGORITHMS_H

#include "types.h"


// Note: Overloaded on basic types so class constant integer inline defines can be used with them.
// Otherwise Clang could cause a linker error for the template version that takes references. Could have used a #define instead in the first place.

RED_INLINE Uint32 Max( Uint32 a, Uint32 b)
{ 
	return (a>=b) ? a : b; 
}

RED_INLINE Int32 Max( Int32 a, Int32 b)
{ 
	return (a>=b) ? a : b; 
}

RED_INLINE Uint64 Max( Uint64 a, Uint64 b)
{ 
	return (a>=b) ? a : b; 
}

RED_INLINE Int64 Max( Int64 a, Int64 b)
{ 
	return (a>=b) ? a : b; 
}

RED_INLINE Uint32 Min( Uint32 a, Uint32 b )
{ 
	return (a<=b) ? a : b; 
}

RED_INLINE Int32 Min( Int32 a, Int32 b )
{ 
	return (a<=b) ? a : b; 
}

RED_INLINE Uint64 Min( Uint64 a, Uint64 b )
{ 
	return (a<=b) ? a : b; 
}

RED_INLINE Int64 Min( Int64 a, Int64 b )
{ 
	return (a<=b) ? a : b; 
}

template <class T> 
RED_INLINE T Max(const T& a, const T& b)   
{ 
	return (a>=b) ? a : b; 
}

template <class T> 
RED_INLINE T Min(const T& a, const T& b)   
{ 
	return (a<=b) ? a : b; 
}

template <class T> 
RED_INLINE T Max(const T& a, const T& b, const T& c)   
{ 
	return Max( Max( a, b ), c );
}

template <class T> 
RED_INLINE T Min(const T& a, const T& b, const T& c)   
{ 
	return Min( Min( a, b ), c );
}

template <class T> 
RED_INLINE T Clamp(const T& x, const T& min, const T& max)		
{ 
	return Min( Max( x, min ), max );
}

template <class T> 
RED_INLINE T Abs(const T& a)				
{ 
	return (a>=T(0)) ? a : -a; 
}
template <>
RED_INLINE Float Abs< Float >(const Float& a)				
{ 
	return fabs( a ) ;
}


template <class T> 
RED_INLINE Int32 Sgn(const T& a)				
{ 
	static const T zero = T(0);
	return (a>zero) ? 1 : ( a<zero ? -1 : 0 );
}

template <class T> 
RED_INLINE void Swap(T& a, T& b)								
{ 
	T tmp = std::move(a); 
	a = std::move(b);	
	b = std::move(tmp); 
}

template < class Val >
RED_INLINE Val ArithmeticAverage( const Val& v1, const Val& v2 )
{
	return (v1 + v2) / 2.f;
}

// Simple template to compute pure type from possibly type reference
template< class T > struct RemoveReference      { typedef T Type; };
template< class T > struct RemoveReference<T&>  { typedef T Type; };
template< class T > struct RemoveReference<T&&> { typedef T Type; };

// Cast type that suggest usage of move constructor/assignement
template< class T >
RED_INLINE typename RemoveReference< T >::Type&& Move( T&& v ) { return static_cast< typename RemoveReference<T>::Type&& >( v ); }

template < >
RED_INLINE Int32 ArithmeticAverage< Int32 >( const Int32& v1, const Int32& v2 ) { return (v1 + v2) / 2; }
template < >
RED_INLINE Uint32 ArithmeticAverage< Uint32 >( const Uint32& v1, const Uint32& v2 ) { return (v1 + v2) / 2; }

template <class T> 
RED_INLINE T Lerp( Float frac, const T& a, const T& b )
{ 
	return a + ( ( b - a ) * frac ) ;
}

template < class T >
struct Less
{
	RED_INLINE Bool operator()(const T& c1, const T& c2) const
	{
		return c1 < c2;
	}
};

template < class T >
struct Greater
{
	RED_INLINE Bool operator()(const T& c1, const T& c2) const
	{
		return c1 > c2;
	}
};

namespace SortUtils
{
	// Median of 3 elements

	template < typename TYPE >
	RED_FORCE_INLINE TYPE MedianOf3( TYPE *data, Int32 lo, Int32 mid, Int32 hi )
	{
		return ( data[ mid ] < data[ lo ] ) ?
			( ( data[ hi ] < data[ mid ] ) ? data[ mid ] : data[ data[ hi ] < data[ lo ] ? hi : lo ] ) :
			( ( data[ hi ] < data[ mid ] ) ? data[ data[ hi ] < data[ lo ] ? lo : hi ] : data[ mid ] );
	}

	template < typename TYPE, typename _Pred >
	RED_FORCE_INLINE TYPE MedianOf3( TYPE *data, Int32 lo, Int32 mid, Int32 hi, const _Pred& pred )
	{
		return ( pred( data[ mid ], data[ lo ] ) ) ?
			( ( pred( data[ hi ], data[ mid ] ) ) ? data[ mid ] : data[ pred( data[ hi ], data[ lo ]) ? hi : lo ] ) :
			( ( pred( data[ hi ], data[ mid ] ) ) ? data[ pred( data[ hi ], data[ lo ]) ? lo : hi ] : data[ mid ] );
	}

	// Partitioning

	template < typename TYPE >
	static RED_FORCE_INLINE Int32 Partition( TYPE* data, Int32 lo, Int32 hi, TYPE& x )
	{
		while ( 1 )
		{
			// Skip smaller to the left

			while ( data[ lo ] < x )
			{
				++lo;
			}

			// Skip larger to the right

			--hi;
			while ( x < data[ hi ] )
			{
				--hi;
			}

			// Processed all elements? - done

			if ( lo >= hi )
			{
				return lo;
			}

			// Swap elements and continue

			::Swap( data[ lo ], data[ hi ] );
			++lo;
		}
	}

	template < typename TYPE, typename _Pred >
	static RED_FORCE_INLINE Int32 Partition( TYPE* data, Int32 lo, Int32 hi, TYPE& x, const _Pred& pred )
	{
		while ( 1 )
		{
			// Skip smaller to the left

			while ( pred( data[ lo ], x ) )
			{
				++lo;
			}

			// Skip larger to the right

			--hi;
			while ( pred( x, data[ hi ] ) )
			{
				--hi;
			}

			// Processed all elements? - done

			if ( lo >= hi )
			{
				return lo;
			}

			// Swap elements and continue

			::Swap( data[ lo ], data[ hi ] );
			++lo;
		}
	}

	// Heap

	template < typename TYPE >
	static void DownHeap( TYPE* data, Int32 i, Int32 n, Int32 lo )
	{
		TYPE d = data[ lo + i - 1 ];
		const Int32 n2 = n >> 1;

		Int32 childIndex;
		while ( i <= n2 )
		{
			childIndex = i << 1;
			if ( childIndex < n && data[ lo + childIndex - 1 ] < data[ lo + childIndex ] )
			{
				++childIndex;
			}
			if ( data[ lo + childIndex - 1 ] < d )
			{
				break;
			}
			data[ lo + i - 1 ] = data[ lo + childIndex - 1 ];
			i = childIndex;
		}

		data[ lo + i - 1 ] = d;
	}

	template < typename TYPE, typename _Pred >
	static void DownHeap( TYPE* data, Int32 i, Int32 n, Int32 lo, const _Pred& pred )
	{
		TYPE d = data[ lo + i - 1 ];
		const Int32 n2 = n >> 1;

		Int32 childIndex;
		while ( i <= n2 )
		{
			childIndex = i << 1;
			if ( childIndex < n && pred( data[ lo + childIndex - 1 ], data[ lo + childIndex ] ) )
			{
				++childIndex;
			}
			if ( pred( data[ lo + childIndex - 1 ], d ) )
			{
				break;
			}
			data[ lo + i - 1 ] = data[ lo + childIndex - 1 ];
			i = childIndex;
		}

		data[ lo + i - 1 ] = d;
	}

	template < typename TYPE >
	static void HeapSort( TYPE* data, Int32 lo, Int32 hi )
	{
		const Int32 n = hi - lo;

		// Make heap in O(N)

		for ( Int32 i = n >> 1; i >= 1; --i )
		{
			DownHeap( data, i, n, lo );
		}

		// Extract smallest element N times and put it at consecutive indices (which results in sorted array)
		// Cost: O(NlogN)

		for ( Int32 i = n; i > 1; --i )
		{
			::Swap( data[lo], data[lo + i - 1] );
			DownHeap( data, 1, i - 1, lo );
		}
	}

	template < typename TYPE, typename _Pred >
	static void HeapSort( TYPE* data, Int32 lo, Int32 hi, const _Pred& pred )
	{
		const Int32 n = hi - lo;

		// Make heap in O(N)

		for ( Int32 i = n >> 1; i >= 1; --i )
		{
			DownHeap( data, i, n, lo, pred );
		}

		// Extract smallest element N times and put it at consecutive indices (which results in sorted array)
		// Cost: O(NlogN)

		for ( Int32 i = n; i > 1; --i )
		{
			::Swap( data[lo], data[lo + i - 1] );
			DownHeap( data, 1, i - 1, lo, pred );
		}
	}

	// Insertion sort

	template < typename TYPE >
	static void InsertionSort( TYPE* data, Int32 lo, Int32 hi )
	{
		// Process all elements

		for ( Int32 i = lo; i < hi; ++i )
		{
			Int32 j = i;
			TYPE curr = data[ i ];

			// Move larger elements to the right

			while ( j != lo && curr < data[ j - 1 ] )
			{
				data[ j ] = data[ j - 1 ];
				j--;
			}

			// Move processed element to the left

			data[ j ] = curr;
		}
	}

	template < typename TYPE, typename _Pred >
	static void InsertionSort( TYPE* data, Int32 lo, Int32 hi, const _Pred& pred )
	{
		// Process all elements

		for ( Int32 i = lo; i < hi; ++i )
		{
			Int32 j = i;
			TYPE curr = data[ i ];

			// Move larger elements to the right

			while ( j != lo && pred( curr, data[ j - 1 ] ) )
			{
				data[ j ] = data[ j - 1 ];
				j--;
			}

			// Move processed element to the left

			data[ j ] = curr;
		}
	}

	template < typename TYPE >
	static void IntroSortRec( TYPE* data, Int32 lo, Int32 hi, Int32 depthLimit )
	{
		static const Int32 maxSortedBlockSize = 16;

		while ( hi - lo > maxSortedBlockSize )
		{
			// Depth limit reached? - do the heapsort to assure total cost of O(NlogN)

			if ( !depthLimit )
			{
				HeapSort( data, lo, hi );
				return;
			}
			depthLimit--;

			// Split according to "median of 3"

			Int32 partitionIndex;
			{
				TYPE median = MedianOf3( data, lo, lo + ( ( hi - lo ) >> 1 ) + 1, hi - 1 );
				partitionIndex = Partition( data, lo, hi, median );
			}

			// Process right set recursively

			IntroSortRec( data, partitionIndex, hi, depthLimit );

			// Process left set in data loop

			hi = partitionIndex;
		}
	}

	template < typename TYPE, typename _Pred >
	static void IntroSortRec( TYPE* data, Int32 lo, Int32 hi, Int32 depthLimit, const _Pred& pred )
	{
		static const Int32 maxSortedBlockSize = 16;

		while ( hi - lo > maxSortedBlockSize )
		{
			// Depth limit reached? - do the heapsort to assure total cost of O(NlogN)

			if ( !depthLimit )
			{
				HeapSort( data, lo, hi, pred );
				return;
			}
			depthLimit--;

			// Split according to "median of 3"

			Int32 partitionIndex;
			{
				TYPE median = MedianOf3( data, lo, lo + ( ( hi - lo ) >> 1 ) + 1, hi - 1, pred );
				partitionIndex = Partition( data, lo, hi, median, pred );
			}

			// Process right set recursively

			IntroSortRec( data, partitionIndex, hi, depthLimit, pred );

			// Process left set in data loop

			hi = partitionIndex;
		}
	}

	RED_FORCE_INLINE Int32 CalcSortDepthLimit( Int32 n )
	{
		return ( Int32 ) ::floor( ::log( n ) / ::log( 2 ) ) << 1;
	}

	// Introspective sort
	template < typename _Type >
	void IntroSort( _Type* data, Int32 n )
	{
		// Do the quicksort pass combined (with heapsort where recursion depth exceeds logN)

		IntroSortRec( data, 0, n, CalcSortDepthLimit( n ) );

		// Finalize sorting of small blocks using insertion sort

		InsertionSort( data, 0, n );
	}

	// Introspective sort with comparison predicate
	template < typename _Type, typename _Pred >
	void IntroSort( _Type* data, Int32 n, const _Pred& pred )
	{
		// Do the quicksort pass combined (with heapsort where recursion depth exceeds logN)

		IntroSortRec( data, 0, n, CalcSortDepthLimit( n ), pred );

		// Finalize sorting of small blocks using insertion sort

		InsertionSort( data, 0, n, pred );
	}

#ifdef RED_ASSERTS_ENABLED
	// Validates compare operator is implemented correctly
	template < typename _Iter >
	void ValidateComparison( _Iter start, _Iter end )
	{
		if ( start == end )
		{
			return;
		}

		// Verify compare for the same element

		_Iter it = start;
		while ( it != end )
		{
			const Bool compare = *it < *it;
			RED_ASSERT( !compare, TXT("Invalid compare operator: a<a mustn't be true for any a. Please implement correct 'less' operator.") );
			++it;
		}

		// Verify compare for different elements

		_Iter left = start;
		_Iter right = start; ++right;
		while ( right != end )
		{
			const Bool leftCompare = *left < *right;
			const Bool rightCompare = *right < *left;
			RED_ASSERT( !leftCompare || !rightCompare, TXT("Invalid compare operator: at least one of a<b or b<a must be false for any a and b. Please implement correct 'less' operator.") );
			left = right;
			++right;
		}
	}

	// Validates compare operator is implemented correctly
	template < typename _Iter, typename _Pred >
	void ValidateComparison( _Iter start, _Iter end, const _Pred& pred )
	{
		if ( start == end )
		{
			return;
		}

		// Verify compare for the same element

		_Iter it = start;
		while ( it != end )
		{
			const Bool compare = pred( *it, *it );
			RED_ASSERT( !compare, TXT("Invalid compare operator: a<a mustn't be true for any a. Please implement correct 'less' predicate.") );
			++it;
		}

		// Verify compare for different elements

		_Iter left = start;
		_Iter right = start; ++right;
		while ( right != end )
		{
			const Bool leftCompare = pred( *left, *right );
			const Bool rightCompare = pred( *right, *left );
			RED_ASSERT( !leftCompare || !rightCompare, TXT("Invalid compare predicate: at least one of a<b or b<a must be false for any a and b. Please implement correct 'less' predicate.") );
			left = right;
			++right;
		}
	}
#endif
}

#ifdef RED_ASSERTS_ENABLED
	#define SORT_UTILS_VALIDATE_COMPARE_OPERATOR( begin, end )			SortUtils::ValidateComparison( begin, end )
	#define SORT_UTILS_VALIDATE_COMPARE_PREDICATE( begin, end, pred )	SortUtils::ValidateComparison( begin, end, pred )
#else
	#define SORT_UTILS_VALIDATE_COMPARE_OPERATOR( begin, end )
	#define SORT_UTILS_VALIDATE_COMPARE_PREDICATE( begin, end, pred )
#endif

/**
 *	Insertion sort.
 *
 *	Notes on performance:
 *	Fast for when number of inversions is low, slow otherwise.
 */
template < typename _Iter >
RED_FORCE_INLINE void InsertionSort( _Iter start, _Iter end )
{
	SORT_UTILS_VALIDATE_COMPARE_OPERATOR( start, end );
	SortUtils::InsertionSort( start, 0, PtrDiffToInt32( ( void* )( end - start ) ) );
}

/**
 *	Insertion sort.
 *
 *	Notes on performance:
 *	Fast for when number of inversions is low, slow otherwise.
 */
template < typename _Iter, typename _Pred >
RED_FORCE_INLINE void InsertionSort( _Iter start, _Iter end, const _Pred& pred )
{
	SORT_UTILS_VALIDATE_COMPARE_PREDICATE( start, end, pred );
	SortUtils::InsertionSort( start, 0, PtrDiffToInt32( ( void* )( end - start ) ), pred );
}

/**
 *	Heap sort.
 *
 *	Reliable performance of O(NlogN).
 *	On average 5-10 times slower than general purpose Sort().
 */
template < typename _Iter >
RED_FORCE_INLINE void HeapSort( _Iter start, _Iter end )
{
	SORT_UTILS_VALIDATE_COMPARE_OPERATOR( start, end );
	SortUtils::HeapSort( start, 0, PtrDiffToInt32( ( void* )( end - start ) ) );
}

/**
 *	Heap sort.
 *
 *	Reliable performance of O(NlogN).
 *	On average 5-10 times slower than general purpose Sort().
 */
template < typename _Iter, typename _Pred >
RED_FORCE_INLINE void HeapSort( _Iter start, _Iter end, const _Pred& pred )
{
	SORT_UTILS_VALIDATE_COMPARE_PREDICATE( start, end, pred );
	SortUtils::HeapSort( start, 0, PtrDiffToInt32( ( void* )( end - start ) ), pred );
}

/**
 *	General purpose sorting function.
 *
 *	Based on introspective sort (http://en.wikipedia.org/wiki/Introsort)
 *	Guaranteed O(NlogN) performance.
 *
 *	More specifically a combination of:
 *	- quicksort (down to logN recursion depth)
 *	- heapsort (below logN recursion level)
 *	- insertion sort (for small blocks)
 */
template < typename _Iter >
RED_FORCE_INLINE void Sort( _Iter start, _Iter end )
{
	SORT_UTILS_VALIDATE_COMPARE_OPERATOR( start, end );
	SortUtils::IntroSort( start, PtrDiffToInt32( ( void* )( end - start ) ) );
}

/**
 *	General purpose sorting function.
 *
 *	Based on introspective sort (http://en.wikipedia.org/wiki/Introsort)
 *	Guaranteed O(NlogN) performance.
 *
 *	More specifically a combination of:
 *	- quicksort (down to logN recursion depth)
 *	- heapsort (below logN recursion level)
 *	- insertion sort (for small blocks)
 */
template < typename _Iter, typename _Pred >
RED_FORCE_INLINE void Sort( _Iter start, _Iter end, const _Pred& pred )
{
	SORT_UTILS_VALIDATE_COMPARE_PREDICATE( start, end, pred );
	SortUtils::IntroSort( start, PtrDiffToInt32( ( void* )( end - start ) ), pred );
}

template <class _Iter, class _Val>
RED_INLINE _Iter Find( _Iter begin, _Iter end, const _Val &val )
{
	while( begin != end )
	{
		if ( *begin == val )
			return begin;

		++begin;
	}

	return end;
}

template <class _Iter, class _Pred>
RED_INLINE _Iter FindIf( _Iter begin, _Iter end, const _Pred &pred )
{
	while( begin != end )
	{
		if ( pred( *begin ) )
			return begin;

		++begin;
	}

	return end;
}

// This functions returns iterator to greatest value lower or equal to given value (LowerBound function is not doing that).
template<class _Iter, class _Val>
RED_INLINE _Iter LowerBoundIndex( _Iter begin, _Iter end, const _Val &val )
{
	_Iter endCopy = end;
	Uint32 count = static_cast< Uint32 >( end - begin );

	while( count > 0 )
	{
		Uint32 halfCount = count / 2;

		_Iter middle = begin + halfCount;
		if ( *middle < val )
		{
			if ( ( middle + 1 < end && *( middle + 1 ) > val ) || middle + 1 == endCopy ) return middle;
			begin = middle+1;
			count = count - halfCount - 1;
		}
		else
		{
			count = halfCount;
		}	
	}

	return begin;
}

template<class _Iter, class _Val>
RED_INLINE _Iter LowerBound( _Iter begin, _Iter end, const _Val &val )
{
	Uint32 count = static_cast< Uint32 >( end - begin );

	while( count > 0 )
	{
		Uint32 halfCount = count / 2;

		_Iter middle = begin + halfCount;
		if ( *middle < val )
		{
			begin = middle+1;
			count = count - halfCount - 1;
		}
		else
		{
			count = halfCount;
		}	
	}

	return begin;
}

template<class _Iter, class _Val, class _Pred>
RED_INLINE _Iter LowerBound( _Iter begin, _Iter end, const _Val &val, const _Pred &pred )
{
	Uint32 count = PtrDiffToUint32( (void*)(end - begin));

	while( count > 0 )
	{
		Uint32 halfCount = count / 2;

		_Iter middle = begin + halfCount;
		if ( pred( *middle, val ) )
		{
			begin = middle+1;
			count = count - halfCount - 1;
		}
		else
		{
			count = halfCount;
		}	
	}

	return begin;
}

template<class _Iter, class _Val>
RED_INLINE _Iter UpperBound( _Iter begin, _Iter end, const _Val &val )
{	
	Uint32 count = PtrDiffToUint32( ( void* )(end - begin) );

	while( count > 0 )
	{
		Uint32 halfCount = count / 2;

		_Iter middle = begin + halfCount;
		if ( !( val < *middle ) )
		{
			begin = middle+1;
			count = count - halfCount - 1;
		}
		else
		{
			count = halfCount;
		}	
	}

	return begin;
}

template<class _Iter, class _Val, class _Pred>
RED_INLINE _Iter UpperBound( _Iter begin, _Iter end, const _Val &val, const _Pred &pred )
{
	Uint32 count = PtrDiffToUint32( ( void* )( end - begin ) );

	while( count > 0 )
	{
		Uint32 halfCount = count / 2;

		_Iter middle = begin + halfCount;
		if ( !pred( val, *middle ) )
		{
			begin = middle+1;
			count = count - halfCount - 1;
		}
		else
		{
			count = halfCount;
		}	
	}

	return begin;
}

template<class _Iter, class _Val>
RED_INLINE Bool BinarySearch( _Iter begin, _Iter end, const _Val &val )
{
	_Iter lowerBound = LowerBound( begin, end, val );
	return ( lowerBound != end ) && ( !( val < *lowerBound ) );
}

template<class _Iter, class _Val, class _Pred>
RED_INLINE Bool BinarySearch( _Iter begin, _Iter end, const _Val &val, const _Pred &pred )
{
	_Iter lowerBound = LowerBound( begin, end, val, pred );
	return ( lowerBound != end ) && ( !pred( val, *lowerBound ) );
}

template<class _Iter, class _Fn>
RED_INLINE void ForEach(_Iter begin, _Iter end,_Fn func)
{
    for (; begin != end; ++begin)
    {
        func(*begin);
    }
}

template< class _Collection, class _Fn >
RED_INLINE void ForEach(_Collection& coll,_Fn func)
{
	for ( auto it = coll.Begin(), end = coll.End(); it != end; ++it )
	{
		func(*it);
	}
}

template< class _Collection, class _Fn >
RED_INLINE void ForEach(const _Collection& coll,_Fn func)
{
	for ( auto it = coll.Begin(), end = coll.End(); it != end; ++it )
	{
		func(*it);
	}
}

template< class _Res, class _Collection, class _Fn >
RED_INLINE _Res ForEachSumResult(const _Collection& coll,_Fn func)
{
	_Res result = 0;
	for ( auto it = coll.Begin(), end = coll.End(); it != end; ++it )
	{
		result += func(*it);
	}
	return result;
}

template < class Val, class Functor >
RED_INLINE Val FunctionalBinarySearch( const Val& minVal, const Val& maxVal, Functor functor )
{
	//ASSERT( functor.Accept(minVal) );
	Val vMin = minVal;			// Inclusive
	Val vMax = maxVal;			// Exclusive
	while ( !functor.Stop( vMax, vMin ) )
	{
		Val vRet = ArithmeticAverage< Val >( vMax, vMin );
		if (functor.Accept(vRet))
		{
			vMin = vRet;
		}
		else
		{
			vMax = vRet;
		}
	}
	return vMin;
};

template< typename Iterator, typename Predicate >
RED_INLINE Iterator RemoveIf( Iterator first, Iterator last, Predicate predicate )
{
	Iterator result = first;
	while ( first != last ) 
	{
		if( !predicate( *first ) ) 
		{
			*result = std::move( *first );
			++result;
		}
		++first;
	}
	return result;
}

template < class Iterator, class Compare >
Bool IsSorted(Iterator itBegin,Iterator itEnd,const Compare& cOrder)
{
	if (itBegin == itEnd)
		return true;
	for (Iterator it = itBegin + 1; it != itEnd; it++)
	{
		if (cOrder(*it,*(it - 1)))
		{
			return false;
		}
	}
	return true;
}

// Unique Algorithm. Removes all but the first element from every consecutive group of equivalent elements in the range.
// Same behavior of std::unique algorithm.
template< typename Iterator >
RED_INLINE Iterator Unique( Iterator first, Iterator last )
{
	if( first != last )
	{
		Iterator result = first;
		while( ++first != last )
		{
			if( !( *result == *first ) )
			{
				*( ++result ) = std::move( *first );
			}
		}
		return ++result;
	}

	return last;
}

#endif /* _ALGORITHMS_H */
