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


#include "PsIntrinsics.h"
#include "PsAlloca.h"
#include "GuRaycastTests.h"

#include "GuGeomUtilsInternal.h"	// For getSegment
#include "PxBoxGeometry.h"
#include "PxCapsuleGeometry.h"
#include "PxSphereGeometry.h"
#include "PxPlaneGeometry.h"
#include "PxConvexMeshGeometry.h"
#include "PxTriangleMeshGeometry.h"
#include "GuIntersectionRayBox.h"
#include "GuIntersectionRayCapsule.h"
#include "GuIntersectionRaySphere.h"
#include "GuIntersectionRayPlane.h"
#include "GuHeightFieldUtil.h"
#include "GuDistancePointSegment.h"

#include "GuCapsule.h"
#include "GuConvexMesh.h"
#include "GuTriangleMesh.h"
#include "CmScaling.h"

#include "OPC_RayCollider.h"
#include "GuIceSupport.h"

#include "PxQueryReport.h"
#include "GuSPUHelpers.h"

using namespace physx;
using namespace Gu;

#ifdef __SPU__
extern CellHeightfieldTileCache g_sampleCache;
#endif

////////////////////////////////////////////////// raycasts //////////////////////////////////////////////////////////////////

PxU32 Gu::raycast_box(GU_RAY_FUNC_PARAMS)
{
	PX_UNUSED(userData);
	PX_UNUSED(hitCB);
	PX_UNUSED(anyHit);
	PX_UNUSED(maxHits);
	PX_ASSERT(geom.getType() == PxGeometryType::eBOX);
	PX_ASSERT(maxHits && hits);
	const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom);

	const PxTransform& absPose = pose;

	PxVec3 localOrigin = rayOrigin - absPose.p;
	localOrigin = absPose.q.rotateInv(localOrigin);

	PxVec3 localDir = absPose.q.rotateInv(rayDir);

	PxVec3 localImpact;
	PxReal t;
	PxVec3 dimensions = boxGeom.halfExtents;
	PxU32 rval = Gu::rayAABBIntersect2(-dimensions, dimensions, localOrigin, localDir, localImpact, t);
	if(!rval)
		return 0;

	hits->position = absPose.transform(localImpact);
	hits->distance = t; //worldRay.orig.distance(hit.worldImpact);	//should be the same, assuming ray dir was normalized!!
	if(t>maxDist)
		return 0;

	hits->faceIndex			= 0;
	hits->u					= 0.0f;
	hits->v					= 0.0f;
	hits->flags				= PxHitFlags(PxHitFlag::ePOSITION|PxHitFlag::eDISTANCE);
	hits->normal			= PxVec3(0.0f);

	// Compute additional information if needed
	if(hintFlags & (PxHitFlag::eNORMAL))
	{
		hits->flags |= PxHitFlag::eNORMAL;

		//Because rayAABBIntersect2 set t = 0 if start point inside shape
		if(t == 0)
			hits->normal = -rayDir;
		else
		{
			//local space normal is:
			rval--;
			PxVec3 n(0,0,0);
			n[rval] = PxReal((localImpact[rval] > 0) ? 1 : -1);
			hits->normal = absPose.q.rotate(n);
		}
	}

	return 1;
}

PxU32 Gu::raycast_sphere(GU_RAY_FUNC_PARAMS)
{
	PX_UNUSED(userData);
	PX_UNUSED(hitCB);
	PX_UNUSED(anyHit);
	PX_UNUSED(maxHits);
	PX_ASSERT(geom.getType() == PxGeometryType::eSPHERE);
	PX_ASSERT(maxHits && hits);

	const PxSphereGeometry& sphereGeom = static_cast<const PxSphereGeometry&>(geom);

	if(!Gu::intersectRaySphere(rayOrigin, rayDir, maxDist, pose.p, sphereGeom.radius, hits->distance, &hits->position))
		return 0;

	/*	// PT: should be useless now
	hit.distance	= worldRay.orig.distance(hit.worldImpact);
	if(hit.distance>maxDist)
	return false;
	*/
	hits->faceIndex			= 0;
	hits->u					= 0.0f;
	hits->v					= 0.0f;
	hits->flags				= PxHitFlags(PxHitFlag::ePOSITION|PxHitFlag::eDISTANCE);
	hits->normal			= PxVec3(0.0f);

	// Compute additional information if needed
	if(hintFlags & (PxHitFlag::eNORMAL))
	{
		// User requested impact normal
		//Because intersectRaySphere set distance = 0 if start point inside shape
		if(hits->distance == 0.0f)
			hits->normal = -rayDir;
		else
		{
			hits->normal = hits->position - pose.p;
			hits->normal.normalize();
		}
		hits->flags |= PxHitFlag::eNORMAL;
	}

	return 1;
}

