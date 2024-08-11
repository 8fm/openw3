/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkMath.h
//
// Library of static functions for math computations.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AKMATH_H_
#define _AKMATH_H_

#include <math.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/AkSimd.h>

#if defined(AK_WIN) || defined(AK_XBOX360) || defined(AK_XBOXONE)
#include <float.h>
#endif

#ifdef AK_XBOX360
#include <ppcintrinsics.h>
#endif

#ifdef __PPU__
#include <ppu_intrinsics.h>
#endif

#define PI					(3.1415926535897932384626433832795f)
#define TWOPI				(6.283185307179586476925286766559f)
#define PIOVERTWO			(1.5707963267948966192313216916398f)
#define PIOVERFOUR			(0.78539816339744830961566084581988f)
#define PIOVEREIGHT			(0.3926990816987241548078304229095f)
#define THREEPIOVERFOUR		(2.3561944901923449288469825374596f)
#define ONEOVERPI			(0.31830988618379067153776752674503f)
#define ONEOVERTWOPI		(0.15915494309189533576888376337251f)
#define ROOTTWO				(1.4142135623730950488016887242097f)
#define LOGTWO				(0.30102999566398119521373889472449f)
#define ONEOVERLOGTWO		(3.3219280948873623478703194294894)
#define ONEOVERTHREESIXTY	(0.0027777777777777777777777777777778f)
#define PIOVERONEEIGHTY		(0.017453292519943295769236907684886f)
#define ONEEIGHTYOVERPI		(57.295779513082320876798154814105f)
#define ONE_OVER_SQRT_OF_TWO (0.70710678118654752440084436210485f)

#define	AK_EPSILON			(1.0e-38f)			// epsilon value for fast log(0)

#define AK_INT_MIN			(-2147483647 - 1)
#define AK_INT_MAX			(2147483647)

struct AkVector;
struct ConeParams;
struct RadiusParams;
struct AkSoundPosition;
struct AkSoundParams;

namespace AkMath
{
	// converts degrees to radians
	extern AkReal32 ToRadians(
		AkReal32	in_fDegrees
		);

	// converts radians to degrees
	extern AkReal32 ToDegrees(
		AkReal32	in_fRadians
		);

	// makes ||inout_Vector|| belong to [0,1]
	extern void Normalise(
		AkVector&	inout_Vector
		);

	// returns the "length"
	extern AkReal32 Magnitude(
		const AkVector&	in_vector
		);

	// returns Vector1.Vector2
	extern AkReal32 DotProduct(
		const AkVector&	Vector1,
		const AkVector&	Vector2
		);

	// returns Vector1 x Vector2
	extern AkVector CrossProduct(
		const AkVector& Vector1,
		const AkVector& Vector2
		);

	// returns |Position1 - Position2|
	extern AkReal32 Distance(
		const AkVector&	Position1,
		const AkVector&	Position2
		);

	// 
	extern AkReal32 Interpolate(
		AkReal32	in_fLowerX,
		AkReal32	in_fLowerY,
		AkReal32	in_fUpperX,
		AkReal32	in_fUpperY,
		AkReal32	in_fX
		);

	// Linearly interpolate, without checking for out-of-bounds condition
	AkForceInline AkReal32 InterpolateNoCheck(
		AkReal32 in_fLowerX,AkReal32 in_fLowerY,
		AkReal32 in_fUpperX,AkReal32 in_fUpperY,
		AkReal32 in_fX
		)
	{
		AkReal32 fA = ( in_fX - in_fLowerX ) / ( in_fUpperX - in_fLowerX );
		return in_fLowerY + ( fA * (in_fUpperY - in_fLowerY) );
	}
	// 
	extern AkReal32 NormalisedInterpolate(
		AkReal32	in_fLowerY,
		AkReal32	in_fUpperY,
		AkReal32	in_fX
		);	

