#include "redmathbase.h"

#ifndef RED_ENABLE_SIMD_MATH

	namespace Red
	{
		namespace Math
		{
			namespace Fpu
			{
				// RedVector2 Constants
				const RedVector2 RedVector2::ZEROS( 0.0f, 0.0f );
				const RedVector2 RedVector2::ONES( 1.0f, 1.0f );
				const RedVector2 RedVector2::EX( 1.0f, 0.0f );
				const RedVector2 RedVector2::EY( 0.0f, 1.0f );
			
				// RedVector3 Constants
				const RedVector3 RedVector3::ZEROS( 0.0f, 0.0f, 0.0f );
				const RedVector3 RedVector3::ONES( 1.0f, 1.0f, 1.0f );
				const RedVector3 RedVector3::EX( 1.0f, 0.0f, 0.0f );
				const RedVector3 RedVector3::EY( 0.0f, 1.0f, 0.0f );
				const RedVector3 RedVector3::EZ( 0.0f, 0.0f, 1.0f );

				// RedVector4 Constants
				const RedVector4 RedVector4::ZEROS( 0.0f, 0.0f, 0.0f, 0.0f );
				const RedVector4 RedVector4::ZERO_3D_POINT( 0.0f, 0.0f, 0.0f, 1.0f);
				const RedVector4 RedVector4::ONES( 1.0f, 1.0f, 1.0f, 1.0f );
				const RedVector4 RedVector4::EX( 1.0f, 0.0f, 0.0f, 0.0f );
				const RedVector4 RedVector4::EY( 0.0f, 1.0f, 0.0f, 0.0f );
				const RedVector4 RedVector4::EZ( 0.0f, 0.0f, 1.0f, 0.0f );
				const RedVector4 RedVector4::EW( 0.0f, 0.0f, 0.0f, 1.0f );
			
				// RedQuaternion Constant
				const RedQuaternion RedQuaternion::IDENTITY( 0.0f, 0.0f, 0.0f, 1.0f );
			
				// RedQsTransformConstant
				const RedQsTransform RedQsTransform::IDENTITY( RedVector4( 0.0f, 0.0f, 0.0f, 0.0f ), RedQuaternion( 0.0f, 0.0f, 0.0f, 1.0f ), RedVector4( 1.0f, 1.0f, 1.0f, 0.0f ) );

				// RedMatrix3x3 Constants
				const RedMatrix3x3 RedMatrix3x3::ZEROS( RedVector3::ZEROS, RedVector3::ZEROS, RedVector3::ZEROS );
				const RedMatrix3x3 RedMatrix3x3::IDENTITY( RedVector3::EX, RedVector3::EY, RedVector3::EZ );

				// RedMatrix4x4 Constants
				const RedMatrix4x4 RedMatrix4x4::ZEROS( RedVector4::ZEROS, RedVector4::ZEROS, RedVector4::ZEROS, RedVector4::ZEROS );
				const RedMatrix4x4 RedMatrix4x4::IDENTITY( RedVector4::EX, RedVector4::EY, RedVector4::EZ, RedVector4::EW );

			}
		}
	}

#endif