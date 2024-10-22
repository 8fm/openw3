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

#include "GuShapeConvex.h"
#include "GuBigConvexData.h"
#include "GuEdgeListData.h"
#include "GuGeomUtilsInternal.h"

#include "CmMatrix34.h"
//#include "PxcSupportVertexMap.h"
#include "GuProjection.h"
#include "GuTriangleVertexPointers.h"
#include "GuBoxProjection.h"
#include "GuHillClimbing.h"

using namespace physx;
using namespace Gu;

PX_FORCE_INLINE PxU32 selectClosestPolygon(PxReal& maxDp_, PxU32 numPolygons, const Gu::HullPolygonData* polys, const PxVec3& axis)
{
	float maxDp = polys[0].mPlane.n.dot(axis);
#ifdef _XBOX
	float closest = 0.0f;
#else
	PxU32 closest = 0;
#endif

	// Loop through polygons
	for(PxU32 i=1; i <numPolygons; i++)
	{
		// Catch current polygon and test its plane
		const PxReal dp = polys[i].mPlane.n.dot(axis);
#ifdef _XBOX
		const float delta = maxDp - dp;
		maxDp = physx::intrinsics::fsel(delta, maxDp, dp);
		closest = physx::intrinsics::fsel(delta, closest, float(i));
#else
		if(dp>maxDp)
		{
			maxDp = dp;
			closest = i;
		}
#endif
	}
	maxDp_ = maxDp;
#ifdef _XBOX
	return PxU32(closest);
#else
	return closest;
#endif
}

static PxU32 SelectClosestEdgeCB_Convex(const PolygonalData& data, const Cm::FastVertex2ShapeScaling& scaling, const PxVec3& localSpaceDirection)
{
	//vertex1TOShape1Skew is a symmetric matrix.  
	//it has the property that (vertex1TOShape1Skew * v)|localSpaceDirection == (vertex1TOShape1Skew * localSpaceDirection)|v 
	const PxVec3 vertexSpaceDirection = scaling * localSpaceDirection;

	const Gu::HullPolygonData* PX_RESTRICT polys = data.mPolygons;

	PxReal maxDp;
	// ##might not be needed
	PxU32 closest = ::selectClosestPolygon(maxDp, data.mNbPolygons, polys, vertexSpaceDirection);

	// Since the convex is closed, at least some poly must satisfy this
	PX_ASSERT(maxDp>=0);

	const PxU32 numEdges = data.mNbEdges;
	const PxU8* const edgeToFace = data.mFacesByEdges;

	//Loop through edges
	PxU32 closestEdge = 0xffffffff;
	PxReal maxDpSq = maxDp * maxDp;
	for(PxU32 i=0; i < numEdges; i++)
	{
		const PxU8 f0 = edgeToFace[i*2];
		const PxU8 f1 = edgeToFace[i*2+1];

		// unnormalized edge normal
		const PxVec3 edgeNormal = polys[f0].mPlane.n + polys[f1].mPlane.n;
		const PxReal enMagSq = edgeNormal.magnitudeSquared();
		//Test normal of current edge - squared test is valid if dp and maxDp both >= 0
		const float dp = edgeNormal.dot(vertexSpaceDirection);
		if(dp>=0.0f && dp*dp>maxDpSq*enMagSq)
		{
			maxDpSq = dp*dp/enMagSq;
			closestEdge = i;
		}
	}

	if(closestEdge!=0xffffffff)
	{
		const PxU8* FBE = edgeToFace;

		const PxU32 f0 = FBE[closestEdge*2];
		const PxU32 f1 = FBE[closestEdge*2+1];

		const PxReal dp0 = polys[f0].mPlane.n.dot(vertexSpaceDirection);
		const PxReal dp1 = polys[f1].mPlane.n.dot(vertexSpaceDirection);
		if(dp0>dp1)
			closest = f0;
		else
			closest = f1;
	}
	return closest;
}
 


static void gHullPrefetchCB(PxU32 numVerts, const PxVec3* PX_RESTRICT verts)
{
	PX_UNUSED(numVerts);
	PX_UNUSED(verts);

#ifdef _XBOX
	const PxU32 vertexSize = numVerts * sizeof(PxVec3);
	const PxU32 prefetchSize = vertexSize<1024 ? vertexSize : 1024;

	const PxU8* PX_RESTRICT adr = reinterpret_cast<const PxU8*>(verts);
//	const PxU8* PX_RESTRICT end = adr + numVerts * sizeof(PxVec3);
	const PxU8* PX_RESTRICT end = adr + prefetchSize;
	while(adr<end)
	{
		Ps::prefetchLine(adr);
		adr += 128;
	}
#endif
}

