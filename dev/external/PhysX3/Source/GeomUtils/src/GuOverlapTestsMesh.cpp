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
#include "GuOverlapTests.h"

#include "CmScaling.h"
#include "PsUtilities.h"

#include "GuIntersectionTriangleBox.h"
#include "GuDistancePointTriangle.h"

#include "PxSphereGeometry.h"
#include "PxBoxGeometry.h"
#include "PxCapsuleGeometry.h"
#include "PxPlaneGeometry.h"
#include "PxConvexMeshGeometry.h"
#include "PxTriangleMeshGeometry.h"

#include "GuCapsule.h"
#include "GuIceSupport.h"
#include "GuBoxConversion.h"

#include "GuTriangleMesh.h"
#include "GuGeomUtilsInternal.h"
#include "GuConvexUtilsInternal.h"
#include "GuSPUHelpers.h"

#include "GuGJKWrapper.h"
#include "GuVecTriangle.h"
#include "GuVecSphere.h"
#include "GuVecCapsule.h"
#include "GuVecConvexHull.h"
#include "GuConvexMesh.h"

using namespace physx;
using namespace Cm;
using namespace Gu;

struct IntersectAnyVsMeshCallback : VolumeColliderTrigCallback
{
	IntersectAnyVsMeshCallback(const Gu::RTreeMidphase& meshModel, const PxMat33& vSkew) :
		mMeshModel			(meshModel),
		mVertexToShapeSkew	(vSkew),
		mAnyHits			(false)
	{
	}
	virtual	~IntersectAnyVsMeshCallback(){}

	const Gu::RTreeMidphase&	mMeshModel;
	const PxMat33&				mVertexToShapeSkew;

	bool						mAnyHits;
	PxF32						mMinDist2;
	PxVec3						mCenter;
	Gu::Capsule					mCapsule;
	Gu::Box						mWorldOBB;
	Cm::Matrix34				mVertexToBox;
private:
	IntersectAnyVsMeshCallback& operator=(const IntersectAnyVsMeshCallback&);
};

enum { eSPHERE, eCAPSULE, eBOX }; // values for tSCB

template<int tSCB>
struct IntersectAnyVsMeshCallback_Any : IntersectAnyVsMeshCallback
{
	IntersectAnyVsMeshCallback_Any(const Gu::RTreeMidphase& meshModel, const PxMat33& vSkew)
		: IntersectAnyVsMeshCallback(meshModel, vSkew)	{}
	virtual ~IntersectAnyVsMeshCallback_Any() {}

	virtual bool processResults(PxU32 count, const PxVec3* verts, const PxU32*, const PxU32*)
	{
		if(mAnyHits)
			return false; // we need first contact only, abort traversal

		PxU32 numTrigs = count;
		while(numTrigs--)
		{
			PxVec3 v0, v1, v2;
			if(tSCB==eBOX)
			{
				v0 = mVertexToBox.transform(verts[numTrigs*3+0]);
				v1 = mVertexToBox.transform(verts[numTrigs*3+1]);
				v2 = mVertexToBox.transform(verts[numTrigs*3+2]);
			}
			else
			{
				v0 = mVertexToShapeSkew * verts[numTrigs*3+0];
				v1 = mVertexToShapeSkew * verts[numTrigs*3+1];
				v2 = mVertexToShapeSkew * verts[numTrigs*3+2];
			}

			bool hit = false;
			if (tSCB==eCAPSULE)
			{
				// (isCapsule && Gu::distanceSegmentTriangleSquared(mCapsule, v0, v1-v0, v2-v0) <= mMinDist2) ||
				Vec3V dummy1, dummy2;
				PxReal dist2 = FStore(Gu::distanceSegmentTriangleSquared(
					V3LoadU(mCapsule.p0), V3LoadU(mCapsule.p1), V3LoadU(v0), V3LoadU(v1), V3LoadU(v2), dummy1, dummy2));
				if (dist2 <= mMinDist2)
				{	
					mAnyHits = true;
					return false; // abort traversal if we are only interested in firstContact
				}
			} else
				hit =
					(tSCB == eSPHERE && Gu::distancePointTriangleSquared(mCenter, v0, v1 - v0, v2 - v0) <= mMinDist2) ||
					(tSCB == eBOX && Gu::intersectTriangleBox(PxVec3(0.0f), mWorldOBB.extents, v0, v1, v2));
			if (hit)
			{
				mAnyHits = true;
				return false; // abort traversal if we are only interested in firstContact
			}
		}

		return true; // no triangles were hit if we are here, continue traversal
	}
};

