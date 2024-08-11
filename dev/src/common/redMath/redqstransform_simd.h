/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _REDMATH_LIB_REDQSTRANSFORM_SIMD_H_
#define _REDMATH_LIB_REDQSTRANSFORM_SIMD_H_

#include "redtransform_simd.h"

namespace RedMath
{
	namespace SIMD
	{
		class RedQsTransform
		{
		public:
			RedVector4 Translation;
			RedQuaternion Rotation;
			RedVector4 Scale;

			// Constructors
			RED_INLINE RedQsTransform();
			RED_INLINE RedQsTransform( const RedQsTransform& _other );
			RED_INLINE RedQsTransform( const RedVector4& _translation, const RedQuaternion& _rotation, const RedVector4& _scale );
			RED_INLINE RedQsTransform( const RedVector4& _translation, const RedQuaternion& _rotation) ;
			
			// Destructor
			RED_INLINE ~RedQsTransform();

			// Operators
			RED_INLINE void operator=( const RedQsTransform& _other );

			RED_INLINE void Set( const RedVector4& _translation, const RedQuaternion& _rotation, const RedVector4& _scale );
			RED_INLINE void Set( const RedMatrix4x4& _m );

			RED_INLINE void SetIdentity();
			RED_INLINE void SetZero();

			RED_INLINE void SetTranslation( const RedVector4& _v );
			RED_INLINE void SetRotation(const RedQuaternion& _r);
			RED_INLINE void SetScale(const RedVector4& _s);

			RED_INLINE const RedVector4& GetTranslation() const;
			RED_INLINE const RedQuaternion& GetRotation() const;
			RED_INLINE const RedVector4& GetScale() const;
			
			RED_INLINE void Lerp( const RedQsTransform& _a, const RedQsTransform& _b, float _t );
			RED_INLINE void Slerp( const RedQsTransform& _a, const RedQsTransform& _b, float _t );
			
			RED_INLINE RedMatrix4x4 ConvertToMatrix() const;
			RED_INLINE RedMatrix4x4 ConvertToMatrixNormalized() const;
			
			RED_INLINE void SetFromTransformNoScale( const RedTransform& _t );
			RED_INLINE void CopyToTransformNoScale( RedTransform& _tOut ) const;
			RED_INLINE void SetFromTransform( const RedTransform& _t );
			RED_INLINE void CopyToTransform( RedTransform& _tOut ) const; 
			RED_INLINE void SetInverse( const RedQsTransform& _t );
			RED_FORCE_INLINE void SetMul( const RedQsTransform& _a, const RedQsTransform& _b );
			RED_INLINE void SetMulInverseMul( const RedQsTransform& _a, const RedQsTransform& _b );
			RED_INLINE void SetMulMulInverse( const RedQsTransform& _a, const RedQsTransform& _b );
			RED_INLINE void SetMulEq( const RedQsTransform& _t );
			RED_INLINE bool Set4x4ColumnMajor( const float* _f );
			RED_INLINE void Get4x4ColumnMajor( float* _f ) const;
			RED_INLINE bool IsOk() const;			
			RED_INLINE bool IsAlmostEqual( const RedQsTransform& _other, float _epsilon = FLT_EPSILON ) const;
			
			RED_INLINE void BlendAddMul( const RedQsTransform& _other,  float _weight = 1.0f );
			RED_INLINE void BlendNormalize( float _totalWeight = 1.0f);
			RED_INLINE void FastRenormalize( float _totalWeight = 1.0f);
			static RED_INLINE void FastRenormalizeBatch( RedQsTransform* _poseOut, float* _weight, unsigned int _numTransforms);

			static const RedQsTransform IDENTITY;
		};
	};
};

#endif // _REDMATH_LIB_REDQSTRANSFORM_SIMD_H_