// Hull projection callback for "small" hulls
#ifdef _XBOX

// PT: we need a new version doing the scaling with VMX code, to get rid of an expensive LHS here
static PX_FORCE_INLINE void pointsOnDir_VMX(const Vec3V dir4, const Vec3V offset4,
											const PxVec3* PX_RESTRICT verts, PxU32 numVerts,
											FloatV& min4, FloatV& max4)

{
	//split to eliminate dependance within loop.
	FloatV min4_1 = FMax();
	FloatV max4_1 = FNegMax();
	FloatV min4_2 = min4_1;
	FloatV max4_2 = max4_1;
	FloatV min4_3 = min4_1;
	FloatV max4_3 = max4_1;
	FloatV min4_4 = min4_1;
	FloatV max4_4 = max4_1;

	const PxU32 numRolled = numVerts & 0x3;

	for(PxU32 i=0; i<numRolled; i++)
	{
		const FloatV dp = V3Dot(dir4, V3LoadUnsafeA(*verts));

		min4_1 = V3Min(min4_1, dp);
		max4_1 = V3Max(max4_1, dp);

		verts++;
	}

	const PxU32 numUnRolled = numVerts & ~0x3;
	const PxVec3* PX_RESTRICT verts_end = verts + numUnRolled;

//const PxU8* PX_RESTRICT cacheLimit = ((const PxU8*)verts) + 512;

	while(verts<verts_end)
	{
/*if(((PxU8*)(verts))>cacheLimit)
{
	cacheLimit += 512;
	Ps::prefetchLine(cacheLimit, 0);
	Ps::prefetchLine(cacheLimit, 128);
	Ps::prefetchLine(cacheLimit, 128*2);
	Ps::prefetchLine(cacheLimit, 128*3);
}*/

		const Vec3V verts_0 = V3LoadUnsafeA(verts[0]);
		const Vec3V verts_1 = V3LoadUnsafeA(verts[1]);
		const Vec3V verts_2 = V3LoadUnsafeA(verts[2]);
		const Vec3V verts_3 = V3LoadUnsafeA(verts[3]);

		const FloatV dp_1 = V3Dot(dir4, verts_0);
		const FloatV dp_2 = V3Dot(dir4, verts_1);
		const FloatV dp_3 = V3Dot(dir4, verts_2);
		const FloatV dp_4 = V3Dot(dir4, verts_3);

		min4_1 = V3Min(min4_1, dp_1);
		max4_1 = V3Max(max4_1, dp_1);

		min4_2 = V3Min(min4_2, dp_2);
		max4_2 = V3Max(max4_2, dp_2);

		min4_3 = V3Min(min4_3, dp_3);
		max4_3 = V3Max(max4_3, dp_3);
		
		min4_4 = V3Min(min4_4, dp_4);
		max4_4 = V3Max(max4_4, dp_4);

		verts+=4;
	}

	min4_1 = V3Min(min4_1, min4_2);
	min4_3 = V3Min(min4_3, min4_4);
	const FloatV min4_tmp = V3Min(min4_1, min4_3);
	min4 = V3Add(min4_tmp, offset4);

	max4_1 = V3Max(max4_1, max4_2);
	max4_3 = V3Max(max4_3, max4_4);
	const FloatV max4_tmp = V3Max(max4_1, max4_3);
	max4 = V3Add(max4_tmp, offset4);
}

