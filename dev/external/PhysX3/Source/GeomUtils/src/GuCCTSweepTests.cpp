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

#define STATIC static // AP: this is for quickly disabling sections of code for SPU debugging

#include "PsIntrinsics.h"
#include "PxIntrinsics.h"
#include "GuSweepTests.h"
#include "PxQueryReport.h"


#include "GuHeightFieldUtil.h"
#include "GuEntityReport.h"
#include "CmScaling.h"
#include "PsArray.h"
#include "PsUtilities.h"
#include "PxGeometryQuery.h"

#include "PxCapsuleGeometry.h"
#include "PxSphereGeometry.h"
#include "PxBoxGeometry.h"
#include "PxPlaneGeometry.h"
#include "PxConvexMeshGeometry.h"
#include "PxTriangleMeshGeometry.h"
#include "GuConvexMesh.h"

#include "GuDistanceSegmentTriangle.h"
#include "GuDistanceSegmentBox.h"
#include "GuDistanceSegmentSegment.h"
#include "GuDistancePointBox.h"
#include "GuDistancePointTriangle.h"
#include "GuIntersectionRayPlane.h"
#include "GuIntersectionRayCapsule.h"
#include "GuIntersectionRaySphere.h"
#include "GuIntersectionRayBox.h"
#include "GuIntersectionBoxBox.h"
#include "GuDistancePointSegment.h"
#include "GuIntersectionEdgeEdge.h"
#include "GuIntersectionTriangleBox.h"
#include "GuOverlapTests.h"

#include "GuCapsule.h"
#include "PsAlloca.h"
#include "./Ice/IceUtils.h"
#include "GuIceSupport.h"
#include "GuBoxConversion.h"
#include "GuGeomUtilsInternal.h"
#include "GuConvexUtilsInternal.h"
#include "GuTriangleMesh.h"
#include "GuSPUHelpers.h"
#include "GuVecTriangle.h"
#include "GuVecBox.h"
#include "GuGJKWrapper.h"
#include "GuCCTSweepTests.h"
#include "GuSweepSharedTests.h"

#include "OPC_RayCollider.h" // for inflated raycast


using namespace physx;
using namespace Gu;
using namespace Cm;


static const PxReal gFatBoxEdgeCoeff = 0.01f;
static const PxReal gFatTriangleCoeff = 0.02f;


static const PxVec3 gNearPlaneNormal[] = 
{
	PxVec3(1.0f, 0.0f, 0.0f),
	PxVec3(0.0f, 1.0f, 0.0f),
	PxVec3(0.0f, 0.0f, 1.0f),
	PxVec3(-1.0f, 0.0f, 0.0f),
	PxVec3(0.0f, -1.0f, 0.0f),
	PxVec3(0.0f, 0.0f, -1.0f)
};

///////////////////////////////////////////

// We have separation if one of those conditions is true:
//     -BoxExt > TriMax (box strictly to the right of the triangle)
//      BoxExt < TriMin (box strictly to the left of the triangle
// <=>  d0 = -BoxExt - TriMax > 0
//      d1 = BoxExt - TriMin < 0
// Hence we have overlap if d0 <= 0 and d1 >= 0
// overlap = (d0<=0.0f && d1>=0.0f)
#ifdef _XBOX
	#define TEST_OVERLAP									\
		const float d0 = -BoxExt - TriMax;					\
		const float d1 = BoxExt - TriMin;					\
		const float cndt0i = physx::intrinsics::fsel(d0, 0.0f, 1.0f);		\
		const float cndt1i = physx::intrinsics::fsel(d1, 1.0f, 0.0f);		\
		const float bIntersect = cndt0i * cndt1i;			\
		bValidMTD *= bIntersect;
#else
	#define TEST_OVERLAP									\
		const float d0 = -BoxExt - TriMax;					\
		const float d1 = BoxExt - TriMin;					\
		const bool bIntersect = (d0<=0.0f && d1>=0.0f);		\
		bValidMTD &= bIntersect;
#endif




//Forward declaration of shared functions
bool sweepSphereTriangles(	PxU32 nbTris, const PxTriangle* PX_RESTRICT triangles,		// Triangle data
							const PxVec3& center, const PxReal radius,					// Sphere data
							const PxVec3& unitDir, PxReal distance,						// Ray data
							const PxU32* PX_RESTRICT cachedIndex,						// Cache data
							PxVec3& _hit, PxVec3& _normal, PxReal& _t, PxU32& _index,	// Results
							bool isDoubleSided);										// Query modifiers

#define OUTPUT_TRI(t, p0, p1, p2){	\
t->verts[0] = p0;					\
t->verts[1] = p1;					\
t->verts[2] = p2;					\
t++;}

#if __SPU__
static void OutputTri2(
	PxTriangle*& t, const PxVec3& p0, const PxVec3& p1, const PxVec3& p2, const PxVec3& d,
	PxVec3& denormalizedNormal, const PxU32 i, PxU32*& ids
	)
{
	t->verts[0] = p0;
	t->verts[1] = p1;
	t->verts[2] = p2;
	t->denormalizedNormal(denormalizedNormal);
	if((denormalizedNormal.dot(d))>0.0f)
	{
		PxVec3 Tmp = t->verts[1];
		t->verts[1] = t->verts[2];
		t->verts[2] = Tmp;
	}
	t++; *ids++ = i;
}
#define OUTPUT_TRI2(t, p0, p1, p2, d) OutputTri2(t, p0, p1, p2, d, denormalizedNormal, i, ids);
#else
#define OUTPUT_TRI2(t, p0, p1, p2, d){		\
t->verts[0] = p0;							\
t->verts[1] = p1;							\
t->verts[2] = p2;							\
t->denormalizedNormal(denormalizedNormal);	\
if((denormalizedNormal.dot(d))>0.0f) {		\
PxVec3 Tmp = t->verts[1];					\
t->verts[1] = t->verts[2];					\
t->verts[2] = Tmp;							\
}											\
t++; *ids++ = i; }
#endif

static PxU32 ExtrudeMesh(	PxU32 nbTris, const PxTriangle* triangles,
							const PxVec3& extrusionDir, PxTriangle* tris, PxU32* ids, const PxVec3& dir, 
							const Gu::Box* sweptBounds)
{
	const PxU32* Base = ids;

	for(PxU32 i=0; i<nbTris; i++)
	{
		const PxTriangle& CurrentTriangle = triangles[i];
		//if (edge_flags)
		//	CurrentFlags = edge_flags[i];
		
		// Create triangle normal
		PxVec3 denormalizedNormal;
		CurrentTriangle.denormalizedNormal(denormalizedNormal);

		// Backface culling
		// bool DoCulling = (CurrentFlags & Gu::TriangleCollisionFlag::eDOUBLE_SIDED)==0;
		// bool Culled = (DoCulling && (denormalizedNormal|dir) > 0.0f);
		const bool Culled = (denormalizedNormal.dot(dir)) > 0.0f;
		if(Culled)	continue;

		if (sweptBounds)
		{
			PxVec3 tmp[3];
			tmp[0] = sweptBounds->rotateInv(CurrentTriangle.verts[0] - sweptBounds->center);
			tmp[1] = sweptBounds->rotateInv(CurrentTriangle.verts[1] - sweptBounds->center);
			tmp[2] = sweptBounds->rotateInv(CurrentTriangle.verts[2] - sweptBounds->center);
			const PxVec3 center(0.0f);
			if(!Gu::intersectTriangleBox(center, sweptBounds->extents, tmp[0], tmp[1], tmp[2]))
				continue;
		}

		PxVec3 p0 = CurrentTriangle.verts[0];
		PxVec3 p1 = CurrentTriangle.verts[1];
		PxVec3 p2 = CurrentTriangle.verts[2];

		PxVec3 p0b = p0 + extrusionDir;
		PxVec3 p1b = p1 + extrusionDir;
		PxVec3 p2b = p2 + extrusionDir;

		p0 -= extrusionDir;
		p1 -= extrusionDir;
		p2 -= extrusionDir;

		if(denormalizedNormal.dot(extrusionDir) >= 0.0f)	OUTPUT_TRI(tris, p0b, p1b, p2b)
		else												OUTPUT_TRI(tris, p0, p1, p2)
		*ids++ = i;

		// ### it's probably useless to extrude all the shared edges !!!!!
		//if(CurrentFlags & Gu::TriangleCollisionFlag::eACTIVE_EDGE12)
		{
			OUTPUT_TRI2(tris, p1, p1b, p2b, dir)
			OUTPUT_TRI2(tris, p1, p2b, p2, dir)
		}
		//if(CurrentFlags & Gu::TriangleCollisionFlag::eACTIVE_EDGE20)
		{
			OUTPUT_TRI2(tris, p0, p2, p2b, dir)
			OUTPUT_TRI2(tris, p0, p2b, p0b, dir)
		}
		//if(CurrentFlags & Gu::TriangleCollisionFlag::eACTIVE_EDGE01)
		{
			OUTPUT_TRI2(tris, p0b, p1b, p1, dir)
			OUTPUT_TRI2(tris, p0b, p1, p0, dir)
		}
	}
	return PxU32(ids-Base);
}

static PxU32 ExtrudeBox(const PxBounds3& localBox, const PxTransform* world, const PxVec3& extrusionDir, PxTriangle* tris, const PxVec3& dir)
{
	// Handle the box as a mesh

	PxTriangle boxTris[12];

	PxVec3 p[8];
	Gu::computeBoxPoints(localBox, p);

	const PxU8* PX_RESTRICT indices = Gu::getBoxTriangles();

	for(PxU32 i=0; i<12; i++)
	{
		const PxU8 VRef0 = indices[i*3+0];
		const PxU8 VRef1 = indices[i*3+1];
		const PxU8 VRef2 = indices[i*3+2];

		PxVec3 p0 = p[VRef0];
		PxVec3 p1 = p[VRef1];
		PxVec3 p2 = p[VRef2];
		if(world)
		{
			p0 = world->transform(p0);
			p1 = world->transform(p1);
			p2 = world->transform(p2);
		}

		boxTris[i].verts[0] = p0;
		boxTris[i].verts[1] = p1;
		boxTris[i].verts[2] = p2;
	}
	PxU32 fakeIDs[12*7];
	return ExtrudeMesh(12, boxTris, extrusionDir, tris, fakeIDs, dir, NULL);
}


//
// The problem of testing a swept capsule against a box is transformed into sweeping a sphere (lying at the center
// of the capsule) against the extruded triangles of the box. The box triangles are extruded along the
// capsule segment axis.
//
static bool SweepCapsuleBox(const Gu::Capsule& capsule, const PxTransform& boxWorldPose, const PxVec3& boxDim, const PxVec3& dir, PxReal length, PxVec3& hit, PxReal& min_dist, PxVec3& normal, PxHitFlags hintFlags)
{
	if(!(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP))
	{
		// PT: test if shapes initially overlap
		if(Gu::distanceSegmentBoxSquared(capsule.p0, capsule.p1, boxWorldPose.p, boxDim, PxMat33(boxWorldPose.q)) < capsule.radius*capsule.radius)
		{
			min_dist	= 0.0f;
			normal		= -dir;
			hit			= boxWorldPose.p;	// PT: this is arbitrary
			return true;
		}
	}

	// Extrusion dir = capsule segment
	const PxVec3 ExtrusionDir = (capsule.p1 - capsule.p0)*0.5f;

	// Extrude box
	PxReal MinDist = length;
	bool Status = false;
	{
		const PxBounds3 aabb(-boxDim, boxDim);

		PX_ALLOCA(triangles, PxTriangle, 12*7);
		PxU32 NbTris = ExtrudeBox(aabb, &boxWorldPose, ExtrusionDir, triangles, dir);
		PX_ASSERT(NbTris<=12*7);

		// Sweep sphere vs extruded box
		PxVec3 n;
		PxReal md;
		PxU32 trash;
		
		if(sweepSphereTriangles(NbTris, triangles, capsule.computeCenter(), capsule.radius, dir, length, NULL, hit, n, md, trash, false))
		{
			MinDist = md;
			normal = n;
			Status = true;
		}
	}

	min_dist = MinDist;
	return Status;
}

