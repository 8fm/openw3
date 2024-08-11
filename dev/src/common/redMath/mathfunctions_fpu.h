
/************************/
/* Math constants		*/
/************************/
#pragma once
#ifndef _REDMATH_LIB_REDMATH_FUNCTIONS_FPU_H
#define _REDMATH_LIB_REDMATH_FUNCTIONS_FPU_H
#include <math.h>

namespace Red
{
	namespace Math
	{
		#ifndef M_PI
		#define M_PI	3.14159265358979323846264338327f
		#endif

		#define DEG2RAD( x ) ( ((x) / 180.0f) * M_PI )
		#define RAD2DEG( x ) ( ((x) / M_PI) * 180.0f )
		#define DEG2RAD_HALF( x ) ( (x) * M_PI / 360.0f )

		/************************************************************************/
		/* Math functions                                                       */
		/************************************************************************/
		RED_INLINE float MSqr( float f )	{	return f * f; }
		RED_INLINE float MSqrt( float f )	{	return ::sqrtf( f ); }
		RED_INLINE float MRsqrt( float f )	{	return 1.0f / ::sqrtf( f ); }
		RED_INLINE float MAbs( float f )	{	return ::fabsf( f ); }
		RED_INLINE float MCeil( float f )	{	return ::ceilf( f ); }
		RED_INLINE float MSin( float f )	{	return ::sinf( f ); }
		RED_INLINE float MCos( float f )	{	return ::cosf( f ); }
		RED_INLINE float MAsin( float f )	{	return ::asinf( f ); }
		RED_INLINE float MAcos( float f )	{	return ::acosf( f ); }
		RED_INLINE float MFloor( float f )	{	return ::floorf( f ); }
		RED_INLINE float MRound( float f )	{	return ::floorf( f + 0.5f ); }
		RED_INLINE double MRoundD( double d ){	return ::floor( d + 0.5 ); }
		RED_INLINE float MFract( float f )  {   return f - ( float )( int )f; }
		RED_INLINE float MTan( float f )	{	return ::tanf( f ); }
		RED_INLINE float MATan2( float y, float x )	{ return ::atan2f( y, x ); }
		RED_INLINE bool IsPow2( int i )	{	return ( i & ( i-1 ) ) == 0; }
		RED_INLINE float MPow( float a, float p ) { return ::powf( a, p ); }
		// safer version of acos - won't return invalid values if argument is out of range
		RED_INLINE float MAcos_safe( float f )
		{
			if( MAbs(f) >= 1.0f )
			{
				f = ( f>0 )	? 0 : M_PI;
				return f;
			}

			return MAcos( f );
		}

		// safer version asin - won't return invalid values if argument is out of range
		RED_INLINE float MAsin_safe( float f )
		{
			if( MAbs(f) >= 1.0f )
			{
				f = ( f>0 )	? 0.5f * M_PI : -0.5f * M_PI;
				return f;
			}

			return MAcos( f );
		}

		RED_INLINE float FloatSelect( float comparand, float valueGE, float valueLT )
		{
			return comparand >= 0.0f ? valueGE : valueLT;
		}

		RED_INLINE float MSign( float f )	
		{ 
			return FloatSelect( f, 1.f, -1.f ); 
		}


		RED_INLINE int RoundUpToPow2( int val )
		{
			int retVal = 1;
			while( retVal < val )
				retVal <<= 1;

			return retVal;
		}


		RED_INLINE int RoundDownToPow2( int val )
		{
			int retVal = 1;
			while( ( retVal << 1 ) <= val )
			{
				retVal <<= 1;
			}
			return retVal;
		}

		RED_INLINE int MLog2( int val )
		{
			static const unsigned char log_2[256] = {
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

		RED_INLINE float MLog10( float val )
		{
			return ( float )log10( val );
		}
	};
};

#endif // _REDMATH_LIB_REDMATH_FUNCTIONS_FPU_H