// Hull projection callback for "small" hulls
static void HullProjectionCB_SmallConvex(	const PolygonalData& data, const PxVec3& dir,
											const Cm::Matrix34& world,
											const Cm::FastVertex2ShapeScaling& scaling,
											PxReal& min, PxReal& max)
{
	const PxVec3* PX_RESTRICT verts = data.mVerts;
	const PxU32 numVerts = data.mNbVerts;
/*
PxU32 contactSize = numVerts * sizeof(PxVec3);
PxU32 prefetchSize = contactSize<1024 ? contactSize : 1024;
const PxU8* PX_RESTRICT prefetchAddr = reinterpret_cast<const PxU8* PX_RESTRICT>(verts);
for(PxU32 offset=0; offset<prefetchSize; offset+=128)
	Ps::prefetchLine(prefetchAddr, offset);
*/
//const PxVec3 localSpaceDirection = world.rotateTranspose(dir);
	const Vec3V dir4 = V3LoadUnsafeA(dir);
	const Vec3V base0 = V3LoadUnsafeA(world.base0);	// ### shouldn't be reloaded all the time!
	const Vec3V base1 = V3LoadUnsafeA(world.base1);	// ### shouldn't be reloaded all the time!
	const Vec3V base2 = V3LoadUnsafeA(world.base2);	// ### shouldn't be reloaded all the time!
	const Vec3V base3 = V3LoadUnsafeA(world.base3);	// ### shouldn't be reloaded all the time!
	const Vec3V localSpaceDirection4 = M33TrnspsMulV3(Mat33V(base0, base1, base2), dir4);
	const FloatV offset4 = V3Dot(base3, dir4);

	//vertex1TOShape1Skew is a symmetric matrix.  
	//it has the property that (vertex1TOShape1Skew * v)|localSpaceDirection == (vertex1TOShape1Skew * localSpaceDirection)|v 
//const PxVec3 vertexSpaceDirection = scaling * localSpaceDirection;	//NB: triangles are always shape 1! eek!

	const PxMat33& newMat = scaling.getVertex2ShapeSkew();
	const Vec3V scaleBase0 = V3LoadUnsafeA(newMat.column0);	// ### shouldn't be reloaded all the time!
	const Vec3V scaleBase1 = V3LoadUnsafeA(newMat.column1);	// ### shouldn't be reloaded all the time!
	const Vec3V scaleBase2 = V3LoadUnsafeA(newMat.column2);	// ### shouldn't be reloaded all the time!
	const Vec3V vertexSpaceDirection4 = M33MulV3(Mat33V(scaleBase0, scaleBase1, scaleBase2), localSpaceDirection4);

	//brute-force, localspace
//	PxcProjection::pointsOnDir(vertexSpaceDirection, verts, numVerts, min, max);
//		const Cm::PxSimd::Vector4 dir4 = Cm::PxSimd::load(vertexSpaceDirection);
		Vec3V min4, max4;

//		pointsOnDir_VMX(dir4, verts, numVerts, min4, max4);
//		pointsOnDir_VMX(localSpaceDirection4, offset4, verts, numVerts, min4, max4);
		pointsOnDir_VMX(vertexSpaceDirection4, offset4, verts, numVerts, min4, max4);

		FStore(min4, &min);
		FStore(max4, &max);
		PX_ASSERT(min <= max);

	// PT: this is a new LHS, wasn't here before!!! Introduced by the scaling code :(
//const PxReal offset = world.base3.dot(dir);
//min += offset;
//max += offset;

	// PT: this one didn't fix it, just moved the LHS
//	PxcProjection::pointsOnDir(vertexSpaceDirection, verts, numVerts, min, max, world.base3.dot(dir));
}
#else
static void HullProjectionCB_SmallConvex(const PolygonalData& data, const PxVec3& dir,
										 const Cm::Matrix34& world,
										 const Cm::FastVertex2ShapeScaling& scaling,
										 PxReal& min, PxReal& max)
{
	const PxVec3* PX_RESTRICT verts = data.mVerts;
	const PxU32 numVerts = data.mNbVerts;

	const PxVec3 localSpaceDirection = world.rotateTranspose(dir);
	//vertex1TOShape1Skew is a symmetric matrix.  
	//it has the property that (vertex1TOShape1Skew * v)|localSpaceDirection == (vertex1TOShape1Skew * localSpaceDirection)|v 
	const PxVec3 vertexSpaceDirection = scaling * localSpaceDirection;

	//brute-force, localspace
	Projection::pointsOnDir(vertexSpaceDirection, verts, numVerts, min, max);

	// PT: this is a new LHS, wasn't here before!!! Introduced by the scaling code :(
	const PxReal offset = world.base3.dot(dir);
	min += offset;
	max += offset;

	// PT: this one didn't fix it, just moved the LHS
//	PxcProjection::pointsOnDir(vertexSpaceDirection, verts, numVerts, min, max, world.base3.dot(dir));
}
#endif

