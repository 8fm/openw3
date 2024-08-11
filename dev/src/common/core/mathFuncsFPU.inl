/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

/************************/
/* Math constants		*/
/************************/

#pragma once

#ifndef M_PI
#define M_PI	3.14159265358979323846264338327f
#endif

#define M_PI_TWO   6.283185307179f
#define M_PI_HALF  1.570796326794896619231f

#include <math.h>
#include "types.h"
#include "algorithms.h"

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 )
#	include <intrin.h> 
#elif defined( RED_PLATFORM_ORBIS )
#	include <x86intrin.h>
#endif


#define DEG2RAD( x ) ( ((x) / 180.0f) * M_PI )
#define RAD2DEG( x ) ( ((x) / M_PI) * 180.0f )
#define DEG2RAD_HALF( x ) ( (x) * M_PI / 360.0f )

/************************************************************************/
/* Math functions                                                       */
/************************************************************************/
RED_INLINE Float MSqrt(Float f)	{	return ::sqrtf(f); }
RED_INLINE Float MRsqrt(Float f)	{	return 1.0f/::sqrt(f); }
RED_INLINE Float MAbs(Float f)	{	return ::fabsf(f); }
RED_INLINE Float MCeil(Float f)	{	return ::ceil(f); }
RED_INLINE Float MSin(Float f)	{	return ::sin(f); }
RED_INLINE Float MCos(Float f)	{	return ::cos(f); }
RED_INLINE Float MAsin(Float f)	{	RED_ASSERT( Abs<Float>(f) <= 1.f );	return ::asin(f); }
RED_INLINE Float MAcos(Float f)	{	RED_ASSERT( Abs<Float>(f) <= 1.f ); return ::acos(f); }
RED_INLINE Float MFloor(Float f)	{	return ::floor(f); }
RED_INLINE Float MRound(Float f)	{	return ::floor(f + 0.5f); }
RED_INLINE Double MRoundD(Double d){	return ::floor(d + 0.5); }
RED_INLINE Float MFract(Float f)  {   return f - (float)(int)f; }
RED_INLINE Float MTan(Float f)	{	return ::tan(f); }
RED_INLINE Float MATan2(Float y, Float x)	{ return ::atan2(y, x); }
RED_INLINE Bool IsPow2(Int32 i)	{	return (i & ( i-1 )) == 0; }

RED_INLINE float FloatSelect( float comparand, float valueGE, float valueLT )
{
	return comparand >= 0.0f ? valueGE : valueLT;
}

// safer version of acos - won't return invalid values if argument is out of range
RED_INLINE Float MAcos_safe( Float f )
{
	if( MAbs(f) >= 1.0f )
	{
		f = ( f>0 )	? 0 : M_PI;
		return f;
	}

	return MAcos( f );
}

// safer version asin - won't return invalid values if argument is out of range
RED_INLINE Float MAsin_safe( Float f )
{
	if( MAbs(f) >= 1.0f )
	{
		f = ( f>0 )	? 0.5f * M_PI : -0.5f * M_PI;
		return f;
	}

	return MAsin( f );
}

RED_INLINE Float MSign(Float f)	{ return FloatSelect(f, 1.f, -1.f); }

template< typename T >
RED_INLINE T RoundUpToPow2(T val)
{
	T retVal = 1;
	while( retVal < val )
		retVal <<= 1;

	return retVal;
}

template< typename T >
RED_INLINE T RoundDownToPow2(T val)
{
	T retVal = 1;
	while( (retVal << 1) <= val )
		retVal <<= 1;

	return retVal;
}

RED_INLINE Int32 MLog2( Int32 val )
{
	static const Uint8 log_2[256] = {
		0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
	};
	int l = -1;
	while ( val >= 256 ) 
	{ 
		l += 8; 
		val >>= 8; 
	}
	return l + log_2[val];
}

RED_INLINE Float MLog10( Float val )
{
	return ( Float )log10( val );
}

RED_INLINE Uint64 MCountLeadingZeros( Uint64 val )
{
#ifndef _M_X64
	const Uint32 loWord = *( ( Uint32* ) &val ); 
	const Uint32 hiWord = *( ( ( Uint32* ) &val ) + 1 ); 
	return ( loWord == 0 ) ? __lzcnt( hiWord ) + 32 : __lzcnt( loWord );
#else
	return __lzcnt64( val );
#endif
}