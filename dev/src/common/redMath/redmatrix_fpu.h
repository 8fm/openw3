/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _REDMATH_LIB_MATRIX_FLOAT_H_
#define _REDMATH_LIB_MATRIX_FLOAT_H_

namespace Red
{
	namespace Math
	{
		namespace Fpu
		{
			RED_ALIGNED_CLASS( RedMatrix3x3, 16 )
			{
				public:
				RedVector3 Matrix[3];

				RED_INLINE RedMatrix3x3();
				RED_INLINE RedMatrix3x3( const float* _f );
				RED_INLINE RedMatrix3x3( const RedMatrix3x3& _m );
				RED_INLINE RedMatrix3x3( const RedVector3& _v0, const RedVector3& _v1, const RedVector3& _v2 );
			
				// Operators
				RED_INLINE RedMatrix3x3 operator+( const RedMatrix3x3& _m ) const;
				RED_INLINE bool operator==( const RedMatrix3x3& _m ) const;
				RED_INLINE bool operator!=( const RedMatrix3x3& _m ) const;
				RED_INLINE bool IsAlmostEqual( const RedMatrix3x3& _m, float _epsilon = FLT_EPSILON ) const;

				RED_INLINE RedMatrix3x3& operator=( const RedMatrix3x3& _m );
				
				// Multiply matrix
				RED_INLINE RedMatrix3x3 operator*( const RedMatrix3x3& _m );
				RED_INLINE static RedMatrix3x3 Mul( const RedMatrix3x3& _a, const RedMatrix3x3& _b );

				// Setting
				RED_INLINE RedMatrix3x3& Set( const RedMatrix3x3& _m );
				RED_INLINE RedMatrix3x3& Set( const RedVector3& _v0, const RedVector3& _v1, const RedVector3& _v2 );
				RED_INLINE RedMatrix3x3& SetRows( const float* _f );
				RED_INLINE RedMatrix3x3& SetCols( const float* _f );

				RED_INLINE RedMatrix3x3& SetRows( const RedVector3& _r0, const RedVector3& _r1, const RedVector3& _r2 );
				RED_INLINE RedMatrix3x3& SetCols( const RedVector3& _c0, const RedVector3& _c1, const RedVector3& _c2 );
			
				RED_INLINE RedMatrix3x3& SetZeros();
				RED_INLINE RedMatrix3x3& SetIdentity();

				// Column/Row access
				RED_INLINE RedMatrix3x3& SetColumn( int _index, const RedVector3& _v );
				RED_INLINE RedMatrix3x3& SetRow( int _index, const RedVector3& _v );

				RED_INLINE RedVector3 GetColumn( int _index ) const;
				RED_INLINE const RedVector3& GetRow( int _index ) const;

				RED_INLINE RedVector3 GetAxisX() const;
				RED_INLINE RedVector3 GetAxisY() const;
				RED_INLINE RedVector3 GetAxisZ() const;

				// Set rotation
				RED_INLINE RedMatrix3x3& SetRotX( float _ccwRadians );
				RED_INLINE RedMatrix3x3& SetRotY( float _ccwRadians );
				RED_INLINE RedMatrix3x3& SetRotZ( float _ccwRadians );

				// Set/Get Pre-Scale/Scale
				RED_INLINE RedMatrix3x3& SetScale( const RedVector3& _scale );
				RED_INLINE RedMatrix3x3& SetPreScale( const RedVector3& _scale );
				RED_INLINE RedMatrix3x3& SetScale( RedVector1& _uniformScale );
				RED_INLINE RedMatrix3x3& SetScale( float _uniformScale );
				RED_INLINE RedVector3 GetScale() const;
				RED_INLINE RedVector3 GetPreScale() const;

				// Build matrix with EY from given direction vector
				RED_INLINE RedMatrix3x3& BuildFromDirectionVector( const RedVector3& _dirVec );
				RED_INLINE RedMatrix3x3& BuildFromQuaternion( const RedVector4& _quaternion );

				// Determinant
				RED_INLINE float Det() const;

				// Invert/Transpose
				RED_INLINE RedMatrix3x3& Invert();
				RED_INLINE RedMatrix3x3 Inverted() const;
				RED_INLINE RedMatrix3x3& Transpose();
				RED_INLINE RedMatrix3x3 Transposed() const;

				// Transformations
				RED_INLINE RedVector3 TransformVector( const RedVector3& _v ) const;			// Assumed W = 0.0f
				//Box TransformBox( const Box& box ) const;

				// Decompose matrix to the orthonormal part and the scale
				RED_INLINE void ExtractScale( RedMatrix3x3& _trMatrix, RedVector3& _scale ) const;

				// Checks for bad values (denormals or infinities).
				RED_INLINE bool IsOk() const;

				// Some predefined matrices
				static const RedMatrix3x3 ZEROS;
				static const RedMatrix3x3 IDENTITY;
			};

