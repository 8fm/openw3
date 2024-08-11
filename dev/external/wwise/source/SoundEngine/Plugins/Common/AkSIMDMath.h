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

#ifndef _AK_SIMDMATH_H_
#define _AK_SIMDMATH_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkSimd.h>
#include "../../AkAudiolib/Common/AkMath.h"

#if defined(AK_XBOX360)

#include <xnamath.h>

#define AKSIMD_WRAPANGLES XMVectorModAngles
#define AKSIMD_SQRT XMVectorSqrt
#define AKSIMD_ATAN2 XMVectorATan2

#define AKSIMD_SINCOS_SETUP()								\
	XMVECTOR AKSIMD_SinCos_S1, AKSIMD_SinCos_S2, AKSIMD_SinCos_S3, AKSIMD_SinCos_S4, AKSIMD_SinCos_S5, AKSIMD_SinCos_S6, AKSIMD_SinCos_S7, AKSIMD_SinCos_S8, AKSIMD_SinCos_S9, AKSIMD_SinCos_S10, AKSIMD_SinCos_S11;	\
    XMVECTOR AKSIMD_SinCos_C1, AKSIMD_SinCos_C2, AKSIMD_SinCos_C3, AKSIMD_SinCos_C4, AKSIMD_SinCos_C5, AKSIMD_SinCos_C6, AKSIMD_SinCos_C7, AKSIMD_SinCos_C8, AKSIMD_SinCos_C9, AKSIMD_SinCos_C10, AKSIMD_SinCos_C11;	\
	AKSIMD_SinCos_C1 = __vspltw(g_XMCosCoefficients0.v, 1);\
    AKSIMD_SinCos_S1 = __vspltw(g_XMSinCoefficients0.v, 1);\
    AKSIMD_SinCos_C2 = __vspltw(g_XMCosCoefficients0.v, 2);\
    AKSIMD_SinCos_S2 = __vspltw(g_XMSinCoefficients0.v, 2);\
    AKSIMD_SinCos_C3 = __vspltw(g_XMCosCoefficients0.v, 3);\
    AKSIMD_SinCos_S3 = __vspltw(g_XMSinCoefficients0.v, 3);\
    AKSIMD_SinCos_C4 = __vspltw(g_XMCosCoefficients1.v, 0);\
    AKSIMD_SinCos_S4 = __vspltw(g_XMSinCoefficients1.v, 0);\
	AKSIMD_SinCos_C5 = __vspltw(g_XMCosCoefficients1.v, 1);\
    AKSIMD_SinCos_S5 = __vspltw(g_XMSinCoefficients1.v, 1);\
    AKSIMD_SinCos_C6 = __vspltw(g_XMCosCoefficients1.v, 2);\
    AKSIMD_SinCos_S6 = __vspltw(g_XMSinCoefficients1.v, 2);\
    AKSIMD_SinCos_C7 = __vspltw(g_XMCosCoefficients1.v, 3);\
    AKSIMD_SinCos_S7 = __vspltw(g_XMSinCoefficients1.v, 3);\
    AKSIMD_SinCos_C8 = __vspltw(g_XMCosCoefficients2.v, 0);\
    AKSIMD_SinCos_S8 = __vspltw(g_XMSinCoefficients2.v, 0);\
	AKSIMD_SinCos_C9 = __vspltw(g_XMCosCoefficients2.v, 1);\
    AKSIMD_SinCos_S9 = __vspltw(g_XMSinCoefficients2.v, 1);\
    AKSIMD_SinCos_C10 = __vspltw(g_XMCosCoefficients2.v, 2);\
    AKSIMD_SinCos_S10 = __vspltw(g_XMSinCoefficients2.v, 2);\
    AKSIMD_SinCos_C11 = __vspltw(g_XMCosCoefficients2.v, 3);\
    AKSIMD_SinCos_S11 = __vspltw(g_XMSinCoefficients2.v, 3);

