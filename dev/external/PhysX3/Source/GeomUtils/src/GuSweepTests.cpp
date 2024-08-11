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
#include "PxScene.h" // for PX_MAX_SWEEP_DISTANCE


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

#include "GuSweepSharedTests.h"  

#include "GuCapsule.h"
#include "PsAlloca.h"
#include "./Ice/IceUtils.h"
#include "GuIceSupport.h"
#include "GuBoxConversion.h"
#include "GuGeomUtilsInternal.h"
#include "GuConvexUtilsInternal.h"
#include "GuTriangleMesh.h"
#include "GuSPUHelpers.h"
#include "GuVecSphere.h"
#include "GuVecCapsule.h"
#include "GuVecBox.h"
#include "GuVecTriangle.h"
#include "GuVecConvexHull.h"
#include "GuGJKWrapper.h"

#include "OPC_RayCollider.h" // for inflated raycast


using namespace physx;
using namespace Gu;
using namespace Cm;


bool Gu::sweepCapsule_BoxGeom(GU_CAPSULE_SWEEP_FUNC_PARAMS)
{
	PX_UNUSED(hintFlags);

	using namespace Ps::aos;
	PX_ASSERT(geom.getType() == PxGeometryType::eBOX);
	const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom);

	const FloatV zero = FZero();
	const Vec3V zeroV = V3Zero();
	const Vec3V boxExtents0 = V3LoadU(boxGeom.halfExtents);
	const FloatV dist = FLoad(distance);
	const Vec3V worldDir = V3LoadU(unitDir);

	PxReal _capsuleHalfHeight = 0.0f;
	const PxTransform capTransform = getCapsuleTransform(lss, _capsuleHalfHeight);


	const QuatV q0 = QuatVLoadU(&capTransform.q.x);
	const Vec3V p0 = V3LoadU(&capTransform.p.x);

	const QuatV q1 = QuatVLoadU(&pose.q.x);
	const Vec3V p1 = V3LoadU(&pose.p.x);

	const PsTransformV capPos(p0, q0);
	const PsTransformV boxPos(p1, q1);
	const PsMatTransformV aToB(boxPos.transformInv(capPos));

	const FloatV capsuleHalfHeight = FLoad(_capsuleHalfHeight);
	const FloatV capsuleRadius = FLoad(lss.radius);

	Gu::BoxV box(zeroV, boxExtents0);
	//Gu::CapsuleV capsule(zeroV, V3Scale(V3UnitX(), capsuleHalfHeight), capsuleRadius);
	Gu::CapsuleV capsule(aToB.p, aToB.rotate(V3Scale(V3UnitX(), capsuleHalfHeight)), capsuleRadius);

	const Vec3V dir = boxPos.rotateInv(V3Neg(V3Scale(worldDir, dist)));

	
	FloatV toi = FMax();
	Vec3V closestA, normal;//closestA and normal is in the local space of box
	bool hit  = Gu::GJKLocalRayCast(capsule, box, zero, zeroV, dir, toi, normal, closestA, lss.radius + inflation, false);

	if(hit)
	{

		
		closestA = V3NegScaleSub(normal, capsuleRadius, closestA);
		const Vec3V worldPointA = boxPos.transform(closestA);

		sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;

		if(FAllGrtrOrEq(zero, toi))
		{
			sweepHit.distance	= 0.0f;
			sweepHit.normal		= -unitDir;
			V3StoreU(worldPointA, sweepHit.position);
			//sweepHit.position		= destWorldPointA;	// should be the deepest penetration point
			return true;
		}
	
		const Vec3V destNormal = boxPos.rotate(normal);
		const FloatV length = FMul(dist, toi);
		const Vec3V destWorldPointA = V3ScaleAdd(worldDir, length, worldPointA);
		V3StoreU(destNormal, sweepHit.normal);
		V3StoreU(destWorldPointA, sweepHit.position);
		FStore(length, &sweepHit.distance);

		return true;
	}

	return false;
}

