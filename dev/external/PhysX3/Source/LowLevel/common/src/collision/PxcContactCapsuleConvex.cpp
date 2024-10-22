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

#include "GuConvexMesh.h"
#include "GuConvexHelper.h"
#include "GuContactBuffer.h"
#include "PxcContactMethodImpl.h"
#include "PxcNpCache.h"
#include "GuGJKWrapper.h"
#include "GuVecConvexHull.h"
#include "GuVecCapsule.h"
#include "GuGeomUtilsInternal.h"

using namespace physx;
using namespace Gu;

///////////
//	#include "CmRenderOutput.h"
//	#include "PxsContext.h"
//	static void gVisualizeLine(const PxVec3& a, const PxVec3& b, PxcNpThreadContext& context, PxU32 color=0xffffff)
//	{
//		PxMat44 m = PxMat44::identity();
//
//		Cm::RenderOutput& out = context.mRenderOutput;
//		out << color << m << Cm::RenderOutput::LINES << a << b;
//	}
///////////

static const PxReal fatConvexEdgeCoeff = 0.01f;

static bool intersectEdgeEdgePreca(const PxVec3& p1, const PxVec3& p2, const PxVec3& v1, const PxPlane& plane, PxU32 i, PxU32 j, float coeff, const PxVec3& dir, const PxVec3& p3, const PxVec3& p4, PxReal& dist, PxVec3& ip, float limit)
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
	PxReal temp = d3 * plane.distance(p4);
	if(temp>0.0f)
		return false;

	// if colliding edge (p3,p4) and plane are parallel return no collision
	PxVec3 v2 = p4 - p3;

	temp = plane.n.dot(v2);
	if(temp==0.0f)
		return false;	// ### epsilon would be better

	// compute intersection point of plane and colliding edge (p3,p4)
	ip = p3-v2*(d3/temp);

	// compute distance of intersection from line (ip, -dir) to line (p1,p2)
	dist =	(v1[i]*(ip[j]-p1[j])-v1[j]*(ip[i]-p1[i]))*coeff;
	if(dist<limit)
		return false;

	// compute intersection point on edge (p1,p2) line
	ip -= dist*dir;

	// check if intersection point (ip) is between edge (p1,p2) vertices
	temp = (p1.x-ip.x)*(p2.x-ip.x)+(p1.y-ip.y)*(p2.y-ip.y)+(p1.z-ip.z)*(p2.z-ip.z);
	if(temp<0.0f)
		return true;	// collision found

	return false;	// no collision
}

static bool PxcTestAxis(const PxVec3& axis, const Gu::Segment& segment, PxReal radius,
						const PolygonalData& polyData, const Cm::FastVertex2ShapeScaling& scaling,
						const Cm::Matrix34& worldTM,
						PxReal& depth)
{
	// Project capsule
	PxReal min0 = segment.p0.dot(axis);
	PxReal max0 = segment.p1.dot(axis);
	if(min0>max0) Ps::swap(min0, max0);
	min0 -= radius;
	max0 += radius;

	// Project convex
	PxReal Min1, Max1;
	(polyData.mProjectHull)(polyData, axis, worldTM, scaling, Min1, Max1);

	// Test projections
	if(max0<Min1 || Max1<min0)
		return false;

	const PxReal d0 = max0 - Min1;
	PX_ASSERT(d0>=0.0f);
	const PxReal d1 = Max1 - min0;
	PX_ASSERT(d1>=0.0f);
	depth = physx::intrinsics::selectMin(d0, d1);
	return true;
}

