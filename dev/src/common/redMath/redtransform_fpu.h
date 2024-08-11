/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _REDMATH_LIB_REDTRANSFORM_FLOAT_H_
#define _REDMATH_LIB_REDTRANSFORM_FLOAT_H_

namespace Red
{
	namespace Math
	{
		namespace Fpu
		{
			class RedTransform
			{
			private:
				RedMatrix3x3 Rotation;
				RedVector4 Translation;
			public:
				// Constructors
				RED_INLINE RedTransform();
				RED_INLINE RedTransform( const RedTransform& _r );
				RED_INLINE RedTransform( const RedMatrix3x3& _r, const RedVector4& _t );
				RED_INLINE RedTransform( const RedQuaternion& _q, const RedVector4& _v );

				RED_INLINE ~RedTransform();

				RED_INLINE float& operator() ( int _row, int _column );
				RED_INLINE const float& operator() ( int _row, int _column ) const;
				RED_INLINE void Set(const RedQuaternion& _q, const RedVector4& _v );
				RED_INLINE void SetIdentity();
				RED_INLINE bool IsAlmostEqual( const RedTransform& _t, float _epsilon = FLT_EPSILON );
				RED_INLINE void SetTranslation( const RedVector4& _t );
				RED_INLINE RedVector4& GetTranslation();
				RED_INLINE const RedVector4& GetTranslation() const;
				
				RED_INLINE void SetRotation( const RedMatrix3x3& _r );
				RED_INLINE void SetRotation( const RedQuaternion& _q );
				RED_INLINE RedMatrix3x3& GetRotation();
				RED_INLINE const RedMatrix3x3& GetRotation() const;
				
				RED_INLINE void SetInverse( const RedMatrix3x3& _r );
				RED_INLINE void SetMul( const RedTransform& _a, const RedTransform& _b );
				RED_INLINE void SetMul( const RedQsTransform& _a, const RedTransform& _b );
				RED_INLINE void SetMulInverseMul( const RedTransform& _a, const RedTransform& _b );
				RED_INLINE void SetMulEq( const RedTransform& _b );

				RED_INLINE void Get4x4ColumnMajor( float* _f ) const;
				RED_INLINE void Set4x4ColumnMajor( const float* _f );

				RED_INLINE void SetRows( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2, const RedVector4& _r3 );
				RED_INLINE bool IsOk() const;

				RED_INLINE RedVector4 GetColumn( int _i ) const;
			};
		}
	}
}

#endif // _REDMATH_LIB_REDTRANSFORM_FLOAT_H_