bool Gu::sweepBox_SphereGeom(GU_BOX_SWEEP_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eSPHERE);
	PX_UNUSED(hintFlags);
	const PxSphereGeometry& sphereGeom = static_cast<const PxSphereGeometry&>(geom);

	//const PxReal sphereRadius = sphereGeom.radius + inflation;

	const FloatV zero = FZero();
	const Vec3V zeroV = V3Zero();
	const Vec3V boxExtents = V3LoadU(box.extents);
	const FloatV worldDist = FLoad(distance);
	const Vec3V  unitDirV = V3LoadU(unitDir);

	const FloatV sphereRadius = FLoad(sphereGeom.radius);

	const PxTransform boxWorldPose = box.getTransform();

	const QuatV q0 = QuatVLoadU(&pose.q.x);
	const Vec3V p0 = V3LoadU(&pose.p.x);

	const QuatV q1 = QuatVLoadU(&boxWorldPose.q.x);
	const Vec3V p1 = V3LoadU(&boxWorldPose.p.x);

	const PsTransformV spherePos(p0, q0);
	const PsTransformV boxPos(p1, q1);

	const PsMatTransformV aToB(boxPos.transformInv(spherePos));

	Gu::BoxV boxV(zeroV, boxExtents);
	Gu::CapsuleV capsuleV(aToB.p, sphereRadius);
	//Gu::CapsuleV capsuleV(zeroV, V3Scale(V3UnitX(), capsuleHalfHeight), capsuleRadius);

	//transform into b space
	const Vec3V dir = boxPos.rotateInv(V3Scale(unitDirV, worldDist));

	
	FloatV toi;
	Vec3V closestA, normal;//closestA and normal is in the local space of box
	/*bool hit  = Gu::GJKRelativeRayCast(capsuleV, boxV, aToB, FZero(), zeroV, dir, lambda, normal, closestA, inflation);*/
	bool hit  = Gu::GJKLocalRayCast(capsuleV, boxV, zero, zeroV, dir, toi, normal, closestA, sphereGeom.radius+inflation, false);

	if(hit)
	{
		sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;

		closestA = V3NegScaleSub(normal, sphereRadius, closestA);
		const Vec3V destWorldPointA = boxPos.transform(closestA);
		V3StoreU(destWorldPointA, sweepHit.position);
		
		//initial overlap
		if(FAllGrtrOrEq(zero, toi))
		{
			sweepHit.distance		= 0.0f;
			sweepHit.normal			= -unitDir;
			return true;
		}
	
		const Vec3V destNormal = V3Neg(boxPos.rotate(normal));
		const FloatV length = FMul(worldDist, toi);
		V3StoreU(destNormal, sweepHit.normal);
		FStore(length, &sweepHit.distance);

		return true;
	}

	return false;
}

bool Gu::sweepBox_CapsuleGeom(GU_BOX_SWEEP_FUNC_PARAMS)
{
	using namespace Ps::aos;
	PX_ASSERT(geom.getType() == PxGeometryType::eCAPSULE);
	PX_UNUSED(hintFlags);
	const PxCapsuleGeometry& capsuleGeom = static_cast<const PxCapsuleGeometry&>(geom);

	const FloatV capsuleHalfHeight = FLoad(capsuleGeom.halfHeight);
	const FloatV capsuleRadius = FLoad(capsuleGeom.radius);

	const FloatV zero = FZero();
	const Vec3V zeroV = V3Zero();
	const Vec3V boxExtents = V3LoadU(box.extents);
	const FloatV worldDist = FLoad(distance);
	const Vec3V  unitDirV = V3LoadU(unitDir);

	const PxTransform boxWorldPose = box.getTransform();

	const QuatV q0 = QuatVLoadU(&pose.q.x);
	const Vec3V p0 = V3LoadU(&pose.p.x);

	const QuatV q1 = QuatVLoadU(&boxWorldPose.q.x);
	const Vec3V p1 = V3LoadU(&boxWorldPose.p.x);

	const PsTransformV capPos(p0, q0);
	const PsTransformV boxPos(p1, q1);

	const PsMatTransformV aToB(boxPos.transformInv(capPos));

	Gu::BoxV boxV(zeroV, boxExtents);
	Gu::CapsuleV capsuleV(aToB.p, aToB.rotate(V3Scale(V3UnitX(), capsuleHalfHeight)), capsuleRadius);
	//Gu::CapsuleV capsuleV(zeroV, V3Scale(V3UnitX(), capsuleHalfHeight), capsuleRadius);

	//transform into b space
	const Vec3V dir = boxPos.rotateInv(V3Scale(unitDirV, worldDist));

	
	FloatV toi;
	Vec3V closestA, normal;//closestA and normal is in the local space of box
	/*bool hit  = Gu::GJKRelativeRayCast(capsuleV, boxV, aToB, FZero(), zeroV, dir, lambda, normal, closestA, inflation);*/
	bool hit  = Gu::GJKLocalRayCast(capsuleV, boxV, zero, zeroV, dir, toi, normal, closestA, capsuleGeom.radius+inflation, false);

	if(hit)
	{
		sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
		closestA = V3NegScaleSub(normal, capsuleRadius, closestA);
		const Vec3V destWorldPointA = boxPos.transform(closestA);
		V3StoreU(destWorldPointA, sweepHit.position);
		
		//initial overlap
		if(FAllGrtrOrEq(zero, toi))
		{
			sweepHit.distance		= 0.0f;
			sweepHit.normal			= -unitDir;
			return true;
		}
	
		const Vec3V destNormal = boxPos.rotate(normal);
		const FloatV length = FMul(worldDist, toi);
		V3StoreU(V3Neg(destNormal), sweepHit.normal);
		FStore(length, &sweepHit.distance);

		return true;
	}

	return false;
	
}

