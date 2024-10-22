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

#include "GuMTD.h"
#include "GuSphere.h"
#include "GuCapsule.h"
#include "GuDistanceSegmentSegment.h"
#include "GuDistanceSegmentBox.h"
#include "PsUtilities.h"
#include "PsMathUtils.h"
#include "GuGJKWrapper.h"
#include "GuGJKPenetrationWrapper.h"
#include "GuEPAPenetrationWrapper.h"
#include "GuVecBox.h"
#include "GuVecShrunkBox.h"
#include "GuVecCapsule.h"
#include "GuVecShrunkConvexHull.h"
#include "GuGeomUtilsInternal.h"
#include "PsVecTransform.h"
#include "GuContactMethodImpl.h"
#include "GuContactBuffer.h"
#include "GuBoxConversion.h"

using namespace physx;
using namespace Gu;

static PX_FORCE_INLINE PxF32 manualNormalize(PxVec3& mtd, const PxVec3& normal, PxReal lenSq)
{
	const PxF32 len = PxSqrt(lenSq);

	// We do a *manual* normalization to check for singularity condition
	if(lenSq < 1e-6f)
		mtd = PxVec3(1.0f, 0.0f, 0.0f);			// PT: zero normal => pick up random one
	else
		mtd = normal * 1.0f / len;

	return len;
}

static PX_FORCE_INLINE float validateDepth(float depth)
{
	// PT: penetration depth must always be positive or null, but FPU accuracy being what it is, we sometimes
	// end up with very small, epsilon-sized negative depths. We clamp those to zero, since they don't indicate
	// real bugs in the MTD functions. However anything larger than epsilon is wrong, and caught with an assert.
	const float epsilon = 1.e-3f;

	//ML: because we are shrunking the shape in this moment, so the depth might be larger than eps, this condition is no longer valid
	//PX_ASSERT(depth>=-epsilon);
	PX_UNUSED(epsilon);
	return PxMax(depth, 0.0f);
}

///////////////////////////////////////////////////////////////////////////////

