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

#include "AkFloat16.h"
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkSIMD.h>
#include <AK/Tools/Common/AkAssert.h>

#define AK_FLOAT_SIGN			0x80000000
#define AK_FLOAT_EXPONENT		0x7F800000
#define AK_FLOAT_MANTISSA		0x007FFFFF
#define AK_FLOAT_ALLBUTSIGN		(AK_FLOAT_EXPONENT | AK_FLOAT_MANTISSA)
#define AK_HALF_SIGN			0x8000
#define AK_HALF_EXPONENT		0x7C00
#define AK_HALF_MANTISSA		0x03FF

namespace AkFloat16
{

// Note: Explicitely not IEEE compliant for optimization purposes
void AkReal32ToReal16( AkReal32 *in_pf32Src, AkReal16 *out_pf16Dst, AkUInt32 in_uNumFrames )
{
    AkReal16 * pf16Out = (AkReal16 *) out_pf16Dst;
    AkUInt32 * pf32In = (AkUInt32 *) in_pf32Src;
	AKASSERT( (void*)in_pf32Src != (void*)out_pf16Dst ); // Does not support in-place processing
	// This code is not rounding, thus precision is not optimal
	// Denormals are zero

	// NaNs and denormals mapped to positive zero
	// +-Inf mapped to half precision finite range extents
	
    
    while( in_uNumFrames-- ) 
	{
        AkUInt32 f32 = *pf32In++;
		AkUInt32 f32Sign = f32 & AK_FLOAT_SIGN;	
		AkUInt32 f32Exp = f32 & AK_FLOAT_EXPONENT; 
		AkUInt32 f32Mantissa = f32 & AK_FLOAT_MANTISSA;
		AkReal16 f16Result;
		if ( f32Exp == 0x0 )
		{
			// Zero or denormals cases
			// policy: No need to preserve sign for zero and denormals are zero
			f16Result = 0x0;
		}
		else if ( (f32Exp >> 23) == 0xFF )
		{
			// +- infinity and NaN cases
			// policy: cap +-inf to max range and make NaN = 0
			if ( f32Mantissa == 0x0 )
			{
				// +- Inf case
				AkUInt32 f16Sign = ((AkUInt32) f32Sign) >> 16;
				AkUInt32 f16Exp = (AkUInt32) (0x1E << 10);
				AkUInt32 f16Mantissa = ((AkUInt32) AK_HALF_MANTISSA);
				f16Result = (AkReal16)(f16Sign | f16Exp | f16Mantissa);	 
			}
			else
			{
				// NaN case
				f16Result = 0x0;
			}
		}
		else
		{
			// Normalized number case
			AkUInt32 f16Sign = ((AkUInt32) f32Sign) >> 16;
			// Need to handle underflow and overflow when converting to smaller range of half precision
			// policy: cap overflow/underflow to max/min hald precision finite range
			AkInt32 iHalfExpBiased = ((AkInt32)(f32Exp >> 23)) - 127 + 15;
            if( iHalfExpBiased >= 0x1F ) 
			{
				// Overflow case
				AkUInt32 f16Exp = (AkUInt32) (0x1E << 10);
				AkUInt32 f16Mantissa = ((AkUInt32) AK_HALF_MANTISSA);
				f16Result = (AkReal16)(f16Sign | f16Exp | f16Mantissa);	
			}
			else if( iHalfExpBiased <= 0 ) 
			{  
				// Underflow case
				// policy: avoid generating denormals (map to zero instead), no need to preserve sign
				f16Result = 0x0;
			}
			else
			{
				// In-range case
				AkInt32 f16Exp =  (iHalfExpBiased << 10);
				AkUInt32 f16Mantissa = (f32Mantissa >> 13);
				f16Result = (AkReal16)(f16Sign | f16Exp | f16Mantissa);	 
			}
		}
		*pf16Out++ = f16Result;
	}
}


// Note: Explicitely not IEEE compliant for optimization purposes
void AkReal16ToReal32( AkReal16 * in_pf16Src, AkReal32 * out_pf32Dst, AkUInt32 in_uNumFrames )
{
    AkReal16 * pf16In = (AkReal16 *) in_pf16Src;	
    AkUInt32 *pf32Out = (AkUInt32 *) out_pf32Dst;
	AKASSERT( (void*)in_pf16Src != (void*)out_pf32Dst ); // Does not support in-place processing
    
    while( in_uNumFrames-- ) 
	{
		AkReal16 f16Half = *pf16In++;
		AkReal16 f16HalfSign = f16Half & AK_HALF_SIGN;
		AkReal16 f16HalfExp = f16Half & AK_HALF_EXPONENT; 
		AkReal16 f16HalfMantissa = f16Half & AK_HALF_MANTISSA; 
		AkUInt32 f32Result;

		// TODO: Put most common case in first branch if not vectorized on certain platforms
		// TODO: Check that the 0 exponent case is really necessary
		if ( f16HalfExp == 0x0 )
		{
			// Zero (or denormals case)
			// Denormal bit patterns are never written when converting to half precision so they don't need to be handled.
			// policy: No need to preserve sign for zero and denormals are zero
			f32Result = 0x0;
		}
		// Note: When we convert to half precision we will never write infinity or NaN bit patterns 
		// so this case does not need to be handled by optimized version
		//else if ( (f16HalfExp >> 10) == 0x1F )
		//{
		//	// +- infinity and NaN cases
		//	// policy: cap +-inf to max range and make NaN = 0
		//	if ( f16HalfMantissa == 0x0 )
		//	{
		//		// +- Inf case
		//		AkUInt32 f32Sign = ((AkUInt32) f16HalfSign) << 16;
		//		AkUInt32 f32Exp = (AkUInt32) (0x1E << 23);
		//		AkUInt32 f32Mantissa = ((AkUInt32) AK_HALF_MANTISSA) << 13;
		//		f32Result = (f32Sign | f32Exp | f32Mantissa);	 
		//	}
		//	else
		//	{
		//		// NaN case
		//		f32Result = 0x0;
		//	}
		//}
		else
		{
			// Non-zero, normalized number case
			AkUInt32 f32Sign = ((AkUInt32) f16HalfSign) << 16;
			AkInt32 i32ExpExtended = ((AkInt32) (f16HalfExp >> 10)) - 15 + 127; 
			AkUInt32 f32Exp = (AkUInt32) (i32ExpExtended << 23);
			AkUInt32 f32Mantissa = ((AkUInt32) f16HalfMantissa) << 13;
			f32Result = (f32Sign | f32Exp | f32Mantissa);	
		}

		*pf32Out++ = f32Result;
    }
}

//// Note: Explicitely not IEEE compliant for optimization purposes
//void AkReal16ToReal32Vec( AkReal16 * in_pf16Src, AkReal32 * out_pf32Dst, AkUInt32 in_uNumFrames )
//{
//    AkReal16 * pf16In = (AkReal16 *) in_pf16Src;	
//    AkUInt32 *pf32Out = (AkUInt32 *) out_pf32Dst;
//	AKASSERT( (void*)in_pf16Src != (void*)out_pf32Dst ); // Does not support in-place processing
//	AKASSERT( in_uNumFrames % 8 == 0 );
//	static const AkInt32 i32AK_HALF_SIGN = AK_HALF_SIGN;
//	static const AkInt32Vector v8HalfSignMask = AKSIMD_LOAD1_V4F32( i32AK_HALF_SIGN );
//	static const AkInt32 i32AK_HALF_EXPONENT = AK_HALF_EXPONENT;
//	static const AkInt32Vector v8HalfExpMask = AKSIMD_LOAD1_V4F32( i32AK_HALF_EXPONENT );
//	static const AkInt32 i32AK_HALF_MANTISSA = AK_HALF_MANTISSA;
//	static const AkInt32Vector v8HalfMantissaMask = AKSIMD_LOAD1_V4F32( i32AK_HALF_MANTISSA );
//    
//    while( in_uNumFrames-- ) 
//	{
//		// AkReal16 f16Half = *pf16In++; 
//		AkUInt16Vector v8Half = AKSIMD_LOAD_V4F32( pf16In ); // Load 8 half
//		pf16In += 8;
//		// AkReal16 f16HalfSign = f16Half & AK_HALF_SIGN;
//		AkUInt16Vector v8HalfSign = AKSIMD_AND( v8Half, v8HalfSignMask );
//		//AkReal16 f16HalfExp = f16Half & AK_HALF_EXPONENT; 
//		AkUInt16Vector v8HalfSign = AKSIMD_AND( v8Half, v8HalfExpMask );
//		//AkReal16 f16HalfMantissa = f16Half & AK_HALF_MANTISSA; 
//		AkUInt16Vector v8HalfSign = AKSIMD_AND( v8Half, v8HalfMantissaMask );
//		AkUInt32 f32Result;
//
//		// AKSIMD_CMPEQ vcmpequw, _mm_cmpeq_ps, spu_cmpeq, vec_cmpeq
//		// AKSIMD_VSEL vec_sel(a,b,c) _mm_or_si128( _mm_and_si128(c,b), _mm_andnot_si128(c,a)) 
//		// vsel, vec_sel, spu_sel
//		if ( f16HalfExp == 0x0 )
//		{
//			// Zero (or denormals case)
//			// Denormal bit patterns are never written when converting to half precision so they don't need to be handled.
//			// policy: No need to preserve sign for zero and denormals are zero
//			f32Result = 0x0;
//		}
//		// Note: When we convert to half precision we will never write infinity or NaN bit patterns 
//		// so this case does not need to be handled by optimized version
//		//else if ( (f16HalfExp >> 10) == 0x1F )
//		//{
//		//	// +- infinity and NaN cases
//		//	// policy: cap +-inf to max range and make NaN = 0
//		//	if ( f16HalfMantissa == 0x0 )
//		//	{
//		//		// +- Inf case
//		//		AkUInt32 f32Sign = ((AkUInt32) f16HalfSign) << 16;
//		//		AkUInt32 f32Exp = (AkUInt32) (0x1E << 23);
//		//		AkUInt32 f32Mantissa = ((AkUInt32) AK_HALF_MANTISSA) << 13;
//		//		f32Result = (f32Sign | f32Exp | f32Mantissa);	 
//		//	}
//		//	else
//		//	{
//		//		// NaN case
//		//		f32Result = 0x0;
//		//	}
//		//}
//		else
//		{
//			// AKSIMD_SLI _mm_slli_epi32 (SSE2), vcmpbfp, spu_sl, vec_sl
//			// Non-zero, normalized number case
//			AkUInt32 f32Sign = ((AkUInt32) f16HalfSign) << 16;
//			// AKSIMD_SRAI, AKSIMD_I32SUB, AKSIMD_I32ADD
//			// _mm_srai_epi32, _mm_sub_epi32, _mm_add_epi32
//			// vsraw, vsubsws, vaddsws 
//			// vec_sra, vec_sub, spu_sub
//			// spu_rlmaska, spu_sub, spu_add
//			AkInt32 i32ExpExtended = ((AkInt32) (f16HalfExp >> 10)) - 15 + 127; 
//			AkUInt32 f32Exp = (AkUInt32) (i32ExpExtended << 23);
//			AkUInt32 f32Mantissa = ((AkUInt32) f16HalfMantissa) << 13;
//			// _mm_or_ps, vor, spu_or, vec_or
//			f32Result = (f32Sign | f32Exp | f32Mantissa);	
//		}
//
//		*pf32Out++ = f32Result;
//    }
//}

#ifdef AKFLOAT16_UNITTEST

#include <float.h>
#include <math.h>

void UnitTestConversion( )
{
	// In-range positive exponents
	AkReal32 f32InRangePosExp[] = { 1.f, -2.f, 100.f, -200.f, 1000.f, -2000.f, 65504.f, -65504.f };
	AkReal16 f16InRangePosExpRes[8];
	AkReal32 f32InRangePosExpRes[8];
	AkReal32ToReal16( f32InRangePosExp, f16InRangePosExpRes, 8 );
	AkReal16ToReal32( f16InRangePosExpRes, f32InRangePosExpRes, 8 );

	// In-range negative exponents
	AkReal32 fInRangeNegExp[] = { 1.e-1f, -2.e-1f, 1.3333e-2f, -2.6666e-2f, 1.9876e-4f, -2.1234e-4f, 6.10352e-5f, -6.10352e-5f };
	AkReal16 f16InRangeNegExpRes[8];
	AkReal32 f32InRangeNegExpRes[8];
	AkReal32ToReal16( fInRangeNegExp, f16InRangeNegExpRes, 8 );
	AkReal16ToReal32( f16InRangeNegExpRes, f32InRangeNegExpRes, 8 );

	// Out of range 
	AkReal32 f32OutRange[] = { 65505.f, -65505.f, 100000.f, -100001.f, 5e-8f, -5e-8f, 7e-12f, -7e-12f };
	AkReal16 f16OutRangeRes[8];
	AkReal32 f32OutRangeRes[8];
	AkReal32ToReal16( f32OutRange, f16OutRangeRes, 8 );
	AkReal16ToReal32( f16OutRangeRes, f32OutRangeRes, 8 );

	// Edge cases
	AkUInt32 NANPattern = 0x7FFFFFFF;
	AkReal32 NAN = *((AkReal32*)&NANPattern);
	AkReal32 fZero = 0.f; AkReal32 fOne = 1.f; AkReal32 fMinusOne = -1.f; 
	AkReal32 POS_INF = fOne/fZero;
	AkReal32 NEG_INF = fMinusOne/fZero;
	AkReal32 f32EdgeCases[] = { 0.f, -0.f, FLT_MAX, FLT_MIN, NAN, POS_INF, NEG_INF };
	AkReal16 f16EdgeCasesRes[7];
	AkReal32 f32EdgeCasesRes[7];
	AkReal32ToReal16( f32EdgeCases, f16EdgeCasesRes, 7 );
	AkReal16ToReal32( f16EdgeCasesRes, f32EdgeCasesRes, 7 );
}

#endif

} // namespace
