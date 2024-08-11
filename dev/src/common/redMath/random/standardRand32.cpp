/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "standardRand.h"
#include <cstdlib>

#define RED_STANDARD_RAND_MAX32 0x3fffffff

#if RAND_MAX == RED_STANDARD_RAND_MAX32

#include "../../redSystem/numericalLimits.h"

namespace Red { namespace Math {

namespace Random {

using namespace System;

//////////////////////////////////////////////////////////////////////////
// Largest possible return value

//////////////////////////////////////////////////////////////////////////
// Unsigned

template<>
Uint8 StandardRand::Max() const
{
	return NumericLimits< Uint8 >::Max();
}

template<>
Uint16 StandardRand::Max() const
{
	return NumericLimits< Uint16 >::Max();
}

template<>
Uint32 StandardRand::Max() const
{
	return RED_STANDARD_RAND_MAX32;
}

template<>
Uint64 StandardRand::Max() const
{
	return ( static_cast< Uint64 >( Max< Uint32 >() ) << 30 ) | Max< Uint32 >();
}

//////////////////////////////////////////////////////////////////////////
// Signed

template<>
Int8 StandardRand::Max() const
{
	return NumericLimits< Int8 >::Max();
}

template<>
Int16 StandardRand::Max() const
{
	return NumericLimits< Int16 >::Max();
}

template<>
Int32 StandardRand::Max() const
{
	return RED_STANDARD_RAND_MAX32;
}

template<>
Int64 StandardRand::Max() const
{
	return ( static_cast< Int64 >( Max< Int32 >() ) << 30 ) | Max< Int32 >();
}

//////////////////////////////////////////////////////////////////////////
// Unsigned Integers

template<>
Uint8 StandardRand::Get()
{
	return static_cast< Uint8 >( rand() & Max< Uint8 >() );
}

template<>
Uint16 StandardRand::Get()
{
	return static_cast< Uint16 >( rand() ) & Max< Uint16 >();
}

template<>
Uint32 StandardRand::Get()
{
	return static_cast< Uint32 >( rand() );
}

template<>
Uint64 StandardRand::Get()
{
	return ( static_cast< Uint64 >( Get< Uint32 >() ) << 30 ) | Get< Uint32 >();
}

//////////////////////////////////////////////////////////////////////////
// Signed Integers

template<>
Int8 StandardRand::Get()
{
	return static_cast< Int8 >( rand() & Max< Int8 >() );
}

template<>
Int16 StandardRand::Get()
{
	return static_cast< Int16 >( rand() ) & Max< Int16 >();
}

template<>
Int32 StandardRand::Get()
{
	return rand();
}

template<>
Int64 StandardRand::Get()
{
	return ( static_cast< Int64 >( Get< Int32 >() ) << 30 ) | Get< Int32 >();
}

} // namespace Random

} } // namespace Red / System

#endif // RAND_MAX == RED_STANDARD_RAND_MAX32