bool Gu::computeMTD_SphereSphere(PxVec3& mtd, PxF32& depth, const Sphere& sphere0, const Sphere& sphere1)
{
	const PxVec3 delta = sphere0.center - sphere1.center;
	const PxReal d2 = delta.magnitudeSquared();
	const PxReal radiusSum = sphere0.radius + sphere1.radius;

	if(d2 > radiusSum*radiusSum)
		return false;

	const PxF32 d = manualNormalize(mtd, delta, d2);

	depth = validateDepth(radiusSum - d);
	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Gu::computeMTD_SphereCapsule(PxVec3& mtd, PxF32& depth, const Sphere& sphere, const Capsule& capsule)
{
	const PxReal radiusSum = sphere.radius + capsule.radius;

	PxReal u;
	const PxReal d2 = distancePointSegmentSquared(capsule, sphere.center, &u);

	if(d2 > radiusSum*radiusSum)
		return false;

	const PxVec3 normal = sphere.center - capsule.getPointAt(u);
	
	const PxReal lenSq = normal.magnitudeSquared();
	const PxF32 d = manualNormalize(mtd, normal, lenSq);

	depth = validateDepth(radiusSum - d);
	return true;
}

///////////////////////////////////////////////////////////////////////////////

// ### TODO: refactor this

//This version is ported 1:1 from novodex
static PX_FORCE_INLINE bool ContactSphereBox(const PxVec3& sphereOrigin, 
							 PxReal sphereRadius,
							 const PxVec3& boxExtents,
//							 const PxcCachedTransforms& boxCacheTransform, 
							 const PxTransform& boxTransform, 
							 PxVec3& point, 
							 PxVec3& normal, 
							 PxReal& separation, 
							 PxReal contactDistance)
{
//	const PxTransform& boxTransform = boxCacheTransform.getShapeToWorld();

	//returns true on contact
	const PxVec3 delta = sphereOrigin - boxTransform.p; // s1.center - s2.center;
	PxVec3 dRot = boxTransform.rotateInv(delta); //transform delta into OBB body coords.

	//check if delta is outside ABB - and clip the vector to the ABB.
	bool outside = false;

	if (dRot.x < -boxExtents.x)
	{ 
		outside = true; 
		dRot.x = -boxExtents.x;
	}
	else if (dRot.x >  boxExtents.x)
	{ 
		outside = true; 
		dRot.x = boxExtents.x;
	}

	if (dRot.y < -boxExtents.y)
	{ 
		outside = true; 
		dRot.y = -boxExtents.y;
	}
	else if (dRot.y >  boxExtents.y)
	{ 
		outside = true; 
		dRot.y = boxExtents.y;
	}

	if (dRot.z < -boxExtents.z)
	{ 
		outside = true; 
		dRot.z =-boxExtents.z;
	}
	else if (dRot.z >  boxExtents.z)
	{ 
		outside = true; 
		dRot.z = boxExtents.z;
	}

	if (outside) //if clipping was done, sphere center is outside of box.
	{
		point = boxTransform.rotate(dRot); //get clipped delta back in world coords.
		normal = delta - point; //what we clipped away.	
		const PxReal lenSquared = normal.magnitudeSquared();
		const PxReal inflatedDist = sphereRadius + contactDistance;
		if (lenSquared > inflatedDist * inflatedDist) 
			return false;	//disjoint

		//normalize to make it into the normal:
		separation = PxRecipSqrt(lenSquared);
		normal *= separation;	
		separation *= lenSquared;
		//any plane that touches the sphere is tangential, so a vector from contact point to sphere center defines normal.
		//we could also use point here, which has same direction.
		//this is either a faceFace or a vertexFace contact depending on whether the box's face or vertex collides, but we did not distinguish. 
		//We'll just use vertex face for now, this info isn't really being used anyway.
		//contact point is point on surface of cube closest to sphere center.
		point += boxTransform.p;
		separation -= sphereRadius;
		return true;
	}
	else
	{
		//center is in box, we definitely have a contact.
		PxVec3 locNorm;	//local coords contact normal

		/*const*/ PxVec3 absdRot;
		absdRot = PxVec3(PxAbs(dRot.x), PxAbs(dRot.y), PxAbs(dRot.z));
		/*const*/ PxVec3 distToSurface = boxExtents - absdRot;	//dist from embedded center to box surface along 3 dimensions.

		//find smallest element of distToSurface
		if (distToSurface.y < distToSurface.x)
		{
			if (distToSurface.y < distToSurface.z)
			{
				//y
				locNorm = PxVec3(0.0f, dRot.y > 0.0f ? 1.0f : -1.0f, 0.0f);
				separation = -distToSurface.y;
			}
			else
			{
				//z
				locNorm = PxVec3(0.0f,0.0f, dRot.z > 0.0f ? 1.0f : -1.0f);
				separation = -distToSurface.z;
			}
		}
		else
		{
			if (distToSurface.x < distToSurface.z)
			{
				//x
				locNorm = PxVec3(dRot.x > 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
				separation = -distToSurface.x;
			}
			else
			{
				//z
				locNorm = PxVec3(0.0f,0.0f, dRot.z > 0.0f ? 1.0f : -1.0f);
				separation = -distToSurface.z;
			}
		}
		//separation so far is just the embedding of the center point; we still have to push out all of the radius.
		point = sphereOrigin;
		normal = boxTransform.rotate(locNorm);
		separation -= sphereRadius;
		return true;
	}
}

bool Gu::computeMTD_SphereBox(PxVec3& mtd, PxF32& depth, const Sphere& sphere, const Box& box)
{
	PxVec3 point;
	if(!ContactSphereBox(	sphere.center, sphere.radius,
							box.extents, PxTransform(box.center, PxQuat(box.rot)),
							point, mtd, depth, 0.0f))
		return false;
	depth = validateDepth(-depth);
	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Gu::computeMTD_CapsuleCapsule(PxVec3& mtd, PxF32& depth, const Capsule& capsule0, const Capsule& capsule1)
{
	PxReal s,t;
	const PxReal d2 = distanceSegmentSegmentSquared2(capsule0, capsule1, &s, &t);

	const PxReal radiusSum = capsule0.radius + capsule1.radius;

	if(d2 > radiusSum*radiusSum)
		return false;

	const PxVec3 normal = capsule0.getPointAt(s) - capsule1.getPointAt(t);

	const PxReal lenSq = normal.magnitudeSquared();
	const PxF32 d = manualNormalize(mtd, normal, lenSq);

	depth = validateDepth(radiusSum - d);
	return true;
}

///////////////////////////////////////////////////////////////////////////////

// ##### refactor with LL

static PX_FORCE_INLINE void reorderMTD(PxVec3& mtd, const PxVec3& center0, const PxVec3& center1)
{
	const PxVec3 witness = center0 - center1;
	if(mtd.dot(witness) < 0.0f)
		mtd = -mtd;
}

static PX_FORCE_INLINE void projectBox(PxReal& min, PxReal& max, const PxVec3& axis, const Box& box)
{
	const PxReal boxCen = box.center.dot(axis);
	const PxReal boxExt = 
			PxAbs(box.rot.column0.dot(axis)) * box.extents.x
		+	PxAbs(box.rot.column1.dot(axis)) * box.extents.y
		+	PxAbs(box.rot.column2.dot(axis)) * box.extents.z;

	min = boxCen - boxExt; 
	max = boxCen + boxExt; 
}

static bool PxcTestAxis(const PxVec3& axis, const Segment& segment, PxReal radius, const Box& box, PxReal& depth)
{
	// Project capsule
	PxReal min0 = segment.p0.dot(axis);
	PxReal max0 = segment.p1.dot(axis);
	if(min0>max0) Ps::swap(min0, max0);
	min0 -= radius;
	max0 += radius;

	// Project box
	PxReal Min1, Max1;
	projectBox(Min1, Max1, axis, box);

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

static bool PxcCapsuleOBBOverlap3(const Segment& segment, PxReal radius, const Box& box, PxReal* t=NULL, PxVec3* pp=NULL)
{
	PxVec3 Sep(0.0f);
	PxReal PenDepth = PX_MAX_REAL;

	// Test normals
	for(PxU32 i=0;i<3;i++)
	{
		PxReal d;
		if(!PxcTestAxis(box.rot[i], segment, radius, box, d))
			return false;

		if(d<PenDepth)
		{
			PenDepth = d;
			Sep = box.rot[i];
		}
	}

	// Test edges
	PxVec3 CapsuleAxis(segment.p1 - segment.p0);
	CapsuleAxis = CapsuleAxis.getNormalized();
	for(PxU32 i=0;i<3;i++)
	{
		PxVec3 Cross = CapsuleAxis.cross(box.rot[i]);
		if(!Ps::isAlmostZero(Cross))
		{
			Cross = Cross.getNormalized();
			PxReal d;
			if(!PxcTestAxis(Cross, segment, radius, box, d))
				return false;

			if(d<PenDepth)
			{
				PenDepth = d;
				Sep = Cross;
			}
		}
	}

/*	const PxVec3 Witness = segment.computeCenter() - box.center;
	if(Sep.dot(Witness) < 0.0f)
		Sep = -Sep;*/
	reorderMTD(Sep, segment.computeCenter(), box.center);

	if(t)
		*t = validateDepth(PenDepth);
	if(pp)
		*pp = Sep;

	return true;
}

bool Gu::computeMTD_CapsuleBox(PxVec3& mtd, PxF32& depth, const Capsule& capsule, const Box& box)
{
	PxReal t;
	PxVec3 onBox;

	const PxReal d2 = distanceSegmentBoxSquared(capsule.p0, capsule.p1, box.center, box.extents, box.rot, &t, &onBox);	

	if(d2 > capsule.radius*capsule.radius)
		return false;

	if(d2 != 0.0f)
	{
		// PT: the capsule segment doesn't intersect the box => distance-based version
		const PxVec3 onSegment = capsule.getPointAt(t);
		onBox = box.center + box.rot.transform(onBox);

		PxVec3 normal = onSegment - onBox;
		PxReal normalLen = normal.magnitude();

		if(normalLen != 0.0f)
		{
			normal *= 1.0f/normalLen;

			mtd = normal;
			depth = validateDepth(capsule.radius - PxSqrt(d2));
			return true;
		}
	}

	// PT: the capsule segment intersects the box => penetration-based version
	return PxcCapsuleOBBOverlap3(capsule, capsule.radius, box, &depth, &mtd);
}

///////////////////////////////////////////////////////////////////////////////

// PT: TODO: optimize this....
static bool PxcTestAxis(const PxVec3& axis, const Box& box0, const Box& box1, PxReal& depth)
{
	// Project box
	PxReal min0, max0;
	projectBox(min0, max0, axis, box0);

	// Project box
	PxReal Min1, Max1;
	projectBox(Min1, Max1, axis, box1);

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

static PX_FORCE_INLINE bool testBoxBoxAxis(PxVec3& mtd, PxF32& depth, const PxVec3& axis, const Box& box0, const Box& box1)
{
	PxF32 d;
	if(!PxcTestAxis(axis, box0, box1, d))
		return false;
	if(d<depth)
	{
		depth = d;
		mtd = axis;
	}
	return true;
}

bool Gu::computeMTD_BoxBox(PxVec3& _mtd, PxF32& _depth, const Box& box0, const Box& box1)
{
	PxVec3 mtd;
	PxF32 depth = PX_MAX_F32;

	if(!testBoxBoxAxis(mtd, depth, box0.rot.column0, box0, box1))
		return false;
	if(!testBoxBoxAxis(mtd, depth, box0.rot.column1, box0, box1))
		return false;
	if(!testBoxBoxAxis(mtd, depth, box0.rot.column2, box0, box1))
		return false;

	if(!testBoxBoxAxis(mtd, depth, box1.rot.column0, box0, box1))
		return false;
	if(!testBoxBoxAxis(mtd, depth, box1.rot.column1, box0, box1))
		return false;
	if(!testBoxBoxAxis(mtd, depth, box1.rot.column2, box0, box1))
		return false;

	for(PxU32 j=0;j<3;j++)
	{
		for(PxU32 i=0;i<3;i++)
		{
			PxVec3 cross = box0.rot[i].cross(box1.rot[j]);
			if(!Ps::isAlmostZero(cross))
			{
				cross = cross.getNormalized();

				if(!testBoxBoxAxis(mtd, depth, cross, box0, box1))
					return false;
			}
		}
	}

/*	const PxVec3 witness = box1.center - box0.center;
	if(mtd.dot(witness) < 0.0f)
		mtd = -mtd;*/
	reorderMTD(mtd, box1.center, box0.center);

	_mtd	= -mtd;
	_depth	= validateDepth(depth);

	return true;
}

///////////////////////////////////////////////////////////////////////////////

#include "PxConvexMeshGeometry.h"
#include "GuConvexMesh.h"
#include "PxMeshScale.h"
#include "GuVecConvexHull.h"
//#include "GuVecSegment.h"
#include "GuVecSphere.h"
#include "GuGJK.h"
#include "PsVecMath.h"
#include "GuEPA.h"

using namespace physx::shdfnd::aos;

//static const ConvexHullData* getConvexV(ConvexHullV& hullV, const PxConvexMeshGeometry& convexGeom, const PxTransform& convexPose)
//{
//	const ConvexMesh* convexMesh = static_cast<const ConvexMesh*>(convexGeom.convexMesh);
//	const ConvexHullData* hull = &convexMesh->getHull();
//	const QuatV q1 = QuatV_From_F32Array(&convexPose.q.x);
//	const Vec3V p1 = Vec3V_From_Vec4V(Vec4V_From_F32Array(&convexPose.p.x));
//	const Vec3V vScale = Vec3V_From_PxVec3(convexGeom.scale.scale);
//	const QuatV vQuat = QuatV_From_F32Array(&convexGeom.scale.rotation.x);
//	const Mat33V rot = QuatGetMat33V(q1);
//	hullV = ConvexHullV(hull, p1, rot, vScale, vQuat);
//	return hull;
//}


/*static*/ bool pointConvexDistance(PxVec3& normal_, PxVec3& closestPoint_, PxReal& sqDistance, const PxVec3& pt, const ConvexMesh* convexMesh, const PxMeshScale& meshScale, const PxTransform& convexPose)
{
//	BigConvexHullV convexHull;
//	const ConvexHullData* hullData = getConvexV(convexHull, convexGeom, convexPose);

	const PxTransform transform0(pt);

	///
  
	PxVec3 onSegment, onConvex;

	using namespace Ps::aos;
	const Vec3V zeroV = V3Zero();
	Vec3V closA, closB, normalV;
	//bool intersect = false;
	PxGJKStatus status;
	FloatV sqDist;
	{
		const ConvexHullData* hullData = &convexMesh->getHull();
		/*const QuatV q1 = QuatV_From_F32Array(&convexPose.q.x);
		const Vec3V p1 = Vec3V_From_Vec4V(Vec4V_From_F32Array(&convexPose.p.x));*/
		const Vec3V vScale = V3LoadU(meshScale.scale);
		const QuatV vQuat = QuatVLoadU(&meshScale.rotation.x);
		//const Mat33V rot = QuatGetMat33V(q1);
		const ConvexHullV convexHull(hullData, zeroV, vScale, vQuat);

		//const QuatV q0 = QuatV_From_F32Array(&transform0.q.x);
		//const Vec3V p0 = Vec3V_From_Vec4V(Vec4V_From_F32Array(&transform0.p.x));

		const PsMatTransformV aToB(convexPose.transformInv(transform0)); 
		
		const CapsuleV capsule(zeroV, zeroV, FZero());//this is a point
		//status = GJK(capsule, convexHull, closA, closB, normalV, sqDist);

		status = GJKRelative(capsule, convexHull, aToB, closA, closB, normalV, sqDist);
	}

	bool intersect = status == GJK_CONTACT;
	if(intersect)
	{
		sqDistance = 0.0f;
	}
	else
	{
		FStore(sqDist, &sqDistance);
		V3StoreU(normalV, normal_);
		V3StoreU(closB, closestPoint_);

		normal_ = convexPose.rotate(normal_);
		closestPoint_ = convexPose.transform(closestPoint_);
	}

	return intersect;
}

bool Gu::computeMTD_SphereConvex(PxVec3& mtd, PxF32& depth, const Sphere& sphere, const PxConvexMeshGeometry& convexGeom, const PxTransform& convexPose)
{
	PxReal d2;
	const ConvexMesh* convexMesh = static_cast<const ConvexMesh*>(convexGeom.convexMesh);
	PxVec3 dummy;
	if(!pointConvexDistance(mtd, dummy, d2, sphere.center, convexMesh, convexGeom.scale, convexPose))
	{
		if(d2 > sphere.radius*sphere.radius)
			return false;

		depth = validateDepth(sphere.radius - PxSqrt(d2));
		mtd = -mtd;
		return true;
	}

	// PT: if we reach this place, the sphere center touched the convex => switch to penetration-based code
	PxU32 nbPolygons = convexMesh->getNbPolygonsFast();
	const HullPolygonData* polygons = convexMesh->getPolygons();
	const PxVec3 localSphereCenter = convexPose.transformInv(sphere.center);
	PxReal dmax = -PX_MAX_F32;
	while(nbPolygons--)
	{
		const HullPolygonData& polygon = *polygons++;
		const PxF32 d = polygon.mPlane.distance(localSphereCenter);
		if(d>dmax)		
		{
			dmax = d;
			mtd = convexPose.rotate(polygon.mPlane.n);
		}
	}
	depth = validateDepth(sphere.radius - dmax);
	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Gu::computeMTD_BoxConvex(PxVec3& mtd, PxF32& depth, const Box& box, const PxConvexMeshGeometry& convexGeom, const PxTransform& convexPose)
{
	// PT: TODO: helper to convert boxes from non-SIMD to SIMD?
	const PxQuat boxRot(box.rot);
	const Vec3V p0 = Vec3V_From_Vec4V(V4LoadU(&box.center.x));
	const Vec3V boxExtents = V3LoadU(box.extents);
	ShrunkBoxV boxV(p0, boxExtents);
	const PxTransform boxTransform(box.center, boxRot);

	// Convex mesh
		const ConvexMesh* convexMesh = static_cast<const ConvexMesh*>(convexGeom.convexMesh);
		const ConvexHullData* hull = &convexMesh->getHull();
		const Vec3V p1		= V3LoadU(&convexPose.p.x);
		const Vec3V vScale	= Vec3V_From_Vec4V(V4LoadU(&convexGeom.scale.scale.x));
		const QuatV vQuat	= QuatVLoadU(&convexGeom.scale.rotation.x);
		Gu::ShrunkConvexHullV convexHullV(hull, p1, vScale, vQuat); 
	//~Convex mesh

	const PxTransform curTrans = convexPose.transformInv(boxTransform);
	const PsTransformV tmp(curTrans);
	const PsMatTransformV aToB(tmp);

	Ps::aos::Vec3V contactA;
	Ps::aos::Vec3V contactB;
	Ps::aos::Vec3V normal;
	Ps::aos::FloatV penetrationDepth;
//	const FloatV contactDist = getContactEps(boxV.getMargin(), convexHull.getMargin());
	
	const FloatV zero = FZero();
	const FloatV contactDist = zero;
	/*const FloatV originalBoxMargin = boxV.getMargin();
	const FloatV originalConvexMargin = convexHullV.getMargin();*/
	boxV.setMargin(zero);
	//convexHullV.setMargin(zero);

	PxU8 aIndice[4];
	PxU8 bIndice[4];
	PxU8 size = 0;
	//contactA and contactB will be in the local space of convex
	PxGJKStatus status = Gu::GJKRelativePenetration(boxV, convexHullV, aToB, contactDist, contactA, contactB, normal, penetrationDepth, aIndice, bIndice, size);
	if(status == EPA_CONTACT)
	{
		status = Gu::EPARelativePenetration(boxV, (Gu::ConvexHullV&)convexHullV, aToB, contactA, contactB, normal, penetrationDepth, aIndice, bIndice, size);
//		status = Gu::EPARelativePenetration((Gu::ExpandedBoxV&)boxV, (Gu::ExpandedConvexHullV&)convexHullV, aToB, contactA, contactB, normal, penetrationDepth, aIndice, bIndice, size);

		if(status == EPA_CONTACT || status == EPA_DEGENERATE)
		{
			normal = V3Normalize(V3Sub(contactB, contactA));
			penetrationDepth = FNeg(V3Length(V3Sub(contactB, contactA)));
		}
		else
		{
			//ML: if epa fail to generate simplex, which means the simplex size is 3 or less, we can use the gjk contact points
			normal = V3Normalize(V3Sub(contactA, contactB));
			penetrationDepth = FNeg(V3Length(V3Sub(contactB, contactA)));
		}
	}

	if(status!=GJK_NON_INTERSECT)
	{
		FStore(penetrationDepth, &depth);
		depth = validateDepth(-depth);
		V3StoreU(normal, mtd);
		mtd = convexPose.rotate(mtd);
	}	
	return status!=GJK_NON_INTERSECT && FAllGrtr(zero, penetrationDepth);
}

///////////////////////////////////////////////////////////////////////////////

bool Gu::computeMTD_CapsuleConvex(PxVec3& mtd, PxF32& depth, const Capsule& capsule, const PxConvexMeshGeometry& convexGeom, const PxTransform& convexPose)
{
	PxF32 halfHeight;
	const PxTransform capsulePose = getCapsuleTransform(capsule, halfHeight);
	const FloatV capsuleHalfHeight = FLoad(halfHeight);
	const FloatV capsuleRadius = FLoad(capsule.radius);


	// Convex mesh
		const ConvexMesh* convexMesh = static_cast<const ConvexMesh*>(convexGeom.convexMesh);
		const ConvexHullData* hull = &convexMesh->getHull();
		//const Vec3V p1		= Vec3V_From_Vec4V(Vec4V_From_F32Array(&convexPose.p.x));
		const Vec3V vScale	= V3LoadU(convexGeom.scale.scale);
		const QuatV vQuat	= QuatVLoadU(&convexGeom.scale.rotation.x);
		//Gu::ShrinkedConvexHullV convexHullV(hull, V3Zero(), vScale, vQuat); 
		Gu::ConvexHullV convexHullV(hull, V3Zero(), vScale, vQuat); 
	//~Convex mesh

	const PxTransform curTrans = convexPose.transformInv(capsulePose);
	const PsTransformV tmp(curTrans);
	const PsMatTransformV aToB(tmp);

	Ps::aos::Vec3V contactA;
	Ps::aos::Vec3V contactB;
	Ps::aos::Vec3V normal;
	Ps::aos::FloatV penetrationDepth = FZero();
	const FloatV zero = FZero();
	const FloatV contactDist = zero;


	Gu::CapsuleV capsuleV(aToB.p, aToB.rotate(V3Scale(V3UnitX(), capsuleHalfHeight)), capsuleRadius);
	//convexHullV.setMargin(zero);
	//capsuleV.setMargin(zero);

	PxU8 aIndice[4];
	PxU8 bIndice[4];
	PxU8 size = 0;

	//ML: contactA and contactB will be in the local space of convexHull
	PxGJKStatus status = Gu::GJKLocalPenetration(capsuleV, (Gu::ShrunkConvexHullV&)convexHullV, contactDist, contactA, contactB, normal, penetrationDepth, aIndice, bIndice, size);

	if(status == EPA_CONTACT)
	{
		status = Gu::EPALocalPenetration(capsuleV, (Gu::ConvexHullV&)convexHullV, contactA, contactB, normal, penetrationDepth, aIndice, bIndice, size);

		if(status == EPA_CONTACT || status == EPA_DEGENERATE)
		{
			normal = V3Normalize(V3Sub(contactB, contactA));
			contactA = V3Sub(contactA, V3Scale(normal, capsuleV.radius));
			penetrationDepth = FNeg(V3Length(V3Sub(contactB, contactA)));
		}
		else
		{
			//ML: if epa fail to generate simplex, need to put back up code in here
			normal = V3Normalize(V3Sub(contactA, contactB));
			contactA = V3Sub(contactA, V3Scale(normal, capsuleV.radius));
			penetrationDepth = FNeg(V3Length(V3Sub(contactB, contactA)));

		}
	}
	
	if(status!=GJK_NON_INTERSECT)
	{
		FStore(penetrationDepth, &depth);
		depth = validateDepth(-depth);
		V3StoreU(normal, mtd);
		mtd = convexPose.rotate(mtd);
	}
	return status!=GJK_NON_INTERSECT && FAllGrtr(zero, penetrationDepth);
}

///////////////////////////////////////////////////////////////////////////////

bool Gu::computeMTD_ConvexConvex(PxVec3& mtd, PxF32& depth, const PxConvexMeshGeometry& convexGeom0, const PxTransform& convexPose0, const PxConvexMeshGeometry& convexGeom1, const PxTransform& convexPose1)
{
	using namespace Ps::aos;

	const Vec3V zeroV = V3Zero();
	// Convex mesh
		const ConvexMesh* convexMesh0 = static_cast<const ConvexMesh*>(convexGeom0.convexMesh);
		const ConvexHullData* hull0 = &convexMesh0->getHull();
		//const Vec3V p0		= Vec3V_From_Vec4V(Vec4V_From_F32Array(&convexPose0.p.x));
		const Vec3V vScale0	= V3LoadU(convexGeom0.scale.scale);
		const QuatV vQuat0	= QuatVLoadU(&convexGeom0.scale.rotation.x);
		Gu::ShrunkConvexHullV convexHullV0(hull0, zeroV, vScale0, vQuat0);
	//~Convex mesh

	// Convex mesh
		const ConvexMesh* convexMesh1 = static_cast<const ConvexMesh*>(convexGeom1.convexMesh);
		const ConvexHullData* hull1 = &convexMesh1->getHull();
		//const Vec3V p1		= Vec3V_From_Vec4V(Vec4V_From_F32Array(&convexPose1.p.x));
		const Vec3V vScale1	= V3LoadU(convexGeom1.scale.scale);
		const QuatV vQuat1	= QuatVLoadU(&convexGeom1.scale.rotation.x);
		Gu::ShrunkConvexHullV convexHullV1(hull1, zeroV, vScale1, vQuat1);
	//~Convex mesh

	const PxTransform curTrans = convexPose1.transformInv(convexPose0);
	const PsTransformV tmp(curTrans);
	const PsMatTransformV aToB(tmp);

	Ps::aos::Vec3V contactA;
	Ps::aos::Vec3V contactB;
	Ps::aos::Vec3V normal;
	Ps::aos::FloatV penetrationDepth;
	const FloatV zero = FZero();
	const FloatV contactDist = zero;

	convexHullV0.setMargin(zero);
	//convexHullV1.setMargin(zero);

	PxU8 aIndice[4];
	PxU8 bIndice[4];
	PxU8 size = 0;
	//contactA and contactB will be in the local space of convex
	PxGJKStatus status = Gu::GJKRelativePenetration(convexHullV0, convexHullV1, aToB, contactDist, contactA, contactB, normal, penetrationDepth, aIndice, bIndice, size);
	if(status == EPA_CONTACT)
	{
		status = Gu::EPARelativePenetration((Gu::ConvexHullV&)convexHullV0, (Gu::ConvexHullV&)convexHullV1, aToB, contactA, contactB, normal, penetrationDepth, aIndice, bIndice, size);

		if(status == EPA_CONTACT || status == EPA_DEGENERATE)
		{
			normal = V3Normalize(V3Sub(contactB, contactA));
			penetrationDepth = FNeg(V3Length(V3Sub(contactB, contactA)));
		}
		else
		{
			//ML: if epa fail to generate simplex. we will need back up code
			normal = V3Normalize(V3Sub(contactA, contactB));
			penetrationDepth = FNeg(V3Length(V3Sub(contactB, contactA)));
		}
	}

	if(status!=GJK_NON_INTERSECT)
	{
		FStore(penetrationDepth, &depth);
		depth = validateDepth(-depth);
		V3StoreU(normal, mtd);
		mtd = convexPose1.rotate(mtd);
	}
	return status!=GJK_NON_INTERSECT && FAllGrtr(zero, penetrationDepth);
}

///////////////////////////////////////////////////////////////////////////////

bool Gu::computeMTD_SpherePlane(PxVec3& mtd, PxF32& depth, const Sphere& sphere, const PxPlane& plane)
{
	const PxReal d = plane.distance(sphere.center);
	if(d>sphere.radius)
		return false;

	mtd		= plane.n;
	depth	= validateDepth(sphere.radius - d);
	return true;
}

bool Gu::computeMTD_PlaneBox(PxVec3& mtd, PxF32& depth, const PxPlane& plane, const Box& box)
{
	PxVec3 pts[8];
	box.computeBoxPoints(pts);

	PxReal dmin = plane.distance(pts[0]);
	for(PxU32 i=1;i<8;i++)
	{
		const PxReal d = plane.distance(pts[i]);
		dmin = physx::intrinsics::selectMin(dmin, d);
	}
	if(dmin>0.0f)
		return false;

	mtd		= -plane.n;
	depth	= validateDepth(-dmin);
	return true;
}

bool Gu::computeMTD_PlaneCapsule(PxVec3& mtd, PxF32& depth, const PxPlane& plane, const Capsule& capsule)
{
	const PxReal d0 = plane.distance(capsule.p0);
	const PxReal d1 = plane.distance(capsule.p1);
	const PxReal dmin = physx::intrinsics::selectMin(d0, d1) - capsule.radius;
	if(dmin>0.0f)
		return false;

	mtd		= -plane.n;
	depth	= validateDepth(-dmin);
	return true;
}

bool Gu::computeMTD_PlaneConvex(PxVec3& mtd, PxF32& depth, const PxPlane& plane, const PxConvexMeshGeometry& convexGeom, const PxTransform& convexPose)
{
	const ConvexMesh* convexMesh = static_cast<const ConvexMesh*>(convexGeom.convexMesh);
	PxU32 nbVerts = convexMesh->getNbVerts();
	const PxVec3* PX_RESTRICT verts = convexMesh->getVerts();

	PxReal dmin = plane.distance(convexPose.transform(verts[0]));
	for(PxU32 i=1;i<nbVerts;i++)
	{
		const PxReal d = plane.distance(convexPose.transform(verts[i]));
		dmin = physx::intrinsics::selectMin(dmin, d);
	}
	if(dmin>0.0f)
		return false;

	mtd		= -plane.n;
	depth	= validateDepth(-dmin);
	return true;
}

///////////////////////////////////////////////////////////////////////////////

static bool processContacts(PxVec3& mtd, PxF32& depth, PxU32 nbContacts, const Gu::ContactPoint* contacts)
{
	if(nbContacts)
	{
		PxVec3 mn(0.0f), mx(0.0f);
		for(PxU32 i=0; i<nbContacts; i++)
		{
			const Gu::ContactPoint& ct = contacts[i];
//			if(ct.separation > sweepSeparation) // just in case we got contacts outside of specified max separation
//				continue;
//			PxVec3 depenetration = -(ct.separation-sweepSeparation) * ct.normal;
//			PxVec3 depenetration = ct.depth * ct.normal;
			PxVec3 depenetration = ct.separation * ct.normal;
			
			mn = mn.minimum(depenetration);
			mx = mx.maximum(depenetration);
		}

		// even if we are already moving in separation direction, we should still depenetrate
		// so no dot velocity test
		// here we attempt to equalize the separations pushing in opposing directions along each axis
		PxVec3 mn1, mx1;
		mn1.x = (mn.x == 0.0f) ? mx.x : mn.x;
		mn1.y = (mn.y == 0.0f) ? mx.y : mn.y;
		mn1.z = (mn.z == 0.0f) ? mx.z : mn.z;
		mx1.x = (mx.x == 0.0f) ? mn.x : mx.x;
		mx1.y = (mx.y == 0.0f) ? mn.y : mx.y;
		mx1.z = (mx.z == 0.0f) ? mn.z : mx.z;
		PxVec3 sepDir((mn1 + mx1)*0.5f);

		if(sepDir.magnitudeSquared() < 1e-10f)
		{
//			PX_ASSERT(0); 
			return false;
/*			// either atom0 is trapped exactly in the middle between opposing faces of other objects
			// or there is insufficient penetration.. not much we can do here,
			// stop the object and roll back to last transform
			// AP newccd magic number
			bool trappedX = (mn.x != 0.0f && mx.x != 0.0f);
			bool trappedY = (mn.y != 0.0f && mx.y != 0.0f);
			bool trappedZ = (mn.z != 0.0f && mx.z != 0.0f);
			if (atom0 && (trappedX || trappedY || trappedZ))
				jammed = true; // report jammed, don't move, nothing we can do, depenetration is impossible
			return PxVec3(0.0f);*/
		}

/*		sepDir.normalize();

		PxReal maxPrj = 0.0f;
		for(PxU32 iContact=0; iContact<nbContacts; iContact++)
		{
//			Gu::ContactPoint& ct = allContacts[iContact];
			const SphereMeshMTDCallback::Contact& ct = contacts[iContact];
//			if (ct.separation > sweepSeparation) // just in case we got contacts outside of specified max separation
//				continue;
			const PxReal prj = sepDir.dot(ct.normal);
			if (prj < 1e-5f)
				// this can happen if contacts are direction opposing
				// in this case the average should equalize penetration when applied on the positive side
				// also can happen if one contact is perpendicular to the other one and the push-out direction
				// is dominated by separation from the first one
				continue;

			// we want to separate to sweepSeparation, so we need to offset ct.separation by sweepSeparation
//			maxPrj = PxMax(maxPrj, (sweepSeparation-ct.separation) / prj);
			maxPrj = PxMax(maxPrj, ct.depth / prj);
		}

		// separate atom0 away from atom1
		// AP newccd - potentially move atom1 instead of atom0
//		const PxVec3 pushout = sepDir * maxPrj;
		mtd = sepDir;
		depth = -maxPrj;*/

		mtd = -sepDir.getNormalized();
		depth = sepDir.magnitude();
	}
	return true;
}

bool Gu::computeMTD_SphereMesh(PxVec3& mtd, PxF32& depth, const Sphere& sphere, const PxTriangleMeshGeometry& meshGeom, const PxTransform& meshPose)
{
	Gu::GeometryUnion shape0;
	shape0.set(PxSphereGeometry(sphere.radius));

	Gu::GeometryUnion shape1;
	shape1.set(meshGeom);

	Cache cache;

	ContactBuffer contactBuffer;
	contactBuffer.reset();
	contactBuffer.meshContactMargin = 0.0f;

	if(!contactSphereMesh(shape0, shape1, PxTransform(sphere.center), meshPose, 0.0f, cache, contactBuffer))
		return false;

	if(!processContacts(mtd, depth, contactBuffer.count, contactBuffer.contacts))
		return false;

	return contactBuffer.count!=0;
}

bool Gu::computeMTD_CapsuleMesh(PxVec3& mtd, PxF32& depth, const Capsule& capsule, const PxTriangleMeshGeometry& meshGeom, const PxTransform& meshPose)
{
	// PT: TODO: this is silly, we go from Capsule to PxCapsuleGeometry/PxTransform to cross the API, but the first thing
	// we do inside "contactCapsuleMesh" is computing a Capsule from the given PxCapsuleGeometry/PxTransform.
	PxReal halfHeight;
	const PxTransform capsuleTransform = PxTransformFromSegment(capsule.p0, capsule.p1, &halfHeight);

	Gu::GeometryUnion shape0;
	shape0.set(PxCapsuleGeometry(capsule.radius, halfHeight));

	Gu::GeometryUnion shape1;
	shape1.set(meshGeom);

	Cache cache;

	ContactBuffer contactBuffer;
	contactBuffer.reset();
	contactBuffer.meshContactMargin = 0.0f;

	if(!contactCapsuleMesh(shape0, shape1, capsuleTransform, meshPose, 0.0f, cache, contactBuffer))
		return false;

	if(!processContacts(mtd, depth, contactBuffer.count, contactBuffer.contacts))
		return false;

	return contactBuffer.count!=0;
}

bool Gu::computeMTD_BoxMesh(PxVec3& mtd, PxF32& depth, const Box& box, const PxTriangleMeshGeometry& meshGeom, const PxTransform& meshPose)
{
	const PxTransform boxPose(box.center, PxQuat(box.rot));

	Gu::GeometryUnion shape0;
	shape0.set(PxBoxGeometry(box.extents));

	Gu::GeometryUnion shape1;
	shape1.set(meshGeom);

	Cache cache;

	ContactBuffer contactBuffer;
	contactBuffer.reset();
	contactBuffer.meshContactMargin = 0.0f;

	if(!contactBoxMesh(shape0, shape1, boxPose, meshPose, 0.0f, cache, contactBuffer))
		return false;

	if(!processContacts(mtd, depth, contactBuffer.count, contactBuffer.contacts))
		return false;

	return contactBuffer.count!=0;
}

bool Gu::computeMTD_ConvexMesh(PxVec3& mtd, PxF32& depth, const PxConvexMeshGeometry& convexGeom, const PxTransform& convexPose, const PxTriangleMeshGeometry& meshGeom, const PxTransform& meshPose)
{
	Gu::GeometryUnion shape0;
	shape0.set(convexGeom);

	Gu::GeometryUnion shape1;
	shape1.set(meshGeom);

	Cache cache;

	ContactBuffer contactBuffer;
	contactBuffer.reset();
	contactBuffer.meshContactMargin = 0.0f;

	if(!contactConvexMesh(shape0, shape1, convexPose, meshPose, 0.0f, cache, contactBuffer))
		return false;

	if(!processContacts(mtd, depth, contactBuffer.count, contactBuffer.contacts))
		return false;

	return contactBuffer.count!=0;
}

bool Gu::computeMTD_SphereHeightField(PxVec3& mtd, PxF32& depth, const Sphere& sphere, const PxHeightFieldGeometry& meshGeom, const PxTransform& meshPose)
{
	Gu::GeometryUnion shape0;
	shape0.set(PxSphereGeometry(sphere.radius));

	Gu::GeometryUnion shape1;
	shape1.set(meshGeom);

	Cache cache;

	ContactBuffer contactBuffer;
	contactBuffer.reset();
	contactBuffer.meshContactMargin = 0.0f;

	PxTransform spherePose(sphere.center);

	if(!contactSphereHeightField(shape0, shape1, spherePose, meshPose, 0.0f, cache, contactBuffer))
		return false;

	if(!processContacts(mtd, depth, contactBuffer.count, contactBuffer.contacts))
		return false;

	return contactBuffer.count!=0;
}

bool Gu::computeMTD_CapsuleHeightField(PxVec3& mtd, PxF32& depth, const Capsule& capsule, const PxHeightFieldGeometry& meshGeom, const PxTransform& meshPose)
{
	// PT: TODO: this is silly, we go from Capsule to PxCapsuleGeometry/PxTransform to cross the API, but the first thing
	// we do inside "contactCapsuleMesh" is computing a Capsule from the given PxCapsuleGeometry/PxTransform.
	PxReal halfHeight;
	const PxTransform capsuleTransform = PxTransformFromSegment(capsule.p0, capsule.p1, &halfHeight);

	Gu::GeometryUnion shape0;
	shape0.set(PxCapsuleGeometry(capsule.radius, halfHeight));

	Gu::GeometryUnion shape1;
	shape1.set(meshGeom);

	Cache cache;

	ContactBuffer contactBuffer;
	contactBuffer.reset();
	contactBuffer.meshContactMargin = 0.0f;

	if(!contactCapsuleHeightfield(shape0, shape1, capsuleTransform, meshPose, 0.0f, cache, contactBuffer))
		return false;

	if(!processContacts(mtd, depth, contactBuffer.count, contactBuffer.contacts))
		return false;

	return contactBuffer.count!=0;
}

bool Gu::computeMTD_BoxHeightField(PxVec3& mtd, PxF32& depth, const Box& box, const PxHeightFieldGeometry& meshGeom, const PxTransform& meshPose)
{
	const PxTransform boxPose(box.center, PxQuat(box.rot));

	Gu::GeometryUnion shape0;
	shape0.set(PxBoxGeometry(box.extents));

	Gu::GeometryUnion shape1;
	shape1.set(meshGeom);

	Cache cache;

	ContactBuffer contactBuffer;
	contactBuffer.reset();
	contactBuffer.meshContactMargin = 0.0f;

	if(!contactBoxHeightfield(shape0, shape1, boxPose, meshPose, 0.0f, cache, contactBuffer))
		return false;

	if(!processContacts(mtd, depth, contactBuffer.count, contactBuffer.contacts))
		return false;

	return contactBuffer.count!=0;
}

bool Gu::computeMTD_ConvexHeightField(PxVec3& mtd, PxF32& depth, const PxConvexMeshGeometry& convexGeom, const PxTransform& convexPose, const PxHeightFieldGeometry& meshGeom, const PxTransform& meshPose)
{
	Gu::GeometryUnion shape0;
	shape0.set(convexGeom);

	Gu::GeometryUnion shape1;
	shape1.set(meshGeom);

	Cache cache;

	ContactBuffer contactBuffer;
	contactBuffer.reset();
	contactBuffer.meshContactMargin = 0.0f;

	if(!contactConvexHeightfield(shape0, shape1, convexPose, meshPose, 0.0f, cache, contactBuffer))
		return false;

	if(!processContacts(mtd, depth, contactBuffer.count, contactBuffer.contacts))
		return false;

	return contactBuffer.count!=0;
}


static bool GeomMTDCallback_NotSupported(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(!"NOT SUPPORTED");
	PX_UNUSED(mtd); PX_UNUSED(depth); PX_UNUSED(geom0); PX_UNUSED(geom1); PX_UNUSED(transform0); PX_UNUSED(transform1);

	return false;
}

static bool GeomMTDCallback_SphereSphere(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eSPHERE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eSPHERE);

	const PxSphereGeometry& sphereGeom0 = static_cast<const PxSphereGeometry&>(geom0);
	const PxSphereGeometry& sphereGeom1 = static_cast<const PxSphereGeometry&>(geom1);

	return computeMTD_SphereSphere(mtd, depth, Sphere(transform0.p, sphereGeom0.radius), Sphere(transform1.p, sphereGeom1.radius));
}

static bool GeomMTDCallback_SpherePlane(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eSPHERE);
	PX_ASSERT(geom1.getType()==PxGeometryType::ePLANE);
	PX_UNUSED(geom1);

	const PxSphereGeometry& sphereGeom = static_cast<const PxSphereGeometry&>(geom0);
	return computeMTD_SpherePlane(mtd, depth, Sphere(transform0.p, sphereGeom.radius), Gu::getPlane(transform1));
}

static bool GeomMTDCallback_SphereCapsule(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eSPHERE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eCAPSULE);

	const PxSphereGeometry& sphereGeom = static_cast<const PxSphereGeometry&>(geom0);
	const PxCapsuleGeometry& capsuleGeom = static_cast<const PxCapsuleGeometry&>(geom1);

	Gu::Capsule capsule;
	getCapsuleSegment(transform1, capsuleGeom, capsule);
	capsule.radius = capsuleGeom.radius;

	return computeMTD_SphereCapsule(mtd, depth, Sphere(transform0.p, sphereGeom.radius), capsule);
}

static bool GeomMTDCallback_SphereBox(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eSPHERE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eBOX);

	const PxSphereGeometry& sphereGeom = static_cast<const PxSphereGeometry&>(geom0);
	const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom1);

	Gu::Box obb;
	buildFrom(obb, transform1.p, boxGeom.halfExtents, transform1.q);

	return computeMTD_SphereBox(mtd, depth, Sphere(transform0.p, sphereGeom.radius), obb);
}

static bool GeomMTDCallback_SphereConvex(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eSPHERE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eCONVEXMESH);

	const PxSphereGeometry& sphereGeom = static_cast<const PxSphereGeometry&>(geom0);
	const PxConvexMeshGeometry& convexGeom = static_cast<const PxConvexMeshGeometry&>(geom1);

	return computeMTD_SphereConvex(mtd, depth, Sphere(transform0.p, sphereGeom.radius), convexGeom, transform1);
}

