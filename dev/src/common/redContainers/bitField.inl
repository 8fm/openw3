/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "containersCommon.h"

namespace Red { namespace Containers {

	////////////////////////////////////////////////////////////////////////////
	// Default CTor
	//
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE BitField< ArrayType, BitMaskWidth >::BitField()
		: m_indexCount( 0 )
	{
	}

	////////////////////////////////////////////////////////////////////////////
	// CTor with index count
	//	Resizes the internal buffer to the correct size
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE BitField< ArrayType, BitMaskWidth >::BitField( Red::System::Uint32 indexCount )
	{
		m_array.Resize( GetRequiredArraySize( indexCount ) );
		m_indexCount = indexCount;
		SetAll( 0 );
	}

	////////////////////////////////////////////////////////////////////////////
	// Copy CTor
	//
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE BitField< ArrayType, BitMaskWidth >::BitField( const BitField< ArrayType, BitMaskWidth >& other )
		: m_array( other )
		, m_indexCount( other.m_indexCount )
	{
	}

	////////////////////////////////////////////////////////////////////////////
	// Move CTor
	//
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE BitField< ArrayType, BitMaskWidth >::BitField( BitField< ArrayType, BitMaskWidth >&& other )
		: m_array( Red::Containers::ElementMoveConstruct( other.m_array ) )
		, m_indexCount( other.m_indexCount )
	{
		other.m_indexCount = 0;
	}

	////////////////////////////////////////////////////////////////////////////
	// DTor
	//
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE BitField< ArrayType, BitMaskWidth >::~BitField()
	{
		m_indexCount = 0;
	}

	////////////////////////////////////////////////////////////////////////////
	// Copy assignment
	//
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE BitField< ArrayType, BitMaskWidth >& BitField< ArrayType, BitMaskWidth >::operator=( const BitField< ArrayType, BitMaskWidth >& other )
	{
		m_array = other.m_array;
		m_indexCount = other.m_indexCount;
	}

	////////////////////////////////////////////////////////////////////////////
	// Move assignment
	//
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE BitField< ArrayType, BitMaskWidth >& BitField< ArrayType, BitMaskWidth >::operator=( BitField< ArrayType, BitMaskWidth >&& other )
	{
		m_array = ElementMoveConstruct( other.m_array );
		m_indexCount = other.m_indexCount;
		other.m_indexCount = 0;
	}

	////////////////////////////////////////////////////////////////////////////
	// GetRequiredArraySize
	//	Return the array size required to store the specified number of masks
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE Red::System::Uint32 BitField< ArrayType, BitMaskWidth >::GetRequiredArraySize( Red::System::Uint32 indexCount )
	{
		Red::System::Uint32 roundUpMask = indexCount & ( IndicesPerElement - 1 );
		Red::System::Uint32 roundUp = roundUpMask ? indexCount + ( IndicesPerElement - roundUpMask ) : indexCount;

		return roundUp / IndicesPerElement;
	}

	////////////////////////////////////////////////////////////////////////////
	// Resize
	//	Resize to support 'fieldIndices' x BitMaskWidth elements
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE void BitField< ArrayType, BitMaskWidth >::Resize( Red::System::Uint32 fieldIndices )
	{
		m_array.Resize( GetRequiredArraySize( fieldIndices ) );
		m_indexCount = fieldIndices;
	}

	////////////////////////////////////////////////////////////////////////////
	// ResizeFast
	//	Resize to support 'fieldIndices' x BitMaskWidth elements (may not resize the internal buffer)
	template< class ArrayType, Red::System::Uint32 BitMaskWidth > 
	RED_INLINE void BitField< ArrayType, BitMaskWidth >::ResizeFast( Red::System::Uint32 fieldIndices )
	{
		m_array.ResizeFast( GetRequiredArraySize( fieldIndices ) );
		m_indexCount = fieldIndices;
	}

	////////////////////////////////////////////////////////////////////////////
	// Clear
	//	Remove all elements and free the buffer
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE void BitField< ArrayType, BitMaskWidth >::Clear()
	{
		m_array.Clear();
		m_indexCount = 0;
	}

	////////////////////////////////////////////////////////////////////////////
	// ClearFast
	//	Remove all elements (does not free the buffer)
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE void BitField< ArrayType, BitMaskWidth >::ClearFast()
	{
		m_array.ClearFast();
		m_indexCount = 0;
	}