//PxU32 Gu::raycast_sphere(
//	const PxGeometry& geom, const PxTransform& pose, const PxVec3& rayOrigin, const PxVec3& rayDir,
//	PxReal maxDist, PxHitFlags hintFlags, PxU32 maxHits, PxRaycastHit* PX_RESTRICT hits, bool anyHit)
//{
//	PX_ASSERT(geom.getType() == PxGeometryType::eSPHERE);
//	PX_ASSERT(maxHits && hits);
//
//	using namespace Ps::aos;
//
//	const PxSphereGeometry& sphereGeom = static_cast<const PxSphereGeometry&>(geom);
//
//	const Vec3V rayOriginV = Vec3V_From_PxVec3(rayOrigin);
//	const Vec3V rayDirV = Vec3V_From_PxVec3(rayDir);
//	const Vec3V center = Vec3V_From_PxVec3(pose.p);
//	const FloatV radius = FloatV_From_F32(sphereGeom.radius);
//	const FloatV maxDistV = FloatV_From_F32(maxDist);
//
//	FloatV dist;
//	Vec3V hitP;
//
//	if(!Gu::intersectRaySphere(rayOriginV, rayDirV, maxDistV, center, radius, dist, hitP))
//		return 0;
//	
//	hits->faceIndex			= 0;
//	hits->u					= 0.0f;
//	hits->v					= 0.0f;
//	hits->flags				= PxHitFlags(PxHitFlag::ePOSITION|PxHitFlag::eDISTANCE);
//
//	// Compute additional information if needed
//	if(hintFlags & (PxHitFlag::eNORMAL))
//	{
//		// User requested impact normal
//		const Vec3V v = V3Normalise(V3Sub(hitP, center));
//		PxVec3_From_Vec3V(v, hits->normal);
//		hits->flags |= PxHitFlag::eNORMAL;
//	}
//
//	return 1;
//}

PxU32 Gu::raycast_capsule(GU_RAY_FUNC_PARAMS)
{
	PX_UNUSED(userData);
	PX_UNUSED(hitCB);
	PX_UNUSED(anyHit);
	PX_UNUSED(maxHits);
	PX_ASSERT(geom.getType() == PxGeometryType::eCAPSULE);
	PX_ASSERT(maxHits && hits);

	const PxCapsuleGeometry& capsuleGeom = static_cast<const PxCapsuleGeometry&>(geom);

	// TODO: PT: could we simplify this ?
	Gu::Capsule capsule;
	getCapsuleSegment(pose, capsuleGeom, capsule);
	capsule.radius = capsuleGeom.radius;

	PxReal s[2];
	PxU32 numISec = Gu::intersectRayCapsule(rayOrigin, rayDir, capsule, s);
	if(!numISec)
		return 0;

	PxReal t;
	if(numISec == 1)
		t = s[0];
	else
	{
		// PT: attempt at fixing TTP 3690. I'm in Sweden and don't have my test cases for the ray-capsule
		// code here, so this fix is a bit uncertain.
		const PxReal epsilon = 1e-6f;
		if(s[0]<-epsilon && s[1]<-epsilon)
			return 0;

		t = (s[0] < s[1]) ? s[0]:s[1];

		if(t<0.0f)
		{
			t=0.0f;
		}
	}

	//	if(t<0.0f || t>maxDist)
	if(t>maxDist)	// PT: this was commented out. Who did it? Why?
		return 0;

	hits->position			= rayOrigin + rayDir*t;
	hits->distance			= t;
	hits->faceIndex			= 0;
	hits->u					= 0.0f;
	hits->v					= 0.0f;
	hits->flags				= PxHitFlags(PxHitFlag::ePOSITION|PxHitFlag::eDISTANCE);
	hits->normal			= PxVec3(0.0f);

	// Compute additional information if needed
	if(hintFlags & (PxHitFlag::eNORMAL))
	{
		// User requested impact normal
		// Regardless of user request we have delivered both
		hits->flags |= (PxHitFlag::eNORMAL);

		if(t==0.0f)
			hits->normal = -rayDir;
		else
		{
			PxReal capsuleT;
			Gu::distancePointSegmentSquared(capsule, hits->position, &capsuleT);
			capsule.computePoint(hits->normal, capsuleT);
			hits->normal = hits->position - hits->normal;	 //this should never be zero. It should have a magnitude of the capsule radius.
			hits->normal.normalize();
		}
	}
	
	return 1;
}