static bool GeomMTDCallback_SphereMesh(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eSPHERE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eTRIANGLEMESH);

	const PxSphereGeometry& sphereGeom = static_cast<const PxSphereGeometry&>(geom0);
	const PxTriangleMeshGeometry& meshGeom = static_cast<const PxTriangleMeshGeometry&>(geom1);	

	return computeMTD_SphereMesh(mtd, depth, Sphere(transform0.p, sphereGeom.radius), meshGeom, transform1);
}

static bool GeomMTDCallback_PlaneCapsule(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::ePLANE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eCAPSULE);
	PX_UNUSED(geom0);
	
	const PxCapsuleGeometry& capsuleGeom = static_cast<const PxCapsuleGeometry&>(geom1);

	Gu::Capsule capsule;
	getCapsuleSegment(transform1, capsuleGeom, capsule);
	capsule.radius = capsuleGeom.radius;

	return computeMTD_PlaneCapsule(mtd, depth, Gu::getPlane(transform0), capsule);
}

static bool GeomMTDCallback_PlaneBox(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::ePLANE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eBOX);
	PX_UNUSED(geom0);

	const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom1);

	Gu::Box obb;
	buildFrom(obb, transform1.p, boxGeom.halfExtents, transform1.q);

	return computeMTD_PlaneBox(mtd, depth, Gu::getPlane(transform0), obb);
}

