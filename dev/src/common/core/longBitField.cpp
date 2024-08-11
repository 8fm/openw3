/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "longBitField.h"
#include "object.h"

IMPLEMENT_RTTI_CLASS( LongBitField );

LongBitField::LongBitField() 
{
}

LongBitField::LongBitField( Uint32 numBits ) 
{ 
	Resize( numBits ); 
	Clear();
}

LongBitField::LongBitField( const LongBitField& other ) 
	: m_mem( other.m_mem )
{}

LongBitField::LongBitField( LongBitField&& other )
	: m_mem( std::move( other.m_mem ) )
{}

LongBitField::~LongBitField() 
{
}

void LongBitField::Swap( LongBitField & value )
{
	m_mem.SwapWith( value.m_mem );
}
