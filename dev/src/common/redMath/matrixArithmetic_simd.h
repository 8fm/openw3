/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _REDMATH_LIB_MATRIX_ARITHMETIC_SIMD_H_
#define _REDMATH_LIB_MATRIX_ARITHMETIC_SIMD_H_

#include "redMatrix4x4_simd.h"
#include "redMatrix3x3_simd.h"

#include "matrixArithmetic_simd.h"

namespace RedMath
{
	namespace SIMD
	{
		//////////////////////////////////////////////////////////////////////////
		// Multiplication 4x4
		//////////////////////////////////////////////////////////////////////////
		void Mul( RedMatrix4x4& _a, const RedMatrix4x4& _b, const RedMatrix4x4& _c );
		RedMatrix4x4 Mul( const RedMatrix4x4& _a, const RedMatrix4x4& _b );
		void SetMul( RedMatrix4x4& _a, const RedMatrix4x4& _b );

		//////////////////////////////////////////////////////////////////////////
		// Multiplication 3x3
		//////////////////////////////////////////////////////////////////////////
		void Mul( RedMatrix3x3& _a, const RedMatrix3x3& _b, const RedMatrix3x3& _c );
		RedMatrix3x3 Mul( const RedMatrix3x3& _a, const RedMatrix3x3& _b );
		void SetMul( RedMatrix3x3& _a, const RedMatrix3x3& _b );
	}
}
#endif // _REDMATH_LIB_MATRIX_ARITHMETIC_SIMD_H_