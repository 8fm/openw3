/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef RED_CONTAINERS_ARRAY_SET_H
#define RED_CONTAINERS_ARRAY_SET_H

namespace Red { namespace Containers {

	// A element set implemented as a sorted array
	// Most implementation is just a wrapper for SortedArray
	template< class ArrayType, class Comparator = DefaultLessthanComparator< typename ArrayType::value_type > >
	class ArraySet
	{
	public:
		ArraySet();
		ArraySet( const ArraySet& other );
		ArraySet( ArraySet&& other );
		~ArraySet();
		ArraySet& operator=( const ArraySet& other );
		ArraySet& operator=( ArraySet&& other );

		// Iterator and type
		typedef typename SortedArray< ArrayType, Comparator >::iterator iterator;
		typedef typename SortedArray< ArrayType, Comparator >::const_iterator const_iterator;
		typedef typename SortedArray< ArrayType, Comparator >::value_type value_type;

		// Array properties
		Red::System::Uint32 Size() const							{ return m_array.Size(); }
		Red::System::Uint32 Capacity() const						{ return m_array.Capacity(); }
		Red::System::Uint32 DataSize() const						{ return m_array.DataSize(); }
		Red::System::Bool Empty() const								{ return m_array.Empty(); }

		// Buffer manipulation
		void Clear()												{ m_array.Clear(); }
		void ClearFast()											{ m_array.ClearFast(); }
		void Reserve( Red::System::Uint32 elementCount )			{ m_array.Reserve( elementCount ); }

		// Element manipulation
		Red::System::Bool Insert( const value_type& value );
		Red::System::Bool Remove( const value_type& value )			{ return m_array.Remove( value ); }
		void Erase( iterator it )									{ m_array.Erase( it ); }

		// Element searching
		iterator Find( const value_type& value );
		const_iterator Find( const value_type& value ) const;
		Red::System::Bool Exist( const value_type& value )			{ return m_array.Exist( value ); }

		// Iteration
		iterator Begin()											{ return m_array.Begin(); }
		iterator End()												{ return m_array.End(); }
		const_iterator Begin() const								{ return m_array.Begin(); }
		const_iterator End() const									{ return m_array.End(); }

		// Comparison
		Red::System::Bool operator==( const ArraySet& other )		{ return m_array == other.m_array; }
		Red::System::Bool operator!=( const ArraySet& other )		{ return m_array != other.m_array; }

	private:
		SortedArray< ArrayType, Comparator > m_array;
	};

} }

#include "arraySet.inl"

#endif