// TODO: This could be unified with other platfoms (preserving XNA-style constant loading). 
#define AKSIMD_SINCOS( vSin, vCos, V )\
	{\
		XMVECTOR V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13;\
		XMVECTOR V14, V15, V16, V17, V18, V19, V20, V21, V22, V23;\
		XMDUMMY_INITIALIZE_VECTOR(vCos);\
		V1 = AKSIMD_WRAPANGLES( V );\
		vCos = __vupkd3d(vCos, VPACK_NORMSHORT2);\
		V2 = __vmulfp(V1, V1);\
		vCos = __vspltw(vCos, 3);\
		V3 = __vmulfp(V2, V1);\
		V4 = __vmulfp(V2, V2);\
		vCos = __vmaddfp(AKSIMD_SinCos_C1, V2, vCos); \
		V5 = __vmulfp(V3, V2);\
		vSin = __vmaddfp(AKSIMD_SinCos_S1, V3, V1);\
		V6 = __vmulfp(V3, V3);\
		V7 = __vmulfp(V4, V3);\
		vCos = __vmaddfp(AKSIMD_SinCos_C2, V4, vCos);   \
		vSin = __vmaddfp(AKSIMD_SinCos_S2, V5, vSin);\
		V8 = __vmulfp(V4, V4);\
		V9 = __vmulfp(V5, V4);\
		vCos = __vmaddfp(AKSIMD_SinCos_C3, V6, vCos);\
		vSin = __vmaddfp(AKSIMD_SinCos_S3, V7, vSin);\
		V10 = __vmulfp(V5, V5);\
		V11 = __vmulfp(V6, V5);\
		vCos = __vmaddfp(AKSIMD_SinCos_C4, V8, vCos);\
		vSin = __vmaddfp(AKSIMD_SinCos_S4, V9, vSin);\
		V12 = __vmulfp(V6, V6);\
		V13 = __vmulfp(V7, V6);\
		vCos = __vmaddfp(AKSIMD_SinCos_C5, V10, vCos);\
		vSin = __vmaddfp(AKSIMD_SinCos_S5, V11, vSin);\
		V14 = __vmulfp(V7, V7);\
		V15 = __vmulfp(V8, V7);\
		vCos = __vmaddfp(AKSIMD_SinCos_C6, V12, vCos);\
		vSin = __vmaddfp(AKSIMD_SinCos_S6, V13, vSin);\
		V16 = __vmulfp(V8, V8);\
		V17 = __vmulfp(V9, V8);\
		vCos = __vmaddfp(AKSIMD_SinCos_C7, V14, vCos);\
		vSin = __vmaddfp(AKSIMD_SinCos_S7, V15, vSin);\
		V18 = __vmulfp(V9, V9);\
		V19 = __vmulfp(V10, V9);\
		vCos = __vmaddfp(AKSIMD_SinCos_C8, V16, vCos);\
		vSin = __vmaddfp(AKSIMD_SinCos_S8, V17, vSin);\
		V20 = __vmulfp(V10, V10);\
		V21 = __vmulfp(V11, V10);\
		vCos = __vmaddfp(AKSIMD_SinCos_C9, V18, vCos);\
		vSin = __vmaddfp(AKSIMD_SinCos_S9, V19, vSin);\
		V22 = __vmulfp(V11, V11);\
		V23 = __vmulfp(V12, V11);\
		vCos = __vmaddfp(AKSIMD_SinCos_C10, V20, vCos);\
		vSin = __vmaddfp(AKSIMD_SinCos_S10, V21, vSin);\
		vCos = __vmaddfp(AKSIMD_SinCos_C11, V22, vCos);\
		vSin = __vmaddfp(AKSIMD_SinCos_S11, V23, vSin);\
		vCos = vCos;\
		vSin = vSin;\
	}

#elif defined(__SPU__) // Only tested on this platform but work can be easily extended to other platforms (AKSIMD_V4F32_SUPPORTED)
AKSIMD_V4F32 AKSIMD_ROUNDTONEAREST( AKSIMD_V4F32 in_Vec )
{
	const AKSIMD_V4F32 vfZero = AKSIMD_SETZERO_V4F32();
	const AKSIMD_V4F32 vfBiasPos = AKSIMD_LOAD1_V4F32(0.5f);
	const AKSIMD_V4F32 vfBiasNeg = AKSIMD_LOAD1_V4F32(-0.5f);
	AKSIMD_CMP_CTRLMASK vBiasCtrl = AKSIMD_GT_V4I32(vfZero, in_Vec);
	AKSIMD_V4F32 vfBias = AKSIMD_VSEL_V4F32(vfBiasPos, vfBiasNeg, vBiasCtrl);
	AKSIMD_V4F32 vfOut = AKSIMD_ADD_V4F32(in_Vec, vfBias);
	vfOut = AKSIMD_CVT_V4I32TOV4F32(AKSIMD_CVT_V4F32TOV4I32( vfOut, 0 ), 0);
	return vfOut;
}