bool Gu::sweepBox_BoxGeom(GU_BOX_SWEEP_FUNC_PARAMS)
{

	PX_ASSERT(geom.getType() == PxGeometryType::eBOX);
	const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom);

	const FloatV zero = FZero();
	const Vec3V zeroV = V3Zero();
	const Vec3V boxExtents0 = V3LoadU(boxGeom.halfExtents);
	const Vec3V boxExtents1 = V3LoadU(box.extents);
	const FloatV worldDist = FLoad(distance);
	const Vec3V  unitDirV = V3LoadU(unitDir);

	const PxTransform boxWorldPose = box.getTransform();

	const QuatV q0 = QuatVLoadU(&pose.q.x);
	const Vec3V p0 = V3LoadU(&pose.p.x);

	const QuatV q1 = QuatVLoadU(&boxWorldPose.q.x);
	const Vec3V p1 = V3LoadU(&boxWorldPose.p.x);

#if 0
	pxPrintf("sweepBoxExt=%.5f %.5f %.5f; unitDir=%.5f %.5f %.5f; dist=%.5f; pose=%.5f %.5f %.5f; %.5f %.5f %.5f %.5f;",
		boxGeom.halfExtents.x, boxGeom.halfExtents.y, boxGeom.halfExtents.z,
		unitDir.x, unitDir.y, unitDir.z, distance,
		pose.p.x, pose.p.y, pose.p.z, pose.q.x, pose.q.y, pose.q.z, pose.q.w);
	pxPrintf("boxEx=%.5f %.5f %.5f\n", box.extents.x, box.extents.y, box.extents.z);
	pxPrintf("boxWp=%.5f %.5f %.5f; %.5f %.5f %.5f %.5f\n", boxWorldPose.p.x, boxWorldPose.p.y, boxWorldPose.p.z,
		boxWorldPose.q.x, boxWorldPose.q.y, boxWorldPose.q.z, boxWorldPose.q.w);
#endif

	const PsTransformV boxTrans0(p0, q0);
	const PsTransformV boxTrans1(p1, q1);

	const PsMatTransformV aToB(boxTrans1.transformInv(boxTrans0));

	Gu::BoxV box0(zeroV, boxExtents0);
	Gu::BoxV box1(zeroV, boxExtents1);

	//transform into b space
	const Vec3V dir = boxTrans1.rotateInv(V3Scale(unitDirV, worldDist));

	FloatV toi;
	Vec3V closestA, normal;//closestA and normal is in the local space of box
	bool hit  = Gu::GJKRelativeRayCast(box0, box1, aToB, zero, zeroV, dir, toi, normal, closestA,
		inflation, !(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP));
	//pxPrintf("GJK relative hit=%d\n", hit);
	
	if(hit)
	{
		const Vec3V destWorldPointA = boxTrans1.transform(closestA);
		V3StoreU(destWorldPointA, sweepHit.position);

		sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;

		if(FAllGrtrOrEq(zero, toi))
		{
			sweepHit.distance		= 0.0f;
			sweepHit.normal			= -unitDir;
			return true;
		}

		const Vec3V destNormal = V3Normalize(boxTrans1.rotate(normal));
		const FloatV length = FMul(worldDist, toi);
		V3StoreU(V3Neg(destNormal), sweepHit.normal);
		FStore(length, &sweepHit.distance);
	
		return true;
	}

	return false;
}

// AP: enable this code once duplicated version in GuCCTSweepTests is merged back with this file
#if ENABLE_AFTER_MERGED_WITH_GU_CCT_SWEEP_TESTS
extern PX_FORCE_INLINE int TriBoxSweepTestBoxSpace(
	const PxTriangle& tri, const PxVec3& extents, const PxVec3& dir, const PxVec3& oneOverDir,
	float tmax, float& toi, bool isDoubleSided);
#endif

struct SweepBoxMeshHitCallback : MeshHitCallback<PxRaycastHit>
{		
	const Cm::Matrix34&			meshToBox;
	PxReal						dist;
	FloatV						distV;
	bool						bDoubleSide;		
	const Gu::Box&				box;
	float						boxRadius;
	const PxVec3&				localMotion;
	const PxVec3&				localDir;
	const PxVec3&				worldUnitDir;
	const PxHitFlags&			hintFlags;
	bool						status;
	PxReal						inflation;
	PxTriangle					hitTriangle;
	Vec3V						minClosestA;
	Vec3V						minNormal;
	Vec3V						localDirV;
	Vec3V						localMotionV;
	PxU32						minTriangleIndex;
	PxVec3						oneOverMotion;