static bool GeomMTDCallback_PlaneConvex(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::ePLANE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eCONVEXMESH);
	PX_UNUSED(geom0);

	const PxConvexMeshGeometry& convexGeom = static_cast<const PxConvexMeshGeometry&>(geom1);

	return computeMTD_PlaneConvex(mtd, depth, Gu::getPlane(transform0), convexGeom, transform1);
}

static bool GeomMTDCallback_CapsuleCapsule(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eCAPSULE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eCAPSULE);

	const PxCapsuleGeometry& capsuleGeom0 = static_cast<const PxCapsuleGeometry&>(geom0);
	const PxCapsuleGeometry& capsuleGeom1 = static_cast<const PxCapsuleGeometry&>(geom1);

	Gu::Capsule capsule0;
	getCapsuleSegment(transform0, capsuleGeom0, capsule0);
	capsule0.radius = capsuleGeom0.radius;

	Gu::Capsule capsule1;
	getCapsuleSegment(transform1, capsuleGeom1, capsule1);
	capsule1.radius = capsuleGeom1.radius;

	return computeMTD_CapsuleCapsule(mtd, depth, capsule0, capsule1);
}

static bool GeomMTDCallback_CapsuleBox(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eCAPSULE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eBOX);

	const PxCapsuleGeometry& capsuleGeom = static_cast<const PxCapsuleGeometry&>(geom0);
	const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom1);

	Gu::Capsule capsule;
	getCapsuleSegment(transform0, capsuleGeom, capsule);
	capsule.radius = capsuleGeom.radius;

	Gu::Box obb;
	buildFrom(obb, transform1.p, boxGeom.halfExtents, transform1.q);

	return computeMTD_CapsuleBox(mtd, depth, capsule, obb);
}

