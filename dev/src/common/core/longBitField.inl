//*******************************************************************************
//* Move Operator																*
//*******************************************************************************
LongBitField& LongBitField::operator = ( LongBitField&& other )
{
	LongBitField( std::move( other ) ).Swap( *this );
	return *this;
}

//*******************************************************************************
//* Copy Operator																*
//*******************************************************************************
LongBitField& LongBitField::operator = ( const LongBitField& other )
{
	LongBitField( other ).Swap( *this );
	return *this;
}

//*******************************************************************************
//* Equal To Operator															*
//*******************************************************************************
Bool LongBitField::operator == ( const LongBitField& other )
{
	ASSERT( Size() == other.Size() );
	for( Uint32 i = 0; i < m_mem.Size(); ++i )
	{
		if( m_mem[ i ] != other.m_mem[i])
		{
			return false;
		}
	}
	return true;
}

//*******************************************************************************
//* Not Equal To Operator														*
//*******************************************************************************
Bool LongBitField::operator != ( const LongBitField& other )
{
	ASSERT( Size() == other.Size() );
	for( Uint32 i = 0; i < m_mem.Size(); ++i )
	{
		if( m_mem[ i ] != other.m_mem[i])
		{
			return true;
		}
	}
	return false;
}	 

//*******************************************************************************
//* Bitwise OR Assignment Operator												*
//*******************************************************************************
LongBitField& LongBitField::operator |= ( const LongBitField& other )
{
	ASSERT( Size() == other.Size() );

	for ( Uint32 i = 0; i < m_mem.Size(); ++i )
	{
		m_mem[ i ] |= other.m_mem[ i ];
	}

	return *this;
}

//*******************************************************************************
//* Bitwise AND Assignment Operator												*
//*******************************************************************************
LongBitField& LongBitField::operator&= ( const LongBitField& other )
{
	ASSERT( Size() == other.Size() );

	for ( Uint32 i = 0; i < m_mem.Size(); ++i )
	{
		m_mem[ i ] &= other.m_mem[ i ];
	}

	return *this;
}

//*******************************************************************************
//* Bitwise XOR Assignment Operator												*
//*******************************************************************************
LongBitField& LongBitField::operator^= ( const LongBitField& other )
{
	ASSERT( Size() == other.Size() );

	for ( Uint32 i = 0; i < m_mem.Size(); ++i )
	{
		m_mem[ i ] ^= other.m_mem[ i ];
	}

	return *this;
}

//*******************************************************************************
//* Bitwise NOT Operator														*
//*******************************************************************************
LongBitField LongBitField::operator~()
{
	LongBitField result( this->Size() );
	for ( Uint32 i = 0; i < m_mem.Size(); ++i )
	{
		result.m_mem[ i ] = ~m_mem[ i ];
	}

	return result;
}

//*******************************************************************************
//* Bitwise AND Operator														*
//*******************************************************************************
LongBitField LongBitField::operator&( const LongBitField& other )
{
	LongBitField result( this->Size() );
	for ( Uint32 i = 0; i < m_mem.Size(); ++i )
	{
		result.m_mem[ i ] = m_mem[ i ] & other.m_mem[i];
	}

	return result;
}

//*******************************************************************************
//* Bitwise OR Operator															*
//*******************************************************************************
RED_FORCE_INLINE LongBitField LongBitField::operator|( const LongBitField& other )
{
	LongBitField result( this->Size() );
	for ( Uint32 i = 0; i < m_mem.Size(); ++i )
	{
		result.m_mem[ i ] = m_mem[ i ] | other.m_mem[i];
	}

	return result;
}

//*******************************************************************************
//* Bitwise XOR Operator														*
//*******************************************************************************
RED_FORCE_INLINE LongBitField LongBitField::operator^( const LongBitField& other )
{
	LongBitField result( this->Size() );
	for ( Uint32 i = 0; i < m_mem.Size(); ++i )
	{
		result.m_mem[ i ] = m_mem[ i ] ^ other.m_mem[i];
	}

	return result;
}

//*******************************************************************************
//* Sets the memory to 0														*
//*******************************************************************************
void LongBitField::Clear()
{
	if ( m_mem.Size() ) 
	{
		Red::System::MemorySet( m_mem.Data(), 0, m_mem.Size() * sizeof( TBaseType ) );
	}
}

//*******************************************************************************
//* Sets the memory to the maximum value of TBaseType.							*
//*******************************************************************************
void LongBitField::SetAllBits()
{
	if ( m_mem.Size() ) 
	{
		Red::System::MemorySet( m_mem.Data(), TBaseType(-1), m_mem.Size() * sizeof( TBaseType ) );
	}
}

//*******************************************************************************
//* Resizes the memory buffer to allow for the containment of the supplied 		*
//* number of bits																*
//*******************************************************************************
void LongBitField::Resize( Uint32 numBits )
{
	m_mem.Resize( ( numBits + ( sizeof( TBaseType ) * 8 ) - 1 ) / ( sizeof( TBaseType ) * 8 ) );
}

//*******************************************************************************
//* Resizes the memory buffer to allow for the containment of the supplied 		*
//* number of bits, using resize fast											*
//*******************************************************************************
void LongBitField::ResizeFast( Uint32 numBits )
{
	m_mem.ResizeFast( ( numBits + ( sizeof( TBaseType ) * 8 ) - 1 ) / ( sizeof( TBaseType ) * 8 ) );
}

//*******************************************************************************
//* Returns the bitsize of the LongBitField										*
//*******************************************************************************
Uint32 LongBitField::Size() const 
{ 
	return m_mem.Size() * sizeof( TBaseType ) * 8; 
}

//*******************************************************************************
//* Checks the bit at the index value to see if its set							*
//*******************************************************************************
Bool LongBitField::IsBitSet( Uint32 index ) const 
{
	const Uint32 arrayIndex = index / ( sizeof( TBaseType ) * 8 );
	return arrayIndex < m_mem.Size() ? ( ( m_mem[ arrayIndex ] & ( 1 << ( index - ( arrayIndex * sizeof( TBaseType ) * 8 ) ) ) ) != 0 ) : false; 
}

//*******************************************************************************
//* Set the bit at the index value to be 1 or 0									*
//*******************************************************************************
void LongBitField::SetBit( Uint32 index, Bool set ) 
{ 
	const Uint32 arrayIndex = index / ( sizeof( TBaseType ) * 8 );
	if ( arrayIndex < m_mem.Size() )
	{
		m_mem[ arrayIndex ] = set 
			? ( m_mem[ arrayIndex ] | ( 1 << ( index - ( arrayIndex * sizeof( TBaseType ) * 8 ) ) ) )
			: ( m_mem[ arrayIndex ] & ~( 1 << ( index - ( arrayIndex * sizeof( TBaseType ) * 8 ) ) ) );
	}
}

//*******************************************************************************
//* Clears the bits																*
//*******************************************************************************
void LongBitField::ClearBits( const LongBitField& other )
{
	ASSERT( Size() == other.Size() );

	for ( Uint32 i = 0; i < m_mem.Size(); ++i )
	{
		m_mem[ i ] &= ~( other.m_mem[ i ] );
	}
}