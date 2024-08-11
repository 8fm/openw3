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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#ifndef OPC_SPHERE_TRI_OVERLAP_H
#define OPC_SPHERE_TRI_OVERLAP_H

#include "Opcode.h"
#include "PsMathUtils.h"
#include "GuDistancePointTriangle.h"

namespace physx
{
namespace Gu
{

PX_FORCE_INLINE Ps::IntBool SphereCollider::SphereTriOverlap(const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2)
{
	float s,t;
	const PxVec3 cp = Gu::closestPtPointTriangle(mCenter, vert0, vert1, vert2, s, t);
	PX_UNUSED(s);
	PX_UNUSED(t);
	const float d2 = (cp - mCenter).magnitudeSquared();
	return d2 <= mRadius2;
}

#ifndef OPC_SUPPORT_VMX128

// Do a partial AABB against triangle overlap test.
PX_FORCE_INLINE	 Ps::IntBool SphereCollider::LooseSphereTriOverlap(const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2)
{
	const PxVec3 sphereMin(mCenter.x - mRadius, mCenter.y - mRadius, mCenter.z - mRadius);
	const PxVec3 sphereMax(mCenter.x + mRadius, mCenter.y + mRadius, mCenter.z + mRadius);

	// Test triangle AABB
	const PxVec3 triMin = PxVec3(PxMin(vert0.x,PxMin(vert1.x, vert2.x)), PxMin(vert0.y,PxMin(vert1.y, vert2.y)), PxMin(vert0.z,PxMin(vert1.z, vert2.z)));
	const PxVec3 triMax = PxVec3(PxMax(vert0.x,PxMax(vert1.x, vert2.x)), PxMax(vert0.y,PxMax(vert1.y, vert2.y)), PxMax(vert0.z,PxMax(vert1.z, vert2.z)));

	if((triMin.x > sphereMax.x) || (triMax.x < sphereMin.x)) return Ps::IntFalse;
	if((triMin.y > sphereMax.y) || (triMax.y < sphereMin.y)) return Ps::IntFalse;
	if((triMin.z > sphereMax.z) || (triMax.z < sphereMin.z)) return Ps::IntFalse;

	// Test the triangle plane.
	PxVec3 normal = (vert1 - vert0).cross(vert2 - vert0);
	float dist = normal.dot(vert0);

	// find the minimum maximum on normal.
	PxVec3 vMin, vMax;

	if(normal.x > 0) 
	{
		vMin.x = sphereMin.x;
		vMax.x = sphereMax.x;
	}
	else
	{
		vMin.x = sphereMax.x;
		vMax.x = sphereMin.x;
	}

	if(normal.y > 0) 
	{
		vMin.y = sphereMin.y;
		vMax.y = sphereMax.y;
	}
	else
	{
		vMin.y = sphereMax.y;
		vMax.y = sphereMin.y;
	}


	if(normal.z > 0) 
	{
		vMin.z = sphereMin.z;
		vMax.z = sphereMax.z;
	}
	else
	{
		vMin.z = sphereMax.z;
		vMax.z = sphereMin.z;
	}

	// are they disjoint?

	float minDist = vMin.dot(normal);
	float maxDist = vMax.dot(normal);

	if((minDist > dist) || (maxDist < dist))
		return Ps::IntFalse;

	// Test edge axes.

	PxVec3 axis;
	float p0, p1, p2, pMin, pMax, axisRadius;

	const PxVec3 v0 = vert0 - mCenter;
	const PxVec3 v1 = vert1 - mCenter;
	const PxVec3 v2 = vert2 - mCenter;

	const PxVec3 edge0 = v1 - v0;
	const PxVec3 edge1 = v2 - v1;
	const PxVec3 edge2 = v0 - v2;

	PxVec3 radius(mRadius, mRadius, mRadius);

	/*
		y*other.z - z*other.y,
		z*other.x - x*other.z,
		x*other.y - y*other.x

		0 - 0
		0 - e0.z
		e0.y - 0
	*/


	// axis == [1,0,0] x e0 == [0, -e0.z, e0.y]
	// x, y, z, w,    x, y, z, w
	// 0, 1, 2, 3,    4, 5, 6, 7

	

	axis = Ps::cross100(edge0);
	p0 = axis.dot(v0);
	p2 = axis.dot(v2);
	pMin = PxMin(p0, p2);
	pMax = PxMax(p0, p2);
	axisRadius = radius.dot(PxVec3(PxAbs(axis.x), PxAbs(axis.y), PxAbs(axis.z)));
	// we test against the sphere radius, not the box radius on the axis...(so this isnt a standard AABB test).
	if((pMin > axisRadius) || (pMax < -axisRadius))
		return Ps::IntFalse;

	// axis == [1,0,0] x e1 == [0, -e1.z, e1.y]
	axis = Ps::cross100(edge1);
	p0 = axis.dot(v0);
	p1 = axis.dot(v1);
	pMin = PxMin(p0, p1);
	pMax = PxMax(p0, p1);
	axisRadius = radius.dot(PxVec3(PxAbs(axis.x), PxAbs(axis.y), PxAbs(axis.z)));
	// we test against the sphere radius, not the box radius on the axis...(so this isnt a standard AABB test).
	if((pMin > axisRadius) || (pMax < -axisRadius))
		return Ps::IntFalse;

	// axis == [1,0,0] x e2 == [0, -e2.z, e2.y]
	axis = Ps::cross100(edge2);
	p0 = axis.dot(v0);
	p1 = axis.dot(v1);
	pMin = PxMin(p0, p1);
	pMax = PxMax(p0, p1);
	axisRadius = radius.dot(PxVec3(PxAbs(axis.x), PxAbs(axis.y), PxAbs(axis.z)));
	// we test against the sphere radius, not the box radius on the axis...(so this isnt a standard AABB test).
	if((pMin > axisRadius) || (pMax < -axisRadius))
		return Ps::IntFalse;

	/*
		y*other.z - z*other.y,
		z*other.x - x*other.z,
		x*other.y - y*other.x

		e0.z - 0
		0 - 0
		0 - e0.x
	*/
	// axis == [0,1,0] x e0 == [e0.z, 0, -e0.x]
	// x, y, z, w,    x, y, z, w
	// 0, 1, 2, 3,    4, 5, 6, 7

	axis = Ps::cross010(edge0);
	p0 = axis.dot(v0);
	p2 = axis.dot(v2);
	pMin = PxMin(p0, p2);
	pMax = PxMax(p0, p2);
	axisRadius = radius.dot(PxVec3(PxAbs(axis.x), PxAbs(axis.y), PxAbs(axis.z)));
	// we test against the sphere radius, not the box radius on the axis...(so this isnt a standard AABB test).
	if((pMin > axisRadius) || (pMax < -axisRadius))
		return Ps::IntFalse;


	// axis == [0,1,0] x e1 == [e1.z, 0, -e1.x]
	axis = Ps::cross010(edge1);
	p0 = axis.dot(v0);
	p1 = axis.dot(v1);
	pMin = PxMin(p0, p1);
	pMax = PxMax(p0, p1);
	axisRadius = radius.dot(PxVec3(PxAbs(axis.x), PxAbs(axis.y), PxAbs(axis.z)));
	// we test against the sphere radius, not the box radius on the axis...(so this isnt a standard AABB test).
	if((pMin > axisRadius) || (pMax < -axisRadius))
		return Ps::IntFalse;

	// axis == [0, 1, 0] x e2 == [e2.z, 0, -e2.x]
	axis = Ps::cross010(edge2);
	p0 = axis.dot(v0);
	p1 = axis.dot(v1);
	pMin = PxMin(p0, p1);
	pMax = PxMax(p0, p1);
	axisRadius = radius.dot(PxVec3(PxAbs(axis.x), PxAbs(axis.y), PxAbs(axis.z)));
	// we test against the sphere radius, not the box radius on the axis...(so this isnt a standard AABB test).
	if((pMin > axisRadius) || (pMax < -axisRadius))
		return Ps::IntFalse;


	/*
		y*other.z - z*other.y,
		z*other.x - x*other.z,
		x*other.y - y*other.x

		0 - e0.y
		e0.x - 0
		0 - 0
	*/

	// axis == [0, 0, 1] x e0 == [-e0.y, e0.x, 0]
	// x, y, z, w,    x, y, z, w
	// 0, 1, 2, 3,    4, 5, 6, 7

	// axis == [0, 1, 0] x e2 == [e2.z, 0, -e2.x]
	axis = Ps::cross001(edge0);
	p0 = axis.dot(v0);
	p2 = axis.dot(v2);
	pMin = PxMin(p0, p2);
	pMax = PxMax(p0, p2);
	axisRadius = radius.dot(PxVec3(PxAbs(axis.x), PxAbs(axis.y), PxAbs(axis.z)));
	// we test against the sphere radius, not the box radius on the axis...(so this isnt a standard AABB test).
	if((pMin > axisRadius) || (pMax < -axisRadius))
		return Ps::IntFalse;


	// axis == [0, 0, 1] x e1 == [-e1.y, e1.x, 0]

	axis = Ps::cross001(edge1);
	p0 = axis.dot(v0);
	p1 = axis.dot(v1);
	pMin = PxMin(p0, p1);
	pMax = PxMax(p0, p1);
	axisRadius = radius.dot(PxVec3(PxAbs(axis.x), PxAbs(axis.y), PxAbs(axis.z)));
	// we test against the sphere radius, not the box radius on the axis...(so this isnt a standard AABB test).
	if((pMin > axisRadius) || (pMax < -axisRadius))
		return Ps::IntFalse;

	// axis == [0, 0, 1] x e2 == [-e2.y, e2.x, 0]

	axis = Ps::cross001(edge2);
	p0 = axis.dot(v0);
	p1 = axis.dot(v1);
	pMin = PxMin(p0, p1);
	pMax = PxMax(p0, p1);
	axisRadius = radius.dot(PxVec3(PxAbs(axis.x), PxAbs(axis.y), PxAbs(axis.z)));
	// we test against the sphere radius, not the box radius on the axis...(so this isnt a standard AABB test).
	if((pMin > axisRadius) || (pMax < -axisRadius))
		return Ps::IntFalse;

	return Ps::IntTrue;
}
#else

PX_FORCE_INLINE Ps::IntBool SphereCollider::LooseSphereTriOverlap(const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2)
{
	Vec3V center = V3LoadU(mCenter);
	Vec3V radius =  V3Load(mRadius);

	Vec3V sphereMin = V3Sub(center, radius);
	Vec3V sphereMax = V3Add(center, radius);

	return LooseSphereTriOverlap(vert0, vert1, vert2,
								center, radius,
								sphereMin, sphereMax);
}

PX_FORCE_INLINE	Ps::IntBool SphereCollider::LooseSphereTriOverlap(
	const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2,
	const Vec3V& center, const Vec3V& radius,
	const Vec3V& sphereMin, const Vec3V& sphereMax)
{
	BoolV ffff = BFFFF();
	Vec3V zero = V3Zero();

	//We could optimize further, but most of the time is spent in these first few instructions.

	//3 unaligned loads are painful:-(
	Vec3V v0 = V3LoadU(vert0);
	Vec3V v1 = V3LoadU(vert1);
	Vec3V v2 = V3LoadU(vert2);

	// Test triangle AABB
	Vec3V triMin = V3Min(v0, V3Min(v1, v2));
	Vec3V triMax = V3Max(v0, V3Max(v1, v2));

	BoolV mask = BOr(V3IsGrtr(triMin, sphereMax), V3IsGrtr(sphereMin, triMax));

	if(!BAllEq(mask, ffff))
		return Ps::IntFalse;


	// Test the triangle plane.

	Vec3V normal = V3Cross(V3Sub(v1, v0), V3Sub(v2, v0));
	FloatV dist = V3Dot(normal, v0);

	BoolV vMask = V3IsGrtr(normal, zero);
	Vec3V vMin = V3Sel(vMask, sphereMin, sphereMax);
	Vec3V vMax = V3Sel(vMask, sphereMax, sphereMin);

	FloatV minDist = V3Dot(vMin, normal);
	FloatV maxDist = V3Dot(vMax, normal);

	mask = BOr(V3IsGrtr(Vec3V_From_FloatV(minDist), Vec3V_From_FloatV(dist)), V3IsGrtr(Vec3V_From_FloatV(dist), Vec3V_From_FloatV(maxDist)));
	
	if(!BAllEq(mask, ffff))
		return Ps::IntFalse;

	//////// test edge axis
	//transform the triangle.

	v0 = V3Sub(v0, center);
	v1 = V3Sub(v1, center);
	v2 = V3Sub(v2, center);

	Vec3V edge0 = V3Sub(v1, v0);
	Vec3V edge1 = V3Sub(v2, v1);
	Vec3V edge2 = V3Sub(v0, v2);

	Vec3V minusEdge0 = V3Neg(edge0);
	Vec3V minusEdge1 = V3Neg(edge1);
	Vec3V minusEdge2 = V3Neg(edge2);


	Vec3V axis;
	FloatV p0, p1, p2, pMin, pMax, axisRadius, minusAxisRadius;

	/*
		y*other.z - z*other.y,
		z*other.x - x*other.z,
		x*other.y - y*other.x

		0 - 0
		0 - e0.z
		e0.y - 0
	*/


	// axis == [1,0,0] x e0 == [0, -e0.z, e0.y]
	// x, y, z, w,    x, y, z, w
	// 0, 1, 2, 3,    4, 5, 6, 7

	axis = V3Perm_Zero_1Z_0Y(edge0, minusEdge0);
	p0 = V3Dot(v0, axis);
	p2 = V3Dot(v2, axis);
	pMin = FMin(p0, p2);
	pMax = FMax(p0, p2);
	axisRadius = V3Dot(radius, V3Abs(axis));
	minusAxisRadius = FNeg(axisRadius);
	// we test against the sphere radius, not the box radius on the axis...(so this isnt a standard AABB test).
	if(FAllGrtr(pMin, axisRadius) || FAllGrtr(minusAxisRadius, pMax))
		return Ps::IntFalse;


	// axis == [1,0,0] x e1 == [0, -e1.z, e1.y]
	axis = V3Perm_Zero_1Z_0Y(edge1, minusEdge1);
	p0 = V3Dot(v0, axis);
	p1 = V3Dot(v1, axis);
	pMin = FMin(p0, p1);
	pMax = FMax(p0, p1);
	axisRadius = V3Dot(radius, V3Abs(axis));
	minusAxisRadius = FNeg(axisRadius);
	if(FAllGrtr(pMin,axisRadius) || FAllGrtr(minusAxisRadius, pMax))
		return Ps::IntFalse;



	// axis == [1,0,0] x e2 == [0, -e2.z, e2.y]
	axis = V3Perm_Zero_1Z_0Y(edge2, minusEdge2);
	p0 = V3Dot(v0, axis);
	p1 = V3Dot(v1, axis);
	pMin = FMin(p0, p1);
	pMax = FMax(p0, p1);
	axisRadius = V3Dot(radius, V3Abs(axis));
	minusAxisRadius = FNeg(axisRadius);
	if(FAllGrtr(pMin, axisRadius) || FAllGrtr(minusAxisRadius, pMax))
		return Ps::IntFalse;


	/*
		y*other.z - z*other.y,
		z*other.x - x*other.z,
		x*other.y - y*other.x

		e0.z - 0
		0 - 0
		0 - e0.x
	*/
	// axis == [0,1,0] x e0 == [e0.z, 0, -e0.x]
	// x, y, z, w,    x, y, z, w
	// 0, 1, 2, 3,    4, 5, 6, 7

	axis = V3Perm_0Z_Zero_1X(edge0, minusEdge0);
	p0 = V3Dot(v0, axis);
	p2 = V3Dot(v2, axis);
	pMin = FMin(p0, p2);
	pMax = FMax(p0, p2);
	axisRadius = V3Dot(radius, V3Abs(axis));
	minusAxisRadius = FNeg(axisRadius);
	if(FAllGrtr(pMin, axisRadius) || FAllGrtr(minusAxisRadius, pMax))
		return Ps::IntFalse;


	// axis == [0,1,0] x e1 == [e1.z, 0, -e1.x]
	axis = V3Perm_0Z_Zero_1X(edge1, minusEdge1);
	p0 = V3Dot(v0, axis);
	p1 = V3Dot(v1, axis);
	pMin = FMin(p0, p1);
	pMax = FMax(p0, p1);
	axisRadius = V3Dot(radius, V3Abs(axis));
	minusAxisRadius = FNeg(axisRadius);
	if(FAllGrtr(pMin, axisRadius) || FAllGrtr(minusAxisRadius, pMax))
		return Ps::IntFalse;

	// axis == [0, 1, 0] x e2 == [e2.z, 0, -e2.x]
	axis = V3Perm_0Z_Zero_1X(edge2, minusEdge2);
	p0 = V3Dot(v0, axis);
	p1 = V3Dot(v1, axis);
	pMin = FMin(p0, p1);
	pMax = FMax(p0, p1);
	axisRadius = V3Dot(radius, V3Abs(axis));
	minusAxisRadius = FNeg(axisRadius);
	if(FAllGrtr(pMin, axisRadius) || FAllGrtr(minusAxisRadius, pMax))
		return Ps::IntFalse;

	/*
		y*other.z - z*other.y,
		z*other.x - x*other.z,
		x*other.y - y*other.x

		0 - e0.y
		e0.x - 0
		0 - 0
	*/

	// axis == [0, 0, 1] x e0 == [-e0.y, e0.x, 0]
	// x, y, z, w,    x, y, z, w
	// 0, 1, 2, 3,    4, 5, 6, 7

	axis = V3Perm_1Y_0X_Zero(edge0, minusEdge0);
	p0 = V3Dot(v0, axis);
	p2 = V3Dot(v2, axis);
	pMin = FMin(p0, p2);
	pMax = FMax(p0, p2);
	axisRadius = V3Dot(radius, V3Abs(axis));
	minusAxisRadius = FNeg(axisRadius);
	if(FAllGrtr(pMin, axisRadius) || FAllGrtr(minusAxisRadius, pMax))
		return Ps::IntFalse;


	// axis == [0, 0, 1] x e1 == [-e1.y, e1.x, 0]
	axis = V3Perm_1Y_0X_Zero(edge1, minusEdge1);
	p0 = V3Dot(v0, axis);
	p1 = V3Dot(v1, axis);
	pMin = FMin(p0, p1);
	pMax = FMax(p0, p1);
	axisRadius = V3Dot(radius, V3Abs(axis));
	minusAxisRadius = FNeg(axisRadius);
	if(FAllGrtr(pMin, axisRadius) || FAllGrtr(minusAxisRadius, pMax))
		return Ps::IntFalse;

	// axis == [0, 0, 1] x e2 == [-e2.y, e2.x, 0]
	axis = V3Perm_1Y_0X_Zero(edge2, minusEdge2);
	p0 = V3Dot(v0, axis);
	p1 = V3Dot(v1, axis);
	pMin = FMin(p0, p1);
	pMax = FMax(p0, p1);
	axisRadius = V3Dot(radius, V3Abs(axis));
	minusAxisRadius = FNeg(axisRadius);
	if(FAllGrtr(pMin, axisRadius) || FAllGrtr(minusAxisRadius, pMax))
		return Ps::IntFalse;


	return Ps::IntTrue;
}

#endif

} // namespace Gu

}

#endif