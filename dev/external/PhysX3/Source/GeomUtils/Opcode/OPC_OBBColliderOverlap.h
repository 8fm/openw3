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


#include "Opcode.h"
#include "OPC_CommonColliderOverlap.h"

namespace physx
{

using namespace Ps::aos;

namespace Gu
{

#ifndef OPC_SUPPORT_VMX128

//! A dedicated version where the box is constant
PX_FORCE_INLINE Ps::IntBool OBBCollider::TriBoxOverlap(const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2) const
{
	PxVec3 v0, v1, v2;

	/* Transform them in a common space */
	TransformPoint(v0, vert0, mRModelToBox, mTModelToBox);
	TransformPoint(v1, vert1, mRModelToBox, mTModelToBox);
	TransformPoint(v2, vert2, mRModelToBox, mTModelToBox);

	const PxVec3& extents = mBoxExtents;

	// use separating axis theorem to test overlap between triangle and box 
	// need to test for overlap in these directions: 
	// 1) the {x,y,z}-directions (actually, since we use the AABB of the triangle 
	//    we do not even need to test these) 
	// 2) normal of the triangle 
	// 3) crossproduct(edge from tri, {x,y,z}-directin) 
	//    this gives 3x3=9 more tests 

	// Box center is already in (0,0,0)

	// First, test overlap in the {x,y,z}-directions
	float minimum,maximum;
	// Find minimum, maximum of the triangle in x-direction, and test for overlap in X
	FINDMINMAX(v0.x, v1.x, v2.x, minimum, maximum);
	if(minimum>mBoxExtents.x || maximum<-mBoxExtents.x) return Ps::IntFalse;

	FINDMINMAX(v0.y, v1.y, v2.y, minimum, maximum);
	if(minimum>mBoxExtents.y || maximum<-mBoxExtents.y) return Ps::IntFalse;

	FINDMINMAX(v0.z, v1.z, v2.z, minimum, maximum);
	if(minimum>mBoxExtents.z || maximum<-mBoxExtents.z) return Ps::IntFalse;

	// 2) Test if the box intersects the plane of the triangle
	// compute plane equation of triangle: normal*x+d=0
	// ### could be precomputed since we use the same leaf triangle several times
	const PxVec3 e0 = v1 - v0;
	const PxVec3 e1 = v2 - v1;
	const PxVec3 normal = e0.cross(e1);
	const float d = -normal.dot(v0);
	if(!planeBoxOverlap(normal, d, mBoxExtents)) return Ps::IntFalse;

	// 3) "Class III" tests - here we always do full tests since the box is a primitive (not a BV)
	{
		IMPLEMENT_CLASS3_TESTS
	}
	return Ps::IntTrue;
}
#else
//! A dedicated version where the box is constant
// using vmx128
//TODO: we should try and unify this code with that in the loose primitive test for LSS

PX_FORCE_INLINE Ps::IntBool OBBCollider::TriBoxOverlap(const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2) const
{
	Vec3V rot_0 = V3LoadU(mRModelToBox[0]);
	Vec3V rot_1 = V3LoadU(mRModelToBox[1]);
	Vec3V rot_2 = V3LoadU(mRModelToBox[2]);

	Vec3V tran = V3LoadU(mTModelToBox);
	//AP: needed on SPU to fix a precision failure in --gtest_filter=SqTestSweep.verify_Big_QueryShape_Small_SceneShape
	Vec3V extents = PX_IS_SPU ? V3LoadU(mBoxExtents+PxVec3(1e-6f)) : V3LoadU(mBoxExtents);

	/*pxPrintf("rte = %.8f %.8f %.8f= %.8f %.8f %.8f= %.8f %.8f %.8f-- %.8f %.8f %.8f; %.8f %.8f %.8f\n",
		mRModelToBox[0].x, mRModelToBox[0].y, mRModelToBox[0].z,
		mRModelToBox[1].x, mRModelToBox[1].y, mRModelToBox[1].z,
		mRModelToBox[2].x, mRModelToBox[2].y, mRModelToBox[2].z,
		mTModelToBox.x, mTModelToBox.y, mTModelToBox.z, mBoxExtents.x, mBoxExtents.y, mBoxExtents.z);*/
	return TriBoxOverlap(vert0, vert1, vert2, rot_0, rot_1, rot_2, tran, extents);
}

PX_FORCE_INLINE Ps::IntBool OBBCollider::TriBoxOverlap
(const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2, const Vec3V& tran, const Vec3V& extents) const
{
	BoolV ffff = BFFFF();

	//transform obb to origin
	Vec3V v0 = V3LoadU(vert0);
	Vec3V v1 = V3LoadU(vert1);
	Vec3V v2 = V3LoadU(vert2);

	//Note this transform is different to the LSS loose version.
	v0 = V3Add(v0, tran);
	v1 = V3Add(v1, tran);
	v2 = V3Add(v2, tran);

	// Test triangle AABB
	Vec3V triMin = V3Min(v0, V3Min(v1, v2));
	Vec3V triMax = V3Max(v0, V3Max(v1, v2));

	BoolV mask = BOr(V3IsGrtr(triMin, extents), V3IsGrtr(V3Neg(extents), triMax));
	
	return BAllEq(mask, ffff);		
}

Ps::IntBool OBBCollider::TriBoxOverlap
(const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2,
 const Vec3V& rot_0, const Vec3V& rot_1, const Vec3V& rot_2, 
 const Vec3V& tran, const Vec3V& extents) const
{
	Mat33V rot(rot_0, rot_1, rot_2);
	BoolV ffff = BFFFF();

	//transform obb to origin
	Vec3V v0 = V3LoadU(vert0);
	Vec3V v1 = V3LoadU(vert1);
	Vec3V v2 = V3LoadU(vert2);

	//Note this transform is different to the LSS loose version.
	v0 = M33MulV3AddV3(rot, v0, tran);
	v1 = M33MulV3AddV3(rot, v1, tran);
	v2 = M33MulV3AddV3(rot, v2, tran);

	Vec3V minusExtents =  V3Neg(extents);

	// Test triangle AABB
	Vec3V triMin = V3Min(v0, V3Min(v1, v2));
	Vec3V triMax = V3Max(v0, V3Max(v1, v2));

	BoolV mask = BOr(V3IsGrtr(triMin, extents), V3IsGrtr(minusExtents, triMax));
	if(!BAllEq(mask, ffff))
		return Ps::IntFalse;

	// test tri plane.
	Vec3V edge0 = V3Sub(v1, v0);
	Vec3V edge2 = V3Sub(v0, v2);

	Vec3V normal = V3Cross(edge2, edge0);
	FloatV dist = V3Dot(normal, v0);
	FloatV maxDist = V3Dot(extents, V3Abs(normal));

	if(FOutOfBounds(dist, maxDist))
		return Ps::IntFalse;


	//////// test edge axis
	Vec3V radius = V3Add(extents, extents);

	Vec3V axis;
	FloatV p0, p1, p2, pMin, pMax, axisRadius;

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

	Vec3V minusEdge0 = V3Neg(edge0);

	axis = V3Perm_Zero_1Z_0Y(edge0, minusEdge0);
	p0 = V3Dot(v0, axis);
	p2 = V3Dot(v2, axis);
	axisRadius = V3Dot(radius, V3Abs(axis));

	pMin = FAdd(p0, p2);
	pMax = FAdd(FAbs(FSub(p0, p2)), axisRadius);
	
	if(FOutOfBounds(pMin, pMax))
		return Ps::IntFalse;
	

	Vec3V edge1 = V3Sub(v2, v1);
	Vec3V minusEdge1 = V3Neg(edge1);
	
	
	// axis == [1,0,0] x e1 == [0, -e1.z, e1.y]
	axis = V3Perm_Zero_1Z_0Y(edge1, minusEdge1);
	p0 = V3Dot(v0, axis);
	p1 = V3Dot(v1, axis);
	axisRadius = V3Dot(radius, V3Abs(axis));
	pMin = FAdd(p0, p1);
	pMax = FAdd(FAbs(FSub(p0, p1)), axisRadius);
	
	if(FOutOfBounds(pMin, pMax))
		return Ps::IntFalse;
	

	Vec3V minusEdge2 = V3Neg(edge2);

	
	// axis == [1,0,0] x e2 == [0, -e2.z, e2.y]
	axis = V3Perm_Zero_1Z_0Y(edge2, minusEdge2);
	p0 = V3Dot(v0, axis);
	p1 = V3Dot(v1, axis);
	axisRadius = V3Dot(radius, V3Abs(axis));

	pMin = FAdd(p0, p1);
	pMax = FAdd(FAbs(FSub(p0, p1)), axisRadius);

	if(FOutOfBounds(pMin, pMax))
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
	axisRadius = V3Dot(radius, V3Abs(axis));
	pMin = FAdd(p0, p2);
	pMax = FAdd(FAbs(FSub(p0, p2)), axisRadius);

	if(FOutOfBounds(pMin, pMax))
		return Ps::IntFalse;

	// axis == [0,1,0] x e1 == [e1.z, 0, -e1.x]
	axis = V3Perm_0Z_Zero_1X(edge1, minusEdge1);
	p0 = V3Dot(v0, axis);
	p1 = V3Dot(v1, axis);
	axisRadius = V3Dot(radius, V3Abs(axis));

	pMin = FAdd(p0, p1);
	pMax = FAdd(FAbs(FSub(p0, p1)), axisRadius);

	if(FOutOfBounds(pMin, pMax))
		return Ps::IntFalse;

	// axis == [0, 1, 0] x e2 == [e2.z, 0, -e2.x]
	axis = V3Perm_0Z_Zero_1X(edge2, minusEdge2);
	p0 = V3Dot(v0, axis);
	p1 = V3Dot(v1, axis);
	axisRadius = V3Dot(radius, V3Abs(axis));

	pMin = FAdd(p0, p1);
	pMax = FAdd(FAbs(FSub(p0, p1)), axisRadius);

	if(FOutOfBounds(pMin, pMax))
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
	axisRadius = V3Dot(radius, V3Abs(axis));

	pMin = FAdd(p0, p2);
	pMax = FAdd(FAbs(FSub(p0, p2)), axisRadius);

	if(FOutOfBounds(pMin, pMax))
		return Ps::IntFalse;


	// axis == [0, 0, 1] x e1 == [-e1.y, e1.x, 0]
	axis = V3Perm_1Y_0X_Zero(edge1, minusEdge1);
	p0 = V3Dot(v0, axis);
	p1 = V3Dot(v1, axis);
	axisRadius = V3Dot(radius, V3Abs(axis));

	pMin = FAdd(p0, p1);
	pMax = FAdd(FAbs(FSub(p0,p1)), axisRadius);

	if(FOutOfBounds(pMin, pMax))
		return Ps::IntFalse;

	// axis == [0, 0, 1] x e2 == [-e2.y, e2.x, 0]
	axis = V3Perm_1Y_0X_Zero(edge2, minusEdge2);
	p0 = V3Dot(v0, axis);
	p1 = V3Dot(v1, axis);
	axisRadius = V3Dot(radius, V3Abs(axis));

	pMin = FAdd(p0, p1);
	pMax = FAdd(FAbs(FSub(p0, p1)), axisRadius);

	if(FOutOfBounds(pMin, pMax))
		return Ps::IntFalse;

	return Ps::IntTrue;
}

#endif //OPC_SUPPORT_VMX128

} // namespace Gu

}
