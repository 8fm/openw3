/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "mersenneTwister.h"

#include "../../redSystem/numericalLimits.h"

namespace Red { namespace Math {

namespace Random {

using namespace System;

MersenneTwister::MersenneTwister()
:	m_index( 0 )
{

}

MersenneTwister::~MersenneTwister()
{

}

void MersenneTwister::Seed()
{
	Seed( 0 );
}

void MersenneTwister::Seed( System::Random::SeedValue seed )
{
	m_seed[ 0 ] = seed;

	for ( Uint32 i = 1; i < SIZE; ++i )
	{
		m_seed[ i ] = 0x6c078965 * ( m_seed[ i - 1 ] ^ m_seed[ i - 1 ] >> 30 ) + i;
	}

	m_index = 0;
}

//////////////////////////////////////////////////////////////////////////
// Largest possible return value

//////////////////////////////////////////////////////////////////////////
// Unsigned Integers

template<>
Uint8 MersenneTwister::Max() const
{
	return NumericLimits< Uint8 >::Max();
}

template<>
Uint16 MersenneTwister::Max() const
{
	return NumericLimits< Uint16 >::Max();
}

template<>
Uint32 MersenneTwister::Max() const
{
	return NumericLimits< Uint32 >::Max();
}

template<>
Uint64 MersenneTwister::Max() const
{
	return NumericLimits< Uint64 >::Max();
}

//////////////////////////////////////////////////////////////////////////
// Signed Integers

template<>
Int8 MersenneTwister::Max() const
{
	return NumericLimits< Int8 >::Max();
}

template<>
Int16 MersenneTwister::Max() const
{
	return NumericLimits< Int16 >::Max();
}

template<>
Int32 MersenneTwister::Max() const
{
	return NumericLimits< Int32 >::Max();
}

template<>
Int64 MersenneTwister::Max() const
{
	return NumericLimits< Int64 >::Max();
}

template<>
Float MersenneTwister::Max() const
{
	return 1.0f;
}

//////////////////////////////////////////////////////////////////////////
// Unsigned Integers

template<>
Uint32 MersenneTwister::Get()
{
	static const Uint32 MATRIX[ 2 ] = { 0, 0x9908b0df };

	Uint32 nextIndex	= ( m_index + 1 ) % SIZE;
	Uint32 returnValue	= ( m_seed[ m_index ] & 0x80000000 ) | ( m_seed[ nextIndex ] & 0x7FFFFFFF );
	returnValue			= m_seed[ ( m_index + 397 ) % SIZE ] ^ ( returnValue >> 1 ) ^ MATRIX[ returnValue & 0x00000001 ];

	m_seed[ m_index ]	= returnValue;
	m_index = nextIndex;

	// Temper value
	returnValue ^= returnValue >> 11;
	returnValue ^= returnValue <<  7 & 0x9d2c5680;
	returnValue ^= returnValue << 15 & 0xefc60000;
	returnValue ^= returnValue >> 18;

	return returnValue;
}

template<>
Uint8 MersenneTwister::Get()
{
	return static_cast< Uint8 >( Get< Uint32 >() & Max< Uint8 >() );
}

template<>
Uint16 MersenneTwister::Get()
{
	return static_cast< Uint16 >( Get< Uint32 >() & Max< Uint16 >() );
}

template<>
Uint64 MersenneTwister::Get()
{
	return ( static_cast< Uint64 >( Get< Uint32 >() ) << 32 ) | Get< Uint32 >();
}

//////////////////////////////////////////////////////////////////////////
// Signed Integers

template<>
Int8 MersenneTwister::Get()
{
	return Get< Uint32 >() & Max< Int8 >();
}

template<>
Int16 MersenneTwister::Get()
{
	return Get< Uint32 >() & Max< Int16 >();
}

template<>
Int32 MersenneTwister::Get()
{
	return Get< Uint32 >() & Max< Int32 >();
}

template<>
Int64 MersenneTwister::Get()
{
	return Get< Uint64 >() & Max< Int64 >();
}

//////////////////////////////////////////////////////////////////////////
// Floating point

template<>
Float MersenneTwister::Get()
{
	return Get< Uint32 >() * ( 1.0f / Max< Uint32 >() );
}

} // namespace Random

} } // namespace Red / System