	AkForceInline void RotateVector(
		const AkVector & in_vector,				// Vector to rotate.
		const AkReal32 * in_pRotationMatrix,	// Unitary rotation matrix (row vectors in row major order).
		AkVector &		 out_rRotatedVector		// Output vector.
		)
	{
		const AkReal32 * AK_RESTRICT pRotationMatrix = in_pRotationMatrix;
		const AkVector * AK_RESTRICT pVector = &in_vector;

		// The matrix is in row major order and thus we interpret the input
		// vector as a row vector which we premultiply with the matrix.
		// Mij where i is the row and j the column:
		// x' = x*m11 + y*m21 + z*m31
		AkReal32 X = pVector->X * pRotationMatrix[0] 
					+ pVector->Y * pRotationMatrix[3]
					+ pVector->Z * pRotationMatrix[6];

		// y' = x*m12 + y*m22 + z*m32
		AkReal32 Y = pVector->X * pRotationMatrix[1] 
					+ pVector->Y * pRotationMatrix[4]
					+ pVector->Z * pRotationMatrix[7];

		// z' = x*m13 + y*m23 + z*m33
		AkReal32 Z = pVector->X * pRotationMatrix[2] 
					+ pVector->Y * pRotationMatrix[5]
					+ pVector->Z * pRotationMatrix[8];

		out_rRotatedVector.X = X;
		out_rRotatedVector.Y = Y;
		out_rRotatedVector.Z = Z;
	}

	// Rotate vector in_vector by transpose of rotation matrix in_pRotationMatrix.
	AkForceInline void UnrotateVector(
		const AkVector & in_vector,				// Vector to rotate by the transpose of in_pRotationMatrix.
		const AkReal32 * in_pRotationMatrix,	// Unitary rotation matrix (row vectors in row major order).
		AkVector &		 out_rRotatedVector		// Output vector.
		)
	{
		const AkReal32 * AK_RESTRICT pRotationMatrix = in_pRotationMatrix;
		const AkVector * AK_RESTRICT pVector = &in_vector;

		// The matrix is in row major order and thus we interpret the input
		// vector as a row vector which we premultiply with the matrix.
		// Mij where i is the row and j the column:
		// x' = x*m11 + y*m21 + z*m31
		AkReal32 X = pVector->X * pRotationMatrix[0] 
					+ pVector->Y * pRotationMatrix[1]
					+ pVector->Z * pRotationMatrix[2];

		// y' = x*m12 + y*m22 + z*m32
		AkReal32 Y = pVector->X * pRotationMatrix[3] 
					+ pVector->Y * pRotationMatrix[4]
					+ pVector->Z * pRotationMatrix[5];

		// z' = x*m13 + y*m23 + z*m33
		AkReal32 Z = pVector->X * pRotationMatrix[6] 
					+ pVector->Y * pRotationMatrix[7]
					+ pVector->Z * pRotationMatrix[8];

		out_rRotatedVector.X = X;
		out_rRotatedVector.Y = Y;
		out_rRotatedVector.Z = Z;
	}

	// returns in_fValue ^ in_fPower
	AkForceInline AkReal32 Pow(
		AkReal32	in_fValue,
		AkReal32	in_fPower
		)
	{
		return powf(in_fValue,in_fPower);
	}

	// 
	AkForceInline AkReal32 Log(
		AkReal32			in_fValue
		)
	{
		return log(in_fValue);
	}

	// 
	AkForceInline AkReal32 Log10(
		AkReal32			in_fValue
		)
	{
		return log10(in_fValue);
	}

	// retuns |in_fNumber|
	AkForceInline AkReal32 Abs(
		AkReal32			in_fNumber
		)
	{
		return fabs(in_fNumber);
	}


	// 
	AkForceInline AkReal32 Sin(
		AkReal32			in_fAngle			// radians
		)
	{
		return sin(in_fAngle);
	}

	// This sin approximation has a maximum absolute error of 0.001
	// Valid only between -pi to pi
	AkForceInline AkReal32 FastSin(float x)
	{
		AKASSERT(x >= -PI && x <= PI);
		static const float B = 4/PI;
		static const float C = -4/(PI*PI);
		static const float P = 0.225f;

		float y = B * x + C * x * Abs(x);

		//  const float Q = 0.775;
		return P * (y * Abs(y) - y) + y;   // Q * y + P * y * abs(y)
	}

	// returns cos(in_fAngle)
	AkForceInline AkReal32 Cos(
		AkReal32			in_fAngle			// radians
		)
	{
		return cos(in_fAngle);
	}

