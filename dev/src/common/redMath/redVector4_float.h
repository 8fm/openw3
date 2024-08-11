/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _REDMATH_LIB_REDVECTOR4_FLOAT_H_
#define _REDMATH_LIB_REDVECTOR4_FLOAT_H_
#include "../redSystem/types.h"
#include "numericalutils.h"
#include "redScalar_float.h"
#include "redVector3_float.h"
#include "mathTypes.h"

namespace RedMath
{
	namespace FLOAT
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

			// Destructor
			RED_INLINE ~RedVector4();

			RED_INLINE const Red::System::Float* AsFloat() const;

			// Operators
			RED_INLINE RedVector4& operator=( const RedScalar& _v );
			RED_INLINE RedVector4& operator=( const RedVector3& _v );
			RED_INLINE RedVector4& operator=( const RedVector4& _v );

			// Methods
			RED_INLINE void Set( const RedScalar& _v );
			RED_INLINE void Set( const RedVector3& _v );
			RED_INLINE void Set( const RedVector3& _v, Red::System::Float _w );
			RED_INLINE void Set( const RedVector4& _v );
			RED_INLINE void Set( Red::System::Float _x, Red::System::Float _y, Red::System::Float _z, Red::System::Float _w = 0.0f );
			RED_INLINE void Set( const Red::System::Float* _f );
			
			/*
			RED_INLINE void SetTransformedPos( const RedQsTransform& _trans, const RedVector4& _v ); 
			RED_INLINE void SetTransformedInversePos( const RedQsTransform& _t, const RedVector4& _v );
			*/

			RED_INLINE void SetZeros();
			RED_INLINE void SetOnes();

			RED_INLINE RedVector4& Negate();
			RED_INLINE RedVector4 Negated() const;
			RED_INLINE RedVector4 Abs() const;

			RED_INLINE RedScalar Sum3() const;
			RED_INLINE RedScalar Sum4() const;
			RED_INLINE RedScalar Length3() const;
			RED_INLINE RedScalar Length4() const;
			RED_INLINE RedScalar SquareLength3() const;
			RED_INLINE RedScalar SquareLength4() const;
				
			/*
			RED_INLINE float DistanceTo( const RedVector4& _t ) const;
			RED_INLINE float DistanceSquaredTo( const RedVector4& _t ) const;
			RED_INLINE float DistanceTo2D( const RedVector4& _t ) const;
			RED_INLINE float DistanceSquaredTo2D( const RedVector4& _t ) const;
			RED_INLINE float DistanceToEdge( const RedVector4& _a, const RedVector4& _b ) const;
			RED_INLINE RedVector4 NearestPointOnEdge( const RedVector4& _a, const RedVector4& _b ) const;
			*/

			RED_INLINE RedVector4& Normalize4();
			RED_INLINE RedVector4 Normalized4() const;
			RED_INLINE RedVector4& Normalize3();
			RED_INLINE RedVector4 Normalized3() const;

			RED_INLINE Red::System::Bool IsNormalized4( Red::System::Float _epsilon = RED_EPSILON ) const;
			RED_INLINE Red::System::Bool IsNormalized3( Red::System::Float _epsilon = RED_EPSILON ) const;

			RED_INLINE Red::System::Bool IsAlmostEqual( const RedVector4& _v, Red::System::Float _epsilon = RED_EPSILON ) const;
			RED_INLINE Red::System::Bool IsAlmostZero(  Red::System::Float _epsilon = RED_EPSILON ) const;
			RED_INLINE Red::System::Bool IsZero() const;

			RED_INLINE RedScalar Upper3() const;
			RED_INLINE RedScalar Upper4() const;
			RED_INLINE RedScalar Lower3() const;
			RED_INLINE RedScalar Lower4() const;

			RED_INLINE RedVector4 ZeroElement( Red::System::Uint32 _i ) const;

			RED_INLINE RedScalar AsScalar( Red::System::Uint32 _i ) const;
			RED_INLINE RedVector3 AsVector3() const;