//PxU32 Gu::raycast_capsule(
//	const PxGeometry& geom, const PxTransform& pose, const PxVec3& rayOrigin, const PxVec3& rayDir, PxReal maxDist,
//	PxHitFlags hintFlags, PxU32 maxHits, PxRaycastHit* PX_RESTRICT hits, bool anyHit)
//{
//	PX_ASSERT(geom.getType() == PxGeometryType::eCAPSULE);
//	PX_ASSERT(maxHits && hits);
//
//	using namespace Ps::aos;
//	const FloatV zero = FZero();
//
//	const PxCapsuleGeometry& capsuleGeom = static_cast<const PxCapsuleGeometry&>(geom);
//
//	Gu::CapsuleV capsuleV;
//	getSegment(capsuleV, capsuleGeom, pose);
//	capsuleV.radius = FloatV_From_F32(capsuleGeom.radius);
//
//	const Vec3V rayOriginV = Vec3V_From_PxVec3(rayOrigin);
//	const Vec3V rayDirV = Vec3V_From_PxVec3(rayDir);
//	const FloatV maxDistV = FloatV_From_F32(maxDist);
//
//	FloatV t;
//	bool intersect = Gu::intersectRayCapsule(rayOriginV, rayDirV, capsuleV, t);
//
//	if(!intersect)
//		return 0;
//
//	PX_ASSERT(FAllGrtrOrEq(t, zero));
//
//	if(FAllGrtr(maxDistV, t))
//		return 0;
//
//	const Vec3V impact		= V3MulAdd(rayDirV, t, rayOriginV);
//	PxVec3_From_Vec3V(impact, hits->position);
//	hits->distance			= PxF32_From_FloatV(t);
//	hits->faceIndex			= 0;
//	hits->u					= 0.0f;
//	hits->v					= 0.0f;
//	hits->flags				= PxHitFlags(PxHitFlag::ePOSITION|PxHitFlag::eDISTANCE);
//
//	// Compute additional information if needed
//	if(hintFlags & (PxHitFlag::eNORMAL))
//	{
//		// User requested impact normal
//		// Regardless of user request we have delivered both
//		hits->flags |= (PxHitFlag::eNORMAL);
//		
//		FloatV t0;
//		distancePointSegmentSquared(capsuleV.p0, capsuleV.p1, impact,t0);
//		const Vec3V point = capsuleV.computePoint(t0);
//		const Vec3V n = V3Normalise(V3Sub(impact, point));	 //this should never be zero. It should have a magnitude of the capsule radius.
//		PxVec3_From_Vec3V(n, hits->normal);
//	}
//	
//	return 1;
//}

PxU32 Gu::raycast_plane(GU_RAY_FUNC_PARAMS)
{
	PX_UNUSED(userData);
	PX_UNUSED(hitCB);
	PX_UNUSED(anyHit);
	PX_UNUSED(maxHits);
	PX_ASSERT(geom.getType() == PxGeometryType::ePLANE);
	PX_ASSERT(maxHits && hits);
	PX_UNUSED(geom);
//	const PxPlaneGeometry& planeGeom = static_cast<const PxPlaneGeometry&>(geom);

	// Perform backface culling so that we can pick objects beyond planes
	PxPlane plane = Gu::getPlane(pose);
	if(rayDir.dot(plane.n)>=0.0f)
		return false;

	PxReal distanceAlongLine;
	if(!Gu::intersectRayPlane(rayOrigin, rayDir, plane, distanceAlongLine, &hits->position))
		return 0;

	/*
	PxReal test = worldRay.orig.distance(hit.worldImpact);

	PxReal dd;
	PxVec3 pp;
	PxSegmentPlaneIntersect(worldRay.orig, worldRay.orig+worldRay.dir*1000.0f, plane, dd, pp);
	*/

	if(distanceAlongLine<0.0f)
		return 0;

	if(distanceAlongLine>maxDist)
		return 0;

	hits->distance			= distanceAlongLine;
	hits->faceIndex			= 0;
	hits->u					= 0.0f;
	hits->v					= 0.0f;
	hits->flags				= PxHitFlag::eDISTANCE|PxHitFlag::ePOSITION;
	hits->normal			= PxVec3(0.0f);

	// Compute additional information if needed
	if(hintFlags & (PxHitFlag::eNORMAL))
	{
		// User requested impact normal
		hits->normal = plane.n;
		// Regardless of user request we have delivered both
		hits->flags |= PxHitFlag::eNORMAL;
	}

	return 1;
}