static bool SweepBoxSphere(const Gu::Box& box, PxReal sphereRadius, const PxVec3& spherePos, const PxVec3& dir, PxReal length, PxReal& min_dist, PxVec3& normal, PxHitFlags hintFlags)
{
	if(!(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP))
	{
		// PT: test if shapes initially overlap
		if(Gu::intersectSphereBox(Gu::Sphere(spherePos, sphereRadius), box))
		{
			// Overlap
			min_dist	= 0.0f;
			normal		= -dir;
//			return false;
			return true;	// PT: TODO: was false, changed to true. Check the consequence on the kinematic CCT.
		}
	}

	PxVec3 WP[8];
	box.computeBoxPoints(WP);
	const PxU8* PX_RESTRICT Edges = Gu::getBoxEdges();
	PxReal MinDist = length;
	bool Status = false;
	for(PxU32 i=0; i<12; i++)
	{
		const PxU8 e0 = *Edges++;
		const PxU8 e1 = *Edges++;
		const Gu::Segment segment(WP[e0], WP[e1]);
		const Gu::Capsule Capsule(segment, sphereRadius);

		PxReal s[2];
		PxU32 n = Gu::intersectRayCapsule(spherePos, dir, Capsule, s);
		if(n)
		{
			PxReal t;
			if (n == 1)	t = s[0];
			else t = (s[0] < s[1]) ? s[0]:s[1];

			if(t>=0.0f && t<=MinDist)
			{
				MinDist = t;

				const PxVec3 ip = spherePos + t*dir;
				Gu::distancePointSegmentSquared(Capsule, ip, &t);

				PxVec3 ip2;
				Capsule.computePoint(ip2, t);

				normal = (ip2 - ip);
				normal.normalize();
				Status = true;
			}
		}
	}

	PxVec3 localPt;
	{
		Cm::Matrix34 M2;
		buildMatrixFromBox(M2, box);

		localPt = M2.rotateTranspose(spherePos - M2.base3);
	}

	Gu::Box WorldBox0 = box;
	Gu::Box WorldBox1 = box;
	Gu::Box WorldBox2 = box;
	WorldBox0.extents.x += sphereRadius;
	WorldBox1.extents.y += sphereRadius;
	WorldBox2.extents.z += sphereRadius;

	const PxVec3* BoxNormals = gNearPlaneNormal;

	const PxVec3 localDir = box.rotateInv(dir);

	PxReal tnear, tfar;
	int plane = Gu::intersectRayAABB(-WorldBox0.extents, WorldBox0.extents, localPt, localDir, tnear, tfar);

	if(plane!=-1 && tnear>=0.0f && tnear <= MinDist)
	{
		MinDist = tnear;
		normal = box.rotate(BoxNormals[plane]);
		Status = true;
	}

	plane = Gu::intersectRayAABB(-WorldBox1.extents, WorldBox1.extents, localPt, localDir, tnear, tfar);

	if(plane!=-1 && tnear>=0.0f && tnear <= MinDist)
	{
		MinDist = tnear;
		normal = box.rotate(BoxNormals[plane]);
		Status = true;
	}

	plane = Gu::intersectRayAABB(-WorldBox2.extents, WorldBox2.extents, localPt, localDir, tnear, tfar);

	if(plane!=-1 && tnear>=0.0f && tnear <= MinDist)
	{
		MinDist = tnear;
		normal = box.rotate(BoxNormals[plane]);
		Status = true;
	}

	min_dist = MinDist;

	return Status;
}

// ### optimize! and refactor. And optimize for aabbs
static bool SweepBoxBox(const Gu::Box& box0, const Gu::Box& box1, const PxVec3& dir, PxReal length, PxVec3& hit, PxVec3& normal, PxReal& t, PxHitFlags hintFlags)
{
	if(!(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP))
	{
		// PT: test if shapes initially overlap

		// ### isn't this dangerous ? It prevents back motions...
		// => It is indeed dangerous. We need to change this for overlap recovery.
		// Overlap *can* sometimes be allowed for gameplay reason (e.g. character-through-glass effect)
		if(Gu::intersectOBBOBB(box0.extents, box0.center, box0.rot, box1.extents, box1.center, box1.rot, true))
		{
			t		= 0.0f;
			normal	= -dir;
			hit		= box0.center;	// PT: this is arbitrary
//			return false;
			return true;	// PT: TODO: this was false, changed it to true. Make sure it still works for the kinematic CCT.
		}
	}

	PxVec3 boxVertices0[8];	box0.computeBoxPoints(boxVertices0);
	PxVec3 boxVertices1[8];	box1.computeBoxPoints(boxVertices1);

	//	float MinDist = PX_MAX_F32;
	PxReal MinDist = length;
	int col = -1;

	// In following VF tests:
	// - the direction is FW/BK since we project one box onto the other *and vice versa*
	// - the normal reaction is FW/BK for the same reason

	// Vertices1 against Box0
	if(1)
	{
		// We need:

		// - Box0 in local space
		const PxVec3 Min0 = -box0.extents;
		const PxVec3 Max0 = box0.extents;

		// - Vertices1 in Box0 space
		Cm::Matrix34 WorldToBox0;
		computeWorldToBoxMatrix(WorldToBox0, box0);

		// - the dir in Box0 space
		const PxVec3 LocalDir0 = WorldToBox0.rotate(dir);

		const PxVec3* BoxNormals0 = gNearPlaneNormal;

		for(PxU32 i=0; i<8; i++)
		{
			PxReal tnear, tfar;
			const int plane = Gu::intersectRayAABB(Min0, Max0, WorldToBox0.transform(boxVertices1[i]), -LocalDir0, tnear, tfar);

			if(plane==-1 || tnear<0.0f)	continue;

			if(tnear <= MinDist)
			{
				MinDist = tnear;
				normal = box0.rotate(BoxNormals0[plane]);
				hit = boxVertices1[i];
				col = 0;
			}
		}
	}

	// Vertices0 against Box1
	if(1)
	{
		// We need:

		// - Box1 in local space
		const PxVec3 Min1 = -box1.extents;
		const PxVec3 Max1 = box1.extents;

		// - Vertices0 in Box1 space
		Cm::Matrix34 WorldToBox1;
		computeWorldToBoxMatrix(WorldToBox1, box1);

		// - the dir in Box1 space
		const PxVec3 LocalDir1 = WorldToBox1.rotate(dir);

		const PxVec3* BoxNormals1 = gNearPlaneNormal;

		for(PxU32 i=0; i<8; i++)
		{
			PxReal tnear, tfar;
			const int plane = Gu::intersectRayAABB(Min1, Max1, WorldToBox1.transform(boxVertices0[i]), LocalDir1, tnear, tfar);

			if(plane==-1 || tnear<0.0f)	continue;

			if(tnear <= MinDist)
			{
				MinDist = tnear;
				normal = box1.rotate(-BoxNormals1[plane]);
				hit = boxVertices0[i] + tnear * dir;
				col = 1;
			}
		}
	}

	if(1)
	{
		const PxU8* PX_RESTRICT Edges0 = Gu::getBoxEdges();
		const PxU8* PX_RESTRICT Edges1 = Gu::getBoxEdges();

		PxVec3 EdgeNormals0[12];
		PxVec3 EdgeNormals1[12];
		for(PxU32 i=0; i<12; i++)	Gu::computeBoxWorldEdgeNormal(box0, i, EdgeNormals0[i]);
		for(PxU32 i=0; i<12; i++)	Gu::computeBoxWorldEdgeNormal(box1, i, EdgeNormals1[i]);

		// Loop through box edges
		for(PxU32 i=0; i<12; i++)	// 12 edges
		{
			if(!(EdgeNormals0[i].dot(dir) >= 0.0f)) continue;

			// Catch current box edge // ### one vertex already known using line-strips

			// Make it fat ###
			PxVec3 p1 = boxVertices0[Edges0[i*2+0]];
			PxVec3 p2 = boxVertices0[Edges0[i*2+1]];
			Ps::makeFatEdge(p1, p2, gFatBoxEdgeCoeff);

			// Loop through box edges
			for(PxU32 j=0;j<12;j++)
			{
				if(EdgeNormals1[j].dot(dir) >= 0.0f) continue;

				// Orientation culling
				// PT: this was commented for some reason, but it fixes the "stuck" bug reported by Ubi.
				// So I put it back. We'll have to see whether it produces Bad Things in particular cases.
				if(EdgeNormals0[i].dot(EdgeNormals1[j]) >= 0.0f)	continue;

				// Catch current box edge

				// Make it fat ###
				PxVec3 p3 = boxVertices1[Edges1[j*2+0]];
				PxVec3 p4 = boxVertices1[Edges1[j*2+1]];
				Ps::makeFatEdge(p3, p4, gFatBoxEdgeCoeff);

				PxReal Dist;
				PxVec3 ip;
				if(Gu::intersectEdgeEdge(p1, p2, dir, p3, p4, Dist, ip))
				{
					if(Dist<=MinDist)
					{
						hit = ip + Dist * dir;

						normal = (p1-p2).cross(p3-p4);
						normal.normalize();
						if((normal.dot(dir)) > 0.0f) normal = -normal;

						col = 2;
						MinDist = Dist;
					}
				}
			}
		}
	}

	if(col==-1)	return false;

	t = MinDist;

	return true;
}


static PxTriangle InflateTriangle(const PxTriangle& triangle, PxReal fat_coeff)
{
	PxTriangle fatTri = triangle;

	// Compute triangle center
	const PxVec3& p0 = triangle.verts[0];
	const PxVec3& p1 = triangle.verts[1];
	const PxVec3& p2 = triangle.verts[2];
	const PxVec3 center = (p0 + p1 + p2)*0.333333333f;

	// Don't normalize?
	// Normalize => add a constant border, regardless of triangle size
	// Don't => add more to big triangles
	for(PxU32 i=0;i<3;i++)
	{
		const PxVec3 v = fatTri.verts[i] - center;
		fatTri.verts[i] += v * fat_coeff;
	}

	return fatTri;
}

// PT: special version to fire N parallel rays against the same tri
static PX_FORCE_INLINE Ps::IntBool RayTriPrecaCull(const PxVec3& orig, const PxVec3& dir, const PxVec3& vert0, const PxVec3& edge1, const PxVec3& edge2, const PxVec3& pvec,
						PxReal det, PxReal oneOverDet, PxReal& t)
{
	// Calculate distance from vert0 to ray origin
	const PxVec3 tvec = orig - vert0;

	// Calculate U parameter and test bounds
	PxReal u = tvec.dot(pvec);
	if((u < 0.0f) || u>det)				return 0;

	// Prepare to test V parameter
	const PxVec3 qvec = tvec.cross(edge1);

	// Calculate V parameter and test bounds
	PxReal v = dir.dot(qvec);
	if((v < 0.0f) || u+v>det)			return 0;

	// Calculate t, scale parameters, ray intersects triangle
	t = edge2.dot(qvec);
	t *= oneOverDet;
	return 1;
}

static PX_FORCE_INLINE Ps::IntBool RayTriPrecaNoCull(const PxVec3& orig, const PxVec3& dir, const PxVec3& vert0, const PxVec3& edge1, const PxVec3& edge2, const PxVec3& pvec,
						PxReal /*det*/, PxReal oneOverDet, PxReal& t)
{
	// Calculate distance from vert0 to ray origin
	const PxVec3 tvec = orig - vert0;

	// Calculate U parameter and test bounds
	PxReal u = (tvec.dot(pvec)) * oneOverDet;
	if((u < 0.0f) || u>1.0f)			return 0;

	// prepare to test V parameter
	const PxVec3 qvec = tvec.cross(edge1);

	// Calculate V parameter and test bounds
	PxReal v = (dir.dot(qvec)) * oneOverDet;
	if((v < 0.0f) || u+v>1.0f)			return 0;

	// Calculate t, ray intersects triangle
	t = (edge2.dot(qvec)) * oneOverDet;
	return 1;
}


static PX_FORCE_INLINE void closestAxis2(const PxVec3& v, PxU32& j, PxU32& k)
{
	// find largest 2D plane projection
	const PxF32 absPx = physx::intrinsics::abs(v.x);
	const PxF32 absPy = physx::intrinsics::abs(v.y);
	const PxF32 absPz = physx::intrinsics::abs(v.z);
#ifdef _XBOX
	const float delta = absPx - absPy;

	float max = physx::intrinsics::fsel(delta, absPx, absPy);
//		float m = physx::intrinsics::fsel(delta, 0.0f, 1.0f);
	float m = physx::intrinsics::fsel(delta, 1.0f, 2.0f);

	const float delta2 = max - absPz;
//		max = physx::intrinsics::fsel(delta2, max, absPz);
//		m = physx::intrinsics::fsel(delta2, m, 2.0f);
	m = physx::intrinsics::fsel(delta2, m, 0.0f);

	j = PxU32(m);
	k=j+1;
	if(k>2)
		k=0;

//		j = Ps::getNextIndex3(i);
//		k = Ps::getNextIndex3(j);

//		return i;
#else
	//PxU32 m = 0;	//x biggest axis
	j = 1;
	k = 2;
	if( absPy > absPx && absPy > absPz)
	{
		//y biggest
		j = 2;
		k = 0;
		//m = 1;
	}
	else if(absPz > absPx)
	{
		//z biggest
		j = 0;
		k = 1;
		//m = 2;
	}
//		return m;
#endif
}

#ifdef PRECOMPUTE_FAT_BOX
#ifdef PRECOMPUTE_FAT_BOX_MORE
struct FatEdgeData
{
	PX_FORCE_INLINE	FatEdgeData(){}
	PX_FORCE_INLINE	~FatEdgeData(){}
	PxPlane		plane;
	PxVec3		p1;
	PxVec3		p2;
	PxVec3		v1;
	PxU32		ii;
	PxU32		jj;
	PxReal		coeff;
	union
	{
		PxReal		edgeNormalDp;
		int			edgeNormalDpBin;
	};
};

