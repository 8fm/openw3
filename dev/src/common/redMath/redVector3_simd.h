/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _REDMATH_LIB_REDVECTOR3_SIMD_H_
#define _REDMATH_LIB_REDVECTOR3_SIMD_H_
#include "../redSystem/types.h"
#include "redScalar_simd.h"
#include "mathTypes.h"

namespace RedMath
{
	namespace SIMD
	{
		RED_ALIGNED_CLASS( RedVector3, 16 )
		{
		public:
			// Constructors
			RED_INLINE RedVector3();
			RED_INLINE RedVector3( const RedScalar& _v );
			RED_INLINE RedVector3( const RedVector3& _v );
			RED_INLINE RedVector3( Red::System::Float _x, Red::System::Float _y, Red::System::Float _z );
			RED_INLINE RedVector3( const Red::System::Float* _f );
			RED_INLINE RedVector3( SIMDVector _v );
			// Destructor
			RED_INLINE ~RedVector3();

			// Operators
			RED_INLINE RedVector3& operator = ( const RedScalar& _v );

			RED_INLINE const Red::System::Float* AsFloat() const;
			RED_INLINE void Store( Red::System::Float* _f ) const;
			//////////////////////////////////////////////////////////////////////////
			// Methods
			//////////////////////////////////////////////////////////////////////////
			RED_INLINE void Set( Red::System::Float _x, Red::System::Float _y, Red::System::Float _z );
			RED_INLINE void Set( const RedScalar& _v );
			RED_INLINE void Set( const RedVector3& _v );
			RED_INLINE void Set( const Red::System::Float* _f );

			RED_INLINE void SetZeros();
			RED_INLINE void SetOnes();

			RED_INLINE RedVector3& Negate();
			RED_INLINE RedVector3 Negated() const;
			RED_INLINE RedVector3 Abs() const;

			RED_INLINE RedScalar Length() const;
			RED_INLINE RedScalar SquareLength() const;
			RED_INLINE RedVector3& Normalize();
			RED_INLINE RedVector3 Normalized() const;
			RED_INLINE RedVector3& NormalizeFast();
			RED_INLINE RedVector3 NormalizedFast();

			RED_INLINE Red::System::Bool IsAlmostEqual( const RedVector3& _v, const SIMDVector _epsilon = EPSILON_VALUE ) const;
			RED_INLINE Red::System::Bool IsAlmostEqual( const RedVector3& _v, float _epsilon ) const;
			RED_INLINE Red::System::Bool IsZero() const;
			RED_INLINE Red::System::Bool IsAlmostZero( const SIMDVector _epsilon = EPSILON_VALUE ) const;

			RED_INLINE Red::System::Bool IsOk() const;

			static const RedVector3 ZEROS;
			static const RedVector3 ONES;
			static const RedVector3 EX;
			static const RedVector3 EY;
			static const RedVector3 EZ;

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
				SIMDVector V;
				float f[4];
			};
		};
	}
}

#endif // _REDMATH_LIB_REDVECTOR3_SIMD_H_