	SweepBoxMeshHitCallback(
		CallbackMode::Enum mode, const Cm::Matrix34& _meshToBox, PxReal distance, bool doubleSide, 
		const Gu::Box& box, const PxVec3& localMotion, const PxVec3& _localDir, const PxVec3& unitDir,
		const PxHitFlags& hintFlags, const PxReal _inflation) 
		:	MeshHitCallback<PxRaycastHit>(mode), meshToBox(_meshToBox), dist(distance),
			bDoubleSide(doubleSide), box(box), localMotion(localMotion), localDir(_localDir),
			worldUnitDir(unitDir), hintFlags(hintFlags), status(false), inflation(_inflation)
	{
		//pxPrintf("SweepBoxMeshHitCallback mtb=%x\n", PxU32(&meshToBox));
		localDirV = V3LoadU(localDir);
		localMotionV = V3LoadU(localMotion);
		distV = FLoad(distance);
		oneOverMotion = PxVec3(
			localDir.x!=0.0f ? 1.0f/(localDir.x * distance) : 0.0f,
			localDir.y!=0.0f ? 1.0f/(localDir.y * distance) : 0.0f,
			localDir.z!=0.0f ? 1.0f/(localDir.z * distance) : 0.0f);
	}

	virtual ~SweepBoxMeshHitCallback() {}

	virtual PxAgain processHit( // all reported coords are in mesh local space including hit.position
		const PxRaycastHit& meshHit, const PxVec3& lp0, const PxVec3& lp1, const PxVec3& lp2,
		bool /*indicesAre16bit*/, void* /* indices*/, PxReal& shrinkMaxT)
	{
		//pxPrintf("sweepboxmesh processHit\n");
// AP: enable this code and add a conditional ePRECISE_SWEEP once duplicated version in GuCCTSweepTests is merged back with this file
#if ENABLE_AFTER_MERGED_WITH_GU_CCT_SWEEP_TESTS
		if (!PX_IS_SPU && 0)
		{
			PxTriangle currentTriangle;	// in world space
			currentTriangle.verts[0] = meshToBox.transform(lp0);
			currentTriangle.verts[1] = meshToBox.transform(lp1);
			currentTriangle.verts[2] = meshToBox.transform(lp2);
			PxF32 t = PX_MAX_REAL; // PT: could be better!
			if(TriBoxSweepTestBoxSpace(currentTriangle, box.extents, localMotion, oneOverMotion, dist, t, bDoubleSide))
				if(t <= dist)
				{
					// PT: test if shapes initially overlap
					dist				= t;
					shrinkMaxT			= t;
					minClosestA			= V3LoadU(currentTriangle.verts[0]); // PT: this is arbitrary
					minNormal			= V3LoadU(-worldUnitDir);
					status				= true;
					minTriangleIndex	= meshHit.faceIndex;
					if (t == 0.0f)
						return false; // abort traversal
				}
		} else
#endif
		{
			const Vec3V zeroV = V3Zero();
			const FloatV zero = FZero();
			const BoolV bFalse = BFFFF();
			const Vec3V boxExtents = V3LoadU(box.extents);
			const Vec3V nBoxExtents = V3Neg(boxExtents);
			
			Gu::BoxV boxV(zeroV, boxExtents);

			const bool DoCulling = !bDoubleSide;

			// Move to box space
			const Vec3V triV0 = V3LoadU(meshToBox.transform(lp0));
			const Vec3V triV1 = V3LoadU(meshToBox.transform(lp1));
			const Vec3V triV2 = V3LoadU(meshToBox.transform(lp2));

			//contruct the AABB around the triangles
			const Vec3V triMax = V3Max(triV2, V3Max(triV1, triV0));
			const Vec3V triMin = V3Min(triV2, V3Min(triV1, triV0));

			const BoolV con = BOr(V3IsGrtr(triMin, boxExtents), V3IsGrtr(nBoxExtents, triMax));

			const bool testOverlap = !(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP) && BAllEq(con, bFalse);

			const Vec3V triNormal = V3Cross(V3Sub(triV2, triV1),V3Sub(triV0, triV1)); 

			if((!testOverlap) && DoCulling && FAllGrtrOrEq(V3Dot(triNormal, localMotionV), zero))
				return true;

			Gu::TriangleV triangleV(triV0, triV1, triV2);
	
			FloatV lambda;   
			Vec3V closestA, normal;//closestA and normal is in the local space of convex hull
			//pxPrintf("calling gjk raycast\n");
			bool gjkHit  = Gu::GJKLocalRayCast(triangleV, boxV, zero, zeroV, localMotionV, lambda, normal, closestA, inflation, false); 
			//pxPrintf("done gjk raycast\n");
			if(gjkHit)
			{
				status = true;
				minClosestA = closestA;
				minTriangleIndex = meshHit.faceIndex;
				if(FAllGrtrOrEq(zero, lambda)) // lambda < 0? => initial overlap
				{
					shrinkMaxT = 0.0f;
					distV = zero;
					dist = 0.0f;
					minNormal = V3LoadU(-worldUnitDir);
					return false; // stop traversal
				}

				dist = FStore(lambda)*dist; // shrink dist
				localMotionV = V3Scale(localMotionV, lambda); // shrink localMotion
				distV = FMul(distV,lambda); // shrink distV
				minNormal = normal;
				if (dist < shrinkMaxT) // shrink shrinkMaxT
					shrinkMaxT = dist;
			}
		}

		//pxPrintf("returning from processHits\n");
		return true;
	}

private:
	SweepBoxMeshHitCallback& operator=(const SweepBoxMeshHitCallback&);
};	