AKSIMD_V4F32 AKSIMD_WRAPANGLES( AKSIMD_V4F32 in_Vec )
{
	const AkReal32 fOneOverTwoPi = 0.159154943f;
	AKSIMD_V4F32 vfOneOverTwoPi = AKSIMD_LOAD1_V4F32( fOneOverTwoPi );
	const AkReal32 fTwoPi = 6.283185307f;
	AKSIMD_V4F32 vfTwoPi = AKSIMD_LOAD1_V4F32( fTwoPi );
	AKSIMD_V4F32 vNorm = AKSIMD_MUL_V4F32(in_Vec, vfOneOverTwoPi);
	vNorm = AKSIMD_ROUNDTONEAREST( vNorm );
	vNorm = spu_nmsub(vfTwoPi, vNorm, in_Vec);
	return vNorm;
}

    // sin(V) ~= V - V^3 / 3! + V^5 / 5! - V^7 / 7! + V^9 / 9! - V^11 / 11! + V^13 / 13! - 
    //           V^15 / 15! + V^17 / 17! - V^19 / 19! + V^21 / 21! - V^23 / 23! (for -PI <= V < PI)
    // cos(V) ~= 1 - V^2 / 2! + V^4 / 4! - V^6 / 6! + V^8 / 8! - V^10 / 10! + V^12 / 12! - 
    //           V^14 / 14! + V^16 / 16! - V^18 / 18! + V^20 / 20! - V^22 / 22! (for -PI <= V < PI)

const AkReal32 g_fAKSIMD_Sin_Coefs[12] = {1.0f, -0.166666667f, 8.333333333e-3f, -1.984126984e-4f, 2.755731922e-6f, -2.505210839e-8f, 1.605904384e-10f, -7.647163732e-13f, 2.811457254e-15f, -8.220635247e-18f, 1.957294106e-20f, -3.868170171e-23f };
const AkReal32 g_fAKSIMD_Cos_Coefs[12] = {1.0f, -0.5f, 4.166666667e-2f, -1.388888889e-3f, 2.480158730e-5f, -2.755731922e-7f, 2.087675699e-9f, -1.147074560e-11f, 4.779477332e-14f, -1.561920697e-16f, 4.110317623e-19f, -8.896791392e-22f };

#define AKSIMD_SINCOS_SETUP()										\
    AKSIMD_V4F32 AKSIMD_SinCos_S1, AKSIMD_SinCos_S2, AKSIMD_SinCos_S3, AKSIMD_SinCos_S4, AKSIMD_SinCos_S5, AKSIMD_SinCos_S6, AKSIMD_SinCos_S7, AKSIMD_SinCos_S8, AKSIMD_SinCos_S9, AKSIMD_SinCos_S10, AKSIMD_SinCos_S11;		\
    AKSIMD_V4F32 AKSIMD_SinCos_C1, AKSIMD_SinCos_C2, AKSIMD_SinCos_C3, AKSIMD_SinCos_C4, AKSIMD_SinCos_C5, AKSIMD_SinCos_C6, AKSIMD_SinCos_C7, AKSIMD_SinCos_C8, AKSIMD_SinCos_C9, AKSIMD_SinCos_C10, AKSIMD_SinCos_C11;		\
 	AKSIMD_SinCos_S1  = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Sin_Coefs[1]);					\
    AKSIMD_SinCos_S2  = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Sin_Coefs[2]);					\
    AKSIMD_SinCos_S3  = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Sin_Coefs[3]);					\
    AKSIMD_SinCos_S4  = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Sin_Coefs[4]);					\
    AKSIMD_SinCos_S5  = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Sin_Coefs[5]);					\
    AKSIMD_SinCos_S6  = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Sin_Coefs[6]);					\
    AKSIMD_SinCos_S7  = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Sin_Coefs[7]);					\
    AKSIMD_SinCos_S8  = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Sin_Coefs[8]);					\
    AKSIMD_SinCos_S9  = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Sin_Coefs[9]);					\
    AKSIMD_SinCos_S10  = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Sin_Coefs[10]);					\
    AKSIMD_SinCos_S11  = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Sin_Coefs[11]);					\
    AKSIMD_SinCos_C1 = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Cos_Coefs[1]);					\
    AKSIMD_SinCos_C2 = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Cos_Coefs[2]);					\
    AKSIMD_SinCos_C3 = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Cos_Coefs[3]);					\
    AKSIMD_SinCos_C4 = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Cos_Coefs[4]);					\
    AKSIMD_SinCos_C5 = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Cos_Coefs[5]);					\
    AKSIMD_SinCos_C6 = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Cos_Coefs[6]);					\
    AKSIMD_SinCos_C7 = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Cos_Coefs[7]);					\
    AKSIMD_SinCos_C8 = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Cos_Coefs[8]);					\
    AKSIMD_SinCos_C9 = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Cos_Coefs[9]);					\
    AKSIMD_SinCos_C10 = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Cos_Coefs[10]);					\
    AKSIMD_SinCos_C11 = AKSIMD_LOAD1_V4F32(g_fAKSIMD_Cos_Coefs[11]);	