#if __SPU__
static void computeFatEdges(const PxVec3* PX_RESTRICT boxVertices, FatEdgeData* PX_RESTRICT fatEdges, const PxVec3& motion)
#else
static PX_FORCE_INLINE void computeFatEdges(const PxVec3* PX_RESTRICT boxVertices, FatEdgeData* PX_RESTRICT fatEdges, const PxVec3& motion)
#endif
{
	const PxU8* PX_RESTRICT Edges = Gu::getBoxEdges();
	const PxVec3* PX_RESTRICT EdgeNormals = Gu::getBoxLocalEdgeNormals();
	// Loop through box edges
	for(PxU32 i=0;i<12;i++)	// 12 edges
	{
		PxVec3 p1 = boxVertices[*Edges++];
		PxVec3 p2 = boxVertices[*Edges++];
		Ps::makeFatEdge(p1, p2, gFatBoxEdgeCoeff);

		fatEdges[i].edgeNormalDp = EdgeNormals[i].dot(motion);

		// While we're at it, precompute some more data for EE tests
		const PxVec3 v1 = p2 - p1;

		// Build plane P based on edge (p1, p2) and direction (dir)
		fatEdges[i].plane.n = v1.cross(motion);
		fatEdges[i].plane.d = -(fatEdges[i].plane.n.dot(p1));

		// find largest 2D plane projection
		PxU32 ii,jj;
	//	Ps::closestAxis(plane.normal, ii, jj);
		closestAxis2(fatEdges[i].plane.n, ii, jj);

		fatEdges[i].coeff = 1.0f / (v1[ii]*motion[jj] - v1[jj]*motion[ii]);

		fatEdges[i].p1 = p1;
		fatEdges[i].p2 = p2;
		fatEdges[i].v1 = v1;
		fatEdges[i].ii = ii;
		fatEdges[i].jj = jj;
	}
}
#endif //PRECOMPUTE_FAT_BOX_MORE
#endif //PRECOMPUTE_FAT_BOX

// PT: specialized version where oneOverDir is available
// PT: why did we change the initial epsilon value?
#define LOCAL_EPSILON_RAY_BOX PX_EPS_F32
//#define LOCAL_EPSILON_RAY_BOX 0.0001f
static PX_FORCE_INLINE int intersectRayAABB2(const PxVec3& minimum, const PxVec3& maximum,
							const PxVec3& ro, const PxVec3& /*rd*/, const PxVec3& oneOverDir,
							float& tnear, float& tfar,
							bool fbx, bool fby, bool fbz)
{
	// PT: this unrolled loop is a lot faster on Xbox

	if(fbx)
		if(ro.x<minimum.x || ro.x>maximum.x)
		{
//			tnear = FLT_MAX;
			return -1;
		}
	if(fby)
		if(ro.y<minimum.y || ro.y>maximum.y)
		{
//			tnear = FLT_MAX;
			return -1;
		}
	if(fbz)
		if(ro.z<minimum.z || ro.z>maximum.z)
		{
//			tnear = FLT_MAX;
			return -1;
		}

#ifdef _XBOX
	const PxReal t1x_candidate = (minimum.x - ro.x) * oneOverDir.x;
	const PxReal t2x_candidate = (maximum.x - ro.x) * oneOverDir.x;
	const PxReal t1y_candidate = (minimum.y - ro.y) * oneOverDir.y;
	const PxReal t2y_candidate = (maximum.y - ro.y) * oneOverDir.y;
	const PxReal t1z_candidate = (minimum.z - ro.z) * oneOverDir.z;
	const PxReal t2z_candidate = (maximum.z - ro.z) * oneOverDir.z;

	const float deltax = t1x_candidate - t2x_candidate;
	const float deltay = t1y_candidate - t2y_candidate;
	const float deltaz = t1z_candidate - t2z_candidate;

	const float t1x = physx::intrinsics::fsel(deltax, t2x_candidate, t1x_candidate);
	const float t1y = physx::intrinsics::fsel(deltay, t2y_candidate, t1y_candidate);
	const float t1z = physx::intrinsics::fsel(deltaz, t2z_candidate, t1z_candidate);

	const float t2x = physx::intrinsics::fsel(deltax, t1x_candidate, t2x_candidate);
	const float t2y = physx::intrinsics::fsel(deltay, t1y_candidate, t2y_candidate);
	const float t2z = physx::intrinsics::fsel(deltaz, t1z_candidate, t2z_candidate);

	const float bxf = physx::intrinsics::fsel(deltax, 3.0f, 0.0f);
	const float byf = physx::intrinsics::fsel(deltay, 4.0f, 1.0f);
	const float bzf = physx::intrinsics::fsel(deltaz, 5.0f, 2.0f);

	tnear = t1x;
	tfar = t2x;
	float ret = bxf;

	const float delta = t1y - tnear;
	tnear = physx::intrinsics::fsel(delta, t1y, tnear);
	ret = physx::intrinsics::fsel(delta, byf, ret);
	tfar = physx::intrinsics::selectMin(tfar, t2y);

	const float delta2 = t1z - tnear;
	tnear = physx::intrinsics::fsel(delta2, t1z, tnear);
	ret = physx::intrinsics::fsel(delta2, bzf, ret);

	tfar = physx::intrinsics::selectMin(tfar, t2z);

	// PT: this fcmp seems cheaper than the alternative LHS below
	// TODO: return a "float bool" here as well, unify with other ray-box code
	if(tnear>tfar || tfar<LOCAL_EPSILON_RAY_BOX)
	{
//		tnear = FLT_MAX;
		return -1;
	}
//	ret = physx::intrinsics::fsel(tfar - tnear, ret, -1.0f);
///	ret = physx::intrinsics::fsel(LOCAL_EPSILON_RAY_BOX - tfar, -1.0f, ret);

	return int(ret);
#else
	PxReal t1x = (minimum.x - ro.x) * oneOverDir.x;
	PxReal t2x = (maximum.x - ro.x) * oneOverDir.x;
	PxReal t1y = (minimum.y - ro.y) * oneOverDir.y;
	PxReal t2y = (maximum.y - ro.y) * oneOverDir.y;
	PxReal t1z = (minimum.z - ro.z) * oneOverDir.z;
	PxReal t2z = (maximum.z - ro.z) * oneOverDir.z;

	int bx;
	int by;
	int bz;

	if(t1x>t2x)
	{
		PxReal t=t1x; t1x=t2x; t2x=t;
		bx = 3;
	}
	else
	{
		bx = 0;
	}

	if(t1y>t2y)
	{
		PxReal t=t1y; t1y=t2y; t2y=t;
		by = 4;
	}
	else
	{
		by = 1;
	}

	if(t1z>t2z)
	{
		PxReal t=t1z; t1z=t2z; t2z=t;
		bz = 5;
	}
	else
	{
		bz = 2;
	}

	int ret;
//	if(t1x>tnear)	// PT: no need to test for the first value
	{
		tnear = t1x;
		ret = bx;
	}
//	tfar = Px::intrinsics::selectMin(tfar, t2x);
	tfar = t2x;		// PT: no need to test for the first value

	if(t1y>tnear)
	{
		tnear = t1y;
		ret = by;
	}
	tfar = physx::intrinsics::selectMin(tfar, t2y);

	if(t1z>tnear)
	{
		tnear = t1z;
		ret = bz;
	}
	tfar = physx::intrinsics::selectMin(tfar, t2z);

	if(tnear>tfar || tfar<LOCAL_EPSILON_RAY_BOX)
		return -1;

	return ret;
#endif
}

// PT: force-inlining this saved 500.000 cycles in the benchmark. Ok to inline, only used once anyway.
static PX_FORCE_INLINE bool intersectEdgeEdge3(const PxPlane& plane, const PxVec3& p1, const PxVec3& p2, const PxVec3& dir, const PxVec3& v1,
						const PxVec3& p3, const PxVec3& p4,
						PxReal& dist, PxVec3& ip, PxU32 i, PxU32 j, const PxReal coeff)
{
	// if colliding edge (p3,p4) does not cross plane return no collision
	// same as if p3 and p4 on same side of plane return 0
	//
	// Derivation:
	// d3 = d(p3, P) = (p3 | plane.n) - plane.d;		Reversed sign compared to Plane::Distance() because plane.d is negated.
	// d4 = d(p4, P) = (p4 | plane.n) - plane.d;		Reversed sign compared to Plane::Distance() because plane.d is negated.
	// if d3 and d4 have the same sign, they're on the same side of the plane => no collision
	// We test both sides at the same time by only testing Sign(d3 * d4).
	// ### put that in the Plane class
	// ### also check that code in the triangle class that might be similar
	const PxReal d3 = plane.distance(p3);

	const PxReal temp = d3 * plane.distance(p4);
	if(temp>0.0f)	return false;
//const float cndt0 = physx::intrinsics::fsel(temp, 0.0f, 1.0f);

	// if colliding edge (p3,p4) and plane are parallel return no collision
	const PxVec3 v2 = p4 - p3;

	const PxReal temp2 = plane.n.dot(v2);
	if(temp2==0.0f)	return false;	// ### epsilon would be better

	// compute intersection point of plane and colliding edge (p3,p4)
	ip = p3-v2*(d3/temp2);

	// compute distance of intersection from line (ip, -dir) to line (p1,p2)
	dist =	(v1[i]*(ip[j]-p1[j])-v1[j]*(ip[i]-p1[i])) * coeff;
	if(dist<0.0f)	return false;
//const float cndt1 = physx::intrinsics::fsel(dist, 1.0f, 0.0f);

	// compute intersection point on edge (p1,p2) line
	ip -= dist*dir;

	// check if intersection point (ip) is between edge (p1,p2) vertices
	const PxReal temp3 = (p1.x-ip.x)*(p2.x-ip.x)+(p1.y-ip.y)*(p2.y-ip.y)+(p1.z-ip.z)*(p2.z-ip.z);
//	if(temp3<0.0f)	return true;	// collision found
//	return false;	// no collision
	return temp3<0.0f;

/*const float cndt2 = physx::intrinsics::fsel(temp3, 0.0f, 1.0f);
	const int ret = int(cndt0 * cndt1 * cndt2);
	return ret!=0;*/
}


