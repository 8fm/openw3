/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _VECTOR_ARITHMATIC_SIMD_H_
#define _VECTOR_ARITHMATIC_SIMD_H_
#include "redScalar_simd.h"
#include "redVector2_simd.h"
#include "redVector3_simd.h"
#include "redVector4_simd.h"

namespace RedMath
{
	namespace SIMD
	{
		//////////////////////////////////////////////////////////////////////////
		// '+' Addition Functions
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void Add( RedScalar& _a, const RedScalar& _b, const RedScalar& _c );
		RED_INLINE void Add( RedVector2& _a, const RedVector2& _b, const RedScalar& _c );
		RED_INLINE void Add( RedVector3& _a, const RedVector3& _b, const RedScalar& _c );
		RED_INLINE void Add( RedVector4& _a, const RedVector4& _b, const RedScalar& _c );

		RED_INLINE void Add( RedVector2& _a, const RedVector2& _b, const RedVector2& _c );
		RED_INLINE void Add( RedVector3& _a, const RedVector3& _b, const RedVector3& _c );
		RED_INLINE void Add( RedVector4& _a, const RedVector4& _b, const RedVector3& _c );
		RED_INLINE void Add( RedVector4& _a, const RedVector4& _b, const RedVector4& _c );

		RED_INLINE void Add3( RedVector4& _a, const RedVector4& _b, const RedVector4& _c );

		RED_INLINE RedScalar Add( const RedScalar& _a, const RedScalar& _b );
		RED_INLINE RedVector2 Add( const RedVector2& _a, const RedScalar& _b );
		RED_INLINE RedVector3 Add( const RedVector3& _a, const RedScalar& _b );
		RED_INLINE RedVector4 Add( const RedVector4& _a, const RedScalar& _b );

		RED_INLINE RedVector2 Add( const RedVector2& _a, const RedVector2& _b );
		RED_INLINE RedVector3 Add( const RedVector3& _a, const RedVector3& _b );
		RED_INLINE RedVector4 Add( const RedVector4& _a, const RedVector3& _b );
		RED_INLINE RedVector4 Add( const RedVector4& _a, const RedVector4& _b );

		RED_INLINE RedVector4 Add3( const RedVector4& _a, const RedVector4& _b );

		RED_INLINE void SetAdd( RedScalar& _a, const RedScalar& _b );
		RED_INLINE void SetAdd( RedVector2& _a, const RedScalar& _b );
		RED_INLINE void SetAdd( RedVector3& _a, const RedScalar& _b );
		RED_INLINE void SetAdd( RedVector4& _a, const RedScalar& _b );

		RED_INLINE void SetAdd( RedVector2& _a, const RedVector2& _b );
		RED_INLINE void SetAdd( RedVector3& _a, const RedVector3& _b );
		RED_INLINE void SetAdd( RedVector4& _a, const RedVector3& _b );
		RED_INLINE void SetAdd( RedVector4& _a, const RedVector4& _b );

		RED_INLINE void SetAdd3( RedVector4& _a, const RedVector4& _b );
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// '-' Subtraction Functions
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void Sub( RedScalar& _a, const RedScalar& _b, const RedScalar& _c );
		RED_INLINE void Sub( RedVector2& _a, const RedVector2& _b, const RedScalar& _c );
		RED_INLINE void Sub( RedVector3& _a, const RedVector3& _b, const RedScalar& _c );
		RED_INLINE void Sub( RedVector4& _a, const RedVector4& _b, const RedScalar& _c );

		RED_INLINE void Sub( RedVector2& _a, const RedVector2& _b, const RedVector2& _c );
		RED_INLINE void Sub( RedVector3& _a, const RedVector3& _b, const RedVector3& _c );
		RED_INLINE void Sub( RedVector4& _a, const RedVector4& _b, const RedVector3& _c );
		RED_INLINE void Sub( RedVector4& _a, const RedVector4& _b, const RedVector4& _c );

		RED_INLINE void Sub3( RedVector4& _a, const RedVector4& _b, const RedVector4& _c );

