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

#ifndef _AKFFTGUTS_H_
#define _AKFFTGUTS_H_

/* ak_fft.h
   defines ak_fft_scalar as either short or a float type
   and defines
   typedef struct { ak_fft_scalar r; ak_fft_scalar i; }ak_fft_cpx; */
#include "ak_fft.h"
//#include <limits.h>
#include <AK/SoundEngine/Common/AkTypes.h>

//#define MAXFACTORS 32
#define MAXFACTORS 8 // Supports FFTSize up to 4^8 in well behaved requested FFT size
/* e.g. an fft of length 128 has 4 factors 
 as far as ak_fft is concerned
 4*4*4*2
 */

AK_ALIGN_SIMD(
struct ak_fft_state
{
    int nfft;
    int inverse;
    int factors[2*MAXFACTORS];
    ak_fft_cpx * twiddles;
	ak_fft_cpx *simdTwiddles;
}
);


/*
  Explanation of macros dealing with complex math:

   C_MUL(m,a,b)         : m = a*b
   C_FIXDIV( c , div )  : if a fixed point impl., c /= div. noop otherwise
   C_SUB( res, a,b)     : res = a - b
   C_SUBFROM( res , a)  : res -= a
   C_ADDTO( res , a)    : res += a
 * */
#ifdef FIXED_POINT
#if (FIXED_POINT==32)
# define FRACBITS 31
# define SAMPPROD int64_t
#define SAMP_MAX 2147483647
#else
# define FRACBITS 15
# define SAMPPROD int32_t 
#define SAMP_MAX 32767
#endif

#define SAMP_MIN -SAMP_MAX

#if defined(CHECK_OVERFLOW)
#  define CHECK_OVERFLOW_OP(a,op,b)  \
	if ( (SAMPPROD)(a) op (SAMPPROD)(b) > SAMP_MAX || (SAMPPROD)(a) op (SAMPPROD)(b) < SAMP_MIN ) { \
		fprintf(stderr,"WARNING:overflow @ " __FILE__ "(%d): (%d " #op" %d) = %ld\n",__LINE__,(a),(b),(SAMPPROD)(a) op (SAMPPROD)(b) );  }
#endif


#   define smul(a,b) ( (SAMPPROD)(a)*(b) )
#   define sround( x )  (ak_fft_scalar)( ( (x) + (1<<(FRACBITS-1)) ) >> FRACBITS )

#   define S_MUL(a,b) sround( smul(a,b) )

#   define C_MUL(m,a,b) \
      do{ (m).r = sround( smul((a).r,(b).r) - smul((a).i,(b).i) ); \
          (m).i = sround( smul((a).r,(b).i) + smul((a).i,(b).r) ); }while(0)

#   define DIVSCALAR(x,k) \
	(x) = sround( smul(  x, SAMP_MAX/k ) )

#   define C_FIXDIV(c,div) \
	do {    DIVSCALAR( (c).r , div);  \
		DIVSCALAR( (c).i  , div); }while (0)

#   define C_MULBYSCALAR( c, s ) \
    do{ (c).r =  sround( smul( (c).r , s ) ) ;\
        (c).i =  sround( smul( (c).i , s ) ) ; }while(0)

#else  /* not FIXED_POINT*/

#   define S_MUL(a,b) ( (a)*(b) )
#define C_MUL(m,a,b) \
    { (m).r = (a).r*(b).r - (a).i*(b).i;\
        (m).i = (a).r*(b).i + (a).i*(b).r; } 
#   define C_FIXDIV(c,div) /* NOOP */
#   define C_MULBYSCALAR( c, s ) \
    { (c).r *= (s);\
        (c).i *= (s); }
#endif

#ifndef CHECK_OVERFLOW_OP
#  define CHECK_OVERFLOW_OP(a,op,b) /* noop */
#endif

#define  C_ADD( res, a,b)\
    { \
	    (res).r=(a).r+(b).r;  (res).i=(a).i+(b).i; \
    }
#define  C_SUB( res, a,b)\
    { \
	    (res).r=(a).r-(b).r;  (res).i=(a).i-(b).i; \
    }
#define C_ADDTO( res , a)\
    { \
	    (res).r += (a).r;  (res).i += (a).i;\
    }

#define C_SUBFROM( res , a)\
    {\
	    (res).r -= (a).r;  (res).i -= (a).i; \
    }


#ifdef FIXED_POINT
#  define AK_FFT_COS(phase)  floor(.5+SAMP_MAX * cos (phase))
#  define AK_FFT_SIN(phase)  floor(.5+SAMP_MAX * sin (phase))
#  define HALF_OF(x) ((x)>>1)
#else
#  define AK_FFT_COS(phase) (ak_fft_scalar) cos(phase)
#  define AK_FFT_SIN(phase) (ak_fft_scalar) sin(phase)
#  define HALF_OF(x) ((x)*.5f)
#endif

// TODO: Init optimizations use faster SinCos implementations to reduce expensive trig calls
#define  kf_cexp(x,phase) \
	{ \
		(x)->r = AK_FFT_COS(phase);\
		(x)->i = AK_FFT_SIN(phase);\
	}


/* a debugging function */
#define pcpx(c)\
    fprintf(stderr,"%g + %gi\n",(double)((c)->r),(double)((c)->i) )

#endif // _AKFFTGUTS_H_