// ### stamps
// ### replace fat tris with epsilon stuff
// Do closest tris first ?
// Use support vertices to cull entire tris?
static bool SweepBoxTriangle(const PxTriangle& tri,
					 const PxBounds3& box, const PxVec3* PX_RESTRICT box_vertices,
#ifdef PRECOMPUTE_FAT_BOX
#ifdef PRECOMPUTE_FAT_BOX_MORE
					 const FatEdgeData* PX_RESTRICT fatEdges,
#else
					 const PxVec3* PX_RESTRICT fatEdges,
#endif
#endif
					 const PxVec3& motion, const PxVec3& oneOverMotion,
					 PxVec3& hit, PxVec3& normal, PxReal& d)
{
	// Create triangle normal
	PxVec3 TriNormal;
	tri.denormalizedNormal(TriNormal);

	// Backface culling
//	const bool DoCulling = (edge_flags & Gu::TriangleCollisionFlag::eDOUBLE_SIDED)==0;
	const bool DoCulling = true;
	if(DoCulling && (TriNormal.dot(motion)) >= 0.0f)
		return false;		// ">=" is important !

	//	TriNormal.Normalize();

	// Make fat triangle
	const PxTriangle FatTri = InflateTriangle(tri, gFatTriangleCoeff);

	PxReal MinDist = d;	// Initialize with current best distance
	int col = -1;

	const PxVec3 negMotion = -motion;
	const PxVec3 negInvMotion = -oneOverMotion;

	if(1)
	{
		// ### cull using box-plane distance ?

		const PxVec3 Edge1 = FatTri.verts[1] - FatTri.verts[0];
		const PxVec3 Edge2 = FatTri.verts[2] - FatTri.verts[0];
		const PxVec3 PVec = motion.cross(Edge2);
		const PxReal Det = Edge1.dot(PVec);

		// Box vertices VS triangle
		// We can't use stamps here since we can still find a better TOI for a given vertex,
		// even if that vertex has already been tested successfully against another triangle.
		const PxVec3* VN = (const PxVec3*)Gu::getBoxVertexNormals();

		const PxReal OneOverDet = Det!=0.0f ? 1.0f / Det : 0.0f;

		PxU32 hitIndex=0;
		if(DoCulling)
		{
			if(Det>=LOCAL_EPSILON)
			{
				for(PxU32 i=0;i<8;i++)
				{
					// Orientation culling
					if((VN[i].dot(TriNormal) >= 0.0f))	// Can't rely on triangle normal for double-sided faces
						continue;

					// ### test this
					// ### ok, this causes the bug in level3's v-shaped desk. Not really a real "bug", it just happens
					// that this VF test fixes this case, so it's a bad idea to cull it. Oh, well.
					// If we use a penetration-depth code to fixup bad cases, we can enable this culling again. (also
					// if we find a better way to handle that desk)
					// Discard back vertices
//					if(VN[i].dot(motion)<0.0f)
//						continue;

					// Shoot a ray from vertex against triangle, in direction "motion"
					PxReal t;
					if(!RayTriPrecaCull(box_vertices[i], motion, FatTri.verts[0], Edge1, Edge2, PVec, Det, OneOverDet, t))
						continue;

					//if(t<=OffsetLength)	t=0.0f;
					// Only consider positive distances, closer than current best
					// ### we could test that first on tri vertices & discard complete tri if it's further than current best (or equal!)
					if(t < 0.0f || t > MinDist)
						continue;

					MinDist = t;
					col = 0;
//					hit = box_vertices[i] + t * motion;
					hitIndex = i;
				}
			}
		}
		else
		{
			if(Det<=-LOCAL_EPSILON || Det>=LOCAL_EPSILON)
			{
				for(PxU32 i=0;i<8;i++)
				{
					// ### test this
					// ### ok, this causes the bug in level3's v-shaped desk. Not really a real "bug", it just happens
					// that this VF test fixes this case, so it's a bad idea to cull it. Oh, well.
					// If we use a penetration-depth code to fixup bad cases, we can enable this culling again. (also
					// if we find a better way to handle that desk)
					// Discard back vertices
					//			if(!VN[i].SameDirection(motion))
					//				continue;

					// Shoot a ray from vertex against triangle, in direction "motion"
					PxReal t;
					if(!RayTriPrecaNoCull(box_vertices[i], motion, FatTri.verts[0], Edge1, Edge2, PVec, Det, OneOverDet, t))
						continue;

					//if(t<=OffsetLength)	t=0.0f;
					// Only consider positive distances, closer than current best
					// ### we could test that first on tri vertices & discard complete tri if it's further than current best (or equal!)
					if(t < 0.0f || t > MinDist)
						continue;

					MinDist = t;
					col = 0;
//					hit = box_vertices[i] + t * motion;
					hitIndex = i;
				}
			}
		}

		// Only copy this once, if needed
		if(col==0)
		{
			hit = box_vertices[hitIndex] + MinDist * motion;
			normal = TriNormal;
#ifndef LAZY_NORMALIZE
			normal.normalize();
#endif
		}
	}

	if(1)
	{
		// PT: precompute fabs-test for ray-box
		// - doing this outside of the ray-box function gets rid of 3 fabs/fcmp per call
		// - doing this with integer code removes the 3 remaining fabs/fcmps totally
		// - doing this outside reduces the LHS
#ifdef _XBOX
		const float epsilon = LOCAL_EPSILON_RAY_BOX;
		const int fabsx = (int&)(negMotion.x) & 0x7fffffff;
		const int fabsy = (int&)(negMotion.y) & 0x7fffffff;
		const int fabsz = (int&)(negMotion.z) & 0x7fffffff;
		const int intEps = (int&)epsilon;
		const bool b0 = fabsx<intEps;
		const bool b1 = fabsy<intEps;
		const bool b2 = fabsz<intEps;
#else
		const bool b0 = physx::intrinsics::abs(negMotion.x)<LOCAL_EPSILON_RAY_BOX;
		const bool b1 = physx::intrinsics::abs(negMotion.y)<LOCAL_EPSILON_RAY_BOX;
		const bool b2 = physx::intrinsics::abs(negMotion.z)<LOCAL_EPSILON_RAY_BOX;
#endif

		// ### have this as a param ?
		const PxVec3& Min = box.minimum;
		const PxVec3& Max = box.maximum;

		const PxVec3* BoxNormals = gNearPlaneNormal;

		// Triangle vertices VS box
		// ### use stamps not to shoot shared vertices multiple times
		// ### discard non-convex verts
		for(PxU32 i=0;i<3;i++)
		{
			PxReal tnear, tfar;
			const int plane = intersectRayAABB2(Min, Max, tri.verts[i], negMotion, negInvMotion, tnear, tfar, b0, b1, b2);

			// The following works as well but we need to call "intersectRayAABB" to get a plane index compatible with BoxNormals.
			// We could fix this by unifying the plane indices returned by the different ray-aabb functions...
			//PxVec3 coord;
			//PxReal t;
			//PxU32 status = Gu::rayAABBIntersect2(Min, Max, tri.verts[i], -motion, coord, t);

			// ### don't test -1 ?
			if(plane==-1 || tnear<0.0f)	continue;
//				if(tnear<0.0f)	continue;
			if(tnear <= MinDist)
			{
				MinDist = tnear;	// ### warning, tnear => flips normals
				normal = BoxNormals[plane];
				col = 1;

				hit = tri.verts[i];
			}
		}
/*
		int h;
		PxReal tnear;
		const int plane = intersectRayAABB3(Min, Max, tri, negMotion, negInvMotion, tnear, h, b0, b1, b2);

		if(plane!=-1 && tnear < MinDist)
		{
			MinDist = tnear;	// ### warning, tnear => flips normals
			normal = BoxNormals[plane];
			col = 1;

			hit = tri.verts[h];
		}*/

	}


	if(1)	// Responsible for blocking character in "corners" + jittering when walking on boxes
	{
#ifndef PRECOMPUTE_FAT_BOX_MORE2
		//const PxVec3* EdgeNormals = Gu::getBoxLocalEdgeNormals();
#endif

#ifndef PRECOMPUTE_FAT_BOX
		const PxU8* PX_RESTRICT Edges = Gu::getBoxEdges();
#endif
//			PxVec3 vec0, vec1;

		// Precompute edges
/*			PxVec3 triEdges[3];
		for(PxU32 j=0; j<3; j++)
		{
			PxU32 k = j+1;
			if(k==3)	k=0;
			triEdges[j] = tri.verts[j] - tri.verts[k];
		}*/

		// Loop through box edges
		for(PxU32 i=0;i<12;i++)	// 12 edges
		{
			// Makes unwrap scene fail
//				if((EdgeNormals[i]|motion)<-INVSQRT2)	continue;

			// Catch current box edge // ### one vertex already known using line-strips

#ifndef PRECOMPUTE_FAT_BOX
			// Make it fat ###
			PxVec3 p1 = box_vertices[*Edges++];
			PxVec3 p2 = box_vertices[*Edges++];
#endif

			// ### let this one *after* *Edges++ !
#ifdef PRECOMPUTE_FAT_BOX_MORE
			if(fatEdges[i].edgeNormalDpBin < 0)
				continue;
#else
			if(!(EdgeNormals[i].dot(motion) >= 0.0f)) continue;
#endif

#ifdef PRECOMPUTE_FAT_BOX
#ifdef PRECOMPUTE_FAT_BOX_MORE
			PxVec3 p1 = fatEdges[i].p1;
			PxVec3 p2 = fatEdges[i].p2;
#else
			PxVec3 p1 = fatEdges[i*2];
			PxVec3 p2 = fatEdges[i*2+1];
#endif
#else
			Ps::makeFatEdge(p1, p2, gFatBoxEdgeCoeff);
#endif
//				const PxVec3 p2_p1 = p2 - p1;

#ifndef PRECOMPUTE_FAT_BOX_MORE
			// Precompute data for EE tests
			const PxVec3 v1 = p2 - p1;

			// Build plane P based on edge (p1, p2) and direction (dir)
			PxPlane plane;
			plane.normal = v1^motion;
			//	plane.normal.normalize();
			plane.d = -(plane.normal|p1);

			// find largest 2D plane projection
			PxU32 ii,jj;
			//	Ps::closestAxis(plane.normal, ii, jj);
			closestAxis2(plane.normal, ii, jj);

			const PxReal coeff = 1.0f / (v1[ii]*motion[jj] - v1[jj]*motion[ii]);
#endif

			// Loop through triangle edges
			for(PxU32 j=0; j<3; j++)
			{
				// Discard non-convex edges
//					if(!edge_flags.mRef[j])	continue;
//				if(!(edge_flags & (1<<j)))	continue;

//					if(EdgeNormals1[j].SameDirection(motion))	continue;
//				if(edge_triangle && !edge_triangle->verts[j].isZero() && (edge_triangle->verts[j].dot(motion) >= 0.0f))	continue;

				// Catch current triangle edge
				// j=0 => 0-1
				// j=1 => 1-2
				// j=2 => 2-0
				// => this is compatible with EdgeList
				PxU32 k = j+1;
				if(k==3)	k=0;

				const PxVec3& p3 = tri.verts[j];
				const PxVec3& p4 = tri.verts[k];

//					const PxVec3 p3_p4 = p3-p4;

				PxReal Dist;
				PxVec3 ip;

#ifdef PRECOMPUTE_FAT_BOX_MORE
				bool b1 = intersectEdgeEdge3(fatEdges[i].plane, p1, p2, motion, fatEdges[i].v1, p3, p4, Dist, ip, fatEdges[i].ii, fatEdges[i].jj, fatEdges[i].coeff);
#else
				bool b1 = intersectEdgeEdge3(plane, p1, p2, motion, v1, p3, p4, Dist, ip, ii, jj, coeff);
#endif
//					bool b1 = Gu::intersectEdgeEdge(p1, p2, motion, p3, p4, Dist, ip);
//					bool b1 = Gu::intersectEdgeEdgeNEW(p1, p2, motion, p3, p4, Dist, ip);

/*					PxReal Dist2;
				PxVec3 ip2;
				bool b2 = Gu::intersectEdgeEdgeNEW(p1, p2, motion, p3, p4, Dist2, ip2);
				assert(b1==b2);*/

				// PT: for safety checks. Remove that eventually.

//					PxReal Dist;
//					PxVec3 ip;			
//					bool b1 = intersectEdgeEdge2(p3, p4, triEdges[j], negMotion, p1, p2, p2_p1, Dist, ip);

/*					PxReal Dist2;
				PxVec3 ip2;			
				bool b2 = Gu::intersectEdgeEdge(p1, p2, motion, p3, p4, Dist2, ip2);
				assert(b1==b2);*/

/*					PxReal Dist2;
				PxVec3 ip2;
				bool b2 = intersectEdgeEdge2(p3, p4, -motion, p1, p2, Dist2, ip2);
				assert(b1==b2);*/

				if(b1)	// TODO: PT: refactor this change to CCT code
				{
					if(Dist<=MinDist)
					{
						// PT: skip the cross product for now
//							normal = (p1_p2)^(p3-p4);
//							vec0 = p2_p1;
//							vec1 = triEdges[j];

/*#ifdef PRECOMPUTE_FAT_BOX_MORE
						vec0 = fatEdges[i].v1;
#else
						vec0 = v1;
#endif
						vec1 = p3-p4;
*/

#ifdef PRECOMPUTE_FAT_BOX_MORE
normal = fatEdges[i].v1.cross(p3-p4);
#else
normal = v1^(p3-p4);
#endif

#ifndef LAZY_NORMALIZE
						normal.normalize();
						if((normal|motion)>0.0f)
							normal = -normal;
#endif
						col = 2;
						MinDist = Dist;

//							hit = ip;
						hit = ip + motion*Dist;	// For v3
					}
				}
			}
		}
//			if(col==2)
//				normal = vec0^vec1;
	}

	if(col==-1)	return false;
	d = MinDist;
	return true;
}

#if __SPU__ // AP: uninline on SPU to save space
static int TestAxis(	const PxTriangle& tri, const PxVec3& extents,
						const PxVec3& dir, const PxVec3& axis,
						bool& bValidMTD, float& tfirst, float& tlast)
#else // !SPU
// PT: inlining this one is important. Returning floats looks bad but is faster on Xbox.
#ifdef _XBOX
static PX_FORCE_INLINE float TestAxis(	const PxTriangle& tri, const PxVec3& extents,
						const PxVec3& dir, const PxVec3& axis,
//						bool& bValidMTD,
						float& bValidMTD,
						float& tfirst, float& tlast)
#else
static PX_FORCE_INLINE int TestAxis(	const PxTriangle& tri, const PxVec3& extents,
						const PxVec3& dir, const PxVec3& axis,
						bool& bValidMTD, float& tfirst, float& tlast)
#endif
#endif // !SPU
{
	const float d0t = tri.verts[0].dot(axis);
	const float d1t = tri.verts[1].dot(axis);
	const float d2t = tri.verts[2].dot(axis);

	float TriMin = physx::intrinsics::selectMin(d0t, d1t);
	float TriMax = physx::intrinsics::selectMax(d0t, d1t);
	TriMin = physx::intrinsics::selectMin(TriMin, d2t);
	TriMax = physx::intrinsics::selectMax(TriMax, d2t);

	////////

	const float BoxExt = physx::intrinsics::abs(axis.x)*extents.x + physx::intrinsics::abs(axis.y)*extents.y + physx::intrinsics::abs(axis.z)*extents.z;
	TEST_OVERLAP

	const float v = dir.dot(axis);
	if(physx::intrinsics::abs(v) < 1.0E-6f)
#ifdef _XBOX
//		return float(bIntersect);
		return bIntersect;
#else
		return bIntersect;
#endif
	const float OneOverV = -1.0f / v;

//	float t0 = d0 * OneOverV;
//	float t1 = d1 * OneOverV;
//	if(t0 > t1)	TSwap(t0, t1);
	const float t0_ = d0 * OneOverV;
	const float t1_ = d1 * OneOverV;
	float t0 = physx::intrinsics::selectMin(t0_, t1_);
	float t1 = physx::intrinsics::selectMax(t0_, t1_);

#ifdef _XBOX
	const float cndt0 = physx::intrinsics::fsel(tlast - t0, 1.0f, 0.0f);
	const float cndt1 = physx::intrinsics::fsel(t1 - tfirst, 1.0f, 0.0f);
#else
	if(t0 > tlast)	return false;
	if(t1 < tfirst)	return false;
#endif

//	if(t1 < tlast)	tlast = t1;
	tlast = physx::intrinsics::selectMin(t1, tlast);

//	if(t0 > tfirst)	tfirst = t0;
	tfirst = physx::intrinsics::selectMax(t0, tfirst);

#ifdef _XBOX
//	return int(cndt0*cndt1);
	return cndt0*cndt1;
#else
	return true;
#endif
}


