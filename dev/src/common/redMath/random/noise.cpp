/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "noise.h"

namespace Red { namespace Math {

namespace Random {

Noise::Noise()
:	m_seed( 0 )
{

}

Noise::~Noise()
{

}

void Noise::Seed()
{
	m_seed = 0;
}

void Noise::Seed( System::Random::SeedValue seed )
{
	m_seed = seed;
}

template<>
System::Float Noise::Max() const
{
	return 1.0f;
}

template<>
System::Float Noise::Get() const
{
	System::Int32 seed = ( m_seed << 13 ) ^ m_seed;
	return 0.5f * ( ( ( seed * ( seed * seed * 15731 + 789221 ) + 1376312589 ) & 0x7fffffff ) / 1073741824.0f );
}

template<>
System::Uint32 Noise::Max() const
{
	return 0x7fffffff;
}

template<>
System::Uint32 Noise::Get() const
{
	System::Int32 seed = ( m_seed << 13 ) ^ m_seed;
	return ( ( seed * ( seed * seed * 15731 + 789221 ) + 1376312589 ) & 0x7fffffff );
}

} // namespace Red { namespace Math {

} } // namespace Random {
