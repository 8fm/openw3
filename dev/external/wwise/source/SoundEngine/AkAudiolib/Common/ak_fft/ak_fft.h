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

#ifndef AK_FFT_H
#define AK_FFT_H

#include <math.h>
#include "SPUInline.h"

//#ifdef __cplusplus
//extern "C" {
//#endif

/*
 ATTENTION!
 If you would like a :
 -- a utility that will handle the caching of fft objects
 -- real-only (no imaginary time component ) FFT
 -- a multi-dimensional FFT
 -- a command-line utility to perform ffts
 -- a command-line utility to perform fast-convolution filtering

 Then see kfc.h ak_fftr.h ak_fftnd.h fftutil.c ak_fastfir.c
  in the tools/ directory.
*/


#ifdef FIXED_POINT
#include <sys/types.h>	
# if (FIXED_POINT == 32)
#  define ak_fft_scalar int32_t
# else	
#  define ak_fft_scalar int16_t
# endif
#else
# ifndef ak_fft_scalar
/*  default is float */
#   define ak_fft_scalar float
# endif
#endif

typedef struct {
    ak_fft_scalar r;
    ak_fft_scalar i;
}ak_fft_cpx;

typedef struct ak_fft_state* ak_fft_cfg;

namespace DSP
{

// Avoid link collisions between compiled version with or without USEALLBUTTERFLIES
#ifdef USEALLBUTTERFLIES
#define BUTTERFLYSET_NAMESPACE AkFFTAllButterflies
#ifndef RESOLVEUSEALLBUTTERFLIES
#define RESOLVEUSEALLBUTTERFLIES(__funccall__) AkFFTAllButterflies::__funccall__
#endif
#else
#define BUTTERFLYSET_NAMESPACE AkFFTSubsetButterflies
#ifndef RESOLVEUSEALLBUTTERFLIES
#define RESOLVEUSEALLBUTTERFLIES(__funccall__) AkFFTSubsetButterflies::__funccall__
#endif
#endif

namespace BUTTERFLYSET_NAMESPACE
{
/* 
 *  ak_fft_alloc
 *  
 *  Initialize a FFT (or IFFT) algorithm's cfg/state buffer.
 *
 *  typical usage:      ak_fft_cfg mycfg=ak_fft_alloc(1024,0,NULL,NULL);
 *
 *  The return value from fft_alloc is a cfg buffer used internally
 *  by the fft routine or NULL.
 *
 *  If lenmem is NULL, then ak_fft_alloc will allocate a cfg buffer using malloc.
 *  The returned value should be free()d when done to avoid memory leaks.
 *  
 *  The state can be placed in a user supplied buffer 'mem':
 *  If lenmem is not NULL and mem is not NULL and *lenmem is large enough,
 *      then the function places the cfg in mem and the size used in *lenmem
 *      and returns mem.
 *  
 *  If lenmem is not NULL and ( mem is NULL or *lenmem is not large enough),
 *      then the function returns NULL and places the minimum cfg 
 *      buffer size in *lenmem.
 * */

SPUStatic ak_fft_cfg ak_fft_alloc(int nfft,int inverse_fft,void * mem,size_t * lenmem); 

/*
 * ak_fft(cfg,in_out_buf)
 *
 * Perform an FFT on a complex input buffer.
 * for a forward FFT,
 * fin should be  f[0] , f[1] , ... ,f[nfft-1]
 * fout will be   F[0] , F[1] , ... ,F[nfft-1]
 * Note that each element is complex and can be accessed like
    f[k].r and f[k].i
 * */
SPUStatic void ak_fft(ak_fft_cfg cfg,const ak_fft_cpx *fin,ak_fft_cpx *fout);

/*
 A more generic version of the above function. It reads its input from every Nth sample.
 * */
SPUStatic void ak_fft_stride(ak_fft_cfg cfg,const ak_fft_cpx *fin,ak_fft_cpx *fout,int fin_stride);

/* If ak_fft_alloc allocated a buffer, it is one contiguous 
   buffer and can be simply free()d when no longer needed*/
#define ak_fft_free free

/*
 * Returns the smallest integer k, such that k>=n and k has only "fast" factors (2,3,5)
 */
SPUStatic int ak_fft_next_fast_size(int n);

/* for real ffts, we need an even size */
#define ak_fftr_next_fast_size_real(n) \
        (ak_fft_next_fast_size( ((n)+1)>>1)<<1)

//#ifdef __cplusplus
//}
//#endif

} // namespace AkFFTAllButterflies or AkFFTSubsetButterflies

} // namespace DSP

#endif // AK_FFT_H