#if __SPU__ // uninline on SPU
static int TestAxisXYZ(	const PxTriangle& tri, const PxVec3& extents,
						const PxVec3& dir, float oneOverDir,
						bool& bValidMTD, float& tfirst, float& tlast, const int XYZ)
#else
#ifdef _XBOX
static PX_FORCE_INLINE float TestAxisXYZ(	const PxTriangle& tri, const PxVec3& extents,
						const PxVec3& dir, float oneOverDir,
						float& bValidMTD,
						float& tfirst, float& tlast, const int XYZ)
#else
static PX_FORCE_INLINE int TestAxisXYZ(	const PxTriangle& tri, const PxVec3& extents,
						const PxVec3& dir, float oneOverDir,
						bool& bValidMTD, float& tfirst, float& tlast, const int XYZ)
#endif
#endif
{
	const float d0t = tri.verts[0][XYZ];
	const float d1t = tri.verts[1][XYZ];
	const float d2t = tri.verts[2][XYZ];

	float TriMin = physx::intrinsics::selectMin(d0t, d1t);
	float TriMax = physx::intrinsics::selectMax(d0t, d1t);
	TriMin = physx::intrinsics::selectMin(TriMin, d2t);
	TriMax = physx::intrinsics::selectMax(TriMax, d2t);

	////////

	const float BoxExt = extents[XYZ];
	TEST_OVERLAP

	const float v = dir[XYZ];
	if(physx::intrinsics::abs(v) < 1.0E-6f)
#ifdef _XBOX
//		return float(bIntersect);
		return bIntersect;
#else
		return bIntersect;
#endif
	const float OneOverV = -1.0f * oneOverDir;

//	float t0 = d0 * OneOverV;
//	float t1 = d1 * OneOverV;
//	if(t0 > t1)	TSwap(t0, t1);
	const float t0_ = d0 * OneOverV;
	const float t1_ = d1 * OneOverV;
	float t0 = physx::intrinsics::selectMin(t0_, t1_);
	float t1 = physx::intrinsics::selectMax(t0_, t1_);

#ifdef _XBOX
	const float cndt0 = physx::intrinsics::fsel(tlast - t0, 1.0f, 0.0f);
	const float cndt1 = physx::intrinsics::fsel(t1 - tfirst, 1.0f, 0.0f);
#else
	if(t0 > tlast)	return false;
	if(t1 < tfirst)	return false;
#endif

//	if(t1 < tlast)	tlast = t1;
	tlast = physx::intrinsics::selectMin(t1, tlast);

//	if(t0 > tfirst)	tfirst = t0;
	tfirst = physx::intrinsics::selectMax(t0, tfirst);

#ifdef _XBOX
	return cndt0*cndt1;
//	return int(cndt0*cndt1);
#else
	return true;
#endif
}


static PX_FORCE_INLINE int TestSeparationAxes(	const PxTriangle& tri, const PxVec3& extents,
								const PxVec3& normal, const PxVec3& dir, const PxVec3& oneOverDir, float tmax, float& tcoll)
{
#ifdef _XBOX
	float bValidMTD = 1.0f;
#else
	bool bValidMTD = true;
#endif
	tcoll = tmax;
	float tfirst = -FLT_MAX;
	float tlast  = FLT_MAX;

	// Triangle normal
#ifdef _XBOX
	if(TestAxis(tri, extents, dir, normal, bValidMTD, tfirst, tlast)==0.0f)
#else
	if(!TestAxis(tri, extents, dir, normal, bValidMTD, tfirst, tlast))
#endif
		return 0;

	// Box normals
#ifdef _XBOX
	if(TestAxisXYZ(tri, extents, dir, oneOverDir.x, bValidMTD, tfirst, tlast, 0)==0.0f)
		return 0;
	if(TestAxisXYZ(tri, extents, dir, oneOverDir.y, bValidMTD, tfirst, tlast, 1)==0.0f)
		return 0;
	if(TestAxisXYZ(tri, extents, dir, oneOverDir.z, bValidMTD, tfirst, tlast, 2)==0.0f)
		return 0;
#else
	if(!TestAxisXYZ(tri, extents, dir, oneOverDir.x, bValidMTD, tfirst, tlast, 0))
		return 0;
	if(!TestAxisXYZ(tri, extents, dir, oneOverDir.y, bValidMTD, tfirst, tlast, 1))
		return 0;
	if(!TestAxisXYZ(tri, extents, dir, oneOverDir.z, bValidMTD, tfirst, tlast, 2))
		return 0;
#endif
	// Edges
	for(PxU32 i=0; i<3; i++)
	{
		int ip1 = i+1;
		if(i>=2)	ip1 = 0;
		const PxVec3 TriEdge = tri.verts[ip1] - tri.verts[i];

#ifdef _XBOX
		{
			const PxVec3 Sep = Ps::cross100(TriEdge);
			if((Sep.dot(Sep))>=1.0E-6f && TestAxis(tri, extents, dir, Sep, bValidMTD, tfirst, tlast)==0.0f)
				return 0;
		}
		{
			const PxVec3 Sep = Ps::cross010(TriEdge);
			if((Sep.dot(Sep))>=1.0E-6f && TestAxis(tri, extents, dir, Sep, bValidMTD, tfirst, tlast)==0.0f)
				return 0;
		}
		{
			const PxVec3 Sep = Ps::cross001(TriEdge);
			if((Sep.dot(Sep))>=1.0E-6f && TestAxis(tri, extents, dir, Sep, bValidMTD, tfirst, tlast)==0.0f)
				return 0;
		}
#else
		{
			const PxVec3 Sep = Ps::cross100(TriEdge);
			if((Sep.dot(Sep))>=1.0E-6f && !TestAxis(tri, extents, dir, Sep, bValidMTD, tfirst, tlast))
				return 0;
		}
		{
			const PxVec3 Sep = Ps::cross010(TriEdge);
			if((Sep.dot(Sep))>=1.0E-6f && !TestAxis(tri, extents, dir, Sep, bValidMTD, tfirst, tlast))
				return 0;
		}
		{
			const PxVec3 Sep = Ps::cross001(TriEdge);
			if((Sep.dot(Sep))>=1.0E-6f && !TestAxis(tri, extents, dir, Sep, bValidMTD, tfirst, tlast))
				return 0;
		}
#endif
	}

	if(tfirst > tmax || tlast < 0.0f)	return 0;
	if(tfirst <= 0.0f)
	{
#ifdef _XBOX
		if(bValidMTD==0.0f)	return 0;
#else
		if(!bValidMTD)	return 0;
#endif
		tcoll = 0.0f;
	}
	else tcoll = tfirst;

	return 1;
}

// PT: SAT-based version, in box space
// AP: uninlining on SPU doesn't help
static PX_FORCE_INLINE int TriBoxSweepTestBoxSpace(	const PxTriangle& tri, const PxVec3& extents,
													const PxVec3& dir, const PxVec3& oneOverDir, float tmax, float& toi, bool isDoubleSided)
{
	// Create triangle normal
	PxVec3 TriNormal;
	tri.denormalizedNormal(TriNormal);

	// Backface culling
	const bool DoCulling = !isDoubleSided;
	if(DoCulling && (TriNormal.dot(dir)) >= 0.0f)	// ">=" is important !
		return 0;

	// The SAT test will properly detect initial overlaps, no need for extra tests
	return TestSeparationAxes(tri, extents, TriNormal, dir, oneOverDir, tmax, toi);
}

bool Gu::sweepCCTCapsule_BoxGeom(GU_CAPSULE_SWEEP_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eBOX);
	PX_UNUSED(inflation);

	const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom);

	if (lss.p0 == lss.p1)  // The capsule is actually a sphere
	{
		//TODO: Check if this is really faster than using a "sphere-aware" version of SweepCapsuleBox

		Gu::Box box;	buildFrom1(box, pose.p, boxGeom.halfExtents, pose.q);
		if(!SweepBoxSphere(box, lss.radius, lss.p0, unitDir, distance, sweepHit.distance, sweepHit.normal, hintFlags))
			return false;

		sweepHit.normal = -sweepHit.normal;
		sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::eNORMAL;
		
		if(hintFlags & PxHitFlag::ePOSITION)
		{
			// The sweep test doesn't compute the impact point automatically, so we have to do it here.
			const PxVec3 NewSphereCenter = lss.p0 + unitDir * sweepHit.distance;
			PxVec3 Closest;
			const PxReal d = Gu::distancePointBoxSquared(NewSphereCenter, box.center, box.extents, box.rot, &Closest);
			PX_UNUSED(d);
			// Compute point on the box, after sweep
			Closest = box.rotate(Closest);
			sweepHit.position = Closest + box.center;
			sweepHit.flags |= PxHitFlag::ePOSITION;
		}
	}
	else
	{
		if(!SweepCapsuleBox(lss, pose, boxGeom.halfExtents, unitDir, distance, sweepHit.position, sweepHit.distance, sweepHit.normal, hintFlags))
			return false;

		sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::eNORMAL;
		
		if(hintFlags & PxHitFlag::ePOSITION)
		{
			// The sweep test doesn't compute the impact point automatically, so we have to do it here.
			Gu::Capsule movedCaps = lss;
			movedCaps.p0 += unitDir * sweepHit.distance;
			movedCaps.p1 += unitDir * sweepHit.distance;
			Gu::Box box;	buildFrom1(box, pose.p, boxGeom.halfExtents, pose.q);
			PxVec3 closest;
			const PxReal d = Gu::distanceSegmentBoxSquared(movedCaps, box, NULL, &closest);
			PX_UNUSED(d);
			// Compute point on the box, after sweep
			closest = pose.q.rotate(closest);
			sweepHit.position = closest + pose.p;
			sweepHit.flags |= PxHitFlag::ePOSITION;
		}
	}

	return true;
}

bool Gu::sweepCCTBox_SphereGeom(GU_BOX_SWEEP_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eSPHERE);
	const PxSphereGeometry& sphereGeom = static_cast<const PxSphereGeometry&>(geom);

	// PT: move to relative space
	const Gu::Box relBox(box.center - pose.p, box.extents, box.rot);

	const PxReal sphereRadius = sphereGeom.radius + inflation;

	if(!SweepBoxSphere(relBox, sphereRadius, PxVec3(0), -unitDir, distance, sweepHit.distance, sweepHit.normal, hintFlags))
		return false;

	sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::eNORMAL;

	if(hintFlags & PxHitFlag::ePOSITION)
	{
		// The sweep test doesn't compute the impact point automatically, so we have to do it here.
		const PxVec3 motion = sweepHit.distance * unitDir;
		const PxVec3 newSphereCenter = - motion;
		PxVec3 closest;
		const PxReal d = Gu::distancePointBoxSquared(newSphereCenter, relBox.center, relBox.extents, relBox.rot, &closest);
		PX_UNUSED(d);
		// Compute point on the box, after sweep
		sweepHit.position = relBox.rotate(closest) + box.center + motion;	// PT: undo move to local space here
		sweepHit.flags |= PxHitFlag::ePOSITION;
	}
	return true;
}

bool Gu::sweepCCTBox_CapsuleGeom(GU_BOX_SWEEP_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eCAPSULE);
	PX_UNUSED(inflation);

	const PxCapsuleGeometry& capsuleGeom = static_cast<const PxCapsuleGeometry&>(geom);

	// PT: move to relative space
	const PxTransform relPose(PxVec3(0), pose.q);
	Gu::Box relBox(box.center - pose.p, box.extents, box.rot);

	PxVec3 n;
	const PxVec3 negDir = -unitDir;

	Gu::Capsule lss;
	Gu::getCapsule(lss, capsuleGeom, relPose);

	const PxTransform boxWorldPose = relBox.getTransform();

	if(!SweepCapsuleBox(lss, boxWorldPose, relBox.extents, negDir, distance, sweepHit.position, sweepHit.distance, n, hintFlags))
		return false;

	sweepHit.normal = -n;
	sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::eNORMAL;

	if(hintFlags & PxHitFlag::ePOSITION)
	{
		// The sweep test doesn't compute the impact point automatically, so we have to do it here.
		relBox.center += (unitDir * sweepHit.distance);
		PxVec3 closest;
		const PxReal d = Gu::distanceSegmentBoxSquared(lss, relBox, NULL, &closest);
		PX_UNUSED(d);
		// Compute point on the box, after sweep
		sweepHit.position = relBox.transform(closest) + pose.p;	// PT: undo move to local space here
		sweepHit.flags |= PxHitFlag::ePOSITION;
	}
	return true;
}

