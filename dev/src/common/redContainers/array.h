/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef RED_CONTAINER_ARRAY_H
#define RED_CONTAINER_ARRAY_H
#pragma once

// Needs to know about memory class type
#include "../redMemoryFramework/redMemoryFrameworkTypes.h"

// Default policies for array element manipulation
#include "arrayCopyPolicy.h"
#include "arrayInitPolicy.h"
#include "arrayComparisonPolicy.h"

// Requires typetraits for Init and Copy policies
#include "../redSystem/typetraits.h"

namespace Red { namespace Containers {

	// Array base class (used to aid in instances where element type is not known (RTTI, serialisation)
	template< class BufferPolicy >
	class ArrayBase
	{
	public:
		ArrayBase();
		~ArrayBase();

		// Size / Capacity
		Red::System::Uint32 Size() const;
		Red::System::Uint32 Capacity() const;

		// Raw data accessors
		void* Data();
		const void* Data() const;

		// Raw buffer manipulation
		void GrowBuffer( Red::System::Uint32 elementsToGrowBy, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass );
		void ShrinkBuffer( Red::System::Uint32 elementsToShrinkBy, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass );
		void Clear( Red::MemoryFramework::MemoryClass memClass );

	protected:
		BufferPolicy m_bufferPolicy;
	};

	// Array Implemented as a collection of policies
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	class Array : public ArrayBase< BufferPolicy >
	{
	public:
		Array();
		explicit Array( Red::System::Uint32 initialSize );
		Array( Array< ElementType, BufferPolicy, MemClass >&& other );
		Array( const Array< ElementType, BufferPolicy, MemClass >& other );
		template< class OtherBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass >
		Array( const Array< ElementType, OtherBufferPolicy, OtherMemClass >& other );		
		~Array();

		// Iterators
		typedef ElementType			value_type;
		typedef ElementType*		iterator;
		typedef const ElementType*	const_iterator;

		// Data accessors
		ElementType* TypedData();
		const ElementType* TypedData() const;
		ElementType& operator[]( Red::System::Uint32 index );
		const ElementType& operator[]( Red::System::Uint32 index ) const;
		ElementType& Back();
		const ElementType& Back() const;

		// Element data
		Red::System::MemSize SizeOfAllElements() const;
		Red::System::MemSize DataSize() const;
		Red::System::Bool Empty() const;

		// Direct buffer manipulation
		void Reserve( Red::System::Uint32 size );
		void Resize( Red::System::Uint32 size );
		void ResizeFast( Red::System::Uint32 size );
		void Clear( Red::System::Uint32 sizeToReserve );	// Clear and reserve
		void Clear();
		void ClearFast();
		void Rewind( Red::System::Uint32 size );
		Red::System::Uint32 Grow( Red::System::Uint32 elementsToGrowBy = 1 );
		void Shrink();		// Resize down so capacity = size where possible

		// Element manipulation
		void PushBack( const ElementType& element );
		void PushBack( ElementType&& element );
		template< class OtherBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass > void PushBack( const Array< ElementType, OtherBufferPolicy, OtherMemClass >& other );
		Red::System::Bool PushBackUnique( const ElementType& element );
		Red::System::Bool PushBackUnique( ElementType&& element );
		template< class OtherBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass > void PushBackUnique( const Array< ElementType, OtherBufferPolicy, OtherMemClass >& other );
		Red::System::Bool Insert( const Red::System::Uint32 position, const ElementType& element );
		Red::System::Bool Insert( const Red::System::Uint32 position, ElementType&& element );
		ElementType PopBack();
		ElementType PopBackFast();
		void Swap( Red::System::Uint32 element1Index, Red::System::Uint32 element2Index );
		void Swap( iterator element1, iterator element2 );
		Red::System::Bool Remove( const ElementType& theElement );
		Red::System::Bool RemoveFast( const ElementType& theElement );
		void RemoveAt( Red::System::Uint32 index );
		void RemoveAtFast( Red::System::Uint32 index );
		
		// Array copying
		Array< ElementType, BufferPolicy, MemClass >& CopyFast( const Array< ElementType, BufferPolicy, MemClass >& other );
		Array< ElementType, BufferPolicy, MemClass > SubArray( Red::System::Uint32 start, Red::System::Uint32 count ) const;

		// Pointer element manipulation
		void ClearPtr();
		void ClearPtrFast();
		void ClearPtrRev();
		const ElementType* FindPtr( const ElementType& element ) const;
		ElementType* FindPtr( const ElementType& element );
		void RemoveEmptyPointers();

		// Element searching
		Red::System::Uint32 GetIndex( const ElementType& theElement ) const;
		Red::System::Uint32 GetIndex( const_iterator theElement ) const;
		Red::System::Bool Exist( const ElementType& element ) const;

		// Iterator manipulation
		iterator Begin();
		const_iterator Begin() const;
		iterator End();
		const_iterator End() const;
		void Erase( iterator it );
		void Erase( iterator first, iterator last );
		void EraseFast( iterator it );

		// Comparison operators
		template< class OtherArrayBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass >
		Red::System::Bool operator==(const Array< ElementType, OtherArrayBufferPolicy, OtherMemClass >& other) const;
		template< class OtherArrayBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass >
		Red::System::Bool operator!=(const Array< ElementType, OtherArrayBufferPolicy, OtherMemClass >& other) const;
		template< class OtherArrayBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass >
		Red::System::Bool operator<(const Array< ElementType, OtherArrayBufferPolicy, OtherMemClass >& other) const;
		template< class OtherArrayBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass >
		Red::System::Bool operator>(const Array< ElementType, OtherArrayBufferPolicy, OtherMemClass >& other) const;
		template< class OtherArrayBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass >
		Red::System::Bool operator<=(const Array< ElementType, OtherArrayBufferPolicy, OtherMemClass >& other) const;
		template< class OtherArrayBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass >
		Red::System::Bool operator>=(const Array< ElementType, OtherArrayBufferPolicy, OtherMemClass >& other) const;

		// Assignment operators (assignment cannot be a template function)
		const Array< ElementType, BufferPolicy, MemClass >& operator=( const Array< ElementType, BufferPolicy, MemClass >& other );
		Array< ElementType, BufferPolicy, MemClass >& operator=( Array< ElementType, BufferPolicy, MemClass >&& other );

	private:

		// Element policies
		typedef TInitPolicy< ElementType, TPlainType< ElementType >::Value > TInit;
		typedef TCopyPolicy< ElementType, TCopyableType< ElementType >::Value > TCopy;
		typedef TComparePolicy< ElementType, TPlainType< ElementType >::Value > TCompare;

	protected:
		// Buffer policy is a dependant type, and must be declared here
		using ArrayBase< BufferPolicy >::m_bufferPolicy;
	};

} }

////////////////////////////////////////////////////////////////
// In-place new for arrays
//	Allocate new element memory at the end of the array
template <class ElementType, class ArrayBufferType, Red::MemoryFramework::MemoryClass MemClass >
void *operator new ( Red::System::MemSize size, Red::Containers::Array< ElementType, ArrayBufferType, MemClass >& ar )
{
	RED_UNUSED( size );
	Red::System::MemSize index = ar.Size();
	ar.GrowBuffer( 1, sizeof( ElementType ), MemClass );
	return &ar[ index ];
}

#include "array.inl"

#endif