//PxU32 Gu::raycast_plane(
//	const PxGeometry& geom, const PxTransform& pose, const PxVec3& rayOrigin, const PxVec3& rayDir, PxReal maxDist,
//	PxHitFlags hintFlags, PxU32 maxHits, PxRaycastHit* PX_RESTRICT hits, bool firstHit)
//{
//	PX_ASSERT(geom.getType() == PxGeometryType::ePLANE);
//	PX_ASSERT(maxHits && hits);
//
//	using namespace Ps::aos;
//	const FloatV zero = FZero();
//	const Vec3V rayOriginV = Vec3V_From_PxVec3(rayOrigin);
//	const Vec3V rayDirV = Vec3V_From_PxVec3(rayDir);
//	const FloatV maxDistV = FloatV_From_F32(maxDist);
//
//	Gu::PlaneV planeV = Gu::getPlaneV(pose);
//	const Vec3V normal = Vec3V_From_Vec4V(planeV.nd);
//	// Perform backface culling so that we can pick objects beyond planes
//	if(FAllGrtrOrEq(V3Dot(rayDirV, normal), zero))
//		return false;
//	
//	FloatV dist;
//	Vec3V ip;
//	const BoolV b0 = Gu::intersectRayPlane(rayOriginV, rayDirV, planeV, dist, ip);
//	const BoolV b1 = FIsGrtr(zero, dist);
//	const BoolV b2 = FIsGrtr(dist, maxDistV);
//
//	const BoolV con = BOr(BNot(b0), BOr(b1, b2));
//
//	if(BAllEq(con, BTTTT()))
//		return 0;
//
//	hits->distance			=PxF32_From_FloatV( dist);
//	hits->faceIndex			= 0;
//	hits->u					= 0.0f;
//	hits->v					= 0.0f;
//	hits->flags				= PxHitFlag::eDISTANCE|PxHitFlag::ePOSITION;
//
//	// Compute additional information if needed
//	if(hintFlags & (PxHitFlag::eNORMAL))
//	{
//		// User requested impact normal
//		PxVec3_From_Vec3V(normal, hits->normal);
//		// Regardless of user request we have delivered both
//		hits->flags |= PxHitFlag::eNORMAL;
//	}
//
//	return 1;
//}

