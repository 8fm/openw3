/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _REDMATH_LIB_REDVECTOR_FLOAT_H_
#define _REDMATH_LIB_REDVECTOR_FLOAT_H_
#ifdef _MSC_VER
RED_WARNING_PUSH()
RED_DISABLE_WARNING_MSC(4201) // nonstandard extension used : nameless struct/union
#endif
namespace Red 
{ 
	namespace Math
	{
		namespace Fpu
		{
			RED_ALIGNED_CLASS( RedVector1, 16 )
			{
			public:
				union
				{
					struct
					{
						float X, Y, Z, W;
					};
					float V[4];
				};
				// Constructor
				RED_INLINE RedVector1();
				RED_INLINE RedVector1( const float _f);									
				RED_INLINE RedVector1( const RedVector1& _v);

				// Destructor
				RED_INLINE ~RedVector1();

				const float* AsFloat() const { return &X; }

				// Operator Overloads
				RED_INLINE RedVector1 operator-() const;

				RED_INLINE RedVector1 operator+( const RedVector1& _v ) const;
				RED_INLINE RedVector1 operator-( const RedVector1& _v ) const;
				RED_INLINE RedVector1 operator*( const RedVector1& _v ) const;
				RED_INLINE RedVector1 operator/( const RedVector1& _v ) const;

				RED_INLINE RedVector1 operator+( float _f ) const;
				RED_INLINE RedVector1 operator-( float _f ) const;
				RED_INLINE RedVector1 operator*( float _f ) const;
				RED_INLINE RedVector1 operator/( float _f ) const;

				RED_INLINE RedVector1& operator=( const RedVector1& _v );
				RED_INLINE RedVector1& operator+=( const RedVector1& _v );
				RED_INLINE RedVector1& operator-=( const RedVector1& _v );
				RED_INLINE RedVector1& operator*=( const RedVector1& _v );
				RED_INLINE RedVector1& operator/=( const RedVector1& _v );

				RED_INLINE RedVector1& operator+=( float _f );
				RED_INLINE RedVector1& operator-=( float _f );
				RED_INLINE RedVector1& operator*=( float _f );
				RED_INLINE RedVector1& operator/=( float _f );

				RED_INLINE bool operator==( const RedVector1& _v ) const;
				RED_INLINE bool operator!=( const RedVector1& _v ) const;

				// Setting
				RED_INLINE RedVector1 Set( float _f );
				RED_INLINE RedVector1 Set( const RedVector1& _v );

				// Special Sets
				RED_INLINE RedVector1& SetZeros();
				RED_INLINE RedVector1& SetOnes();

				// Negate vector
				RED_INLINE RedVector1& Negate();
				RED_INLINE RedVector1 Negated() const;

				// Make Abs vector
				RED_INLINE RedVector1 Abs() const;

				// Checks
				RED_INLINE bool IsZero() const;
				RED_INLINE bool IsAlmostZero(float _epsilon = FLT_EPSILON) const;

			};

