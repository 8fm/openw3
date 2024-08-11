// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.

#ifndef APEX_MATH_H
#define APEX_MATH_H

#include "foundation/PxMat44.h"
#include "PsVecMath.h"
#include "PsMathUtils.h"

namespace physx
{

#define APEX_ALIGN_UP(offset, alignment) (((offset) + (alignment)-1) & ~((alignment)-1))

PX_INLINE bool operator != (const physx::PxMat44& a, const physx::PxMat44& b)
{
	PX_ASSERT((((size_t)&a) & 0xf) == 0); // verify 16 byte alignment
	PX_ASSERT((((size_t)&b) & 0xf) == 0); // verify 16 byte alignment

	using namespace physx::shdfnd::aos;

	PxU32 allEq = 0xffffffff;
	const Vec4V ca1 = V4LoadU(&a.column0.x);
	const Vec4V cb1 = V4LoadU(&b.column0.x);
	allEq &= V4AllEq(ca1, cb1);
	const Vec4V ca2 = V4LoadU(&a.column1.x);
	const Vec4V cb2 = V4LoadU(&b.column1.x);
	allEq &= V4AllEq(ca2, cb2);
	const Vec4V ca3 = V4LoadU(&a.column2.x);
	const Vec4V cb3 = V4LoadU(&b.column2.x);
	allEq &= V4AllEq(ca3, cb3);
	const Vec4V ca4 = V4LoadU(&a.column3.x);
	const Vec4V cb4 = V4LoadU(&b.column3.x);
	allEq &= V4AllEq(ca4, cb4);

	return allEq == 0;
}


/**
 * computes weight * _origin + (1.0f - weight) * _target
 */
PX_INLINE PxMat44 interpolateMatrix(float weight, const PxMat44& _origin, const PxMat44& _target)
{
	// target: normalize, save scale, transform to quat
	PxMat34Legacy target = _target;
	PxVec3 axis0 = target.M.getColumn(0);
	PxVec3 axis1 = target.M.getColumn(1);
	PxVec3 axis2 = target.M.getColumn(2);
	const PxVec4 targetScale(axis0.normalize(), axis1.normalize(), axis2.normalize(), 1.0f);
	target.M.setColumn(0, axis0);
	target.M.setColumn(1, axis1);
	target.M.setColumn(2, axis2);
	const PxQuat targetQ = target.M.toQuat();

	// origin: normalize, save scale, transform to quat
	PxMat34Legacy origin = _origin;
	axis0 = origin.M.getColumn(0);
	axis1 = origin.M.getColumn(1);
	axis2 = origin.M.getColumn(2);
	const PxVec4 originScale(axis0.normalize(), axis1.normalize(), axis2.normalize(), 1.0f);
	origin.M.setColumn(0, axis0);
	origin.M.setColumn(1, axis1);
	origin.M.setColumn(2, axis2);
	const PxQuat originQ = origin.M.toQuat();

	// interpolate
	PxQuat relativeQ = slerp(1.0f - weight, originQ, targetQ);
	PxMat34Legacy relative;
	relative.t = weight * origin.t + (1.0f - weight) * target.t;
	relative.M.fromQuat(relativeQ);

	PxMat44 _relative = relative;
	const PxVec4 scale = weight * originScale + (1.0f - weight) * targetScale;
	_relative.scale(scale);

	return _relative;
}

}

#endif // APEX_MATH_H