PxU32 Gu::raycast_convexMesh(GU_RAY_FUNC_PARAMS)
{ 
	PX_UNUSED(userData);
	PX_UNUSED(hitCB);
	PX_UNUSED(anyHit);
	PX_UNUSED(maxHits);
	PX_ASSERT(geom.getType() == PxGeometryType::eCONVEXMESH);
	PX_ASSERT(maxHits && hits);
	PX_ASSERT(PxAbs(rayDir.magnitudeSquared()-1)<1e-4);

	const PxConvexMeshGeometry& convexGeom = static_cast<const PxConvexMeshGeometry&>(geom);

	//PxMat34Legacy absPose(pose);

	PX_ALIGN_PREFIX(16)  PxU8 convexMeshBuffer[sizeof(Gu::ConvexMesh)+32] PX_ALIGN_SUFFIX(16);
	Gu::ConvexMesh* convexMesh = Cm::memFetchAsync<Gu::ConvexMesh>(convexMeshBuffer, (uintptr_t)(convexGeom.convexMesh), sizeof(Gu::ConvexHullData),1);

	PxRaycastHit& hit = *hits;
	
	//scaling: transform the ray to vertex space
	Cm::Matrix34 world2vertexSkew = convexGeom.scale.getInverse() * pose.getInverse();	

	Cm::memFetchWait(1); // convexMesh	

	//Gu::ConvexMesh* cmesh = static_cast<Gu::ConvexMesh*>(convexGeom.convexMesh);
	PxU32 nPolys = convexMesh->getNbPolygonsFast();
	const Gu::HullPolygonData* PX_RESTRICT polysEA = convexMesh->getPolygons();

#ifdef __SPU__
	const PxU32 polysSize = sizeof(Gu::HullPolygonData)*nPolys;


	PX_COMPILE_TIME_ASSERT(&((Gu::ConvexMesh*)NULL)->getHull()==NULL);

	 //The number of polygons is limited to 256.
	PX_ALIGN_PREFIX(16)  PxU8 hullBuffer[sizeof(Gu::HullPolygonData)*256+32] PX_ALIGN_SUFFIX(16);
	Gu::HullPolygonData* polys = Cm::memFetchAsync<Gu::HullPolygonData>(hullBuffer, (uintptr_t)(polysEA), polysSize, 1);
#else
	const Gu::HullPolygonData* polys = polysEA;
#endif

	PxVec3 vrayOrig = world2vertexSkew.transform( rayOrigin );
	PxVec3 vrayDir = world2vertexSkew.rotate( rayDir );	

	Cm::memFetchWait(1);// polys

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
	bool originInsideAllPlanes = true;
	PxReal latestEntry = -FLT_MAX;
	PxReal earlyestExit = FLT_MAX;
	const PxPlane* bestVertSpacePlane = NULL;
	PxU32 bestPolygonIndex = 0;
	PxU32 currentIndex = 0;

	while (nPolys--)
	{
		const Gu::HullPolygonData& poly = *(polys++);
		const PxPlane& vertSpacePlane = poly.mPlane;

		const PxReal distToPlane = vertSpacePlane.distance(vrayOrig);
		const PxReal dn = vertSpacePlane.n.dot(vrayDir);
		const PxReal distAlongRay = -distToPlane/dn;

		if(distToPlane > 0)	
			originInsideAllPlanes = false;	//origin not behind plane == ray starts outside the convex.

		if (dn > 1E-7f)	//the ray direction "exits" from the back side
		{
			earlyestExit = physx::intrinsics::selectMin(earlyestExit, distAlongRay);
		}
		else if (dn < -1E-7f)	//the ray direction "enters" from the front side
		{
			if (distAlongRay > latestEntry)
			{
				latestEntry = distAlongRay;
				bestVertSpacePlane = &vertSpacePlane;
				bestPolygonIndex = currentIndex;
			}
		}
		else
		{
			//plane normal and ray dir are orthogonal
			if(distToPlane > 0)	
				return 0;	//a plane is parallel with ray -- and we're outside the ray -- we definitely miss the entire convex!
		}

		currentIndex++;
	}

	if (originInsideAllPlanes)	//ray starts inside convex -- we don't count that as a hit.
	{
		hit.position			= rayOrigin;
		hit.distance		= 0.0f;
		hit.faceIndex		= 0;	//TODO: API needs to change!  So far here we were providing trig indices!!
		hit.u				= 0.0f;	//TODO: no more trigs!
		hit.v				= 0.0f;
		hit.flags			= PxHitFlag::eDISTANCE|PxHitFlag::ePOSITION;
		hit.normal			= PxVec3(0.0f);

		// Compute additional information if needed

		if(hintFlags & (PxHitFlag::eNORMAL))
		{
			hit.normal = -rayDir;
			hit.flags |= PxHitFlag::eNORMAL;
		}

		return 1;
	}

	// AP: changed to latestEntry < maxDist-1e-5f so that we have a conservatively negative result near end of ray
	if (latestEntry < earlyestExit && latestEntry > 0.0f && latestEntry < maxDist-1e-5f)
	{
		const PxVec3& pointOnPlane = vrayOrig + latestEntry * vrayDir;
		hit.position			= pose.transform(convexGeom.scale.toMat33() * pointOnPlane);
		hit.distance		= latestEntry;
		hit.faceIndex		= bestPolygonIndex;	// we do now return the polygon index
		hit.u				= 0.0f;	//TODO: no more trigs!
		hit.v				= 0.0f;
		hit.flags			= PxHitFlag::eDISTANCE|PxHitFlag::ePOSITION;
		hit.normal			= PxVec3(0.0f);

		// Compute additional information if needed

		if(hintFlags & (PxHitFlag::eNORMAL))
		{
			//when we have nonuniform scaling we actually have to transform by the transpose of the inverse of vertex2worldSkew.M == transpose of world2vertexSkew:
			hit.normal = world2vertexSkew.rotateTranspose(bestVertSpacePlane->n);
			hit.normal.normalize();
			hit.flags |= PxHitFlag::eNORMAL;
		}

		return 1;
	}

#if 0
	//this version does face tests and when the ray hits it may finish without touching all the faces.
	while (nPolys--)
	{
		const Gu::HullPolygonData&	poly = *(polys++);
		const PxPlane& vertSpacePlane = ((const PxPlane&)poly.mPlane[0]);
		//this is based on Gu::rayPlaneIntersect, but with early outs in case it's stabbing from behind.
		PxReal distToPlane = vertSpacePlane.distance(vrayOrig);
		if(distToPlane < 0)	continue;	//origin not in front of the plane
		PxReal dn = vertSpacePlane.normal|vrayDir;
		if (dn > -1E-7f) continue;	//we're shooting away from the plane
		PxReal distAlongRay  = -distToPlane/dn;
		PxVec3& pointOnPlane = vrayOrig + distAlongRay * vrayDir;

		//check if this point is contained in the polygon:
		const PxU8 * vRefBase = hullData.mVertexData8;

		if (pointInConvexPolygon(vertSpacePlane.normal, hullVertices, hullData.mVertexData8 + poly.mVRef8, poly.mNbVerts, pointOnPlane))
		{
			hit.worldImpact =	pose.transform(convexGeom.scale.toMat33() * pointOnPlane);
			hit.distance		= distAlongRay;
			//hit.faceID			= 0;	// Always 0 for a single plane
			hit.internalFaceID	= 0;	//TODO: API needs to change!  So far here we were providing trig indices!!
			hit.u				= 0.0f;	//TODO: no more trigs!
			hit.v				= 0.0f;
			hit.flags			|= PxHitFlag::eDISTANCE|PxHitFlag::ePOSITION;

			// Compute additional information if needed
			if(hintFlags & (PxHitFlag::eNORMAL))
			{
				//when we have nonuniform scaling we actually have to transform by the transpose of the inverse of vertex2worldSkew.M == transpose of world2vertexSkew:
				world2vertexSkew.M.multiplyByTranspose(vertSpacePlane.normal, hit.worldNormal);
				hit.worldNormal.normalize();
				hit.flags |= PxHitFlag::eNORMAL;
			}
			return true;
		}
	}
#endif

	return 0;
}

