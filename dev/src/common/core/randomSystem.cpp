/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "randomSystem.h"

Uint32 CRandomSystem::GetRandomIntFromSeed( Uint32& seed )
{
	Uint32 hi, lo;
	lo = 16807 * (seed & 0xffff);
	hi = 16807 * (seed >> 16);
	lo += (hi & 0x7fff) << 16;
	lo += hi >> 15;
	seed = (lo & 0x7FFFFFFF) + (lo >> 31);
	return seed;
}

Float CRandomSystem::GetRandomFloatFromSeed( Uint32& seed )
{
	// TODO: Not a very well made but I need it fast. Should be rewritten.
	Uint32 i = GetRandomIntFromSeed( seed ) & 0xffff;
	Float f = Float( i ) / Float( 0xffff );
	return f;
}