		RED_INLINE RedScalar Sub( const RedScalar& _a, const RedScalar& _b );
		RED_INLINE RedVector2 Sub( const RedVector2& _a, const RedScalar& _b );
		RED_INLINE RedVector3 Sub( const RedVector3& _a, const RedScalar& _b );
		RED_INLINE RedVector4 Sub( const RedVector4& _a, const RedScalar& _b );

		RED_INLINE RedVector2 Sub( const RedVector2& _a, const RedVector2& _b );
		RED_INLINE RedVector3 Sub( const RedVector3& _a, const RedVector3& _b );
		RED_INLINE RedVector4 Sub( const RedVector4& _a, const RedVector3& _b );
		RED_INLINE RedVector4 Sub( const RedVector4& _a, const RedVector4& _b );

		RED_INLINE RedVector4 Sub3( const RedVector4& _a, const RedVector4& _b );

		RED_INLINE void SetSub( RedScalar& _a, const RedScalar& _b );
		RED_INLINE void SetSub( RedVector2& _a, const RedScalar& _b );
		RED_INLINE void SetSub( RedVector3& _a, const RedScalar& _b );
		RED_INLINE void SetSub( RedVector4& _a, const RedScalar& _b );

		RED_INLINE void SetSub( RedVector2& _a, const RedVector2& _b );
		RED_INLINE void SetSub( RedVector3& _a, const RedVector3& _b );
		RED_INLINE void SetSub( RedVector4& _a, const RedVector3& _b );
		RED_INLINE void SetSub( RedVector4& _a, const RedVector4& _b );

		RED_INLINE void SetSub3( RedVector4& _a, const RedVector4& _b );
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// '*' Multiplication Functions
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void Mul( RedScalar& _a, const RedScalar& _b, const RedScalar& _c );
		RED_INLINE void Mul( RedVector2& _a, const RedVector2& _b, const RedScalar& _c );
		RED_INLINE void Mul( RedVector3& _a, const RedVector3& _b, const RedScalar& _c );
		RED_INLINE void Mul( RedVector4& _a, const RedVector4& _b, const RedScalar& _c );

		RED_INLINE void Mul( RedVector2& _a, const RedVector2& _b, const RedVector2& _c );
		RED_INLINE void Mul( RedVector3& _a, const RedVector3& _b, const RedVector3& _c );
		RED_INLINE void Mul( RedVector4& _a, const RedVector4& _b, const RedVector3& _c );
		RED_INLINE void Mul( RedVector4& _a, const RedVector4& _b, const RedVector4& _c );

		RED_INLINE void Mul3( RedVector4& _a, const RedVector4& _b, const RedVector4& _c );

		RED_INLINE RedScalar Mul( const RedScalar& _a, const RedScalar& _b );
		RED_INLINE RedVector2 Mul( const RedVector2& _a, const RedScalar& _b );
		RED_INLINE RedVector3 Mul( const RedVector3& _a, const RedScalar& _b );
		RED_INLINE RedVector4 Mul( const RedVector4& _a, const RedScalar& _b );

		RED_INLINE RedVector2 Mul( const RedVector2& _a, const RedVector2& _b );
		RED_INLINE RedVector3 Mul( const RedVector3& _a, const RedVector3& _b );
		RED_INLINE RedVector4 Mul( const RedVector4& _a, const RedVector3& _b );
		RED_INLINE RedVector4 Mul( const RedVector4& _a, const RedVector4& _b );

		RED_INLINE RedVector4 Mul3( const RedVector4& _a, const RedVector4& _b );

		RED_INLINE void SetMul( RedScalar& _a, const RedScalar& _b );
		RED_INLINE void SetMul( RedVector2& _a, const RedScalar& _b );
		RED_INLINE void SetMul( RedVector3& _a, const RedScalar& _b );
		RED_INLINE void SetMul( RedVector4& _a, const RedScalar& _b );

		RED_INLINE void SetMul( RedVector2& _a, const RedVector2& _b );
		RED_INLINE void SetMul( RedVector3& _a, const RedVector3& _b );
		RED_INLINE void SetMul( RedVector4& _a, const RedVector3& _b );
		RED_INLINE void SetMul( RedVector4& _a, const RedVector4& _b );