// PT: TODO: unify this and the ones in PX2ICE.h. Also, figure out what it does!
PX_INLINE Gu::Box transform(const PxMat33& transfo, const Gu::Box& box)
{
	Gu::Box ret;
	PxMat33& obbBasis = ret.rot;

	obbBasis.column0 = transfo * (box.rot.column0 * box.extents.x);
	obbBasis.column1 = transfo * (box.rot.column1 * box.extents.y);
	obbBasis.column2 = transfo * (box.rot.column2 * box.extents.z);

	ret.center = transfo * box.center;
	ret.extents = Ps::optimizeBoundingBox(obbBasis);
	return ret;
}

struct ParamsAny
{
	ParamsAny() : mResult(false)	{}
	bool	mResult;
};
static bool gReportCallbackFirstContact(PxU32 primIndex, void* userData)
{
	PX_UNUSED(primIndex);

	ParamsAny* params = (ParamsAny*)userData;
	params->mResult = true;
	return false;	// PT: i.e. abort query if possible, we got our result
}

template<int tSCB>
static bool intersectAnyVsMesh_NonIdentity(
	IntersectAnyVsMeshCallback& callback,
	const Gu::Sphere* worldSphere, const Gu::Capsule* worldCapsule, const Gu::Box* worldOBB,
	const RTreeMidphaseData& hmd, const PxTransform& meshTransform, const PxMeshScale& scaling,
	const Cm::Matrix34& absPose, const PxMat33& shapeToVertexSkew)
{
	// sphere center in shape space 
	if(tSCB == eSPHERE)
	{
		callback.mCenter = absPose.transformTranspose(worldSphere->center);
		callback.mMinDist2 = worldSphere->radius*worldSphere->radius;

		// sphere bounds in vertex space
		const PxBounds3 bounds = PxBounds3::basisExtent(shapeToVertexSkew * callback.mCenter, shapeToVertexSkew, PxVec3(worldSphere->radius));

		// do conservative opcode query
		Gu::HybridAABBCollider collider;
		collider.SetPrimitiveTests(false);

		Gu::CollisionAABB box;
		box.mCenter = bounds.getCenter();
		box.mExtents = bounds.getExtents();

		// AP: SetPrimitiveTests(false) is set above so we pass false here until we clean up the SetPrimitiveTests flags
		collider.Collide(box, hmd, false, &callback);
		return callback.mAnyHits;
	}

	else if(tSCB == eCAPSULE)
	{
		const PxF32 radius = worldCapsule->radius;
		callback.mMinDist2 = radius * radius;

		//transform world capsule into mesh shape's space
		callback.mCapsule.p0		= absPose.transformTranspose(worldCapsule->p0);
		callback.mCapsule.p1		= absPose.transformTranspose(worldCapsule->p1);
		callback.mCapsule.radius	= radius;

		// make vertex space OBB
		Gu::Box box;
		box.create(callback.mCapsule);
		box = transform(Cm::Matrix34(shapeToVertexSkew), box);

		//do conservative opcode query
		Gu::HybridOBBCollider collider; // no prim tests, report verts
		collider.Collide<0, 1, 0>(box, hmd, &callback, NULL, NULL);
		return callback.mAnyHits;
	}
	
	else if(tSCB == eBOX)
	{
		callback.mWorldOBB = *worldOBB;

		// PT: TODO: eventually refactor with computeVertexSpaceOBB?

			const PxMat33 vertexToWorldSkew_Rot = PxMat33(meshTransform.q) * scaling.toMat33();
			const PxVec3& vertexToWorldSkew_Trans = meshTransform.p;

			PxMat33 worldToVertexSkew_Rot;
			PxVec3 worldToVertexSkew_Trans;
			getInverse(worldToVertexSkew_Rot, worldToVertexSkew_Trans, vertexToWorldSkew_Rot, vertexToWorldSkew_Trans);

			//make vertex space OBB
			const Cm::Matrix34 _worldToVertexSkew(worldToVertexSkew_Rot, worldToVertexSkew_Trans);
			const Gu::Box vertexSpaceBox = transform(_worldToVertexSkew, *worldOBB);

		// Setup the collider
		Gu::HybridOBBCollider collider;

		Cm::Matrix34 tmp;
		buildMatrixFromBox(tmp, *worldOBB);
		const Cm::Matrix34 inv = tmp.getInverseRT();
		const Cm::Matrix34 _vertexToWorldSkew(vertexToWorldSkew_Rot, vertexToWorldSkew_Trans);
		callback.mVertexToBox = inv * _vertexToWorldSkew;

		// vertexSpaceBox is used for rtree, worldOBB is used for accurate testing vs vertices inside of callback
		collider.Collide<0, 1, 0>(vertexSpaceBox, hmd, &callback, NULL, NULL);
		return callback.mAnyHits;
	}
	
	PX_ASSERT(0);
	return false;
}

