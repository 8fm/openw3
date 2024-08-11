/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _REDMATH_LIB_REDMATRIX3X3_FLOAT_H_
#define _REDMATH_LIB_REDMATRIX3X3_FLOAT_H_
#include "redScalar_float.h"
#include "redVector2_float.h"
#include "redVector3_float.h"
#include "redVector4_float.h"

namespace RedMath
{
	namespace FLOAT
	{
		RED_ALIGNED_CLASS( RedMatrix3x3, 16 )
		{
		public:
			// Constructors
			RED_INLINE RedMatrix3x3();
			RED_INLINE RedMatrix3x3( const Red::System::Float* _f );
			RED_INLINE RedMatrix3x3( const RedMatrix3x3& _m );
			RED_INLINE RedMatrix3x3( const RedVector3& _v0, const RedVector3& _v1, const RedVector3& _v2 );

			// Destructor
			RED_INLINE ~RedMatrix3x3();

			// Operators
			RED_INLINE RedMatrix3x3& operator = ( const RedMatrix3x3& _m );

			//////////////////////////////////////////////////////////////////////////
			// Methods
			//////////////////////////////////////////////////////////////////////////
			RED_INLINE const Red::System::Float* AsFloat() const;
			RED_INLINE void Set( const RedMatrix3x3& _m );
			RED_INLINE void Set( const RedVector3& _v0, const RedVector3& _v1, const RedVector3& _v2 );
			RED_INLINE void SetIdentity();
			
			RED_INLINE void SetRows( const Red::System::Float* _f );
			RED_INLINE void SetCols( const Red::System::Float* _f );
			RED_INLINE void SetRows( const RedVector3& _r0, const RedVector3& _r1, const RedVector3& _r2 );
			RED_INLINE void SetCols( const RedVector3& _c0, const RedVector3& _c1, const RedVector3& _c2 );

			RED_INLINE void SetZeros();

			RED_INLINE void SetRotX( Red::System::Float _ccwRadians );
			RED_INLINE void SetRotY( Red::System::Float _ccwRadians );
			RED_INLINE void SetRotZ( Red::System::Float _ccwRadians );

			RED_INLINE void SetScale( const RedVector3& _scale );
			RED_INLINE void SetPreScale( const RedVector3& _scale );
			RED_INLINE void SetScale( const RedScalar& _uniformScale );
			RED_INLINE void SetScale( Red::System::Float _uniformScale );
			RED_INLINE RedVector3 GetScale() const;
			RED_INLINE RedVector3 GetPreScale() const;

			RED_INLINE void SetColumn( Red::System::Uint32 _index, const RedVector3& _v );
			RED_INLINE void SetRow( Red::System::Uint32 _index, const RedVector3& _v );
			RED_INLINE RedVector3 GetColumn( Red::System::Uint32 _index ) const;
			RED_INLINE const RedVector3& GetRow( Red::System::Uint32 _index ) const;
			
			RED_INLINE RedVector3 GetAxisX() const;
			RED_INLINE RedVector3 GetAxisY() const;
			RED_INLINE RedVector3 GetAxisZ() const;

			// Build matrix with EY from given direction vector
			RED_INLINE void BuildFromDirectionVector( const RedVector3& _dirVec );
			RED_INLINE void BuildFromQuaternion( const RedVector4& _quaternion );
			//RED_INLINE void BuildFromQuaternion( const RedQuaternion& _quaternion );
			//RED_INLINE void BuildFromEularAngle( const RedEularAngles& _eular );

			RED_INLINE RedScalar Det() const;

			// Invert/Transpose
			RED_INLINE void Invert();
			RED_INLINE RedMatrix3x3 Inverted() const;
			RED_INLINE void Transpose();
			RED_INLINE RedMatrix3x3 Transposed() const;

			//RED_INLINE RedVector3 TransformVector( const RedVector3& _v ) const;
			RED_INLINE void ExtractScale( RedMatrix3x3& _trMatrix, RedVector3& _scale ) const;

			RED_INLINE Red::System::Bool IsAlmostEqual( const RedMatrix3x3& _m, Red::System::Float _epsilon = FLT_EPSILON ) const;
			RED_INLINE Red::System::Bool IsZero() const;
			RED_INLINE Red::System::Bool IsIdentity() const;

			RED_INLINE Red::System::Bool IsOk() const;

			static const RedMatrix3x3 ZEROS;
			static const RedMatrix3x3 IDENTITY;

			union
			{
				struct
				{
					RedVector3 Row0;
					RedVector3 Row1;
					RedVector3 Row2;
				};

				struct
				{
					RedVector3 Rows[3];
				};

				struct 
				{
					Red::System::Float m00;
					Red::System::Float m01;
					Red::System::Float m02;
					Red::System::Float _unused1;
					Red::System::Float m10;
					Red::System::Float m11;
					Red::System::Float m12;
					Red::System::Float _unused2;
					Red::System::Float m20;
					Red::System::Float m21;
					Red::System::Float m22;
					Red::System::Float _unused3;
				};

				Red::System::Float Data[12];
			};
		};
	}
}

#include "redMatrix3x3_float.inl"

#endif // _REDMATH_LIB_REDMATRIX3X3_FLOAT_H_