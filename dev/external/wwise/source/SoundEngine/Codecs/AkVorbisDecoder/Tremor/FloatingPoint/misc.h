/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2003    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: miscellaneous math and prototypes

 ********************************************************************/

#ifndef _V_RANDOM_H_
#define _V_RANDOM_H_

#include "ivorbiscodec.h"
#include "os_types.h"

/*#define _VDBG_GRAPHFILE "_0.m"*/


#ifdef _VDBG_GRAPHFILE
extern void *_VDBG_malloc(void *ptr,long bytes,char *file,long line); 
extern void _VDBG_free(void *ptr,char *file,long line); 

#undef _ogg_malloc
#undef _ogg_calloc
#undef _ogg_realloc
#undef _ogg_free

#define _ogg_malloc(x) _VDBG_malloc(NULL,(x),__FILE__,__LINE__)
#define _ogg_calloc(x,y) _VDBG_malloc(NULL,(x)*(y),__FILE__,__LINE__)
#define _ogg_realloc(x,y) _VDBG_malloc((x),(y),__FILE__,__LINE__)
#define _ogg_free(x) _VDBG_free((x),__FILE__,__LINE__)
#endif

#ifndef _V_WIDE_MATH
#define _V_WIDE_MATH
  
#ifndef  _LOW_ACCURACY_
/* 64 bit multiply */

//#include <sys/types.h>

static inline ogg_int32_t MULT32(ogg_int32_t x, ogg_int32_t y)
{
	float Prod	= (float)x * (float)y;
	Prod		= Prod * 0.00000000023283064365386962890625f;
	return (ogg_int32_t)Prod;
}

static inline ogg_int32_t MULT31(ogg_int32_t x, ogg_int32_t y)
{
	float Prod	= (float)x * (float)y;
	Prod		= Prod * 0.0000000004656612873077392578125f;
	return (ogg_int32_t)Prod;
}

static inline float fMULT31(float x, float y)
{
	float Prod	= x * y;
	Prod		= Prod * 0.0000000004656612873077392578125f;
	return Prod;
}

static inline ogg_int32_t MULT31_SHIFT15(ogg_int32_t x, ogg_int32_t y)
{
	float Prod	= (float)x * (float)y;
	Prod		= Prod * 0.000030517578125f;
	return (ogg_int32_t)Prod;
}

#else
/* 32 bit multiply, more portable but less accurate */

/*
 * Note: Precision is biased towards the first argument therefore ordering
 * is important.  Shift values were chosen for the best sound quality after
 * many listening tests.
 */

/*
 * For MULT32 and MULT31: The second argument is always a lookup table
 * value already preshifted from 31 to 8 bits.  We therefore take the 
 * opportunity to save on text space and use unsigned char for those
 * tables in this case.
 */

static inline ogg_int32_t MULT32(ogg_int32_t x, ogg_int32_t y) {
  return (x >> 9) * y;  /* y preshifted >>23 */
}

static inline ogg_int32_t MULT31(ogg_int32_t x, ogg_int32_t y) {
  return (x >> 8) * y;  /* y preshifted >>23 */
}

static inline ogg_int32_t MULT31_SHIFT15(ogg_int32_t x, ogg_int32_t y) {
  return (x >> 6) * y;  /* y preshifted >>9 */
}

#endif

/*
 * This should be used as a memory barrier, forcing all cached values in
 * registers to wr writen back to memory.  Might or might not be beneficial
 * depending on the architecture and compiler.
 */
#define MB()

static inline void XPROD32(ogg_int32_t  a, ogg_int32_t  b,
			   ogg_int32_t  t, ogg_int32_t  v,
			   ogg_int32_t *x, ogg_int32_t *y)
{
  *x = MULT32(a, t) + MULT32(b, v);
  *y = MULT32(b, t) - MULT32(a, v);
}

static inline void XPROD31(	ogg_int32_t  a, ogg_int32_t  b,
							ogg_int32_t  t, ogg_int32_t  v,
							ogg_int32_t *x, ogg_int32_t *y)
{
  *x = MULT31(a, t) + MULT31(b, v);
  *y = MULT31(b, t) - MULT31(a, v);
}

static inline void fXPROD31(float  a, float  b,
							float  t, float  v,
							ogg_int32_t *x, ogg_int32_t *y)
{
  *x = (ogg_int32_t)((a * t) + (b * v));
  *y = (ogg_int32_t)((b * t) - (a * v));
}

static inline void ffXPROD31(float  a, float  b,
							float  t, float  v,
							float *x, float *y)
{
  *x = (a * t) + (b * v);
  *y = (b * t) - (a * v);
}

static inline void XNPROD31(ogg_int32_t  a, ogg_int32_t  b,
							ogg_int32_t  t, ogg_int32_t  v,
							ogg_int32_t *x, ogg_int32_t *y)
{
  *x = MULT31(a, t) - MULT31(b, v);
  *y = MULT31(b, t) + MULT31(a, v);
}

static inline void fXNPROD31(float  a, float  b,
							float  t, float  v,
							ogg_int32_t *x, ogg_int32_t *y)
{
  *x = (ogg_int32_t)((a * t) - (b * v));
  *y = (ogg_int32_t)((b * t) + (a * v));
}

static inline void ffXNPROD31(float  a, float  b,
							float  t, float  v,
							float *x, float *y)
{
  *x = ((a * t) - (b * v));
  *y = ((b * t) + (a * v));
}

#endif

#ifndef _V_CLIP_MATH
#define _V_CLIP_MATH

static inline ogg_int32_t CLIP_TO_15(ogg_int32_t x)
{
	int ret=x;
	ret-= ((x<=32767)-1)&(x-32767);
	ret-= ((x>=-32768)-1)&(x+32768);
	return(ret);
}

#endif

#endif