static bool GeomMTDCallback_CapsuleConvex(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eCAPSULE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eCONVEXMESH);

	const PxCapsuleGeometry& capsuleGeom = static_cast<const PxCapsuleGeometry&>(geom0);
	const PxConvexMeshGeometry& convexGeom = static_cast<const PxConvexMeshGeometry&>(geom1);

	Gu::Capsule capsule;
	getCapsuleSegment(transform0, capsuleGeom, capsule);
	capsule.radius = capsuleGeom.radius;

	return computeMTD_CapsuleConvex(mtd, depth, capsule, convexGeom, transform1);
}

static bool GeomMTDCallback_CapsuleMesh(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eCAPSULE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eTRIANGLEMESH);

	const PxCapsuleGeometry& capsuleGeom = static_cast<const PxCapsuleGeometry&>(geom0);
	const PxTriangleMeshGeometry& meshGeom = static_cast<const PxTriangleMeshGeometry&>(geom1);	

	Gu::Capsule capsule;
	getCapsuleSegment(transform0, capsuleGeom, capsule);
	capsule.radius = capsuleGeom.radius;

	return computeMTD_CapsuleMesh(mtd, depth, capsule, meshGeom, transform1);
}

static bool GeomMTDCallback_BoxBox(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eBOX);
	PX_ASSERT(geom1.getType()==PxGeometryType::eBOX);

	const PxBoxGeometry& boxGeom0 = static_cast<const PxBoxGeometry&>(geom0);
	const PxBoxGeometry& boxGeom1 = static_cast<const PxBoxGeometry&>(geom1);

	Gu::Box obb0;
	buildFrom(obb0, transform0.p, boxGeom0.halfExtents, transform0.q);

	Gu::Box obb1;
	buildFrom(obb1, transform1.p, boxGeom1.halfExtents, transform1.q);

	return computeMTD_BoxBox(mtd, depth, obb0, obb1);
}