#define AKSIMD_SINCOS( vSin, vCos, V )											\
{																				\
	AKSIMD_V4F32 V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18, V19, V20, V21, V22, V23;\
	V1 = AKSIMD_WRAPANGLES( V );\
    V2 = AKSIMD_MUL_V4F32(V1, V1);\
    V3 = AKSIMD_MUL_V4F32(V2, V1);\
    V4 = AKSIMD_MUL_V4F32(V2, V2);\
    V5 = AKSIMD_MUL_V4F32(V3, V2);\
    V6 = AKSIMD_MUL_V4F32(V3, V3);\
    V7 = AKSIMD_MUL_V4F32(V4, V3);\
    V8 = AKSIMD_MUL_V4F32(V4, V4);\
    V9 = AKSIMD_MUL_V4F32(V5, V4);\
    V10 = AKSIMD_MUL_V4F32(V5, V5);\
    V11 = AKSIMD_MUL_V4F32(V6, V5);\
    V12 = AKSIMD_MUL_V4F32(V6, V6);\
    V13 = AKSIMD_MUL_V4F32(V7, V6);\
    V14 = AKSIMD_MUL_V4F32(V7, V7);\
    V15 = AKSIMD_MUL_V4F32(V8, V7);\
    V16 = AKSIMD_MUL_V4F32(V8, V8);\
    V17 = AKSIMD_MUL_V4F32(V9, V8);\
    V18 = AKSIMD_MUL_V4F32(V9, V9);\
    V19 = AKSIMD_MUL_V4F32(V10, V9);\
    V20 = AKSIMD_MUL_V4F32(V10, V10);\
    V21 = AKSIMD_MUL_V4F32(V11, V10);\
    V22 = AKSIMD_MUL_V4F32(V11, V11);\
    V23 = AKSIMD_MUL_V4F32(V12, V11);\
    vSin = AKSIMD_MADD_V4F32(AKSIMD_SinCos_S1, V3, V1);\
    vSin = AKSIMD_MADD_V4F32(AKSIMD_SinCos_S2, V5, vSin);\
    vSin = AKSIMD_MADD_V4F32(AKSIMD_SinCos_S3, V7, vSin);\
    vSin = AKSIMD_MADD_V4F32(AKSIMD_SinCos_S4, V9, vSin);\
    vSin = AKSIMD_MADD_V4F32(AKSIMD_SinCos_S5, V11, vSin);\
    vSin = AKSIMD_MADD_V4F32(AKSIMD_SinCos_S6, V13, vSin);\
    vSin = AKSIMD_MADD_V4F32(AKSIMD_SinCos_S7, V15, vSin);\
    vSin = AKSIMD_MADD_V4F32(AKSIMD_SinCos_S8, V17, vSin);\
    vSin = AKSIMD_MADD_V4F32(AKSIMD_SinCos_S9, V19, vSin);\
    vSin = AKSIMD_MADD_V4F32(AKSIMD_SinCos_S10, V21, vSin);\
    vSin = AKSIMD_MADD_V4F32(AKSIMD_SinCos_S11, V23, vSin);\
    vCos = AKSIMD_MADD_V4F32(AKSIMD_SinCos_C1, V2, AKSIMD_LOAD1_V4F32(1.0f));\
    vCos = AKSIMD_MADD_V4F32(AKSIMD_SinCos_C2, V4, vCos);\
    vCos = AKSIMD_MADD_V4F32(AKSIMD_SinCos_C3, V6, vCos);\
    vCos = AKSIMD_MADD_V4F32(AKSIMD_SinCos_C4, V8, vCos);\
    vCos = AKSIMD_MADD_V4F32(AKSIMD_SinCos_C5, V10, vCos);\
    vCos = AKSIMD_MADD_V4F32(AKSIMD_SinCos_C6, V12, vCos);\
    vCos = AKSIMD_MADD_V4F32(AKSIMD_SinCos_C7, V14, vCos);\
    vCos = AKSIMD_MADD_V4F32(AKSIMD_SinCos_C8, V16, vCos);\
    vCos = AKSIMD_MADD_V4F32(AKSIMD_SinCos_C9, V18, vCos);\
    vCos = AKSIMD_MADD_V4F32(AKSIMD_SinCos_C10, V20, vCos);\
    vCos = AKSIMD_MADD_V4F32(AKSIMD_SinCos_C11, V22, vCos);\
}

