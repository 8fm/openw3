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

#include "PxGeometryQuery.h"
#include "GuGeomUtilsInternal.h"
#include "GuOverlapTests.h"
#include "GuCCTSweepTests.h"
#include "GuSweepTests.h"
#include "GuRaycastTests.h"
#include "GuBoxConversion.h"
#include "GuInternalTriangleMesh.h"
#include "GuMTD.h"

using namespace physx;

bool PxGeometryQuery::sweep(const PxVec3& unitDir, const PxReal distance,
							const PxGeometry& geom0, const PxTransform& pose0,
							const PxGeometry& geom1, const PxTransform& pose1,
							PxSweepHit& sweepHit, PxHitFlags hintFlags,
							const PxReal inflation)
{
	PX_SIMD_GUARD;
	PX_CHECK_AND_RETURN_VAL(pose0.isValid(), "PxGeometryQuery::sweep(): pose0 is not valid.", false);
	PX_CHECK_AND_RETURN_VAL(pose1.isValid(), "PxGeometryQuery::sweep(): pose1 is not valid.", false);
	PX_CHECK_AND_RETURN_VAL(unitDir.isFinite(), "PxGeometryQuery::sweep(): unitDir is not valid.", false);
	PX_CHECK_AND_RETURN_VAL(PxIsFinite(distance), "PxGeometryQuery::sweep(): distance is not valid.", false);
	PX_CHECK_AND_RETURN_VAL(distance > 0 || !(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP),
		"PxGeometryQuery::sweep(): sweep motion length must be greater than 0.", false);

	switch(geom0.getType())
	{
		case PxGeometryType::eSPHERE:
		{
			const PxSphereGeometry& sphereGeom = static_cast<const PxSphereGeometry&>(geom0);

			const Gu::Capsule worldCapsule(Gu::Segment(pose0.p, pose0.p), sphereGeom.radius);

			//pxPrintf("sweep sphere vs %d\n", geom1.getType());
			Gu::SweepCapsuleFunc func = Gu::gSweepCapsuleMap[geom1.getType()];
			return func(geom1, pose1, worldCapsule, unitDir, distance, sweepHit, hintFlags, inflation);
		}

		case PxGeometryType::eCAPSULE:
		{
			const PxCapsuleGeometry& capsGeom = static_cast<const PxCapsuleGeometry&>(geom0);

			Gu::Capsule worldCapsule;
			Gu::getCapsule(worldCapsule, capsGeom, pose0);

			//pxPrintf("sweep capsule vs %d\n", geom1.getType());
			if(!PX_IS_SPU && (hintFlags & PxHitFlag::ePRECISE_SWEEP))
			{
				Gu::SweepCapsuleFunc func = Gu::gSweepCCTCapsuleMap[geom1.getType()];
				return func(geom1, pose1, worldCapsule, unitDir, distance, sweepHit, hintFlags, inflation);
			}
			else
			{
				Gu::SweepCapsuleFunc func = Gu::gSweepCapsuleMap[geom1.getType()];
				return func(geom1, pose1, worldCapsule, unitDir, distance, sweepHit, hintFlags, inflation);
			}
		}

		case PxGeometryType::eBOX:
		{
			const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom0);

			Gu::Box box;	buildFrom(box, pose0.p, boxGeom.halfExtents, pose0.q);

			//pxPrintf("sweep box vs %d\n", geom1.getType());
			if(!PX_IS_SPU && (hintFlags & PxHitFlag::ePRECISE_SWEEP))
			{
				Gu::SweepBoxFunc func = Gu::gSweepCCTBoxMap[geom1.getType()];
				return func(geom1, pose1, box, unitDir, distance, sweepHit, hintFlags, inflation);
			}
			else
			{
				Gu::SweepBoxFunc func = Gu::gSweepBoxMap[geom1.getType()];
				return func(geom1, pose1, box, unitDir, distance, sweepHit, hintFlags, inflation);
			}
		}

		case PxGeometryType::eCONVEXMESH:
		{
			const PxConvexMeshGeometry& convexGeom = static_cast<const PxConvexMeshGeometry&>(geom0);

			//pxPrintf("sweep convex vs %d\n", geom1.getType());
			// AP: backface culling is not supported?
			Gu::SweepConvexFunc func = Gu::gSweepConvexMap[geom1.getType()];
			return func(geom1, pose1, convexGeom, pose0, unitDir, distance, sweepHit, hintFlags, inflation);
		}

		default :
			PX_CHECK_MSG(false, "PxGeometryQuery::sweep(): first geometry object parameter must be sphere, capsule, box or convex geometry.");
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

bool PxGeometryQuery::overlap(	const PxGeometry& geom0, const PxTransform& pose0,
								const PxGeometry& geom1, const PxTransform& pose1)
{
	PX_SIMD_GUARD;
	PX_CHECK_AND_RETURN_VAL(pose0.isValid(), "PxGeometryQuery::overlap(): pose0 is not valid.", false);
	PX_CHECK_AND_RETURN_VAL(pose1.isValid(), "PxGeometryQuery::overlap(): pose1 is not valid.", false);
	
	if(geom0.getType() > geom1.getType())
	{
		//pxPrintf("geomQuery overlap path A %d vs %d\n", geom0.getType(), geom1.getType());
		Gu::GeomOverlapFunc overlapFunc = Gu::gGeomOverlapMethodTable[geom1.getType()][geom0.getType()];
		PX_ASSERT(overlapFunc);
		return overlapFunc(geom1, pose1, geom0, pose0, NULL);
	}
	else
	{
		//pxPrintf("geomQuery overlap path B %d vs %d\n", geom0.getType(), geom1.getType());
		Gu::GeomOverlapFunc overlapFunc = Gu::gGeomOverlapMethodTable[geom0.getType()][geom1.getType()];
		PX_ASSERT(overlapFunc);
		return overlapFunc(geom0, pose0, geom1, pose1, NULL);
	}
}

///////////////////////////////////////////////////////////////////////////////

//#define USE_LOCAL_MAP	// PT: TODO: this is a lot slower for unknown reasons. Figure out why.
#ifdef USE_LOCAL_MAP
static const Gu::RaycastFunc gLocalRaycastMap[7] =
{
	Gu::raycast_sphere,
	Gu::raycast_plane,
	Gu::raycast_capsule,
	Gu::raycast_box,
	Gu::raycast_convexMesh,
	Gu::raycast_triangleMesh,
	Gu::raycast_heightField
};
#endif

PxU32 PxGeometryQuery::raycast(	const PxVec3& rayOrigin, const PxVec3& rayDir,
								const PxGeometry& geom, const PxTransform& pose,
								PxReal maxDist, PxHitFlags hintFlags,
								PxU32 maxHits, PxRaycastHit* PX_RESTRICT rayHits,
								bool anyHit)
{
	PX_SIMD_GUARD;
	PX_CHECK_AND_RETURN_VAL(rayDir.isFinite(), "PxGeometryQuery::raycast(): rayDir is not valid.", 0);
	PX_CHECK_AND_RETURN_VAL(rayOrigin.isFinite(), "PxGeometryQuery::raycast(): rayOrigin is not valid.", 0);
	PX_CHECK_AND_RETURN_VAL(pose.isValid(), "PxGeometryQuery::raycast(): pose is not valid.", 0);
	PX_CHECK_AND_RETURN_VAL(PxIsFinite(maxDist), "PxGeometryQuery::sweep(): maxDist is not valid.", false);
	PX_CHECK_AND_RETURN_VAL(PxAbs(rayDir.magnitudeSquared()-1)<1e-4, "PxGeometryQuery::raycast(): ray direction must be unit vector.", false);

#ifdef USE_LOCAL_MAP
	Gu::RaycastFunc func = gLocalRaycastMap[geom.getType()];
#else
	Gu::RaycastFunc func = Gu::gRaycastMap[geom.getType()];
#endif
	return func(geom, pose, rayOrigin, rayDir, maxDist, hintFlags, maxHits, rayHits, anyHit, NULL, NULL);
}

///////////////////////////////////////////////////////////////////////////////

#include "GuConvexMesh.h"
#include "GuDistancePointBox.h"
bool pointConvexDistance(PxVec3& normal_, PxVec3& closestPoint_, PxReal& sqDistance, const PxVec3& pt, const Gu::ConvexMesh* convexMesh, const PxMeshScale& meshScale, const PxTransform& convexPose);

PxReal PxGeometryQuery::pointDistance(const PxVec3& point, const PxGeometry& geom, const PxTransform& pose, PxVec3* closestPoint)
{
	PX_SIMD_GUARD;
	PX_CHECK_AND_RETURN_VAL(pose.isValid(), "PxGeometryQuery::pointDistance(): pose is not valid.", false);

	switch(geom.getType())
	{
		case PxGeometryType::eSPHERE:
		{
			const PxSphereGeometry& sphereGeom = static_cast<const PxSphereGeometry&>(geom);

			const PxReal r = sphereGeom.radius;

			PxVec3 delta = point - pose.p;
			const PxReal d = delta.magnitude();
			if(d<=r)
				return 0.0f;

			if(closestPoint)
			{
				delta /= d;
				*closestPoint = pose.p + delta * r;
			}

			return (d - r)*(d - r);
		}
		break;

		case PxGeometryType::eCAPSULE:
		{
			const PxCapsuleGeometry& capsGeom = static_cast<const PxCapsuleGeometry&>(geom);

			// PT: TODO: don't use "world space" here
			Gu::Capsule capsule;
			getCapsule(capsule, capsGeom, pose);

			const PxReal r = capsGeom.radius;

			PxReal param;
			const PxReal sqDistance = distancePointSegmentSquared(capsule, point, &param);
			if(sqDistance<=r*r)
				return 0.0f;

			const PxReal d = physx::intrinsics::sqrt(sqDistance);

			if(closestPoint)
			{
				const PxVec3 cp = capsule.getPointAt(param);

				PxVec3 delta = point - cp;
				delta.normalize();

				*closestPoint = cp + delta * r;
			}
			return (d - r)*(d - r);
		}
		break;

		case PxGeometryType::eBOX:
		{
			const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom);

			// PT: TODO: don't use "world space" here
			Gu::Box obb;
			buildFrom(obb, pose.p, boxGeom.halfExtents, pose.q);

			PxVec3 boxParam;
			const PxReal sqDistance = distancePointBoxSquared(point, obb, &boxParam);
			if(closestPoint && sqDistance!=0.0f)
			{
				*closestPoint = obb.transform(boxParam);
			}
			return sqDistance;
		}
		break;

		case PxGeometryType::eCONVEXMESH:
		{
			const PxConvexMeshGeometry& convexGeom = static_cast<const PxConvexMeshGeometry&>(geom);

			PxVec3 normal, cp;
			PxReal sqDistance;
			pointConvexDistance(normal, cp, sqDistance, point, static_cast<Gu::ConvexMesh*>(convexGeom.convexMesh), convexGeom.scale, pose);
			if(closestPoint)
				*closestPoint = cp;
			return sqDistance;
		}
		break;

		default :
			PX_CHECK_AND_RETURN_VAL(false, "PxGeometryQuery::pointDistance(): geometry object parameter must be sphere, capsule box or convex geometry.", -1.0f);
		break;
	}
	return -1.0f;
}