			RED_ALIGNED_CLASS( RedMatrix4x4, 16 )
			{
			public:
				RedVector4 Matrix[4];

				// Constructors
				RED_INLINE RedMatrix4x4();
				RED_INLINE RedMatrix4x4( const float* _f );
				RED_INLINE RedMatrix4x4( const RedMatrix4x4& _m );
				RED_INLINE RedMatrix4x4( const RedMatrix3x3& _m );
				RED_INLINE RedMatrix4x4( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2, const RedVector4& _r3 );
				RED_INLINE RedMatrix4x4( const RedVector3& _r0, const RedVector3& _r1, const RedVector3& _r2 );
				const float* AsFloat() const { return &Matrix[0].X; }

				// Operators
				RED_INLINE bool operator==( const RedMatrix4x4& _m ) const;
				RED_INLINE bool operator==( const RedMatrix3x3& _m ) const;
				RED_INLINE bool operator!=( const RedMatrix4x4& _m ) const;
				RED_INLINE bool operator!=( const RedMatrix3x3& _m ) const;
				RED_INLINE RedMatrix4x4& operator=( const RedMatrix4x4& _m );
				RED_INLINE RedMatrix4x4& operator=( const RedMatrix3x3& _m );
			
				// Multiply matrix
				RED_INLINE RedMatrix4x4 operator*( const RedMatrix4x4& _m ) const;
				RED_INLINE static RedMatrix4x4 Mul( const RedMatrix4x4& _a, const RedMatrix4x4& _b );

				// Setting
				RED_INLINE RedMatrix4x4& Set( const RedMatrix4x4& _m );
				RED_INLINE RedMatrix4x4& Set( const RedMatrix3x3& _m );
				RED_INLINE RedMatrix4x4& Set( const RedVector3& _x, const RedVector3& _y, const RedVector3& _z );
				RED_INLINE RedMatrix4x4& Set( const RedVector4& _x, const RedVector4& _y, const RedVector4& _z, const RedVector4& _w );
				RED_INLINE RedMatrix4x4& Set33( const RedVector4& _x, const RedVector4& _y, const RedVector4& _z );

				RED_INLINE RedMatrix4x4& SetRows( const float* _f );
				RED_INLINE RedMatrix4x4& SetCols( const float* _f );
			
				RED_INLINE RedMatrix4x4& SetRows( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2, const RedVector4& _r3 );
				RED_INLINE RedMatrix4x4& SetCols( const RedVector4& _c0, const RedVector4& _c1, const RedVector4& _c2, const RedVector4& _c3 );

				// Column/Row access
				RED_INLINE RedMatrix4x4& SetColumn( int _index, const RedVector4& _a );
				RED_INLINE RedMatrix4x4& SetRow( int _index, const RedVector4& _a );
				RED_INLINE RedVector4 GetColumn( int _index ) const;
				RED_INLINE const RedVector4& GetRow( int _index ) const;

				RED_INLINE RedVector4 GetAxisX() const;
				RED_INLINE RedVector4 GetAxisY() const;
				RED_INLINE RedVector4 GetAxisZ() const;

				RED_INLINE RedMatrix4x4& SetZeros();
				RED_INLINE RedMatrix4x4& SetIdentity();

				RED_INLINE RedMatrix4x4& SetRotX( float _ccwRadians );
				RED_INLINE RedMatrix4x4& SetRotY( float _ccwRadians );
				RED_INLINE RedMatrix4x4& SetRotZ( float _ccwRadians );

				RED_INLINE RedMatrix4x4& SetScale( const RedVector1& _uniformScale );
				RED_INLINE RedMatrix4x4& SetScale( float _uniformScale );
				RED_INLINE RedMatrix4x4& SetScale( const RedVector3& _scale );
				RED_INLINE RedMatrix4x4& SetScale( const RedVector4& _scale );
				RED_INLINE RedMatrix4x4& SetScale33( const RedVector4& _scale );

				RED_INLINE RedMatrix4x4& SetPreScale44( const RedVector4& _scale );
				RED_INLINE RedMatrix4x4& SetPreScale33( const RedVector4& _scale );
			
				RED_INLINE RedVector4 GetScale() const;
				RED_INLINE RedVector4 GetPreScale() const;

				// Set translation part, only XYZ is used
				RED_INLINE RedMatrix4x4& SetTranslation( const RedVector4& _v );
				RED_INLINE RedMatrix4x4& SetTranslation( float _x, float _y, float _z );
				RED_INLINE RedVector4 GetTranslation() const;
				RED_INLINE const RedVector4& GetTranslationRef() const;

				
				// Convert rotation to euler angles
				RED_INLINE RedEulerAngles ToEulerAngles() const;
				RED_INLINE float GetYaw() const;
				// Convert rotation to euler angles, works properly also for scaled matrices
				RED_INLINE RedEulerAngles ToEulerAnglesFull() const;
				RED_INLINE void ToAngleVectors( RedVector4* _forward, RedVector4* _right, RedVector4* _up ) const;
				RED_INLINE RedQuaternion ToQuaternion() const;

				// Build more complex type of matrices
				RED_INLINE RedMatrix4x4& BuildPerspectiveLH( float _fovy, float _aspect, float _zn, float _zf );
				RED_INLINE RedMatrix4x4& BuildOrthoLH( float _w, float _h, float _zn, float _zf );	

				// Build matrix with EY from given direction vector
				RED_INLINE RedMatrix4x4& BuildFromDirectionVector( const RedVector4& _dirVec );
				RED_INLINE RedMatrix4x4& BuildFromQuaternion( const RedVector4& _quaternion );

				// Determinant
				RED_INLINE float Det() const;
				RED_INLINE float CoFactor( int _i, int _j ) const;

				// Invert/Transpose
				RED_INLINE RedMatrix4x4& Invert();
				RED_INLINE RedMatrix4x4& FullInvert();
				RED_INLINE RedMatrix4x4& Transpose();
				RED_INLINE RedMatrix4x4 Inverted() const;
				RED_INLINE RedMatrix4x4 FullInverted() const;
				RED_INLINE RedMatrix4x4 Transposed() const;

				RED_INLINE void GetColumnMajor( float* _data ) const;

				// Transformations
				RED_INLINE RedVector4 TransformVector( const RedVector4& _v ) const;			// Assumed W = 0.0f
				RED_INLINE RedVector4 TransformVectorWithW( const RedVector4& _v ) const;		// W used directly
				RED_INLINE RedVector4 TransformVectorAsPoint( const RedVector4& _v ) const;	// Assumed W = 1.0f
				RED_INLINE RedVector4 TransformPoint( const RedVector4& _v ) const;			// Assumed W = 1.0f

				// Decompose matrix to the orthonormal part and the scale
				RED_INLINE void ExtractScale( RedMatrix4x4& _trMatrix, RedVector4& _scale ) const;

				// Checks for bad values (denormals or infinities).
				RED_INLINE bool IsOk() const;

				// Some predefined matrices
				static const RedMatrix4x4 ZEROS;
				static const RedMatrix4x4 IDENTITY;
			};
		};
	};
};

#endif // _REDMATH_LIB_MATRIX_FLOAT_H_