	// This cos approximation has a maximum absolute error of 0.001
	// Valid only between -pi to pi
	AkForceInline AkReal32 FastCos(AkReal32 x)
	{
		x += PI/2;
		if(x > PI)   // Original x > pi/2
		{
			x -= 2 * PI;   // Wrap: cos(x) = cos(x - 2 pi)
		}

		return FastSin(x);
	}

	// returns acos(in_fAngle)
	AkForceInline AkReal32 ACos(
		AkReal32			in_fAngle
		)
	{
		AKASSERT((in_fAngle <= 1.0f) && (in_fAngle >= -1.0f));
		return acos(in_fAngle);
	}

	// This atan approximation has a maximum absolute error of 0.005 rad
	AkForceInline AkReal32 FastAtan2f( AkReal32 y, AkReal32 x )
	{
		if ( x != 0.0f )
		{
			float atan;
			float z = y/x;
			if ( Abs( z ) < 1.0f )
			{
				atan = z/(1.0f + 0.28f*z*z);
				if ( x < 0.0f )
				{
					if ( y < 0.0f ) 
						return atan - PI;
					return atan + PI;
				}
			}
			else
			{
				atan = PIOVERTWO - z/(z*z + 0.28f);
				if ( y < 0.0f ) 
					return atan - PI;
			}
			return atan;
		}

		if ( y > 0.0f ) return PIOVERTWO;
		if ( y == 0.0f ) return 0.0f;
		return -PIOVERTWO;
	}

	// returns a random float value between in_fMin and in_fMax
	extern AkReal32 RandRange(
		AkReal32			in_fMin,
		AkReal32			in_fMax
		);

#ifndef AKSIMD_FAST_EXP_IMPLEMENTED
	// Faster scalar implementation of 10^x.
	// Use vector intrinsics instead of this function if available.
	AkForceInline AkReal32 FastPow10(
		AkReal32 x
		)
	{
		// Below -37 we get denormals. Below -38, x * SCALE is in the range of BIAS, resulting in a valid int and a NAN.
		if ( x < -37 )
			return 0.f;

		// Fast pow(10,x):
		static const AkReal32 LOG2 = 0.30102999566398119521f;
		static const AkReal32 SCALE = (AkUInt32)(1 << 23)/LOG2;	// 2^23 / log10(2)
		static const AkReal32 BIAS = (AkUInt32)(1 << 23) * 127.f;	// 127 * 2^23
		
		union IntOrFloat
		{
			AkReal32 f;
			AkUInt32 u;
		};

		// Coarse eval: push the input into the exponent part of the IEEE float.
		IntOrFloat iof;
		iof.u = (AkUInt32)( x * SCALE + BIAS );

		// Read back into floating point, the input IS exponentiated: iof has an exponent which 
		// corresponds to 2^(floor(x)), and the fractional part of x spills into the mantissa
		// and can be viewed as a linear interpolation between neighboring integer exponents.
		
		// At this point, the maximum error is ~6%. We keep the exponent aside,
		// and evaluate the mantissa against a 2nd order polynomial fitting of 2^(x-1) in the range [1,2[.
		// The resulting error is ~0.2%, at least as good as typical approximations of the 
		// exponential function implemented with intrinsics.

		IntOrFloat iofexp;
		iofexp.u = iof.u & 0xFF800000;	// Exponent only.
		iof.u &= 0x007FFFFF;			// Mantissa only.
		iof.u += (127 << 23);
		
		AkReal32 fInterp = 0.653043474544611f + iof.f * (2.08057721186389e-2f + iof.f * 3.25189772597707e-1f);

		AkReal32 fExp = iofexp.f * fInterp;

		return fExp;
	}
#endif