bool Gu::sweepBox_MeshGeom(GU_BOX_SWEEP_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eTRIANGLEMESH);
	const PxTriangleMeshGeometry& triMeshGeom = static_cast<const PxTriangleMeshGeometry&>(geom);

	GU_FETCH_MESH_DATA(triMeshGeom);
	const Gu::RTreeMidphase& collisionModel = meshData->mOpcodeModel;

	Cm::Matrix34 meshToWorldSkew;
	PxVec3 sweptAABBMeshSpaceExtents, meshSpaceOrigin, meshSpaceDir;

	// Input sweep params: geom, pose, box, unitDir, distance
	// We convert the origin from world space to mesh local space
	// and convert the box+pose to mesh space AABB
	if(triMeshGeom.scale.isIdentity())
	{
		meshToWorldSkew = Cm::Matrix34(pose);
		PxMat33 worldToMeshRot(pose.q.getConjugate()); // extract rotation matrix from pose.q
		meshSpaceOrigin = worldToMeshRot.transform(box.center - pose.p);
		meshSpaceDir = worldToMeshRot.transform(unitDir) * distance;
		PxMat33 boxToMeshRot = worldToMeshRot * box.rot;
		sweptAABBMeshSpaceExtents = boxToMeshRot.column0.abs() * box.extents.x + 
						   boxToMeshRot.column1.abs() * box.extents.y + 
						   boxToMeshRot.column2.abs() * box.extents.z;
	}
	else
	{
		meshToWorldSkew = pose * triMeshGeom.scale;
		const PxMat33 meshToWorldSkew_Rot = PxMat33(pose.q) * triMeshGeom.scale.toMat33();
		const PxVec3& meshToWorldSkew_Trans = pose.p;

		PxMat33 worldToVertexSkew_Rot;
		PxVec3 worldToVertexSkew_Trans;
		getInverse(worldToVertexSkew_Rot, worldToVertexSkew_Trans, meshToWorldSkew_Rot, meshToWorldSkew_Trans);

		//make vertex space OBB
		Gu::Box vertexSpaceBox1;
		const Cm::Matrix34 worldToVertexSkew(worldToVertexSkew_Rot, worldToVertexSkew_Trans);
		vertexSpaceBox1 = transform(worldToVertexSkew, box);
		// compute swept aabb
		sweptAABBMeshSpaceExtents = vertexSpaceBox1.computeAABBExtent();

		meshSpaceOrigin = worldToVertexSkew.transform(box.center);
		meshSpaceDir = worldToVertexSkew.rotate(unitDir*distance); // also applies scale to direction/length
	}

	sweptAABBMeshSpaceExtents += PxVec3(inflation); // inflate the bounds with additive inflation
	sweptAABBMeshSpaceExtents *= 1.01f; // fatten the bounds to account for numerical discrepancies

	RTreeMidphaseData hmd;
	collisionModel.getRTreeMidphaseData(hmd);

	PxReal dirLen = PxMax(meshSpaceDir.magnitude(), 1e-5f);

	// Move to AABB space
	Cm::Matrix34 WorldToBox;
	computeWorldToBoxMatrix(WorldToBox, box);

	bool status = false;
	const PxU32 meshBothSides = hintFlags & PxHitFlag::eMESH_BOTH_SIDES;
	const bool isDoubleSided = (triMeshGeom.meshFlags & PxMeshGeometryFlag::eDOUBLE_SIDED) || meshBothSides;

	const Cm::Matrix34 meshToBox = WorldToBox*meshToWorldSkew;
	const PxTransform boxTransform = box.getTransform();
	const Vec3V p0 = V3LoadU(&boxTransform.p.x);
	const QuatV q0 = QuatVLoadU(&boxTransform.q.x);
	const PsTransformV boxPos(p0, q0);

	const PxVec3 localDir = WorldToBox.rotate(unitDir);
	const PxVec3 localDirDist = localDir*distance;
	SweepBoxMeshHitCallback callback( // using eMULTIPLE with shrinkMaxT
		CallbackMode::eMULTIPLE, meshToBox, distance, isDoubleSided, box, localDirDist, localDir, unitDir, hintFlags, inflation);

	bool bothSides = triMeshGeom.meshFlags.isSet(PxMeshGeometryFlag::eDOUBLE_SIDED) || hintFlags.isSet(PxHitFlag::eMESH_BOTH_SIDES);
	//pxPrintf("mesh collider begin\n");
	MeshRayCollider<1>::Collide(
		meshSpaceOrigin, meshSpaceDir/dirLen, dirLen, bothSides, hmd, callback, collisionModel.mGeomEpsilon,
		NULL, &sweptAABBMeshSpaceExtents);
	//pxPrintf("mesh collider done\n");

	status = callback.status;
	Vec3V minClosestA = callback.minClosestA;
	Vec3V minNormal = callback.minNormal;
	PxU32 minTriangleIndex = callback.minTriangleIndex;

	if(status)
	{
		sweepHit.faceIndex	= minTriangleIndex;
		sweepHit.distance = callback.dist;
		if(callback.dist==0.0f)
		{
			sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::eNORMAL;
			V3StoreU(minNormal, sweepHit.normal);
		}
		else
		{
			sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
			const Vec3V destNormal = V3Neg(boxPos.rotate(minNormal));
			const Vec3V destWorldPointA = boxPos.transform(minClosestA);
			V3StoreU(destNormal, sweepHit.normal);
			V3StoreU(destWorldPointA, sweepHit.position);
		}
	}
	return status;
}