static PxU32 computeNearestOffset(const PxU32 subdiv, const PxVec3& dir)
{
	// ComputeCubemapNearestOffset(const Point& dir, udword subdiv)

	// PT: ok so why exactly was the code duplicated here?
	// PxU32 CI = CubemapLookup(dir,u,v)

	PxU32 index;
	PxReal coeff;
	// find largest axis
	PxReal absNx = PxAbs(dir.x);
	PxReal absNy = PxAbs(dir.y);
	PxReal absNz = PxAbs(dir.z);

	if( absNy > absNx &&
		absNy > absNz)
	{
		//y biggest
		index = 1;
		coeff = 1.0f/absNy;
	}
	else if(absNz > absNx)
	{
		index = 2;
		coeff = 1.0f/absNz;
	}
	else
	{
		index = 0;
		coeff = 1.0f/absNx;
	}

	union
	{
		PxU32 aU32;
		PxReal aFloat;
	} conv;

	conv.aFloat = dir[index];
	PxU32 sign = conv.aU32>>31;
	
	const PxU32 index2 = Ps::getNextIndex3(index);
	const PxU32 index3 = Ps::getNextIndex3(index2);
	PxReal u = dir[index2] * coeff;
	PxReal v = dir[index3] * coeff;
	
	PxU32 CI = (sign | (index+index));

	//Remap to [0, subdiv[
	coeff = 0.5f * PxReal(subdiv-1);
	u += 1.0f; u *= coeff;
	v += 1.0f; v *= coeff;

	//Round to nearest
	PxU32 ui = PxU32(u);
	PxU32 vi = PxU32(v);

	PxReal du = u - PxReal(ui);
	PxReal dv = v - PxReal(vi);
	if(du>0.5f) ui++;
	if(dv>0.5f) vi++;

	//Compute offset
	return CI*(subdiv*subdiv) + ui*subdiv + vi;
}

// Hull projection callback for "big" hulls
static void HullProjectionCB_BigConvex(const PolygonalData& data, const PxVec3& dir, const Cm::Matrix34& world, const Cm::FastVertex2ShapeScaling& scaling, PxReal& minimum, PxReal& maximum)
{
	const PxVec3* PX_RESTRICT verts = data.mVerts;

	const PxVec3 localSpaceDirection = world.rotateTranspose(dir);
	//vertex1TOShape1Skew is a symmetric matrix.  
	//it has the property that (vertex1TOShape1Skew * v)|localSpaceDirection == (vertex1TOShape1Skew * localSpaceDirection)|v 
	const PxVec3 vertexSpaceDirection = scaling * localSpaceDirection;	//NB: triangles are always shape 1! eek!

	// This version is better for objects with a lot of vertices
	const Gu::BigConvexRawData* bigData = data.mBigData;
	PxU32 minID = 0, maxID = 0;
	{
		const PxU32 offset = computeNearestOffset(bigData->mSubdiv, -vertexSpaceDirection);
		minID = bigData->mSamples[offset];
		maxID = bigData->getSamples2()[offset];
	}
	
	// Do hillclimbing!
	localSearch(minID, -vertexSpaceDirection, verts, bigData);
	localSearch(maxID, vertexSpaceDirection, verts, bigData);

	const PxReal offset = world.base3.dot(dir);
	minimum = offset + verts[minID].dot(vertexSpaceDirection);
	maximum = offset + verts[maxID].dot(vertexSpaceDirection);
	PX_ASSERT(maximum >= minimum);
}