struct RayMeshColliderCallback  : public MeshHitCallback<PxRaycastHit>
{
	PxRaycastHit*		mDstBase;
	PxU32				mHitNum;
	PxU32				mMaxHits;
	const PxMeshScale*	mScale;
	const PxTransform*	mPose;
	const Cm::Matrix34*	mWorld2vertexSkew;
	PxU32				mHintFlags;
	Gu::RaycastHitFunc	mHitCB;
	void*				mUserData;
	const PxVec3&		mRayDir;
	bool				mIsDoubleSided;
	bool				mOverflow;

	RayMeshColliderCallback(
		CallbackMode::Enum mode, PxRaycastHit* hits, PxU32 maxHits, const PxMeshScale* scale, const PxTransform* pose,
		const Cm::Matrix34* world2vertexSkew, PxU32 hintFlags, Gu::RaycastHitFunc hitCB, void* userData, const PxVec3& rayDir, bool isDoubleSided) :
			MeshHitCallback<PxRaycastHit>	(mode),
			mDstBase						(hits),
			mHitNum							(0),
			mMaxHits						(maxHits),
			mScale							(scale),
			mPose							(pose),
			mWorld2vertexSkew				(world2vertexSkew),
			mHintFlags						(hintFlags),
			mHitCB							(hitCB),
			mUserData						(userData),
			mRayDir							(rayDir),
			mIsDoubleSided					(isDoubleSided),
			mOverflow						(false)
	{
	}