static bool GeomMTDCallback_BoxConvex(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eBOX);
	PX_ASSERT(geom1.getType()==PxGeometryType::eCONVEXMESH);

	const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom0);
	const PxConvexMeshGeometry& convexGeom = static_cast<const PxConvexMeshGeometry&>(geom1);

	Gu::Box obb;
	buildFrom(obb, transform0.p, boxGeom.halfExtents, transform0.q);

	return computeMTD_BoxConvex(mtd, depth, obb, convexGeom, transform1);
}

static bool GeomMTDCallback_BoxMesh(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eBOX);
	PX_ASSERT(geom1.getType()==PxGeometryType::eTRIANGLEMESH);

	const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom0);
	const PxTriangleMeshGeometry& meshGeom = static_cast<const PxTriangleMeshGeometry&>(geom1);	

	Gu::Box obb;
	buildFrom(obb, transform0.p, boxGeom.halfExtents, transform0.q);

	return computeMTD_BoxMesh(mtd, depth, obb, meshGeom, transform1);
}

static bool GeomMTDCallback_ConvexConvex(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eCONVEXMESH);
	PX_ASSERT(geom1.getType()==PxGeometryType::eCONVEXMESH);

	const PxConvexMeshGeometry& convexGeom0 = static_cast<const PxConvexMeshGeometry&>(geom0);
	const PxConvexMeshGeometry& convexGeom1 = static_cast<const PxConvexMeshGeometry&>(geom1);

	return computeMTD_ConvexConvex(mtd, depth, convexGeom0, transform0, convexGeom1, transform1);
}