template<int tSCB>
static bool intersectAnyVsMesh_Any(
	const Gu::Sphere* worldSphere, const Gu::Capsule* worldCapsule, const Gu::Box* worldOBB,
	const Gu::RTreeMidphase& meshModel, const PxTransform& meshTransform, const PxMeshScale& scaling)
{
	PX_ASSERT(tSCB >= eSPHERE && tSCB <= eBOX);
	RTreeMidphaseData hmd;
	meshModel.getRTreeMidphaseData(hmd);

	if(scaling.isIdentity())
	{
		// Convert transform to matrix
		const Cm::Matrix34 vertex2world(meshTransform);

		if(tSCB == eSPHERE)
		{
			Gu::HybridSphereCollider collider;
			collider.SetPrimitiveTests(true);	// PT: enable primitive tests in this case since there's no second pass

			ParamsAny params;
			collider.Collide(gReportCallbackFirstContact, &params, *worldSphere, hmd, NULL, &vertex2world);
			return params.mResult;
		}
		else if(tSCB == eCAPSULE)
		{
			Gu::HybridLSSCollider collider;
			collider.SetPrimitiveTests(true);	// PT: enable primitive tests in this case since there's no second pass

			ParamsAny params;
			collider.Collide(gReportCallbackFirstContact, &params, *worldCapsule, hmd, NULL, &vertex2world);
			return params.mResult;
		}
		else if(tSCB == eBOX)
		{
			Gu::HybridOBBCollider collider; // PT: enable primitive tests in this case since there's no second pass
			// don't report verts

			VolumeColliderAnyHitCallback callback; // prim tests, no verts
			collider.Collide<1, PX_IS_SPU, 0>(*worldOBB, hmd, &callback, NULL, &vertex2world);
			return callback.anyHits;
		}
	}
	else
	{
		const Cm::Matrix34 absPose(meshTransform);
		const PxMat33 vertexToShapeSkew = scaling.toMat33();
		const PxMat33 shapeToVertexSkew = vertexToShapeSkew.getInverse();

		IntersectAnyVsMeshCallback_Any<tSCB> callback(
			meshModel, vertexToShapeSkew);

		return intersectAnyVsMesh_NonIdentity<tSCB>(
			callback, worldSphere, worldCapsule, worldOBB,
			hmd, meshTransform, scaling, absPose, shapeToVertexSkew);
	}
	return false;
}

struct LimitedResults
{
	PxU32*	mResults;
	PxU32	mNbResults;
	PxU32	mNbSkipped;
	PxU32	mMaxResults;
	PxU32	mStartIndex;
	bool	mOverflow;

	PX_FORCE_INLINE	void	reset()
	{
		mNbResults	= 0;
		mNbSkipped	= 0;
		mOverflow	= false;
	}

	PX_FORCE_INLINE	bool	add(PxU32 index)
	{
		if(mNbResults>=mMaxResults)
		{
			mOverflow = true;
			return false;
		}

		if(mNbSkipped>=mStartIndex)
			mResults[mNbResults++] = index;
		else
			mNbSkipped++;

		return true;
	}
};

struct ParamsAll
{
	ParamsAll(LimitedResults& results) : mResults(results)
	{
		results.reset();
	}
	LimitedResults&	mResults;
private:
	ParamsAll& operator=(const ParamsAll&);
};
static bool gReportCallbackAllContacts(PxU32 primIndex, void* userData)
{
	ParamsAll* params = (ParamsAll*)userData;
	return params->mResults.add(primIndex);
}

