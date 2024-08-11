/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef __GEOMMATH_H__
#define __GEOMMATH_H__

#ifndef M_PI
	#define M_PI float(3.1415926535897932384626433832795)
#endif

#include <vectormath.h>

typedef sce::Vectormath::Scalar::Aos::Point3 Point3;

typedef sce::Vectormath::Scalar::Aos::Vector2 Vector2;
typedef sce::Vectormath::Scalar::Aos::Vector3 Vector3;
typedef sce::Vectormath::Scalar::Aos::Vector4 Vector4;

typedef sce::Vectormath::Scalar::Aos::Quat Quat;

typedef sce::Vectormath::Scalar::Aos::Matrix3 Matrix3;
typedef sce::Vectormath::Scalar::Aos::Matrix4 Matrix4;

#include "vector2unaligned.h"
#include "vector3unaligned.h"
#include "vector4unaligned.h"
#include "matrix4unaligned.h"

#endif