			RED_ALIGNED_CLASS( RedVector2, 16 )
			{
			public:
				union
				{
					struct
					{
						float X, Y, Z, W;
					};
					float V[4];
				};

				// Constructors
				RED_INLINE RedVector2();
				RED_INLINE RedVector2( const RedVector1& _v );
				RED_INLINE RedVector2( const RedVector2& _v );
				RED_INLINE RedVector2( float _x, float _y );
				RED_INLINE RedVector2( const float* _f );

				// Destructor
				RED_INLINE ~RedVector2();

				// Operators
				RED_INLINE RedVector2& operator=( const RedVector1& _v );
				RED_INLINE RedVector2& operator=( const RedVector2& _v );

				RED_INLINE RedVector2 operator+( float _f ) const;
				RED_INLINE RedVector2 operator-( float _f ) const;
				RED_INLINE RedVector2 operator*( float _f ) const;
				RED_INLINE RedVector2 operator/( float _f ) const;
			
				RED_INLINE RedVector2 operator+( const RedVector1& _v ) const;
				RED_INLINE RedVector2 operator-( const RedVector1& _v ) const;
				RED_INLINE RedVector2 operator*( const RedVector1& _v ) const;
				RED_INLINE RedVector2 operator/( const RedVector1& _v ) const;
				RED_INLINE RedVector2 operator+( const RedVector2& _v ) const;
				RED_INLINE RedVector2 operator-( const RedVector2& _v ) const;
				RED_INLINE RedVector2 operator*( const RedVector2& _v ) const;
				RED_INLINE RedVector2 operator/( const RedVector2& _v ) const;

				RED_INLINE RedVector2& operator+=( const RedVector1& _v);
				RED_INLINE RedVector2& operator-=( const RedVector1& _v);
				RED_INLINE RedVector2& operator*=( const RedVector1& _v);
				RED_INLINE RedVector2& operator/=( const RedVector1& _v);
				RED_INLINE RedVector2& operator+=( const RedVector2& _v);
				RED_INLINE RedVector2& operator-=( const RedVector2& _v);
				RED_INLINE RedVector2& operator*=( const RedVector2& _v);
				RED_INLINE RedVector2& operator/=( const RedVector2& _v);
			
				RED_INLINE bool operator==(const RedVector1& _v) const;
				RED_INLINE bool operator!=(const RedVector1& _v) const;

				RED_INLINE bool operator==(const RedVector2& _v) const;
				RED_INLINE bool operator!=(const RedVector2& _v) const;

				// Methods
				RED_INLINE float Length() const;
				RED_INLINE float SquareLength() const;
				RED_INLINE RedVector2& Normalize();
				RED_INLINE RedVector2 Normalized() const;
				RED_INLINE float Dot( const RedVector2& _v ) const;
				RED_INLINE void Set( float _x, float _y );
				RED_INLINE float CrossZ( const RedVector2& _v ) const;

				static const RedVector2 ZEROS;
				static const RedVector2 ONES;
				static const RedVector2 EX;
				static const RedVector2 EY;
			};

