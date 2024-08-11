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



#include "ak_fftr.h"
#include "_ak_fft_guts.h"
#include <AK/Tools/Common/AkAssert.h>
#include <AK/SoundEngine/Common/AkSimd.h>

#ifdef __SPU__

#include <AK/Plugin/PluginServices/PS3/MultiCoreServices.h>

// Important Note:
// ak_fftr_state is already allocated in a single memory block, but the structure (and child structure also)
// holds addresses to different points within this monolithic memory block. 
// Because this structure is a read only parameter on the SPU (i.e. not sent back to main memory), we can fix the offsets
// of the pointers to point to local storage using the function below

// Final layout of memory block looks like this:
//[ak_fftr_state] -> sizeof(ak_fftr_state)
//[ak_fft_state] -> sizeof(ak_fft_state)
//[ak_fft_state->twiddles] -> AK_ALIGN_SIZE_FOR_DMA( sizeof(ak_fft_cpx)*(nfft-1) )
//[ak_fftr_state->tmpbuf] -> sizeof(ak_fft_cpx)*nfft
//[ak_fftr_state->super_twiddles] -> sizeof(ak_fft_cpx)*nfft/2

void ak_fftr_state::FixPointersToLS( )
{
	AkUInt8 * pLSAddress = (AkUInt8 *)this;
	pLSAddress += sizeof(ak_fftr_state); // skip over itself
	substate = (ak_fft_cfg)pLSAddress;
	pLSAddress += sizeof(ak_fft_state); // skip over substate
	substate->twiddles = (ak_fft_cpx *) pLSAddress;
	pLSAddress += AK_ALIGN_SIZE_FOR_DMA( sizeof(ak_fft_cpx)*(substate->nfft-1) ); // skip over substate twiddles
	tmpbuf = (ak_fft_cpx *) pLSAddress;
	pLSAddress += sizeof(ak_fft_cpx)*substate->nfft; // skip over tmpbuf
	super_twiddles = (ak_fft_cpx *) pLSAddress;
	// pLSAddress += sizeof(ak_fft_cpx)*substate->nfft/2; // skip over super_twiddles
}
#endif // __SPU__

