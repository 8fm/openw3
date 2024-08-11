/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _VECTOR_FUNCTIONS_FLOAT_H_
#define _VECTOR_FUNCTIONS_FLOAT_H_
#include "vectorArithmetic_float.h"

namespace RedMath
{
	namespace FLOAT
	{
		//////////////////////////////////////////////////////////////////////////
		// 'SQUARE ROOT' Functions
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void SqrRoot( RedScalar& _a );
		RED_INLINE RedScalar SqrRoot( const RedScalar& _a );
		RED_INLINE void SqrRoot( RedVector2& _a );
		RED_INLINE RedVector2 SqrRoot( const RedVector2& _a );
		RED_INLINE void SqrRoot( RedVector3& _a );
		RED_INLINE RedVector3 SqrRoot( const RedVector3& _a );
		RED_INLINE void SqrRoot( RedVector4& _a );
		RED_INLINE RedVector4 SqrRoot( const RedVector4& _a );

		RED_INLINE void SqrRoot3( RedVector4& _a );
		RED_INLINE RedVector4 SqrRoot3( const RedVector4& _a );
		//////////////////////////////////////////////////////////////////////////
		// 'RECIPROCAL SQUARE ROOT' Functions
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RSqrRoot( RedScalar& _a );
		RED_INLINE RedScalar RSqrRoot( const RedScalar& _a );
		RED_INLINE void RSqrRoot( RedVector2& _a );
		RED_INLINE RedVector2 RSqrRoot( const RedVector2& _a );
		RED_INLINE void RSqrRoot( RedVector3& _a );
		RED_INLINE RedVector3 RSqrRoot( const RedVector3& _a );
		RED_INLINE void RSqrRoot( RedVector4& _a );
		RED_INLINE RedVector4 RSqrRoot( const RedVector4& _a );

		RED_INLINE void RSqrRoot3( RedVector4& _a );
		RED_INLINE RedVector4 RSqrRoot3( const RedVector4& _a );
		//////////////////////////////////////////////////////////////////////////
		// 'DOT PRODUCT' Functions
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void Dot( RedScalar& _a, const RedVector2& _b, const RedVector2& _c );
		RED_INLINE RedScalar Dot( const RedVector2& _a, const RedVector2& _b );
		RED_INLINE void Dot( RedScalar& _a, const RedVector3& _b, const RedVector3& _c );
		RED_INLINE RedScalar Dot( const RedVector3& _a, const RedVector3& _b );
		RED_INLINE void Dot( RedScalar& _a, const RedVector4& _b, const RedVector3& _c );
		RED_INLINE RedScalar Dot( const RedVector4& _a, const RedVector3& _b );
		RED_INLINE void Dot( RedScalar& _a, const RedVector4& _b, const RedVector4& _c );
		RED_INLINE RedScalar Dot( const RedVector4& _a, const RedVector4& _b );

		RED_INLINE void Dot3( RedScalar& _a, const RedVector4& _b, const RedVector4& _c );
		RED_INLINE RedScalar Dot3( const RedVector4& _a, const RedVector4& _b );
		//////////////////////////////////////////////////////////////////////////
		// 'NORMALIZED DOT PRODUCT' Functions
		// Ensure the input vectors are of unit length.
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void UnitDot( RedScalar& _a, const RedVector2& _b, const RedVector2& _c );
		RED_INLINE RedScalar UnitDot( const RedVector2& _a, const RedVector2& _b );
		RED_INLINE void UnitDot( RedScalar& _a, const RedVector3& _b, const RedVector3& _c );
		RED_INLINE RedScalar UnitDot( const RedVector3& _a, const RedVector3& _b );
		RED_INLINE void UnitDot( RedScalar& _a, const RedVector4& _b, const RedVector3& _c );
		RED_INLINE RedScalar UnitDot( const RedVector4& _a, const RedVector3& _b );
		RED_INLINE void UnitDot( RedScalar& _a, const RedVector4& _b, const RedVector4& _c );
		RED_INLINE RedScalar UnitDot( const RedVector4& _a, const RedVector4& _b );

		RED_INLINE void UnitDot3( RedScalar& _a, const RedVector4& _b, const RedVector4& _c );
		RED_INLINE RedScalar UnitDot3( const RedVector4& _a, const RedVector4& _b );
		//////////////////////////////////////////////////////////////////////////
		// 'CROSS PRODUCT' Functions
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void Cross( RedScalar& _a, const RedVector2& _b, const RedVector2& _c );
		RED_INLINE RedScalar Cross( const RedVector2& _a, const RedVector2& _b );
		RED_INLINE void Cross( RedVector3& _a, const RedVector3& _b, const RedVector3& _c );
		RED_INLINE RedVector3 Cross( const RedVector3& _a, const RedVector3& _b );
		RED_INLINE void Cross( RedVector4& _a, const RedVector4& _b, const RedVector3& _c );
		RED_INLINE RedVector4 Cross( const RedVector4& _a, const RedVector3& _b );
		RED_INLINE void Cross( RedVector4& _a, const RedVector4& _b, const RedVector4& _c );
		RED_INLINE RedVector4 Cross( const RedVector4& _a, const RedVector4& _b );
	};
};

#include "vectorFunctions_float.inl"

#endif //_VECTOR_FUNCTIONS_FLOAT_H_