			RED_ALIGNED_CLASS( RedVector3, 16 )
			{
			public:
				union
				{
					struct
					{
						float X, Y, Z, W;
					};
					float V[4];
				};
				// Constructors
				RED_INLINE RedVector3();
				RED_INLINE RedVector3( const RedVector1& _v );
				RED_INLINE RedVector3( const RedVector2& _v );
				RED_INLINE RedVector3( const RedVector3& _v );
				RED_INLINE RedVector3( float _f );
				RED_INLINE RedVector3( const float* _f );
				RED_INLINE RedVector3( float _x, float _y, float _z );

				// Destructor
				RED_INLINE ~RedVector3();

				// Operators
				RED_INLINE RedVector3& operator=( const RedVector1& _v );
				RED_INLINE RedVector3& operator=( const RedVector2& _v );
				RED_INLINE RedVector3& operator=( const RedVector3& _v );

				RED_INLINE RedVector3 operator+( float _f ) const;
				RED_INLINE RedVector3 operator-( float _f ) const;
				RED_INLINE RedVector3 operator*( float _f ) const;
				RED_INLINE RedVector3 operator/( float _f ) const;

				RED_INLINE RedVector3 operator+( const RedVector1& _v ) const;
				RED_INLINE RedVector3 operator-( const RedVector1& _v ) const;
				RED_INLINE RedVector3 operator*( const RedVector1& _v ) const;
				RED_INLINE RedVector3 operator/( const RedVector1& _v ) const;
				RED_INLINE RedVector3 operator+( const RedVector2& _v ) const;
				RED_INLINE RedVector3 operator-( const RedVector2& _v ) const;
				RED_INLINE RedVector3 operator*( const RedVector2& _v ) const;
				RED_INLINE RedVector3 operator/( const RedVector2& _v ) const;
				RED_INLINE RedVector3 operator+( const RedVector3& _v ) const;
				RED_INLINE RedVector3 operator-( const RedVector3& _v ) const;
				RED_INLINE RedVector3 operator*( const RedVector3& _v ) const;
				RED_INLINE RedVector3 operator/( const RedVector3& _v ) const;

				RED_INLINE RedVector3& operator+=( const RedVector1& _v);
				RED_INLINE RedVector3& operator-=( const RedVector1& _v);
				RED_INLINE RedVector3& operator*=( const RedVector1& _v);
				RED_INLINE RedVector3& operator/=( const RedVector1& _v);
				RED_INLINE RedVector3& operator+=( const RedVector2& _v);
				RED_INLINE RedVector3& operator-=( const RedVector2& _v);
				RED_INLINE RedVector3& operator*=( const RedVector2& _v);
				RED_INLINE RedVector3& operator/=( const RedVector2& _v);
				RED_INLINE RedVector3& operator+=( const RedVector3& _v);
				RED_INLINE RedVector3& operator-=( const RedVector3& _v);
				RED_INLINE RedVector3& operator*=( const RedVector3& _v);
				RED_INLINE RedVector3& operator/=( const RedVector3& _v);

				RED_INLINE bool operator==( const RedVector3& _v ) const;
				RED_INLINE bool operator!=( const RedVector3& _v ) const;

				RED_INLINE RedVector3& SetZeros();
				RED_INLINE RedVector3& SetOnes();

				// Methods
				RED_INLINE bool IsAlmostEqual( const RedVector3& _v, float _epsilon = FLT_EPSILON ) const;
				RED_INLINE bool IsZero() const;
				RED_INLINE bool IsAlmostZero(float _epsilon = FLT_EPSILON) const;
				RED_INLINE float Length() const;
				RED_INLINE float SquareLength() const;
				RED_INLINE RedVector3& Normalize();
				RED_INLINE RedVector3 Normalized() const;
				RED_INLINE float Dot( const RedVector3& _v ) const;

				RED_INLINE RedVector3 Cross( const RedVector3& _v ) const;
				RED_INLINE static RedVector3 Cross( const RedVector3& _a, const RedVector3& _b );

				RED_INLINE void Set( float _x, float _y, float _z );

				RED_INLINE bool IsOk() const;

				static const RedVector3 ZEROS;
				static const RedVector3 ONES;
				static const RedVector3 EX;
				static const RedVector3 EY;
				static const RedVector3 EZ;

			};