bool Gu::sweepBox_HeightFieldGeom(GU_BOX_SWEEP_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eHEIGHTFIELD);
	const PxHeightFieldGeometry& heightFieldGeom = static_cast<const PxHeightFieldGeometry&>(geom);

	PxVec3 inflationBound(box.extents.x + inflation, box.extents.y + inflation, box.extents.z + inflation);

	// Compute swept box
	Gu::Box sweptBox;
	computeSweptBox(inflationBound, box.center, box.rot, unitDir, distance, sweptBox);

	//### Temp hack until we can directly collide the OBB against the HF
	const PxTransform sweptBoxTR = sweptBox.getTransform();
	const PxBounds3 bounds = PxBounds3::poseExtent(sweptBoxTR, sweptBox.extents);

	const PxU32 flags = PxHfQueryFlags::eWORLD_SPACE;

	// Move to AABB space
	const PxTransform BoxToWorld = box.getTransform();
	PX_ALIGN_PREFIX(16) PxTransform WorldToBox PX_ALIGN_SUFFIX(16); WorldToBox = BoxToWorld.getInverse();

	const QuatV q1 = QuatVLoadA(&WorldToBox.q.x);
	const Vec3V p1 = V3LoadA(&WorldToBox.p.x);
	const PsTransformV WorldToBoxV(p1, q1);

	const PxVec3 motion = unitDir * distance;
	const PxVec3 localMotion = WorldToBox.rotate(motion);

	Gu::BoxV boxV(V3Zero(), V3LoadU(box.extents));

	sweepHit.distance = PX_MAX_F32;

	struct LocalReport : Gu::EntityReport<PxU32>
	{
		virtual bool onEvent(PxU32 nb, PxU32* indices)
		{
			const FloatV zero=FZero();
			const Vec3V zeroV = V3Zero();
			const Vec3V dir = V3LoadU(localMotion);
			//FloatV minToi = FMax();
			FloatV toi;
			Vec3V closestA, normal;//closestA and normal is in the local space of box

			for(PxU32 i=0; i<nb; i++)
			{
				PxU32 triangleIndex = indices[i];

				PxTriangle currentTriangle;	// in world space
				hfUtil->getTriangle(*pose, currentTriangle, NULL, NULL, triangleIndex, true, true);

				const Vec3V localV0 = V3LoadU(currentTriangle.verts[0]);
				const Vec3V localV1 = V3LoadU(currentTriangle.verts[1]);
				const Vec3V localV2 = V3LoadU(currentTriangle.verts[2]);

				const Vec3V triV0 = WorldToBoxV->transform(localV0);
				const Vec3V triV1 = WorldToBoxV->transform(localV1);
				const Vec3V triV2 = WorldToBoxV->transform(localV2);

				Gu::TriangleV triangle(triV0, triV1, triV2);

				////move triangle to box space
				//const Vec3V localV0 = Vec3V_From_PxVec3(WorldToBox.transform(currentTriangle.verts[0]));
				//const Vec3V localV1 = Vec3V_From_PxVec3(WorldToBox.transform(currentTriangle.verts[1]));
				//const Vec3V localV2 = Vec3V_From_PxVec3(WorldToBox.transform(currentTriangle.verts[2]));

				//Gu::TriangleV triangle(localV0, localV1, localV2);
	
				bool ok  = Gu::GJKLocalRayCast(triangle, *box, zero, zeroV, dir, toi, normal, closestA, inflation, false);

				if(ok)
				{
					if(FAllGrtr(minToi, toi))
					{
						minToi = toi;
						FStore(toi, &hit->distance);
						V3StoreU(normal, hit->normal);
						V3StoreU(closestA, hit->position);
						hit->faceIndex		= triangleIndex;
						status				= true;
					}
				}

			}
#ifdef LAZY_NORMALIZE
			if(status)
			{
				hit->normal.normalize();
				if((hit->normal.dot(localMotion))>0.0f)
					hit->normal = -hit->normal;
			}
#endif
			return true;
		}

		//PxTransform WorldToBox;
		const PsTransformV*	WorldToBoxV;
		const PxTransform* pose;
		Gu::HeightFieldUtil* hfUtil;
		Gu::BoxV* box;
		
		FloatV minToi;
		PxVec3 localMotion;
		PxSweepHit* hit;
		PxReal inflation;
		bool status;
		bool mInitialOverlap;
		bool mInitialOverlapTests;
		bool mInitialOverlapKeep;
	} myReport;