void Gu::getPolygonalData_Convex(PolygonalData* PX_RESTRICT dst, const Gu::ConvexHullData* PX_RESTRICT src, const Cm::FastVertex2ShapeScaling& scaling)
{
	dst->mCenter			= scaling * src->mCenterOfMass;
	dst->mNbVerts			= src->mNbHullVertices;
	dst->mNbPolygons		= src->mNbPolygons;
	dst->mNbEdges			= src->mNbEdges;
	dst->mPolygons			= src->mPolygons;
	dst->mVerts				= src->getHullVertices();
	dst->mPolygonVertexRefs	= src->getVertexData8();
	dst->mFacesByEdges		= src->getFacesByEdges8();

// TEST_INTERNAL_OBJECTS
	// PT: weird stuff here:
	// - if we copy the members one by one we get one LHS for each line because of a "integer-to-float" ???. That's 1536*4 LHS in the convex stack scene...
	// - if we use memcpy the LHS disappear but the code gets a lot slower (function call, etc)
	// - best version is when using the IR macro to do the copy. LHS disappear (except the ones from 4Kb boundary aliasing)
	#ifdef _XBOX
//	dst->mInternal.mRadius			= src->mInternal.mRadius;
//	dst->mInternal.mExtents[0]		= src->mInternal.mExtents[0];
//	dst->mInternal.mExtents[1]		= src->mInternal.mExtents[1];
//	dst->mInternal.mExtents[2]		= src->mInternal.mExtents[2];
//	data.mInternal					= mHullData->mInternal;
	IR(dst->mInternal.mRadius)		= IR(src->mInternal.mRadius);
	IR(dst->mInternal.mExtents[0])	= IR(src->mInternal.mExtents[0]);
	IR(dst->mInternal.mExtents[1])	= IR(src->mInternal.mExtents[1]);
	IR(dst->mInternal.mExtents[2])	= IR(src->mInternal.mExtents[2]);
	#else
	dst->mInternal					= src->mInternal;
	#endif
//~TEST_INTERNAL_OBJECTS

	dst->mBigData			= src->mBigConvexRawData;

	// This threshold test doesnt cost much and many customers cook on PC and use this on 360.
	// 360 has a much higher threshold than PC(and it makes a big difference)
	// PT: the cool thing is that this test is now done once by contact generation call, not once by hull projection
	if(!src->mBigConvexRawData)
		dst->mProjectHull = HullProjectionCB_SmallConvex;
	else
		dst->mProjectHull = HullProjectionCB_BigConvex;
	dst->mSelectClosestEdgeCB = SelectClosestEdgeCB_Convex;
	dst->mPrefetchHull = gHullPrefetchCB;
}

// Box emulating convex mesh

// Face0: 0-1-2-3
// Face1: 1-5-6-2
// Face2: 5-4-7-6
// Face3: 4-0-3-7
// Face4; 3-2-6-7
// Face5: 4-5-1-0

//     7+------+6			0 = ---
//     /|     /|			1 = +--
//    / |    / |			2 = ++-
//   / 4+---/--+5			3 = -+-
// 3+------+2 /    y   z	4 = --+
//  | /    | /     |  /		5 = +-+
//  |/     |/      |/		6 = +++
// 0+------+1      *---x	7 = -++

static const PxU8 gPxcBoxPolygonData[] = {
	0, 1, 2, 3,
	1, 5, 6, 2,
	5, 4, 7, 6,
	4, 0, 3, 7,
	3, 2, 6, 7,
	4, 5, 1, 0,
};

// ### this one needs serious checks, manually hardcoded
/*static const PxU16 PxcBoxEdgeData[] = {
	0, 1, 2, 3,
	1, 8, 5, 9,
	6, 7, 4, 5,
	11, 3, 10, 7,
	2, 9, 4, 10,
	6, 8, 0, 11
};*/

#define	INVSQRT2	0.707106781188f				//!< 1 / sqrt(2)
static PxVec3 gPxcBoxEdgeNormals[] = 
{
	PxVec3(0,			-INVSQRT2,	-INVSQRT2),	// 0-1
	PxVec3(INVSQRT2,	0,			-INVSQRT2),	// 1-2
	PxVec3(0,			INVSQRT2,	-INVSQRT2),	// 2-3
	PxVec3(-INVSQRT2,	0,			-INVSQRT2),	// 3-0

	PxVec3(0,			INVSQRT2,	INVSQRT2),	// 7-6
	PxVec3(INVSQRT2,	0,			INVSQRT2),	// 6-5
	PxVec3(0,			-INVSQRT2,	INVSQRT2),	// 5-4
	PxVec3(-INVSQRT2,	0,			INVSQRT2),	// 4-7

	PxVec3(INVSQRT2,	-INVSQRT2,	0),			// 1-5
	PxVec3(INVSQRT2,	INVSQRT2,	0),			// 6-2
	PxVec3(-INVSQRT2,	INVSQRT2,	0),			// 3-7
	PxVec3(-INVSQRT2,	-INVSQRT2,	0)			// 4-0
};
#undef INVSQRT2

/*static PxU8 PxcBoxEdge8[] = {
	0, 1,	1, 2,	2, 3,	3, 0,
	7, 6,	6, 5,	5, 4,	4, 7,
	1, 5,	6, 2,
	3, 7,	4, 0
};*/

// ### needs serious checkings
	// Flags(16), Count(16), Offset(32);