	// Faster scalar implementation of 2^x.
	// Use vector intrinsics instead of this function if available.
	AkForceInline AkReal32 FastPow2(
		AkReal32 x
		)
	{
		// Below -37 we get denormals. Below -38, x * SCALE is in the range of BIAS, resulting in a valid int and a NAN.
		if ( x < -37 )
			return 0.f;

		// Fast pow(2,x):
		static const AkReal32 SCALE = (AkUInt32)(1 << 23);	// 2^23
		static const AkReal32 BIAS = (AkUInt32)(1 << 23) * 127.f;	// 127 * 2^23
			
		union IntOrFloat
		{
			AkReal32 f;
			AkUInt32 u;
		};

		// Coarse eval: push the input into the exponent part of the IEEE float.
		IntOrFloat iof;
		iof.u = (AkUInt32)( x * SCALE + BIAS );

		// Read back into floating point, the input IS exponentiated: iof has an exponent which 
		// corresponds to 2^(floor(x)), and the fractional part of x spills into the mantissa
		// and can be viewed as a linear interpolation between neighboring integer exponents.
			
		// At this point, the maximum error is ~6%. We keep the exponent aside,
		// and evaluate the mantissa against a 2nd order polynomial fitting of 2^(x-1) in the range [1,2[.
		// The resulting error is ~0.2%, at least as good as typical approximations of the 
		// exponential function implemented with intrinsics.

		IntOrFloat iofexp;
		iofexp.u = iof.u & 0xFF800000;	// Exponent only.
		iof.u &= 0x007FFFFF;			// Mantissa only.
		iof.u += (127 << 23);
			
		AkReal32 fInterp = 0.653043474544611f + iof.f * (2.08057721186389e-2f + iof.f * 3.25189772597707e-1f);

		AkReal32 fExp = iofexp.f * fInterp;

		return fExp;
	}

	AkForceInline AkReal32 FastExp(AkReal32 x)
	{
		if ( x < -37 )
			return 0.f;

		// Fast exp(x):
		static const AkReal32 SCALE = (AkUInt32)(1 << 23)/0.693147180559945f;	// 2^23 / log_of_2_in_target_base
		static const AkReal32 BIAS = (AkUInt32)(1 << 23) * 127.f;	// 127 * 2^23

		union IntOrFloat
		{
			AkReal32 f;
			AkUInt32 u;
		};

		// Coarse eval: push the input into the exponent part of the IEEE float.
		IntOrFloat iof;
		iof.u = (AkUInt32)( x * SCALE + BIAS );

		// Read back into floating point, the input IS exponentiated: iof has an exponent which 
		// corresponds to 2^(floor(x)), and the fractional part of x spills into the mantissa
		// and can be viewed as a linear interpolation between neighboring integer exponents.

		// At this point, the maximum error is ~6%. We keep the exponent aside,
		// and evaluate the mantissa against a 2nd order polynomial fitting of 2^(x-1) in the range [1,2[.
		// The resulting error is ~0.2%, at least as good as typical approximations of the 
		// exponential function implemented with intrinsics.

		IntOrFloat iofexp;
		iofexp.u = iof.u & 0xFF800000;	// Exponent only.
		iof.u &= 0x007FFFFF;			// Mantissa only.
		iof.u += (127 << 23);

		AkReal32 fInterp = 0.653043474544611f + iof.f * (2.08057721186389e-2f + iof.f * 3.25189772597707e-1f);

		AkReal32 fExp = iofexp.f * fInterp;

		return fExp;
	}

	// Convert decibels to linear scale.
	AkForceInline AkReal32 dBToLin(
		AkReal32 in_fdB 
		)
	{
#ifdef AKSIMD_FAST_EXP_IMPLEMENTED
		return AKSIMD_DB2LIN_SCALAR( in_fdB );
#else
		return FastPow10( in_fdB * 0.05f );
#endif		
	}
	
