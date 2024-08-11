/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _REDMATH_LIB_QUATERNION_SIMD_H_
#define _REDMATH_LIB_QUATERNION_SIMD_H_

#include "../redSystem/types.h"
#include "math.h"
#include "mathTypes.h"

namespace RedMath
{ 
	namespace SIMD
	{
		RED_ALIGNED_CLASS( RedQuaternion, 16 )
		{
		public:

			RED_INLINE RedQuaternion();
			RED_INLINE RedQuaternion( float _x, float _y, float _z, float _r );
			RED_INLINE RedQuaternion( const RedVector4& _axis, float _angle );
			RED_INLINE RedQuaternion( const RedQuaternion& _q );

			// Destructor
			RED_INLINE ~RedQuaternion();

			// Operators
			RED_INLINE void operator=( const RedQuaternion& _q );
			RED_INLINE void operator=( const RedVector4& _v );


			RED_INLINE void ConstructFromMatrix( const RedMatrix3x3& _rotMat );
			RED_INLINE void Set( float _x, float _y, float _z, float _w );
			RED_INLINE void Set( const RedMatrix3x3& _rotMat );

			RED_INLINE void SetIdentity();

			RED_INLINE void SetInverse( const RedQuaternion& _q );

			RED_FORCE_INLINE void Normalize();

			static RED_FORCE_INLINE RedQuaternion Mul( const RedQuaternion& _a, const RedQuaternion& _b );
			static RED_INLINE RedQuaternion Add( const RedQuaternion& _a, const RedQuaternion& _b );
			RED_FORCE_INLINE void SetMul( const RedQuaternion& _a, const RedQuaternion& _b ); 
			
			RED_INLINE void SetMulInverse( const RedQuaternion& _a, const RedQuaternion& _b );
			RED_INLINE void SetInverseMul( const RedQuaternion& _a, const RedQuaternion& _b );

			RED_INLINE RedVector4 EstimateAngleTo( const RedQuaternion& _to );

			RED_INLINE void SetShortestRotation( const RedVector4& _from, const RedVector4& _to );
			RED_INLINE void SetShortestRotationDamped( float _gain, const RedVector4& _fromt, const RedVector4& _to );

			RED_INLINE void SetAxisAngle( const RedVector4& _axis, float _angle );

			RED_INLINE void SetAndNormalize( const RedMatrix3x3& _rotMat );

			RED_INLINE void RemoveAxisComponent( const RedVector4& _axis );

			RED_INLINE void DecomposeRestAxis( const RedVector4& _axis, RedQuaternion& _restOut, float& _angleOut ) const;

			RED_FORCE_INLINE void SetLerp( const RedQuaternion& _a, const RedQuaternion& _b, float _t );
			RED_INLINE void SetSlerp( const RedQuaternion& _a, const RedQuaternion& _b, float _t );

			RED_INLINE void SetReal( float _f );
			RED_INLINE float GetReal() const;

			RED_INLINE void SetImaginary( const RedVector4& _v );

			RED_INLINE const RedVector4& GetImaginary() const;

			RED_INLINE float GetAngle() const;

			RED_INLINE float GetYaw() const;

			RED_INLINE RedVector4 GetAxis() const;

			RED_INLINE bool HasValidAxis() const;

			RED_INLINE bool IsOk() const;

			RED_INLINE void SetFlippedRotation( const RedVector4& _from );
			
			static const RedQuaternion IDENTITY;

			RedVector4 Quat;
		};
	};
};

#endif // _REDMATH_LIB_QUATERNION_SIMD_H_