struct VolumeColliderLimitedResultsCallback : VolumeColliderTrigCallback
{
	LimitedResults&	mResults;
	VolumeColliderLimitedResultsCallback(LimitedResults& results) : mResults(results)
	{
		results.reset();
	}
	virtual ~VolumeColliderLimitedResultsCallback() {}

	virtual bool processResults(PxU32 count, const PxVec3*, const PxU32* buf, const PxU32*)
	{
		while(count--)
			if(!mResults.add(*buf++))
				return false;
		return true;
	}
private:
	VolumeColliderLimitedResultsCallback& operator=(const VolumeColliderLimitedResultsCallback&);
};

template<int tSCB>
struct IntersectAnyVsMeshCallback_All : IntersectAnyVsMeshCallback
{
	LimitedResults&		mResults;

	IntersectAnyVsMeshCallback_All(const Gu::RTreeMidphase& meshModel, const PxMat33& vSkew, LimitedResults& results)
		: IntersectAnyVsMeshCallback(meshModel, vSkew), mResults(results)
	{
		mResults.reset();
	}

	virtual ~IntersectAnyVsMeshCallback_All() {}

	virtual bool processResults(PxU32 count, const PxVec3* verts, const PxU32* indices, const PxU32*)
	{
		PxU32 numTrigs = count;
		while(numTrigs--)
		{
			PxVec3 v0, v1, v2;
			if(tSCB == eBOX)
			{
				v0 = mVertexToBox.transform(verts[numTrigs*3+0]);
				v1 = mVertexToBox.transform(verts[numTrigs*3+1]);
				v2 = mVertexToBox.transform(verts[numTrigs*3+2]);
			}
			else
			{
				v0 = mVertexToShapeSkew * verts[numTrigs*3+0];
				v1 = mVertexToShapeSkew * verts[numTrigs*3+1];
				v2 = mVertexToShapeSkew * verts[numTrigs*3+2];
			}

			if(
				(tSCB == eSPHERE && Gu::distancePointTriangleSquared(mCenter, v0, v1 - v0, v2 - v0) <= mMinDist2) ||
				(tSCB == eCAPSULE && Gu::distanceSegmentTriangleSquared(mCapsule, v0, v1-v0, v2-v0) <= mMinDist2) ||
				(tSCB == eBOX && Gu::intersectTriangleBox(PxVec3(0.0f), mWorldOBB.extents, v0, v1, v2))
			)
			{
				if(!mResults.add(indices[numTrigs]))
					return false;
				mAnyHits = true;
			}
		}
		return true;
	}
};

template<int tSCB>
static PxU32 intersectAnyVsMesh_All(
	const Gu::Sphere* worldSphere, const Gu::Capsule* worldCapsule, const Gu::Box* worldOBB,
	const Gu::RTreeMidphase& meshModel, const PxTransform& meshTransform, const PxMeshScale& scaling,
	LimitedResults& results)
{
	PX_ASSERT(tSCB >= eSPHERE && tSCB <= eBOX);
	RTreeMidphaseData hmd;
	meshModel.getRTreeMidphaseData(hmd);

	if(scaling.isIdentity())
	{
		// Convert transform to matrix
		const Cm::Matrix34 vertex2world(meshTransform);

		if(tSCB == eSPHERE)
		{
			Gu::HybridSphereCollider collider;
			collider.SetPrimitiveTests(true);	// PT: enable primitive tests in this case since there's no second pass

			ParamsAll params(results);
			collider.Collide(gReportCallbackAllContacts, &params, *worldSphere, hmd, NULL, &vertex2world);
			return results.mNbResults;
		}
		else if(tSCB == eCAPSULE)
		{
			Gu::HybridLSSCollider collider;
			collider.SetPrimitiveTests(true);	// PT: enable primitive tests in this case since there's no second pass

			ParamsAll params(results);
			collider.Collide(gReportCallbackAllContacts, &params, *worldCapsule, hmd, NULL, &vertex2world);
			return results.mNbResults;
		}
		else if(tSCB == eBOX)
		{
			Gu::HybridOBBCollider collider;

			VolumeColliderLimitedResultsCallback callback(results);
			collider.Collide<1, PX_IS_SPU, 0>(*worldOBB, hmd, &callback, NULL, &vertex2world); // prim tests, no verts
			return results.mNbResults;
		}
	}
	else
	{
		const Cm::Matrix34 absPose(meshTransform);
		const PxMat33 vertexToShapeSkew = scaling.toMat33();
		const PxMat33 shapeToVertexSkew = vertexToShapeSkew.getInverse();

		IntersectAnyVsMeshCallback_All<tSCB> callback(
			meshModel, vertexToShapeSkew, results);

		intersectAnyVsMesh_NonIdentity<tSCB>(
			callback, worldSphere, worldCapsule, worldOBB,
			hmd, meshTransform, scaling, absPose, shapeToVertexSkew);
		return results.mNbResults;
	}
	return 0;
}