bool Gu::sweepCCTBox_BoxGeom(GU_BOX_SWEEP_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eBOX);
	PX_UNUSED(inflation);

	const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom);

	// PT: move to local space
	const Gu::Box relBox(box.center - pose.p, box.extents, box.rot);
	Gu::Box staticBox;	buildFrom1(staticBox, PxVec3(0), boxGeom.halfExtents, pose.q);

	if(SweepBoxBox(relBox, staticBox, unitDir, distance, sweepHit.position, sweepHit.normal, sweepHit.distance, hintFlags))
	{
		sweepHit.position += pose.p;	// PT: undo move to local space
		sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
		return true;
	}
	return false;
}

// PT: not inlining this rarely-run function makes the benchmark ~500.000 cycles faster...
// PT: using this version all the time makes the benchmark ~300.000 cycles slower. So we just use it as a backup.
static void runBackupProcedure(PxSweepHit& sweepHit, const PxVec3& localDir, const Gu::Box& box, const PxTriangle& currentTriangle, const PxVec3* PX_RESTRICT /*boxVertices*/)
{
	const PxVec3 delta = localDir*sweepHit.distance - localDir*0.1f;
	const PxVec3 mp0 = currentTriangle.verts[0] - delta;
	const PxVec3 mp1 = currentTriangle.verts[1] - delta;
	const PxVec3 mp2 = currentTriangle.verts[2] - delta;

	const Vec3V v0 = V3LoadU(mp0);
	const Vec3V v1 = V3LoadU(mp1);
	const Vec3V v2 = V3LoadU(mp2);
	const Gu::TriangleV triangleV(v0, v1, v2);

	// PT: the box is in the triangle's space already
	const Gu::BoxV boxV(V3LoadU(PxVec3(0.0f)), V3LoadU(box.extents), 
						V3LoadU(PxVec3(1.0f, 0.0f, 0.0f)), V3LoadU(PxVec3(0.0f, 1.0f, 0.0f)), V3LoadU(PxVec3(0.0f, 0.0f, 1.0f)));

	Ps::aos::Vec3V closestA;
	Ps::aos::Vec3V closestB;
	Ps::aos::Vec3V normalV;
	Ps::aos::FloatV sqDistV;
	//PxGJKStatus status_ = GJK(triangleV, boxV, closestA, closestB, normalV, sqDistV);
	PxGJKStatus status_ = GJKLocal(triangleV, boxV, closestA, closestB, normalV, sqDistV);

	if(status_==GJK_NON_INTERSECT)
	{
		//PxVec3 ml_closestA;
		PxVec3 ml_closestB;
		PxVec3 ml_normal;
		//float ml_sqDist;
		//PxVec3_From_Vec3V(closestA, ml_closestA);
		V3StoreU(closestB, ml_closestB);
		V3StoreU(normalV, ml_normal);
		//PxF32_From_FloatV(sqDistV, &ml_sqDist);

		sweepHit.position = ml_closestB + localDir*sweepHit.distance;
		sweepHit.normal = -ml_normal;
	}
	else
	{
		// PT: if the backup procedure fails, we give up
		sweepHit.position = box.center;
		sweepHit.normal = -localDir;
	}

}


static void computeImpactData(PxSweepHit& sweepHit, const Gu::Box& box, const PxVec3& localDir, const PxVec3& localMotion, const PxVec3& oneOverMotion, const PxTriangle& currentTriangle)
{
	// PT: compute impact point/normal in a second pass. Here we simply re-sweep the box against the best triangle,
	// using the feature-based code (which computes impact point and normal). This is not great because:
	// - we know there's an impact so why do all tests again?
	// - the SAT test & the feature-based tests could return different results because of FPU accuracy.
	//   The assert below captures this, but it won't do anything good the day we find a bad case.
	// It would be more robust to move the box to impact position, then compute the closest points between the box and the triangle.
	// But we don't have a ready-to-use box-triangle distance routine, and using GJK just for this seems overkill.
	const PxBounds3 aabb(-box.extents, box.extents);
	PxVec3 boxVertices[8];
	Gu::computeBoxPoints(aabb, boxVertices);

#ifdef PRECOMPUTE_FAT_BOX
	#ifdef PRECOMPUTE_FAT_BOX_MORE
	FatEdgeData fatEdges[12];
	computeFatEdges(boxVertices, fatEdges, localMotion);
	#else
	PxVec3 fatEdges[24];
	computeFatEdges(boxVertices, fatEdges);
	#endif
#endif

	float t = PX_MAX_F32;
	bool ret = SweepBoxTriangle(currentTriangle, aabb, boxVertices,
	#ifdef PRECOMPUTE_FAT_BOX
		fatEdges,
	#endif
		localMotion, oneOverMotion, sweepHit.position, sweepHit.normal, t);
	PX_UNUSED(ret);
	if(!ret)
		runBackupProcedure(sweepHit, localDir, box, currentTriangle, boxVertices);
	
}

// PT: test: new version for CCT, based on code for general sweeps. Just to check it works or not with rotations
// TODO: refactor this and the similar code in sweptBox for box-vs-mesh. Not so easy though.
static bool sweepBoxVsTriangles(PxU32 nbTris, const PxTriangle* triangles, const Gu::Box& box, const PxVec3& unitDir, const PxReal distance, PxSweepHit& sweepHit,
								PxHitFlags hintFlags, const PxU32* cachedIndex=NULL)
{
	if(!nbTris)
		return false;

	// Move to AABB space
	Cm::Matrix34 WorldToBox;
	computeWorldToBoxMatrix(WorldToBox, box);

	const PxVec3 localDir = WorldToBox.rotate(unitDir);
	const PxVec3 localMotion = localDir * distance;

#ifndef USE_SAT_VERSION
	PxBounds3 aabb;
	aabb.maximum = box.extents;
	aabb.minimum = -box.extents;
	PxVec3 boxVertices[8];
	Gu::computeBoxPoints(aabb, boxVertices);

#ifdef PRECOMPUTE_FAT_BOX
	#ifdef PRECOMPUTE_FAT_BOX_MORE
	FatEdgeData fatEdges[12];
	computeFatEdges(boxVertices, fatEdges, localDir * distance);
	#else
	PxVec3 fatEdges[24];
	computeFatEdges(boxVertices, fatEdges);
	#endif
#endif
#endif

	bool status = false;
	sweepHit.distance = distance;	//was PX_MAX_F32, but that may trigger an assert in the caller!

	const PxVec3 oneOverMotion(
		localDir.x!=0.0f ? 1.0f/(localDir.x * distance) : 0.0f,
		localDir.y!=0.0f ? 1.0f/(localDir.y * distance) : 0.0f,
		localDir.z!=0.0f ? 1.0f/(localDir.z * distance) : 0.0f);

// PT: experimental code, don't clean up before I test it more and validate it

// Project box
/*float boxRadius0 =
			PxAbs(dir.x) * box.extents.x
		+	PxAbs(dir.y) * box.extents.y
		+	PxAbs(dir.z) * box.extents.z;*/

float boxRadius =
			PxAbs(localDir.x) * box.extents.x
		+	PxAbs(localDir.y) * box.extents.y
		+	PxAbs(localDir.z) * box.extents.z;

if(0)	// PT: run this to check the box radius is correctly computed
{
	PxVec3 boxVertices2[8];
	box.computeBoxPoints(boxVertices2);
	float dpmin = FLT_MAX;
	float dpmax = -FLT_MAX;
	for(int i=0;i<8;i++)
	{
		const float dp = boxVertices2[i].dot(unitDir);
		if(dp<dpmin)	dpmin = dp;
		if(dp>dpmax)	dpmax = dp;
	}
	const float goodRadius = (dpmax-dpmin)/2.0f;
	PX_UNUSED(goodRadius);
}

const float dpc0 = box.center.dot(unitDir);
float localMinDist = 1.0f;
#ifdef PX_DEBUG
	PxU32 totalTestsExpected = nbTris;
	PxU32 totalTestsReal = 0;
	PX_UNUSED(totalTestsExpected);
	PX_UNUSED(totalTestsReal);
#endif

//	const bool has16BitIndices = tm.mesh.has16BitIndices();

//	const bool isDoubleSided = (PxU32(triMeshGeom.meshFlags & PxMeshGeometryFlag::eDOUBLE_SIDED))!=0;
	const bool isDoubleSided = false;

	const PxU32 idx = cachedIndex ? *cachedIndex : 0;

	PxTriangle currentTriangle;	// in world space
	for(PxU32 ii=0;ii<nbTris;ii++)
	{
		const PxU32 triangleIndex = getTriangleIndex(ii, idx);

		// ### try prefetching here
//		::getScaledTriangle(triMeshGeom, vertexToWorldSkew, currentTriangle, triangleIndex); // ### move to local space...
		currentTriangle = triangles[triangleIndex];

#ifdef _XBOX
		if(CullTriangle(currentTriangle, unitDir, boxRadius, localMinDist*distance, dpc0)==0.0f)
			continue;
#else
		if(!CullTriangle(currentTriangle, unitDir, boxRadius, localMinDist*distance, dpc0))
			continue;
#endif

#ifdef PX_DEBUG
		totalTestsReal++;
#endif
		// Move to box space
		currentTriangle.verts[0] = WorldToBox.transform(currentTriangle.verts[0]);
		currentTriangle.verts[1] = WorldToBox.transform(currentTriangle.verts[1]);
		currentTriangle.verts[2] = WorldToBox.transform(currentTriangle.verts[2]);

#ifdef USE_SAT_VERSION
		PxF32 t = PX_MAX_F32;		// could be better!
		if(TriBoxSweepTestBoxSpace(currentTriangle, box.extents, localMotion, oneOverMotion, localMinDist, t, isDoubleSided))
		{
			if(t <= localMinDist)
			{
				// PT: test if shapes initially overlap
				if(t==0.0f)
				{
					sweepHit.flags		= PxHitFlag::eDISTANCE|PxHitFlag::eNORMAL|PxHitFlag::ePOSITION;
					sweepHit.distance	= 0.0f;
					sweepHit.faceIndex	= triangleIndex;
					sweepHit.position	= box.center;	// PT: this is arbitrary
					sweepHit.normal		= -unitDir;
					return true;
				}

				localMinDist			= t;
				sweepHit.distance		= t * distance;
				sweepHit.faceIndex		= triangleIndex;
				status					= true;
			}
		}
#else
		PxVec3 normal;
		PxVec3 ip;

		float dd = localMinDist;	// Initialize with current best distance

		if(SweepBoxTriangle(currentTriangle, aabb, boxVertices,
#ifdef PRECOMPUTE_FAT_BOX
			fatEdges,
#endif
			localMotion, oneOverMotion, ip, normal, dd))
		{
			if(dd < localMinDist)
			{
				localMinDist			= dd;
				sweepHit.distance		= dd * distance;
				sweepHit.normal			= normal;
				sweepHit.position			= ip;
				sweepHit.faceIndex		= triangleIndex;
				status					= true;
			}
		}
#endif
	}


	if(status)
	{
		sweepHit.flags = PxHitFlag::eDISTANCE;

#ifdef USE_SAT_VERSION
		if(hintFlags & (PxHitFlag::eNORMAL|PxHitFlag::ePOSITION))
		{
			PxTriangle currentTriangle;	// in world space
//			::getScaledTriangle(triMeshGeom, vertexToWorldSkew, currentTriangle, sweepHit.faceIndex); // ### move to local space...
			currentTriangle = triangles[sweepHit.faceIndex];

			// Move to box space
			currentTriangle.verts[0] = WorldToBox.transform(currentTriangle.verts[0]);
			currentTriangle.verts[1] = WorldToBox.transform(currentTriangle.verts[1]);
			currentTriangle.verts[2] = WorldToBox.transform(currentTriangle.verts[2]);

			computeImpactData(sweepHit, box, localDir, localMotion, oneOverMotion, currentTriangle);
		}
#endif

		if(hintFlags & PxHitFlag::eNORMAL)
		{
#ifdef LAZY_NORMALIZE
			sweepHit.normal.normalize();
			if((sweepHit.normal.dot(localMotion))>0.0f)
				sweepHit.normal = -sweepHit.normal;
#endif
			sweepHit.normal = box.rotate(sweepHit.normal);
			sweepHit.flags |= PxHitFlag::eNORMAL;
		}
		if(hintFlags & PxHitFlag::ePOSITION)
		{
			sweepHit.position = box.rotate(sweepHit.position) + box.center;
			sweepHit.flags |= PxHitFlag::ePOSITION;
		}
	}
	return status;
}


bool Gu::sweepCCTBox_MeshGeom(GU_BOX_SWEEP_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eTRIANGLEMESH);
	PX_UNUSED(inflation);

	const PxTriangleMeshGeometry& triMeshGeom = static_cast<const PxTriangleMeshGeometry&>(geom);

	//- optimize this
	//- use the obb sweep collider directly ?
	//const Gu::TriangleMesh& tm = *static_cast<Gu::TriangleMesh*>(triMeshGeom.triangleMesh);
	GU_FETCH_MESH_DATA(triMeshGeom);
	const Gu::RTreeMidphase& collisionModel = meshData->mOpcodeModel;

	Gu::HybridOBBCollider boxCollider; // enabling tests to save code size on SPU to avoid template expansion

	// Compute swept box
	Box sweptBox;
	computeSweptBox(box.extents, box.center, box.rot, unitDir, distance, sweptBox);

	const Cm::Matrix34 vertexToWorldSkew = pose * triMeshGeom.scale;

	Gu::Box vertexSpaceBox;
	computeVertexSpaceOBB(vertexSpaceBox, sweptBox, pose, triMeshGeom.scale);