			RED_INLINE Red::System::Bool IsOk() const;

			/*
			RED_INLINE static RedVector4 Max( const RedVector4& _a, const RedVector4& _b );
			RED_INLINE static RedVector4 Min( const RedVector4& _a, const RedVector4& _b );
			RED_INLINE static RedVector4 Clamp( const RedVector4& _a, float _min, float _max );
			*/


			// I don't think these functions are required they just bloat the code.
			/*
			static RedVector4 Add( const RedVector4& _a, const RedVector4& _b );
			static RedVector4 Sub( const RedVector4& _a, const RedVector4& _b );
			static RedVector4 Mul( const RedVector4& _a, const RedVector4& _b );
			static RedVector4 Div( const RedVector4& _a, const RedVector4& _b );

			static RedVector4 Add( const RedVector4& _a, float _f );
			static RedVector4 Sub( const RedVector4& _a, float _f );
			static RedVector4 Mul( const RedVector4& _a, float _f );
			static RedVector4 Div( const RedVector4& _a, float _f );

			RedVector4 Add( const RedVector4& _v );
			RedVector4 Sub( const RedVector4& _v );
			RedVector4 Mul( const RedVector4& _v );
			RedVector4 Div( const RedVector4& _v );

			RedVector4 Add( float _f );
			RedVector4 Sub( float _f );
			RedVector4 Mul( float _f );
			RedVector4 Div( float _f );
			*/

			/*
			RED_INLINE static bool Equal( const RedVector4& _a, const RedVector2& _b );
			RED_INLINE static bool Equal( const RedVector4& _a, const RedVector3& _b );
			RED_INLINE static bool Equal( const RedVector4& _a, const RedVector4& _b );

			RED_INLINE static bool Near( const RedVector4& _a, const RedVector2& _b, float _epsilon = FLT_EPSILON );
			RED_INLINE static bool Near( const RedVector4& _a, const RedVector3& _b, float _epsilon = FLT_EPSILON );
			RED_INLINE static bool Near( const RedVector4& _a, const RedVector4& _b, float _epsilon = FLT_EPSILON );

			RED_INLINE static float Dot( const RedVector4& _a, const RedVector2& _b );
			RED_INLINE static float Dot( const RedVector4& _a, const RedVector3& _b );
			RED_INLINE static float Dot( const RedVector4& _a, const RedVector4& _b );
			RED_INLINE static float Dot2( const RedVector4& _a, const RedVector4& _b );
			RED_INLINE static float Dot3( const RedVector4& _a, const RedVector4& _b );

			RED_INLINE static RedVector4 Cross( const RedVector4& _a, const RedVector4& _b, float w = 1.0f );
			RED_INLINE static float Cross2( const RedVector4& _a, const RedVector4& _b );

			RED_INLINE static RedVector4 Permute( const RedVector4& _a, const RedVector4& _b, unsigned int _x, unsigned int _y, unsigned int _z, unsigned int _w);
			*/

			/*
			RED_INLINE static RedVector4 Lerp( const RedVector4& _a, const RedVector4& _b, const float _weight );
			RED_INLINE void Lerp( const RedVector4& _a, const float _weight );
				
			RED_INLINE void RotateDirection( const RedQuaternion& _quat, const RedVector4& _direction );
			RED_INLINE static void AxisRotateVector( RedVector4& vec, const RedVector4& normAxis, float angle );

			RED_INLINE void InverseRotateDirection( const RedMatrix3x3& _m, const RedVector4& _v );
			RED_INLINE void InverseRotateDirection( const RedQuaternion& _quat, const RedVector4& _v );

			RED_INLINE static RedVector4 Project( const RedVector4& _v, const RedVector4& _onNormal );
				RED_INLINE static void CalculatePerpendicularVector( const RedVector4& _in, RedVector4& _out );
			*/

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
				Red::System::Float V[4];
			};
		};
	}
}

#include "redVector4_float.inl"

#endif // _REDMATH_LIB_REDVECTOR4_FLOAT_H_