bool Gu::intersectSphereMeshAny(const Gu::Sphere& worldSphere, const Gu::RTreeMidphase& meshModel,
								const PxTransform& meshTransform, const PxMeshScale& scaling)
{
	return intersectAnyVsMesh_Any<eSPHERE>(&worldSphere, NULL, NULL, meshModel, meshTransform, scaling);
}

bool Gu::intersectCapsuleMeshAny(	const Gu::Capsule& worldCapsule, const Gu::RTreeMidphase& meshModel,
									const PxTransform& meshTransform, const PxMeshScale& scaling)
{
	return intersectAnyVsMesh_Any<eCAPSULE>(NULL, &worldCapsule, NULL, meshModel, meshTransform, scaling);
}

bool Gu::intersectBoxMeshAny(	const Gu::Box& worldOBB, const Gu::RTreeMidphase& meshModel,
								const PxTransform& meshTransform, const PxMeshScale& scaling)
{
	return intersectAnyVsMesh_Any<eBOX>(NULL, NULL, &worldOBB, meshModel, meshTransform, scaling);
}

PxU32 Gu::findOverlapSphereMesh(const Gu::Sphere& worldSphere, const Gu::RTreeMidphase& meshModel,
								const PxTransform& meshTransform, const PxMeshScale& scaling,
								PxU32* PX_RESTRICT results, PxU32 maxResults, PxU32 startIndex, bool& overflow)
{
	LimitedResults limitedResults;
	limitedResults.mResults		= results;
	limitedResults.mMaxResults	= maxResults;
	limitedResults.mStartIndex	= startIndex;
	const PxU32 nbResults = intersectAnyVsMesh_All<eSPHERE>(&worldSphere, NULL, NULL, meshModel, meshTransform, scaling, limitedResults);
	overflow = limitedResults.mOverflow;
	return nbResults;
}

PxU32 Gu::findOverlapCapsuleMesh(	const Gu::Capsule& worldCapsule, const Gu::RTreeMidphase& meshModel,
									const PxTransform& meshTransform, const PxMeshScale& scaling,
									PxU32* PX_RESTRICT results, PxU32 maxResults, PxU32 startIndex, bool& overflow)
{
	LimitedResults limitedResults;
	limitedResults.mResults		= results;
	limitedResults.mMaxResults	= maxResults;
	limitedResults.mStartIndex	= startIndex;
	const PxU32 nbResults = intersectAnyVsMesh_All<eCAPSULE>(NULL, &worldCapsule, NULL, meshModel, meshTransform, scaling, limitedResults);
	overflow = limitedResults.mOverflow;
	return nbResults;
}

PxU32 Gu::findOverlapOBBMesh(	const Gu::Box& worldOBB, const Gu::RTreeMidphase& meshModel,
								const PxTransform& meshTransform, const PxMeshScale& scaling,
								PxU32* PX_RESTRICT results, PxU32 maxResults, PxU32 startIndex, bool& overflow)
{
	LimitedResults limitedResults;
	limitedResults.mResults		= results;
	limitedResults.mMaxResults	= maxResults;
	limitedResults.mStartIndex	= startIndex;
	const PxU32 nbResults = intersectAnyVsMesh_All<eBOX>(NULL, NULL, &worldOBB, meshModel, meshTransform, scaling, limitedResults);
	overflow = limitedResults.mOverflow;
	return nbResults;
}

bool Gu::checkOverlapSphere_triangleGeom(const PxGeometry& geom, const PxTransform& pose, const Gu::Sphere& sphere)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eTRIANGLEMESH);
	const PxTriangleMeshGeometry& triGeom = static_cast<const PxTriangleMeshGeometry&>(geom);

	GU_FETCH_MESH_DATA(triGeom);
	return intersectSphereMeshAny(sphere, meshData->mOpcodeModel, pose, triGeom.scale);
}

