/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "xmmintrin.h"
/************************************************************************/
/*	Load/Store intrinsics                                               */
/************************************************************************/
RED_INLINE void VecRegLoad(VecReg& out, Float* in)
{
	out = _mm_loadu_ps(in);
}

RED_INLINE void VecRegLoadAligned(VecReg& out, Float* in)
{
	out = _mm_load_ps(in);
}

RED_INLINE void VecRegStore(Float* out, VecReg& in)
{
	_mm_storeu_ps(out, in);
}

RED_INLINE void VecRegStoreAligned(Float* out, VecReg& in)
{
	_mm_store_ps(out, in);
}

RED_INLINE void VecRegSet(VecReg& out, Float w)
{
	out = _mm_set_ss(w);
}

RED_INLINE void VecRegSet(VecReg& out, Float x, Float y, Float z, Float w)
{
	out = _mm_set_ps(w, z, y, x);
}
/************************************************************************/
/* Arithmetic operations intrinsics                                     */
/************************************************************************/
RED_INLINE void VecRegAdd(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = _mm_add_ps(a, b);
}

RED_INLINE void VecRegSub(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = _mm_sub_ps(a, b);
}

RED_INLINE void VecRegMul(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = _mm_mul_ps(a, b);
}

RED_INLINE void VecRegDiv(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = _mm_div_ps(a, b);
}

RED_INLINE void VecRegSqrt(VecReg& out, const VecReg& a)
{
	out = _mm_sqrt_ps(a);
}

RED_INLINE void VecRegRcp(VecReg& out, const VecReg& a)
{
	out = _mm_rcp_ps(a);
}

RED_INLINE void VecRegRsqrt(VecReg& out, const VecReg& a)
{
	out = _mm_rsqrt_ps(a);
}

RED_INLINE void VecRegMin(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = _mm_min_ps(a, b);
}

RED_INLINE void VecRegMax(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = _mm_max_ps(a, b);
}
/************************************************************************/
/* Logical operations intrinsics                                        */
/************************************************************************/
RED_INLINE void VecRegAnd(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = _mm_and_ps(a, b);
}

RED_INLINE void VecRegOr(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = _mm_or_ps(a, b);
}

RED_INLINE void VecRegXor(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = _mm_xor_ps(a, b);
}
/************************************************************************/
/* Compound operations                                                  */
/************************************************************************/
RED_INLINE void VecRegRep0(VecReg& out, const VecReg& a)
{
	out = _mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 0, 0, 0));
}

RED_INLINE void VecRegRep1(VecReg& out, const VecReg& a)
{
	out = _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1));
}

RED_INLINE void VecRegRep2(VecReg& out, const VecReg& a)
{
	out = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 2, 2, 2));
}

RED_INLINE void VecRegRep3(VecReg& out, const VecReg& a)
{
	out = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 3, 3, 3));
}

RED_INLINE void VecRegZero(VecReg& out)
{
	out = _mm_setzero_ps();
}

RED_INLINE void VecRegOne(VecReg& out)
{
	out = _mm_set_ps1(1.0f);
}