	// return false for early out
	virtual bool processHit(
		const PxRaycastHit& lHit, const PxVec3& lp0, const PxVec3& lp1, const PxVec3& lp2,
		bool indices16, void* indices, PxReal&)
	{
		PX_UNUSED(indices);
		PX_UNUSED(indices16);

		const PxReal u = lHit.u, v = lHit.v;
		const PxVec3 localImpact = (1.0f - u - v)*lp0 + u*lp1 + v*lp2;

		//not worth concatenating to do 1 transform: PxMat34Legacy vertex2worldSkew = scaling.getVertex2WorldSkew(absPose);
		// PT: TODO: revisit this for N hits
		PxRaycastHit hit = lHit;
		hit.position	= mPose->transform(mScale->transform(localImpact));
		hit.flags		= PxHitFlag::ePOSITION|PxHitFlag::eDISTANCE|PxHitFlag::eUV;
		hit.normal		= PxVec3(0.0f);

		// Compute additional information if needed
		if(mHintFlags & PxHitFlag::eNORMAL)
		{
			// User requested impact normal
			const PxVec3 localNormal = (lp1 - lp0).cross(lp2 - lp0);

			if(mWorld2vertexSkew)
				hit.normal = mWorld2vertexSkew->rotateTranspose(localNormal);				
			else
				hit.normal = hit.normal = mPose->rotate(localNormal);
			hit.normal.normalize();

			// PT: figure out correct normal orientation (DE7458)
			// - if the mesh is single-sided the normal should be the regular triangle normal N, regardless of eMESH_BOTH_SIDES.
			// - if the mesh is double-sided the correct normal can be either N or -N. We take the one opposed to ray direction.
			if(mIsDoubleSided && hit.normal.dot(mRayDir) > 0.0f)
				hit.normal = -hit.normal;

			hit.flags |= PxHitFlag::eNORMAL;
		}

		// PT: if a callback is available, use it
		if(mHitCB)
			return (mHitCB)(hit, mUserData);

		// PT: no callback => store results in provided buffer
		if(mHitNum == mMaxHits)
		{
			mOverflow = true;
			return false;
		}

		// AP: the loop below merges hits at the same distance within some arbitrary tolerance
		// PT: TODO: remove that O(n^2) check (DE6085)
		// PT: the check is done independently in the callback when it's used, since we don't store hits
		// locally anymore in that case.
		// distance check
		for(PxU32 i=0; i<mHitNum; i++)
		{
			if(fabsf(mDstBase[i].distance - hit.distance) < 1e-4)
				return true;
		}

		mDstBase[mHitNum++] = hit;
		return true;
	}

private:
	RayMeshColliderCallback& operator=(const RayMeshColliderCallback&);
};

PxU32 Gu::raycast_triangleMesh(GU_RAY_FUNC_PARAMS) 
{
	PX_ASSERT(geom.getType() == PxGeometryType::eTRIANGLEMESH);
	PX_ASSERT((maxHits && hits) || hitCB);
	PX_ASSERT(PxAbs(rayDir.magnitudeSquared()-1)<1e-4);

	PxRaycastHit* PX_RESTRICT dst = hits;
	const PxTriangleMeshGeometry& meshGeom = static_cast<const PxTriangleMeshGeometry&>(geom);

	GU_FETCH_MESH_DATA(meshGeom)

	//scaling: transform the ray to vertex space

	PxVec3 orig, dir;
	Cm::Matrix34 world2vertexSkew;
	Cm::Matrix34* world2vertexSkewP = NULL;
	if(meshGeom.scale.isIdentity())
	{
		orig = pose.transformInv(rayOrigin);
		dir = pose.rotateInv(rayDir);
	}
	else
	{
		world2vertexSkew = meshGeom.scale.getInverse() * pose.getInverse();
		world2vertexSkewP = &world2vertexSkew;
		orig = world2vertexSkew.transform(rayOrigin);
		dir = world2vertexSkew.rotate(rayDir);
	}

	const bool isDoubleSided = meshGeom.meshFlags.isSet(PxMeshGeometryFlag::eDOUBLE_SIDED);
	RayMeshColliderCallback callback(
		(hitCB || maxHits > 1) ? CallbackMode::eMULTIPLE : (anyHit ? CallbackMode::eANY : CallbackMode::eCLOSEST),
		dst, maxHits, &meshGeom.scale, &pose, world2vertexSkewP, hintFlags, hitCB, userData, rayDir, isDoubleSided);

	const bool bothSides = isDoubleSided || (hintFlags & PxHitFlag::eMESH_BOTH_SIDES);

	RTreeMidphaseData hmd;	// PT: I suppose doing the "conversion" at runtime is fine
	meshData->mOpcodeModel.getRTreeMidphaseData(hmd);
	MeshRayCollider<0>::Collide(orig, dir, maxDist, bothSides, hmd, callback, meshData->mOpcodeModel.mGeomEpsilon);

	return callback.mHitNum;
}