namespace DSP
{

// Avoid link collisions between compiled version with or without USEALLBUTTERFLIES
namespace BUTTERFLYSET_NAMESPACE
{

ak_fftr_cfg ak_fftr_alloc(int nfft,int inverse_fft,void * mem,size_t * lenmem)
{
    int i;
    ak_fftr_cfg st = NULL;
    size_t subsize, memneeded;

    if (nfft & 1) {
        AKASSERT(!"Real FFT optimization must be even.");
        return NULL;
    }
    nfft >>= 1;

	RESOLVEUSEALLBUTTERFLIES( ak_fft_alloc (nfft, inverse_fft, NULL, &subsize) );
    memneeded = sizeof(struct ak_fftr_state) + subsize + sizeof(ak_fft_cpx) * ( nfft * 3 / 2);

	AKASSERT( lenmem != NULL );
	if (*lenmem >= memneeded)
		st = (ak_fftr_cfg) mem;
	*lenmem = memneeded;
    if (!st)
        return NULL;

    st->substate = (ak_fft_cfg) (st + 1); /*just beyond ak_fftr_state struct */
    st->tmpbuf = (ak_fft_cpx *) (((char *) st->substate) + subsize);
	AKASSERT( (AkUIntPtr)st->tmpbuf % AK_SIMD_ALIGNMENT == 0 ); // SIMD optimizations require this
    st->super_twiddles = st->tmpbuf + nfft;
	AKASSERT( (AkUIntPtr)st->super_twiddles % AK_SIMD_ALIGNMENT == 0 ); // SIMD optimizations require this
	RESOLVEUSEALLBUTTERFLIES( ak_fft_alloc(nfft, inverse_fft, st->substate, &subsize) );

	if ( inverse_fft )
	{
		for (i = 0; i < nfft/2; ++i) 
		{
			double phase = 3.14159265358979323846264338327 * ((double) (i+1) / nfft + .5);
			kf_cexp (st->super_twiddles+i,phase);
		}
	}
	else
	{
		for (i = 0; i < nfft/2; ++i) 
		{
			double phase = -3.14159265358979323846264338327 * ((double) (i+1) / nfft + .5);
			kf_cexp (st->super_twiddles+i,phase);
		}
	}
    return st;
}

#ifndef __PPU__

#ifdef AKSIMD_V4F32_SUPPORTED

void ak_fftr(ak_fftr_cfg st,const ak_fft_scalar *timedata,ak_fft_cpx * AK_RESTRICT freqdata)
{
    /* input buffer timedata is stored row-wise */

	AKASSERT( st->substate->inverse == 0 );

    const int ncfft = st->substate->nfft;

    /*perform the parallel fft of two real signals packed in real,imag*/
	RESOLVEUSEALLBUTTERFLIES( ak_fft( st->substate , (const ak_fft_cpx*)timedata, st->tmpbuf ) );
    /* The real part of the DC element of the frequency spectrum in st->tmpbuf
     * contains the sum of the even-numbered elements of the input time sequence
     * The imag part is the sum of the odd-numbered elements
     *
     * The sum of tdc.r and tdc.i is the sum of the input time sequence. 
     *      yielding DC of input time sequence
     * The difference of tdc.r - tdc.i is the sum of the input (dot product) [1,-1,1,-1... 
     *      yielding Nyquist bin of input time sequence
     */
 
	// Each loop iterations handles 2 complex numbers
	// Reading into data forward and backward simulataneously
	AKASSERT( (ncfft % 4) == 0 );
	AKASSERT( ((AkUIntPtr)st->tmpbuf % AK_SIMD_ALIGNMENT) == 0 );
	AKASSERT( ((AkUIntPtr)st->super_twiddles % AK_SIMD_ALIGNMENT) == 0 );
	ak_fft_cpx * AK_RESTRICT tmpBuf = (ak_fft_cpx * AK_RESTRICT)st->tmpbuf;
	ak_fft_cpx * AK_RESTRICT twiddles = (ak_fft_cpx * AK_RESTRICT)st->super_twiddles;

	const unsigned int uHalfSize = ncfft/2;
	static const AKSIMD_DECLARE_V4F32( vImagSignFlip, 1.f, -1.f, 1.f, -1.f );
	static const AKSIMD_DECLARE_V4F32( vHalf, 0.5f, 0.5f, 0.5f, 0.5f ); 
    for ( unsigned int k=1;k <= uHalfSize; k+=2 ) 
	{
		AKSIMD_V4F32 vFpk = AKSIMD_LOADU_V4F32( (AkReal32*)&tmpBuf[k] );
		AKSIMD_V4F32 vFpnk = AKSIMD_LOAD_V4F32( (AkReal32*)&tmpBuf[ncfft-(k+1)] );

		vFpnk = AKSIMD_SHUFFLE_CDAB( vFpnk );
		vFpnk = AKSIMD_MUL_V4F32( vFpnk, vImagSignFlip );

		AKSIMD_V4F32 vF1k = AKSIMD_ADD_V4F32( vFpk, vFpnk );
		AKSIMD_V4F32 vF2k = AKSIMD_SUB_V4F32( vFpk, vFpnk );
		AKSIMD_V4F32 vTwiddles = AKSIMD_LOAD_V4F32( (AkReal32*)&twiddles[k-1] );
		AKSIMD_V4F32 vTw = AKSIMD_COMPLEXMUL( vF2k, vTwiddles );

		AKSIMD_V4F32 vOutk = AKSIMD_ADD_V4F32( vF1k, vTw );
		vOutk = AKSIMD_MUL_V4F32( vOutk, vHalf );
		AKSIMD_STOREU_V4F32( (AkReal32*)&freqdata[k], vOutk );
		AKSIMD_V4F32 vOutkn = AKSIMD_SUB_V4F32( vF1k, vTw );
		vOutkn = AKSIMD_MUL_V4F32( vOutkn, vImagSignFlip );
		vOutkn = AKSIMD_MUL_V4F32( vOutkn, vHalf );
		vOutkn = AKSIMD_SHUFFLE_CDAB( vOutkn );
		AKSIMD_STORE_V4F32( (AkReal32*)&freqdata[ncfft-(k+1)], vOutkn );
    }

	// Deal with special frequency points (DC and Nyquist)
	ak_fft_cpx tdc = tmpBuf[0];
    freqdata[0].r = tdc.r + tdc.i;
    freqdata[ncfft].r = tdc.r - tdc.i;
    freqdata[ncfft].i = freqdata[0].i = 0;
}

#else // AKSIMD_V4F32_SUPPORTED

// Original routine
void ak_fftr(ak_fftr_cfg st,const ak_fft_scalar *timedata,ak_fft_cpx *freqdata)
{
    /* input buffer timedata is stored row-wise */
    int k,ncfft;
    ak_fft_cpx fpnk,fpk,f1k,f2k,tw,tdc;

	AKASSERT( st->substate->inverse == 0 );

    ncfft = st->substate->nfft;

    /*perform the parallel fft of two real signals packed in real,imag*/
    ak_fft( st->substate , (const ak_fft_cpx*)timedata, st->tmpbuf );
    /* The real part of the DC element of the frequency spectrum in st->tmpbuf
     * contains the sum of the even-numbered elements of the input time sequence
     * The imag part is the sum of the odd-numbered elements
     *
     * The sum of tdc.r and tdc.i is the sum of the input time sequence. 
     *      yielding DC of input time sequence
     * The difference of tdc.r - tdc.i is the sum of the input (dot product) [1,-1,1,-1... 
     *      yielding Nyquist bin of input time sequence
     */
 
    tdc.r = st->tmpbuf[0].r;
    tdc.i = st->tmpbuf[0].i;
    C_FIXDIV(tdc,2);
    CHECK_OVERFLOW_OP(tdc.r ,+, tdc.i);
    CHECK_OVERFLOW_OP(tdc.r ,-, tdc.i);
    freqdata[0].r = tdc.r + tdc.i;
    freqdata[ncfft].r = tdc.r - tdc.i;
    freqdata[ncfft].i = freqdata[0].i = 0;

    for ( k=1;k <= ncfft/2 ; ++k ) {
        fpk    = st->tmpbuf[k]; 
        fpnk.r =   st->tmpbuf[ncfft-k].r;
        fpnk.i = - st->tmpbuf[ncfft-k].i;
        C_FIXDIV(fpk,2);
        C_FIXDIV(fpnk,2);

        C_ADD( f1k, fpk , fpnk );
        C_SUB( f2k, fpk , fpnk );
        C_MUL( tw , f2k , st->super_twiddles[k-1]);

        freqdata[k].r = HALF_OF(f1k.r + tw.r);
        freqdata[k].i = HALF_OF(f1k.i + tw.i);
        freqdata[ncfft-k].r = HALF_OF(f1k.r - tw.r);
        freqdata[ncfft-k].i = HALF_OF(tw.i - f1k.i);
    }
}

#endif // AKSIMD_V4F32_SUPPORTED

#ifdef AKSIMD_V4F32_SUPPORTED

void ak_fftri(ak_fftr_cfg st,const ak_fft_cpx *freqdata,ak_fft_scalar *timedata)
{
    /* input buffer timedata is stored row-wise */
	AKASSERT( st->substate->inverse != 0 );

    const int ncfft = st->substate->nfft;

	// Each loop iterations handles 2 complex numbers
	// Reading into data forward and backward simulataneously
	AKASSERT( (ncfft % 4) == 0 );
	AKASSERT( ((AkUIntPtr)st->tmpbuf % AK_SIMD_ALIGNMENT) == 0 );
	AKASSERT( ((AkUIntPtr)st->super_twiddles % AK_SIMD_ALIGNMENT) == 0 );
	ak_fft_cpx * AK_RESTRICT tmpBuf = (ak_fft_cpx * AK_RESTRICT)st->tmpbuf;
	ak_fft_cpx * AK_RESTRICT twiddles = (ak_fft_cpx * AK_RESTRICT)st->super_twiddles;

    tmpBuf[0].r = freqdata[0].r + freqdata[ncfft].r;
    tmpBuf[0].i = freqdata[0].r - freqdata[ncfft].r;

	const unsigned int uHalfSize = ncfft/2;
	static const AKSIMD_DECLARE_V4F32( vImagSignFlip, 1.f, -1.f, 1.f, -1.f );
    for ( unsigned int k = 1; k <= uHalfSize; k+=2) 
	{
      	AKSIMD_V4F32 vFk = AKSIMD_LOADU_V4F32( (AkReal32*)&freqdata[k] );
		AKSIMD_V4F32 vFnkc = AKSIMD_LOAD_V4F32( (AkReal32*)&freqdata[ncfft-(k+1)] );
		vFnkc = AKSIMD_SHUFFLE_CDAB( vFnkc );
		vFnkc = AKSIMD_MUL_V4F32( vFnkc, vImagSignFlip );		

      	AKSIMD_V4F32 vFek = AKSIMD_ADD_V4F32( vFk, vFnkc );
      	AKSIMD_V4F32 vTmp = AKSIMD_SUB_V4F32( vFk, vFnkc );
		AKSIMD_V4F32 vTwiddles = AKSIMD_LOAD_V4F32( (AkReal32*)&twiddles[k-1] );
		AKSIMD_V4F32 vFok = AKSIMD_COMPLEXMUL( vTmp, vTwiddles );
		AKSIMD_V4F32 vOutk = AKSIMD_ADD_V4F32( vFek, vFok );
		AKSIMD_STOREU_V4F32( (AkReal32*)&tmpBuf[k], vOutk );
		AKSIMD_V4F32 vOutkn = AKSIMD_SUB_V4F32( vFek, vFok );
		vOutkn = AKSIMD_MUL_V4F32( vOutkn, vImagSignFlip );
		vOutkn = AKSIMD_SHUFFLE_CDAB( vOutkn );
		AKSIMD_STORE_V4F32( (AkReal32*)&tmpBuf[ncfft-(k+1)], vOutkn );        
    }

	RESOLVEUSEALLBUTTERFLIES( ak_fft (st->substate, st->tmpbuf, (ak_fft_cpx *) timedata) );
}

#else // AKSIMD_V4F32_SUPPORTED

// Original routine
void ak_fftri(ak_fftr_cfg st,const ak_fft_cpx *freqdata,ak_fft_scalar *timedata)
{
    /* input buffer timedata is stored row-wise */
    int k, ncfft;

	AKASSERT( st->substate->inverse != 0 );

    ncfft = st->substate->nfft;

    st->tmpbuf[0].r = freqdata[0].r + freqdata[ncfft].r;
    st->tmpbuf[0].i = freqdata[0].r - freqdata[ncfft].r;
    C_FIXDIV(st->tmpbuf[0],2);

    for (k = 1; k <= ncfft / 2; ++k) {
        ak_fft_cpx fk, fnkc, fek, fok, tmp;
        fk = freqdata[k];
        fnkc.r = freqdata[ncfft - k].r;
        fnkc.i = -freqdata[ncfft - k].i;
        C_FIXDIV( fk , 2 );
        C_FIXDIV( fnkc , 2 );

        C_ADD (fek, fk, fnkc);
        C_SUB (tmp, fk, fnkc);
        C_MUL (fok, tmp, st->super_twiddles[k-1]);
        C_ADD (st->tmpbuf[k],     fek, fok);
        C_SUB (st->tmpbuf[ncfft - k], fek, fok);
        st->tmpbuf[ncfft - k].i *= -1;
    }
    ak_fft (st->substate, st->tmpbuf, (ak_fft_cpx *) timedata);
}

#endif // AKSIMD_V4F32_SUPPORTED

#endif // __PPU__

} // namespace BUTTERFLYSET_NAMESPACE

} // namespace DSP
