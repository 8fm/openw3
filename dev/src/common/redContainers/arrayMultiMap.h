/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_CONTAINER_ARRAY_MAP_H
#define RED_CONTAINER_ARRAY_MAP_H

#include "sortedArray.h"

namespace Red { namespace Containers {

	// This class represents a map built from an array of Pair<k,v> (as opposed to a set). Duplicates CAN exist
	// It must be created with ArrayType containing a Pair<k,v> as its element type
	// Note, that like SortedArray, using base Array functions can invalidate the internal element ordering, so be careful to call Resort after modifying elements
	template< class ArrayType, class Comparator = DefaultPairLessthanComparator< typename ArrayType::value_type > >
	class ArrayMultiMap : public SortedArray< ArrayType, Comparator >
	{
	public:
		ArrayMultiMap();
		ArrayMultiMap( ArrayMultiMap< ArrayType, Comparator >&& other );
		ArrayMultiMap( const ArrayMultiMap< ArrayType, Comparator >& other );
		~ArrayMultiMap();

		ArrayMultiMap< ArrayType, Comparator >& operator=( const ArrayMultiMap< ArrayType, Comparator >& other );
		ArrayMultiMap< ArrayType, Comparator >& operator=( const ArrayMultiMap< ArrayType, Comparator >&& other );

		// Key / value typedefs
		typedef typename ArrayType::value_type::first_type key_type;
		typedef typename ArrayType::value_type::second_type elem_type;

		// ArrayMultiMap interface
		typename ArrayType::iterator Insert(const key_type& cKey, const elem_type& cElem);
		typename ArrayType::iterator Find(const key_type& cKey);
		typename ArrayType::const_iterator Find(const key_type& cKey) const;
		Red::System::Bool Erase(const key_type& cKey);
		void Erase(typename ArrayType::iterator it);
	};
} }

#include "ArrayMultiMap.inl"

#endif