namespace physx
{
namespace Gu
{

	class RayCastCallback 
	{
	public:
		PxVec3 hitPoint;
		PxU32 hitTriangle;
		bool hit;

		RayCastCallback() : hit(false){}

		PX_INLINE bool underFaceHit(
			const Gu::HeightFieldUtil&, const PxVec3&, const PxVec3&,
			PxF32, PxF32, PxF32, PxU32)
		{ return true; } // true means continue traversal

		bool faceHit(const Gu::HeightFieldUtil&, const PxVec3& aHitPoint, PxU32 aTriangleIndex)
		{
			hitPoint = aHitPoint;
			hitTriangle = aTriangleIndex;
			hit = true;
			return false; // first hit, false = stop traversal
		}
	};   
} // namespace
}

PxU32 Gu::raycast_heightField(GU_RAY_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eHEIGHTFIELD);
	PX_ASSERT(maxHits && hits);
	PX_UNUSED(userData);
	PX_UNUSED(hitCB);
	PX_UNUSED(anyHit);
	PX_UNUSED(maxHits);

	struct LSEASwitcher
	{
		LSEASwitcher(PxHeightFieldGeometry& geo, PxHeightField* LS)
		{
			pGeo = &geo;
			EA = geo.heightField;
			geo.heightField = LS;
		}

		~LSEASwitcher()
		{
			pGeo->heightField = EA;
		}

		PxHeightFieldGeometry* pGeo;
		PxHeightField* EA;
	};
	const PxHeightFieldGeometry& hfGeom = static_cast<const PxHeightFieldGeometry&>(geom);

	PX_ALIGN_PREFIX(16)  PxU8 heightFieldBuffer[sizeof(Gu::HeightField)+32] PX_ALIGN_SUFFIX(16);
	Gu::HeightField* heightField = Cm::memFetchAsync<Gu::HeightField>(heightFieldBuffer, (uintptr_t)(hfGeom.heightField), sizeof(Gu::HeightField), 1);
		
	hits->flags = PxHitFlags(0);

	PxTransform invAbsPose = pose.getInverse();
	PxVec3 localRayOrig = invAbsPose.transform(rayOrigin);
	PxVec3 localRayDir = invAbsPose.rotate(rayDir);
	
	Cm::memFetchWait(1);

#ifdef __SPU__
	g_sampleCache.init((uintptr_t)(heightField->getData().samples), heightField->getData().tilesU);
#endif

	//hfGeom.heightField = heightField;
	LSEASwitcher switcher(const_cast<PxHeightFieldGeometry&>(hfGeom),heightField);

	Gu::HeightFieldUtil hfUtil(hfGeom);
	RayCastCallback callback;

	PxVec3 normRayDir = localRayDir;
	normRayDir.normalizeSafe(); // nothing will happen if length is < PX_NORMALIZATION_EPSILON
	hfUtil.traceSegment<RayCastCallback, false, false>(
		localRayOrig, localRayOrig + normRayDir * PxMin(maxDist, 1e10f), &callback);

	if (callback.hit)
	{
		hits->faceIndex = callback.hitTriangle;

		//We need the normal for the dot product.
		PxVec3 normal = pose.q.rotate(hfUtil.getNormalAtShapePoint(callback.hitPoint.x, callback.hitPoint.z)); 
		normal.normalize();

		if (hintFlags & PxHitFlag::eNORMAL)
		{
			hits->normal = normal; 
			hits->flags |= PxHitFlag::eNORMAL;
		}
		if (hintFlags & PxHitFlag::eDISTANCE)
		{
			hits->distance = (callback.hitPoint - localRayOrig).dot(localRayDir);
			hits->flags |= PxHitFlag::eDISTANCE;
		}
		if (hintFlags & PxHitFlag::ePOSITION)
		{
			hits->position = pose.transform(callback.hitPoint);
			hits->flags |= PxHitFlag::ePOSITION;
		}
		
		return 1;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Gu::RaycastFunc Gu::GetRaycastFunc(PxU16 type)
{
	return Gu::gRaycastMap[type];
}

const Gu::RaycastFunc Gu::gRaycastMap[7] =
{
	Gu::raycast_sphere,
	Gu::raycast_plane,
	Gu::raycast_capsule,
	Gu::raycast_box,
	Gu::raycast_convexMesh,
	Gu::raycast_triangleMesh,
	Gu::raycast_heightField
};