bool Gu::checkOverlapOBB_triangleGeom(const PxGeometry& geom, const PxTransform& pose, const Gu::Box& box)
{

	PX_ASSERT(geom.getType() == PxGeometryType::eTRIANGLEMESH);
	const PxTriangleMeshGeometry& triGeom = static_cast<const PxTriangleMeshGeometry&>(geom);

	GU_FETCH_MESH_DATA(triGeom);
	return intersectBoxMeshAny(box, meshData->mOpcodeModel, pose, triGeom.scale);
}

bool Gu::checkOverlapCapsule_triangleGeom(const PxGeometry& geom, const PxTransform& pose, const Gu::Capsule& worldCapsule)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eTRIANGLEMESH);
	const PxTriangleMeshGeometry& triGeom = static_cast<const PxTriangleMeshGeometry&>(geom);

	GU_FETCH_MESH_DATA(triGeom);
	return intersectCapsuleMeshAny(worldCapsule, meshData->mOpcodeModel, pose, triGeom.scale);
}

bool GeomOverlapCallback_SphereMesh(GEOM_OVERLAP_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eSPHERE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eTRIANGLEMESH);
	PX_UNUSED(cache);

	const PxSphereGeometry& sphereGeom = static_cast<const PxSphereGeometry&>(geom0);
	const PxTriangleMeshGeometry& meshGeom = static_cast<const PxTriangleMeshGeometry&>(geom1);	

	const Gu::Sphere worldSphere(transform0.p, sphereGeom.radius);

	GU_FETCH_MESH_DATA(meshGeom);
	return Gu::intersectSphereMeshAny(worldSphere, meshData->mOpcodeModel, transform1, meshGeom.scale);
}

bool GeomOverlapCallback_CapsuleMesh(GEOM_OVERLAP_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eCAPSULE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eTRIANGLEMESH);
	PX_UNUSED(cache);

	const PxCapsuleGeometry& capsuleGeom = static_cast<const PxCapsuleGeometry&>(geom0);
	const PxTriangleMeshGeometry& meshGeom = static_cast<const PxTriangleMeshGeometry&>(geom1);

	GU_FETCH_MESH_DATA(meshGeom);

	Gu::Capsule capsule;
	Gu::getCapsule(capsule, capsuleGeom, transform0);

	return Gu::intersectCapsuleMeshAny(capsule, meshData->mOpcodeModel, transform1, meshGeom.scale);
}

bool GeomOverlapCallback_BoxMesh(GEOM_OVERLAP_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eBOX);
	PX_ASSERT(geom1.getType()==PxGeometryType::eTRIANGLEMESH);
	PX_UNUSED(cache);

	const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom0);
	const PxTriangleMeshGeometry& meshGeom = static_cast<const PxTriangleMeshGeometry&>(geom1);

	GU_FETCH_MESH_DATA(meshGeom);

	Gu::Box box;
	buildFrom(box, transform0.p, boxGeom.halfExtents, transform0.q);

	return Gu::intersectBoxMeshAny(box, meshData->mOpcodeModel, transform1, meshGeom.scale);
}

///////////////////////////////////////////////////////////////////////////////
struct ConvexVsMeshOverlapCallback : VolumeColliderTrigCallback
{
	ConvexVsMeshOverlapCallback(const Gu::ConvexMesh& cm, const PxMeshScale& convexScale, const Cm::FastVertex2ShapeScaling& meshScale, const PxTransform& tr0, const PxTransform& tr1) :
			mMeshScale		(meshScale),
			mAnyHit			(false)			
			{
				using namespace Ps::aos;

				const ConvexHullData* hullData = &cm.getHull();

				const Vec3V vScale0 = V3LoadU(convexScale.scale);
				const QuatV vQuat0 = QuatVLoadU(&convexScale.rotation.x);

				mConvex =  Gu::ConvexHullV(hullData, V3Zero(), vScale0, vQuat0);
				aToB = PsMatTransformV(tr0.transformInv(tr1));
				const FloatV convexTolerance = CalculateConvexTolerance(hullData, vScale0);
				mSqTolerance = FMul(convexTolerance, convexTolerance);
			}
	virtual ~ConvexVsMeshOverlapCallback()	{}