static bool PxcCapsuleConvexOverlap(const Gu::Segment& segment, PxReal radius,
									const PolygonalData& polyData,
									const Cm::FastVertex2ShapeScaling& scaling,
									const PxTransform& transform,
									PxReal* t, PxVec3* pp, bool isSphere)
{
	// TODO:
	// - test normal & edge in same loop
	// - local space
	// - use precomputed face value
	// - optimize projection

	PxVec3 Sep(0,0,0);
	PxReal PenDepth = PX_MAX_REAL;

	PxU32 nbPolys = polyData.mNbPolygons;
	const Gu::HullPolygonData* polys = polyData.mPolygons;

	const Cm::Matrix34 worldTM(transform);

	// Test normals
	for(PxU32 i=0;i<nbPolys;i++)
	{
		const Gu::HullPolygonData& poly = polys[i];
		const PxPlane& vertSpacePlane = poly.mPlane;

		const PxVec3 worldNormal = worldTM.rotate(vertSpacePlane.n);

		PxReal d;
		if(!PxcTestAxis(worldNormal, segment, radius, polyData, scaling, worldTM, d))
			return false;

		if(d<PenDepth)
		{
			PenDepth = d;
			Sep = worldNormal;
		}
	}

	// Test edges
	if(!isSphere)
	{
		PxVec3 CapsuleAxis(segment.p1 - segment.p0);
		CapsuleAxis = CapsuleAxis.getNormalized();
		for(PxU32 i=0;i<nbPolys;i++)
		{
			const Gu::HullPolygonData& poly = polys[i];
			const PxPlane& vertSpacePlane = poly.mPlane;

			const PxVec3 worldNormal = worldTM.rotate(vertSpacePlane.n);

			PxVec3 Cross = CapsuleAxis.cross(worldNormal);
			if(!Ps::isAlmostZero(Cross))
			{
				Cross = Cross.getNormalized();
				PxReal d;
				if(!PxcTestAxis(Cross, segment, radius, polyData, scaling, worldTM, d))
					return false;

				if(d<PenDepth)
				{
					PenDepth = d;
					Sep = Cross;
				}
			}
		}
	}

	const PxVec3 Witness = segment.computeCenter() - transform.transform(polyData.mCenter);

	if(Sep.dot(Witness) < 0.0f)
		Sep = -Sep;

	if(t)	*t = PenDepth;
	if(pp)	*pp = Sep;

	return true;
}





static bool raycast_convexMesh2(	const PolygonalData& polyData,
									const PxVec3& vrayOrig, const PxVec3& vrayDir,
									PxReal maxDist, PxF32& t)
{ 
	PxU32 nPolys = polyData.mNbPolygons;
	const Gu::HullPolygonData* PX_RESTRICT polys = polyData.mPolygons;

	/*
	Purely convex planes based algorithm
	Iterate all planes of convex, with following rules:
	* determine of ray origin is inside them all or not.  
	* planes parallel to ray direction are immediate early out if we're on the outside side (plane normal is sep axis)
	* else 
		- for all planes the ray direction "enters" from the front side, track the one furthest along the ray direction (A)
		- for all planes the ray direction "exits" from the back side, track the one furthest along the negative ray direction (B)
	if the ray origin is outside the convex and if along the ray, A comes before B, the directed line stabs the convex at A
	*/
	PxReal latestEntry = -FLT_MAX;
	PxReal earlyestExit = FLT_MAX;

	while(nPolys--)
	{
		const Gu::HullPolygonData& poly = *polys++;
		const PxPlane& vertSpacePlane = poly.mPlane;

		const PxReal distToPlane = vertSpacePlane.distance(vrayOrig);
		const PxReal dn = vertSpacePlane.n.dot(vrayDir);
		const PxReal distAlongRay = -distToPlane/dn;

		if (dn > 1E-7f)	//the ray direction "exits" from the back side
		{
			earlyestExit = physx::intrinsics::selectMin(earlyestExit, distAlongRay);
		}
		else if (dn < -1E-7f)	//the ray direction "enters" from the front side
		{
/*			if (distAlongRay > latestEntry)
			{
				latestEntry = distAlongRay;
			}*/
			latestEntry = physx::intrinsics::selectMax(latestEntry, distAlongRay);
		}
		else
		{
			//plane normal and ray dir are orthogonal
			if(distToPlane > 0.0f)	
				return false;	//a plane is parallel with ray -- and we're outside the ray -- we definitely miss the entire convex!
		}
	}

	if(latestEntry < earlyestExit)
	{
		if(latestEntry < maxDist)
		{
			t = latestEntry;
			return true;
		}
	}
	return false;
}

// PT: version based on Gu::raycast_convexMesh to handle scaling, but modified to make sure it works when ray starts inside the convex
static void PxcGenerateVFContacts2(ContactBuffer& contactBuffer,
									//
									const PxTransform& convexPose,
									const PolygonalData& polyData,	// Convex data
									const PxMeshScale& scale,
									//
									PxU32 nbPts,
									const PxVec3* PX_RESTRICT points,
									const PxReal radius,		// Capsule's radius
									//
									const PxVec3& normal,
									const PxReal contactDistance)
{
	PX_ASSERT(PxAbs(normal.magnitudeSquared()-1)<1e-4);

	//scaling: transform the ray to vertex space
	const Cm::Matrix34 world2vertexSkew = scale.getInverse() * convexPose.getInverse();	

	const PxVec3 vrayDir = world2vertexSkew.rotate( -normal );	

	const PxReal maxDist = contactDistance + radius;

	for(PxU32 i=0;i<nbPts;i++)
	{
		const PxVec3& rayOrigin = points[i];

		const PxVec3 vrayOrig = world2vertexSkew.transform( rayOrigin );
		PxF32 t;
		if(raycast_convexMesh2(polyData, vrayOrig, vrayDir, maxDist, t))
		{
			contactBuffer.contact(rayOrigin - t * normal, normal, t - radius);
		}
	}
}