#ifdef __SPU__
	PX_ALIGN_PREFIX(16)  PxU8 heightFieldBuffer[sizeof(Gu::HeightField)+32] PX_ALIGN_SUFFIX(16);
	Gu::HeightField* heightField = memFetchAsync<Gu::HeightField>(heightFieldBuffer, (uintptr_t)(heightFieldGeom.heightField), sizeof(Gu::HeightField), 1);
	memFetchWait(1);

	g_sampleCache.init((uintptr_t)(heightField->getData().samples), heightField->getData().tilesU);

	const_cast<PxHeightFieldGeometry&>(heightFieldGeom).heightField = heightField;
#endif

	Gu::HeightFieldUtil hfUtil(heightFieldGeom);

	myReport.WorldToBoxV = &WorldToBoxV;
	myReport.status = false;
	myReport.mInitialOverlap = false;
	myReport.mInitialOverlapTests = !(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP);
	myReport.mInitialOverlapKeep = true;
	myReport.pose = &pose;
	myReport.hfUtil = &hfUtil;
	myReport.box = &boxV;
	myReport.localMotion = localMotion;
	myReport.hit = &sweepHit;
	myReport.inflation = inflation;
	myReport.minToi = FMax();


	hfUtil.overlapAABBTriangles(pose, bounds, flags, &myReport);

	if(myReport.mInitialOverlap)
	{
		sweepHit.distance	= 0.0f;
		sweepHit.normal		= -unitDir;
		//sweepHit.position		= box.center;	// PT: this is arbitrary
		sweepHit.position = BoxToWorld.transform(sweepHit.position);
		sweepHit.flags		= PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
		return true;
	}

	if(myReport.status)
	{
		sweepHit.distance *= distance;  // stored as toi [0,1] during computation -> scale
		sweepHit.normal = BoxToWorld.rotate(sweepHit.normal);
		sweepHit.position = BoxToWorld.transform(sweepHit.position);
		sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
	}
	return myReport.status;
}


