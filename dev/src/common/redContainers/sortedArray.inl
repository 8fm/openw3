/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "algorithms.h"

namespace Red { namespace Containers {

	//////////////////////////////////////////////////////////////////
	// Default CTor
	//
	template< class ArrayType, class Comparator >
	SortedArray< ArrayType, Comparator >::SortedArray()
		: ArrayType()
	{

	}

	//////////////////////////////////////////////////////////////////
	// Copy Ctor
	//
	template< class ArrayType, class Comparator >
	SortedArray< ArrayType, Comparator >::SortedArray( const SortedArray< ArrayType, Comparator >& other )
		: ArrayType( static_cast< const ArrayType& >( other ) )
	{

	}

	//////////////////////////////////////////////////////////////////
	// Move CTor
	//
	template< class ArrayType, class Comparator >
	SortedArray< ArrayType, Comparator >::SortedArray( SortedArray< ArrayType, Comparator >&& other )
		: ArrayType( static_cast< ArrayType&& >( other ) )
	{

	}

	//////////////////////////////////////////////////////////////////
	// Dtor
	//
	template< class ArrayType, class Comparator >
	SortedArray< ArrayType, Comparator >::~SortedArray()
	{

	}

	//////////////////////////////////////////////////////////////////
	// Copy assignment
	//
	template< class ArrayType, class Comparator >
	SortedArray< ArrayType, Comparator >& SortedArray< ArrayType, Comparator >::operator=( const SortedArray< ArrayType, Comparator >& other )
	{
		ArrayType::operator=( static_cast< const ArrayType& >( other ) );
		return *this;
	}

	//////////////////////////////////////////////////////////////////
	// Move assignment
	//
	template< class ArrayType, class Comparator >
	SortedArray< ArrayType, Comparator >& SortedArray< ArrayType, Comparator >::operator=( const SortedArray< ArrayType, Comparator >&& other )
	{
		ArrayType::operator=( static_cast< ArrayType&& >( other ) );
		return *this;
	}

	//////////////////////////////////////////////////////////////////
	// Insert element
	//
	template< class ArrayType, class Comparator >
	typename ArrayType::iterator SortedArray< ArrayType, Comparator >::Insert( const typename ArrayType::value_type& element )
	{
		Comparator comparator;
		typename ArrayType::iterator it = Algorithms::LowerBound( ArrayType::Begin(), ArrayType::End(), element, comparator );
		Red::System::MemSize pos = (it - ArrayType::Begin());
		ArrayType::Insert( static_cast< Red::System::Uint32 >( pos ), element );
		return ArrayType::Begin() + pos;
	}

	//////////////////////////////////////////////////////////////////
	// Move-Insert element
	//
	template< class ArrayType, class Comparator >
	typename ArrayType::iterator SortedArray< ArrayType, Comparator >::Insert( typename ArrayType::value_type&& element )
	{
		Comparator comparator;
		typename ArrayType::iterator it = Algorithms::LowerBound( ArrayType::Begin(), ArrayType::End(), element, comparator );
		Red::System::MemSize pos = (it - ArrayType::Begin());
		ArrayType::Insert( static_cast< Red::System::Uint32 >( pos ), element );
		return ArrayType::Begin() + pos;
	}

	//////////////////////////////////////////////////////////////////
	// Find element
	//
	template< class ArrayType, class Comparator >
	typename ArrayType::iterator SortedArray< ArrayType, Comparator >::Find( const typename ArrayType::value_type& element )
	{
		Comparator comparator;
		typename ArrayType::iterator it = Algorithms::LowerBound( ArrayType::Begin(), ArrayType::End(), element, comparator );
		return ( it != ArrayType::End() && !( comparator( element, *it ) ) ) ? it : ArrayType::End();
	}

	//////////////////////////////////////////////////////////////////
	// Find Element
	//
	template< class ArrayType, class Comparator >
	typename ArrayType::const_iterator SortedArray< ArrayType, Comparator >::Find( const typename ArrayType::value_type& element ) const
	{
		Comparator comparator;
		typename ArrayType::iterator it = Algorithms::LowerBound( ArrayType::Begin(), ArrayType::End(), element, comparator );
		return ( it != ArrayType::End() && !( comparator( element, *it ) ) ) ? it : ArrayType::End();
	}

	//////////////////////////////////////////////////////////////////
	// Resort
	//
	template< class ArrayType, class Comparator >
	void SortedArray< ArrayType, Comparator >::Resort()
	{
		Sort( ArrayType::Begin(), ArrayType::End() );
	}

} }