#ifndef __SPU__
//	Gu::Container tempContainer;
//	tempContainer.Reserve(256);
//	tempContainer.ForceSize(0);
	LocalContainer(tempContainer, 128);
	VolumeColliderContainerCallback callback(tempContainer);
#endif	// __SPU__

	RTreeMidphaseData hmd;	// PT: I suppose doing the "conversion" at runtime is fine
	collisionModel.getRTreeMidphaseData(hmd);

#ifndef __SPU__
	// PT: DO NOT move this away. We want to keep the early exit when the box doesn't touch any triangle.

	// AP: careful with changing the template params - can negatively affect SPU_Sweep module size
	boxCollider.Collide<1,1,1>(vertexSpaceBox, hmd, &callback, NULL, NULL);
	//printf(">>>>\n");

	// Get results
	PxU32 nb = tempContainer.GetNbEntries();
	if(!nb)
		return false;

	const PxU32* PX_RESTRICT indices = tempContainer.GetEntries();
	//printf("count=%d\n", nb);

	#ifdef SORT_TRIS
	float* keys = (float*)PxAlloca(nb*sizeof(float));
	for(PxU32 i=0;i<nb;i++)
	{
		const PxU32 triangleIndex = indices[i];

		Triangle currentTriangle;	// in world space
		::getScaledTriangle(triMeshGeom, vertexToWorldSkew, currentTriangle, triangleIndex);

		float dp0 = currentTriangle.verts[0]|motion;
		float dp1 = currentTriangle.verts[1]|motion;
		float dp2 = currentTriangle.verts[2]|motion;
		float mindp = physx::intrinsics::selectMin(dp0, dp1);
		mindp = physx::intrinsics::selectMin(mindp, dp2);
		keys[i] = mindp;
	}

	PxU32* ranks0 = (PxU32*)PxAlloca(nb*sizeof(PxU32));
	PxU32* ranks1 = (PxU32*)PxAlloca(nb*sizeof(PxU32));
	StackRadixSort(RS, ranks0, ranks1);
	const PxU32* sorted = RS.Sort(keys, nb).GetRanks();
	#endif	//SORT_TRIS

	#ifdef BEST_TRI_FIRST
	float best = FLT_MAX;
	float bestTri;
	for(PxU32 i=0;i<nb;i++)
	{
		const PxU32 triangleIndex = indices[i];

		Triangle currentTriangle;	// in world space
		::getScaledTriangle(triMeshGeom, vertexToWorldSkew, currentTriangle, triangleIndex);

		const float dp0 = currentTriangle.verts[0]|motion;
		const float dp1 = currentTriangle.verts[1]|motion;
		const float dp2 = currentTriangle.verts[2]|motion;
		float mindp = physx::intrinsics::selectMin(dp0, dp1);
		mindp = physx::intrinsics::selectMin(mindp, dp2);

		const float delta = mindp - best;
		best = physx::intrinsics::fsel(delta, mindp, best);
		bestTri = physx::intrinsics::fsel(delta, float(i), bestTri);
/*		if(mindp<best)
		{
			mindp=best;
			bestTri=i;
		}*/
	}
	int ibest = int(bestTri);
	PxU32* indices_ = const_cast<PxU32*>(indices);
	const PxU32 tmp = indices_[ibest];
	indices_[ibest] = indices_[0];
	indices_[0] = tmp;
	#endif	// BEST_TRI_FIRST
#endif	// __SPU__

	// Move to AABB space
	Cm::Matrix34 WorldToBox;
	computeWorldToBoxMatrix(WorldToBox, box);

	const PxVec3 localDir = WorldToBox.rotate(unitDir);
	const PxVec3 localMotion = localDir * distance;

#ifndef USE_SAT_VERSION
	PxBounds3 aabb;
	aabb.maximum = box.extents;
	aabb.minimum = -box.extents;
	PxVec3 boxVertices[8];
	Gu::computeBoxPoints(aabb, boxVertices);

#ifdef PRECOMPUTE_FAT_BOX
	#ifdef PRECOMPUTE_FAT_BOX_MORE
	FatEdgeData fatEdges[12];
	computeFatEdges(boxVertices, fatEdges, localDir * distance);
	#else
	PxVec3 fatEdges[24];
	computeFatEdges(boxVertices, fatEdges);
	#endif
#endif
#endif

	bool status = false;
	sweepHit.distance = distance;	//was PX_MAX_F32, but that may trigger an assert in the caller!

	const PxVec3 oneOverMotion(
		localDir.x!=0.0f ? 1.0f/(localDir.x * distance) : 0.0f,
		localDir.y!=0.0f ? 1.0f/(localDir.y * distance) : 0.0f,
		localDir.z!=0.0f ? 1.0f/(localDir.z * distance) : 0.0f);

	// PT: experimental code, don't clean up before I test it more and validate it

	// Project box
	/*float boxRadius0 =
				PxAbs(dir.x) * box.extents.x
			+	PxAbs(dir.y) * box.extents.y
			+	PxAbs(dir.z) * box.extents.z;*/

	float boxRadius =
				PxAbs(localDir.x) * box.extents.x
			+	PxAbs(localDir.y) * box.extents.y
			+	PxAbs(localDir.z) * box.extents.z;

	if(0)	// PT: run this to check the box radius is correctly computed
	{
		PxVec3 boxVertices2[8];
		box.computeBoxPoints(boxVertices2);
		float dpmin = FLT_MAX;
		float dpmax = -FLT_MAX;
		for(int i=0;i<8;i++)
		{
			const float dp = boxVertices2[i].dot(unitDir);
			if(dp<dpmin)	dpmin = dp;
			if(dp>dpmax)	dpmax = dp;
		}
		const float goodRadius = (dpmax-dpmin)/2.0f;
		PX_UNUSED(goodRadius);
	}

	const float dpc0 = box.center.dot(unitDir);
//	const bool has16BitIndices = tm.mesh.has16BitIndices();

	const bool isDoubleSided = triMeshGeom.meshFlags & PxMeshGeometryFlag::eDOUBLE_SIDED;

#ifdef __SPU__
	struct VolumeColliderContainerLessCallback : VolumeColliderTrigCallback
	{		
		PxSweepHit&					sweepHit;
		const Cm::Matrix34&			vertexToWorldSkew;
		PxReal						distance;
		bool						bDoubleSide;		
		const Gu::Box&				box;
		float						boxRadius;
		float						dpc0;
		const Cm::Matrix34&			WorldToBox;
		const PxVec3&				localMotion;
		const PxVec3&				oneOverMotion;
		const PxVec3&				unitDir;
		const PxHitFlags&	hintFlags;
		bool						status;
		float						localMinDist;
		PxTriangle					hitTriangle;

		VolumeColliderContainerLessCallback( PxSweepHit& sweepHit, const Cm::Matrix34& worldMatrix, PxReal distance, bool doubleSide, 
			const Gu::Box& box, float boxRadius, float dpc0, const Cm::Matrix34& WorldToBox, const PxVec3& localMotion, const PxVec3& oneOverMotion, const PxVec3& unitDir, const PxHitFlags& hintFlags) 
			:sweepHit(sweepHit), vertexToWorldSkew(worldMatrix), distance(distance), bDoubleSide(doubleSide), 
			box(box), boxRadius(boxRadius), dpc0(dpc0), WorldToBox(WorldToBox), localMotion(localMotion), oneOverMotion(oneOverMotion),
			unitDir(unitDir), hintFlags(hintFlags), status(false),localMinDist(1.0f)
		{}
		virtual ~VolumeColliderContainerLessCallback() {}

		virtual bool processResults(PxU32 count, const PxVec3* verts, const PxU32* buf)
		{
			//pxPrintf("count=%d\n", count);
			PxTriangle currentTriangle;	// in world space

			for(PxU32 i = 0; i < count; i++)
			{
				PxU32 triangleIndex = buf[i];
				getScaledTriangle( &verts[3*i], 1, vertexToWorldSkew, &currentTriangle);

		#ifdef _XBOX
				if(CullTriangle(currentTriangle, unitDir, boxRadius, localMinDist*distance, dpc0)==0.0f)
					continue;
		#else
				if(!CullTriangle(currentTriangle, unitDir, boxRadius, localMinDist*distance, dpc0))
					continue;
		#endif

				// Move to box space
				currentTriangle.verts[0] = WorldToBox.transform(currentTriangle.verts[0]);
				currentTriangle.verts[1] = WorldToBox.transform(currentTriangle.verts[1]);
				currentTriangle.verts[2] = WorldToBox.transform(currentTriangle.verts[2]);

				PxF32 t = PX_MAX_F32;		// could be better!
				if(TriBoxSweepTestBoxSpace(currentTriangle, box.extents, localMotion, oneOverMotion, localMinDist, t, bDoubleSide))
				{
					if(t < localMinDist)
					{
						// PT: test if shapes initially overlap
						if(t==0.0f)
						{
							sweepHit.flags		= PxHitFlag::eDISTANCE|PxHitFlag::eNORMAL|PxHitFlag::ePOSITION;
							sweepHit.distance	= 0.0f;
							sweepHit.faceIndex	= triangleIndex;
							sweepHit.position	= box.center;	// PT: this is arbitrary
							sweepHit.normal		= -unitDir;
							return true;
						}
						hitTriangle = currentTriangle;
						localMinDist			= t;
						sweepHit.distance		= t * distance;
						sweepHit.faceIndex		= triangleIndex;
						status					= true;
					}
				}

			}

			//status = sweepCapsuleTriangles(count, tmpt, lss, unitDir, distance, NULL, hit.distance, hit.normal, hit.position, hit.faceIndex, hintFlags, bDoubleSide);
			//if(status)
			//{
			//	hit.faceIndex = buf[hit.faceIndex];
			//	hit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
			//}
			return true;
		}
		
		void getScaledTriangle( const PxVec3* verts, PxU32 triCount, const Cm::Matrix34& worldMatrix, PxTriangle* output)
		{
			for(PxU32 i=0; i < triCount; i++)
			{
				output[i].verts[0] = worldMatrix.transform(verts[3*i+0]);
				output[i].verts[1] = worldMatrix.transform(verts[3*i+1]);
				output[i].verts[2] = worldMatrix.transform(verts[3*i+2]);
			}
		}
	} callback(sweepHit, vertexToWorldSkew, distance, isDoubleSided, box, boxRadius, dpc0, WorldToBox, localMotion, oneOverMotion, unitDir, hintFlags);

	// AP: careful with changing the template params - can negatively affect SPU_Sweep module size
	boxCollider.Collide<1,1,1>(vertexSpaceBox, hmd, &callback, NULL, NULL);
	//pxPrintf(">>>>\n");

	status = callback.status;
#else

	float localMinDist = 1.0f;

	#ifdef PX_DEBUG
		PxU32 totalTestsExpected = nb;
		PxU32 totalTestsReal = 0;
		PX_UNUSED(totalTestsExpected);
		PX_UNUSED(totalTestsReal);
	#endif

	PxTriangle currentTriangle;	// in world space
#ifdef PREFETCH_TRI
//	PxU32 counter = 0;
#endif
	while(nb--)
	{
#ifdef PREFETCH_TRI
/*		if(!counter)
		{
			counter=8;

			PxU32 nbToGo = nb+1;
			if(nbToGo>counter)	nbToGo=counter;

			for(PxU32 np=0;np<nbToGo;np++)
				prefetchTriangle(triMeshGeom, indices[np]);
		}*/
#endif

#ifdef SORT_TRIS
		const PxU32 triangleIndex = indices[*sorted++];
#else
		const PxU32 triangleIndex = *indices++;
#endif
#ifdef PREFETCH_TRI
		if(nb)
			prefetchTriangle(triMeshGeom, *indices);
#endif

		// ### try prefetching here
		::getScaledTriangle(triMeshGeom, vertexToWorldSkew, currentTriangle, triangleIndex); // ### move to local space...
//		::getScaledTriangle(tm, vertexToWorldSkew, currentTriangle, triangleIndex, has16BitIndices);

#ifdef _XBOX
		if(CullTriangle(currentTriangle, unitDir, boxRadius, localMinDist*distance, dpc0)==0.0f)
			continue;
#else
		if(!CullTriangle(currentTriangle, unitDir, boxRadius, localMinDist*distance, dpc0))
			continue;
#endif

#ifdef PX_DEBUG
		totalTestsReal++;
#endif
		// Move to box space
		currentTriangle.verts[0] = WorldToBox.transform(currentTriangle.verts[0]);
		currentTriangle.verts[1] = WorldToBox.transform(currentTriangle.verts[1]);
		currentTriangle.verts[2] = WorldToBox.transform(currentTriangle.verts[2]);

#ifdef USE_SAT_VERSION
		PxF32 t = PX_MAX_F32;		// could be better!
		if(TriBoxSweepTestBoxSpace(currentTriangle, box.extents, localMotion, oneOverMotion, localMinDist, t, isDoubleSided))
		{
			if(t <= localMinDist)
			{
				// PT: test if shapes initially overlap
				if(t==0.0f)
				{
					sweepHit.flags		= PxHitFlag::eDISTANCE|PxHitFlag::eNORMAL|PxHitFlag::ePOSITION;
					sweepHit.distance	= 0.0f;
					sweepHit.faceIndex	= triangleIndex;
					sweepHit.position	= box.center;	// PT: this is arbitrary
					sweepHit.normal		= -unitDir;
					return true;
				}

				localMinDist		= t;
				sweepHit.distance	= t * distance;
				sweepHit.faceIndex	= triangleIndex;
				status				= true;
			}
		}
#else
		PxVec3 normal;
		PxVec3 ip;

		float dd = localMinDist;	// Initialize with current best distance

		if(SweepBoxTriangle(currentTriangle, aabb, boxVertices,
#ifdef PRECOMPUTE_FAT_BOX
			fatEdges,
#endif
			localMotion, oneOverMotion, ip, normal, dd))
		{
			if(dd <= localMinDist)
			{
				localMinDist		= dd;
				sweepHit.distance	= dd * distance;
				sweepHit.normal		= normal;
				sweepHit.position	= ip;
				sweepHit.faceIndex	= triangleIndex;
				status				= true;
			}
		}
#endif
	}