static void PxcGenerateEEContacts(	ContactBuffer& contactBuffer,
									//
									const Gu::Segment& segment,
									const PxReal radius,
									const PxReal contactDistance,
									//
									const PolygonalData& polyData,
									const PxTransform& transform,
									const Cm::FastVertex2ShapeScaling& scaling,
									//
									const PxVec3& normal)
{
	PxU32 numPolygons = polyData.mNbPolygons;
	const Gu::HullPolygonData* PX_RESTRICT polygons = polyData.mPolygons;
	const PxU8* PX_RESTRICT vertexData = polyData.mPolygonVertexRefs;

	ConvexEdge edges[512];
	PxU32 nbEdges = findUniqueConvexEdges(512, edges, numPolygons, polygons, vertexData);

	//
	PxVec3 s0 = segment.p0;
	PxVec3 s1 = segment.p1;
	Ps::makeFatEdge(s0, s1, fatConvexEdgeCoeff);

	// PT: precomputed part of edge-edge intersection test
//		const PxVec3 v1 = segment.p1 - segment.p0;
		const PxVec3 v1 = s1 - s0;
		PxPlane plane;
		plane.n = v1.cross(normal);
//		plane.d = -(plane.normal|segment.p0);
		plane.d = -(plane.n.dot(s0));

		PxU32 ii,jj;
		Ps::closestAxis(plane.n, ii, jj);

		const float coeff = 1.0f /(v1[ii]*normal[jj]-v1[jj]*normal[ii]);

	//

	const PxVec3* PX_RESTRICT verts = polyData.mVerts;
	for(PxU32 i=0;i<nbEdges;i++)
	{
		const PxU8 vi0 = edges[i].vref0;
		const PxU8 vi1 = edges[i].vref1;

//		PxVec3 p1 = transform.transform(verts[vi0]);
//		PxVec3 p2 = transform.transform(verts[vi1]);
//		Ps::makeFatEdge(p1, p2, fatConvexEdgeCoeff);	// PT: TODO: make fat segment instead
		const PxVec3 p1 = transform.transform(scaling * verts[vi0]);
		const PxVec3 p2 = transform.transform(scaling * verts[vi1]);

		PxReal dist;
		PxVec3 ip;
//		if(intersectEdgeEdgePreca(segment.p0, segment.p1, v1, plane, ii, jj, coeff, normal, p1, p2, dist, ip))
//		if(intersectEdgeEdgePreca(s0, s1, v1, plane, ii, jj, coeff, normal, p1, p2, dist, ip, -FLT_MAX))
		if(intersectEdgeEdgePreca(s0, s1, v1, plane, ii, jj, coeff, normal, p1, p2, dist, ip, -radius-contactDistance))
//		if(intersectEdgeEdgePreca(s0, s1, v1, plane, ii, jj, coeff, normal, p1, p2, dist, ip, 0))
		{
			contactBuffer.contact(ip-normal*dist, normal, - (radius + dist));
//			if(contactBuffer.count==2)	// PT: we only need 2 contacts to be stable
//				return;
		}
	}
}