#endif 

#ifdef AKSIMD_V4F32_SUPPORTED
// Trig functions approximation (based on the Fast versions found in AkMath.h)
AkForceInline AKSIMD_V4F32 AKSIMD_SIN_V4F32(const AKSIMD_V4F32 x)
{
	const AKSIMD_V4F32 B = AKSIMD_SET_V4F32(4/PI);
	const AKSIMD_V4F32 C = AKSIMD_SET_V4F32(-4/(PI*PI));
	const AKSIMD_V4F32 P = AKSIMD_SET_V4F32(0.225f);

	//float y = B * x + C * x * fabs(x); //float y = X*(B+C*fabs(x));

	AKSIMD_V4F32 y = AKSIMD_ABS_V4F32(x);
	y = AKSIMD_MADD_V4F32(y, C, B);
	y = AKSIMD_MUL_V4F32(y, x);

	//	return P * (y * fabs(y) - y) + y; 
	AKSIMD_V4F32 sine = AKSIMD_ABS_V4F32(y);
	sine = AKSIMD_MSUB_V4F32(y, sine, y);
	sine = AKSIMD_MADD_V4F32(sine, P, y);
	return sine;
}

AkForceInline AKSIMD_V4F32 AKSIMD_COS_V4F32(const AKSIMD_V4F32 x)
{
	//Compute the offset needed for the cosinus.  If you compare with FastCos, the constants have been combined.
	const AKSIMD_V4F32 offsetNoWrap = AKSIMD_SET_V4F32(PI/2);				// cos = sin(x+pi/2)
	const AKSIMD_V4F32 offsetWrap = AKSIMD_SET_V4F32(PI/2-2*PI);		// Wrap: cos(x) = cos(x - 2 pi)
	const AKSIMD_V4F32 vHalfPI = AKSIMD_SET_V4F32(PI/2);
	
	// (cond1 >= cond2) ? a : b
	AKSIMD_V4F32 offset = AKSIMD_SEL_GTEZ_V4F32(AKSIMD_SUB_V4F32(x, vHalfPI), offsetWrap, offsetNoWrap);
	return AKSIMD_SIN_V4F32(AKSIMD_ADD_V4F32(x, offset));
}

