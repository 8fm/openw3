/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef RED_CONTAINERS_HEAP_H
#define RED_CONTAINERS_HEAP_H

namespace Red { namespace Containers {

	// A Heap container stored in a dynamic array as a binary tree.
	// Root node in array[0]
	template< class ArrayType, class Comparator = DefaultLessthanComparator< typename ArrayType::value_type > >
	class Heap
	{
	public:
		Heap();
		Heap( const Heap& other );
		Heap( Heap&& other );
		~Heap();
		Heap& operator=( const Heap& other );
		Heap& operator=( Heap&& other );

		// Heap iterator
		typedef typename ArrayType::iterator iterator;
		typedef typename ArrayType::const_iterator const_iterator;
		typedef Comparator comparator;

		// Heap manipulation
		void Push( const typename ArrayType::value_type& element );		// Add an element (and rebuild the heap)
		typename ArrayType::value_type Pop();							// Pop an element with lowest index (and rebuild the heap)
		void UpdateElement( iterator it );								// Update an element that has changed (faster than full resort)

		// If any elements are changed using non-const Begin / End, then UpdateElement must be called with the iterator used
		typename ArrayType::iterator Begin();
		typename ArrayType::iterator End();
		typename ArrayType::const_iterator Begin() const;
		typename ArrayType::const_iterator End() const;
		typename ArrayType::value_type& operator[]( Red::System::Uint32 index );
		const typename ArrayType::value_type& operator[]( Red::System::Uint32 index ) const;

		// Resort. Call this if elements are modified directly in a way that would affect their order in the heap
		void Resort();

	private:
		ArrayType m_array;
	};

} }

#include "heap.inl"

#endif