///////////////////////////////////////////////////////////////////////////////

PxBounds3 PxGeometryQuery::getWorldBounds(const PxGeometry& geom, const PxTransform& pose, float inflation)
{
	PX_SIMD_GUARD;
	PX_CHECK_AND_RETURN_VAL(pose.isValid(), "Gu::GeometryQuery::getWorldBounds(): pose is not valid.", PxBounds3::empty());
	Gu::GeometryUnion gu;
	gu.set(geom);

	PxBounds3 result;
	gu.computeBounds(result, pose, 0.0f, NULL);
	PX_ASSERT(result.isValid());

	// PT: unfortunately we can't just scale the min/max vectors, we need to go through center/extents.
	// It would be more efficient to return center/extents from 'computeBounds'...
	const PxVec3 center = result.getCenter();
	const PxVec3 inflatedExtents = result.getExtents() * inflation;
	return PxBounds3::centerExtents(center, inflatedExtents);
}

///////////////////////////////////////////////////////////////////////////////

bool PxGeometryQuery::computePenetration(	PxVec3& mtd, PxF32& depth,
							const PxGeometry& geom0, const PxTransform& pose0,
							const PxGeometry& geom1, const PxTransform& pose1)
{
	PX_SIMD_GUARD;
	PX_CHECK_AND_RETURN_VAL(pose0.isValid(), "PxGeometryQuery::computePenetration(): pose0 is not valid.", false);
	PX_CHECK_AND_RETURN_VAL(pose1.isValid(), "PxGeometryQuery::computePenetration(): pose1 is not valid.", false);

	if(geom0.getType() > geom1.getType())
	{
		Gu::GeomMTDFunc mtdFunc = Gu::gGeomMTDMethodTable[geom1.getType()][geom0.getType()];
		PX_ASSERT(mtdFunc);
		if(!mtdFunc(mtd, depth, geom1, pose1, geom0, pose0))
			return false;
		mtd = -mtd;
		return true;
	}
	else
	{
		Gu::GeomMTDFunc mtdFunc = Gu::gGeomMTDMethodTable[geom0.getType()][geom1.getType()];
		PX_ASSERT(mtdFunc);
		return mtdFunc(mtd, depth, geom0, pose0, geom1, pose1);
	}
}