AkForceInline AKSIMD_V4F32 AKSIMD_ATAN2_V4F32(AKSIMD_V4F32 y, AKSIMD_V4F32 x)
{
	const AKSIMD_V4F32 vNeg = AKSIMD_SET_V4F32(-1.0f);
	const AKSIMD_V4F32 vOne = AKSIMD_SET_V4F32(1.0f);
	const AKSIMD_V4F32 vZero = AKSIMD_SET_V4F32(0.0f);
	const AKSIMD_V4F32 vK = AKSIMD_SET_V4F32(0.28f);
	const AKSIMD_V4F32 vKRepro = AKSIMD_SET_V4F32(1.f/0.28f);
	const AKSIMD_V4F32 vHalfPI = AKSIMD_SET_V4F32(PI/2);
	const AKSIMD_V4F32 vPI = AKSIMD_SET_V4F32(PI);
	const AKSIMD_V4F32 vEpsilon = AKSIMD_SET_V4F32(1e-20f);

	//Ensure x is not zero a == 0 ? b : c.
	x = AKSIMD_VSEL_V4F32(x, vEpsilon, AKSIMD_EQ_V4F32(x, vZero));

	AKSIMD_V4F32 z = AKSIMD_DIV_V4F32(y, x);
	AKSIMD_V4F32 absz = AKSIMD_ABS_V4F32(z);
	AKSIMD_V4COND zcond = AKSIMD_GTEQ_V4F32(vOne, absz);

	//The approximation is done in 2 segments of the form: offset + z/a*(z*z + b);

	//if ( fabsf( z ) < 1.0f ) then use .28 for the a coef
	AKSIMD_V4F32 a = AKSIMD_VSEL_V4F32(vNeg, vK, zcond);

	//if ( fabsf( z ) < 1.0f ) then use 1 for the b factor, else use 0.28
	AKSIMD_V4F32 b = AKSIMD_VSEL_V4F32(vK, vKRepro, zcond);

	AKSIMD_V4F32 atan = AKSIMD_MADD_V4F32(z, z, b);
	atan = AKSIMD_MUL_V4F32(atan, a);
	atan = AKSIMD_DIV_V4F32(z, atan);

	//Adjust for quadrant
	//	zcond	x<0		y<0	 offset
	//	1		0		0	 0			
	//	1		0		1	 0			
	//	1		1		0	 +PI		
	//	1		1		1	 -PI		
	//	0		0		0	 +PI/2		
	//	0		0		1	 -PI/2		
	//	0		1		0	 +PI/2		
	//	0		1		1	 -PI/2		

	AKSIMD_V4F32 offsetByX = AKSIMD_SEL_GTEZ_V4F32(x, vZero, vPI);
	AKSIMD_V4F32 offset = AKSIMD_VSEL_V4F32(vHalfPI, offsetByX, zcond);
	AKSIMD_V4F32 sign = AKSIMD_SEL_GTEZ_V4F32(y, vOne, vNeg);

	//Apply computed offset.  
	atan = AKSIMD_MADD_V4F32(offset, sign, atan);
	return atan;
}
#elif defined AKSIMD_V2F32_SUPPORTED
// Trig functions approximation (based on the Fast versions found in AkMath.h)
AkForceInline AKSIMD_V2F32 AKSIMD_SIN_V2F32(const AKSIMD_V2F32 x)
{
	const AKSIMD_V2F32 B = {4/PI, 4/PI};
	const AKSIMD_V2F32 C = {-4/(PI*PI), -4/(PI*PI)};
	const AKSIMD_V2F32 P = {0.225, 0.225};

	//float y = B * x + C * x * fabs(x); //float y = X*(B+C*fabs(x));
	
	AKSIMD_V2F32 y = AKSIMD_ABS_V2F32(x);
	y = AKSIMD_MADD_V2F32(y, C, B);
	y = AKSIMD_MUL_V2F32(y, x);

	//	return P * (y * fabs(y) - y) + y; 
	AKSIMD_V2F32 sine = AKSIMD_ABS_V2F32(y);
	sine = AKSIMD_MSUB_V2F32(y, sine, y);
	sine = AKSIMD_MADD_V2F32(sine, P, y);
	return sine;
}

AkForceInline AKSIMD_V2F32 AKSIMD_COS_V2F32(const AKSIMD_V2F32 x)
{
	//Compute the offset needed for the cosinus.  If you compare with FastCos, the constants have been combined.
	const AKSIMD_V2F32 offsetNoWrap = {PI/2, PI/2};				// cos = sin(x+pi/2)
	const AKSIMD_V2F32 offsetWrap = {PI/2-2*PI, PI/2-2*PI};		// Wrap: cos(x) = cos(x - 2 pi)
	const AKSIMD_V2F32 vPI = {PI/2, PI/2};

	// (cond1 >= cond2) ? a : b
	AKSIMD_V2F32 offset = AKSIMD_SEL_V2F32(AKSIMD_SUB_V2F32(x, vPI), offsetWrap, offsetNoWrap);
	return AKSIMD_SIN_V2F32(AKSIMD_ADD_V2F32(x, offset));
}