		RED_INLINE void SetMul3( RedVector4& _a, const RedVector4& _b );
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// '/' Divisor Function
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void Div( RedScalar& _a, const RedScalar& _b, const RedScalar& _c );
		RED_INLINE void Div( RedVector2& _a, const RedVector2& _b, const RedScalar& _c );
		RED_INLINE void Div( RedVector3& _a, const RedVector3& _b, const RedScalar& _c );
		RED_INLINE void Div( RedVector4& _a, const RedVector4& _b, const RedScalar& _c );

		RED_INLINE void Div( RedVector2& _a, const RedVector2& _b, const RedVector2& _c );
		RED_INLINE void Div( RedVector3& _a, const RedVector3& _b, const RedVector3& _c );
		RED_INLINE void Div( RedVector4& _a, const RedVector4& _b, const RedVector3& _c );
		RED_INLINE void Div( RedVector4& _a, const RedVector4& _b, const RedVector4& _c );

		RED_INLINE void Div3( RedVector4& _a, const RedVector4& _b, const RedVector4& _c );

		RED_INLINE RedScalar Div( const RedScalar& _a, const RedScalar& _b );
		RED_INLINE RedVector2 Div( const RedVector2& _a, const RedScalar& _b );
		RED_INLINE RedVector3 Div( const RedVector3& _a, const RedScalar& _b );
		RED_INLINE RedVector4 Div( const RedVector4& _a, const RedScalar& _b );

		RED_INLINE RedVector2 Div( const RedVector2& _a, const RedVector2& _b );
		RED_INLINE RedVector3 Div( const RedVector3& _a, const RedVector3& _b );
		RED_INLINE RedVector4 Div( const RedVector4& _a, const RedVector3& _b );
		RED_INLINE RedVector4 Div( const RedVector4& _a, const RedVector4& _b );

		RED_INLINE RedVector4 Div3( const RedVector4& _a, const RedVector4& _b );

		RED_INLINE void SetDiv( RedScalar& _a, const RedScalar& _b );
		RED_INLINE void SetDiv( RedVector2& _a, const RedScalar& _b );
		RED_INLINE void SetDiv( RedVector3& _a, const RedScalar& _b );
		RED_INLINE void SetDiv( RedVector4& _a, const RedScalar& _b );

		RED_INLINE void SetDiv( RedVector2& _a, const RedVector2& _b );
		RED_INLINE void SetDiv( RedVector3& _a, const RedVector3& _b );
		RED_INLINE void SetDiv( RedVector4& _a, const RedVector3& _b );
		RED_INLINE void SetDiv( RedVector4& _a, const RedVector4& _b );

		RED_INLINE void SetDiv3( RedVector4& _a, const RedVector4& _b );
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// 'MIN/MAX' Functions
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void Min( RedScalar& _a, const RedVector2& _b, const RedVector2& _c );
		RED_INLINE RedScalar Min( const RedVector2& _a, const RedVector2& _b );
		RED_INLINE void Min( RedVector3& _a, const RedVector3& _b, const RedVector3& _c );
		RED_INLINE RedVector3 Min( const RedVector3& _a, const RedVector3& _b );
		RED_INLINE void Min( RedVector4& _a, const RedVector4& _b, const RedVector3& _c );
		RED_INLINE RedVector4 Min( const RedVector4& _a, const RedVector3& _b );
		RED_INLINE void Min( RedVector4& _a, const RedVector4& _b, const RedVector4& _c );
		RED_INLINE RedVector4 Min( const RedVector4& _a, const RedVector4& _b );

		RED_INLINE void Max( RedScalar& _a, const RedVector2& _b, const RedVector2& _c );
		RED_INLINE RedScalar Max( const RedVector2& _a, const RedVector2& _b );
		RED_INLINE void Max( RedVector3& _a, const RedVector3& _b, const RedVector3& _c );
		RED_INLINE RedVector3 Max( const RedVector3& _a, const RedVector3& _b );
		RED_INLINE void Max( RedVector4& _a, const RedVector4& _b, const RedVector3& _c );
		RED_INLINE RedVector4 Max( const RedVector4& _a, const RedVector3& _b );
		RED_INLINE void Max( RedVector4& _a, const RedVector4& _b, const RedVector4& _c );
		RED_INLINE RedVector4 Max( const RedVector4& _a, const RedVector4& _b );
	}
}

#endif // _VECTOR_ARITHMATIC_SIMD_H_