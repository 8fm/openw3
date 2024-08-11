/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef RED_CONTAINERS_BITFIELD_H
#define RED_CONTAINERS_BITFIELD_H

namespace Red { namespace Containers {

	// Variable-width Bitfield class
	// This class contains an array of bitmasks, with each mask containing one or more bits
	// Individual element width MUST be a power-of two, and maximum of ArrayType::value_type bits
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	class BitField
	{
	public:
		BitField();
		explicit BitField( Red::System::Uint32 indexCount );
		BitField( const BitField& other );
		BitField( BitField&& other );
		~BitField();

		// Assignment ops
		BitField& operator=( const BitField& other );
		BitField& operator=( BitField&& other );

		// Bit-field operations (these modify the entire bitfield)
		void Resize( Red::System::Uint32 fieldIndexCount );					// Resize to support 'fieldIndexCount' x BitMaskWidth elements
		void ResizeFast( Red::System::Uint32 fieldIndexCount );				// Resize to support 'fieldIndexCount' x BitMaskWidth elements (may not resize the internal buffer)
		void Clear();														// Remove all elements and free the buffer
		void ClearFast();													// Remove all elements (does not free the buffer)
		void SetAll( typename ArrayType::value_type value );				// Set all masks to value

		// Properties
		Red::System::Uint32 GetIndexCount() const;							// Returns the number of bitmasks set
		Red::System::Uint32 GetArrayElementCount() const;					// Returns the number of elements in the internal array
		Red::System::MemSize GetBufferSize() const;							// Returns the size of the buffer in bytes

		// Element manipulation
		void Set( Red::System::Uint32 index, typename ArrayType::value_type value );		// Set the mask at an index
		typename ArrayType::value_type Get( Red::System::Uint32 index );					// Return the value of a mask at an index

		// Bitwise ops
		BitField	operator~();										// NOT
		BitField	operator&( const BitField& other );					// AND
		BitField&	operator&=( const BitField& other );
		BitField	operator|( const BitField& other );					// OR
		BitField&	operator|=( const BitField& other );
		BitField	operator^( const BitField& other );					// XOR
		BitField&	operator^=( const BitField& other );

		// Comparators
		Red::System::Bool operator!=( const BitField& other );
		Red::System::Bool operator==( const BitField& other );

	private:
		Red::System::Uint32 GetRequiredArraySize( Red::System::Uint32 indexCount );

		ArrayType				m_array;			// An array used to represent the bitmask internally (these should be an unsigned integral type)
		Red::System::Uint32		m_indexCount;		// The number of actual indices (bitmasks) contained in this field

		// Ensure the Bitmask width is valid
		static_assert( BitMaskWidth <= sizeof( typename ArrayType::value_type ) * 8, "Bitmasks cannot be bigger than the array element" );		
		static_assert( BitMaskWidth > 0, " Bitmasks cannot be of 0 size" );													
		static_assert( (BitMaskWidth & ( BitMaskWidth - 1 ) ) == 0, "Bitmasks must be power-of-two bits" );					

		// Stuff used internally
		static const Red::System::Uint32			IndicesPerElement = ( sizeof( typename ArrayType::value_type ) * 8 ) / BitMaskWidth;
		static const typename ArrayType::value_type MaximumBitmaskValue = ( 1 << BitMaskWidth );
		static const typename ArrayType::value_type	BitsValueMask = MaximumBitmaskValue - 1;
	};

} }

#include "bitField.inl"

#endif