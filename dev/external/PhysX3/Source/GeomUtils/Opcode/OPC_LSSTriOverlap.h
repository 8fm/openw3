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


// Following code from Magic-Software (http://www.magic-software.com/)
// A bit modified for Opcode

#include "Opcode.h"
#include "PsMathUtils.h"
#include "GuDistanceSegmentTriangle.h"
#include "GuDistanceSegmentTriangleSIMD.h"

namespace physx
{
namespace Gu
{

PX_FORCE_INLINE Ps::IntBool LSSCollider::LSSTriOverlap(const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2)
{
	#define V3 V3LoadU
	#ifdef V3
		Vec3V dummy1, dummy2;
		FloatV result = Gu::distanceSegmentTriangleSquared(V3(mSeg.p0), V3(mSeg.p1), V3(vert0), V3(vert1), V3(vert2), dummy1, dummy2);
		PxReal dist2 = FStore(result);
		#undef V3
	#else
		PxReal t, u, v;
		PxReal dist2 = Gu::distanceSegmentTriangleSquared(mSeg.p0, mSeg.p1-mSeg.p0, vert0, vert1-vert0, vert2-vert0, &t, &u, &v);
	#endif

	if(dist2<=mRadius2) // PT: objects are defined as closed, so we return 'true' in case of equality
		return Ps::IntTrue;
	return Ps::IntFalse;
}

#ifndef OPC_SUPPORT_VMX128

PX_FORCE_INLINE Ps::IntBool LSSCollider::LooseLSSTriOverlap(const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2)
{
	const PxVec3 v0 = mOBB.rot.transformTranspose(vert0 - mOBB.center);//should be inverse rotate.
	const PxVec3 v1 = mOBB.rot.transformTranspose(vert1 - mOBB.center);
	const PxVec3 v2 = mOBB.rot.transformTranspose(vert2 - mOBB.center);
// now do a tri-AABB test, with AABB at origin.

	//test tri AABB

	const PxVec3 triMin = PxVec3(PxMin(v0.x, PxMin(v1.x, v2.x)), PxMin(v0.y, PxMin(v1.y, v2.y)), PxMin(v0.z, PxMin(v1.z, v2.z)));
	const PxVec3 triMax = PxVec3(PxMax(v0.x, PxMax(v1.x, v2.x)), PxMax(v0.y, PxMax(v1.y, v2.y)), PxMax(v0.z, PxMax(v1.z, v2.z)));

	if((triMin.x > mOBB.extents.x) || (triMin.y > mOBB.extents.y) || (triMin.z > mOBB.extents.z))
		return Ps::IntFalse;

	if((triMax.x < -mOBB.extents.x) || (triMax.y < -mOBB.extents.y) || (triMax.z < -mOBB.extents.z))
		return Ps::IntFalse;

	// test tri plane.

	PxVec3 normal = (v1 - v0).cross(v2 - v0);
	float dist = normal.dot(v0);

	// find the minimum maximum on normal.
	PxVec3 vMin, vMax;

	if(normal.x > 0) 
	{
		vMin.x = -mOBB.extents.x;
		vMax.x = mOBB.extents.x;
	}
	else
	{
		vMin.x = mOBB.extents.x;
		vMax.x = -mOBB.extents.x;
	}

	if(normal.y > 0) 
	{
		vMin.y = -mOBB.extents.y;
		vMax.y = mOBB.extents.y;
	}
	else
	{
		vMin.y = mOBB.extents.y;
		vMax.y = -mOBB.extents.y;
	}


	if(normal.z > 0) 
	{
		vMin.z = -mOBB.extents.z;
		vMax.z = mOBB.extents.z;
	}
	else
	{
		vMin.z = mOBB.extents.z;
		vMax.z = -mOBB.extents.z;
	}

	// are they disjoint?

	float minDist = vMin.dot(normal);
	float maxDist = vMax.dot(normal);

	if((minDist > dist) || (maxDist < dist))
		return Ps::IntFalse;


	// Test edge axes.

	PxVec3 axis;
	float p0, p1, p2, pMin, pMax, axisRadius;

	PxVec3 edge0 = v1 - v0;
	PxVec3 edge1 = v2 - v1;
	PxVec3 edge2 = v0 - v2;

	PxVec3 radius = mOBB.extents;

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
	if((pMin > axisRadius) || (pMax < -axisRadius))
		return Ps::IntFalse;

	// axis == [1,0,0] x e1 == [0, -e1.z, e1.y]
	axis = Ps::cross100(edge1);
	p0 = axis.dot(v0);
	p1 = axis.dot(v1);
	pMin = PxMin(p0, p1);
	pMax = PxMax(p0, p1);
	axisRadius = radius.dot(PxVec3(PxAbs(axis.x), PxAbs(axis.y), PxAbs(axis.z)));
	if((pMin > axisRadius) || (pMax < -axisRadius))
		return Ps::IntFalse;

	// axis == [1,0,0] x e2 == [0, -e2.z, e2.y]
	axis = Ps::cross100(edge2);
	p0 = axis.dot(v0);
	p1 = axis.dot(v1);
	pMin = PxMin(p0, p1);
	pMax = PxMax(p0, p1);
	axisRadius = radius.dot(PxVec3(PxAbs(axis.x), PxAbs(axis.y), PxAbs(axis.z)));
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
	if((pMin > axisRadius) || (pMax < -axisRadius))
		return Ps::IntFalse;


	// axis == [0,1,0] x e1 == [e1.z, 0, -e1.x]
	axis = Ps::cross010(edge1);
	p0 = axis.dot(v0);
	p1 = axis.dot(v1);
	pMin = PxMin(p0, p1);
	pMax = PxMax(p0, p1);
	axisRadius = radius.dot(PxVec3(PxAbs(axis.x), PxAbs(axis.y), PxAbs(axis.z)));
	if((pMin > axisRadius) || (pMax < -axisRadius))
		return Ps::IntFalse;

	// axis == [0, 1, 0] x e2 == [e2.z, 0, -e2.x]
	axis = Ps::cross010(edge2);
	p0 = axis.dot(v0);
	p1 = axis.dot(v1);
	pMin = PxMin(p0, p1);
	pMax = PxMax(p0, p1);
	axisRadius = radius.dot(PxVec3(PxAbs(axis.x), PxAbs(axis.y), PxAbs(axis.z)));
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
	if((pMin > axisRadius) || (pMax < -axisRadius))
		return Ps::IntFalse;


	// axis == [0, 0, 1] x e1 == [-e1.y, e1.x, 0]

	axis = Ps::cross001(edge1);
	p0 = axis.dot(v0);
	p1 = axis.dot(v1);
	pMin = PxMin(p0, p1);
	pMax = PxMax(p0, p1);
	axisRadius = radius.dot(PxVec3(PxAbs(axis.x), PxAbs(axis.y), PxAbs(axis.z)));
	if((pMin > axisRadius) || (pMax < -axisRadius))
		return Ps::IntFalse;

	// axis == [0, 0, 1] x e2 == [-e2.y, e2.x, 0]

	axis = Ps::cross001(edge2);
	p0 = axis.dot(v0);
	p1 = axis.dot(v1);
	pMin = PxMin(p0, p1);
	pMax = PxMax(p0, p1);
	axisRadius = radius.dot(PxVec3(PxAbs(axis.x), PxAbs(axis.y), PxAbs(axis.z)));
	if((pMin > axisRadius) || (pMax < -axisRadius))
		return Ps::IntFalse;


	return Ps::IntTrue;
}

#else
PX_FORCE_INLINE Ps::IntBool LSSCollider::LooseLSSTriOverlap(const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2)
{
	//transform obb to origin
	Vec3V v0 = V3LoadU(vert0);
	Vec3V v1 = V3LoadU(vert1);
	Vec3V v2 = V3LoadU(vert2);

	Mat33V rot = Mat33V(V3LoadU(mOBB.rot[0]),V3LoadU(mOBB.rot[1]),V3LoadU(mOBB.rot[2]));

	Vec3V center = V3LoadU(mOBB.center);

	v0 = M33TrnspsMulV3(rot, V3Sub(v0, center));
	v1 = M33TrnspsMulV3(rot, V3Sub(v1, center));
	v2 = M33TrnspsMulV3(rot, V3Sub(v2, center));

	BoolV ffff = BFFFF();
	Vec3V zero = V3Zero();


	Vec3V extents = V3LoadU(mOBB.extents);
	Vec3V minusExtents = V3Neg(extents);

// now do a tri-AABB test, with AABB at origin.

// Test triangle AABB
	Vec3V triMin = V3Min(v0, V3Min(v1, v2));
	Vec3V triMax = V3Max(v0, V3Max(v1, v2));


	BoolV mask = BOr(V3IsGrtr(triMin, extents), V3IsGrtr(minusExtents, triMax));
	if(!BAllEq(mask, ffff))
		return Ps::IntFalse;

	// test tri plane.
	Vec3V normal = V3Cross(V3Sub(v1, v0), V3Sub(v2, v0));
	FloatV dist = V3Dot(normal, v0);

	
	BoolV vMask = V3IsGrtr(normal, zero);
	Vec3V vMin = V3Sel(vMask, minusExtents, extents);
	Vec3V vMax = V3Sel(vMask, extents, minusExtents);

	FloatV minDist = V3Dot(vMin, normal);
	FloatV maxDist = V3Dot(vMax, normal);

	mask = BOr(FIsGrtr(minDist, dist), FIsGrtr(dist, maxDist));
	if(!BAllEq(mask, ffff))
		return Ps::IntFalse;

	// Test edge axes.
//////// test edge axis
	//transform the triangle.

	Vec3V edge0 = V3Sub(v1, v0);
	Vec3V edge1 = V3Sub(v2, v1);
	Vec3V edge2 = V3Sub(v0, v2);

	
	Vec3V minusEdge0 = V3Neg(edge0);
	Vec3V minusEdge1 = V3Neg(edge1);
	Vec3V minusEdge2 = V3Neg(edge2);

	Vec3V radius = extents;

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
	if(FAllGrtr(pMin, axisRadius) || FAllGrtr(minusAxisRadius, pMax))
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