	// Fast log10(x).
	// NOTE: Scalar implementation returns a finite arbitrary value (~-38) for log(0), while
	// implementations using intrinsics return -INFINITE.
	AkForceInline AkReal32 FastLog10( AkReal32 fX )
	{
#ifdef AKSIMD_FAST_LOG_IMPLEMENTED
		return AKSIMD_LOG10_SCALAR( fX );
#else

		static const AkReal32 LOGN2 = 0.6931471805f;
		static const AkReal32 INVLOGN10 = 0.4342944819f;

		// log10(fX) = ln(fX) / ln(10)
		// Range contraction -> ln(fX) = exponent(fX) * ln(2) + ln(mantissa(fX))

		union 
		{
			AkReal32 f;
			AkUInt32 u;
		} IntOrFloat;

		// Extract float mantissa
		IntOrFloat.f = fX;
		IntOrFloat.u &= 0x007FFFFF;
		IntOrFloat.u += (127 << 23);
		AkReal32 fMantissa = IntOrFloat.f;

		// Extract float exponent
		IntOrFloat.f = fX;
		IntOrFloat.u <<= 1;
		IntOrFloat.u >>= 24;
		AkReal32 fExp = IntOrFloat.u - 127.f;

		//// 5th order Taylor series approximation of ln(mantissa(fX))
		//// Note: Worst case error: 0.0012512 dB
		//AkReal32 fZ = (fMantissa-1.f)/(fMantissa+1.f);
		//AkReal32 fZ2 = fZ*fZ;
		//AkReal32 fZ3 = fZ*fZ2;
		//AkReal32 fLnfXMantissa = 2.f*(fZ+0.3333333333f*fZ3+0.2f*fZ3*fZ2);

		// 3rd order Taylor series approximation of ln(mantissa(fX))
		// Note: Worst case error: 0.0155487 dB
		AkReal32 fZ = (fMantissa-1.f)/(fMantissa+1.f);
		AkReal32 fLnfXMantissa = 2.f * fZ * ( 1 + fZ * fZ * 0.3333333333f );

		// Reconstruct ln(fX)
		AkReal32 fLnfX = fExp * LOGN2 + fLnfXMantissa;

		// Compute log10(fX)
		return fLnfX * INVLOGN10;
#endif
	}

	// Convert level in linear scale to decibels.
	// NOTE: Scalar implementation returns a finite arbitrary value (~-764dB) for 0, while
	// implementations using intrinsics return -INFINITE dB.
	AkForceInline AkReal32 FastLinTodB( AkReal32 in_fLinValue )
	{
#ifdef AKSIMD_FAST_LOG_IMPLEMENTED
		return AKSIMD_LIN2DB_SCALAR( in_fLinValue );
#else
		return 20 * FastLog10( in_fLinValue );
#endif
	}

	// Scaling macros.
	//
	// Scaling from prescaled decibels.
	// Implementation of the dB scaling folded around zero. Ideally this should go 
	// away, as it is not very stable numerically and barely usable above 0 dB either.
	// It should be replaced by a "gain" scale (linear), converted in dBs the normal way.
	/// IMPORTANT: If you change this, update CAkMusicPBI::SetAutomationValue() accordingly.
	// Above ~100 exponentiated dBs, we pass 0 to log(). Need to avoid infinite, remain small enough
	// to avoid precision drift, and yet be large enough to honor stacks of nodes with db-scaled RTPC.
#define AK_SAFE_MINIMUM_VOLUME_LEVEL (-4096.f)
	AkForceInline AkReal32 ScalingFromLin_dB( AkReal32 in_fLin )	// exponentiated dBs
	{
#if defined(AK_WIN)
		// This is usually the fastest implementation on PC, and the clearest.
		if ( in_fLin < 0 )
		{
			if ( in_fLin > -1.f )
				return FastLinTodB( in_fLin + 1 );
			else
				return AK_SAFE_MINIMUM_VOLUME_LEVEL;
		}
		else
		{
			if ( in_fLin < 1.f )
				return -FastLinTodB( 1 - in_fLin );
			else
				return -AK_SAFE_MINIMUM_VOLUME_LEVEL;
		}
///#elif defined (AKSIMD_FAST_LOG_IMPLEMENTED)
		// For platforms that have log approximation intrinsics.
///		return AKSIMD_SCALINGFROMLINDB_SCALAR( in_fLin );
#else
		// Fastest measured scalar implementation on other platforms.
		AkReal32 fInvSign = (in_fLin < 0) ? 1.f : -1.f;
		in_fLin = AkClamp( in_fLin, -1.f, 1.f );
		AkReal32 fLin = fInvSign * in_fLin + 1;
#if defined (AKSIMD_FAST_LOG_IMPLEMENTED)
		if ( AK_EXPECT_FALSE( fLin == 0 ) )
			return fInvSign * AK_SAFE_MINIMUM_VOLUME_LEVEL;
#endif
		return fInvSign * FastLinTodB( fLin );
#endif
	}
	