	////////////////////////////////////////////////////////////////////////////
	// SetAll
	//	Set all masks to value
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE void BitField< ArrayType, BitMaskWidth >::SetAll( typename ArrayType::value_type value )
	{
		RED_ASSERT( value < MaximumBitmaskValue, TXT( "Value is too high!" ) );

		// Build a mask for a single array element that contains all internal masks with the correct value
		typename ArrayType::value_type maskValue = 0;
		const Red::System::Uint32 maskTotalSizeBits = ( sizeof( maskValue ) * 8 );
		Red::System::Uint32 maskShift = 0;
		while( maskShift < maskTotalSizeBits )
		{
			maskValue |= ( value & BitsValueMask ) << maskShift;
			maskShift += BitMaskWidth;
		}

		if( m_array.Size() > 0 )
		{
			Red::System::MemorySet( m_array.Data(), maskValue, m_array.DataSize() );
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// GetIndexCount
	//	Returns the number of bitmasks set
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE Red::System::Uint32 BitField< ArrayType, BitMaskWidth >::GetIndexCount() const
	{
		return m_indexCount;
	}

	////////////////////////////////////////////////////////////////////////////
	// GetBufferSize
	//	Returns the size of the buffer in bytes
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE Red::System::MemSize BitField< ArrayType, BitMaskWidth >::GetBufferSize() const
	{
		return m_array.Size() * sizeof( typename ArrayType::value_type );
	}

	////////////////////////////////////////////////////////////////////////////
	// GetArrayElementCount
	//	Return the size of the internal array
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE Red::System::Uint32 BitField< ArrayType, BitMaskWidth >::GetArrayElementCount() const
	{
		return m_array.Size(); 
	}

	////////////////////////////////////////////////////////////////////////////
	// Set
	//	Set the mask at an index
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE void BitField< ArrayType, BitMaskWidth >::Set( Red::System::Uint32 index, typename ArrayType::value_type value )
	{
		RED_ASSERT( value < MaximumBitmaskValue, TXT( "Value is too high!" ) );

		// TODO: Optimise this to shift / mask later
		const Red::System::Uint32 arrayIndex = index / IndicesPerElement;	
		const typename ArrayType::value_type bitShift = ( index & ( IndicesPerElement - 1 ) ) * BitMaskWidth;
		const typename ArrayType::value_type shiftedMask = BitsValueMask << bitShift;
		const typename ArrayType::value_type shiftedInput = value << bitShift;
		const typename ArrayType::value_type oldValue = m_array[ arrayIndex ];
		m_array[ arrayIndex ] = ( oldValue & ~shiftedMask ) | shiftedInput;
	}

	////////////////////////////////////////////////////////////////////////////
	// Get
	//	Return the value of a mask at an index
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE typename ArrayType::value_type BitField< ArrayType, BitMaskWidth >::Get( Red::System::Uint32 index )
	{
		// TODO: Optimise this to shift / mask later
		const Red::System::Uint32 arrayIndex = index / IndicesPerElement;	
		const typename ArrayType::value_type rawValue = m_array[ arrayIndex ];
		const typename ArrayType::value_type bitShift = ( index & ( IndicesPerElement - 1 ) ) * BitMaskWidth;
		const typename ArrayType::value_type shiftedMask = BitsValueMask << bitShift;
		return ( rawValue & shiftedMask ) >> bitShift;
	}

	////////////////////////////////////////////////////////////////////////////
	// Bitwise NOT
	//	Since the individual masks are always power of two, it is safe to NOT entire array elements
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE BitField< ArrayType, BitMaskWidth > BitField< ArrayType, BitMaskWidth >::operator~()
	{
		BitField< ArrayType, BitMaskWidth > result( m_indexCount );
		const Red::System::Uint32 arrayCount = GetArrayElementCount();
		for( Red::System::Uint32 i = 0; i < arrayCount; ++i )
		{
			result.m_array[i] = ~m_array[i];
		}
		return result;
	}

	////////////////////////////////////////////////////////////////////////////
	// Bitwise AND
	//	Since the individual masks are always power of two, it is safe to AND entire array elements
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE BitField< ArrayType, BitMaskWidth >	BitField< ArrayType, BitMaskWidth >::operator&( const BitField< ArrayType, BitMaskWidth >& other )
	{
		RED_ASSERT( m_indexCount == other.m_indexCount, TXT( "Cannot use & operator on fields with different sizes" ) );
		BitField< ArrayType, BitMaskWidth > result( m_indexCount );
		const Red::System::Uint32 arrayCount = other.GetArrayElementCount();
		for( Red::System::Uint32 i = 0; i < arrayCount; ++i )
		{
			result.m_array[i] = m_array[i] & other.m_array[i];
		}
		return result;
	}

	////////////////////////////////////////////////////////////////////////////
	// AND assignment
	//	Since the individual masks are always power of two, it is safe to AND entire array elements
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE BitField< ArrayType, BitMaskWidth >& BitField< ArrayType, BitMaskWidth >::operator&=( const BitField< ArrayType, BitMaskWidth >& other )
	{
		RED_ASSERT( m_indexCount == other.m_indexCount, TXT( "Cannot use & operator on fields with different sizes" ) );
		const Red::System::Uint32 arrayCount = other.GetArrayElementCount();
		for( Red::System::Uint32 i = 0; i < arrayCount; ++i )
		{
			m_array[i] &= other.m_array[i];
		}
		return *this;
	}

	////////////////////////////////////////////////////////////////////////////
	// Bitwise OR
	//	Since the individual masks are always power of two, it is safe to OR entire array elements
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE BitField< ArrayType, BitMaskWidth >	BitField< ArrayType, BitMaskWidth >::operator|( const BitField< ArrayType, BitMaskWidth >& other )
	{
		RED_ASSERT( m_indexCount == other.m_indexCount, TXT( "Cannot use | operator on fields with different sizes" ) );
		BitField< ArrayType, BitMaskWidth > result( m_indexCount );
		const Red::System::Uint32 arrayCount = other.GetArrayElementCount();
		for( Red::System::Uint32 i = 0; i < arrayCount; ++i )
		{
			result.m_array[i] = m_array[i] | other.m_array[i];
		}
		return result;
	}

	////////////////////////////////////////////////////////////////////////////
	// OR assignment
	//	Since the individual masks are always power of two, it is safe to OR entire array elements
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE BitField< ArrayType, BitMaskWidth >&	BitField< ArrayType, BitMaskWidth >::operator|=( const BitField< ArrayType, BitMaskWidth >& other )
	{
		RED_ASSERT( m_indexCount == other.m_indexCount, TXT( "Cannot use | operator on fields with different sizes" ) );
		const Red::System::Uint32 arrayCount = other.GetArrayElementCount();
		for( Red::System::Uint32 i = 0; i < arrayCount; ++i )
		{
			m_array[i] |= other.m_array[i];
		}
		return *this;
	}

	////////////////////////////////////////////////////////////////////////////
	// Bitwise XOR
	//	Since the individual masks are always power of two, it is safe to XOR entire array elements
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE BitField< ArrayType, BitMaskWidth >	BitField< ArrayType, BitMaskWidth >::operator^( const BitField< ArrayType, BitMaskWidth >& other )
	{
		RED_ASSERT( m_indexCount == other.m_indexCount, TXT( "Cannot use ^ operator on fields with different sizes" ) );
		BitField< ArrayType, BitMaskWidth > result( m_indexCount );
		const Red::System::Uint32 arrayCount = other.GetArrayElementCount();
		for( Red::System::Uint32 i = 0; i < arrayCount; ++i )
		{
			result.m_array[i] = m_array[i] ^ other.m_array[i];
		}
		return result;
	}

	////////////////////////////////////////////////////////////////////////////
	// XOR assignment
	//	Since the individual masks are always power of two, it is safe to XOR entire array elements
	template< class ArrayType, Red::System::Uint32 BitMaskWidth >
	RED_INLINE BitField< ArrayType, BitMaskWidth >& BitField< ArrayType, BitMaskWidth >::operator^=( const BitField< ArrayType, BitMaskWidth >& other )
	{
		RED_ASSERT( m_indexCount == other.m_indexCount, TXT( "Cannot use ^ operator on fields with different sizes" ) );
		const Red::System::Uint32 arrayCount = other.GetArrayElementCount();
		for( Red::System::Uint32 i = 0; i < arrayCount; ++i )
		{
			m_array[i] ^= other.m_array[i];
		}
		return *this;
	}

} }