static bool GeomMTDCallback_ConvexMesh(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eCONVEXMESH);
	PX_ASSERT(geom1.getType()==PxGeometryType::eTRIANGLEMESH);

	const PxConvexMeshGeometry& convexGeom = static_cast<const PxConvexMeshGeometry&>(geom0);
	const PxTriangleMeshGeometry& meshGeom = static_cast<const PxTriangleMeshGeometry&>(geom1);	

	return computeMTD_ConvexMesh(mtd, depth, convexGeom, transform0, meshGeom, transform1);
}

static bool GeomMTDCallback_SphereHeightField(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eSPHERE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eHEIGHTFIELD);

	const PxSphereGeometry& sphereGeom = static_cast<const PxSphereGeometry&>(geom0);
	const PxHeightFieldGeometry& meshGeom = static_cast<const PxHeightFieldGeometry&>(geom1);	

	Gu::Sphere sphere(transform0.p, sphereGeom.radius);

	return computeMTD_SphereHeightField(mtd, depth, sphere, meshGeom, transform1);
}

static bool GeomMTDCallback_CapsuleHeightField(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eCAPSULE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eHEIGHTFIELD);

	const PxCapsuleGeometry& capsuleGeom = static_cast<const PxCapsuleGeometry&>(geom0);
	const PxHeightFieldGeometry& meshGeom = static_cast<const PxHeightFieldGeometry&>(geom1);	

	Gu::Capsule capsule;
	getCapsuleSegment(transform0, capsuleGeom, capsule);
	capsule.radius = capsuleGeom.radius;

	return computeMTD_CapsuleHeightField(mtd, depth, capsule, meshGeom, transform1);
}