			RED_ALIGNED_CLASS( RedVector4, 16 )
			{
			public:
				union
				{
					struct
					{
						float X, Y, Z, W;
					};

					float V[4];
				};

				// Constructors
				RED_INLINE RedVector4();
				RED_INLINE RedVector4( const RedVector1& _v );
				RED_INLINE RedVector4( const RedVector2& _v );
				RED_INLINE RedVector4( const RedVector3& _v );
				RED_INLINE RedVector4( const RedVector3& _v, float _w );
				RED_INLINE RedVector4( const RedVector4& _v );
				RED_INLINE RedVector4( float _f );
				RED_INLINE RedVector4( const float* _f );
				RED_INLINE RedVector4( float _x, float _y, float _z, float _w = 1.0f );

				// Destructor
				RED_INLINE ~RedVector4();
				// Operators
				RED_INLINE RedVector4& operator=( const RedVector1& _v );
				RED_INLINE RedVector4& operator=( const RedVector2& _v );
				RED_INLINE RedVector4& operator=( const RedVector3& _v );
				RED_INLINE RedVector4& operator=( const RedVector4& _v );

				RED_INLINE RedVector4 operator+( float _f ) const;
				RED_INLINE RedVector4 operator-( float _f ) const;
				RED_INLINE RedVector4 operator*( float _f ) const;
				RED_INLINE RedVector4 operator/( float _f ) const;
				RED_INLINE RedVector4& operator+=( const float _f );
				RED_INLINE RedVector4& operator-=( const float _f );
				RED_INLINE RedVector4& operator*=( const float _f );
				RED_INLINE RedVector4& operator/=( const float _f );


				RED_INLINE RedVector4 operator+( const RedVector1& _v ) const;
				RED_INLINE RedVector4 operator-( const RedVector1& _v ) const;
				RED_INLINE RedVector4 operator*( const RedVector1& _v ) const;
				RED_INLINE RedVector4 operator/( const RedVector1& _v ) const;
				RED_INLINE RedVector4 operator+( const RedVector2& _v ) const;
				RED_INLINE RedVector4 operator-( const RedVector2& _v ) const;
				RED_INLINE RedVector4 operator*( const RedVector2& _v ) const;
				RED_INLINE RedVector4 operator/( const RedVector2& _v ) const;
				RED_INLINE RedVector4 operator+( const RedVector3& _v ) const;
				RED_INLINE RedVector4 operator-( const RedVector3& _v ) const;
				RED_INLINE RedVector4 operator*( const RedVector3& _v ) const;
				RED_INLINE RedVector4 operator/( const RedVector3& _v ) const;
				RED_INLINE RedVector4 operator+( const RedVector4& _v ) const;
				RED_INLINE RedVector4 operator-( const RedVector4& _v ) const;
				RED_INLINE RedVector4 operator*( const RedVector4& _v ) const;
				RED_INLINE RedVector4 operator/( const RedVector4& _v ) const;

				RED_INLINE RedVector4& operator+=( const RedVector1& _v );
				RED_INLINE RedVector4& operator-=( const RedVector1& _v );
				RED_INLINE RedVector4& operator*=( const RedVector1& _v );
				RED_INLINE RedVector4& operator/=( const RedVector1& _v );
				RED_INLINE RedVector4& operator+=( const RedVector2& _v );
				RED_INLINE RedVector4& operator-=( const RedVector2& _v );
				RED_INLINE RedVector4& operator*=( const RedVector2& _v );
				RED_INLINE RedVector4& operator/=( const RedVector2& _v );
				RED_INLINE RedVector4& operator+=( const RedVector3& _v );
				RED_INLINE RedVector4& operator-=( const RedVector3& _v );
				RED_INLINE RedVector4& operator*=( const RedVector3& _v );
				RED_INLINE RedVector4& operator/=( const RedVector3& _v );
				RED_INLINE RedVector4& operator+=( const RedVector4& _v );
				RED_INLINE RedVector4& operator-=( const RedVector4& _v );
				RED_INLINE RedVector4& operator*=( const RedVector4& _v );
				RED_INLINE RedVector4& operator/=( const RedVector4& _v );

				RED_INLINE bool operator==( const RedVector4& _v ) const;
				RED_INLINE bool operator==( const RedVector3& _v ) const;

				RED_INLINE bool operator!=( const RedVector4& _v ) const;
				RED_INLINE bool operator!=( const RedVector3& _v ) const;

				RED_INLINE bool IsAlmostEqual( const RedVector4& _v, float _epsilon = FLT_EPSILON ) const;

				// Methods;
				RED_INLINE RedVector4& Set( float _x );
				RED_INLINE RedVector4& Set( const RedVector1& _v );
				RED_INLINE RedVector4& Set( float _x, float _y, float _z, float _w );
				RED_INLINE RedVector4& Set( const RedVector3& _v, float _w );
				RED_INLINE RedVector4& Set( const RedVector4& _v );
				RED_INLINE RedVector4& SetTransformedPos( const RedQsTransform& _trans, const RedVector4& _v ); 
				RED_INLINE RedVector4& SetTransformedInversePos( const RedQsTransform& _t, const RedVector4& _v );

				RED_INLINE RedVector4& SetZeros();
				RED_INLINE RedVector4& SetOnes();

				RED_INLINE RedVector4& Negate();
				RED_INLINE RedVector4 Negated() const;
				RED_INLINE RedVector4 Abs() const;

				RED_INLINE float Sum() const;
				RED_INLINE float Length() const;
				RED_INLINE float SquareLength() const;
				RED_INLINE float Sum2() const;
				RED_INLINE float Length2() const;
				RED_INLINE float SquareLength2() const;
				RED_INLINE float Sum3() const;
				RED_INLINE float Length3() const;
				RED_INLINE float SquareLength3() const;

				RED_INLINE float DistanceTo( const RedVector4& _t ) const;
				RED_INLINE float DistanceSquaredTo( const RedVector4& _t ) const;
				RED_INLINE float DistanceTo2D( const RedVector4& _t ) const;
				RED_INLINE float DistanceSquaredTo2D( const RedVector4& _t ) const;
				RED_INLINE float DistanceToEdge( const RedVector4& _a, const RedVector4& _b ) const;
				RED_INLINE RedVector4 NearestPointOnEdge( const RedVector4& _a, const RedVector4& _b ) const;

				RED_FORCE_INLINE RedVector4& Normalize();
				RED_INLINE RedVector4& Normalize2();
				RED_INLINE RedVector4& Normalize3();
				RED_INLINE RedVector4 Normalized() const;
				RED_INLINE RedVector4 Normalized2() const;
				RED_INLINE RedVector4 Normalized3() const;

				RED_INLINE bool IsNormalized( float _epsilon = 1e-4f ) const;
				RED_INLINE bool IsNormalized2( float _epsilon = 1e-4f ) const;
				RED_INLINE bool IsNormalized3( float _epsilon = 1e-4f ) const;

				RED_INLINE static RedVector4 Max( const RedVector4& _a, const RedVector4& _b );
				RED_INLINE static RedVector4 Min( const RedVector4& _a, const RedVector4& _b );

				RED_INLINE static RedVector4 Clamp( const RedVector4& _a, float _min, float _max );

				RED_INLINE float Upper3() const;
				RED_INLINE float Upper4() const;
				RED_INLINE float Lower3() const;
				RED_INLINE float Lower4() const;

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

				RED_INLINE RedVector4 ZeroElement( unsigned int _i ) const;

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
				

				RED_INLINE const RedVector1& AsVector1() const;
				RED_INLINE RedVector1& AsVector1();
				RED_INLINE const RedVector2& AsVector2() const;
				RED_INLINE RedVector2& AsVector2();
				RED_INLINE const RedVector3& AsVector3() const;
				RED_INLINE RedVector3& AsVector3();

				RED_INLINE RedVector4 Position();
				RED_INLINE const RedVector4 Position() const;

				RED_INLINE static RedVector4 Lerp( const RedVector4& _a, const RedVector4& _b, const float _weight );
				RED_INLINE void Lerp( const RedVector4& _a, const float _weight );

				RED_FORCE_INLINE void RotateDirection( const RedQuaternion& _quat, const RedVector4& _direction );
				RED_INLINE static void AxisRotateVector( RedVector4& vec, const RedVector4& normAxis, float angle );

				RED_INLINE void InverseRotateDirection( const RedMatrix3x3& _m, const RedVector4& _v );
				RED_INLINE void InverseRotateDirection( const RedQuaternion& _quat, const RedVector4& _v );

				RED_INLINE static RedVector4 Project( const RedVector4& _v, const RedVector4& _onNormal );
			
				RED_INLINE bool IsOk() const;

				RED_INLINE unsigned int GetConvertedToUByte4Color() const;

				RED_INLINE static void CalculatePerpendicularVector( const RedVector4& _in, RedVector4& _out );

				static const RedVector4 ZEROS;
				static const RedVector4 ZERO_3D_POINT;
				static const RedVector4 ONES;
				static const RedVector4 EX;
				static const RedVector4 EY;
				static const RedVector4 EZ;
				static const RedVector4 EW;
			};
		};
	};
};
#ifdef _MSC_VER
RED_WARNING_POP()
#endif

#endif // _REDMATH_LIB_REDVECTOR_FLOAT_H_