static void PxcGenerateEEContacts2b(ContactBuffer& contactBuffer,
									//
									const Gu::Segment& segment, 
									const PxReal radius, 
									//
									const Cm::Matrix34& transform,
									const PolygonalData& polyData,
									const Cm::FastVertex2ShapeScaling& scaling,
									//
									const PxVec3& normal,
									const PxReal contactDistance)
{
	// TODO:
	// - local space

	const PxVec3 localDir = transform.rotateTranspose(normal);
	PxU32 polyIndex = (polyData.mSelectClosestEdgeCB)(polyData, scaling, localDir);

	PxVec3 s0 = segment.p0;
	PxVec3 s1 = segment.p1;
	Ps::makeFatEdge(s0, s1, fatConvexEdgeCoeff);

	// PT: precomputed part of edge-edge intersection test
//		const PxVec3 v1 = segment.p1 - segment.p0;
		const PxVec3 v1 = s1 - s0;
		PxPlane plane;
		plane.n = -(v1.cross(normal));
//		plane.d = -(plane.normal|segment.p0);
		plane.d = -(plane.n.dot(s0));

		PxU32 ii,jj;
		Ps::closestAxis(plane.n, ii, jj);

		const float coeff = 1.0f /(v1[jj]*normal[ii]-v1[ii]*normal[jj]);
	//

	const PxVec3* PX_RESTRICT verts = polyData.mVerts;

	const Gu::HullPolygonData& polygon = polyData.mPolygons[polyIndex];
	const PxU8* PX_RESTRICT vRefBase = polyData.mPolygonVertexRefs + polygon.mVRef8;
	PxU32 numEdges = polygon.mNbVerts;

	PxU32 a = numEdges - 1;
	PxU32 b = 0;
	while(numEdges--)
	{
//		const PxVec3 p1 = transform.transform(verts[vRefBase[a]]);
//		const PxVec3 p2 = transform.transform(verts[vRefBase[b]]);
		const PxVec3 p1 = transform.transform(scaling * verts[vRefBase[a]]);
		const PxVec3 p2 = transform.transform(scaling * verts[vRefBase[b]]);

		PxReal dist;
		PxVec3 ip;
//		bool contact = intersectEdgeEdgePreca(segment.p0, segment.p1, v1, plane, ii, jj, coeff, -normal, p1, p2, dist, ip);
		bool contact = intersectEdgeEdgePreca(s0, s1, v1, plane, ii, jj, coeff, -normal, p1, p2, dist, ip, 0.0f);
		if(contact && dist < radius + contactDistance)
		{
			contactBuffer.contact(ip-normal*dist, normal, dist - radius);
//			if(contactBuffer.count==2)	// PT: we only need 2 contacts to be stable
//				return;
		}

		a = b;
		b++;
	}
}