static bool GeomMTDCallback_BoxHeightField(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eBOX);
	PX_ASSERT(geom1.getType()==PxGeometryType::eHEIGHTFIELD);

	const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom0);
	const PxHeightFieldGeometry& meshGeom = static_cast<const PxHeightFieldGeometry&>(geom1);	

	Gu::Box obb;
	buildFrom(obb, transform0.p, boxGeom.halfExtents, transform0.q);

	return computeMTD_BoxHeightField(mtd, depth, obb, meshGeom, transform1);
}

static bool GeomMTDCallback_ConvexHeightField(GEOM_MTD_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eCONVEXMESH);
	PX_ASSERT(geom1.getType()==PxGeometryType::eHEIGHTFIELD);

	const PxConvexMeshGeometry& convexGeom = static_cast<const PxConvexMeshGeometry&>(geom0);
	const PxHeightFieldGeometry& meshGeom = static_cast<const PxHeightFieldGeometry&>(geom1);	

	return computeMTD_ConvexHeightField(mtd, depth, convexGeom, transform0, meshGeom, transform1);
}

Gu::GeomMTDFunc Gu::gGeomMTDMethodTable[][7] = 
{
	//PxGeometryType::eSPHERE
	{
		GeomMTDCallback_SphereSphere,		//PxGeometryType::eSPHERE
		GeomMTDCallback_SpherePlane,		//PxGeometryType::ePLANE
		GeomMTDCallback_SphereCapsule,		//PxGeometryType::eCAPSULE
		GeomMTDCallback_SphereBox,			//PxGeometryType::eBOX
		GeomMTDCallback_SphereConvex,		//PxGeometryType::eCONVEXMESH
		GeomMTDCallback_SphereMesh,			//PxGeometryType::eTRIANGLEMESH
		GeomMTDCallback_SphereHeightField,	//PxGeometryType::eHEIGHTFIELD
		
	},

	//PxGeometryType::ePLANE
	{
		0,									//PxGeometryType::eSPHERE
		GeomMTDCallback_NotSupported,		//PxGeometryType::ePLANE
		GeomMTDCallback_PlaneCapsule,		//PxGeometryType::eCAPSULE
		GeomMTDCallback_PlaneBox,			//PxGeometryType::eBOX
		GeomMTDCallback_PlaneConvex,		//PxGeometryType::eCONVEXMESH
		GeomMTDCallback_NotSupported,		//PxGeometryType::eTRIANGLEMESH
		GeomMTDCallback_NotSupported,		//PxGeometryType::eHEIGHTFIELD
	},

	//PxGeometryType::eCAPSULE
	{
		0,									//PxGeometryType::eSPHERE
		0,									//PxGeometryType::ePLANE
		GeomMTDCallback_CapsuleCapsule,		//PxGeometryType::eCAPSULE
		GeomMTDCallback_CapsuleBox,			//PxGeometryType::eBOX
		GeomMTDCallback_CapsuleConvex,		//PxGeometryType::eCONVEXMESH
		GeomMTDCallback_CapsuleMesh,		//PxGeometryType::eTRIANGLEMESH
		GeomMTDCallback_CapsuleHeightField,	//PxGeometryType::eHEIGHTFIELD
	},

	//PxGeometryType::eBOX
	{
		0,									//PxGeometryType::eSPHERE
		0,									//PxGeometryType::ePLANE
		0,									//PxGeometryType::eCAPSULE
		GeomMTDCallback_BoxBox,				//PxGeometryType::eBOX
		GeomMTDCallback_BoxConvex,			//PxGeometryType::eCONVEXMESH
		GeomMTDCallback_BoxMesh,			//PxGeometryType::eTRIANGLEMESH
		GeomMTDCallback_BoxHeightField,		//PxGeometryType::eHEIGHTFIELD
	},

	//PxGeometryType::eCONVEXMESH
	{
		0,									//PxGeometryType::eSPHERE
		0,									//PxGeometryType::ePLANE
		0,									//PxGeometryType::eCAPSULE
		0,									//PxGeometryType::eBOX
		GeomMTDCallback_ConvexConvex,		//PxGeometryType::eCONVEXMESH
		GeomMTDCallback_ConvexMesh,			//PxGeometryType::eTRIANGLEMESH
		GeomMTDCallback_ConvexHeightField,	//PxGeometryType::eHEIGHTFIELD
	},

	//PxGeometryType::eTRIANGLEMESH
	{
		0,									//PxGeometryType::eSPHERE
		0,									//PxGeometryType::ePLANE
		0,									//PxGeometryType::eCAPSULE
		0,									//PxGeometryType::eBOX
		0,									//PxGeometryType::eCONVEXMESH
		GeomMTDCallback_NotSupported,		//PxGeometryType::eTRIANGLEMESH
		GeomMTDCallback_NotSupported,		//PxGeometryType::eHEIGHTFIELD
	},

	//PxGeometryType::eHEIGHTFIELD
	{
		0,									//PxGeometryType::eSPHERE
		0,									//PxGeometryType::ePLANE
		0,									//PxGeometryType::eCAPSULE
		0,									//PxGeometryType::eBOX
		0,									//PxGeometryType::eCONVEXMESH
		0,									//PxGeometryType::eTRIANGLEMESH
		GeomMTDCallback_NotSupported,		//PxGeometryType::eHEIGHTFIELD
	},
};
