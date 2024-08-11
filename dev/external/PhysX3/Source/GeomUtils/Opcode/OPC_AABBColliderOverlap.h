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
namespace Gu
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Triangle-Box overlap test using the separating axis theorem.
 *	This is the code from Tomas Moeller, a bit optimized:
 *	- with some more lazy evaluation (faster path on PC)
 *	- with a tiny bit of assembly
 *	- with "SAT-lite" applied if needed
 *	- and perhaps with some more minor modifs...
 *
 *	\param		center		[in] box center
 *	\param		extents		[in] box extents
 *	\return		true if triangle & box overlap
 */

#ifdef OPC_SUPPORT_VMX128

PX_INLINE Ps::IntBool AABBCollider::TriBoxOverlap(const Vec3V& leafVerts0, const Vec3V& leafVerts1, const Vec3V& leafVerts2,
										 const Vec3V& center, const Vec3V& extents)
{
	// use separating axis theorem to test overlap between triangle and box 
	// need to test for overlap in these directions: 
	// 1) the {x,y,z}-directions (actually, since we use the AABB of the triangle 
	//    we do not even need to test these) 
	// 2) normal of the triangle 
	// 3) crossproduct(edge from tri, {x,y,z}-directin) 
	//    this gives 3x3=9 more tests 

	// move everything so that the boxcenter is in (0,0,0) 
	/*Point v0, v1, v2;
	v0.x = mLeafVerts[0].x - center.x;
	v1.x = mLeafVerts[1].x - center.x;
	v2.x = mLeafVerts[2].x - center.x;*/
	Vec3V v0, v1, v2;
	v0 = V3Sub(leafVerts0, center);
	v1 = V3Sub(leafVerts1, center);
	v2 = V3Sub(leafVerts2, center);

	//// First, test overlap in the {x,y,z}-directions
	//float minimum,maximum;
	//// Find minimum, maximum of the triangle in x-direction, and test for overlap in X
	//FINDMINMAX(v0.x, v1.x, v2.x, minimum, maximum);
	//if(minimum>extents.x || maximum<-extents.x) return Ps::IntFalse;

	//// Same for Y
	//v0.y = mLeafVerts[0].y - center.y;
	//v1.y = mLeafVerts[1].y - center.y;
	//v2.y = mLeafVerts[2].y - center.y;

	//FINDMINMAX(v0.y, v1.y, v2.y, minimum, maximum);
	//if(minimum>extents.y || maximum<-extents.y) return Ps::IntFalse;

	//// Same for Z
	//v0.z = mLeafVerts[0].z - center.z;
	//v1.z = mLeafVerts[1].z - center.z;
	//v2.z = mLeafVerts[2].z - center.z;

	//FINDMINMAX(v0.z, v1.z, v2.z, minimum, maximum);
	//if(minimum>extents.z || maximum<-extents.z) return Ps::IntFalse;	

	Vec3V minusExtents = V3Neg(extents);

	// Test triangle AABB
	Vec3V triMin = V3Min(v0, V3Min(v1, v2));
	Vec3V triMax = V3Max(v0, V3Max(v1, v2));

	BoolV mask = BOr(V3IsGrtr(triMin, extents), V3IsGrtr(minusExtents, triMax));
	if(!BAllEq(mask, BFFFF()))
		return Ps::IntFalse;


	// 2) Test if the box intersects the plane of the triangle
	// compute plane equation of triangle: normal*x+d=0
	// ### could be precomputed since we use the same leaf triangle several times
	/*const Point e0 = v1 - v0;
	const Point e1 = v2 - v1;
	const Point normal = e0 ^ e1;
	const float d = -normal|v0;
	if(!planeBoxOverlap(normal, d, extents)) return Ps::IntFalse;*/

	// test tri plane.
	Vec3V edge0 = V3Sub(v1, v0);
	Vec3V edge2 = V3Sub(v0, v2);
	Vec3V normal = V3Cross(edge2, edge0);
	FloatV dist = V3Dot(normal, v0);
	FloatV maxDist = V3Dot(extents, V3Abs(normal));
	if(FOutOfBounds(dist, maxDist))
		return Ps::IntFalse;


	// 3) "Class III" tests - here we always do full tests since the box is a primitive (not a BV)
	/*{
		IMPLEMENT_CLASS3_TESTS
	}*/

	//////// test edge axis
	Vec3V radius = V3Add(extents, extents);

	Vec3V axis;
	FloatV p0, p1, p2, pMin, pMax, axisRadius;
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

	//OPC_SIMD_PERMUTE(PERMUTE_0Z0W1X0W, OPC_SIMD_0Z, OPC_SIMD_0W, OPC_SIMD_1X, OPC_SIMD_0W);

	Vec3V minusEdge0 = V3Neg(edge0);

	axis = V3Perm_0Z_Zero_1X(edge0, minusEdge0);
	p0 = V3Dot(v0, axis);
	p2 = V3Dot(v2, axis);
	axisRadius = V3Dot(radius, V3Abs(axis));


	pMin = FAdd(p0, p2);
	pMax = FAdd(FAbs(FSub(p0,p2)), axisRadius);
	
	if(FOutOfBounds(pMin, pMax))
		return Ps::IntFalse;



	// axis == [0,1,0] x e1 == [e1.z, 0, -e1.x]

	Vec3V edge1 = V3Sub(v2, v1);
	Vec3V minusEdge1 = V3Neg(edge1);

	axis = V3Perm_0Z_Zero_1X(edge1, minusEdge1);
	p0 = V3Dot(v0, axis);
	p1 = V3Dot(v1, axis);
	axisRadius = V3Dot(radius, V3Abs(axis));

	pMin = FAdd(p0, p1);
	pMax = FAdd(FAbs(FSub(p0, p1)), axisRadius);

	if(FOutOfBounds(pMin, pMax))
		return Ps::IntFalse;


	// axis == [0, 1, 0] x e2 == [e2.z, 0, -e2.x]
	Vec3V minusEdge2 = V3Neg(edge2);

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

	//OPC_SIMD_PERMUTE(PERMUTE_1Y0X0W0W, OPC_SIMD_1Y, OPC_SIMD_0X, OPC_SIMD_0W, OPC_SIMD_0W);

	axis = V3Perm_1Y_0X_Zero(edge0, minusEdge0);
	p0 = V3Dot(v0, axis);
	p2 = V3Dot(v2, axis);
	axisRadius = V3Dot(radius, V3Abs(axis));


	pMin = FAdd(p0, p2);
	pMax = FAdd(FAbs(FSub(p0, p2)), axisRadius);
	
	if(FOutOfBounds(pMin, pMax))
		return Ps::IntFalse;

	axis = V3Perm_1Y_0X_Zero(edge1, minusEdge1);
	p0 = V3Dot(v0, axis);
	p1 = V3Dot(v1, axis);
	axisRadius = V3Dot(radius, V3Abs(axis));

	pMin = FAdd(p0, p1);
	pMax = FAdd(FAbs(FSub(p0, p1)), axisRadius);
	
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

//! ...and another one, jeez
PX_INLINE Ps::IntBool AABBCollider::TriBoxOverlap(const PxVec3& v0a, const PxVec3& v1a, const PxVec3& v2a)
{
	// Hook
	const PxVec3& center	= mBox.mCenter;
	const PxVec3& extents	= mBox.mExtents;

	// use separating axis theorem to test overlap between triangle and box 
	// need to test for overlap in these directions: 
	// 1) the {x,y,z}-directions (actually, since we use the AABB of the triangle 
	//    we do not even need to test these) 
	// 2) normal of the triangle 
	// 3) crossproduct(edge from tri, {x,y,z}-directin) 
	//    this gives 3x3=9 more tests 

	// move everything so that the boxcenter is in (0,0,0) 
	PxVec3 v0, v1, v2;
	v0.x = v0a.x - center.x;
	v1.x = v1a.x - center.x;
	v2.x = v2a.x - center.x;

	// First, test overlap in the {x,y,z}-directions
	float min,max;
	// Find minimum, maximum of the triangle in x-direction, and test for overlap in X
	FINDMINMAX(v0.x, v1.x, v2.x, min, max);
	if(min>extents.x || max<-extents.x) return Ps::IntFalse;

	// Same for Y
	v0.y = v0a.y - center.y;
	v1.y = v1a.y - center.y;
	v2.y = v2a.y - center.y;

	FINDMINMAX(v0.y, v1.y, v2.y, min, max);
	if(min>extents.y || max<-extents.y) return Ps::IntFalse;

	// Same for Z
	v0.z = v0a.z - center.z;
	v1.z = v1a.z - center.z;
	v2.z = v2a.z - center.z;

	FINDMINMAX(v0.z, v1.z, v2.z, min, max);
	if(min>extents.z || max<-extents.z) return Ps::IntFalse;

	// 2) Test if the box intersects the plane of the triangle
	// compute plane equation of triangle: normal*x+d=0
	// ### could be precomputed since we use the same leaf triangle several times
	const PxVec3 e0 = v1 - v0;
	const PxVec3 e1 = v2 - v1;
	const PxVec3 normal = e0.cross(e1);
	const float d = -normal.dot(v0);
	if(!planeBoxOverlap(normal, d, extents)) return Ps::IntFalse;

	// 3) "Class III" tests - here we always do full tests since the box is a primitive (not a BV)
	{
		IMPLEMENT_CLASS3_TESTS
	}
	return Ps::IntTrue;
}

} // namespace Gu

}
