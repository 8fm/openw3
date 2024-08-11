/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _RED_MATH_LIB_REDVECTOR4_SIMD_H_
#define _RED_MATH_LIB_REDVECTOR4_SIMD_H_

#include "../redSystem/types.h"
#include "redScalar_simd.h"
#include "redVector3_simd.h"
#include "mathTypes.h"

namespace RedMath
{
	namespace SIMD
	{
		RED_ALIGNED_CLASS( RedVector4, 16 )
		{
		public:
			// Constructors
			RED_INLINE RedVector4();
			RED_INLINE RedVector4( const RedScalar& _v );
			RED_INLINE RedVector4( const RedVector3& _v );
			RED_INLINE RedVector4( const RedVector3& _v, Red::System::Float _w );
			RED_INLINE RedVector4( const RedVector4& _v );
			RED_INLINE RedVector4( const Red::System::Float* _f );
			RED_INLINE RedVector4( Red::System::Float _x, Red::System::Float _y, Red::System::Float _z, Red::System::Float _w = 0.0f );
			RED_INLINE RedVector4( SIMDVector _v );
			
			// Destructor
			RED_INLINE ~RedVector4();

			RED_INLINE const Red::System::Float* AsFloat() const;
			RED_INLINE void Store( Red::System::Float* _f ) const;

			// Operators
			RED_INLINE RedVector4& operator=( const RedScalar& _v );
			RED_INLINE RedVector4& operator=( const RedVector3& _v );
			
			// Methods
			RED_INLINE void Set( const RedScalar& _v );
			RED_INLINE void Set( const RedVector3& _v );
			RED_INLINE void Set( const RedVector3& _v, Red::System::Float _w );
			RED_INLINE void Set( const RedVector4& _v );
			RED_INLINE void Set( Red::System::Float _x, Red::System::Float _y, Red::System::Float _z, Red::System::Float _w = 0.0f );
			RED_INLINE void Set( const Red::System::Float* _f );
			
			RED_INLINE void SetTransformedInversePos(const RedQsTransform& _t, const RedVector4& _v );
			RED_INLINE void RotateDirection( const RedQuaternion& _quat, const RedVector4& _direction );
			RED_INLINE void InverseRotateDirection( const RedQuaternion& _quat, const RedVector4& _v );
			RED_INLINE RedVector4& SetTransformedPos( const RedQsTransform& _trans, const RedVector4& _v );
			
			RED_INLINE void SetZeros();
			RED_INLINE void SetOnes();

			RED_INLINE RedVector4& Negate();
			RED_INLINE RedVector4 Negated() const;
			RED_INLINE RedVector4 Abs() const;

			RED_INLINE RedScalar Sum3() const;
			RED_INLINE RedScalar Sum4() const;
			RED_INLINE RedScalar Length3() const;
			RED_INLINE RedScalar Length4() const;
			RED_INLINE RedScalar SquareLength2() const;
			RED_INLINE RedScalar SquareLength3() const;
			RED_INLINE RedScalar SquareLength4() const;

			RED_INLINE RedScalar DistanceSquaredTo( const RedVector4& _t ) const;
			RED_INLINE RedScalar DistanceTo2D( const RedVector4& _t ) const;
			RED_INLINE RedScalar DistanceTo( const RedVector4& _t ) const;

			RED_INLINE RedVector4& Normalize4();
			RED_INLINE RedVector4 Normalized4() const;
			RED_INLINE RedVector4& Normalize3();
			RED_INLINE RedVector4 Normalized3() const;

			RED_INLINE RedVector4& NormalizeFast4();
			RED_INLINE RedVector4 NormalizedFast4() const;
			RED_INLINE RedVector4& NormalizeFast3();
			RED_INLINE RedVector4 NormalizedFast3() const;

			RED_INLINE Red::System::Bool IsNormalized4( const SIMDVector _epsilon = EPSILON_VALUE ) const;
			RED_INLINE Red::System::Bool IsNormalized3( const SIMDVector _epsilon = EPSILON_VALUE ) const;
			RED_INLINE Red::System::Bool IsNormalized4( float _epsilon ) const;
			RED_INLINE Red::System::Bool IsNormalized3( float _epsilon ) const;

			RED_INLINE Red::System::Bool IsAlmostEqual( const RedVector4& _v, const SIMDVector _epsilon = EPSILON_VALUE ) const;
			RED_INLINE Red::System::Bool IsAlmostEqual( const RedVector4& _v, float _epsilon ) const;
			RED_INLINE Red::System::Bool IsAlmostZero( const SIMDVector _epsilon = EPSILON_VALUE ) const;
			RED_INLINE Red::System::Bool IsZero() const;

			RED_INLINE RedScalar Upper3() const;
			RED_INLINE RedScalar Upper4() const;
			RED_INLINE RedScalar Lower3() const;
			RED_INLINE RedScalar Lower4() const;

			RED_INLINE RedVector4 ZeroElement( Red::System::Uint32 _i ) const;

			RED_INLINE RedScalar AsScalar( Red::System::Uint32 _i ) const;
			RED_INLINE RedVector3 AsVector3() const;

			RED_INLINE void InverseRotateDirection( const RedMatrix3x3& _m, const RedVector4& _v );
	
			RED_INLINE Red::System::Bool IsOk() const;

 			RED_INLINE static RedVector4 Lerp( const RedVector4& _a, const RedVector4& _b, const float _weight );
 			RED_INLINE void Lerp( const RedVector4& _a, const float _weight );
		
			static const RedVector4 ZEROS;
			static const RedVector4 ZERO_3D_POINT;
			static const RedVector4 ONES;
			static const RedVector4 EX;
			static const RedVector4 EY;
			static const RedVector4 EZ;
			static const RedVector4 EW;

			union
			{
				struct
				{
					Red::System::Float X;
					Red::System::Float Y;
					Red::System::Float Z;
					Red::System::Float W;
				};

				struct 
				{
					Red::System::Uint32 Xi;
					Red::System::Uint32 Yi;
					Red::System::Uint32 Zi;
					Red::System::Uint32 Wi;
				};

				Red::System::Float f[4];
				SIMDVector V;
			};
		};
	}
}

#endif // _RED_MATH_LIB_REDVECTOR4_SIMD_H_