static Gu::EdgeDescData gPxcBoxEdgeDesc[] = {
	{Gu::PX_EDGE_ACTIVE, 2, 0},
	{Gu::PX_EDGE_ACTIVE, 2, 2},
	{Gu::PX_EDGE_ACTIVE, 2, 4},
	{Gu::PX_EDGE_ACTIVE, 2, 6},
	{Gu::PX_EDGE_ACTIVE, 2, 8},
	{Gu::PX_EDGE_ACTIVE, 2, 10},
	{Gu::PX_EDGE_ACTIVE, 2, 12},
	{Gu::PX_EDGE_ACTIVE, 2, 14},
	{Gu::PX_EDGE_ACTIVE, 2, 16},
	{Gu::PX_EDGE_ACTIVE, 2, 18},
	{Gu::PX_EDGE_ACTIVE, 2, 20},
	{Gu::PX_EDGE_ACTIVE, 2, 22},
};

// ### needs serious checkings
static PxU8 gPxcBoxFaceByEdge[] = {
	0,5, 	// Edge 0-1
	0,1, 	// Edge 1-2
	0,4, 	// Edge 2-3
	0,3, 	// Edge 3-0
	2,4, 	// Edge 7-6
	1,2, 	// Edge 6-5
	2,5, 	// Edge 5-4
	2,3, 	// Edge 4-7
	1,5, 	// Edge 1-5
	1,4, 	// Edge 6-2
	3,4, 	// Edge 3-7
	3,5, 	// Edge 4-0
};

static PxU32 SelectClosestEdgeCB_Box(const PolygonalData& data, const Cm::FastVertex2ShapeScaling& scaling, const PxVec3& localDirection)
{
	PX_UNUSED(scaling);

	PxReal maxDp;
	// ##might not be needed
	PxU32 closest = ::selectClosestPolygon(maxDp, 6, data.mPolygons, localDirection);

	PxU32 numEdges = 12;
	const PxVec3* PX_RESTRICT edgeNormals = gPxcBoxEdgeNormals;

	//Loop through edges
	PxU32 closestEdge = 0xffffffff;
	for(PxU32 i=0; i < numEdges; i++)
	{
		//Test normal of current edge
		const float dp = edgeNormals[i].dot(localDirection);
		if(dp>maxDp)
		{
			maxDp = dp;
			closestEdge = i;
		}
	}

	if(closestEdge!=0xffffffff)
	{
		const Gu::EdgeDescData* PX_RESTRICT ED = gPxcBoxEdgeDesc;
		const PxU8* PX_RESTRICT FBE = gPxcBoxFaceByEdge;

		PX_ASSERT(ED[closestEdge].Count==2);
		const PxU32 f0 = FBE[ED[closestEdge].Offset];
		const PxU32 f1 = FBE[ED[closestEdge].Offset+1];

		const PxReal dp0 = data.mPolygons[f0].mPlane.n.dot(localDirection);
		const PxReal dp1 = data.mPolygons[f1].mPlane.n.dot(localDirection);
		if(dp0>dp1)
			closest = f0;
		else
			closest = f1;
	}

	return closest;
}

static void	HullProjectionCB_Box(const PolygonalData& data, const PxVec3& dir, const Cm::Matrix34& world, const Cm::FastVertex2ShapeScaling& scaling, PxReal& minimum, PxReal& maximum)
{
	PX_UNUSED(scaling);

	const PxVec3 localDir = world.rotateTranspose(dir);

	PxVec3 p;
	projectBox(p, localDir, *data.mHalfSide);

	const PxReal offset = world.base3.dot(dir);
	const PxReal tmp = p.dot(localDir);
	maximum = offset + tmp;
	minimum = offset - tmp;
}