/*
	Based on the FastAtan2F found in AkMath
	if ( x != 0.0f )
	{
		float atan;
		float z = y/x;
		if ( fabsf( z ) < 1.0f )
		{
			atan = z/(1.0f + 0.28f*z*z);
			if ( x < 0.0f )
			{
				if ( y < 0.0f ) 
					return atan - PI_FLOAT;
				else
					return atan + PI_FLOAT;
			}
		}
		else
		{
			atan = - z/(z*z + 0.28f);
			if ( y < 0.0f )
				return atan - PIBY2_FLOAT;
			else
				return atan + PIBY2_FLOAT;
		}
		return atan;
	}*/
// This is 1.47x times faster than calling FastAtan2F twice
AkForceInline AKSIMD_V2F32 AKSIMD_ATAN2_V2F32( AKSIMD_V2F32 y, AKSIMD_V2F32 x )
{
	AKSIMD_V2F32 vNeg = AKSIMD_SET_V2F32(-1.0f);
	AKSIMD_V2F32 vOne = AKSIMD_SET_V2F32(1.0f);
	AKSIMD_V2F32 vZero = AKSIMD_SET_V2F32(0.0f);
	AKSIMD_V2F32 vK = AKSIMD_SET_V2F32(0.28f);
	AKSIMD_V2F32 vKRepro = AKSIMD_SET_V2F32(1.f/0.28f);
	AKSIMD_V2F32 vHalfPI = AKSIMD_SET_V2F32(PI/2);
	AKSIMD_V2F32 vPI = AKSIMD_SET_V2F32(PI);
	AKSIMD_V2F32 vEpsilon = AKSIMD_SET_V2F32(1e-20f);
	
	//Ensure x is not zero a == 0 ? b : c.
	//(SEL means a >= 0 ? b : c.  Therefore, if the -abs(x) >= 0, it means it is actually = 0.  
	//This is for WiiU.  Other platforms, might have something more intelligent to do what we want
	x = AKSIMD_SEL_V2F32(AKSIMD_NEG_V2F32(AKSIMD_ABS_V2F32(x)), vEpsilon, x);

	AKSIMD_V2F32 z = AKSIMD_DIV_V2F32(y, x);
	AKSIMD_V2F32 absz = AKSIMD_ABS_V2F32(z);
	AKSIMD_V2F32 zcond = AKSIMD_SUB_V2F32(vOne, absz);

	//The approximation is done in 2 segments of the form: offset + z/a*(z*z + b);

	//if ( fabsf( z ) < 1.0f ) then use .28 for the a coef
	AKSIMD_V2F32 a = AKSIMD_SEL_V2F32(zcond, vK, vNeg);

	//if ( fabsf( z ) < 1.0f ) then use 1 for the b factor, else use 0.28
	AKSIMD_V2F32 b = AKSIMD_SEL_V2F32(zcond, vKRepro, vK);

	AKSIMD_V2F32 atan = AKSIMD_MADD_V2F32(z, z, b);
	atan = AKSIMD_MUL_V2F32(atan, a);
	atan = AKSIMD_DIV_V2F32(z, atan);

	//Adjust for quadrant
	//	zcond	x<0		y<0	 offset
	//	1		0		0	 0			
	//	1		0		1	 0			
	//	1		1		0	 +PI		
	//	1		1		1	 -PI		
	//	0		0		0	 +PI/2		
	//	0		0		1	 -PI/2		
	//	0		1		0	 +PI/2		
	//	0		1		1	 -PI/2		

	AKSIMD_V2F32 offsetByX = AKSIMD_SEL_V2F32(x, vZero, vPI);
	AKSIMD_V2F32 offset = AKSIMD_SEL_V2F32(zcond, offsetByX, vHalfPI);
	AKSIMD_V2F32 sign = AKSIMD_SEL_V2F32(y, vOne, vNeg);

	//Apply computed offset.  
	atan = AKSIMD_MADD_V2F32(offset, sign, atan);

	return atan;
}
#endif

#endif // _AK_SIMDMATH_H_