	virtual bool processResults(PxU32 count, const PxVec3* verts, const PxU32*, const PxU32*)
	{
		using namespace Ps::aos;
		while(count--)
		{
			/*const Vec3V v0 = mTransform1.transform(Vec3V_From_PxVec3(mMeshScale * verts[0]));
			const Vec3V v1 = mTransform1.transform(Vec3V_From_PxVec3(mMeshScale * verts[1]));
			const Vec3V v2 = mTransform1.transform(Vec3V_From_PxVec3(mMeshScale * verts[2]));
			verts += 3;

			Gu::TriangleV triangle(v0, v1, v2);
			const FloatV contactOffSet = FloatV_From_F32(GJK_CONTACT_OFFSET);
			const FloatV sqContactOffSet = FMul(contactOffSet, contactOffSet);
			const PsMatTransformV aToB(tr.transformInv(sphereTrans)); 
			Vec3V contactA, contactB, normal;
			FloatV sqDist;
			PxGJKStatus status;
			status = Gu::GJK(triangle, mConvex, contactA, contactB, normal, sqDist);
			if(status == GJK_CONTACT || FAllGrtrOrEq(sqContactOffSet, sqDist))
			{
				mAnyHit = true;
				return false;
			}*/

			const Vec3V v0 = V3LoadU(mMeshScale * verts[0]);
			const Vec3V v1 = V3LoadU(mMeshScale * verts[1]);
			const Vec3V v2 = V3LoadU(mMeshScale * verts[2]);
			verts += 3;

			Gu::TriangleV triangle(v0, v1, v2);
			Vec3V contactA, contactB, normal;
			FloatV sqDist;
			PxGJKStatus status;
			status = Gu::GJKRelative(triangle, mConvex, aToB, contactA, contactB, normal, sqDist);
			if(status == GJK_CONTACT || FAllGrtrOrEq(mSqTolerance, sqDist))
			{
				mAnyHit = true;
				return false;
			}
		}
		return true;
	}
	
	Gu::ConvexHullV						mConvex;
	PsMatTransformV						aToB;
	Ps::aos::FloatV						mSqTolerance;//for gjk
	const Cm::FastVertex2ShapeScaling&	mMeshScale;
	bool								mAnyHit;

private:
	ConvexVsMeshOverlapCallback& operator=(const ConvexVsMeshOverlapCallback&);
};

// PT: TODO: refactor bits of this with convex-vs-mesh code
bool GeomOverlapCallback_ConvexMesh(GEOM_OVERLAP_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eCONVEXMESH);
	PX_ASSERT(geom1.getType()==PxGeometryType::eTRIANGLEMESH);
	PX_UNUSED(cache);

	const PxConvexMeshGeometry& convexGeom = static_cast<const PxConvexMeshGeometry&>(geom0);
	const PxTriangleMeshGeometry& meshGeom = static_cast<const PxTriangleMeshGeometry&>(geom1);

	GU_FETCH_CONVEX_DATA(convexGeom);
	GU_FETCH_MESH_DATA(meshGeom);

	const bool idtScaleConvex = convexGeom.scale.isIdentity();
	const bool idtScaleMesh = meshGeom.scale.isIdentity();

	Cm::FastVertex2ShapeScaling convexScaling;
	if(!idtScaleConvex)
		convexScaling.init(convexGeom.scale);

	Cm::FastVertex2ShapeScaling meshScaling;
	if(!idtScaleMesh)
		meshScaling.init(meshGeom.scale);

	const Cm::Matrix34 world0(transform0);
	const Cm::Matrix34 world1(transform1);

	PX_ASSERT(!cm->getLocalBoundsFast().isEmpty());
	PxBounds3 hullAABB = PxBounds3::transformFast(convexScaling.getVertex2ShapeSkew(), cm->getLocalBoundsFast());

	Gu::Box hullOBB;
	computeHullOBB(hullOBB, hullAABB, 0.0f, transform0, world0, world1, meshScaling, idtScaleMesh);

	Gu::RTreeMidphaseData hmd;	// PT: I suppose doing the "conversion" at runtime is fine
	meshData->mOpcodeModel.getRTreeMidphaseData(hmd);

	Gu::HybridOBBCollider collider;

	ConvexVsMeshOverlapCallback cb(*cm, convexGeom.scale, meshScaling, transform0, transform1);
	collider.Collide<1, 1, 0>(hullOBB, hmd, &cb, NULL, NULL); // prim tests, verts
	return cb.mAnyHit;
}
