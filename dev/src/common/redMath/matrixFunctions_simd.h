/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _REDMATH_LIB_MATRIX_FUNCTIONS_SIMD_H_
#define _REDMATH_LIB_MATRIX_FUNCTIONS_SIMD_H_

#include "matrixArithmetic_simd.h"

#include "redMatrix4x4_simd.h"
#include "redVector2_simd.h"
#include "redVector4_simd.h"

namespace RedMath
{
	namespace SIMD
	{
		//////////////////////////////////////////////////////////////////////////
		// Build
		//////////////////////////////////////////////////////////////////////////
		extern RedMatrix4x4 BuildPerspectiveLH( Red::System::Float _fovy, Red::System::Float _aspect, Red::System::Float _zn, Red::System::Float _zf );
		extern RedMatrix4x4 BuildOrthoLH( Red::System::Float _w, Red::System::Float _h, Red::System::Float _zn, Red::System::Float _zf );
		//////////////////////////////////////////////////////////////////////////
		// BuildTo
		//////////////////////////////////////////////////////////////////////////
		//RedEulerAngles ToEulerAngles();
		extern void ToAngleVectors( const RedMatrix4x4& _mat, RedVector4& _forward, RedVector4& _right, RedVector4& _up );
		//RedQuaternion ToQuaternion();
		//////////////////////////////////////////////////////////////////////////
		// BuildFrom
		//////////////////////////////////////////////////////////////////////////
		
		// Build matrix with EY from given direction vector
		extern RedMatrix4x4 BuildFromDirectionVector( const RedVector4& _dirVec );

		// Build a matrix from a RedVector4 representing a quaternion.
		extern RedMatrix4x4 BuildFromQuaternion( const RedVector4& _quaternion );
		
		//extern RedMatrix4x4 BuildFromQuaternion( const RedQuaternion& _quaternion );
		//extern RedMatrix4x4 BuildFromEulerAngles( const RedEulerAngles& _angles );
		//////////////////////////////////////////////////////////////////////////
		// Transform
		//////////////////////////////////////////////////////////////////////////
		
		// Assumed W = 0.0f
		extern RedVector4 TransformVector( const RedMatrix4x4& _mat, const RedVector4& _v );			
		// W used directly
		extern RedVector4 TransformVectorWithW( const RedMatrix4x4& _mat, const RedVector4& _v );
		// Assumed W = 1.0f
		extern RedVector4 TransformPoint( const RedMatrix4x4& _mat, const RedVector4& _v );			

		extern RedVector3 TransformVector(const RedMatrix3x3& _mat, const RedVector3& _v );

		//////////////////////////////////////////////////////////////////////////
		// Scale Extraction - Decompose matrix to the orthonormal part and the scale
		//////////////////////////////////////////////////////////////////////////
		extern void ExtractScale( const RedMatrix4x4& _m, RedMatrix4x4& _trMatrix, RedVector4& _scale );
	}
}


#endif // _REDMATH_LIB_MATRIX_FUNCTIONS_SIMD_H_