bool Gu::SweepBoxTriangles(	PxU32 nbTris, const PxTriangle* triangles,
							const PxBoxGeometry& boxGeom, const PxTransform& boxPose, const PxVec3& dir, const PxReal length, PxVec3& _hit,
							PxVec3& _normal, float& _d, PxU32& _index, const PxU32* cachedIndex, const PxReal inflation, PxHitFlags hintFlags)
{
	PX_UNUSED(hintFlags);

	if(!nbTris)
		return false;

	Gu::Box box;
	buildFrom1(box, boxPose.p, boxGeom.halfExtents, boxPose.q);

	PxSweepHit sweepHit;
	// Move to AABB space
	Cm::Matrix34 WorldToBox;
	computeWorldToBoxMatrix(WorldToBox, box);

	const PxVec3 localDir = WorldToBox.rotate(dir);
	const PxVec3 localMotion = localDir * length;

	const Vec3V base0 = V3LoadU(WorldToBox.base0);
	const Vec3V base1 = V3LoadU(WorldToBox.base1);
	const Vec3V base2 = V3LoadU(WorldToBox.base2);
	const Mat33V matV(base0, base1, base2);
	const Vec3V p	  = V3LoadU(WorldToBox.base3);
	const PsMatTransformV WorldToBoxV(p, matV);

	const FloatV zero = FZero();
	const Vec3V zeroV = V3Zero();
	const BoolV bTrue = BTTTT();
	const Vec3V boxExtents = V3LoadU(box.extents);
	const Vec3V boxDir = V3LoadU(localDir);
	const FloatV inflationV = FLoad(inflation);
	const Vec3V absBoxDir = V3Abs(boxDir);
	const FloatV boxRadiusV = FAdd(V3Dot(absBoxDir, boxExtents), inflationV);
	Gu::BoxV boxV(zeroV, boxExtents);

#ifdef PX_DEBUG
	PxU32 totalTestsExpected = nbTris;
	PxU32 totalTestsReal = 0;
	PX_UNUSED(totalTestsExpected);
	PX_UNUSED(totalTestsReal);
#endif

	Vec3V boxLocalMotion = V3LoadU(localMotion);
	Vec3V minClosestA = zeroV, minNormal = zeroV;
	PxU32 minTriangleIndex = 0;
	FloatV dist = FLoad(length);

	const QuatV q0 = QuatVLoadU(&boxPose.q.x);
	const Vec3V p0 = V3LoadU(&boxPose.p.x);
	const PsTransformV boxPos(p0, q0);

	bool status = false;

	const PxU32 idx = cachedIndex ? *cachedIndex : 0;

	for(PxU32 ii=0;ii<nbTris;ii++)
	{
		const PxU32 triangleIndex = getTriangleIndex(ii, idx);

		//currentTriangle = triangles[triangleIndex];

		const Vec3V localV0 =  V3LoadU(triangles[triangleIndex].verts[0]);
		const Vec3V localV1 =  V3LoadU(triangles[triangleIndex].verts[1]);
		const Vec3V localV2 =  V3LoadU(triangles[triangleIndex].verts[2]);

		const Vec3V triV0 = WorldToBoxV.transform(localV0);
		const Vec3V triV1 = WorldToBoxV.transform(localV1);
		const Vec3V triV2 = WorldToBoxV.transform(localV2);

		/*const Vec3V triV0 = Vec3V_From_PxVec3(WorldToBox.transform(triangles[triangleIndex].verts[0]));
		const Vec3V triV1 = Vec3V_From_PxVec3(WorldToBox.transform(triangles[triangleIndex].verts[1]));
		const Vec3V triV2 = Vec3V_From_PxVec3(WorldToBox.transform(triangles[triangleIndex].verts[2]));*/

		
		const Vec3V triNormal = V3Cross(V3Sub(triV2, triV1),V3Sub(triV0, triV1)); 

		if(FAllGrtrOrEq(V3Dot(triNormal, boxLocalMotion), zero))
			continue;

		const FloatV dp0 = V3Dot(triV0, boxDir);
		const FloatV dp1 = V3Dot(triV1, boxDir);
		const FloatV dp2 = V3Dot(triV2, boxDir);
		
		const FloatV dp = FMin(dp0, FMin(dp1, dp2));

		const Vec3V dpV = V3Merge(dp0, dp1, dp2);

		const FloatV temp1 = FAdd(boxRadiusV, dist);
		const BoolV con0 = FIsGrtr(dp, temp1);
		const BoolV con1 = V3IsGrtr(zeroV, dpV);

		if(BAllEq(BOr(con0, con1), bTrue))
			continue;

#ifdef PX_DEBUG
		totalTestsReal++;
#endif

		Gu::TriangleV triangleV(triV0, triV1, triV2);
		
		FloatV lambda;   
		Vec3V closestA, normal;//closestA and normal is in the local space of convex hull
		//bool hit  = Gu::GJKLocalRayCast(triangleV, boxV, zero, zeroV, boxLocalMotion, lambda, normal, closestA, 0.02f, false); 
		bool hit  = Gu::GJKLocalRayCast(triangleV, boxV, zero, zeroV, boxLocalMotion, lambda, normal, closestA, inflation, false); 
		
		if(hit)
		{
			//hitCount++;
		
			if(FAllGrtrOrEq(zero, lambda))
			{
				/*const Vec3V normalizedTriNormal = V3Normalize(triNormal);
				const FloatV proj2 = V3Dot(normalizedTriNormal, boxDir);*/
//				const FloatV proj = V3Dot(normal, boxDir);
//				if(FAllGrtr(proj, FLoad(0.05f)))
				//if(FAllGrtr(FloatV_From_F32(-0.05f), proj))
				{
					//const Vec3V destNormal = V3Neg(boxPos.rotate(normalizedTriNormal));
					const Vec3V destNormal = V3Neg(V3Normalize(boxPos.rotate(normal)));
					const Vec3V destWorldPointA = boxPos.transform(closestA);
					_d			= 0.0f;
					//_normal		= -dir;
					_index		= triangleIndex;
					V3StoreU(destNormal,	_normal);
					V3StoreU(destWorldPointA, _hit);
					return true;
				}
				continue;
			}

			dist = FMul(dist,lambda);
			boxLocalMotion = V3Scale(boxDir, dist);  
			minClosestA = closestA;
			minNormal = normal;
			minTriangleIndex = triangleIndex;
			status = true;
		}
	}

	if(status)
	{
		_index	= minTriangleIndex;
		const Vec3V destNormal = V3Neg(V3Normalize(boxPos.rotate(minNormal)));
		const Vec3V destWorldPointA = boxPos.transform(minClosestA);
		V3StoreU(destNormal, _normal);
		V3StoreU(destWorldPointA, _hit);
		FStore(dist, &_d);
		return true;
	}

	return false;
}

const Gu::SweepCapsuleFunc* Gu::GetSweepCapsuleMap()
{
	return &Gu::gSweepCapsuleMap[0];
}

const Gu::SweepCapsuleFunc Gu::gSweepCapsuleMap[7] = 
{
	sweepCapsule_SphereGeom,
	sweepCapsule_PlaneGeom,
	sweepCapsule_CapsuleGeom,
	sweepCapsule_BoxGeom,
	sweepCapsule_ConvexGeom,
	sweepCapsule_MeshGeom,
	sweepCapsule_HeightFieldGeom
};

const Gu::SweepBoxFunc* Gu::GetSweepBoxMap()
{
	return &Gu::gSweepBoxMap[0];
}

const Gu::SweepBoxFunc Gu::gSweepBoxMap[7] = 
{
	sweepBox_SphereGeom,
	sweepBox_PlaneGeom,
	sweepBox_CapsuleGeom,
	sweepBox_BoxGeom,
	sweepBox_ConvexGeom,
	sweepBox_MeshGeom,
	sweepBox_HeightFieldGeom
};