PolygonalBox::PolygonalBox(const PxVec3& halfSide) : mHalfSide(halfSide)
{
	//Precompute the convex data
	//     7+------+6			0 = ---
	//     /|     /|			1 = +--
	//    / |    / |			2 = ++-
	//   / 4+---/--+5			3 = -+-
	// 3+------+2 /    y   z	4 = --+
	//  | /    | /     |  /		5 = +-+
	//  |/     |/      |/		6 = +++
	// 0+------+1      *---x	7 = -++

	PxVec3 minimum = -mHalfSide;
	PxVec3 maximum = mHalfSide;
	// Generate 8 corners of the bbox
	mVertices[0] = PxVec3(minimum.x, minimum.y, minimum.z);
	mVertices[1] = PxVec3(maximum.x, minimum.y, minimum.z);
	mVertices[2] = PxVec3(maximum.x, maximum.y, minimum.z);
	mVertices[3] = PxVec3(minimum.x, maximum.y, minimum.z);
	mVertices[4] = PxVec3(minimum.x, minimum.y, maximum.z);
	mVertices[5] = PxVec3(maximum.x, minimum.y, maximum.z);
	mVertices[6] = PxVec3(maximum.x, maximum.y, maximum.z);
	mVertices[7] = PxVec3(minimum.x, maximum.y, maximum.z);

	//Setup the polygons
	for(PxU8 i=0; i < 6; i++)
	{
		mPolygons[i].mNbVerts = 4;
		mPolygons[i].mVRef8 = i*4;
	}

	// ### planes needs *very* careful checks
	// X axis
	mPolygons[1].mPlane.n = PxVec3(1.0f, 0.0f, 0.0f);
	mPolygons[1].mPlane.d = -mHalfSide.x;
	mPolygons[3].mPlane.n = PxVec3(-1.0f, 0.0f, 0.0f);
	mPolygons[3].mPlane.d = -mHalfSide.x;
	
	mPolygons[1].mMinIndex = 0;
	mPolygons[3].mMinIndex = 1;

//	mPolygons[1].mMinObsolete = -mHalfSide.x;
//	mPolygons[3].mMinObsolete = -mHalfSide.x;

	PX_ASSERT(mPolygons[1].getMin(mVertices) == -mHalfSide.x); 
	PX_ASSERT(mPolygons[3].getMin(mVertices) == -mHalfSide.x);


	// Y axis
	mPolygons[4].mPlane.n = PxVec3(0.f, 1.0f, 0.0f);
	mPolygons[4].mPlane.d = -mHalfSide.y;
	mPolygons[5].mPlane.n = PxVec3(0.0f, -1.0f, 0.0f);
	mPolygons[5].mPlane.d = -mHalfSide.y;

	mPolygons[4].mMinIndex = 0;
	mPolygons[5].mMinIndex = 2;
//	mPolygons[4].mMinObsolete = -mHalfSide.y;
//	mPolygons[5].mMinObsolete = -mHalfSide.y;

	PX_ASSERT(mPolygons[4].getMin(mVertices) == -mHalfSide.y); 
	PX_ASSERT(mPolygons[5].getMin(mVertices) == -mHalfSide.y);

	// Z axis
	mPolygons[2].mPlane.n = PxVec3(0.f, 0.0f, 1.0f);
	mPolygons[2].mPlane.d = -mHalfSide.z;
	mPolygons[0].mPlane.n = PxVec3(0.0f, 0.0f, -1.0f);
	mPolygons[0].mPlane.d = -mHalfSide.z;

	mPolygons[2].mMinIndex = 0;
	mPolygons[0].mMinIndex = 4;
//	mPolygons[2].mMinObsolete = -mHalfSide.z;
//	mPolygons[0].mMinObsolete = -mHalfSide.z;
	PX_ASSERT(mPolygons[2].getMin(mVertices) == -mHalfSide.z); 
	PX_ASSERT(mPolygons[0].getMin(mVertices) == -mHalfSide.z);
}

void PolygonalBox::getPolygonalData(PolygonalData* PX_RESTRICT dst) const
{
	dst->mCenter				= PxVec3(0.0f, 0.0f, 0.0f);
	dst->mNbVerts				= 8;
	dst->mNbPolygons			= 6;
	dst->mPolygons				= mPolygons;
	dst->mNbEdges				= 0;
	dst->mVerts					= mVertices;
	dst->mPolygonVertexRefs		= gPxcBoxPolygonData;
	dst->mFacesByEdges			= NULL;
	dst->mInternal.mRadius		= 0.0f;
	dst->mInternal.mExtents[0]	= 0.0f;
	dst->mInternal.mExtents[1]	= 0.0f;
	dst->mInternal.mExtents[2]	= 0.0f;
//	dst->mBigData				= NULL;
	dst->mHalfSide				= &mHalfSide;
	dst->mProjectHull			= HullProjectionCB_Box;
	dst->mSelectClosestEdgeCB	= SelectClosestEdgeCB_Box;
	dst->mPrefetchHull			= NULL;
}
