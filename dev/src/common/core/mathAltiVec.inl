/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
/************************************************************************/
/*	Load/Store intrinsics                                               */
/************************************************************************/
RED_INLINE void VecRegLoad(VecReg& out, Float* in)
{
	VecReg v0, v1, permute;

	v0 = __lvx(in, 0);                            // Load the 16 bytes starting at address pSource & ~0xF
	permute = __lvsl(in, 0);                      // Compute the permute control vector for shift left
	v1 = __lvx(in, 15);                           // Load the 16 bytes starting at address ((BYTE*)pSource + 15) & ~0xF
	out = __vperm(v0, v1, permute); 
}

RED_INLINE void VecRegLoadAligned(VecReg& out, Float* in)
{
	out = __lvx(in, 0);
}

RED_INLINE void VecRegStore(Float* out, VecReg& in)
{
	__stvlx(in, out, 0 );
	__stvrx(in, out, 16);
}

RED_INLINE void VecRegStoreAligned(Float* out, VecReg& in)
{
	__stvx(in, out, 0);
}

RED_INLINE void VecRegSet(VecReg& out, Float w)
{
	VecReg v0, v1, v2, permute;
	
	v0 = __lvx(&w, 0);                            // Load the 16 bytes starting at address pSource & ~0xF
	permute = __lvsl(&w, 0);                      // Compute the permute control vector for shift left
	v1 = __lvx(&w, 15);                           // Load the 16 bytes starting at address ((BYTE*)pSource + 15) & ~0xF
	v2 = __vperm(v0, v1, permute); 
	out = __vspltw(v2, 0);
}

RED_INLINE void VecRegSet(VecReg& out, Float x, Float y, Float z, Float w)
{
	out.v[0] = x;
	out.v[1] = y;
	out.v[2] = z;
	out.v[3] = w;
}
/************************************************************************/
/* Arithmetic operations intrinsics                                     */
/************************************************************************/
RED_INLINE void VecRegAdd(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = __vaddfp(a, b);
}

RED_INLINE void VecRegSub(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = __vsubfp(a, b);
}

RED_INLINE void VecRegMul(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = __vmulfp(a, b);
}

RED_INLINE void VecRegDiv(VecReg& out, const VecReg& a, const VecReg& b)
{
	// the estimate has a relative error in precision no greater than one part in 4,096
	out = __vmulfp(a, __vrefp(b));
}

RED_INLINE void VecRegSqrt(VecReg& out, const VecReg& a)
{
	// the estimate has a relative error in precision no greater than one part in 4,096
	out = __vrefp(__vrsqrtefp(a));
}

RED_INLINE void VecRegRcp(VecReg& out, const VecReg& a)
{
	// the estimate has a relative error in precision no greater than one part in 4,096
	out = __vrefp(a);
}

RED_INLINE void VecRegRsqrt(VecReg& out, const VecReg& a)
{
	// the estimate has a relative error in precision no greater than one part in 4,096
	out = __vrsqrtefp(a);
}

RED_INLINE void VecRegMin(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = __vminfp(a, b);
}

RED_INLINE void VecRegMax(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = __vmaxfp(a, b);
}
/************************************************************************/
/* Logical operations intrinsics                                        */
/************************************************************************/
RED_INLINE void VecRegAnd(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = __vand(a, b);
}

RED_INLINE void VecRegOr(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = __vor(a, b);
}

RED_INLINE void VecRegXor(VecReg& out, const VecReg& a, const VecReg& b)
{
	out = __vxor(a, b);
}
/************************************************************************/
/* Compound operations                                                  */
/************************************************************************/
RED_INLINE void VecRegRep0(VecReg& out, const VecReg& a)
{
	out = __vspltw(a, 0);
}

RED_INLINE void VecRegRep1(VecReg& out, const VecReg& a)
{
	out = __vspltw(a, 1);
}

RED_INLINE void VecRegRep2(VecReg& out, const VecReg& a)
{
	out = __vspltw(a, 2);
}

RED_INLINE void VecRegRep3(VecReg& out, const VecReg& a)
{
	out = __vspltw(a, 3);
}

RED_INLINE void VecRegZero(VecReg& out)
{
	out = __vspltisw(0);
}

RED_INLINE void VecRegOne(VecReg& out)
{
	out = __vupkd3d(out, VPACK_NORMSHORT2);
	out = __vspltw(out, 3);
}
