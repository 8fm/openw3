/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "standardRand.h"

#include <cstdlib>

#define RED_STANDARD_RAND_MAX16 0x7fff

namespace Red { namespace Math {

namespace Random {

using namespace System;

StandardRand::StandardRand()
{

}

StandardRand::~StandardRand()
{

}

void StandardRand::Seed()
{
	srand( 0 );
}

void StandardRand::Seed( System::Random::SeedValue seed )
{
	srand( seed );
}

//////////////////////////////////////////////////////////////////////////
// Largest possible return value

template<>
Float StandardRand::Max() const
{
	return 1.0f;
}

//////////////////////////////////////////////////////////////////////////
// Floating point

template<>
Float StandardRand::Get()
{
	return Get< Uint32 >() * ( 1.0f / Max< Uint32 >() );
}

} // namespace Random

} } // namespace Red / System
