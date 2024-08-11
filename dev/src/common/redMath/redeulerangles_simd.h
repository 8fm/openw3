/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _REDMATH_LIB_REDEULERANGLES_SIMD_H_
#define _REDMATH_LIB_REDEULERANGLES_SIMD_H_

namespace RedMath
{
	namespace SIMD
	{
		class RedVector2; 

		RED_ALIGNED_CLASS( RedEulerAngles, 16 )
		{
		public:
			// Constructors
			RED_INLINE RedEulerAngles();
			RED_INLINE RedEulerAngles( const RedEulerAngles& _e );
			RED_INLINE RedEulerAngles( float _roll, float _pitch, float _yaw );
			RED_INLINE RedEulerAngles( const RedVector3& _v );
			RED_INLINE RedEulerAngles( const RedVector4& _v );

			RED_INLINE ~RedEulerAngles();

			RED_INLINE bool IsAlmostEqual( const RedEulerAngles& _e, float _epsilon = FLT_EPSILON ) const;

			RED_INLINE RedMatrix4x4 ToMatrix4() const;
			RED_INLINE void ToMatrix4( RedMatrix4x4& _out ) const;
			RED_INLINE void ToAngleVectors( RedVector4* _forward, RedVector4* _right, RedVector4* _up ) const;
			RED_INLINE RedQuaternion ToQuaternion() const; 
			RED_INLINE RedVector4 TransformPoint( const RedVector4& _v ) const;
			RED_INLINE RedVector4 TransformVector( const RedVector4& _v ) const;
			RED_INLINE static float NormalizeAngle( float _angle );
			RED_INLINE RedEulerAngles& Normalize();

			RED_INLINE static float  YawFromXY( float _x, float _y );
			RED_INLINE static RedVector4 YawToVector4( float _yaw );
			RED_INLINE static RedVector2 YawToVector2( float _yaw );

			RED_INLINE static float AngleDistance( float _a, float _b );
			RED_INLINE static RedEulerAngles AngleDistance( const RedEulerAngles& _a, const RedEulerAngles& _b );

			RED_INLINE static RedEulerAngles InterpolateEulerAngles(const RedEulerAngles& a, const RedEulerAngles& b, float weight);

			// Predefined values
			static const RedEulerAngles ZEROS;

			union
			{
				struct  
				{
					RedVector3 angles;
				};
		
				struct  
				{
					float Roll;
					float Pitch; 
					float Yaw;
				};
			};
		};
	};
};

#endif // _REDMATH_LIB_REDEULERANGLES_SIMD_H_