	// Generic scaling from logarithmic values.
	AkForceInline AkReal32 ScalingFromLog( AkReal32 in_fLog )	// log value
	{
#ifdef AKSIMD_FAST_EXP_IMPLEMENTED
		return AKSIMD_POW10_SCALAR( in_fLog );
#else
		return FastPow10( in_fLog );
#endif
	}

	// Conversion functions for authoring tool: prescaling of curve points in order to
	// match post-scaling of curves evaluated at run-time.
#if defined(AK_WIN) && !defined(AK_OPTIMIZED)
	// Prescaling function for values in dBs. 
	// Inverse of run-time scaling ScalingFromLin_dB().
	// See note about ScalingFromLin_dB above.
	AkForceInline double Prescaling_ToLin_dB( double in_dblLevel )
	{
		if ( in_dblLevel < 0 )
			return pow( 10, in_dblLevel * 0.05 ) - 1;
		else
			return -pow( 10, -in_dblLevel * 0.05 ) + 1;	
	}
	// Prescaling function for generic values to be used in logarithmic scale. 
	// Inverse of run-time scaling ScalingFromLog().
	AkForceInline double Prescaling_ToLog( double in_dblFreq )
	{
		return log10( in_dblFreq );
	}
#endif


	AkForceInline bool IsValidFloatInput( AkReal32 in_fValue ) // returns non-zero if valid
	{
#if defined(AK_WIN) || defined(AK_XBOX360) || defined(AK_XBOXONE)
		return _finite( in_fValue ) != 0;
#elif defined(__SPU__) || ( defined(AK_IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED && __IPHONE_OS_VERSION_MIN_REQUIRED < 60000 )
		return true; // do not bring in this unecessary call into jobs
#else
		return isfinite( in_fValue );
#endif
	}

	// returns the minimum value
	AkForceInline AkReal32 Min(
		AkReal32			in_value1,
		AkReal32			in_value2
		)
	{
#ifdef AK_XBOX360
		return (AkReal32)fpmin(in_value1, in_value2);
#elif defined(__PPU__)
		// __fsels( a, c, b );
		// if(a >= 0) return c;
		// else return b;
		return __fsels( in_value1 - in_value2, in_value2, in_value1 );
#else
		return AkMin(in_value1, in_value2);
#endif
	}

	// returns the maximum value
	AkForceInline AkReal32 Max(
		AkReal32			in_value1,
		AkReal32			in_value2
		)
	{
#ifdef AK_XBOX360
		return (AkReal32)fpmax(in_value1, in_value2);
#elif defined(__PPU__)
		// __fsels( a, c, b );
		// if(a >= 0) return c;
		// else return b;
		return __fsels( in_value1 - in_value2, in_value1, in_value2 );
#else
		return AkMax(in_value1, in_value2);
#endif
	}
	
	// rounds a float to a signed 32-bit integer
	AkForceInline AkInt32 Round(
		AkReal32			in_value
		)
	{
		AkReal32 fFract = ( in_value > 0 ) ? 0.5f : -0.5f;
		return (AkInt32)( in_value + fFract );
	}

	// rounds a positive float to an unsigned 32-bit integer
	AkForceInline AkUInt32 RoundU(
		AkReal32			in_value
		)
	{
		AKASSERT( in_value >= 0 );
		return (AkUInt32)( in_value + 0.5f );
	}
	
