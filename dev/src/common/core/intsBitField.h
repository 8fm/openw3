#pragma once

////////////////////////////////////////////////////////////////////////////
// Array of 'compressed' integers. Usable for array of 2 or 4 byte integers.
template < Int32 BIT_SHIFT, EMemoryClass memoryClass = MC_Bitfield >
class TIntsBitField
{
	
public:
	static const Uint32 INT_SIZE											= 32;
	static const Uint32 INT_SHIFT											= 5;
	
	static const Uint32 BIT_SIZE											= 1 << BIT_SHIFT;
	static const Uint32 MAX_VALUE											= 1 << BIT_SIZE;
	static const Uint32 BIT_MASK											= (1 << BIT_SIZE) - 1;
	static const Uint32 SHIFT_INT_TO_INDEX									= (INT_SHIFT - BIT_SHIFT);
	static const Uint32 MASK_INT_TO_SHIFT									= (1 << SHIFT_INT_TO_INDEX) - 1;
	
	static_assert( (1U << INT_SHIFT) == INT_SIZE, "!" );


	RED_INLINE TIntsBitField() : m_size( 0 )								{}
	RED_INLINE TIntsBitField( Uint32 size )
		: m_size( size )													{ Resize( size ); }

	RED_INLINE void Resize( Uint32 size )									{ m_size = size; m_base.Resize( IndexCount() ); }
	RED_INLINE void Clear()													{ m_size = 0; m_base.ClearFast(); }
	RED_INLINE Uint32 Size() const											{ return m_size; }

	RED_INLINE void Set( Uint32 index, Uint32 val );
	RED_INLINE Uint32 Get( Uint32 index ) const;

	RED_INLINE void SetZero();
	RED_INLINE Uint32 IndexCount() const;

	RED_INLINE const void* DataPtr() const									{ return m_base.Data(); }
	RED_INLINE void* DataPtr()												{ return m_base.Data(); }
	RED_INLINE Uint32 DataSize() const										{ return IndexCount() * sizeof( Uint32 ); }

	void OnSerialize( IFile& file )
	{
		file<<m_size;
		file<<m_base;
	}
protected:
	Uint32										m_size;
	TDynArray< Uint32, memoryClass >			m_base;
};

////////////////////////////////////////////////////////////////////////////
// Typed integer bit field. Templated interface hides conversion from int.
// Usable eg. for storing enumerator values.
template < Int32 BIT_SHIFT, typename TIntType, EMemoryClass memoryClass = MC_Bitfield >
class TTypedIntsBitField : public TIntsBitField< BIT_SHIFT, memoryClass >
{
	typedef TIntsBitField< BIT_SHIFT, memoryClass > Super;
public:
	RED_INLINE  TTypedIntsBitField()
		: Super()															{}
	RED_INLINE  TTypedIntsBitField( Uint32 size )
		: Super( size )														{}

	RED_INLINE void Set( Uint32 index, TIntType val )						{ Super::Set( index, val ); }
	RED_INLINE TIntType Get( Uint32 index )	const						{ return TIntType( Super::Get( index ) ); }
};




template < Int32 BIT_SHIFT, EMemoryClass memoryClass >
RED_INLINE void TIntsBitField< BIT_SHIFT, memoryClass >::Set( Uint32 i, Uint32 val )
{
	ASSERT( val < MAX_VALUE );
	Uint32 index = i >> SHIFT_INT_TO_INDEX;
	Uint32 fieldVal = m_base[ index ];
	Uint32 bitShift = BIT_SIZE * (i & MASK_INT_TO_SHIFT);
	Uint32 bitMask = BIT_MASK << bitShift;;
	m_base[ index ] = (fieldVal & (~bitMask)) | (val << bitShift);
}
template < Int32 BIT_SHIFT, EMemoryClass memoryClass >
RED_INLINE Uint32 TIntsBitField< BIT_SHIFT, memoryClass >::Get( Uint32 i ) const
{
	Uint32 field = m_base[ i >> SHIFT_INT_TO_INDEX ];
	return ( field  >> (BIT_SIZE * (i & MASK_INT_TO_SHIFT) )) & BIT_MASK;
}
template < Int32 BIT_SHIFT, EMemoryClass memoryClass >
RED_INLINE void TIntsBitField< BIT_SHIFT, memoryClass >::SetZero()
{
	if ( m_size )
	{
		Red::System::MemorySet( m_base.Data(), 0, m_base.DataSize() );
	}
}
template < Int32 BIT_SHIFT, EMemoryClass memoryClass >
RED_INLINE Uint32 TIntsBitField< BIT_SHIFT, memoryClass >::IndexCount() const
{
	Uint32 indexCount = m_size >> SHIFT_INT_TO_INDEX;
	if ( m_size & MASK_INT_TO_SHIFT )
	{
		++indexCount;
	}
	return indexCount;
}