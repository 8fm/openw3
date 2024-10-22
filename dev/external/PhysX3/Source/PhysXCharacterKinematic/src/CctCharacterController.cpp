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
  
#include "CctCharacterController.h"
#include "CctSweptBox.h"
#include "CctSweptCapsule.h"
#include "CctObstacleContext.h"
#include "PxRigidDynamic.h"
#include "CmRenderOutput.h"
#include "PsMathUtils.h"
#include "GuIntersectionBoxBox.h"
#include "GuDistanceSegmentBox.h"
#include "PxMeshQuery.h"
//#include <Windows.h>
//#include <stdio.h>

// PT: TODO: remove those includes.... shouldn't be allowed from here
#include "PxControllerObstacles.h"	// (*)
#include "CctInternalStructs.h"		// (*)
#include "PxControllerManager.h"	// (*)
#include "PxControllerBehavior.h"	// (*)

//#define DEBUG_MTD
#ifdef DEBUG_MTD
	#include <stdio.h>
#endif

#define	MAX_ITER	10

using namespace physx;
using namespace Cct;
using namespace Gu;

static const PxU32 gObstacleDebugColor = (PxU32)PxDebugColor::eARGB_CYAN;
//static const PxU32 gCCTBoxDebugColor = (PxU32)PxDebugColor::eARGB_YELLOW;
static const PxU32 gTBVDebugColor = (PxU32)PxDebugColor::eARGB_MAGENTA;
static const bool gUsePartialUpdates = true;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static PX_FORCE_INLINE PxHitFlags getSweepHintFlags(const CCTParams& params)
{
	PxHitFlags sweepHintFlags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
	if(params.mPreciseSweeps)
		sweepHintFlags |= PxHitFlag::ePRECISE_SWEEP;
	return sweepHintFlags;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const bool gUseLocalSpace = true;
static PxVec3 worldToLocal(const PxObstacle& obstacle, const PxExtendedVec3& worldPos)
{
	const PxTransform tr(toVec3(obstacle.mPos), obstacle.mRot);
	return tr.transformInv(toVec3(worldPos));
}

static PxVec3 localToWorld(const PxObstacle& obstacle, const PxVec3& localPos)
{
	const PxTransform tr(toVec3(obstacle.mPos), obstacle.mRot);
	return tr.transform(localPos);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static PX_INLINE void computeReflexionVector(PxVec3& reflected, const PxVec3& incomingDir, const PxVec3& outwardNormal)
{
	reflected = incomingDir - outwardNormal * 2.0f * (incomingDir.dot(outwardNormal));
}

static PX_INLINE bool collisionResponse(PxExtendedVec3& targetPosition, const PxExtendedVec3& currentPosition, const PxVec3& currentDir, const PxVec3& hitNormal, PxF32 bump, PxF32 friction, bool normalize=false, bool side=false)
{
	//char str[1000] = {0};

	// Compute reflect direction
	PxVec3 reflectDir;
	computeReflexionVector(reflectDir, currentDir, hitNormal);
	reflectDir.normalize();

	// Decompose it
	PxVec3 normalCompo, tangentCompo;
	Ps::decomposeVector(normalCompo, tangentCompo, reflectDir, hitNormal);

	// Compute new destination position
	const PxExtended amplitude = distance(targetPosition, currentPosition);

	//sprintf( str, "[Warning] >>>> colresponse - target {%f,%f,%f} amp %f dotND %f normalized %i \n", targetPosition.x, targetPosition.y, targetPosition.z, amplitude, currentDir.dot(hitNormal), !!normalize );
	//OutputDebugStringA( str );
	//sprintf( str, "[Warning] >>>> colresponse - drnt {%f,%f,%f}{%f,%f,%f}{%f,%f,%f}{%f,%f,%f} \n", currentDir.x, currentDir.y, currentDir.z, reflectDir.x, reflectDir.y, reflectDir.z, hitNormal.x, hitNormal.y, hitNormal.z, tangentCompo.x, tangentCompo.y, tangentCompo.z );
	//OutputDebugStringA( str );

	targetPosition = currentPosition;
	if(bump!=0.0f)
	{
		if(normalize)
			normalCompo.normalize();
		targetPosition += normalCompo*float(bump*amplitude);
	}
	if(friction!=0.0)
	{
		// side collision response with vertical projection of movement dir onto ground
		if ( side && hitNormal.z > 0.5f )
		{
			//sprintf( str, "[Error] >>>> colresponse res - normal %f\n", hitNormal.z );
			//OutputDebugStringA( str );

			PxVec3 p0 = currentDir;		
			PxVec3 p1 = currentDir;
			p0.z = 5.0f; p1.z = -5.0f;
			PxVec3 pDif = p1-p0;

			float u = hitNormal.dot( -p0 );
			float v = hitNormal.dot( pDif );

			//sprintf( str, "[Error] >>>> colresponse res - normal u %f v %f\n", u, v );
			//OutputDebugStringA( str );
			
			// are not parallel to ground (v>0.0)
			if ( fabsf( v ) > 0.001f )
			{
				float r = u/v;

				//sprintf( str, "[Error] >>>> colresponse res - normal r %f\n", r );
				//OutputDebugStringA( str );

				// solution exists on ground plane (0.0 < r <= 1.0)
				if ( r > 0.00001f && r <= 1.0f )
				{
					PxVec3 pr = p0 + pDif*r;

					//sprintf( str, "[Warning] >>>> colresponse - resres {%f,%f,%f}{%f,%f,%f}{%f,%f,%f} \n", pr.x, pr.y, pr.z, p0.x, p0.y, p0.z, pDif.x, pDif.y, pDif.z );
					//OutputDebugStringA( str );

					// apply displacement amplitude
					pr.normalize();
					pr *= (float)amplitude;

					//sprintf( str, "[Warning] >>>> colresponse - res pr {%f,%f,%f} \n", pr.x, pr.y, pr.z );
					//OutputDebugStringA( str );

					// move
					targetPosition += pr;
				}
			}
		}

		// std collision response
		else
		{
			//sprintf( str, "[Warning] >>>> colresponse res - std %f\n", hitNormal.z );
			//OutputDebugStringA( str );

			if(normalize)
				tangentCompo.normalize();

			// move
			targetPosition += tangentCompo*float(friction*amplitude);
		}

		// temporarily disabled - need more testing for tangent response vector
		if ( false && hitNormal.z < 0.71f && fabsf( currentDir.z ) < 0.1f )
		{
			tangentCompo.z = 0.0f;

			PxVec3 dir = currentDir;
			dir.normalize();
			PxVec3 normal = hitNormal;
			normal.z = 0.0f;
			normal.normalize();

			PxVec3 tanHorizontal = normal.cross( dir.cross( normal ) );
			const float tangentCompoLen = tangentCompo.magnitude();
			tanHorizontal *= tangentCompoLen;
			tangentCompo = tanHorizontal;

			//sprintf( str, "[Warning] >>>> colresponse res - target after {%f,%f,%f} - bump %f\n", tangentCompo.x, tangentCompo.y, tangentCompo.z, tangentCompoLen );
			//OutputDebugStringA( str );

			targetPosition += tangentCompo*float(friction*amplitude);

			//sprintf( str, "[Warning] >>>> colresponse res - target after {%f,%f,%f}\n", targetPosition.x, targetPosition.y, targetPosition.z );
			//OutputDebugStringA( str );
			return false;
		}
	}

	//sprintf( str, "[Warning] >>>> colresponse res - target after {%f,%f,%f}\n", targetPosition.x, targetPosition.y, targetPosition.z );
	//OutputDebugStringA( str );

	return true;
}

static PX_INLINE void relocateBox(PxBoxGeometry& boxGeom, PxTransform& pose, const PxExtendedVec3& center, const PxVec3& extents, const PxExtendedVec3& origin, const PxQuat& quatFromUp)
{
	boxGeom.halfExtents = extents;

	pose.p.x = float(center.x - origin.x);
	pose.p.y = float(center.y - origin.y);
	pose.p.z = float(center.z - origin.z);

	pose.q = quatFromUp;
}

static PX_INLINE void relocateBox(PxBoxGeometry& boxGeom, PxTransform& pose, const TouchedUserBox& userBox)
{
	relocateBox(boxGeom, pose, userBox.mBox.center, userBox.mBox.extents, userBox.mOffset, userBox.mBox.rot);
}

static PX_INLINE void relocateBox(PxBoxGeometry& boxGeom, PxTransform& pose, const TouchedBox& box)
{
	boxGeom.halfExtents = box.mExtents;

	pose.p = box.mCenter;
	pose.q = box.mRot;
}

static PX_INLINE void relocateCapsule(
	PxCapsuleGeometry& capsuleGeom, PxTransform& pose, const SweptCapsule* sc,
	const PxQuat& quatFromUp,
	const PxExtendedVec3& center, const PxExtendedVec3& origin)
{
	capsuleGeom.radius = sc->mRadius;
	capsuleGeom.halfHeight = 0.5f * sc->mHeight;

	pose.p.x = float(center.x - origin.x);
	pose.p.y = float(center.y - origin.y);
	pose.p.z = float(center.z - origin.z);

	pose.q = quatFromUp;
}

static PX_INLINE void relocateCapsule(PxCapsuleGeometry& capsuleGeom, PxTransform& pose, const PxVec3& p0, const PxVec3& p1, PxReal radius)
{
	capsuleGeom.radius = radius;
	pose = PxTransformFromSegment(p0, p1, &capsuleGeom.halfHeight);
}

static PX_INLINE void relocateCapsule(PxCapsuleGeometry& capsuleGeom, PxTransform& pose, const TouchedUserCapsule& userCapsule)
{
	PxVec3 p0, p1;
	p0.x = float(userCapsule.mCapsule.p0.x - userCapsule.mOffset.x);
	p0.y = float(userCapsule.mCapsule.p0.y - userCapsule.mOffset.y);
	p0.z = float(userCapsule.mCapsule.p0.z - userCapsule.mOffset.z);
	p1.x = float(userCapsule.mCapsule.p1.x - userCapsule.mOffset.x);
	p1.y = float(userCapsule.mCapsule.p1.y - userCapsule.mOffset.y);
	p1.z = float(userCapsule.mCapsule.p1.z - userCapsule.mOffset.z);

	relocateCapsule(capsuleGeom, pose, p0, p1, userCapsule.mCapsule.radius);
}

static bool SweepBoxUserBox(const SweepTest* test, const SweptVolume* volume, const TouchedGeom* geom, const PxExtendedVec3& center, const PxVec3& dir, SweptContact& impact)
{
	PX_ASSERT(volume->getType()==SweptVolumeType::eBOX);
	PX_ASSERT(geom->mType==TouchedGeomType::eUSER_BOX);
	const SweptBox* SB = static_cast<const SweptBox*>(volume);
	const TouchedUserBox* TC = static_cast<const TouchedUserBox*>(geom);

	PxBoxGeometry boxGeom0;
	PxTransform boxPose0;
	// To precompute
	relocateBox(boxGeom0, boxPose0, center, SB->mExtents, TC->mOffset, test->mUserParams.mQuatFromUp);

	PxBoxGeometry boxGeom1;
	PxTransform boxPose1;
	relocateBox(boxGeom1, boxPose1, *TC);

	PxSweepHit sweepHit;
	if(!PxGeometryQuery::sweep(dir, impact.mDistance, boxGeom0, boxPose0, boxGeom1, boxPose1, sweepHit, getSweepHintFlags(test->mUserParams)))
		return false;

	if(sweepHit.distance >= impact.mDistance)
		return false;

	impact.mWorldNormal		= sweepHit.normal;
	impact.mDistance		= sweepHit.distance;
	impact.mInternalIndex	= PX_INVALID_U32;
	impact.mTriangleIndex	= PX_INVALID_U32;
	impact.setWorldPos(sweepHit.position, TC->mOffset);
	return true;
}

static bool SweepBoxUserCapsule(const SweepTest* test, const SweptVolume* volume, const TouchedGeom* geom, const PxExtendedVec3& center, const PxVec3& dir, SweptContact& impact)
{
	PX_ASSERT(volume->getType()==SweptVolumeType::eBOX);
	PX_ASSERT(geom->mType==TouchedGeomType::eUSER_CAPSULE);
	const SweptBox* SB = static_cast<const SweptBox*>(volume);
	const TouchedUserCapsule* TC = static_cast<const TouchedUserCapsule*>(geom);

	PxBoxGeometry boxGeom;
	PxTransform boxPose;
	// To precompute
	relocateBox(boxGeom, boxPose, center, SB->mExtents, TC->mOffset, test->mUserParams.mQuatFromUp);

	PxCapsuleGeometry capsuleGeom;
	PxTransform capsulePose;
	relocateCapsule(capsuleGeom, capsulePose, *TC);

	PxSweepHit sweepHit;
	if(!PxGeometryQuery::sweep(dir, impact.mDistance, boxGeom, boxPose, capsuleGeom, capsulePose, sweepHit, getSweepHintFlags(test->mUserParams)))
		return false;

	if(sweepHit.distance >= impact.mDistance)
		return false;

	impact.mDistance		= sweepHit.distance;
	impact.mWorldNormal		= sweepHit.normal;
	impact.mInternalIndex	= PX_INVALID_U32;
	impact.mTriangleIndex	= PX_INVALID_U32;
	//TO CHECK: Investigate whether any significant performance improvement can be achieved through
	//          making the impact point computation optional in the sweep calls and compute it later
	/*{
		// ### check this
		float t;
		PxVec3 p;
		float d = gUtilLib->PxSegmentOBBSqrDist(Capsule, Box0.center, Box0.extents, Box0.rot, &t, &p);
		Box0.rot.multiply(p,p);
		impact.mWorldPos.x = p.x + Box0.center.x + TC->mOffset.x;
		impact.mWorldPos.y = p.y + Box0.center.y + TC->mOffset.y;
		impact.mWorldPos.z = p.z + Box0.center.z + TC->mOffset.z;
	}*/
	{
		impact.setWorldPos(sweepHit.position, TC->mOffset);
	}
	return true;
}

static bool sweepVolumeVsMesh(	const SweepTest* sweepTest, const TouchedMesh* touchedMesh, SweptContact& impact,
								const PxVec3& unitDir, const PxGeometry& geom, const PxTransform& pose,
								PxU32 nbTris, const PxTriangle* triangles,
								PxU32 cachedIndex)
{
	PxSweepHit sweepHit;
	if(PxMeshQuery::sweep(unitDir, impact.mDistance, geom, pose, nbTris, triangles, sweepHit, getSweepHintFlags(sweepTest->mUserParams), &cachedIndex))
	{
		if(sweepHit.distance >= impact.mDistance)
			return false;

		impact.mDistance	= sweepHit.distance;
		impact.mWorldNormal	= sweepHit.normal;
		impact.setWorldPos(sweepHit.position, touchedMesh->mOffset);

		// Returned index is only between 0 and nbTris, i.e. it indexes the array of cached triangles, not the original mesh.
		PX_ASSERT(sweepHit.faceIndex < nbTris);
		sweepTest->mCachedTriIndex[sweepTest->mCachedTriIndexIndex] = sweepHit.faceIndex;

		// The CCT loop will use the index from the start of the cache...
		impact.mInternalIndex = sweepHit.faceIndex + touchedMesh->mIndexWorldTriangles;
		const PxU32* triangleIndices = &sweepTest->mTriangleIndices[touchedMesh->mIndexWorldTriangles];
		impact.mTriangleIndex = triangleIndices[sweepHit.faceIndex];
		return true;
	}
	return false;
}

static bool SweepBoxMesh(const SweepTest* sweep_test, const SweptVolume* volume, const TouchedGeom* geom, const PxExtendedVec3& center, const PxVec3& dir, SweptContact& impact)
{
	PX_ASSERT(volume->getType()==SweptVolumeType::eBOX);
	PX_ASSERT(geom->mType==TouchedGeomType::eMESH);
	const SweptBox* SB = static_cast<const SweptBox*>(volume);
	const TouchedMesh* TM = static_cast<const TouchedMesh*>(geom);

	PxU32 nbTris = TM->mNbTris;
	if(!nbTris)
		return false;

	// Fetch triangle data for current mesh (the stream may contain triangles from multiple meshes)
	const PxTriangle* T = &sweep_test->mWorldTriangles[TM->mIndexWorldTriangles];

	// PT: this only really works when the CCT collides with a single mesh, but that's the most common case. When it doesn't, there's just no speedup but it still works.
	PxU32 CachedIndex = sweep_test->mCachedTriIndex[sweep_test->mCachedTriIndexIndex];
	if(CachedIndex>=nbTris)
		CachedIndex=0;

	PxBoxGeometry boxGeom;
	boxGeom.halfExtents = SB->mExtents;
	PxTransform boxPose(PxVec3(float(center.x - TM->mOffset.x), float(center.y - TM->mOffset.y), float(center.z - TM->mOffset.z)), sweep_test->mUserParams.mQuatFromUp);  // Precompute
	return sweepVolumeVsMesh(sweep_test, TM, impact, dir, boxGeom, boxPose, nbTris, T, CachedIndex);
}

static bool SweepCapsuleMesh(
	const SweepTest* sweep_test, const SweptVolume* volume, const TouchedGeom* geom,
	const PxExtendedVec3& center, const PxVec3& dir, SweptContact& impact)
{
	PX_ASSERT(volume->getType()==SweptVolumeType::eCAPSULE);
	PX_ASSERT(geom->mType==TouchedGeomType::eMESH);
	const SweptCapsule* SC = static_cast<const SweptCapsule*>(volume);
	const TouchedMesh* TM = static_cast<const TouchedMesh*>(geom);

	PxU32 nbTris = TM->mNbTris;
	if(!nbTris)
		return false;

	// Fetch triangle data for current mesh (the stream may contain triangles from multiple meshes)
	const PxTriangle* T	= &sweep_test->mWorldTriangles[TM->mIndexWorldTriangles];

	// PT: this only really works when the CCT collides with a single mesh, but that's the most common case.
	// When it doesn't, there's just no speedup but it still works.
	PxU32 CachedIndex = sweep_test->mCachedTriIndex[sweep_test->mCachedTriIndexIndex];
	if(CachedIndex>=nbTris)
		CachedIndex=0;

	PxCapsuleGeometry capsuleGeom;
	PxTransform capsulePose;
	relocateCapsule(capsuleGeom, capsulePose, SC, sweep_test->mUserParams.mQuatFromUp, center, TM->mOffset);

	return sweepVolumeVsMesh(sweep_test, TM, impact, dir, capsuleGeom, capsulePose, nbTris, T, CachedIndex);
}

static bool SweepBoxBox(const SweepTest* test, const SweptVolume* volume, const TouchedGeom* geom, const PxExtendedVec3& center, const PxVec3& dir, SweptContact& impact)
{
	PX_ASSERT(volume->getType()==SweptVolumeType::eBOX);
	PX_ASSERT(geom->mType==TouchedGeomType::eBOX);
	const SweptBox* SB = static_cast<const SweptBox*>(volume);
	const TouchedBox* TB = static_cast<const TouchedBox*>(geom);

	PxBoxGeometry boxGeom0;
	PxTransform boxPose0;
	// To precompute
	relocateBox(boxGeom0, boxPose0, center, SB->mExtents, TB->mOffset, test->mUserParams.mQuatFromUp);

	PxBoxGeometry boxGeom1;
	PxTransform boxPose1;
	relocateBox(boxGeom1, boxPose1, *TB);

	PxSweepHit sweepHit;
	if(!PxGeometryQuery::sweep(dir, impact.mDistance, boxGeom0, boxPose0, boxGeom1, boxPose1, sweepHit, getSweepHintFlags(test->mUserParams)))
		return false;

	if(sweepHit.distance >= impact.mDistance)
		return false;

	impact.mWorldNormal		= sweepHit.normal;
	impact.mDistance		= sweepHit.distance;
	impact.mInternalIndex	= PX_INVALID_U32;
	impact.mTriangleIndex	= PX_INVALID_U32;
	impact.setWorldPos(sweepHit.position, TB->mOffset);
	return true;
}

static bool SweepBoxSphere(const SweepTest* test, const SweptVolume* volume, const TouchedGeom* geom, const PxExtendedVec3& center, const PxVec3& dir, SweptContact& impact)
{
	PX_ASSERT(volume->getType()==SweptVolumeType::eBOX);
	PX_ASSERT(geom->mType==TouchedGeomType::eSPHERE);
	const SweptBox* SB = static_cast<const SweptBox*>(volume);
	const TouchedSphere* TS = static_cast<const TouchedSphere*>(geom);

	PxBoxGeometry boxGeom;
	PxTransform boxPose;
	// To precompute
	relocateBox(boxGeom, boxPose, center, SB->mExtents, TS->mOffset, test->mUserParams.mQuatFromUp);

	PxSphereGeometry sphereGeom;
	sphereGeom.radius = TS->mRadius;
	PxTransform spherePose;
	spherePose.p = TS->mCenter;
	spherePose.q = PxQuat(PxIdentity);

	PxSweepHit sweepHit;
	if(!PxGeometryQuery::sweep(dir, impact.mDistance, boxGeom, boxPose, sphereGeom, spherePose, sweepHit, getSweepHintFlags(test->mUserParams)))
		return false;

	impact.mDistance		= sweepHit.distance;
	impact.mWorldNormal		= sweepHit.normal;
	impact.mInternalIndex	= PX_INVALID_U32;
	impact.mTriangleIndex	= PX_INVALID_U32;
	//TO CHECK: Investigate whether any significant performance improvement can be achieved through
	//          making the impact point computation optional in the sweep calls and compute it later
	/*
	{
		// The sweep test doesn't compute the impact point automatically, so we have to do it here.
		PxVec3 NewSphereCenter = TS->mSphere.center - d * dir;
		PxVec3 Closest;
		gUtilLib->PxPointOBBSqrDist(NewSphereCenter, Box0.center, Box0.extents, Box0.rot, &Closest);
		// Compute point on the box, after sweep
		Box0.rot.multiply(Closest, Closest);
		impact.mWorldPos.x = TS->mOffset.x + Closest.x + Box0.center.x + d * dir.x;
		impact.mWorldPos.y = TS->mOffset.y + Closest.y + Box0.center.y + d * dir.y;
		impact.mWorldPos.z = TS->mOffset.z + Closest.z + Box0.center.z + d * dir.z;

		impact.mWorldNormal = -impact.mWorldNormal;
	}*/
	{
		impact.setWorldPos(sweepHit.position, TS->mOffset);
	}
	return true;
}

static bool SweepBoxCapsule(const SweepTest* test, const SweptVolume* volume, const TouchedGeom* geom, const PxExtendedVec3& center, const PxVec3& dir, SweptContact& impact)
{
	PX_ASSERT(volume->getType()==SweptVolumeType::eBOX);
	PX_ASSERT(geom->mType==TouchedGeomType::eCAPSULE);
	const SweptBox* SB = static_cast<const SweptBox*>(volume);
	const TouchedCapsule* TC = static_cast<const TouchedCapsule*>(geom);

	PxBoxGeometry boxGeom;
	PxTransform boxPose;
	// To precompute
	relocateBox(boxGeom, boxPose, center, SB->mExtents, TC->mOffset, test->mUserParams.mQuatFromUp);

	PxCapsuleGeometry capsuleGeom;
	PxTransform capsulePose;
	relocateCapsule(capsuleGeom, capsulePose, TC->mP0, TC->mP1, TC->mRadius);

	PxSweepHit sweepHit;
	if(!PxGeometryQuery::sweep(dir, impact.mDistance, boxGeom, boxPose, capsuleGeom, capsulePose, sweepHit, getSweepHintFlags(test->mUserParams)))
		return false;

	if(sweepHit.distance >= impact.mDistance)
		return false;

	impact.mDistance		= sweepHit.distance;
	impact.mWorldNormal		= sweepHit.normal;
	impact.mInternalIndex	= PX_INVALID_U32;
	impact.mTriangleIndex	= PX_INVALID_U32;
	//TO CHECK: Investigate whether any significant performance improvement can be achieved through
	//          making the impact point computation optional in the sweep calls and compute it later
	/*{
		float t;
		PxVec3 p;
		float d = gUtilLib->PxSegmentOBBSqrDist(TC->mCapsule, Box0.center, Box0.extents, Box0.rot, &t, &p);
		Box0.rot.multiply(p,p);
		impact.mWorldPos.x = p.x + Box0.center.x + TC->mOffset.x;
		impact.mWorldPos.y = p.y + Box0.center.y + TC->mOffset.y;
		impact.mWorldPos.z = p.z + Box0.center.z + TC->mOffset.z;
	}*/
	{
		impact.setWorldPos(sweepHit.position, TC->mOffset);
	}
	return true;
}

static bool SweepCapsuleBox(const SweepTest* test, const SweptVolume* volume, const TouchedGeom* geom, const PxExtendedVec3& center, const PxVec3& dir, SweptContact& impact)
{
	PX_ASSERT(volume->getType()==SweptVolumeType::eCAPSULE);
	PX_ASSERT(geom->mType==TouchedGeomType::eBOX);
	const SweptCapsule* SC = static_cast<const SweptCapsule*>(volume);
	const TouchedBox* TB = static_cast<const TouchedBox*>(geom);

	PxCapsuleGeometry capsuleGeom;
	PxTransform capsulePose;
	relocateCapsule(capsuleGeom, capsulePose, SC, test->mUserParams.mQuatFromUp, center, TB->mOffset);

	PxBoxGeometry boxGeom;
	PxTransform boxPose;
	// To precompute
	relocateBox(boxGeom, boxPose, *TB);

	// The box and capsule coordinates are relative to the center of the cached bounding box
	PxSweepHit sweepHit;
	if(!PxGeometryQuery::sweep(dir, impact.mDistance, capsuleGeom, capsulePose, boxGeom, boxPose, sweepHit, getSweepHintFlags(test->mUserParams)))
		return false;

	if(sweepHit.distance >= impact.mDistance)
		return false;

	impact.mDistance		= sweepHit.distance;
	impact.mWorldNormal		= sweepHit.normal;
	impact.mInternalIndex	= PX_INVALID_U32;
	impact.mTriangleIndex	= PX_INVALID_U32;

	//TO CHECK: Investigate whether any significant performance improvement can be achieved through
	//          making the impact point computation optional in the sweep calls and compute it later
	/*{
		float t;
		PxVec3 p;
		float d = gUtilLib->PxSegmentOBBSqrDist(Capsule, TB->mBox.center, TB->mBox.extents, TB->mBox.rot, &t, &p);
		TB->mBox.rot.multiply(p,p);
		p += TB->mBox.center;
		impact.mWorldPos.x = p.x + TB->mOffset.x;
		impact.mWorldPos.y = p.y + TB->mOffset.y;
		impact.mWorldPos.z = p.z + TB->mOffset.z;
	}*/
	{
		impact.setWorldPos(sweepHit.position, TB->mOffset);
	}
	return true;
}

static bool SweepCapsuleSphere(const SweepTest* test, const SweptVolume* volume, const TouchedGeom* geom, const PxExtendedVec3& center, const PxVec3& dir, SweptContact& impact)
{
	PX_ASSERT(volume->getType()==SweptVolumeType::eCAPSULE);
	PX_ASSERT(geom->mType==TouchedGeomType::eSPHERE);
	const SweptCapsule* SC = static_cast<const SweptCapsule*>(volume);
	const TouchedSphere* TS = static_cast<const TouchedSphere*>(geom);

	PxCapsuleGeometry capsuleGeom;
	PxTransform capsulePose;
	relocateCapsule(capsuleGeom, capsulePose, SC, test->mUserParams.mQuatFromUp, center, TS->mOffset);

	PxSphereGeometry sphereGeom;
	sphereGeom.radius = TS->mRadius;
	PxTransform spherePose;
	spherePose.p = TS->mCenter;
	spherePose.q = PxQuat(PxIdentity);

	PxSweepHit sweepHit;
	if(!PxGeometryQuery::sweep(dir, impact.mDistance, capsuleGeom, capsulePose, sphereGeom, spherePose, sweepHit, getSweepHintFlags(test->mUserParams)))
		return false;

	if(sweepHit.distance >= impact.mDistance)
		return false;

	impact.mDistance		= sweepHit.distance;
	impact.mWorldNormal		= sweepHit.normal;
	impact.mInternalIndex	= PX_INVALID_U32;
	impact.mTriangleIndex	= PX_INVALID_U32;
	impact.setWorldPos(sweepHit.position, TS->mOffset);
	return true;
}

static bool SweepCapsuleCapsule(const SweepTest* test, const SweptVolume* volume, const TouchedGeom* geom, const PxExtendedVec3& center, const PxVec3& dir, SweptContact& impact)
{
	PX_ASSERT(volume->getType()==SweptVolumeType::eCAPSULE);
	PX_ASSERT(geom->mType==TouchedGeomType::eCAPSULE);
	const SweptCapsule* SC = static_cast<const SweptCapsule*>(volume);
	const TouchedCapsule* TC = static_cast<const TouchedCapsule*>(geom);

	PxCapsuleGeometry capsuleGeom0;
	PxTransform capsulePose0;
	relocateCapsule(capsuleGeom0, capsulePose0, SC, test->mUserParams.mQuatFromUp, center, TC->mOffset);

	PxCapsuleGeometry capsuleGeom1;
	PxTransform capsulePose1;
	relocateCapsule(capsuleGeom1, capsulePose1, TC->mP0, TC->mP1, TC->mRadius);

	//char str[1000] = {0};
	//if ( fabsf( dir.z ) < 0.001f )
	//{
	//	sprintf( str, "[Warning] >> sweepcc - pose0 %0.3f pose1 %0.3f tc: p0 %0.3f p1 %0.3f - dir %0.3f dist %f\n", capsulePose0.p.z, capsulePose1.p.z, TC->mP0.z, TC->mP1.z, dir.z, impact.mDistance );
	//	OutputDebugStringA( str );
	//}

	PxSweepHit sweepHit;
	if(!PxGeometryQuery::sweep(dir, impact.mDistance, capsuleGeom0, capsulePose0, capsuleGeom1, capsulePose1, sweepHit, getSweepHintFlags(test->mUserParams)))
		return false;

	//if ( fabsf( dir.z ) < 0.001f )
	//{
	//	sprintf( str, "[Warning] >>> DONE sweepcc - pose0 %0.3f pose1 %0.3f tc: p0 %0.3f p1 %0.3f - dir %0.3f dist %f/%f/%f\n", capsulePose0.p.z, capsulePose1.p.z, TC->mP0.z, TC->mP1.z, dir.z, impact.mDistance, sweepHit.distance, impact.mDistance );
	//	OutputDebugStringA( str );
	//}

	if(sweepHit.distance >= impact.mDistance)
		return false;

	impact.mDistance		= sweepHit.distance;
	impact.mWorldNormal		= sweepHit.normal;
	impact.mInternalIndex	= PX_INVALID_U32;
	impact.mTriangleIndex	= PX_INVALID_U32;
	impact.setWorldPos(sweepHit.position, TC->mOffset);
	return true;
}

static bool SweepCapsuleUserCapsule(const SweepTest* test, const SweptVolume* volume, const TouchedGeom* geom, const PxExtendedVec3& center, const PxVec3& dir, SweptContact& impact)
{
	PX_ASSERT(volume->getType()==SweptVolumeType::eCAPSULE);
	PX_ASSERT(geom->mType==TouchedGeomType::eUSER_CAPSULE);
	const SweptCapsule* SC = static_cast<const SweptCapsule*>(volume);
	const TouchedUserCapsule* TC = static_cast<const TouchedUserCapsule*>(geom);

	PxCapsuleGeometry capsuleGeom0;
	PxTransform capsulePose0;
	relocateCapsule(capsuleGeom0, capsulePose0, SC, test->mUserParams.mQuatFromUp, center, TC->mOffset);

	PxCapsuleGeometry capsuleGeom1;
	PxTransform capsulePose1;
	relocateCapsule(capsuleGeom1, capsulePose1, *TC);

	PxSweepHit sweepHit;
	if(!PxGeometryQuery::sweep(dir, impact.mDistance, capsuleGeom0, capsulePose0, capsuleGeom1, capsulePose1, sweepHit, getSweepHintFlags(test->mUserParams)))
		return false;

	if(sweepHit.distance >= impact.mDistance)
		return false;

	impact.mDistance		= sweepHit.distance;
	impact.mWorldNormal		= sweepHit.normal;
	impact.mInternalIndex	= PX_INVALID_U32;
	impact.mTriangleIndex	= PX_INVALID_U32;
	impact.setWorldPos(sweepHit.position, TC->mOffset);
	return true;
}

static bool SweepCapsuleUserBox(const SweepTest* test, const SweptVolume* volume, const TouchedGeom* geom, const PxExtendedVec3& center, const PxVec3& dir, SweptContact& impact)
{
	PX_ASSERT(volume->getType()==SweptVolumeType::eCAPSULE);
	PX_ASSERT(geom->mType==TouchedGeomType::eUSER_BOX);
	const SweptCapsule* SC = static_cast<const SweptCapsule*>(volume);
	const TouchedUserBox* TB = static_cast<const TouchedUserBox*>(geom);

	PxCapsuleGeometry capsuleGeom;
	PxTransform capsulePose;
	relocateCapsule(capsuleGeom, capsulePose, SC, test->mUserParams.mQuatFromUp, center, TB->mOffset);

	PxBoxGeometry boxGeom;
	PxTransform boxPose;
	relocateBox(boxGeom, boxPose, *TB);

	PxSweepHit sweepHit;
	if(!PxGeometryQuery::sweep(dir, impact.mDistance, capsuleGeom, capsulePose, boxGeom, boxPose, sweepHit, getSweepHintFlags(test->mUserParams)))
		return false;

	if(sweepHit.distance >= impact.mDistance)
		return false;

	impact.mDistance		= sweepHit.distance;
	impact.mWorldNormal		= sweepHit.normal;
	impact.mInternalIndex	= PX_INVALID_U32;
	impact.mTriangleIndex	= PX_INVALID_U32;

	//TO CHECK: Investigate whether any significant performance improvement can be achieved through
	//          making the impact point computation optional in the sweep calls and compute it later
	/*{
		// ### check this
		float t;
		PxVec3 p;
		float d = gUtilLib->PxSegmentOBBSqrDist(Capsule, Box.center, Box.extents, Box.rot, &t, &p);
		p += Box.center;
		impact.mWorldPos.x = p.x + TB->mOffset.x;
		impact.mWorldPos.y = p.y + TB->mOffset.y;
		impact.mWorldPos.z = p.z + TB->mOffset.z;
	}*/
	{
		impact.setWorldPos(sweepHit.position, TB->mOffset);
	}
	return true;
}

typedef bool (*SweepFunc) (const SweepTest*, const SweptVolume*, const TouchedGeom*, const PxExtendedVec3&, const PxVec3&, SweptContact&);

static SweepFunc gSweepMap[SweptVolumeType::eLAST][TouchedGeomType::eLAST] = {
	// Box funcs
	{
	SweepBoxUserBox,
	SweepBoxUserCapsule,
	SweepBoxMesh,
	SweepBoxBox,
	SweepBoxSphere,
	SweepBoxCapsule
	},

	// Capsule funcs
	{
	SweepCapsuleUserBox,
	SweepCapsuleUserCapsule,
	SweepCapsuleMesh,
	SweepCapsuleBox,
	SweepCapsuleSphere,
	SweepCapsuleCapsule
	}
};

PX_COMPILE_TIME_ASSERT(sizeof(gSweepMap)==SweptVolumeType::eLAST*TouchedGeomType::eLAST*sizeof(SweepFunc));

static const PxU32 GeomSizes[] = 
{
	sizeof(TouchedUserBox),
	sizeof(TouchedUserCapsule),
	sizeof(TouchedMesh),
	sizeof(TouchedBox),
	sizeof(TouchedSphere),
	sizeof(TouchedCapsule),
};

static TouchedGeom* CollideGeoms(
	const SweepTest* sweep_test, const SweptVolume& volume, const IntArray& geom_stream,
	const PxExtendedVec3& center, const PxVec3& dir, SweptContact& impact, bool discardInitialOverlap)
{
	impact.mInternalIndex	= PX_INVALID_U32;
	impact.mTriangleIndex	= PX_INVALID_U32;
	impact.mGeom			= NULL;

	const PxU32* Data = geom_stream.begin();
	const PxU32* Last = geom_stream.end();
	while(Data!=Last)
	{
		TouchedGeom* CurrentGeom = (TouchedGeom*)Data;

		SweepFunc ST = gSweepMap[volume.getType()][CurrentGeom->mType];
		if(ST)
		{
			SweptContact C;
			C.mDistance			= impact.mDistance;	// Initialize with current best distance
			C.mInternalIndex	= PX_INVALID_U32;
			C.mTriangleIndex	= PX_INVALID_U32;
			if((ST)(sweep_test, &volume, CurrentGeom, center, dir, C))
			{
				if(C.mDistance==0.0f)
				{
					if(!discardInitialOverlap)
					{
						if(CurrentGeom->mType==TouchedGeomType::eUSER_BOX || CurrentGeom->mType==TouchedGeomType::eUSER_CAPSULE)
						{
						}
						else
						{
							const PxRigidActor* touchedActor = CurrentGeom->mActor;
							PX_ASSERT(touchedActor);
							// PT: we must let the dynamic objects go through the CCT for proper 2-way interactions
							if(touchedActor->getConcreteType()==PxConcreteType::eRIGID_STATIC)
							{
								impact = C;
								impact.mGeom = CurrentGeom;
								return CurrentGeom;
							}
						}
					}
				}
/*				else
				if(discardInitialOverlap && C.mDistance==0.0f)
				{
					// PT: we previously used eINITIAL_OVERLAP without eINITIAL_OVERLAP_KEEP, i.e. initially overlapping shapes got ignored.
					// So we replicate this behavior here.
				}*/
				else if(C.mDistance<impact.mDistance)
				{
					impact = C;
					impact.mGeom = CurrentGeom;
					if(C.mDistance <= 0.0f)	// there is no point testing for closer hits
						return CurrentGeom;	// since we are touching a shape already
				}
			}
		}

		PxU8* ptr = (PxU8*)Data;
		ptr += GeomSizes[CurrentGeom->mType];
		Data = (const PxU32*)ptr;
	}
	return impact.mGeom;
}

static PxVec3 computeMTD(const SweepTest* sweep_test, const SweptVolume& volume, const IntArray& geom_stream, const PxExtendedVec3& center, float contactOffset)
{
	PxVec3 p = toVec3(center);

//	contactOffset += 0.01f;

	const PxU32 maxIter = 4;
	PxU32 nbIter = 0;
	bool isValid = true;
	while(isValid && nbIter<maxIter)
	{
		const PxU32* Data = geom_stream.begin();
		const PxU32* Last = geom_stream.end();
		while(Data!=Last)
		{
			TouchedGeom* CurrentGeom = (TouchedGeom*)Data;

			if(CurrentGeom->mType==TouchedGeomType::eUSER_BOX || CurrentGeom->mType==TouchedGeomType::eUSER_CAPSULE)
			{
			}
			else
			{
				const PxRigidActor* touchedActor = CurrentGeom->mActor;
				PX_ASSERT(touchedActor);

				// PT: we must let the dynamic objects go through the CCT for proper 2-way interactions
				if(touchedActor->getConcreteType()==PxConcreteType::eRIGID_STATIC)
				{
					PxShape* touchedShape = (PxShape*)CurrentGeom->mTGUserData;
					PX_ASSERT(touchedShape);

					const PxGeometryHolder gh = touchedShape->getGeometry();
					const PxTransform globalPose = getShapeGlobalPose(*touchedShape, *touchedActor);

					PxVec3 mtd;
					PxF32 depth;

					const PxTransform volumePose(p, sweep_test->mUserParams.mQuatFromUp);
					if(volume.getType()==SweptVolumeType::eCAPSULE)
					{
						const SweptCapsule& sc = static_cast<const SweptCapsule&>(volume);
						const PxCapsuleGeometry capsuleGeom(sc.mRadius+contactOffset, sc.mHeight*0.5f);
						isValid = PxGeometryQuery::computePenetration(mtd, depth, capsuleGeom, volumePose, gh.any(), globalPose);
					}
					else
					{
						PX_ASSERT(volume.getType()==SweptVolumeType::eBOX);
						const SweptBox& sb = static_cast<const SweptBox&>(volume);
						const PxBoxGeometry boxGeom(sb.mExtents+PxVec3(contactOffset));
						isValid = PxGeometryQuery::computePenetration(mtd, depth, boxGeom, volumePose, gh.any(), globalPose);
					}

					if(isValid)
					{
						nbIter++;
						PX_ASSERT(depth>=0.0f);
						PX_ASSERT(mtd.isFinite());
						PX_ASSERT(PxIsFinite(depth));
#ifdef DEBUG_MTD
						PX_ASSERT(depth<=1.0f);
						if(depth>1.0f || !mtd.isFinite() || !PxIsFinite(depth))
						{
							int stop=1;
							(void)stop;
						}
						printf("Depth: %f\n", depth);
						printf("mtd: %f %f %f\n", mtd.x, mtd.y, mtd.z);
#endif
						p += mtd * depth;
					}
				}
			}

			PxU8* ptr = (PxU8*)Data;
			ptr += GeomSizes[CurrentGeom->mType];
			Data = (const PxU32*)ptr;
		}
	}
	return p;
}



static bool ParseGeomStream(const void* object, const IntArray& geom_stream)
{
	const PxU32* Data = geom_stream.begin();
	const PxU32* Last = geom_stream.end();
	while(Data!=Last)
	{
		TouchedGeom* CurrentGeom = (TouchedGeom*)Data;
		if(CurrentGeom->mTGUserData==object)
			return true;

		PxU8* ptr = (PxU8*)Data;
		ptr += GeomSizes[CurrentGeom->mType];
		Data = (const PxU32*)ptr;
	}
	return false;
}

CCTParams::CCTParams() :
	mNonWalkableMode		(PxControllerNonWalkableMode::ePREVENT_CLIMBING),
	mQuatFromUp				(PxQuat(PxIdentity)),
	mUpDirection			(PxVec3(0.0f)),
	mSlopeLimit				(0.0f),
	mContactOffset			(0.0f),
	mStepOffset				(0.0f),
	mInvisibleWallHeight	(0.0f),
	mMaxJumpHeight			(0.0f),
	mMaxEdgeLength2			(0.0f),
	mTessellation			(false),
	mHandleSlope			(false),
	mOverlapRecovery		(false),
	mPreciseSweeps			(true)
{
}

SweepTest::SweepTest() :
	mRenderBuffer		(NULL),
	mRenderFlags		(0),
	mWorldTriangles		(PX_DEBUG_EXP("sweepTestTrigs")),
	mTriangleIndices	(PX_DEBUG_EXP("sweepTestTriangleIndices")),
	mGeomStream			(PX_DEBUG_EXP("sweepTestStream")),
	mSQTimeStamp		(0xffffffff),
	mNbFullUpdates		(0),
	mNbPartialUpdates	(0),
	mNbIterations		(0),
	mFlags				(0)
{
	mCachedTBV.setEmpty();
	mCachedTriIndexIndex	= 0;
	mCachedTriIndex[0] = mCachedTriIndex[1] = mCachedTriIndex[2] = 0;
	mNbCachedStatic = 0;
	mNbCachedT		= 0;

	mTouchedShape		= NULL;
	mTouchedActor		= NULL;
	mTouchedObstacleHandle	= INVALID_OBSTACLE_HANDLE;
	mTouchedPos			= PxVec3(0);
	mTouchedPosShape_Local		= PxVec3(0);
	mTouchedPosShape_World		= PxVec3(0);
	mTouchedPosObstacle_Local	= PxVec3(0);
	mTouchedPosObstacle_World	= PxVec3(0);

//	mVolumeGrowth	= 1.2f;	// Must be >1.0f and not too big
	mVolumeGrowth	= 1.5f;	// Must be >1.0f and not too big
//	mVolumeGrowth	= 2.0f;	// Must be >1.0f and not too big

	mContactNormalDownPass = PxVec3(0.0f);
	mContactNormalSidePass = PxVec3(0.0f);
	mTouchedTriMin = 0.0f;
	mTouchedTriMax = 0.0f;
}


SweepTest::~SweepTest()
{
}

void SweepTest::voidTestCache()
{
	mCachedTBV.setEmpty();
	mTouchedShape = NULL;
	mTouchedActor = NULL;
	mTouchedObstacleHandle	= INVALID_OBSTACLE_HANDLE;
}

void SweepTest::onRelease(const PxBase& observed)
{
	// add additional actor test
	if(observed.getConcreteType()==PxConcreteType:: eRIGID_DYNAMIC || observed.getConcreteType()==PxConcreteType:: eRIGID_STATIC)
	{
		if(mTouchedActor==&observed)
		{
			mTouchedShape = NULL;
			mTouchedActor = NULL;
			return;
		}
	}

	if(observed.getConcreteType()!=PxConcreteType::eSHAPE)
		return;

	if(ParseGeomStream(&observed, mGeomStream))
		mCachedTBV.setEmpty();

	if(mTouchedShape==&observed)
		mTouchedShape = NULL;
}

void SweepTest::onObstacleAdded(ObstacleHandle index, const PxObstacleContext* context, const PxVec3& origin, const PxVec3& unitDir, const PxReal distance )
{
	if(mTouchedObstacleHandle != INVALID_OBSTACLE_HANDLE)
	{
		// check if new obstacle is closer
		const ObstacleContext* obstContext = static_cast<const ObstacleContext*> (context);
		PxRaycastHit obstacleHit;
		const PxObstacle* obst = obstContext->raycastSingle(obstacleHit,index,origin,unitDir,distance);		

		if(obst && (obstacleHit.position.dot(unitDir))<(mTouchedPosObstacle_World.dot(unitDir)))
		{
			PX_ASSERT(obstacleHit.distance<=distance);
			mTouchedObstacleHandle = index;
			if(!gUseLocalSpace)
			{
				mTouchedPos = toVec3(obst->mPos);
			}
			else
			{
				mTouchedPosObstacle_World = obstacleHit.position;
				mTouchedPosObstacle_Local = worldToLocal(*obst, PxExtendedVec3(obstacleHit.position.x,obstacleHit.position.y,obstacleHit.position.z));
			}
		}
	}
}

void SweepTest::onObstacleRemoved(ObstacleHandle index)
{
	if(index == mTouchedObstacleHandle)
	{
		mTouchedObstacleHandle = INVALID_OBSTACLE_HANDLE;
	}
}

void SweepTest::onObstacleUpdated(ObstacleHandle index, const PxObstacleContext* context, const PxVec3& origin, const PxVec3& unitDir, const PxReal distance)
{
	if(index == mTouchedObstacleHandle)
	{
		// check if updated obstacle is still closest
		const ObstacleContext* obstContext = static_cast<const ObstacleContext*> (context);
		PxRaycastHit obstacleHit;
		ObstacleHandle closestHandle = INVALID_OBSTACLE_HANDLE;
		const PxObstacle* obst = obstContext->raycastSingle(obstacleHit,origin,unitDir,distance,closestHandle);

		if(mTouchedObstacleHandle == closestHandle)
			return;

		if(obst)
		{
			PX_ASSERT(obstacleHit.distance<=distance);
			mTouchedObstacleHandle = closestHandle;
			if(!gUseLocalSpace)
			{
				mTouchedPos = toVec3(obst->mPos);
			}
			else
			{
				mTouchedPosObstacle_World = obstacleHit.position;
				mTouchedPosObstacle_Local = worldToLocal(*obst, PxExtendedVec3(obstacleHit.position.x,obstacleHit.position.y,obstacleHit.position.z));
			}
		}
	}
}

void SweepTest::onOriginShift(const PxVec3& shift)
{
	mCachedTBV.minimum -= shift;
	mCachedTBV.maximum -= shift;

	if(mTouchedShape)
	{
		const PxRigidActor* rigidActor = mTouchedActor;
		if(rigidActor->getConcreteType() != PxConcreteType::eRIGID_STATIC)
		{
			mTouchedPosShape_World -= shift;
		}
	}
	else if (mTouchedObstacleHandle != INVALID_OBSTACLE_HANDLE)
	{
		if(!gUseLocalSpace)
		{
			mTouchedPos -= shift;
		}
		else
		{
			mTouchedPosObstacle_World -= shift;
		}
	}

	// adjust cache
	const PxU32* data = mGeomStream.begin();
	const PxU32* last = mGeomStream.end();
	while(data != last)
	{
		TouchedGeom* currentGeom = (TouchedGeom*)data;

		currentGeom->mOffset -= shift;

		PxU8* ptr = (PxU8*)data;
		ptr += GeomSizes[currentGeom->mType];
		data = (const PxU32*)ptr;
	}
}

static PxBounds3 getBounds3(const PxExtendedBounds3& extended)
{
	return PxBounds3(toVec3(extended.minimum), toVec3(extended.maximum));	// LOSS OF ACCURACY
}

// PT: finds both touched CCTs and touched user-defined obstacles
void SweepTest::findTouchedObstacles(const UserObstacles& userObstacles, const PxExtendedBounds3& worldBox)
{
	PxExtendedVec3 Origin;	// Will be TouchedGeom::mOffset
	getCenter(worldBox, Origin);

	{
		const PxU32 nbBoxes = userObstacles.mNbBoxes;
		const PxExtendedBox* boxes = userObstacles.mBoxes;
		const void** boxUserData = userObstacles.mBoxUserData;

		const PxBounds3 singlePrecisionWorldBox = getBounds3(worldBox);

		// Find touched boxes, i.e. other box controllers
		for(PxU32 i=0;i<nbBoxes;i++)
		{
			Gu::Box obb;
			obb.rot		= PxMat33(boxes[i].rot);	// #### PT: TODO: useless conversion here
			obb.center	= toVec3(boxes[i].center);	// LOSS OF ACCURACY
			obb.extents	= boxes[i].extents;

			if(!Gu::intersectOBBAABB(obb, singlePrecisionWorldBox))
				continue;

			TouchedUserBox* UserBox = (TouchedUserBox*)reserve(mGeomStream, sizeof(TouchedUserBox)/sizeof(PxU32));
			UserBox->mType			= TouchedGeomType::eUSER_BOX;
			UserBox->mTGUserData	= boxUserData[i];
			UserBox->mActor			= NULL;
			UserBox->mOffset		= Origin;
			UserBox->mBox			= boxes[i];
		}
	}

	{
		// Find touched capsules, i.e. other capsule controllers
		const PxU32 nbCapsules = userObstacles.mNbCapsules;
		const PxExtendedCapsule* capsules = userObstacles.mCapsules;
		const void** capsuleUserData = userObstacles.mCapsuleUserData;

		PxExtendedVec3 Center;
		PxVec3 Extents;
		getCenter(worldBox, Center);
		getExtents(worldBox, Extents);

		for(PxU32 i=0;i<nbCapsules;i++)
		{
			// PT: do a quick AABB check first, to avoid calling the SDK too much
			const PxF32 r = capsules[i].radius;
			const float capMinx = float(PxMin(capsules[i].p0.x, capsules[i].p1.x));
			const float capMaxx = float(PxMax(capsules[i].p0.x, capsules[i].p1.x));
			if((capMinx - r > worldBox.maximum.x) || (worldBox.minimum.x > capMaxx + r)) continue;

			const float capMiny = float(PxMin(capsules[i].p0.y, capsules[i].p1.y));
			const float capMaxy = float(PxMax(capsules[i].p0.y, capsules[i].p1.y));
			if((capMiny - r > worldBox.maximum.y) || (worldBox.minimum.y > capMaxy + r)) continue;

			const float capMinz = float(PxMin(capsules[i].p0.z, capsules[i].p1.z));
			const float capMaxz = float(PxMax(capsules[i].p0.z, capsules[i].p1.z));
			if((capMinz - r > worldBox.maximum.z) || (worldBox.minimum.z > capMaxz + r)) continue;

			// PT: more accurate capsule-box test. Not strictly necessary but worth doing if available
			const PxReal d2 = Gu::distanceSegmentBoxSquared(toVec3(capsules[i].p0), toVec3(capsules[i].p1), toVec3(Center), Extents, PxMat33(PxIdentity));
			if(d2>r*r)
				continue;

			TouchedUserCapsule* UserCapsule = (TouchedUserCapsule*)reserve(mGeomStream, sizeof(TouchedUserCapsule)/sizeof(PxU32));
			UserCapsule->mType			= TouchedGeomType::eUSER_CAPSULE;
			UserCapsule->mTGUserData	= capsuleUserData[i];
			UserCapsule->mActor			= NULL;
			UserCapsule->mOffset		= Origin;
			UserCapsule->mCapsule		= capsules[i];
		}
	}
}

void SweepTest::updateTouchedGeoms(	const InternalCBData_FindTouchedGeom* userData, const UserObstacles& userObstacles,
									const PxExtendedBounds3& worldBox, const PxControllerFilters& filters)
{
	/*
	- if this is the first iteration (new frame) we have to redo the dynamic objects & the CCTs. The static objects can
	be cached.
	- if this is not, we can cache everything
	*/

	// PT: using "worldBox" instead of "mCachedTBV" seems to produce TTP 6207
//#define DYNAMIC_BOX	worldBox
#define DYNAMIC_BOX	mCachedTBV

	bool NewCachedBox = false;

	CCTFilter filter;
	filter.mFilterData		= filters.mFilterData;
	filter.mFilterCallback	= filters.mFilterCallback;
	filter.mPreFilter		= filters.mFilterFlags & PxQueryFlag::ePREFILTER;
	filter.mPostFilter		= filters.mFilterFlags & PxQueryFlag::ePOSTFILTER;

	// PT: detect changes to the static pruning structure
	bool sceneHasChanged = false;
	{
		const PxU32 currentTimestamp = getSceneTimestamp(userData);
		if(currentTimestamp!=mSQTimeStamp)
		{
			mSQTimeStamp = currentTimestamp;
			sceneHasChanged = true;
		}
	}

	//char str[1000] = {0};
	//sprintf( str, "[Warning] > [updateTouchedGeoms] mcachedtbv: min %0.3f max: %0.3f\n", mCachedTBV.minimum.z, mCachedTBV.maximum.z );
	//OutputDebugStringA( str );

	// If the input box is inside the cached box, nothing to do
	if(gUsePartialUpdates && !sceneHasChanged && worldBox.isInside(mCachedTBV))
	{
		//sprintf( str, "[Warning] CACHEIN%d\n", !!(mFlags & STF_FIRST_UPDATE));
		//OutputDebugStringA( str );

		if(mFlags & STF_FIRST_UPDATE)
		{
			mFlags &= ~STF_FIRST_UPDATE;

			// Only redo the dynamic
			mGeomStream.forceSize_Unsafe(mNbCachedStatic);
			mWorldTriangles.forceSize_Unsafe(mNbCachedT);
			mTriangleIndices.forceSize_Unsafe(mNbCachedT);

			filter.mStaticShapes	= false;
			if(filters.mFilterFlags & PxQueryFlag::eDYNAMIC)
				filter.mDynamicShapes	= true;
			findTouchedGeometry(userData, DYNAMIC_BOX, mWorldTriangles, mTriangleIndices, mGeomStream, filter, mUserParams);

			findTouchedObstacles(userObstacles, DYNAMIC_BOX);

			mNbPartialUpdates++;
		}
	}
	else
	{
		//sprintf( str, "[Warning] > [updateTouchedGeoms] CACHEOUTNS=%d\n", mNbCachedStatic);
		//OutputDebugStringA( str );

		NewCachedBox = true;

		// Cache BV used for the query
		mCachedTBV = worldBox;

		// Grow the volume a bit. The temporal box here doesn't take sliding & collision response into account.
		// In bad cases it is possible to eventually touch a portion of space not covered by this volume. Just
		// in case, we grow the initial volume slightly. Then, additional tests are performed within the loop
		// to make sure the TBV is always correct. There's a tradeoff between the original (artificial) growth
		// of the volume, and the number of TBV recomputations performed at runtime...
		scale(mCachedTBV, mVolumeGrowth);

		// Gather triangles touched by this box. This covers multiple meshes.
		mWorldTriangles.clear();
		mTriangleIndices.clear();
		mGeomStream.clear();
		mCachedTriIndexIndex	= 0;
		mCachedTriIndex[0] = mCachedTriIndex[1] = mCachedTriIndex[2] = 0;

		mNbFullUpdates++;

		if(filters.mFilterFlags & PxQueryFlag::eSTATIC)
			filter.mStaticShapes	= true;
		filter.mDynamicShapes	= false;
		findTouchedGeometry(userData, mCachedTBV, mWorldTriangles, mTriangleIndices, mGeomStream, filter, mUserParams);

		mNbCachedStatic = mGeomStream.size();
		mNbCachedT = mWorldTriangles.size();
		PX_ASSERT(mTriangleIndices.size()==mNbCachedT);

		filter.mStaticShapes	= false;
		if(filters.mFilterFlags & PxQueryFlag::eDYNAMIC)
			filter.mDynamicShapes	= true;
		findTouchedGeometry(userData, DYNAMIC_BOX, mWorldTriangles, mTriangleIndices, mGeomStream, filter, mUserParams);
		// We can't early exit when no tris are touched since we also have to handle the boxes

		findTouchedObstacles(userObstacles, DYNAMIC_BOX);

		mFlags &= ~STF_FIRST_UPDATE;
		//printf("CACHEOUTNSDONE=%d\n", mNbCachedStatic);
	}

	if(mRenderBuffer)
	{
		// PT: worldBox = temporal BV for this frame
		Cm::RenderOutput out(*mRenderBuffer);

		if(mRenderFlags & PxControllerDebugRenderFlag::eTEMPORAL_BV)
		{
			out << gTBVDebugColor;
			out << Cm::DebugBox(getBounds3(worldBox));
		}

		if(mRenderFlags & PxControllerDebugRenderFlag::eCACHED_BV)
		{
			if(NewCachedBox)
				out << (PxU32)(PxDebugColor::eARGB_RED);
			else
				out << (PxU32)(PxDebugColor::eARGB_GREEN);
			out << Cm::DebugBox(getBounds3(mCachedTBV));
		}
	}
}

// This is the generic sweep test for all swept volumes, but not character-controller specific
bool SweepTest::doSweepTest(const InternalCBData_FindTouchedGeom* userData,
							const InternalCBData_OnHit* userHitData,
							const UserObstacles& userObstacles,
							SweptVolume& swept_volume,
							const PxVec3& direction, PxU32 max_iter, PxU32* nb_collisions,
							float min_dist, const PxControllerFilters& filters, SweepPass sweepPass/*, PxVec3 sideMove*/ )
{
	// Early exit when motion is zero. Since the motion is decomposed into several vectors
	// and this function is called for each of them, it actually happens quite often.
	if(direction.isZero())
		return false;

	bool HasMoved = false;
	mFlags &= ~(STF_VALIDATE_TRIANGLE_DOWN|STF_TOUCH_OTHER_CCT|STF_TOUCH_OBSTACLE);
	mTouchedShape = NULL;
	mTouchedActor = NULL;
	mTouchedObstacleHandle	= INVALID_OBSTACLE_HANDLE;

	PxExtendedVec3 CurrentPosition = swept_volume.mCenter;
	PxExtendedVec3 TargetOrientation = swept_volume.mCenter;
	TargetOrientation += direction;

	//char str[1000] = {0};

	PxU32 NbCollisions = 0;
	mNbIterations = 0;
	while(max_iter--)
	{
		mNbIterations++;
		// Compute current direction
		PxVec3 CurrentDirection = TargetOrientation - CurrentPosition;

		//sprintf( str, "[Warning] ----- [dosweep] pass %i iter %i colls %i dirZ {%f,%f,%f} pos {%f,%f,%f} target {%f,%f,%f} \n", sweepPass, max_iter, NbCollisions, CurrentDirection.x, CurrentDirection.y, CurrentDirection.z, CurrentPosition.x, CurrentPosition.y, CurrentPosition.z, TargetOrientation.x, TargetOrientation.y, TargetOrientation.z );
		//OutputDebugStringA( str );

		// Make sure the new TBV is still valid
		{
			// Compute temporal bounding box. We could use a capsule or an OBB instead:
			// - the volume would be smaller
			// - but the query would be slower
			// Overall it's unclear whether it's worth it or not.
			// TODO: optimize this part ?
			PxExtendedBounds3 TemporalBox;
			swept_volume.computeTemporalBox(*this, TemporalBox, CurrentPosition, CurrentDirection);

			// Gather touched geoms
			updateTouchedGeoms(	userData, userObstacles, TemporalBox, filters);
		}

		const float Length = CurrentDirection.magnitude();
		if(Length<=min_dist) //Use <= to handle the case where min_dist is zero.
			break;

		CurrentDirection /= Length;

		// From Quake2: "if velocity is against the original velocity, stop dead to avoid tiny occilations in sloping corners"
		if((CurrentDirection.dot(direction)) <= 0.0f)
			break;

		// From this point, we're going to update the position at least once
		HasMoved = true;

		// Find closest collision
		SweptContact C;
		C.mDistance = Length + mUserParams.mContactOffset;

		if(!CollideGeoms(this, swept_volume, mGeomStream, CurrentPosition, CurrentDirection, C, !mUserParams.mOverlapRecovery))
		{
			// no collision found => move to desired position
			CurrentPosition = TargetOrientation;

			//sprintf( str, "[Warning] >> [dosweep] not-collide!! it: %i, pos {%f,%f,%f} \n", mNbIterations, CurrentPosition.x, CurrentPosition.y, CurrentPosition.z );
			//OutputDebugStringA( str );
			break;
		}

		PX_ASSERT(C.mGeom);	// If we reach this point, we must have touched a geom

		if(mUserParams.mOverlapRecovery && C.mDistance==0.0f)
		{
/*			SweptContact C;
			C.mDistance = 10.0f;
			CollideGeoms(this, swept_volume, mGeomStream, CurrentPosition, -CurrentDirection, C, true);
			CurrentPosition -= CurrentDirection*C.mDistance;

			C.mDistance = 10.0f;
			CollideGeoms(this, swept_volume, mGeomStream, CurrentPosition, CurrentDirection, C, true);
			const float DynSkin = mUserParams.mContactOffset;
			if(C.mDistance>DynSkin)
				CurrentPosition += CurrentDirection*(C.mDistance-DynSkin);*/

			const PxVec3 mtd = computeMTD(this, swept_volume, mGeomStream, CurrentPosition, mUserParams.mContactOffset);

			if(nb_collisions)
				*nb_collisions = NbCollisions;

#ifdef DEBUG_MTD
			printf("MTD FIXUP: %f %f %f\n", mtd.x - swept_volume.mCenter.x, mtd.y - swept_volume.mCenter.y, mtd.z - swept_volume.mCenter.z);
#endif
			swept_volume.mCenter.x = mtd.x;
			swept_volume.mCenter.y = mtd.y;
			swept_volume.mCenter.z = mtd.z;
			return HasMoved;
//			CurrentPosition.x = mtd.x;
//			CurrentPosition.y = mtd.y;
//			CurrentPosition.z = mtd.z;
//			continue;
		}

		bool stopSliding = true;
		if(C.mGeom->mType==TouchedGeomType::eUSER_BOX || C.mGeom->mType==TouchedGeomType::eUSER_CAPSULE)
		{
			if(sweepPass!=SWEEP_PASS_SENSOR)
			{
				// We touched a user object, typically another CCT, but can also be a user-defined obstacle

				// PT: TODO: technically lines marked with (*) shouldn't be here... revisit later

				const PxObstacle* touchedObstacle = NULL;	// (*)
				ObstacleHandle	touchedObstacleHandle = INVALID_OBSTACLE_HANDLE;
	//			if(mValidateCallback)
				{
					PxInternalCBData_OnHit* internalData = (PxInternalCBData_OnHit*)(userHitData);	// (*)
					internalData->touchedObstacle = NULL;											// (*)
					internalData->touchedObstacleHandle = INVALID_OBSTACLE_HANDLE;
					const PxU32 behaviorFlags = userHitCallback(userHitData, C, CurrentDirection, Length);
					stopSliding = (behaviorFlags & PxControllerBehaviorFlag::eCCT_SLIDE)==0;		// (*)
					touchedObstacle = internalData->touchedObstacle;								// (*)
					touchedObstacleHandle = internalData->touchedObstacleHandle;
				}
	//			printf("INTERNAL: %d\n", int(touchedObstacle));

				if(sweepPass==SWEEP_PASS_DOWN)
				{
					// (*)
					if(touchedObstacle)
					{
						mFlags |= STF_TOUCH_OBSTACLE;

						mTouchedObstacleHandle = touchedObstacleHandle;
						if(!gUseLocalSpace)
						{
							mTouchedPos = toVec3(touchedObstacle->mPos);
						}
						else
						{
							mTouchedPosObstacle_World = toVec3(C.mWorldPos);
							mTouchedPosObstacle_Local = worldToLocal(*touchedObstacle, C.mWorldPos);
						}
					}
					else
					{
						mFlags |= STF_TOUCH_OTHER_CCT;
					}
				}
			}
		}
		else
		{
			PxShape* touchedShape = (PxShape*)C.mGeom->mTGUserData;
			PX_ASSERT(touchedShape);
			const PxRigidActor* touchedActor = C.mGeom->mActor;
			PX_ASSERT(touchedActor);

			// We touched a normal object
			if(sweepPass==SWEEP_PASS_DOWN)
			{
				mFlags &= ~(STF_TOUCH_OTHER_CCT|STF_TOUCH_OBSTACLE);

//#ifdef USE_CONTACT_NORMAL_FOR_SLOPE_TEST
				mFlags |= STF_VALIDATE_TRIANGLE_DOWN;
				mContactNormalDownPass = C.mWorldNormal;
/*#else

				// Work out if the shape is attached to a static or dynamic actor.
				// The slope limit is currently only considered when walking on static actors.
				// It is ignored for shapes attached attached to dynamics and kinematics.
				// TODO:  1. should we treat stationary kinematics the same as statics. 
				//		  2. should we treat all kinematics the same as statics.
				//		  3. should we treat no kinematics the same as statics.
				if((touchedActor->getConcreteType() == PxConcreteType::eRIGID_STATIC) && (C.mInternalIndex!=PX_INVALID_U32))
				{
					mFlags |= STF_VALIDATE_TRIANGLE_DOWN;
					const PxTriangle& touchedTri = mWorldTriangles[C.mInternalIndex];
					const PxVec3& upDirection = mUserParams.mUpDirection;
					const float dp0 = touchedTri.verts[0].dot(upDirection);
					const float dp1 = touchedTri.verts[1].dot(upDirection);
					const float dp2 = touchedTri.verts[2].dot(upDirection);
					float dpmin = dp0;
					dpmin = physx::intrinsics::selectMin(dpmin, dp1);
					dpmin = physx::intrinsics::selectMin(dpmin, dp2);
					float dpmax = dp0;
					dpmax = physx::intrinsics::selectMax(dpmax, dp1);
					dpmax = physx::intrinsics::selectMax(dpmax, dp2);

					PxExtendedVec3 cacheCenter;
					getCenter(mCachedTBV, cacheCenter);
					const float offset = upDirection.dot(toVec3(cacheCenter));
					mTouchedTriMin = dpmin + offset;
					mTouchedTriMax = dpmax + offset;

					touchedTri.normal(mContactNormalDownPass);
				}
#endif*/
				// Update touched shape in down pass
				mTouchedShape = touchedShape;
				mTouchedActor = touchedActor;
//				mTouchedPos = getShapeGlobalPose(*touchedShape).p;
				const PxTransform shapeTransform = getShapeGlobalPose(*touchedShape, *touchedActor);
				const PxVec3 worldPos = toVec3(C.mWorldPos);
				mTouchedPosShape_World = worldPos;
				mTouchedPosShape_Local = shapeTransform.transformInv(worldPos);
			}
			else if(sweepPass==SWEEP_PASS_SIDE || sweepPass==SWEEP_PASS_SENSOR)
			{
				if((touchedActor->getConcreteType() == PxConcreteType::eRIGID_STATIC) && (C.mInternalIndex!=PX_INVALID_U32))
				{
					mFlags |= STF_VALIDATE_TRIANGLE_SIDE;

					//sprintf( str, "[Error] >> [dosweep] side pass {%f,%f,%f} \n", C.mWorldNormal.x, C.mWorldNormal.y, C.mWorldNormal.z );
					//OutputDebugStringA( str );

					//mContactNormalSidePass = C.mWorldNormal;
					const PxTriangle& touchedTri = mWorldTriangles[C.mInternalIndex];
					touchedTri.normal(mContactNormalSidePass);
//					printf("%f | %f | %f\n", mContactNormalSidePass.x, mContactNormalSidePass.y, mContactNormalSidePass.z);
				}
			}

			if(sweepPass!=SWEEP_PASS_SENSOR)
//			if(mValidateCallback)
			{
				const PxU32 behaviorFlags = shapeHitCallback(userHitData, C, CurrentDirection, Length);
				stopSliding = (behaviorFlags & PxControllerBehaviorFlag::eCCT_SLIDE)==0;		// (*)
				if( behaviorFlags & PxControllerBehaviorFlag::eCCT_FORCE_UNWALKABLE )
				{
					mFlags |= STF_HIT_NON_WALKABLE | STF_HIT_FORCE_NON_WALKABLE;
				}
				if(behaviorFlags & PxControllerBehaviorFlag::eCCT_PREVENT_CLIMB)
				{
					mFlags |= STF_PREVENT_CLIMB;
				}
			}
		}

/*		if(sweepPass==SWEEP_PASS_DOWN && !stopSliding)
		{
			// Trying to solve the following problem:
			// - by default, the CCT "friction" is infinite, i.e. a CCT will not slide on a slope (this is by design)
			// - this produces bad results when a capsule CCT stands on top of another capsule CCT, without sliding. Visually it looks
			//   like the character is standing on the other character's head, it looks bad. So, here, we would like to let the CCT
			//   slide away, i.e. we don't want friction.
			// So here we simply increase the number of iterations (== let the CCT slide) when the first down collision is with another CCT.
			if(!NbCollisions)
				max_iter += 9;
//				max_iter += 1;
		}*/

		NbCollisions++;
//		mContactPointHeight = (float)C.mWorldPos[mUserParams.mUpDirection];	// UBI
		mContactPointHeight = toVec3(C.mWorldPos).dot(mUserParams.mUpDirection);	// UBI

		const float DynSkin = mUserParams.mContactOffset + ( (sweepPass == 1) ? 0.1f : 0.0f );

		if(C.mDistance>DynSkin/*+0.01f*/)
		{
			CurrentPosition += CurrentDirection*(C.mDistance-DynSkin);

			//sprintf( str, "[Error] >> [dosweep] coll dist %f/%f, dir {%f,%f,%f} pos {%f,%f,%f}\n", C.mDistance, (C.mDistance-DynSkin), CurrentDirection.x, CurrentDirection.y, CurrentDirection.z, CurrentPosition.x, CurrentPosition.y, CurrentPosition.z );
			//OutputDebugStringA( str );
		}

// DE6513
/*		else if(sweepPass==SWEEP_PASS_SIDE)
		{
			// Might be better to do a proper sweep pass here, in the opposite direction
			CurrentPosition += CurrentDirection*(C.mDistance-DynSkin);
		}*/
//~DE6513

		PxVec3 WorldNormal = C.mWorldNormal;
		if((mFlags & STF_WALK_EXPERIMENT) && (mUserParams.mNonWalkableMode!=PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING))
		{
			// Make sure the auto-step doesn't bypass this !
			// PT: cancel out normal compo
//			WorldNormal[mUserParams.mUpDirection]=0.0f;
//			WorldNormal.normalize();
			PxVec3 normalCompo, tangentCompo;
			Ps::decomposeVector(normalCompo, tangentCompo, WorldNormal, mUserParams.mUpDirection);
			WorldNormal = tangentCompo;
			WorldNormal.normalize();
		}

		//sprintf( str, "[Warning] >> [dosweep] response - height %0.3f pos %0.3f vol %0.3f = %f mag %f \n", mContactPointHeight, CurrentPosition.z, swept_volume.mHalfHeight, mContactPointHeight-CurrentPosition.z+swept_volume.mHalfHeight, sideMove.magnitude() );
		//OutputDebugStringA( str );

		const float Bump = 0.0f;	// ### doesn't work when !=0 because of Quake2 hack!
		const float Friction = 1.0f;	
		/*bool res =*/ collisionResponse( TargetOrientation, CurrentPosition, CurrentDirection, WorldNormal, Bump, Friction, (mFlags & STF_NORMALIZE_RESPONSE)!=0, sweepPass==SWEEP_PASS_SIDE || sweepPass==SWEEP_PASS_SENSOR );
	}

	if(nb_collisions)
		*nb_collisions = NbCollisions;

	// Final box position that should be reflected in the graphics engine
	swept_volume.mCenter = CurrentPosition;

	//sprintf( str, "[Warning] >> [dosweep] result it: %i, pos {%f,%f,%f} \n", mNbIterations, CurrentPosition.x, CurrentPosition.y, CurrentPosition.z );
	//OutputDebugStringA( str );

	// If we didn't move, don't update the box position at all (keeping possible lazy-evaluated structures valid)
	return HasMoved;
}

// ### have a return code to tell if we really moved or not

// Using swept code & direct position update (no physics engine)
// This function is the generic character controller logic, valid for all swept volumes
int iters = 1;
PxControllerCollisionFlags SweepTest::moveCharacter(
					const InternalCBData_FindTouchedGeom* userData,
					const InternalCBData_OnHit* userHitData,
					SweptVolume& volume,
					const PxVec3& direction,
					const UserObstacles& userObstacles,
					float min_dist,
					const PxControllerFilters& filters,
					bool constrainedClimbingMode,
					bool standingOnMoving
					 )
{
	bool standingOnMovingUp = standingOnMoving;

	mFlags &= ~STF_HIT_NON_WALKABLE;
	PxControllerCollisionFlags CollisionFlags = PxControllerCollisionFlags(0);
	const PxU32 MaxIter = MAX_ITER;	// 1 for "collide and stop"
	const PxU32 MaxIterSides = MaxIter;
	PxU32 MaxIterDown = ((mFlags & STF_WALK_EXPERIMENT) && !(mFlags & STF_HIT_FORCE_NON_WALKABLE) && mUserParams.mNonWalkableMode==PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING) ? MaxIter : iters;
//	const PxU32 MaxIterDown = 1;

	// check additional up iteration
	if ( filters.mFilterData->word1 & (1 << (16+6)) /* EPFDF_VerticalSlidingIteration */ )
		mFlags |= SFT_UP_SLIDE_ITERATION;

	// ### this causes the artificial gap on top of chars
	float StepOffset = mUserParams.mStepOffset;	// Default step offset can be cancelled in some cases.

	// Save initial height
	const PxVec3& upDirection = mUserParams.mUpDirection;
//	const PxExtended OriginalHeight = volume.mCenter[upDirection];
	const PxVec3 volumeCenter = toVec3(volume.mCenter);
	const PxExtended OriginalHeight = volumeCenter.dot(upDirection);
	const PxExtended OriginalBottomPoint = OriginalHeight - volume.mHalfHeight;	// UBI

	// TEST! Disable auto-step when flying. Not sure this is really useful.
//	if(direction[upDirection]>0.0f)
	const float dir_dot_up = direction.dot(upDirection);
//printf("%f\n", dir_dot_up);
	if(dir_dot_up>0.0f)
	{
		mFlags |= STF_IS_MOVING_UP;

		// PT: this makes it fail on a platform moving up when jumping
		// However if we don't do that a jump when moving up a slope doesn't work anymore!
		// Not doing this also creates jittering when a capsule CCT jumps against another capsule CCT
		if(!standingOnMovingUp)	// PT: if we're standing on something moving up it's safer to do the up motion anyway, even though this won't work well before we add the flag in TA13542
		{
//			static int count=0;	printf("Cancelling step offset... %d\n", count++);
			StepOffset = 0.0f;
		}
	}
	else
	{
		mFlags &= ~STF_IS_MOVING_UP;
	}

	// Decompose motion into 3 independent motions: up, side, down
	// - if the motion is purely down (gravity only), the up part is needed to fight accuracy issues. For example if the
	// character is already touching the geometry a bit, the down sweep test might have troubles. If we first move it above
	// the geometry, the problems disappear.
	// - if the motion is lateral (character moving forward under normal gravity) the decomposition provides the autostep feature
	// - if the motion is purely up, the down part can be skipped

	PxVec3 UpVector(0.0f, 0.0f, 0.0f);
	PxVec3 DownVector(0.0f, 0.0f, 0.0f);

	PxVec3 normal_compo, tangent_compo;
	Ps::decomposeVector(normal_compo, tangent_compo, direction, upDirection);

//	if(direction[upDirection]<0.0f)
	if(dir_dot_up<=0.0f)
//		DownVector[upDirection] = direction[upDirection];
		DownVector = normal_compo;
	else
//		UpVector[upDirection] = direction[upDirection];
		UpVector = normal_compo;

//	PxVec3 SideVector = direction;
//	SideVector[upDirection] = 0.0f;
	PxVec3 SideVector = tangent_compo;

	// If the side motion is zero, i.e. if the character is not really moving, disable auto-step.
	// This is important to prevent the CCT from automatically climbing on small objects that move
	// against it. We should climb over those only if there's a valid side motion from the player.
	const bool sideVectorIsZero = !standingOnMovingUp && Ps::isAlmostZero(SideVector);	// We can't use PxVec3::isZero() safely with arbitrary up vectors
	// #### however if we do this the up pass is disabled, with bad consequences when the CCT is on a dynamic object!!
	// ### this line makes it possible to push other CCTs by jumping on them
//	const bool sideVectorIsZero = false;
//	printf("sideVectorIsZero: %d\n", sideVectorIsZero);

//	if(!SideVector.isZero())
	if(!sideVectorIsZero)
//		UpVector[upDirection] += StepOffset;
		UpVector += upDirection*StepOffset;
//	printf("StepOffset: %f\n", StepOffset);

	// ==========[ Initial volume query ]===========================
	if(1)
	{
		PxExtendedBounds3 TemporalBox;
		volume.computeTemporalBox(*this, TemporalBox, volume.mCenter, direction);

		// Gather touched geoms
		updateTouchedGeoms(	userData, userObstacles, TemporalBox, filters);
	}

	// ==========[ UP PASS ]===========================

	mCachedTriIndexIndex = 0;
	const bool PerformUpPass = true;
	PxU32 NbCollisions=0;

	PxU32 upIters = 1;
	if ( mFlags & SFT_UP_SLIDE_ITERATION )
		++upIters;

	const PxU32 MaxIterUp = Ps::isAlmostZero(SideVector) ? MaxIter : upIters;

	if(PerformUpPass)
	{
//		printf("%f | %f | %f\n", UpVector.x, UpVector.y, UpVector.z);

		// Prevent user callback for up motion. This up displacement is artificial, and only needed for auto-stepping.
		// If we call the user for this, we might eventually apply upward forces to objects resting on top of us, even
		// if we visually don't move. This produces weird-looking motions.
//		mValidateCallback = false;
		// PT: actually I think the previous comment is wrong. It's not only needed for auto-stepping: when the character
		// jumps there's a legit up motion and the lack of callback in that case could need some object can't be pushed
		// by the character's 'head' (for example). So I now think it's better to use the callback all the time, and
		// let users figure out what to do using the available state (like "isMovingUp", etc).
//		mValidateCallback = true;

		// In the walk-experiment we explicitly want to ban any up motions, to avoid characters climbing slopes they shouldn't climb.
		// So let's bypass the whole up pass.
		if(!(mFlags & STF_WALK_EXPERIMENT))
		{
			// ### MaxIter here seems to "solve" the V bug
			if(doSweepTest(userData, userHitData, userObstacles, volume, UpVector, MaxIterUp, &NbCollisions, min_dist, filters, SWEEP_PASS_UP/*, SideVector*/))
			{
				if(NbCollisions)
				{
					CollisionFlags |= PxControllerCollisionFlag::eCOLLISION_UP;

					// Clamp step offset to make sure we don't undo more than what we did
//					PxExtended Delta = volume.mCenter[upDirection] - OriginalHeight;
					PxExtended Delta = toVec3(volume.mCenter).dot(upDirection) - OriginalHeight;
					if(Delta<StepOffset)
					{
						StepOffset=float(Delta);
					}
				}
			}
		}
	}

	// ==========[ SIDE PASS ]===========================

	mCachedTriIndexIndex = 1;
//	mValidateCallback = true;
	const bool PerformSidePass = true;

	mFlags &= ~STF_VALIDATE_TRIANGLE_SIDE;
	if(PerformSidePass)
	{
		NbCollisions=0;

		//printf("BS:%.2f %.2f %.2f NS=%d\n", volume.mCenter.x, volume.mCenter.y, volume.mCenter.z, mNbCachedStatic);
		if(doSweepTest(userData, userHitData, userObstacles, volume, SideVector, MaxIterSides, &NbCollisions, min_dist, filters, SWEEP_PASS_SIDE/*, SideVector*/))
		{
			if (NbCollisions)
				CollisionFlags |= PxControllerCollisionFlag::eCOLLISION_SIDES;
		}
		//printf("AS:%.2f %.2f %.2f NS=%d\n", volume.mCenter.x, volume.mCenter.y, volume.mCenter.z, mNbCachedStatic);

		if(1 && constrainedClimbingMode && volume.getType()==SweptVolumeType::eCAPSULE && !(mFlags & STF_VALIDATE_TRIANGLE_SIDE))
		{
			const float capsuleRadius = static_cast<const SweptCapsule&>(volume).mRadius;

			const float sideM = SideVector.magnitude();
			if(sideM<capsuleRadius)
			{
				const PxVec3 sensor = SideVector.getNormalized() * capsuleRadius;
				
				mFlags &= ~STF_VALIDATE_TRIANGLE_SIDE;
				NbCollisions=0;
				//printf("BS:%.2f %.2f %.2f NS=%d\n", volume.mCenter.x, volume.mCenter.y, volume.mCenter.z, mNbCachedStatic);
				const PxExtendedVec3 saved = volume.mCenter;
				doSweepTest(userData, userHitData, userObstacles, volume, sensor, 1, &NbCollisions, min_dist, filters, SWEEP_PASS_SENSOR/*, SideVector*/);
				volume.mCenter = saved;
			}
		}
	}

	// ==========[ DOWN PASS ]===========================

	mCachedTriIndexIndex = 2;
	const bool PerformDownPass = true;

	if(PerformDownPass)
	{
		NbCollisions=0;

//		if(!SideVector.isZero())	// We disabled that before so we don't have to undo it in that case
		if(!sideVectorIsZero)		// We disabled that before so we don't have to undo it in that case
//			DownVector[upDirection] -= StepOffset;	// Undo our artificial up motion
			DownVector -= upDirection*StepOffset;	// Undo our artificial up motion

		mFlags &= ~STF_VALIDATE_TRIANGLE_DOWN;
		mTouchedShape = NULL;
		mTouchedActor = NULL;
		mTouchedObstacleHandle	= INVALID_OBSTACLE_HANDLE;
		// min_dist actually makes a big difference :(
		// AAARRRGGH: if we get culled because of min_dist here, mValidateTriangle never becomes valid!
		if(doSweepTest(userData, userHitData, userObstacles, volume, DownVector, MaxIterDown, &NbCollisions, min_dist, filters, SWEEP_PASS_DOWN/*, SideVector*/))
		{
			if(NbCollisions)
			{
				if(dir_dot_up<=0.0f)	// PT: fix attempt
					CollisionFlags |= PxControllerCollisionFlag::eCOLLISION_DOWN;

				if(mUserParams.mHandleSlope && !(mFlags & (STF_TOUCH_OTHER_CCT|STF_TOUCH_OBSTACLE)))	// PT: I think the following fix shouldn't be performed when mHandleSlope is false.
				{
					// PT: the following code is responsible for a weird capsule behaviour,
					// when colliding against a highly tesselated terrain:
					// - with a large direction vector, the capsule gets stuck against some part of the terrain
					// - with a slower direction vector (but in the same direction!) the capsule manages to move
					// I will keep that code nonetheless, since it seems to be useful for them.
//printf("%d\n", mFlags & STF_VALIDATE_TRIANGLE_SIDE);
					// constrainedClimbingMode
					if((mFlags & STF_VALIDATE_TRIANGLE_SIDE) && testSlope(mContactNormalSidePass, upDirection, mUserParams.mSlopeLimit))
					{
//printf("%d\n", mFlags & STF_VALIDATE_TRIANGLE_SIDE);
						if(constrainedClimbingMode && mContactPointHeight > OriginalBottomPoint + StepOffset)
						{
							mFlags |= STF_HIT_NON_WALKABLE;
							if(!(mFlags & STF_WALK_EXPERIMENT))
								return CollisionFlags;
	//						printf("Contrained\n");
						}
					}
					//~constrainedClimbingMode
				}
			}
		}
		//printf("AD:%.2f %.2f %.2f NS=%d\n", volume.mCenter.x, volume.mCenter.y, volume.mCenter.z, mNbCachedStatic);
//		printf("%d\n", mTouchOtherCCT);

		// TEST: do another down pass if we're on a non-walkable poly
		// ### kind of works but still not perfect
		// ### could it be because we zero the Y impulse later?
		// ### also check clamped response vectors
//		if(mUserParams.mHandleSlope && mValidateTriangle && direction[upDirection]<0.0f)
//		if(mUserParams.mHandleSlope && !mTouchOtherCCT  && !mTouchObstacle && mValidateTriangle && dir_dot_up<0.0f)
		if(mUserParams.mHandleSlope && !(mFlags & (STF_TOUCH_OTHER_CCT|STF_TOUCH_OBSTACLE)) && (mFlags & STF_VALIDATE_TRIANGLE_DOWN) && dir_dot_up<=0.0f)
		{
			PxVec3 Normal;
		#ifdef USE_CONTACT_NORMAL_FOR_SLOPE_TEST
			Normal = mContactNormalDownPass;
		#else
			//mTouchedTriangle.normal(Normal);
			Normal = mContactNormalDownPass;
		#endif

			const float touchedTriHeight = float(mTouchedTriMax - OriginalBottomPoint);

/*			if(touchedTriHeight>mUserParams.mStepOffset)
			{
				if(constrainedClimbingMode && mContactPointHeight > OriginalBottomPoint + StepOffset)
				{
					mFlags |= STF_HIT_NON_WALKABLE;
					if(!(mFlags & STF_WALK_EXPERIMENT))
						return CollisionFlags;
				}
			}*/

			if( touchedTriHeight>mUserParams.mStepOffset && testSlope(Normal, upDirection, mUserParams.mSlopeLimit))
			{
				mFlags |= STF_HIT_NON_WALKABLE;
				// Early exit if we're going to run this again anyway...
				if(!(mFlags & STF_WALK_EXPERIMENT))
					return CollisionFlags;
		/*		CatchScene()->GetRenderer()->AddLine(mTouchedTriangle.mVerts[0], mTouched.mVerts[1], ARGB_YELLOW);
				CatchScene()->GetRenderer()->AddLine(mTouchedTriangle.mVerts[0], mTouched.mVerts[2], ARGB_YELLOW);
				CatchScene()->GetRenderer()->AddLine(mTouchedTriangle.mVerts[1], mTouched.mVerts[2], ARGB_YELLOW);
		*/

				// ==========[ WALK EXPERIMENT ]===========================

				mFlags |= STF_NORMALIZE_RESPONSE;

				const PxExtended tmp = toVec3(volume.mCenter).dot(upDirection);
				PxExtended Delta = tmp > OriginalHeight ? tmp - OriginalHeight : 0.0f;
//				PxExtended Delta = volume.mCenter[upDirection] > OriginalHeight ? volume.mCenter[upDirection] - OriginalHeight : 0.0f;
				Delta += fabsf(direction.dot(upDirection));
//				Delta += fabsf(direction[upDirection]);
				PxExtended Recover = Delta;

				NbCollisions=0;
				const PxExtended MD = Recover < min_dist ? Recover/float(MaxIter) : min_dist;

				PxVec3 RecoverPoint(0,0,0);
//				RecoverPoint[upDirection]=-float(Recover);
				RecoverPoint = -upDirection*float(Recover);

				// PT: we pass "SWEEP_PASS_UP" for compatibility with previous code, but it's technically wrong (this is a 'down' pass)
				if(doSweepTest(userData, userHitData, userObstacles, volume, RecoverPoint, MaxIter, &NbCollisions, float(MD), filters, SWEEP_PASS_UP/*, SideVector*/))
				{
		//			if(NbCollisions)	CollisionFlags |= COLLISION_Y_DOWN;
					// PT: why did we do this ? Removed for now. It creates a bug (non registered event) when we land on a steep poly.
					// However this might have been needed when we were sliding on those polygons, and we didn't want the land anim to
					// start while we were sliding.
		//			if(NbCollisions)	CollisionFlags &= ~PxControllerFlag::eCOLLISION_DOWN;
				}
				mFlags &= ~STF_NORMALIZE_RESPONSE;
			}
		}
	}

	return CollisionFlags;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This is an interface between NX users and the internal character controller module.

#include "CctInternalStructs.h"
#include "CctBoxController.h"
#include "CctCapsuleController.h"
#include "CctCharacterControllerManager.h"
#include "PxActor.h"
#include "PxScene.h"
#include "PxControllerBehavior.h"
#include "CctObstacleContext.h"

	// PT: we use a local class instead of making "Controller" a PxQueryFilterCallback, since it would waste more memory.
	// Ideally we'd have a C-style callback and a user-data pointer, instead of being forced to create a class.
	class ControllerFilter : public PxQueryFilterCallback
	{
	public:
		PxQueryHitType::Enum	preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags)
		{
			// PT: we want to discard our own internal shapes only
			if(mShapeHashSet->contains(const_cast<PxShape*>(shape)))
				return PxQueryHitType::eNONE;

			// PT: otherwise we revert to the user-callback, if it exists, and if users enabled that call
			if(mUserFilterCallback && (mUserFilterFlags | PxQueryFlag::ePREFILTER))
				return mUserFilterCallback->preFilter(filterData, shape, actor, queryFlags);

			return PxQueryHitType::eBLOCK;
		}

		PxQueryHitType::Enum	postFilter(const PxFilterData& filterData, const PxQueryHit& hit)
		{
			// PT: we may get called if users have asked for such a callback
			if(mUserFilterCallback && (mUserFilterFlags | PxQueryFlag::ePOSTFILTER))
				return mUserFilterCallback->postFilter(filterData, hit);

			PX_ASSERT(0);	// PT: otherwise we shouldn't have been called
			return PxQueryHitType::eNONE;
		}

		Ps::HashSet<PxShape*>*	mShapeHashSet;
		PxQueryFilterCallback*	mUserFilterCallback;
		PxQueryFlags			mUserFilterFlags;
	};


bool Controller::filterTouchedShape(const PxControllerFilters& filters)
{
	PxQueryFlags filterFlags = PxQueryFlag::eDYNAMIC|PxQueryFlag::ePREFILTER;
	PxQueryFilterData filterData(filters.mFilterData ? *filters.mFilterData : PxFilterData(), filterFlags);
	PxHitFlags hitFlags = PxHitFlag::eDISTANCE;

	if(filters.mFilterCallback && (filters.mFilterFlags | PxQueryFlag::ePREFILTER))
	{
		PxQueryHitType::Enum retVal = filters.mFilterCallback->preFilter(filterData.data, mCctModule.mTouchedShape, mCctModule.mTouchedActor, hitFlags);
		if(retVal != PxQueryHitType::eNONE)		
			return true;		
		else
			return false;
	}

	return true;
}

void Controller::findTouchedObject(const PxControllerFilters& filters, const PxObstacleContext* obstacleContext, const PxVec3& upDirection)
{
	PX_ASSERT(!mCctModule.mTouchedShape && (mCctModule.mTouchedObstacleHandle == INVALID_OBSTACLE_HANDLE));

	// PT: the CCT works perfectly on statics without this extra mechanism, so we only raycasts against dynamics.
	// The pre-filter callback is used to filter out our own proxy actor shapes. We need to make sure our own filter
	// doesn't disturb the user-provided filter(s).

	// PT: for starter, if user doesn't want to collide against dynamics, we can skip the whole thing
	if(filters.mFilterFlags & PxQueryFlag::eDYNAMIC)
	{
		ControllerFilter preFilter;
		preFilter.mShapeHashSet			= &mManager->mCCTShapes;
		preFilter.mUserFilterCallback	= filters.mFilterCallback;
		preFilter.mUserFilterFlags		= filters.mFilterFlags;

		// PT: for our own purpose we just want dynamics & pre-filter
		PxQueryFlags filterFlags = PxQueryFlag::eDYNAMIC|PxQueryFlag::ePREFILTER;
		// PT: but we may need the post-filter callback as well if users want it
		if(filters.mFilterFlags & PxQueryFlag::ePOSTFILTER)
			filterFlags |= PxQueryFlag::ePOSTFILTER;

		PxQueryFilterData filterData(filters.mFilterData ? *filters.mFilterData : PxFilterData(), filterFlags);

		const PxF32 probeLength = getHalfHeightInternal();	// Distance to feet
		const PxF32 extra = 0.0f;//probeLength * 0.1f;

		const PxVec3 rayOrigin = toVec3(mPosition);

		PxRaycastBuffer hit;
		hit.block.distance = FLT_MAX;
		if(mScene->raycast(rayOrigin, -upDirection, probeLength+extra, hit, PxHitFlag::eDISTANCE, filterData, &preFilter))
		{
			// copy touching hit to blocking so that the rest of the code works with .block
			hit.block = hit.getAnyHit(0);
			PX_ASSERT(hit.block.shape);
			PX_ASSERT(hit.block.actor);
			PX_ASSERT(hit.block.distance<=probeLength+extra);
			mCctModule.mTouchedShape = hit.block.shape;
			mCctModule.mTouchedActor = hit.block.actor;
//			mCctModule.mTouchedPos = getShapeGlobalPose(*hit.shape).p - upDirection*(probeLength-hit.distance);
			// PT: we only care about the up delta here
			const PxTransform shapeTransform = getShapeGlobalPose(*hit.block.shape, *hit.block.actor);
			mCctModule.mTouchedPosShape_World = PxVec3(0) - upDirection*(probeLength-hit.block.distance);
			mCctModule.mTouchedPosShape_Local = shapeTransform.transformInv(PxVec3(0));

			mPreviousSceneTimestamp = mScene->getTimestamp()-1;	// PT: just make sure cached timestamp is different
		}

		if(obstacleContext)
		{
			const ObstacleContext* obstacles = static_cast<const ObstacleContext*>(obstacleContext);
			PxRaycastHit obstacleHit;
			ObstacleHandle obstacleHandle;
			const PxObstacle* touchedObstacle = obstacles->raycastSingle(obstacleHit, rayOrigin, -upDirection, probeLength+extra, obstacleHandle);
//			printf("Touched raycast obstacle: %d\n", int(touchedObstacle));
			if(touchedObstacle && obstacleHit.distance<hit.block.distance)
			{
				PX_ASSERT(obstacleHit.distance<=probeLength+extra);
				mCctModule.mTouchedObstacleHandle = obstacleHandle;
				if(!gUseLocalSpace)
				{
					mCctModule.mTouchedPos = toVec3(touchedObstacle->mPos) - upDirection*(probeLength-obstacleHit.distance);
				}
				else
				{
					// PT: we only care about the up delta here
					mCctModule.mTouchedPosObstacle_World = PxVec3(0) - upDirection*(probeLength-obstacleHit.distance);
					mCctModule.mTouchedPosObstacle_Local = worldToLocal(*touchedObstacle, PxExtendedVec3(0,0,0));
				}
			}
		}
	}
}

bool Controller::rideOnTouchedObject(SweptVolume& volume, const PxVec3& upDirection, PxVec3& disp, const PxObstacleContext* obstacleContext)
{
	PX_ASSERT(mCctModule.mTouchedShape || (mCctModule.mTouchedObstacleHandle != INVALID_OBSTACLE_HANDLE));

	bool standingOnMoving = false;

	bool canDoUpdate = true;	// Always true on obstacles
	PxU32 behaviorFlags = 0;	// Default on shapes
	PxVec3 delta(0);
	float timeCoeff = 1.0f;

	if(mCctModule.mTouchedShape)
	{
		// PT: riding on a shape

		// PT: it is important to skip this stuff for static meshes,
		// otherwise accuracy issues create bugs like TA14007.
		const PxRigidActor& rigidActor = *mCctModule.mTouchedActor;
		if(rigidActor.getConcreteType()!=PxConcreteType::eRIGID_STATIC)
		{
			// PT: we only do the update when the timestamp has changed, otherwise "delta" will be zero
			// even if the underlying shape is moving.
			const PxU32 timestamp = mScene->getTimestamp();
//			printf("TimeStamp: %d\n", timestamp);
			canDoUpdate = timestamp!=mPreviousSceneTimestamp;
			if(canDoUpdate)
			{
				mPreviousSceneTimestamp = timestamp;

				const float elapsedTime = mGlobalTime - mPreviousGlobalTime;
				mPreviousGlobalTime = mGlobalTime;
				timeCoeff = 1.0f / elapsedTime;

				if(mBehaviorCallback)
					behaviorFlags = mBehaviorCallback->getBehaviorFlags(*mCctModule.mTouchedShape, *mCctModule.mTouchedActor);

//				delta = getShapeGlobalPose(*mCctModule.mTouchedShape).p - mCctModule.mTouchedPos;
				const PxTransform shapeTransform = getShapeGlobalPose(*mCctModule.mTouchedShape, rigidActor);
				const PxVec3 posPreviousFrame = mCctModule.mTouchedPosShape_World;
				const PxVec3 posCurrentFrame = shapeTransform.transform(mCctModule.mTouchedPosShape_Local);
				delta = posCurrentFrame - posPreviousFrame;
			}
		}
	}
	else
	{
		// PT: riding on an obstacle
		behaviorFlags = PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;	// Default on obstacles

		const float elapsedTime = mGlobalTime - mPreviousGlobalTime;
		mPreviousGlobalTime = mGlobalTime;
		timeCoeff = 1.0f / elapsedTime;

		const PxObstacle* touchedObstacle = obstacleContext->getObstacleByHandle(mCctModule.mTouchedObstacleHandle);
		PX_ASSERT(touchedObstacle);

		if(mBehaviorCallback)
			behaviorFlags = mBehaviorCallback->getBehaviorFlags(*touchedObstacle);

		if(!gUseLocalSpace)
		{
			delta = toVec3(touchedObstacle->mPos) - mCctModule.mTouchedPos;
		}
		else
		{
			PxVec3 posPreviousFrame = mCctModule.mTouchedPosObstacle_World;
			PxVec3 posCurrentFrame = localToWorld(*touchedObstacle, mCctModule.mTouchedPosObstacle_Local);
			delta = posCurrentFrame - posPreviousFrame;
		}
	}

	if(canDoUpdate && !(behaviorFlags & PxControllerBehaviorFlag::eCCT_USER_DEFINED_RIDE))
	{
		// PT: amazingly enough even isAlmostZero doesn't solve this one.
		// Moving on a static mesh sometimes produces delta bigger than 1e-6f!
		// This may also explain the drift on some rotating platforms. It looks
		// like this delta computation is not very accurate.
//			standingOnMoving = !delta.isZero();
		standingOnMoving = !Ps::isAlmostZero(delta);
		mCachedStandingOnMoving = standingOnMoving;
//printf("%f %f %f\n", delta.x, delta.y, delta.z);
		if(standingOnMoving)
		{
			const float dir_dot_up = delta.dot(upDirection);
			const bool deltaMovingUp = dir_dot_up>0.0f;

			PxVec3 deltaUpDisp, deltaSideDisp;
			Ps::decomposeVector(deltaUpDisp, deltaSideDisp, delta, upDirection);

			if(deltaMovingUp)
			{
				volume.mCenter.x += deltaUpDisp.x;
				volume.mCenter.y += deltaUpDisp.y;
				volume.mCenter.z += deltaUpDisp.z;
			}
			else
			{
				disp += deltaUpDisp;
			}

			if(behaviorFlags & PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT)
				disp += deltaSideDisp;
		}
//		printf("delta in: %f %f %f (%f)\n", delta.x, delta.y, delta.z, 1.0f/timeCoeff);
		mDeltaXP = delta * timeCoeff;
	}
	else
	{
		standingOnMoving = mCachedStandingOnMoving;
	}
//	mDelta = delta;

	return standingOnMoving;
}

PxControllerCollisionFlags Controller::move(SweptVolume& volume, const PxVec3& originalDisp, PxF32 minDist, PxF32 elapsedTime, const PxControllerFilters& filters, const PxObstacleContext* obstacleContext, bool constrainedClimbingMode)
{
	mGlobalTime += elapsedTime;

	// Init CCT with per-controller settings
	Cm::RenderBuffer* renderBuffer			= mManager->mRenderBuffer;
	const PxU32 debugRenderFlags			= mManager->mDebugRenderingFlags;
	mCctModule.mRenderBuffer				= renderBuffer;
	mCctModule.mRenderFlags					= debugRenderFlags;
	mCctModule.mUserParams					= mUserParams;
	mCctModule.mFlags						|= STF_FIRST_UPDATE;
	mCctModule.mNbFullUpdates				= 0;
	mCctModule.mNbPartialUpdates			= 0;
	mCctModule.mNbIterations				= 0;
	mCctModule.mUserParams.mMaxEdgeLength2	= mManager->mMaxEdgeLength * mManager->mMaxEdgeLength;
	mCctModule.mUserParams.mTessellation	= mManager->mTessellation;
	mCctModule.mUserParams.mOverlapRecovery	= mManager->mOverlapRecovery;
	mCctModule.mUserParams.mPreciseSweeps	= mManager->mPreciseSweeps;

	const PxVec3& upDirection = mUserParams.mUpDirection;

	///////////

	PxVec3 disp = originalDisp + mOverlapRecover;
	mOverlapRecover = PxVec3(0.0f);

	bool standingOnMoving = false;	// PT: whether the CCT is currently standing on a moving object
	//printf("Touched shape: %d\n", int(mCctModule.mTouchedShape));
//standingOnMoving=true;
//	printf("Touched obstacle: %d\n", int(mCctModule.mTouchedObstacle));

	if(mCctModule.mTouchedActor && mCctModule.mTouchedShape)
	{
		PxU32 nbShapes = mCctModule.mTouchedActor->getNbShapes();
		bool found = false;
		for(PxU32 i=0;i<nbShapes;i++)
		{
			PxShape* shape = NULL;
			mCctModule.mTouchedActor->getShapes(&shape, 1, i);
			if(shape==mCctModule.mTouchedShape)
			{
				found = true;
				break;
			}
		}

		if(!found)
		{
			mCctModule.mTouchedActor = NULL;
			mCctModule.mTouchedShape = NULL;
		}
		else
		{
			// check if we are still in the same scene
			if(mCctModule.mTouchedActor->getScene() != mScene)
			{
				mCctModule.mTouchedShape = NULL;
				mCctModule.mTouchedActor = NULL;
			}
			else
			{
				// check if the shape still does have the sq flag
				if(!(mCctModule.mTouchedShape->getFlags() & PxShapeFlag::eSCENE_QUERY_SHAPE))
				{
					mCctModule.mTouchedShape = NULL;
					mCctModule.mTouchedActor = NULL;
				}
				else
				{
					// invoke the CCT filtering for the shape
					if(!filterTouchedShape(filters))
					{
						mCctModule.mTouchedShape = NULL;
						mCctModule.mTouchedActor = NULL;
					}
				}
			}
		}
	}

	if(!mCctModule.mTouchedShape && (mCctModule.mTouchedObstacleHandle == INVALID_OBSTACLE_HANDLE))
		findTouchedObject(filters, obstacleContext, upDirection);

	if(mCctModule.mTouchedShape || (mCctModule.mTouchedObstacleHandle != INVALID_OBSTACLE_HANDLE))
	{
		standingOnMoving = rideOnTouchedObject(volume, upDirection, disp, obstacleContext);
	}
	else	
	{
		mCachedStandingOnMoving = false;
		mDeltaXP = PxVec3(0.0f);
	}
//	printf("standingOnMoving: %d\n", standingOnMoving);

	///////////
	Ps::Array<const void*>&			boxUserData		= mManager->mBoxUserData;
	Ps::Array<PxExtendedBox>&		boxes			= mManager->mBoxes;
	Ps::Array<const void*>&			capsuleUserData	= mManager->mCapsuleUserData;
	Ps::Array<PxExtendedCapsule>&	capsules		= mManager->mCapsules;
	PX_ASSERT(!boxUserData.size());
	PX_ASSERT(!boxes.size());
	PX_ASSERT(!capsuleUserData.size());
	PX_ASSERT(!capsules.size());

	if(1)
	{
		// Experiment - to do better
		const PxU32 nbControllers = mManager->getNbControllers();
		Controller** controllers = mManager->getControllers();

		for(PxU32 i=0;i<nbControllers;i++)
		{
			Controller* currentController = controllers[i];
			if(currentController==this)
				continue;

			bool keepController = true;
			if(filters.mCCTFilterCallback)
				keepController = filters.mCCTFilterCallback->filter(*getPxController(), *currentController->getPxController());

			if(keepController)
			{
				if(currentController->mType==PxControllerShapeType::eBOX)
				{
					// PT: TODO: optimize this
					BoxController* BC = static_cast<BoxController*>(currentController);
					PxExtendedBox obb;
					BC->getOBB(obb);

					boxes.pushBack(obb);

#ifdef REMOVED
					if(renderBuffer /*&& (debugRenderFlags & PxControllerDebugRenderFlag::eOBSTACLES)*/)
					{
						Cm::RenderOutput out(*renderBuffer);
						out << gCCTBoxDebugColor;

						out << PxTransform(toVec3(obb.center), obb.rot);

						out << Cm::DebugBox(obb.extents, true);
					}
#endif
					const size_t code = encodeUserObject(i, USER_OBJECT_CCT);
					boxUserData.pushBack((const void*)code);
				}
				else if(currentController->mType==PxControllerShapeType::eCAPSULE)
				{
					CapsuleController* CC = static_cast<CapsuleController*>(currentController);

					// PT: TODO: optimize this
					PxExtendedCapsule worldCapule;
					CC->getCapsule(worldCapule);
					capsules.pushBack(worldCapule);

					const size_t code = encodeUserObject(i, USER_OBJECT_CCT);
					capsuleUserData.pushBack((const void*)code);
				}
				else PX_ASSERT(0);
			}
		}
	}

	const ObstacleContext* obstacles = NULL;
	if(obstacleContext)
	{
		obstacles = static_cast<const ObstacleContext*>(obstacleContext);

		// PT: TODO: optimize this
		const PxU32 nbExtraBoxes = obstacles->mBoxObstacles.size();
		for(PxU32 i=0;i<nbExtraBoxes;i++)
		{
			const PxBoxObstacle& userBoxObstacle = obstacles->mBoxObstacles[i].mData;

			PxExtendedBox extraBox;
			extraBox.center		= userBoxObstacle.mPos;
			extraBox.extents	= userBoxObstacle.mHalfExtents;
			extraBox.rot		= userBoxObstacle.mRot;
			boxes.pushBack(extraBox);

			const size_t code = encodeUserObject(i, USER_OBJECT_BOX_OBSTACLE);
			boxUserData.pushBack((const void*)code);

			if(renderBuffer && (debugRenderFlags & PxControllerDebugRenderFlag::eOBSTACLES))
			{
				Cm::RenderOutput out(*renderBuffer);
				out << gObstacleDebugColor;

				out << PxTransform(toVec3(userBoxObstacle.mPos), userBoxObstacle.mRot);

				out << Cm::DebugBox(userBoxObstacle.mHalfExtents, true);
			}
		}

		const PxU32 nbExtraCapsules = obstacles->mCapsuleObstacles.size();
		for(PxU32 i=0;i<nbExtraCapsules;i++)
		{
			const PxCapsuleObstacle& userCapsuleObstacle = obstacles->mCapsuleObstacles[i].mData;

			PxExtendedCapsule extraCapsule;
			const PxVec3 capsuleAxis = userCapsuleObstacle.mRot.getBasisVector0() * userCapsuleObstacle.mHalfHeight;
			extraCapsule.p0		= PxExtendedVec3(	userCapsuleObstacle.mPos.x - capsuleAxis.x,
													userCapsuleObstacle.mPos.y - capsuleAxis.y,
													userCapsuleObstacle.mPos.z - capsuleAxis.z);
			extraCapsule.p1		= PxExtendedVec3(	userCapsuleObstacle.mPos.x + capsuleAxis.x,
													userCapsuleObstacle.mPos.y + capsuleAxis.y,
													userCapsuleObstacle.mPos.z + capsuleAxis.z);

			extraCapsule.radius	= userCapsuleObstacle.mRadius;
			capsules.pushBack(extraCapsule);
			const size_t code = encodeUserObject(i, USER_OBJECT_CAPSULE_OBSTACLE);
			capsuleUserData.pushBack((const void*)code);

			if(renderBuffer && (debugRenderFlags & PxControllerDebugRenderFlag::eOBSTACLES))
			{
				Cm::RenderOutput out(*renderBuffer);
				out << gObstacleDebugColor;

				const PxMat33 rotM(userCapsuleObstacle.mRot);

				Cm::Matrix34 absPose;
				absPose.base0	= rotM.column0;
				absPose.base1	= rotM.column1;
				absPose.base2	= rotM.column2;
				absPose.base3	= toVec3(userCapsuleObstacle.mPos);
				out.outputCapsule(userCapsuleObstacle.mRadius, userCapsuleObstacle.mHalfHeight, absPose);
			}
		}
	}


	UserObstacles userObstacles;

	const PxU32 nbBoxes = boxes.size();
	userObstacles.mNbBoxes			= nbBoxes;
	userObstacles.mBoxes			= nbBoxes ? boxes.begin() : NULL;
	userObstacles.mBoxUserData		= nbBoxes ? boxUserData.begin() : NULL;

	const PxU32 nbCapsules = capsules.size();
	userObstacles.mNbCapsules		= nbCapsules;
	userObstacles.mCapsules			= nbCapsules ? capsules.begin() : NULL;
	userObstacles.mCapsuleUserData	= nbCapsules ? capsuleUserData.begin() : NULL;

	PxInternalCBData_OnHit userHitData;
	userHitData.controller	= this;
	userHitData.obstacles	= obstacles;

	///////////

	PxControllerCollisionFlags collisionFlags = PxControllerCollisionFlags(0);

	PxInternalCBData_FindTouchedGeom findGeomData;
	findGeomData.scene				= mScene;
	findGeomData.renderBuffer		= renderBuffer;
	findGeomData.cctShapeHashSet	= &mManager->mCCTShapes;

	mCctModule.mFlags &= ~STF_WALK_EXPERIMENT;
	mCctModule.mFlags &= ~STF_PREVENT_CLIMB;

	PxExtendedVec3 Backup = volume.mCenter;
	collisionFlags = mCctModule.moveCharacter(&findGeomData, &userHitData, volume, disp, userObstacles, minDist, filters, constrainedClimbingMode, standingOnMoving);

	PxExtendedVec3 Backup2 = volume.mCenter;

	if(mCctModule.mFlags & STF_HIT_NON_WALKABLE)
	{
		mCctModule.mFlags |= STF_WALK_EXPERIMENT;
		// A bit slow, but everything else I tried was less convincing...
		volume.mCenter = Backup;

		PxVec3 xpDisp;
		if(mUserParams.mNonWalkableMode==PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING )
		{
			PxVec3 tangent_compo;
			Ps::decomposeVector(xpDisp, tangent_compo, disp, upDirection);
		}
		else xpDisp = disp;

		collisionFlags = mCctModule.moveCharacter(&findGeomData, &userHitData, volume, xpDisp, userObstacles, minDist, filters, constrainedClimbingMode, standingOnMoving);

		mCctModule.mFlags &= ~STF_WALK_EXPERIMENT;

		if(Backup.x == volume.mCenter.x && Backup.y == volume.mCenter.y && Backup.z == volume.mCenter.z )
		{
			if( !( mCctModule.mFlags & STF_HIT_FORCE_NON_WALKABLE ) )
			{
				//WALK EXPERIMENT failed even more than standard pass, takin previous result then
				volume.mCenter = Backup2;

				//char str[1000];
				//sprintf( str, "[Error] force non walkable \n" );
				//OutputDebugStringA( str );
			}
		}
	}

	mCollisionFlags = collisionFlags;

	if( mCctModule.mFlags & STF_PREVENT_CLIMB )
	{
		mPosition.x = volume.mCenter.x;
		mPosition.y = volume.mCenter.y;
		if( mPosition.z > volume.mCenter.z )
		{
			mPosition.z = volume.mCenter.z;
		}
	}
	else
	{
		mPosition = volume.mCenter;
	}

	// Update kinematic actor
	if(mKineActor)
	{
		const PxVec3 delta = Backup - volume.mCenter;
		const PxF32 deltaM2 = delta.magnitudeSquared();
		if(deltaM2!=0.0f)
		{
			PxTransform targetPose = mKineActor->getGlobalPose();
			targetPose.p = toVec3(mPosition);
			targetPose.q = mUserParams.mQuatFromUp;
			mKineActor->setKinematicTarget(targetPose);
		}
	}

	mManager->resetObstaclesBuffers();

	return collisionFlags;
}


PxControllerCollisionFlags BoxController::move(const PxVec3& disp, PxF32 minDist, PxF32 elapsedTime, const PxControllerFilters& filters, const PxObstacleContext* obstacles)
{
	PX_SIMD_GUARD;

	// Create internal swept box
	SweptBox sweptBox;
	sweptBox.mCenter		= mPosition;
	sweptBox.mExtents		= PxVec3(mHalfHeight, mHalfSideExtent, mHalfForwardExtent);
	sweptBox.mHalfHeight	= mHalfHeight;	// UBI
	return Controller::move(sweptBox, disp, minDist, elapsedTime, filters, obstacles, false);
}

PxControllerCollisionFlags CapsuleController::move(const PxVec3& disp, PxF32 minDist, PxF32 elapsedTime, const PxControllerFilters& filters, const PxObstacleContext* obstacles)
{
	PX_SIMD_GUARD;

	// Create internal swept capsule
	SweptCapsule sweptCapsule;
	sweptCapsule.mCenter		= mPosition;
	sweptCapsule.mRadius		= mRadius;
	sweptCapsule.mHeight		= mHeight;
	sweptCapsule.mHalfHeight	= mHeight*0.5f + mRadius;	// UBI
	return Controller::move(sweptCapsule, disp, minDist, elapsedTime, filters, obstacles, mClimbingMode==PxCapsuleClimbingMode::eCONSTRAINED);
}
