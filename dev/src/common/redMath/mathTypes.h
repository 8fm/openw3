/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _REDMATH_LIB_TYPES_H
#define _REDMATH_LIB_TYPES_H
#include "../redSystem/error.h"
#include <tmmintrin.h>
#include <xutility>
#include <float.h>
#include <math.h>
#include "../redSystem/compilerExtensions.h"
#include "mathfunctions_fpu.h"

#define RED_EPSILON FLT_EPSILON
#define RED_ISNAN( x ) _isnan(x)
#define RED_ISFINITE( x ) isfinite<float>( x )
static const unsigned int __nan[2] = { 0x7ff00000, 0xffffffff };
static const unsigned int __inf[2] = { 0x7f800000, 0xff800000 };

#define RED_INFINITY (*(const float *)__inf)
#define RED_NEG_INFINITY (*(const float *) ( &__inf[1] ) )
#define RED_NAN (*(const float *)__nan)
#define RED_NEG_NAN (*(const float *) ( &__nan[1] ) )

#ifndef RED_FINAL_BUILD
	#define RED_MATH_MEM_ALIGNMENT_CHECK(ptr, alignment) \
	RED_FATAL_ASSERT(0 == (reinterpret_cast<intptr_t>(ptr) & static_cast<intptr_t>(alignment - 1)), "FAILED: Memory not aligned to %d byte.", alignment);
#else
	#define RED_MATH_MEM_ALIGNMENT_CHECK(ptr, alignment)
#endif

#define RED_MATH_SSE_MEMORY_ALIGNMENT 16

#define RED_MATH_SSE_VERSION 3

typedef __m128 SIMDVector;

namespace RedMath
{
	namespace SIMD
	{
		namespace _Internal
		{
			extern const unsigned int X_MASKDATA[4];
			extern const unsigned int Y_MASKDATA[4];
			extern const unsigned int Z_MASKDATA[4];
			extern const unsigned int W_MASKDATA[4];
			extern const unsigned int XY_MASKDATA[4];
			extern const unsigned int XZ_MASKDATA[4];
			extern const unsigned int XW_MASKDATA[4];
			extern const unsigned int YZ_MASKDATA[4];
			extern const unsigned int YW_MASKDATA[4];
			extern const unsigned int ZW_MASKDATA[4];
			extern const unsigned int XYZ_MASKDATA[4];
			extern const unsigned int YZW_MASKDATA[4];
		}

		extern const SIMDVector SIGN_MASK; 
		extern const SIMDVector X_MASK;
		extern const SIMDVector Y_MASK;
		extern const SIMDVector Z_MASK;
		extern const SIMDVector W_MASK;
		extern const SIMDVector XY_MASK;
		extern const SIMDVector XZ_MASK;
		extern const SIMDVector XW_MASK;
		extern const SIMDVector YZ_MASK;
		extern const SIMDVector YW_MASK;
		extern const SIMDVector ZW_MASK;
		extern const SIMDVector XYZ_MASK;
		extern const SIMDVector YZW_MASK;
		extern const SIMDVector EPSILON_VALUE; 
	}
}

#endif