namespace physx
{
bool PxcContactCapsuleConvex(CONTACT_METHOD_ARGS)
{
	PX_UNUSED(npCache);

	// Get actual shape data
	const PxCapsuleGeometry& shapeCapsule = shape0.get<const PxCapsuleGeometry>();
	const PxConvexMeshGeometryLL& shapeConvex = shape1.get<const PxConvexMeshGeometryLL>();

	PxVec3 onSegment, onConvex;
	PxReal sqDistance;
	PxVec3 normal_;
	{
		Gu::ConvexMesh* cm = static_cast<Gu::ConvexMesh*>(shapeConvex.convexMesh);

		using namespace Ps::aos;
		Vec3V closA, closB, normalV;
		//bool intersect = false;
		PxGJKStatus status;
		FloatV sqDist;
		{
			/*const Gu::ConvexHullData* hullData = &cm->getHull();
			const QuatV q0 = QuatV_From_F32Array(&transform0.q.x);
			const Vec3V p0 = Vec3V_From_Vec4V(Vec4V_From_F32Array(&transform0.p.x));
			const QuatV q1 = QuatV_From_F32Array(&transform1.q.x);
			const Vec3V p1 = Vec3V_From_Vec4V(Vec4V_From_F32Array(&transform1.p.x));

			const FloatV capsuleHalfHeight = FloatV_From_F32(shapeCapsule.halfHeight);
			const FloatV capsuleRadius = FloatV_From_F32(shapeCapsule.radius);

			const Vec3V vScale = Vec3V_From_PxVec3(shapeConvex.scale.scale);
			const QuatV vQuat = QuatV_From_F32Array(&shapeConvex.scale.rotation.x);

			const Mat33V rot = QuatGetMat33V(q1);
			const Gu::ConvexHullV convexHull(hullData, p1, rot, vScale, vQuat);

			const Vec3V basisVector0 = QuatGetBasisVector0(q0);
			const Vec3V tmp0 = V3Scale(basisVector0,  capsuleHalfHeight);
			const Gu::CapsuleV capsule(p0, tmp0, capsuleRadius);
			status = Gu::GJK(capsule, convexHull, closA, closB, normalV, sqDist);*/
			const Vec3V zeroV = V3Zero();
			const Gu::ConvexHullData* hullData = &cm->getHull();

			const FloatV capsuleHalfHeight = FLoad(shapeCapsule.halfHeight);
			const FloatV capsuleRadius = FLoad(shapeCapsule.radius);

			const Vec3V vScale = V3LoadU(shapeConvex.scale.scale);
			const QuatV vQuat = QuatVLoadU(&shapeConvex.scale.rotation.x);

			const PsMatTransformV aToB(transform1.transformInv(transform0));

			const Gu::ConvexHullV convexHull(hullData, zeroV, vScale, vQuat);

			const Gu::CapsuleV capsule(zeroV, V3Scale(V3UnitX(), capsuleHalfHeight), capsuleRadius);

			status = Gu::GJKRelative(capsule, convexHull, aToB, closA, closB, normalV, sqDist);
		}

		if(status == GJK_CONTACT)
			sqDistance = 0.f;
		else
		{
			V3StoreU(closB, onConvex);
			FStore(sqDist, &sqDistance);
			V3StoreU(normalV, normal_);
			onConvex = transform1.transform(onConvex);
			normal_ = transform1.rotate(normal_);
		}
		
	}

	const PxReal inflatedRadius = shapeCapsule.radius + contactDistance;
	const PxReal sqInflatedRadius = inflatedRadius*inflatedRadius;
	if(sqDistance >= sqInflatedRadius)  
		return false;

	Gu::Segment worldSegment;
	getCapsuleSegment(transform0, shapeCapsule, worldSegment);

	const bool isSphere = worldSegment.p0 == worldSegment.p1;
	const PxU32 nbPts = isSphere ? 1 : 2;

	PX_ASSERT(contactBuffer.count==0);

	Cm::FastVertex2ShapeScaling convexScaling;
	const bool idtConvexScale = shapeConvex.scale.isIdentity();
	if(!idtConvexScale)
		convexScaling.init(shapeConvex.scale);

	PolygonalData polyData;
	getPolygonalData_Convex(&polyData, shapeConvex.hullData, convexScaling);

//	if(0)
	if(sqDistance > 0.f)
	{
		// PT: the capsule segment doesn't intersect the convex => distance-based version
		PxVec3 normal = -normal_;

		// PT: generate VF contacts for segment's vertices vs convex
		PxcGenerateVFContacts2(	contactBuffer,
								transform1, polyData, shapeConvex.scale,
								nbPts, &worldSegment.p0, shapeCapsule.radius,
								normal, contactDistance);

		// PT: early exit if we already have 2 stable contacts
		if(contactBuffer.count==2)
			return true;

		// PT: else generate slower EE contacts
		if(!isSphere)
		{
			const Cm::Matrix34 worldTM(transform1);
			PxcGenerateEEContacts2b(contactBuffer, worldSegment, shapeCapsule.radius,
									worldTM, polyData, convexScaling,
									normal, contactDistance);
		}

		// PT: run VF case for convex-vertex-vs-capsule only if we don't have any contact yet
		if(!contactBuffer.count)
		{
//			gVisualizeLine(onConvex, onConvex + normal, context, PxDebugColor::eARGB_RED);
			PxReal distance = PxSqrt(sqDistance);
			contactBuffer.contact(onConvex, normal, distance - shapeCapsule.radius);
		}
	}
	else
	{
		// PT: the capsule segment intersects the convex => penetration-based version
//printf("Penetration-based:\n");

		// PT: compute penetration vector (MTD)
		PxVec3 SepAxis;
		if(!PxcCapsuleConvexOverlap(worldSegment, shapeCapsule.radius, polyData, convexScaling, transform1, NULL, &SepAxis, isSphere))
		{
//printf("- no overlap\n");
			return false;
		}

		// PT: generate VF contacts for segment's vertices vs convex
		PxcGenerateVFContacts2(	contactBuffer,
								transform1, polyData, shapeConvex.scale,
								nbPts, &worldSegment.p0, shapeCapsule.radius,
								SepAxis, contactDistance);

		// PT: early exit if we already have 2 stable contacts
//printf("- %d VF contacts\n", contactBuffer.count);
		if(contactBuffer.count==2)
			return true;

		// PT: else generate slower EE contacts
		if(!isSphere)
		{
			PxcGenerateEEContacts(contactBuffer, worldSegment, shapeCapsule.radius, contactDistance, polyData, transform1, convexScaling, SepAxis);
//printf("- %d total contacts\n", contactBuffer.count);
		}
	}

	return true;
}

bool PxcContactSphereConvex(CONTACT_METHOD_ARGS)
{
	return PxcContactCapsuleConvex(shape0, shape1, transform0, transform1, contactDistance, npCache, contactBuffer);
}
}