#endif

	if(status)
	{
		sweepHit.flags = PxHitFlag::eDISTANCE;

#ifdef USE_SAT_VERSION
		if(hintFlags & (PxHitFlag::eNORMAL|PxHitFlag::ePOSITION))
		{
	#ifdef __SPU__
			PxTriangle& currentTriangle = callback.hitTriangle;	// in world space
	#else
			PxTriangle currentTriangle;	// in world space
			::getScaledTriangle(triMeshGeom, vertexToWorldSkew, currentTriangle, sweepHit.faceIndex); // ### move to local space...

			// Move to box space
			currentTriangle.verts[0] = WorldToBox.transform(currentTriangle.verts[0]);
			currentTriangle.verts[1] = WorldToBox.transform(currentTriangle.verts[1]);
			currentTriangle.verts[2] = WorldToBox.transform(currentTriangle.verts[2]);
	#endif
			computeImpactData(sweepHit, box, localDir, localMotion, oneOverMotion, currentTriangle);
		}
#endif

		if(hintFlags & PxHitFlag::eNORMAL)
		{
#ifdef LAZY_NORMALIZE
			sweepHit.normal.normalize();
			if((sweepHit.normal.dot(localMotion))>0.0f)
				sweepHit.normal = -sweepHit.normal;
#endif
			sweepHit.normal = box.rotate(sweepHit.normal);
			sweepHit.flags |= PxHitFlag::eNORMAL;
		}
		if(hintFlags & PxHitFlag::ePOSITION)
		{
			sweepHit.position = box.rotate(sweepHit.position) + box.center;
			sweepHit.flags |= PxHitFlag::ePOSITION;
		}
	}
	return status;
}

bool Gu::sweepCCTBox_HeightFieldGeom(GU_BOX_SWEEP_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eHEIGHTFIELD);
	PX_UNUSED(inflation);
	const PxHeightFieldGeometry& heightFieldGeom = static_cast<const PxHeightFieldGeometry&>(geom);

	// Compute swept box
	Gu::Box sweptBox;
	computeSweptBox(box.extents, box.center, box.rot, unitDir, distance, sweptBox);

	//### Temp hack until we can directly collide the OBB against the HF
	const PxTransform sweptBoxTR = sweptBox.getTransform();
	const PxBounds3 bounds = PxBounds3::poseExtent(sweptBoxTR, sweptBox.extents);

	sweepHit.distance = PX_MAX_F32;

	struct LocalReport : Gu::EntityReport<PxU32>
	{
		virtual bool onEvent(PxU32 nb, PxU32* indices)
		{
			for(PxU32 i=0; i<nb; i++)
			{
				const PxU32 triangleIndex = indices[i];

				PxTriangle currentTriangle;	// in world space
				mHFUtil->getTriangle(*mPose, currentTriangle, NULL, NULL, triangleIndex, true, true);

				PxSweepHit sweepHit;
				const bool b = sweepBoxVsTriangles(1, &currentTriangle, mBox, mDir, mDist, sweepHit, mHintFlags, NULL);
				if(b && sweepHit.distance<mHit->distance)
				{
					*mHit = sweepHit;
					mHit->faceIndex	= triangleIndex;
					mStatus			= true;
				}
			}
			return true;
		}

		const Gu::HeightFieldUtil*	mHFUtil;
		const PxTransform*			mPose;
		PxSweepHit*					mHit;
		bool						mStatus;
		Gu::Box						mBox;
		PxVec3						mDir;
		float						mDist;
		PxHitFlags					mHintFlags;
	} myReport;

#ifdef __SPU__
	PX_ALIGN_PREFIX(16)  PxU8 heightFieldBuffer[sizeof(Gu::HeightField)+32] PX_ALIGN_SUFFIX(16);
	Gu::HeightField* heightField = memFetchAsync<Gu::HeightField>(heightFieldBuffer, (uintptr_t)(heightFieldGeom.heightField), sizeof(Gu::HeightField), 1);
	memFetchWait(1);

	g_sampleCache.init((uintptr_t)(heightField->getData().samples), heightField->getData().tilesU);

	const_cast<PxHeightFieldGeometry&>(heightFieldGeom).heightField = heightField;
#endif

	Gu::HeightFieldUtil hfUtil(heightFieldGeom);

	myReport.mBox		= box;
	myReport.mDir		= unitDir;
	myReport.mDist		= distance;
	myReport.mHintFlags	= hintFlags;
	myReport.mHFUtil	= &hfUtil;
	myReport.mStatus	= false;
	myReport.mPose		= &pose;
	myReport.mHit		= &sweepHit;

	hfUtil.overlapAABBTriangles(pose, bounds, PxHfQueryFlags::eWORLD_SPACE, &myReport);

	return myReport.mStatus;
}



bool Gu::SweepCCTBoxTriangles(	PxU32 nbTris, const PxTriangle* triangles,
								const PxBoxGeometry& boxGeom, const PxTransform& boxPose, const PxVec3& dir, const PxReal length, PxVec3& _hit,
								PxVec3& _normal, float& _d, PxU32& _index, const PxU32* cachedIndex, const PxReal inflation, PxHitFlags hintFlags)
{
	PX_UNUSED(inflation);

	if(1)
	{
		// PT: this codepath works with arbitrary rotations but we need to check its speed

		Gu::Box box;
		buildFrom1(box, boxPose.p, boxGeom.halfExtents, boxPose.q);

		PxSweepHit sweepHit;
		const bool status = sweepBoxVsTriangles(nbTris, triangles, box, dir, length, sweepHit, hintFlags, cachedIndex);
		if(status)
		{
			_hit	= sweepHit.position;
			_normal	= sweepHit.normal;
			_d		= sweepHit.distance;
			_index	= sweepHit.faceIndex;
		}
		return status;
	}
#ifdef REMOVED
	else
	{
		// PT: this old codepath was well tested but doesn't work anymore with arbitrary rotations

	PxU32 idx = 0;
	if(cachedIndex)
		idx = *cachedIndex;

	PxBounds3 boxBounds(-boxGeom.halfExtents, boxGeom.halfExtents);
	PxVec3 boxVertices[8];
	Gu::computeBoxPoints(boxBounds, boxVertices);	// Precompute
#ifdef PRECOMPUTE_FAT_BOX
	#ifdef PRECOMPUTE_FAT_BOX_MORE
	FatEdgeData fatEdges[12];
	computeFatEdges(boxVertices, fatEdges, dir);
	#else
	PxVec3 fatEdges[24];
	computeFatEdges(boxVertices, fatEdges);
	#endif
#endif

	//------- early exit --------
	PxVec3 boxCenter = boxPose.p;

	// Project box
	const float boxRadius =
				PxAbs(dir.x) * boxGeom.halfExtents.x
			+	PxAbs(dir.y) * boxGeom.halfExtents.y
			+	PxAbs(dir.z) * boxGeom.halfExtents.z;

	const float dpc0 = boxCenter.dot(dir);
	//---------------------------

	bool Status = false;
	float localMinDist = length;	// Initialize with current best distance
	PxVec3 localHit;
	PxVec3 localNormal;
	PxU32 localIndex=0;
#ifdef USE_SAT_VERSION_CCT
	PxTriangle savedTri;
#endif

	bool hasRotation = (boxPose.q.x != 0.0f || boxPose.q.y != 0.0f || boxPose.q.z != 0.0f || boxPose.q.w != 1.0f);
	// Not sure it is worth it to avoid the rotation in this cases

	const PxVec3 oneOverDir(
		dir.x!=0.0f ? 1.0f/dir.x : 0.0f,
		dir.y!=0.0f ? 1.0f/dir.y : 0.0f,
		dir.z!=0.0f ? 1.0f/dir.z : 0.0f);

	const bool isDoubleSided = false;

	for(PxU32 ii=0; ii<nbTris; ii++)
	{
		const PxU32 i = getTriangleIndex(ii, idx);

#ifdef _XBOX
		if(CullTriangle(triangles[i], dir, boxRadius, localMinDist, dpc0)==0.0f)
			continue;
#else
		if(!CullTriangle(triangles[i], dir, boxRadius, localMinDist, dpc0))
			continue;
#endif

		PxTriangle CurrentTri = triangles[i];
		CurrentTri.verts[0] -= boxPose.p;
		CurrentTri.verts[1] -= boxPose.p;
		CurrentTri.verts[2] -= boxPose.p;
		if(hasRotation)
		{
			CurrentTri.verts[0] = boxPose.rotateInv(CurrentTri.verts[0]);
			CurrentTri.verts[1] = boxPose.rotateInv(CurrentTri.verts[1]);
			CurrentTri.verts[2] = boxPose.rotateInv(CurrentTri.verts[2]);
		}

#ifdef USE_SAT_VERSION_CCT
		PxF32 dd = PX_MAX_F32;	// could be better!
		bool b1 = TriBoxSweepTestBoxSpace(CurrentTri, boxGeom.halfExtents, dir, oneOverDir, localMinDist, dd, isDoubleSided)!=0;
#else
		PxVec3 Hit, Normal;
		float dd = localMinDist;	// Initialize with current best distance
		bool b1 = SweepBoxTriangle(CurrentTri, boxBounds, boxVertices,
	#ifdef PRECOMPUTE_FAT_BOX
			fatEdges,
	#endif
			dir, oneOverDir, Hit, Normal, dd);
#endif
		if(b1)
		{
			if(dd <= localMinDist)
			{
				localMinDist	= dd;
#ifdef USE_SAT_VERSION_CCT
				savedTri		= CurrentTri;
#else
				localNormal		= Normal;
				localHit		= Hit;
#endif
				localIndex		= i;
				Status			= true;
			}
		}
	}

	if(Status)
	{
#ifdef USE_SAT_VERSION_CCT
		float dd = localMinDist*2.0f;
		bool b = SweepBoxTriangle(savedTri, boxBounds, boxVertices,
	#ifdef PRECOMPUTE_FAT_BOX
			fatEdges,
	#endif
			dir, oneOverDir, localHit, localNormal, dd);
		PX_UNUSED(b);
		if(!b)
		{
			Gu::Box box;	buildFrom1(box, boxPose.p, boxGeom.halfExtents, boxPose.q);
			PxSweepHit sweepHit;
			sweepHit.distance = localMinDist;
			runBackupProcedure(sweepHit, dir, box, savedTri, boxVertices);
			localNormal = sweepHit.normal;
			localHit = sweepHit.position;
		}
#endif

#ifdef LAZY_NORMALIZE
		localNormal.normalize();
		if((localNormal.dot(dir))>0.0f)
			localNormal = -localNormal;
#endif

		_d		= localMinDist;
		_index	= localIndex;
		_normal	= localNormal;
		_hit	= localHit;

		if (hasRotation)
		{
			_normal	= boxPose.rotate(_normal);
			_hit	= boxPose.rotate(_hit);
		}

		_hit += boxPose.p;
	}
	return Status;
	}
#endif
}

const Gu::SweepCapsuleFunc Gu::gSweepCCTCapsuleMap[7] = 
{
	sweepCapsule_SphereGeom,
	sweepCapsule_PlaneGeom,
	sweepCapsule_CapsuleGeom,
	sweepCCTCapsule_BoxGeom,
	sweepCapsule_ConvexGeom,
	sweepCapsule_MeshGeom,
	sweepCapsule_HeightFieldGeom
};

const Gu::SweepBoxFunc* Gu::GetSweepCCTBoxMap()
{
	return &Gu::gSweepCCTBoxMap[0];
}

const Gu::SweepBoxFunc Gu::gSweepCCTBoxMap[7] = 
{
	sweepCCTBox_SphereGeom,
	sweepBox_PlaneGeom,
	sweepCCTBox_CapsuleGeom,
	sweepCCTBox_BoxGeom,
	sweepBox_ConvexGeom,
	sweepCCTBox_MeshGeom,
	sweepCCTBox_HeightFieldGeom
};