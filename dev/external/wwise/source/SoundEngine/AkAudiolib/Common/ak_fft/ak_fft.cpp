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

#include <AK/SoundEngine/Common/AkSimd.h>
#include <AK/Tools/Common/AkAssert.h>
#include "_ak_fft_guts.h"

/* The guts header contains all the multiplication and addition macros that are defined for
 fixed or floating point complex numbers.  It also delares the kf_ internal functions.
 */

namespace DSP
{

// Avoid link collisions between compiled version with or without USEALLBUTTERFLIES
namespace BUTTERFLYSET_NAMESPACE
{

#ifndef __PPU__

#ifdef USEALLBUTTERFLIES 
static void kf_bfly2(
        ak_fft_cpx * Fout,
        const size_t fstride,
        const ak_fft_cfg st,
        int m
        )
{
    ak_fft_cpx * Fout2;
    ak_fft_cpx * tw1 = st->twiddles;
    ak_fft_cpx t;
    Fout2 = Fout + m;
    do{
        C_FIXDIV(*Fout,2); C_FIXDIV(*Fout2,2);

        C_MUL (t,  *Fout2 , *tw1);
        tw1 += fstride;
        C_SUB( *Fout2 ,  *Fout , t );
        C_ADDTO( *Fout ,  t );
        ++Fout2;
        ++Fout;
    }while (--m);
}

AkForceInline void kf_bfly2_m1(ak_fft_cpx * AK_RESTRICT Fout)
{
	ak_fft_cpx * Fout2;
	ak_fft_cpx t;
	Fout2 = Fout + 1;

	t.r = Fout2->r;
	t.i = Fout2->i; 

	Fout2->r = Fout->r - t.r;  
	Fout2->i = Fout->i - t.i;

	Fout->r += t.r;  
	Fout->i += t.i;
}

AkForceInline void kf_bfly2_m1x4(ak_fft_cpx * AK_RESTRICT Fout, const ak_fft_cpx * AK_RESTRICT Fin, size_t fstride)
{
	ak_fft_cpx Fin1;
	ak_fft_cpx Fin2;
	ak_fft_cpx * AK_RESTRICT Fout2;

	for(int i = 0; i < 4; i++)	//Hopefully unrolled!
	{
		Fin1 = *Fin;
		Fin2 = *(Fin+fstride*4);
		Fout2 = Fout + 1;

		Fout2->r = Fin1.r - Fin2.r;  
		Fout2->i = Fin1.i - Fin2.i;

		Fout->r = Fin1.r + Fin2.r;  
		Fout->i = Fin1.i + Fin2.i;

		Fout+=2;
		Fin += fstride;
	}
}
#endif // #ifdef USEALLBUTTERFLIES

#ifdef AKSIMD_V2F32_SUPPORTED
static void kf_bfly4_m1_n1024(
							 ak_fft_cpx * AK_RESTRICT Fout,
							 const ak_fft_cfg st
							 )
{
	//The twiddles are 1 or -1 in this case.  Reversed if we're doing an inverse fft.
	AKSIMD_V2F32 tw = {-1, 1};
	if (st->inverse)
		tw = AKSIMD_NEG_V2F32(tw);

	AKSIMD_V2F32 twSwap = AKSIMD_NEG_V2F32(tw);

	AKSIMD_V2F32 vi0 = AKSIMD_LOAD_V2F32_OFFSET(Fout, 0);
	AKSIMD_V2F32 vi1 = AKSIMD_LOAD_V2F32_OFFSET(Fout, sizeof(ak_fft_cpx));
	AKSIMD_V2F32 vi2 = AKSIMD_LOAD_V2F32_OFFSET(Fout, 2*sizeof(ak_fft_cpx));
	AKSIMD_V2F32 vi3 = AKSIMD_LOAD_V2F32_OFFSET(Fout, 3*sizeof(ak_fft_cpx));

	AKSIMD_V2F32 vTmp = AKSIMD_ADD_V2F32(vi0, vi2);
	AKSIMD_V2F32 vFout0 = AKSIMD_ADD_V2F32(vTmp, vi1);
	vFout0 = AKSIMD_ADD_V2F32(vFout0, vi3);

	AKSIMD_V2F32 vFout2 = AKSIMD_SUB_V2F32(vTmp, vi1);
	vFout2 = AKSIMD_SUB_V2F32(vFout2, vi3);

	vi1 = AKSIMD_SWAP_V2F32(vi1);
	vi3 = AKSIMD_SWAP_V2F32(vi3);
	vTmp = AKSIMD_SUB_V2F32(vi0, vi2);
	AKSIMD_V2F32 vFout1 = AKSIMD_MADD_V2F32(vi1, twSwap, vTmp);
	vFout1 = AKSIMD_MADD_V2F32(vi3, tw, vFout1);

	AKSIMD_V2F32 vFout3 = AKSIMD_MADD_V2F32(vi1, tw, vTmp);
	vFout3 = AKSIMD_MADD_V2F32(vi3, twSwap, vFout3);
}
#else
static void kf_bfly4_m1_n1024(ak_fft_cpx * AK_RESTRICT Fout, const ak_fft_cfg st)
{
	const float tw1 = st->twiddles[256].i;
	const float tw2 = st->twiddles[768].i;

	ak_fft_cpx i0 = Fout[0];  
	ak_fft_cpx i1 = Fout[1];
	ak_fft_cpx i2 = Fout[2];
	ak_fft_cpx i3 = Fout[3];

	Fout[ 0 ].r = i0.r + i1.r + i2.r + i3.r;
	Fout[ 0 ].i = i0.i + i1.i + i2.i + i3.i;

	Fout[ 1 ].r = i0.r - i1.i*tw1 - i2.r - i3.i*tw2;
	Fout[ 1 ].i = i0.i + i1.r*tw1 - i2.i + i3.r*tw2;

	Fout[ 2 ].r = i0.r - i1.r + i2.r - i3.r;
	Fout[ 2 ].i = i0.i - i1.i + i2.i - i3.i;

	Fout[ 3 ].r = i0.r - i1.i*tw2 - i2.r - i3.i*tw1;
	Fout[ 3 ].i = i0.i + i1.r*tw2 - i2.i + i3.r*tw1;
}
#endif

#define USE_OPTIMIZEDBUTTERFLY
#ifndef USE_OPTIMIZEDBUTTERFLY

// Original routine (keep as reference)
static void kf_bfly4(
        ak_fft_cpx * Fout,
        const size_t fstride,
        const ak_fft_cfg st,
        const size_t m
        )
{
    ak_fft_cpx *tw1,*tw2,*tw3;
    ak_fft_cpx scratch[6];
    size_t k=m;
    const size_t m2=2*m;
    const size_t m3=3*m;

    tw3 = tw2 = tw1 = st->twiddles;

	do {
			C_FIXDIV(*Fout,4); C_FIXDIV(Fout[m],4); C_FIXDIV(Fout[m2],4); C_FIXDIV(Fout[m3],4);

			C_MUL(scratch[0],Fout[m] , *tw1 );
			C_MUL(scratch[1],Fout[m2] , *tw2 );
			C_MUL(scratch[2],Fout[m3] , *tw3 );

			C_SUB( scratch[5] , *Fout, scratch[1] );
			C_ADDTO(*Fout, scratch[1]);
			C_ADD( scratch[3] , scratch[0] , scratch[2] );
			C_SUB( scratch[4] , scratch[0] , scratch[2] );
			C_SUB( Fout[m2], *Fout, scratch[3] );
			tw1 += fstride;
			tw2 += fstride*2;
			tw3 += fstride*3;
			C_ADDTO( *Fout , scratch[3] );

        	if(st->inverse) {
				Fout[m].r = scratch[5].r - scratch[4].i;
				Fout[m].i = scratch[5].i + scratch[4].r;
				Fout[m3].r = scratch[5].r + scratch[4].i;
				Fout[m3].i = scratch[5].i - scratch[4].r;
        	}else{
				Fout[m].r = scratch[5].r + scratch[4].i;
				Fout[m].i = scratch[5].i - scratch[4].r;
				Fout[m3].r = scratch[5].r - scratch[4].i;
				Fout[m3].i = scratch[5].i + scratch[4].r;
			}
			++Fout;
	}while(--k);
}

#else // #ifndef USE_OPTIMIZEDBUTTERFLY
#ifdef AKSIMD_V4F32_SUPPORTED
// Cross-platform specific intrisics that assume stuff about the current algorithm in order to be optimal
#if defined(AK_XBOX360)
	// because address is aligned on 8 bytes this will give a complete complex number (2 floats)
	static const __vector4i vMoveLowHigh = { 0x00010203, 0x04050607, 0x10111213, 0x14151617 };
	static AkForceInline AKSIMD_V4F32 AKSIMD_LOADUNALIGNED_DUAL( AkReal32 * in_dAddress1, AkReal32 * in_dAddress2 )
	{	
		AKSIMD_V4F32 vA = __lvlx( in_dAddress1, 0 );
		AKSIMD_V4F32 vB = __lvlx( in_dAddress2, 0 );
		return __vperm( vA, vB, *((__vector4*)&vMoveLowHigh));
	}
#elif defined(__SPU__)
	// because address is aligned on 8 bytes this will give a complete complex number (2 floats)
	static const vec_uchar16 vMoveLowHigh = { 0,1,2,3, 4,5,6,7, 16,17,18,19, 20,21,22,23 };
	static AkForceInline AKSIMD_V4F32 AKSIMD_LOADUNALIGNED_DUAL( AkReal32 * in_dAddress1, AkReal32 * in_dAddress2 )
	{	
		AKSIMD_V4F32 vA = *((AKSIMD_V4F32 *) in_dAddress1);
		unsigned int shift = (unsigned) in_dAddress1 & 0xf;
		vA = spu_slqwbyte(vA, shift);
		AKSIMD_V4F32 vB = *((AKSIMD_V4F32 *) in_dAddress2);
		shift = (unsigned) in_dAddress2 & 0xf;
		vB = spu_slqwbyte(vB, shift);
		return spu_shuffle( vA, vB, vMoveLowHigh);
	}
#elif defined(AK_CPU_ARM_NEON)
	static AkForceInline AKSIMD_V4F32 AKSIMD_LOADUNALIGNED_DUAL( AkReal32 * in_dAddress1, AkReal32 * in_dAddress2 )
	{
		AKSIMD_V2F32 vA = AKSIMD_LOAD_V2F32( in_dAddress1 ); 
		AKSIMD_V2F32 vB = AKSIMD_LOAD_V2F32( in_dAddress2 ); 
		return AKSIMD_COMBINE_V2F32( vA, vB );
	}
#else
	static AkForceInline AKSIMD_V4F32 AKSIMD_LOADUNALIGNED_DUAL( AkReal32 * in_dAddress1, AkReal32 * in_dAddress2 )
	{
		AKSIMD_V4F32 vA = AKSIMD_LOADU_V4F32( in_dAddress1 ); 
		AKSIMD_V4F32 vB = AKSIMD_LOADU_V4F32( in_dAddress2 ); 
		return AKSIMD_MOVELH_V4F32( vA, vB );
	}
#endif
#endif // AKSIMD_V4F32_SUPPORTED

#if defined AKSIMD_V2F32_SUPPORTED
static AkForceInline AKSIMD_V2F32 COMPLEX_MUL_WITH_TWIDDLE_REG(AkReal32* AK_RESTRICT in_dTWAddress, AKSIMD_V2F32 vV) 
{
	static const AKSIMD_V2F32 vSign = {-1, 1};

	AKSIMD_V2F32 vTw = AKSIMD_LOAD_V2F32_OFFSET(in_dTWAddress, 0);							
	AKSIMD_V2F32 real = AKSIMD_UNPACKLO_V2F32(vV, vV); //Duplicate real part
	AKSIMD_V2F32 vTmp1 = AKSIMD_MUL_V2F32( real, vTw );
	AKSIMD_V2F32 img = AKSIMD_UNPACKHI_V2F32(vV, vV); //Duplicate imaginary part
	AKSIMD_V2F32 vTmp2 = AKSIMD_MUL_V2F32( img, vSign );
	vTmp2 = AKSIMD_MADD_V2F32( vTmp2, AKSIMD_SWAP_V2F32( vTw ), vTmp1 );
	return vTmp2;
}

#define COMPLEX_MUL_WITH_TWIDDLE(in_dTWAddress, Fout, m)  COMPLEX_MUL_WITH_TWIDDLE_REG(in_dTWAddress, AKSIMD_LOAD_V2F32_OFFSET((AkReal32*)Fout, m*sizeof(ak_fft_cpx)));

static void kf_bfly4(
	ak_fft_cpx * AK_RESTRICT Fout,
	const size_t fstride,
	const ak_fft_cfg st,
	const size_t m
	)
{
	ak_fft_cpx *tw1,*tw2,*tw3;
	size_t k=m;
	const size_t m2=2*m;
	const size_t m3=3*m;

	tw3 = tw2 = tw1 = st->twiddles;
	const size_t tw1Offset = fstride;
	const size_t tw2Offset = fstride*2;
	const size_t tw3Offset = fstride*3;

	//Process ONE complex at the time
	AKSIMD_V2F32 vSignA = { 1.f, -1.f };
	AKSIMD_V2F32 vSignB = { -1.f, 1.f };

	if(st->inverse)
	{
		AKSIMD_V2F32 vTmp = vSignA;
		vSignA = vSignB;
		vSignB = vTmp;
	}

	do 
	{
		AKSIMD_V2F32 vScratch0 = COMPLEX_MUL_WITH_TWIDDLE((AkReal32*)tw1, Fout, m);
		AKSIMD_V2F32 vScratch1 = COMPLEX_MUL_WITH_TWIDDLE((AkReal32*)tw3, Fout, m3);
		AKSIMD_V2F32 vScratch2 = AKSIMD_ADD_V2F32( vScratch0, vScratch1 );
		vScratch0 = AKSIMD_SUB_V2F32( vScratch0, vScratch1 );
		vScratch1 = COMPLEX_MUL_WITH_TWIDDLE((AkReal32*)tw2, Fout, m2);

		AKSIMD_V2F32 vFout = AKSIMD_LOAD_V2F32_OFFSET( (AkReal32*)&Fout[0], 0 );
		AKSIMD_V2F32 vScratch3 = AKSIMD_SUB_V2F32( vFout, vScratch1 );
		vFout = AKSIMD_ADD_V2F32( vFout, vScratch1 );

		AKSIMD_V2F32 vFoutM2 = AKSIMD_SUB_V2F32( vFout, vScratch2 );
		AKSIMD_STORE_V2F32_OFFSET( (AkReal32*)&Fout[m2], 0, vFoutM2);
		vFout = AKSIMD_ADD_V2F32( vFout, vScratch2 );
		AKSIMD_STORE_V2F32_OFFSET( (AkReal32*)&Fout[0], 0, vFout );

		vScratch1 = AKSIMD_SWAP_V2F32( vScratch0 );
		vScratch1 = AKSIMD_MUL_V2F32(vScratch1, vSignA);
		AKSIMD_V2F32 vFoutm = AKSIMD_ADD_V2F32( vScratch3, vScratch1 ) ;
		AKSIMD_V2F32 vFoutm3 = AKSIMD_SUB_V2F32( vScratch3, vScratch1 ) ;
		AKSIMD_STORE_V2F32_OFFSET( (AkReal32*)&Fout[m], 0, vFoutm );
		AKSIMD_STORE_V2F32_OFFSET( (AkReal32*)&Fout[m3], 0, vFoutm3 );

		tw1 += tw1Offset;
		tw2 += tw2Offset;
		tw3 += tw3Offset;
		Fout ++;
	}while(--k);
}

#else

// SIMD optimized routine processing 2 complex numbers per iteration
static void kf_bfly4(
        ak_fft_cpx * AK_RESTRICT Fout,
        const size_t fstride,
        const ak_fft_cfg st,
        const size_t m
        )
{
    ak_fft_cpx *tw1,*tw2,*tw3;
    size_t k=m;
    const size_t m2=2*m;
    const size_t m3=3*m;

	tw3 = tw2 = tw1 = st->twiddles;
	const size_t tw1Offset = fstride;
	const size_t tw2Offset = fstride*2;
	const size_t tw3Offset = fstride*3;

#ifdef AKSIMD_V4F32_SUPPORTED
	if ( k >= 2 )
	{
		AKASSERT( (k % 2) == 0 );
		// TODO: Because k will be limited to 4,16,64,256 consider writing a special case for each with fixed offsets (macros)
		k >>= 1; // Each iteration process 2 interleaved complex number at a time 

		AKSIMD_DECLARE_V4F32( vSignA, 1.f, -1.f, 1.f, -1.f  );
		AKSIMD_DECLARE_V4F32( vSignB, -1.f, 1.f, -1.f, 1.f );

		if(st->inverse)
		{
			AKSIMD_DECLARE_V4F32_TYPE vTmp = vSignA;
			vSignA = vSignB;
			vSignB = vTmp;
		}

		do 
		{
			AKSIMD_V4F32 vFoutM = AKSIMD_LOAD_V4F32( (AkReal32*)&Fout[m] ); 
			AKSIMD_V4F32 vTw1  = AKSIMD_LOADUNALIGNED_DUAL( (AkReal32*)tw1, (AkReal32*)(tw1+tw1Offset) );
			AKSIMD_V4F32 vScratch0 = AKSIMD_COMPLEXMUL( vFoutM, vTw1 );
			AKSIMD_V4F32 vFoutM3 = AKSIMD_LOAD_V4F32( (AkReal32*)&Fout[m3] ); 
			AKSIMD_V4F32 vTw3  = AKSIMD_LOADUNALIGNED_DUAL( (AkReal32*)tw3, (AkReal32*)(tw3+tw3Offset) );		
			AKSIMD_V4F32 vScratch1 = AKSIMD_COMPLEXMUL( vFoutM3, vTw3 );
			AKSIMD_V4F32 vScratch2 = AKSIMD_ADD_V4F32( vScratch0, vScratch1 );
			vScratch0 = AKSIMD_SUB_V4F32( vScratch0, vScratch1 );

			AKSIMD_V4F32 vFoutM2 = AKSIMD_LOAD_V4F32( (AkReal32*)&Fout[m2] ); 			
			AKSIMD_V4F32 vTw2  = AKSIMD_LOADUNALIGNED_DUAL( (AkReal32*)tw2, (AkReal32*)(tw2+tw2Offset) );
			vScratch1 = AKSIMD_COMPLEXMUL( vFoutM2, vTw2 );		

			AKSIMD_V4F32 vFout = AKSIMD_LOAD_V4F32( (AkReal32*)&Fout[0] ); 
			AKSIMD_V4F32 vScratch3 = AKSIMD_SUB_V4F32( vFout, vScratch1 );
			vFout = AKSIMD_ADD_V4F32( vFout, vScratch1 );

			vFoutM2 = AKSIMD_SUB_V4F32( vFout, vScratch2 );
			AKSIMD_STORE_V4F32( (AkReal32*)&Fout[m2], vFoutM2 );
			vFout = AKSIMD_ADD_V4F32( vFout, vScratch2 );
			AKSIMD_STORE_V4F32( (AkReal32*)&Fout[0], vFout );

			vScratch1 = AKSIMD_SHUFFLE_BADC( vScratch0 ); 	
			vScratch1 = AKSIMD_MUL_V4F32( vScratch1, vSignA );
			vScratch1 = AKSIMD_ADD_V4F32( vScratch3, vScratch1 );
			AKSIMD_STORE_V4F32( (AkReal32*)&Fout[m], vScratch1 );
			vScratch2 = AKSIMD_SHUFFLE_BADC( vScratch0 ); 	
			vScratch2 = AKSIMD_MUL_V4F32( vScratch2, vSignB );
			vScratch2 = AKSIMD_ADD_V4F32( vScratch3, vScratch2 );
			AKSIMD_STORE_V4F32( (AkReal32*)&Fout[m3], vScratch2 );

			tw1 += 2*tw1Offset;
			tw2 += 2*tw2Offset;
			tw3 += 2*tw3Offset;
			Fout += 2;

		}while(--k);
	}
	else
	{
#endif // AKSIMD_V4F32_SUPPORTED
		 // Cannot use vector code for k == 1
		ak_fft_cpx scratch0, scratch1, scratch2, scratch3;
		const int inverseFFT = st->inverse;

		do 
		{
			C_MUL(scratch0,Fout[m] , *tw1 );
			C_MUL(scratch1,Fout[m3] , *tw3 );
			C_ADD( scratch2 , scratch0 , scratch1 );
			C_SUB( scratch0 , scratch0 , scratch1 );

			C_MUL(scratch1,Fout[m2] , *tw2 );
			C_SUB( scratch3 , Fout[0], scratch1 );
			C_ADDTO(Fout[0], scratch1);
			
			C_SUB( Fout[m2], Fout[0], scratch2 );
			C_ADDTO( Fout[0] , scratch2 );
			
			if(inverseFFT) 
			{
				Fout[m].r = scratch3.r - scratch0.i;
				Fout[m].i = scratch3.i + scratch0.r;
				Fout[m3].r = scratch3.r + scratch0.i;
				Fout[m3].i = scratch3.i - scratch0.r;
			}
			else
			{
				Fout[m].r = scratch3.r + scratch0.i;
				Fout[m].i = scratch3.i - scratch0.r;
				Fout[m3].r = scratch3.r - scratch0.i;
				Fout[m3].i = scratch3.i + scratch0.r;
			}		

			tw1 += tw1Offset;
			tw2 += tw2Offset;
			tw3 += tw3Offset;
			++Fout;
		}while(--k);

#ifdef AKSIMD_V4F32_SUPPORTED
	}
#endif 
}
#endif
#endif // #ifndef USE_OPTIMIZEDBUTTERFLY

#ifdef AKSIMD_V4F32_SUPPORTED

#ifdef AK_CPU_ARM_NEON

#define ADJUST_ALIGMENT

#define GET_PAIRS \
	{ \
		AKSIMD_V2F32 i0A = AKSIMD_LOAD_V2F32(pInA); pInA += 256; \
		AKSIMD_V2F32 i1A = AKSIMD_LOAD_V2F32(pInA); pInA += 256; \
		AKSIMD_V2F32 i2A = AKSIMD_LOAD_V2F32(pInA); pInA += 256; \
		AKSIMD_V2F32 i3A = AKSIMD_LOAD_V2F32(pInA); \
		\
		AKSIMD_V2F32 i0B = AKSIMD_LOAD_V2F32(pInB); pInB += 256; \
		AKSIMD_V2F32 i1B = AKSIMD_LOAD_V2F32(pInB); pInB += 256; \
		AKSIMD_V2F32 i2B = AKSIMD_LOAD_V2F32(pInB); pInB += 256; \
		AKSIMD_V2F32 i3B = AKSIMD_LOAD_V2F32(pInB); \
		\
		i0AB = vcombine_f32(i0A, i0B); \
		i1AB = vcombine_f32(i1A, i1B); \
		i2AB = vcombine_f32(i2A, i2B); \
		i3AB = vcombine_f32(i3A, i3B); \
	}

#else

#define ADJUST_ALIGMENT \
	bool bUnaligned = false; \
	if ((AkUIntPtr)Fin  & (AK_SIMD_ALIGNMENT-1)) \
	{ \
		AKASSERT(((AkUIntPtr)Fin & (AK_SIMD_ALIGNMENT-1)) == sizeof(ak_fft_cpx)); \
		Fin--; \
		bUnaligned = true; \
	}

//Keep the low complex or the high complex only.  The other is discarded.

#define GET_PAIRS \
	{ \
		AKSIMD_V4F32 i0A = AKSIMD_LOAD_V4F32(pInA); pInA += 256; \
		AKSIMD_V4F32 i1A = AKSIMD_LOAD_V4F32(pInA); pInA += 256; \
		AKSIMD_V4F32 i2A = AKSIMD_LOAD_V4F32(pInA); pInA += 256; \
		AKSIMD_V4F32 i3A = AKSIMD_LOAD_V4F32(pInA); \
		\
		AKSIMD_V4F32 i0B = AKSIMD_LOAD_V4F32(pInB); pInB += 256; \
		AKSIMD_V4F32 i1B = AKSIMD_LOAD_V4F32(pInB); pInB += 256; \
		AKSIMD_V4F32 i2B = AKSIMD_LOAD_V4F32(pInB); pInB += 256; \
		AKSIMD_V4F32 i3B = AKSIMD_LOAD_V4F32(pInB); \
		\
		if (bUnaligned)	\
		{	\
			i0AB = AKSIMD_MOVEHL_V4F32(i0B, i0A);	\
			i1AB = AKSIMD_MOVEHL_V4F32(i1B, i1A);	\
			i2AB = AKSIMD_MOVEHL_V4F32(i2B, i2A);	\
			i3AB = AKSIMD_MOVEHL_V4F32(i3B, i3A);	\
		}	\
		else	\
		{	\
			i0AB = AKSIMD_MOVELH_V4F32(i0A, i0B);	\
			i1AB = AKSIMD_MOVELH_V4F32(i1A, i1B);	\
			i2AB = AKSIMD_MOVELH_V4F32(i2A, i2B);	\
			i3AB = AKSIMD_MOVELH_V4F32(i3A, i3B);	\
		} \
	}

#endif // AK_CPU_ARM_NEON

//This function will process three stages of the radix-4 FFT (stages 1, 4 and 16) , mostly all in-register.  
static void kf_bfly4_m1_s256_n1024_SIMD(
								   ak_fft_cpx * AK_RESTRICT Fout,
								   const ak_fft_cpx * Fin,
								   const ak_fft_cfg st
								   )
{
	//The twiddles are 1 or -1 in this case.  Reversed if we're doing an inverse fft.
	AKSIMD_DECLARE_V4F32( s_tw, -1, 1, -1, 1 );
	AKSIMD_V4F32 tw = s_tw;
	if (st->inverse)
		tw = AKSIMD_NEG_V4F32(tw);

	AKSIMD_V4F32 twSwap = AKSIMD_NEG_V4F32(tw);

	ADJUST_ALIGMENT;
	
	const ak_fft_cpx * AK_RESTRICT pInA = Fin;
	const ak_fft_cpx * AK_RESTRICT pInB = Fin + 64;

	AKSIMD_V4F32 i0AB;
	AKSIMD_V4F32 i1AB;
	AKSIMD_V4F32 i2AB;
	AKSIMD_V4F32 i3AB;

	//////////////////////////////////////////////////////////////////////////
	// First Pair of blocks
	GET_PAIRS;

	pInA = Fin + 128;
	pInB = pInA + 64;

	AKSIMD_V4F32 i1ABSwap = AKSIMD_SHUFFLE_BADC(i1AB);
	AKSIMD_V4F32 i3ABSwap = AKSIMD_SHUFFLE_BADC(i3AB);

	AKSIMD_V4F32 tmp = AKSIMD_ADD_V4F32(i0AB, i2AB);	//reused
	AKSIMD_V4F32 F0F4 = AKSIMD_ADD_V4F32(tmp, i1AB);
	F0F4 = AKSIMD_ADD_V4F32(F0F4, i3AB);

	AKSIMD_V4F32 F1F5 = AKSIMD_MADD_V4F32(i1ABSwap, twSwap, i0AB);
	F1F5 = AKSIMD_SUB_V4F32(F1F5, i2AB);
	F1F5 = AKSIMD_MADD_V4F32(i3ABSwap, tw, F1F5);

	AKSIMD_V4F32 F2F6 = AKSIMD_SUB_V4F32(tmp, i1AB);
	F2F6 = AKSIMD_SUB_V4F32(F2F6, i3AB);

	AKSIMD_V4F32 F3F7 = AKSIMD_MADD_V4F32(i1ABSwap, tw, i0AB);
	F3F7 = AKSIMD_SUB_V4F32(F3F7, i2AB);
	F3F7 = AKSIMD_MADD_V4F32(i3ABSwap, twSwap, F3F7); 

	//////////////////////////////////////////////////////////////////////////
	// Second Pair of blocks
	GET_PAIRS;

	i1ABSwap = AKSIMD_SHUFFLE_BADC(i1AB);
	i3ABSwap = AKSIMD_SHUFFLE_BADC(i3AB);

	tmp = AKSIMD_ADD_V4F32(i0AB, i2AB);	//reused
	AKSIMD_V4F32 F8F12 = AKSIMD_ADD_V4F32(tmp, i1AB);
	F8F12 = AKSIMD_ADD_V4F32(F8F12, i3AB);

	AKSIMD_V4F32 F9F13 = AKSIMD_MADD_V4F32(i1ABSwap, twSwap, i0AB);
	F9F13 = AKSIMD_SUB_V4F32(F9F13, i2AB);
	F9F13 = AKSIMD_MADD_V4F32(i3ABSwap, tw, F9F13);

	AKSIMD_V4F32 F10F14 = AKSIMD_SUB_V4F32(tmp, i1AB);
	F10F14 = AKSIMD_SUB_V4F32(F10F14, i3AB);

	AKSIMD_V4F32 F11F15 = AKSIMD_MADD_V4F32(i1ABSwap, tw, i0AB);
	F11F15 = AKSIMD_SUB_V4F32(F11F15, i2AB);
	F11F15 = AKSIMD_MADD_V4F32(i3ABSwap, twSwap, F11F15); 

	//////////////////////////////////////////////////////////////////////////
	// At this point we did 4 radix-4 ffts.  Now do the next combining butterfly stage (still radix 4).  This part is normally done by kf_bfly4.
	// Doing this saves one write and one read per complex on top of all the shuffles needed for both read and writes.

	const size_t m=4;
	const size_t m2=8;
	const size_t m3=12;

	AKSIMD_V4F32 vFoutM = AKSIMD_MOVEHL_V4F32( F1F5, F0F4 ); 
	AKSIMD_V4F32 vTw1 = AKSIMD_LOAD_V4F32(st->simdTwiddles);
	AKSIMD_V4F32 vScratch0 = AKSIMD_COMPLEXMUL( vFoutM, vTw1 );
	AKSIMD_V4F32 vFoutM3 = AKSIMD_MOVEHL_V4F32( F9F13, F8F12 ); 
	AKSIMD_V4F32 vTw3 = AKSIMD_LOAD_V4F32(st->simdTwiddles+4);
	AKSIMD_V4F32 vScratch1 = AKSIMD_COMPLEXMUL( vFoutM3, vTw3 );
	AKSIMD_V4F32 vScratch2 = AKSIMD_ADD_V4F32( vScratch0, vScratch1 );
	vScratch0 = AKSIMD_SUB_V4F32( vScratch0, vScratch1 );

	AKSIMD_V4F32 vFoutM2 = AKSIMD_MOVELH_V4F32( F8F12, F9F13 ); 	
	AKSIMD_V4F32 vTw2 = AKSIMD_LOAD_V4F32(st->simdTwiddles+2);
	vScratch1 = AKSIMD_COMPLEXMUL( vFoutM2, vTw2 );		

	AKSIMD_V4F32 vFout =  AKSIMD_MOVELH_V4F32( F0F4, F1F5 ); 
	AKSIMD_V4F32 vScratch3 = AKSIMD_SUB_V4F32( vFout, vScratch1 );
	vFout = AKSIMD_ADD_V4F32( vFout, vScratch1 );

	vFoutM2 = AKSIMD_SUB_V4F32( vFout, vScratch2 );
	AKSIMD_STORE_V4F32( (AkReal32*)&Fout[m2], vFoutM2 );
	vFout = AKSIMD_ADD_V4F32( vFout, vScratch2 );
	AKSIMD_STORE_V4F32( (AkReal32*)&Fout[0], vFout );

	vScratch0 = AKSIMD_SHUFFLE_BADC( vScratch0 );
	vScratch2 = AKSIMD_MUL_V4F32( vScratch0, twSwap );
	vScratch1 = AKSIMD_ADD_V4F32( vScratch3, vScratch2 );
	AKSIMD_STORE_V4F32( (AkReal32*)&Fout[m], vScratch1 );
	vScratch2 = AKSIMD_SUB_V4F32( vScratch3, vScratch2 );
	AKSIMD_STORE_V4F32( (AkReal32*)&Fout[m3], vScratch2 );

	Fout += 2;

	vFoutM = AKSIMD_MOVEHL_V4F32( F3F7, F2F6 ); 
	vTw1 = AKSIMD_LOAD_V4F32(st->simdTwiddles+6);
	vScratch0 = AKSIMD_COMPLEXMUL( vFoutM, vTw1 );
	vFoutM3 = AKSIMD_MOVEHL_V4F32( F11F15, F10F14 ); 
	vTw3 = AKSIMD_LOAD_V4F32(st->simdTwiddles+10);	
	vScratch1 = AKSIMD_COMPLEXMUL( vFoutM3, vTw3 );
	vScratch2 = AKSIMD_ADD_V4F32( vScratch0, vScratch1 );
	vScratch0 = AKSIMD_SUB_V4F32( vScratch0, vScratch1 );

	vFoutM2 = AKSIMD_MOVELH_V4F32( F10F14, F11F15 ); 	
	vTw2 = AKSIMD_LOAD_V4F32(st->simdTwiddles+8);
	vScratch1 = AKSIMD_COMPLEXMUL( vFoutM2, vTw2 );		

	vFout =  AKSIMD_MOVELH_V4F32( F2F6, F3F7 ); 
	vScratch3 = AKSIMD_SUB_V4F32( vFout, vScratch1 );
	vFout = AKSIMD_ADD_V4F32( vFout, vScratch1 );

	vFoutM2 = AKSIMD_SUB_V4F32( vFout, vScratch2 );
	AKSIMD_STORE_V4F32( (AkReal32*)&Fout[m2], vFoutM2 );
	vFout = AKSIMD_ADD_V4F32( vFout, vScratch2 );
	AKSIMD_STORE_V4F32( (AkReal32*)&Fout[0], vFout );

	vScratch0 = AKSIMD_SHUFFLE_BADC( vScratch0 );
	vScratch2 = AKSIMD_MUL_V4F32( vScratch0, twSwap );
	vScratch1 = AKSIMD_ADD_V4F32( vScratch3, vScratch2 );
	AKSIMD_STORE_V4F32( (AkReal32*)&Fout[m], vScratch1 );
	vScratch2 = AKSIMD_SUB_V4F32( vScratch3, vScratch2 );
	AKSIMD_STORE_V4F32( (AkReal32*)&Fout[m3], vScratch2 );
}

#elif defined AKSIMD_V2F32_SUPPORTED

#define DO_FIRST_STAGE_m1_x4(a,b,c,d) \
	vi0 = AKSIMD_LOAD_V2F32_OFFSET(pInA, 0*256*sizeof(ak_fft_cpx));	\
	vi1 = AKSIMD_LOAD_V2F32_OFFSET(pInA, 1*256*sizeof(ak_fft_cpx));	\
	vi2 = AKSIMD_LOAD_V2F32_OFFSET(pInA, 2*256*sizeof(ak_fft_cpx));	\
	vi3 = AKSIMD_LOAD_V2F32_OFFSET(pInA, 3*256*sizeof(ak_fft_cpx));	\
	\
	vTmp = AKSIMD_ADD_V2F32(vi0, vi2);	\
	AKSIMD_V2F32 vFout##a = AKSIMD_ADD_V2F32(vTmp, vi1);	\
	vFout##a = AKSIMD_ADD_V2F32(vFout##a, vi3);	\
\
	AKSIMD_V2F32 vFout##c = AKSIMD_SUB_V2F32(vTmp, vi1);	\
	vFout##c = AKSIMD_SUB_V2F32(vFout##c, vi3);	\
\
	vi1 = AKSIMD_SWAP_V2F32(vi1);	\
	vi3 = AKSIMD_SWAP_V2F32(vi3);	\
	vTmp = AKSIMD_SUB_V2F32(vi0, vi2);	\
	AKSIMD_V2F32 vFout##b = AKSIMD_MADD_V2F32(vi1, twSwap, vTmp);	\
	vFout##b = AKSIMD_MADD_V2F32(vi3, tw, vFout##b);	\
\
	AKSIMD_V2F32 vFout##d = AKSIMD_MADD_V2F32(vi1, tw, vTmp);	\
	vFout##d = AKSIMD_MADD_V2F32(vi3, twSwap, vFout##d);

#define DO_SECOND_STAGE_m4_x4(a,b,c,d) \
	vScratch0 = COMPLEX_MUL_WITH_TWIDDLE_REG((AkReal32*)tw1, vFout##b);	\
	vScratch1 = COMPLEX_MUL_WITH_TWIDDLE_REG((AkReal32*)tw3, vFout##d);	\
	vScratch2 = AKSIMD_ADD_V2F32( vScratch0, vScratch1 );	\
	vScratch0 = AKSIMD_SUB_V2F32( vScratch0, vScratch1 );	\
	vScratch1 = COMPLEX_MUL_WITH_TWIDDLE_REG((AkReal32*)tw2, vFout##c);	\
	\
	vScratch3 = AKSIMD_SUB_V2F32( vFout##a, vScratch1 );	\
	vFout##a = AKSIMD_ADD_V2F32( vFout##a, vScratch1 );	\
	\
	vFout##c = AKSIMD_SUB_V2F32( vFout##a, vScratch2 );	\
	AKSIMD_STORE_V2F32_OFFSET( (AkReal32*)&Fout[8], 0, vFout##c);	\
	vFout##a = AKSIMD_ADD_V2F32( vFout##a, vScratch2 );	\
	AKSIMD_STORE_V2F32_OFFSET( (AkReal32*)&Fout[0], 0, vFout##a );	\
	\
	vScratch1 = AKSIMD_SWAP_V2F32( vScratch0 );	\
	vScratch1 = AKSIMD_MUL_V2F32(vScratch1, twSwap);	\
	vFout##b = AKSIMD_ADD_V2F32( vScratch3, vScratch1 ) ;	\
	vFout##d = AKSIMD_SUB_V2F32( vScratch3, vScratch1 ) ;	\
	AKSIMD_STORE_V2F32_OFFSET( (AkReal32*)&Fout[4], 0, vFout##b );	\
	AKSIMD_STORE_V2F32_OFFSET( (AkReal32*)&Fout[12], 0, vFout##d );	\
	\
	tw1 += tw1Offset;	\
	tw2 += tw2Offset;	\
	tw3 += tw3Offset;	\
	Fout ++;


static void kf_bfly4_m1_s256_n1024_SIMD(ak_fft_cpx * AK_RESTRICT Fout,
										const ak_fft_cpx * Fin,
										const ak_fft_cfg st
										)
{
	//The twiddles are 1 or -1 in this case.  Reversed if we're doing an inverse fft.
	AKSIMD_V2F32 tw = {-1, 1};
	if (st->inverse)
		tw = AKSIMD_NEG_V2F32(tw);

	AKSIMD_V2F32 twSwap = AKSIMD_NEG_V2F32(tw);

	AKSIMD_V2F32 vi0;
	AKSIMD_V2F32 vi1;
	AKSIMD_V2F32 vi2;
	AKSIMD_V2F32 vi3;
	AKSIMD_V2F32 vTmp;

	// Block 0
	const ak_fft_cpx * AK_RESTRICT pInA = Fin;
	DO_FIRST_STAGE_m1_x4(0,1,2,3);

	// Block 1
	pInA += 64;
	DO_FIRST_STAGE_m1_x4(4,5,6,7);

	// Block 2
	pInA += 64;
	DO_FIRST_STAGE_m1_x4(8,9,10,11);

	// Block 3
	pInA += 64;
	DO_FIRST_STAGE_m1_x4(12,13,14,15);
	
	/////////////////////////////////////////////////////////////
	// Now do the second stage over the first stage

	ak_fft_cpx *tw1,*tw2,*tw3;
	tw3 = tw2 = tw1 = st->twiddles;
	const size_t tw1Offset = 64;
	const size_t tw2Offset = 64*2;
	const size_t tw3Offset = 64*3;

	AKSIMD_V2F32 vScratch0;
	AKSIMD_V2F32 vScratch1;
	AKSIMD_V2F32 vScratch2;
	AKSIMD_V2F32 vScratch3;

	DO_SECOND_STAGE_m4_x4(0, 4, 8, 12);
	DO_SECOND_STAGE_m4_x4(1, 5, 9, 13);
	DO_SECOND_STAGE_m4_x4(2, 6, 10, 14);
	DO_SECOND_STAGE_m4_x4(3, 7, 11, 15);
}
#endif



#ifdef USEALLBUTTERFLIES  
static void kf_bfly3(
         ak_fft_cpx * Fout,
         const size_t fstride,
         const ak_fft_cfg st,
         size_t m
         )
{
     size_t k=m;
     const size_t m2 = 2*m;
     ak_fft_cpx *tw1,*tw2;
     ak_fft_cpx scratch[5];
     ak_fft_cpx epi3;
     epi3 = st->twiddles[fstride*m];

     tw1=tw2=st->twiddles;

     do{
         C_FIXDIV(*Fout,3); C_FIXDIV(Fout[m],3); C_FIXDIV(Fout[m2],3);

         C_MUL(scratch[1],Fout[m] , *tw1);
         C_MUL(scratch[2],Fout[m2] , *tw2);

         C_ADD(scratch[3],scratch[1],scratch[2]);
         C_SUB(scratch[0],scratch[1],scratch[2]);
         tw1 += fstride;
         tw2 += fstride*2;

         Fout[m].r = Fout->r - HALF_OF(scratch[3].r);
         Fout[m].i = Fout->i - HALF_OF(scratch[3].i);

         C_MULBYSCALAR( scratch[0] , epi3.i );

         C_ADDTO(*Fout,scratch[3]);

         Fout[m2].r = Fout[m].r + scratch[0].i;
         Fout[m2].i = Fout[m].i - scratch[0].r;

         Fout[m].r -= scratch[0].i;
         Fout[m].i += scratch[0].r;

         ++Fout;
     }while(--k);
}

static void kf_bfly5(
        ak_fft_cpx * Fout,
        const size_t fstride,
        const ak_fft_cfg st,
        int m
        )
{
    ak_fft_cpx *Fout0,*Fout1,*Fout2,*Fout3,*Fout4;
    int u;
    ak_fft_cpx scratch[13];
    ak_fft_cpx * twiddles = st->twiddles;
    ak_fft_cpx *tw;
    ak_fft_cpx ya,yb;
    ya = twiddles[fstride*m];
    yb = twiddles[fstride*2*m];

    Fout0=Fout;
    Fout1=Fout0+m;
    Fout2=Fout0+2*m;
    Fout3=Fout0+3*m;
    Fout4=Fout0+4*m;

    tw=st->twiddles;
    for ( u=0; u<m; ++u ) {
        C_FIXDIV( *Fout0,5); C_FIXDIV( *Fout1,5); C_FIXDIV( *Fout2,5); C_FIXDIV( *Fout3,5); C_FIXDIV( *Fout4,5);
        scratch[0] = *Fout0;

        C_MUL(scratch[1] ,*Fout1, tw[u*fstride]);
        C_MUL(scratch[2] ,*Fout2, tw[2*u*fstride]);
        C_MUL(scratch[3] ,*Fout3, tw[3*u*fstride]);
        C_MUL(scratch[4] ,*Fout4, tw[4*u*fstride]);

        C_ADD( scratch[7],scratch[1],scratch[4]);
        C_SUB( scratch[10],scratch[1],scratch[4]);
        C_ADD( scratch[8],scratch[2],scratch[3]);
        C_SUB( scratch[9],scratch[2],scratch[3]);

        Fout0->r += scratch[7].r + scratch[8].r;
        Fout0->i += scratch[7].i + scratch[8].i;

        scratch[5].r = scratch[0].r + S_MUL(scratch[7].r,ya.r) + S_MUL(scratch[8].r,yb.r);
        scratch[5].i = scratch[0].i + S_MUL(scratch[7].i,ya.r) + S_MUL(scratch[8].i,yb.r);

        scratch[6].r =  S_MUL(scratch[10].i,ya.i) + S_MUL(scratch[9].i,yb.i);
        scratch[6].i = -S_MUL(scratch[10].r,ya.i) - S_MUL(scratch[9].r,yb.i);

        C_SUB(*Fout1,scratch[5],scratch[6]);
        C_ADD(*Fout4,scratch[5],scratch[6]);

        scratch[11].r = scratch[0].r + S_MUL(scratch[7].r,yb.r) + S_MUL(scratch[8].r,ya.r);
        scratch[11].i = scratch[0].i + S_MUL(scratch[7].i,yb.r) + S_MUL(scratch[8].i,ya.r);
        scratch[12].r = - S_MUL(scratch[10].i,yb.i) + S_MUL(scratch[9].i,ya.i);
        scratch[12].i = S_MUL(scratch[10].r,yb.i) - S_MUL(scratch[9].r,ya.i);

        C_ADD(*Fout2,scratch[11],scratch[12]);
        C_SUB(*Fout3,scratch[11],scratch[12]);

        ++Fout0;++Fout1;++Fout2;++Fout3;++Fout4;
    }
}

/* perform the butterfly for one stage of a mixed radix FFT */
static void kf_bfly_generic(
        ak_fft_cpx * Fout,
        const size_t fstride,
        const ak_fft_cfg st,
        int m,
        int p
        )
{
    int u,k,q1,q;
    ak_fft_cpx * twiddles = st->twiddles;
    ak_fft_cpx t;
    int Norig = st->nfft;
	
	// Not quite generic but supports any FFT size between 256 and 8192 that is aligned on 256 frames boundary. 
	// This requirement is generic enough, otherwise consider using alloca 
	/* Matlab code that computes this:
	ws = [256:256:8192]./2;
	maximum = 0;
	for ind = 1:length(ws)
		maxprime = max(factor(ws(ind)));
		if (maxprime > maximum)
			maximum = maxprime;
		end
	end
	*/
	ak_fft_cpx scratchbuf[31];

    for ( u=0; u<m; ++u ) {
        k=u;
        for ( q1=0 ; q1<p ; ++q1 ) {
            scratchbuf[q1] = Fout[ k  ];
            C_FIXDIV(scratchbuf[q1],p);
            k += m;
        }

        k=u;
        for ( q1=0 ; q1<p ; ++q1 ) {
            int twidx=0;
            Fout[ k ] = scratchbuf[0];
            for (q=1;q<p;++q ) {
                twidx += (int)fstride * k;
                if (twidx>=Norig) twidx-=Norig;
                C_MUL(t,scratchbuf[q] , twiddles[twidx] );
                C_ADDTO( Fout[ k ] ,t);
            }
            k += m;
        }
    }
}

static inline void kf_bfly4_m1(
	 ak_fft_cpx * AK_RESTRICT Fout,
	 const ak_fft_cfg st
	 )
{
    const ak_fft_cpx * AK_RESTRICT twiddles = st->twiddles;
    ak_fft_cpx t;
    int Norig = st->nfft - 1;
	
	ak_fft_cpx i0 = Fout[0];  
	ak_fft_cpx i1 = Fout[1];
	ak_fft_cpx i2 = Fout[2];
	ak_fft_cpx i3 = Fout[3];

	int twidx=0;
	Fout[ 0 ] = i0;
	Fout[ 0 ].r += i1.r;  
	Fout[ 0 ].i += i1.i;

	Fout[ 0 ].r += i2.r;  
	Fout[ 0 ].i += i2.i;

	Fout[ 0 ].r += i3.r;  
	Fout[ 0 ].i += i3.i;

	Fout[ 1 ] = i0;
	
	twidx = 256 & Norig;

	t.r = i1.r*twiddles[twidx].r - i1.i*twiddles[twidx].i;
	t.i = i1.r*twiddles[twidx].i + i1.i*twiddles[twidx].r; 
	Fout[ 1 ].r += t.r;  Fout[ 1 ].i += t.i;

	twidx = 512 & Norig;

	t.r = i2.r*twiddles[twidx].r - i2.i*twiddles[twidx].i;
	t.i = i2.r*twiddles[twidx].i + i2.i*twiddles[twidx].r; 
	Fout[ 1 ].r += t.r;  Fout[ 1 ].i += t.i;

	twidx = 768 & Norig;

	t.r = i3.r*twiddles[twidx].r - i3.i*twiddles[twidx].i;
	t.i = i3.r*twiddles[twidx].i + i3.i*twiddles[twidx].r; 
	Fout[ 1 ].r += t.r;  Fout[ 1 ].i += t.i;

	Fout[ 2 ] = i0;

	twidx = 256*2 & Norig;
	t.r = i1.r*twiddles[twidx].r - i1.i*twiddles[twidx].i;
	t.i = i1.r*twiddles[twidx].i + i1.i*twiddles[twidx].r; 
	Fout[ 2 ].r += t.r;  Fout[ 2 ].i += t.i;

	twidx = (twidx + 256*2) & Norig;

	t.r = i2.r*twiddles[twidx].r - i2.i*twiddles[twidx].i;
	t.i = i2.r*twiddles[twidx].i + i2.i*twiddles[twidx].r; 
	Fout[ 2 ].r += t.r;  Fout[ 2 ].i += t.i;

	twidx = (twidx + 256*2) & Norig;

	t.r = i3.r*twiddles[twidx].r - i3.i*twiddles[twidx].i;
	t.i = i3.r*twiddles[twidx].i + i3.i*twiddles[twidx].r; 
	Fout[ 2 ].r += t.r;  Fout[ 2 ].i += t.i;

	Fout[ 3 ] = i0;

	twidx = 256*3 & Norig;

	t.r = i1.r*twiddles[twidx].r - i1.i*twiddles[twidx].i;
	t.i = i1.r*twiddles[twidx].i + i1.i*twiddles[twidx].r; 
	Fout[ 3 ].r += t.r;  Fout[ 3 ].i += t.i;

	twidx = (twidx + 256*3) & Norig;

	t.r = i2.r*twiddles[twidx].r - i2.i*twiddles[twidx].i;
	t.i = i2.r*twiddles[twidx].i + i2.i*twiddles[twidx].r; 
	Fout[ 3 ].r += t.r;  Fout[ 3 ].i += t.i;

	twidx = (twidx + 256*3) & Norig;

	t.r = i3.r*twiddles[twidx].r - i3.i*twiddles[twidx].i;
	t.i = i3.r*twiddles[twidx].i + i3.i*twiddles[twidx].r; 
	Fout[ 3 ].r += t.r;  Fout[ 3 ].i += t.i;
}

#endif // #ifdef USEALLBUTTERFLIES 

static void kf_work(
        ak_fft_cpx * Fout,
        const ak_fft_cpx * f,
        const size_t fstride,
        int in_stride,
        int * factors,
        const ak_fft_cfg st
        )
{
    ak_fft_cpx * Fout_beg=Fout;
    const int p=*factors++; /* the radix  */
    const int m=*factors++; /* stage's fft length/p */
    const ak_fft_cpx * Fout_end = Fout + p*m;


    if (m==1) {
		const unsigned int uCombStride = (unsigned int) (fstride*in_stride);
        do{
            *Fout = *f;
            f += uCombStride;
        }while(++Fout != Fout_end );
    }
	else
	{
		// Check if there is an optimized routine for this particular case.
#if defined AKSIMD_V2F32_SUPPORTED || defined AKSIMD_V4F32_SUPPORTED
		if(st->nfft == 1024 && *(factors+1) == 1 && *factors == 4 && fstride == 64 && in_stride == 1) //If next stage is stage 1 and we're doing a radix-4
		{
			//This function will process 4 blocks of the next stage (one block has 4 complex, so 16 complex total) and *this* stage (normally done through kf_bfly4
			kf_bfly4_m1_s256_n1024_SIMD(Fout, f, st);
			return;
		}
		else
#endif
#ifdef USEALLBUTTERFLIES
		if (*(factors+1) == 1 && *factors == 2 && p == 4) //If next stage is stage 1 and we're doing a radix-2
		{
			kf_bfly2_m1x4(Fout, f, fstride);
		}
		else
#endif
		{
			do{
				// recursive call:
				// DFT of size m*p performed by doing
				// p instances of smaller DFTs of size m, 
				// each one takes a decimated version of the input
				kf_work( Fout , f, fstride*p, in_stride, factors,st);
				f += fstride*in_stride;
			}while( (Fout += m) != Fout_end );
		}
    }

    Fout=Fout_beg;

    // recombine the p smaller DFTs 
	// Sound engine block size restrictions are such that other butterfly are not required	
#ifdef USEALLBUTTERFLIES 
    switch (p) {
        case 2: 
			if (m == 1)
				kf_bfly2_m1(Fout);
			else
				kf_bfly2(Fout,fstride,st,m); 
			break;
        case 3: kf_bfly3(Fout,fstride,st,m); break; 
        case 4: 
 			if (m == 1 && fstride == 256)
 			{
				AKASSERT(st->nfft == 1024);
 				//if (st->nfft == 1024)
 					kf_bfly4_m1_n1024(Fout, st);
 				/*else
 					kf_bfly4_m1(Fout,st);	//m = 1; p = 4; stride = 256;*/
 			}
 			else
#if defined AKSIMD_V4F32_SUPPORTED
			{
				// SIMD optimizations introduced additional constraints
				// use generic butterfly if conditions are not met 
				if ( m % 2 == 0 )
					kf_bfly4(Fout,fstride,st,m);
				else
					kf_bfly_generic(Fout,fstride,st,m,p);
			}
#else
			kf_bfly4(Fout,fstride,st,m);
#endif // USEALLBUTTERFLIES
			break;
        case 5: kf_bfly5(Fout,fstride,st,m); break; 
        default: kf_bfly_generic(Fout,fstride,st,m,p); break;
    }
#else
	AKASSERT( p == 4 ); 
	kf_bfly4(Fout,fstride,st,m);
#endif 
}

void ak_fft_stride(ak_fft_cfg st,const ak_fft_cpx *fin,ak_fft_cpx *fout,int in_stride)
{
	AKASSERT( fin != fout );
    kf_work( fout, fin, 1,in_stride, st->factors,st );
}

//Number of twiddles that are needed for the optimized first stage.  For the bfly4 x 4 we need 12.  Plus 1 to align properly.
#define SIMD_TWIDDLES_SIZE (12+1)
void ak_fft(ak_fft_cfg cfg,const ak_fft_cpx *fin,ak_fft_cpx *fout)
{
#ifdef AKSIMD_V4F32_SUPPORTED
	AK_ALIGN_SIMD(ak_fft_cpx simdTw[SIMD_TWIDDLES_SIZE]);
	if (cfg->nfft == 1024)
	{
		//This is the only case we are going to use this.  (for now!)
		cfg->simdTwiddles = simdTw;
		AKASSERT(((AkUIntPtr)cfg->simdTwiddles & AK_SIMD_ALIGNMENT-1) == 0);
		cfg->simdTwiddles[0] = cfg->twiddles[0];
		cfg->simdTwiddles[1] = cfg->twiddles[64];
		cfg->simdTwiddles[2] = cfg->twiddles[0];
		cfg->simdTwiddles[3] = cfg->twiddles[64*2];
		cfg->simdTwiddles[4] = cfg->twiddles[0];
		cfg->simdTwiddles[5] = cfg->twiddles[64*3];
		cfg->simdTwiddles[6] = cfg->twiddles[0+64*2];
		cfg->simdTwiddles[7] = cfg->twiddles[64+64*2];
		cfg->simdTwiddles[8] = cfg->twiddles[0+64*4];
		cfg->simdTwiddles[9] = cfg->twiddles[64*2+64*4];
		cfg->simdTwiddles[10] = cfg->twiddles[0+64*6];
		cfg->simdTwiddles[11] = cfg->twiddles[64*3+64*6];
	}
#endif

	ak_fft_stride(cfg,fin,fout,1);
}

#endif // #ifdef __PPU__

/*  facbuf is populated by p1,m1,p2,m2, ...
    where 
    p[i] * m[i] = m[i-1]
    m0 = n                  */
static 
void kf_factor(int n,int * facbuf)
{
    int p=4;
    double floor_sqrt;
    floor_sqrt = floor( sqrt((double)n) );

    /*factor out powers of 4, powers of 2, then any remaining primes */
    do {
        while (n % p) {
            switch (p) {
                case 4: p = 2; break;
                case 2: p = 3; break;
                default: p += 2; break;
            }
            if (p > floor_sqrt)
                p = n;          /* no more factors, skip to end */
        }
        n /= p;
        *facbuf++ = p;
        *facbuf++ = n;
    } while (n > 1);
}


/*
 *
 * User-callable function to allocate all necessary storage space for the fft.
 *
 * The return value is a contiguous block of memory, allocated with malloc.  As such,
 * It can be freed with free(), rather than a ak_fft-specific function.
 * */

ak_fft_cfg ak_fft_alloc(int nfft,int inverse_fft,void * mem,size_t * lenmem )
{
    ak_fft_cfg st=NULL;
    size_t memneeded = sizeof(struct ak_fft_state)
        + sizeof(ak_fft_cpx)*(nfft-1); /* twiddle factors*/
	memneeded = AKSIMD_ALIGNSIZE(memneeded); // alignment for SIMD

	AKASSERT( lenmem!=NULL );
	if (mem != NULL && *lenmem >= memneeded)
		st = (ak_fft_cfg)mem;
	*lenmem = memneeded;
    if (st) 
	{
        int i;
        st->nfft=nfft;
        st->inverse = inverse_fft;
		st->twiddles = (ak_fft_cpx*)((char *)mem + sizeof(struct ak_fft_state));
		AKASSERT( ((AkUIntPtr)st->twiddles % AK_SIMD_ALIGNMENT) == 0 );
		
		if (st->inverse)
		{
			for (i=0;i<nfft;++i) 
			{
				const double pi = 3.14159265358979323846264338327;
				double phase = 2*pi*i / nfft;
				kf_cexp(st->twiddles+i, phase );
			}
		}
		else
		{
			for (i=0;i<nfft;++i) 
			{ 
				const double pi = 3.14159265358979323846264338327;
				double phase = -2*pi*i / nfft;
				kf_cexp(st->twiddles+i, phase );
			}
		}
        kf_factor(nfft,st->factors);
    }
    return st;
}  

int ak_fft_next_fast_size(int n)
{
    while(1) {
        int m=n;
        while ( (m%2) == 0 ) m/=2;
        while ( (m%3) == 0 ) m/=3;
        while ( (m%5) == 0 ) m/=5;
        if (m<=1)
            break; /* n is completely factorable by twos, threes, and fives */
        n++;
    }
    return n;
}

} // namespace BUTTERFLYSET_NAMESPACE

} // namespace DSP
