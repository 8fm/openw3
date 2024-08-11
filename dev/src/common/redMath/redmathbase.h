/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _REDMATH_LIB_BASE_H_
#define _REDMATH_LIB_BASE_H_
#include "../redSystem/os.h"
#include "../redSystem/error.h"
#include <float.h>
#include <math.h>
#include "numericalutils.h"
#include "mathfunctions_fpu.h"


// Disabling the SIMD math (using the legacy Fpu math) is not fully supported currently
#define RED_ENABLE_SIMD_MATH

/************************************************************************/
/* Header Declarations													*/
/************************************************************************/

#ifdef RED_ENABLE_SIMD_MATH

	namespace RedMath
	{
		namespace SIMD
		{
			class RedVector1;
			class RedVector2;
			class RedVector3;
			class RedVector4;
			class RedMatrix3x3;
			class RedMatrix4x4;
			class RedEulerAngles;
			class RedQuaternion;
			class RedTransform;
			class RedQsTransform;
			class RedAABB;
		};
	};

	#include "vectorFunctions_simd.h"
	#include "matrixFunctions_simd.h"
	#include "redQuaternion_simd.h"
	#include "redScalar_simd.h"
	#include "redvector2_simd.h"
	#include "redvector3_simd.h"
	#include "redvector4_simd.h"
	#include "redMatrix3x3_simd.h"
	#include "redMatrix4x4_simd.h"
	#include "redqstransform_simd.h"
	#include "redtransform_simd.h"
	#include "redaabb_simd.h"
	#include "redeulerangles_simd.h"

	/************************************************************************/
	/* Inline implementation                                                */
	/************************************************************************/
	
	#include "redScalar_simd.inl"
	#include "redvector2_simd.inl"
	#include "redvector3_simd.inl"
	#include "redvector4_simd.inl"
	#include "redmatrix3x3_simd.inl"
	#include "redmatrix4x4_simd.inl"
	#include "vectorFunctions_simd.inl"
	#include "vectorArithmetic_simd.inl"
	#include "redquaternion_simd.inl"
	#include "redtransform_simd.inl"
	#include "redqstransform_simd.inl"
	#include "redaabb_simd.inl"
	#include "redeulerangles_simd.inl"

	typedef RedMath::SIMD::RedScalar RedFloat1;
	typedef RedMath::SIMD::RedVector2 RedFloat2;
	typedef RedMath::SIMD::RedVector3 RedFloat3;
	typedef RedMath::SIMD::RedVector4 RedFloat4;
	typedef RedMath::SIMD::RedVector2 RedPoint2;
	typedef RedMath::SIMD::RedVector3 RedPoint3;
	typedef RedMath::SIMD::RedVector4 RedPoint4;
	typedef RedMath::SIMD::RedVector2 RedVector2;
	typedef RedMath::SIMD::RedVector3 RedVector3;
	typedef RedMath::SIMD::RedVector4 RedVector4;
	typedef RedMath::SIMD::RedMatrix3x3 RedMatrix3x3;
	typedef RedMath::SIMD::RedMatrix4x4 RedMatrix4x4;
	typedef RedMath::SIMD::RedQuaternion RedQuaternion;
	typedef RedMath::SIMD::RedTransform RedTransform;
	typedef RedMath::SIMD::RedQsTransform RedQsTransform;
	typedef RedMath::SIMD::RedEulerAngles RedEulerAngles;
	typedef RedMath::SIMD::RedAABB RedAABB;

#else  // Legacy math code

	namespace Red
	{
		namespace Math
		{
			namespace Fpu
			{
				class RedVector1;
				class RedVector2;
				class RedVector3;
				class RedVector4;
				class RedMatrix3x3;
				class RedMatrix4x4;
				class RedEulerAngles;
				class RedQuaternion;
				class RedTransform;
				class RedQsTransform;
				class RedAABB;
			};
		};
	};

	#include "redvector_fpu.h"
	#include "redmatrix_fpu.h"
	#include "redquaternion_fpu.h"
	#include "redeulerangles_fpu.h"
	#include "redqstransform_fpu.h"
	#include "redtransform_fpu.h"
	#include "redaabb_fpu.h"
	#include "redMatrix4x4_simd.h"


	/************************************************************************/
	/* Inline implementation                                                */
	/************************************************************************/

	#include "redvector_fpu.inl"
	#include "redmatrix_fpu.inl"
	#include "redquaternion_fpu.inl"
	#include "redeulerangles_fpu.inl"
	#include "redtransform_fpu.inl"
	#include "redqstransform_fpu.inl"
	#include "redaabb_fpu.inl"

	typedef Red::Math::Fpu::RedVector1 RedFloat1;
	typedef Red::Math::Fpu::RedVector2 RedFloat2;
	typedef Red::Math::Fpu::RedVector3 RedFloat3;
	typedef Red::Math::Fpu::RedVector4 RedFloat4;
	typedef Red::Math::Fpu::RedVector2 RedPoint2;
	typedef Red::Math::Fpu::RedVector3 RedPoint3;
	typedef Red::Math::Fpu::RedVector4 RedPoint4;
	typedef Red::Math::Fpu::RedVector2 RedVector2;
	typedef Red::Math::Fpu::RedVector3 RedVector3;
	typedef Red::Math::Fpu::RedVector4 RedVector4;
	typedef Red::Math::Fpu::RedMatrix3x3 RedMatrix3x3;
	typedef Red::Math::Fpu::RedMatrix4x4 RedMatrix4x4;
	typedef Red::Math::Fpu::RedQuaternion RedQuaternion;
	typedef Red::Math::Fpu::RedEulerAngles RedEulerAngles;
	typedef Red::Math::Fpu::RedTransform RedTransform;
	
	typedef Red::Math::Fpu::RedQsTransform RedQsTransform;
	
	typedef Red::Math::Fpu::RedAABB RedAABB;

#endif


#define REDMATH_USED
#endif // _REDMATH_LIB_BASE_H_