/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _RED_MATH_LIB_MATRIX_SIMD_H_
#define _RED_MATH_LIB_MATRIX_SIMD_H_

#include "../redSystem/types.h"
#include "redScalar_simd.h"
#include "redVector4_simd.h"
#include "math.h"
#include "mathTypes.h"

namespace RedMath
{
	namespace SIMD
	{
		RED_ALIGNED_CLASS( RedMatrix4x4, 16 )
		{
		public:

			// Constructors
			RED_INLINE RedMatrix4x4();
			RED_INLINE RedMatrix4x4( const RedMatrix4x4& _m );
			RED_INLINE RedMatrix4x4( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2, const RedVector4& _r3 );
			RED_INLINE RedMatrix4x4( const RedVector3& _r0, const RedVector3& _r1, const RedVector3& _r2 );
			RED_INLINE RedMatrix4x4( const Red::System::Float* _f );

			// Destructor
			RED_INLINE ~RedMatrix4x4();

			// Operators
			RED_INLINE RedMatrix4x4& operator=( const RedMatrix4x4& _m );

			// Methods
			RED_INLINE const Red::System::Float* AsFloat() const;

			RED_INLINE void Set( const RedMatrix4x4& _m );
			RED_INLINE void Set( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2, const RedVector4& _r3 );
			RED_INLINE void Set( const RedVector3& _r0, const RedVector3& _r1, const RedVector3& _r2 );
			RED_INLINE void Set33( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2 );

			RED_INLINE void SetZeros();
			RED_INLINE void SetIdentity();

			RED_INLINE void SetRows( const Red::System::Float* _f );
			RED_INLINE void SetRows( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2, const RedVector4& _r3 );
			RED_INLINE void SetRow( Red::System::Uint32 _index, const RedVector4& _a );
			RED_INLINE const RedVector4& GetRow( Red::System::Uint32 _index ) const;

			RED_INLINE void SetCols( const Red::System::Float* _f );
			RED_INLINE void SetCols( const RedVector4& _c0, const RedVector4& _c1, const RedVector4& _c2, const RedVector4& _c3 );
			RED_INLINE void SetColumn( Red::System::Uint32 _index, const RedVector4& _a );
			RED_INLINE RedVector4 GetColumn( Red::System::Uint32 _index ) const;

			RED_INLINE const RedVector4& GetAxisX() const;
			RED_INLINE const RedVector4& GetAxisY() const;
			RED_INLINE const RedVector4& GetAxisZ() const;
			RED_INLINE void SetRotX( Red::System::Float _ccwRadians );
			RED_INLINE void SetRotY( Red::System::Float _ccwRadians );
			RED_INLINE void SetRotZ( Red::System::Float _ccwRadians );

			RED_INLINE void SetScale( const RedScalar& _uniformScale );
			RED_INLINE void SetScale( Red::System::Float _uniformScale );
			RED_INLINE void SetScale( const RedVector3& _scale );
			RED_INLINE void SetScale( const RedVector4& _scale );
			RED_INLINE void SetScale33( const RedVector4& _scale );

			RED_INLINE void SetPreScale44( const RedVector4& _scale );
			RED_INLINE void SetPreScale33( const RedVector4& _scale );

			RED_INLINE RedVector4 GetScale() const;
			RED_INLINE RedVector4 GetPreScale() const;

			RED_INLINE void SetTranslation( const RedVector4& _v );
			RED_INLINE void SetTranslation( Red::System::Float _x, Red::System::Float _y, Red::System::Float _z, Red::System::Float _w = 1.0f );
			RED_INLINE RedVector4 GetTranslation() const;
			RED_INLINE const RedVector4& GetTranslationRef() const;

			// Determinant
			RedScalar Det() const;
			RED_INLINE RedScalar CoFactor( Red::System::Uint32 _i, Red::System::Uint32 _j ) const;

			// Invert/Transpose
			void Invert();
			RED_INLINE RedMatrix4x4 Inverted() const;
			RED_INLINE void Transpose();
			RED_INLINE RedMatrix4x4 Transposed() const;

			RED_INLINE void GetColumnMajor( Red::System::Float* _f ) const;

			// Checks for bad values (denormals or infinities).
			RED_INLINE Red::System::Bool IsOk() const;

			RED_INLINE RedQuaternion ToQuaternion() const;
			RED_INLINE RedEulerAngles ToEulerAngles() const;
			RED_INLINE RedEulerAngles ToEulerAnglesFull() const;

			// Some predefined matrices
			static const RedMatrix4x4 ZEROS;
			static const RedMatrix4x4 IDENTITY;

			union
			{
				struct
				{
					RedVector4 Row0;
					RedVector4 Row1;
					RedVector4 Row2;
					RedVector4 Row3;
				};

				struct 
				{
					Red::System::Float m00;
					Red::System::Float m01;
					Red::System::Float m02;
					Red::System::Float m03;
					Red::System::Float m10;
					Red::System::Float m11;
					Red::System::Float m12;
					Red::System::Float m13;
					Red::System::Float m20;
					Red::System::Float m21;
					Red::System::Float m22;
					Red::System::Float m23;
					Red::System::Float m30;
					Red::System::Float m31;
					Red::System::Float m32;
					Red::System::Float m33;
				};

				Red::System::Float Data[16];
			};
		};
	}
}

#endif // _RED_MATH_LIB_MATRIX_SIMD_H_