	// rounds a double to a signed 32-bit integer
	AkForceInline AkInt32 Round64(
		AkReal64			in_value
		)
	{
		AkReal64 fFract = ( in_value > 0 ) ? 0.5 : -0.5;
		return (AkInt32)( in_value + fFract );
	}

#define MAT(p, row, col) *(p+col+row*3)
#define A(row,col) MAT(pA,row,col)
#define B(row,col) MAT(pB,row,col)
#define C(row,col) MAT(pC,row,col)
	AkForceInline void MatrixMul3by3(
		const AkReal32 * AK_RESTRICT pA,
		const AkReal32 * AK_RESTRICT pB,
		AkReal32 *pC)
	{	
		C(0,0) = A(0,0) * B(0,0) + A(0,1) * B(1,0) + A(0,2) * B(2,0);
		C(0,1) = A(0,0) * B(0,1) + A(0,1) * B(1,1) + A(0,2) * B(2,1);
		C(0,2) = A(0,0) * B(0,2) + A(0,1) * B(1,2) + A(0,2) * B(2,2);

		C(1,0) = A(1,0) * B(0,0) + A(1,1) * B(1,0) + A(1,2) * B(2,0);
		C(1,1) = A(1,0) * B(0,1) + A(1,1) * B(1,1) + A(1,2) * B(2,1);
		C(1,2) = A(1,0) * B(0,2) + A(1,1) * B(1,2) + A(1,2) * B(2,2);

		C(2,0) = A(2,0) * B(0,0) + A(2,1) * B(1,0) + A(2,2) * B(2,0);
		C(2,1) = A(2,0) * B(0,1) + A(2,1) * B(1,1) + A(2,2) * B(2,1);
		C(2,2) = A(2,0) * B(0,2) + A(2,1) * B(1,2) + A(2,2) * B(2,2);
	}

#undef MAT
#undef A
#undef B
#undef C
}

#if defined AK_WII
static inline AkReal32 AkSqrtEstimate( AkReal32 x )
{
	if(x > 0.0f)
	{
		AkReal32 guess = __frsqrte(x);   // returns an approximation to    
		AkReal32 hx = 0.5f * x;
		guess = guess * ( 1.5f - guess * guess * hx );  // now have 8 sig bits
		guess = guess * ( 1.5f - guess * guess * hx );  // now have 16 sig bits
		// Uncomment one line to gain accuracy
		//guess = guess * ( 1.5f - guess * guess * hx );  // now have 32 sig bits	// not working on floats, only on double
		//guess = guess * ( 1.5f - guess * guess * hx );  // now have > 53 sig bits // not working on floats, only on double
		return x * guess ;
	}
	return 0;
}
static inline AkReal32 AkInvSqrtEstimate( AkReal32 x )
{
	if(x > 0.0f)
	{
		AkReal32 guess = __frsqrte(x);   // returns an approximation to    
		AkReal32 hx = 0.5f * x;
		guess = guess * ( 1.5f - guess * guess * hx );  // now have 8 sig bits
		guess = guess * ( 1.5f - guess * guess * hx );  // now have 16 sig bits
		// Uncomment one line to gain accuracy
		//guess = guess * ( 1.5f - guess * guess * hx );  // now have 32 sig bits	// not working on floats, only on double
		//guess = guess * ( 1.5f - guess * guess * hx );  // now have > 53 sig bits // not working on floats, only on double
		return guess ;
	}
	return 0;
}
#elif defined AK_WIIU
static inline AkReal32 AkSqrtEstimate( AkReal32 x )
{
	if(x > 0.0f)
	{
		return __FRES(__FRSQRTE(x));
	}
	return 0;
}
static inline AkReal32 AkInvSqrtEstimate( AkReal32 x )
{
	if(x > 0.0f)
	{
		return __FRSQRTE(x);
	}
	return 0;
}
#else
static inline AkReal32 AkSqrtEstimate( AkReal32 x )
{
	return sqrtf( x );
}
static inline AkReal32 AkInvSqrtEstimate( AkReal32 x )
{
	return 1.0f/sqrtf( x );
}
#endif


#ifndef __SPU__

// Interface for private plugins.
// Note: They can use AkMath directly, but the AK namespace is used here for backward compatibility.
namespace AK
{
	// convert decibels [-96.3..0] to linear scale [0..1]
	AkForceInline AkReal32 dBToLin( AkReal32 in_fdB )
	{
		return AkMath::dBToLin( in_fdB );
	}

	/// Computes log10(x) faster.
	/// \note Avoid 0 input values.
	/// \return Logarithmic value
	AkForceInline AkReal32 FastLog10( AkReal32 fX )
	{
		return AkMath::FastLog10( fX );
	}
}

#endif	// __SPU__

#endif	//_AKMATH_H_
