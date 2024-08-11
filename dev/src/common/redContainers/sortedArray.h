/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_CONTAINER_SORTED_ARRAY_H
#define RED_CONTAINER_SORTED_ARRAY_H

#include "array.h"

namespace Red { namespace Containers {

	// SortedArray. A wrapper around a standard array, but with ordered inserts and sorting operators
	// Note that using any functions other than the ones declared explicitly in SortedArray can invalidate
	// the order of elements in the underlying array
	template< class ArrayType, class Comparator = DefaultLessthanComparator< typename ArrayType::value_type > >
	class SortedArray : public ArrayType
	{
	public:
		SortedArray();
		SortedArray( SortedArray< ArrayType, Comparator >&& other );
		SortedArray( const SortedArray< ArrayType, Comparator >& other );
		~SortedArray();

		SortedArray< ArrayType, Comparator >& operator=( const SortedArray< ArrayType, Comparator >& other );
		SortedArray< ArrayType, Comparator >& operator=( const SortedArray< ArrayType, Comparator >&& other );

		// Insert in-order
		typename ArrayType::iterator Insert( const typename ArrayType::value_type& element );
		typename ArrayType::iterator Insert( typename ArrayType::value_type&& element );

		// Find assumes the array is in fact, sorted, and uses a binary search
		typename ArrayType::iterator Find( const typename ArrayType::value_type& element );
		typename ArrayType::const_iterator Find( const typename ArrayType::value_type& element ) const;

		// Resorts the array. Call this if elements are modified directly
		void Resort();
	};

} }

#include "sortedArray.inl"

#endif