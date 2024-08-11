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


#include "GuGJKPenetrationWrapper.h"
#include "GuEPAPenetrationWrapper.h"
#include "GuVecCapsule.h"
#include "GuVecConvexHull.h"
#include "GuVecConvexHullNoScale.h"
#include "GuGeometryUnion.h"

#include "GuContactMethodImpl.h"
#include "GuContactBuffer.h"

namespace physx
{
namespace Gu
{

bool pcmContactSphereConvex(GU_CONTACT_METHOD_ARGS)
{
	using namespace Ps::aos;

	PX_ASSERT(transform1.q.isSane());
	PX_ASSERT(transform0.q.isSane());
	
	
	const PxConvexMeshGeometryLL& shapeConvex = shape1.get<const PxConvexMeshGeometryLL>();
	const PxSphereGeometry& shapeSphere = shape0.get<const PxSphereGeometry>();

	Gu::PersistentContactManifold& manifold = cache.getManifold();

	const Vec3V zeroV = V3Zero();

	const QuatV q0 = QuatVLoadA(&transform0.q.x);
	const Vec3V p0 = V3LoadA(&transform0.p.x);

	Ps::prefetchLine(shapeConvex.hullData);
	const QuatV q1 = QuatVLoadA(&transform1.q.x);
	const Vec3V p1 = V3LoadA(&transform1.p.x);
	const Vec3V vScale = V3LoadU(shapeConvex.scale.scale);
	const FloatV sphereRadius = FLoad(shapeSphere.radius);
	const FloatV contactDist = FLoad(contactDistance);
	const Gu::ConvexHullData* hullData = shapeConvex.hullData;
	
	//Transfer A into the local space of B
	const PsTransformV transf0(p0, q0);
	const PsTransformV transf1(p1, q1);
	const PsTransformV curRTrans(transf1.transformInv(transf0));
	//const PsTransformV curRTrans(transform1.transformInv(transform0));
	const PsMatTransformV aToB(curRTrans);
	
	
	const FloatV convexMargin = Gu::CalculatePCMConvexMargin(hullData, vScale);

	const PxU32 initialContacts = manifold.mNumContacts;
	const FloatV minMargin = FMin(convexMargin, sphereRadius);
	//const FloatV distanceBreakingThreshold = FMul(minMargin, FloatV_From_F32(0.05f));
	const FloatV projectBreakingThreshold = FMul(minMargin, FLoad(0.05f));
	
	const FloatV refreshDistance = FAdd(sphereRadius, contactDist);
	//manifold.refreshContactPoints(aToB, projectBreakingThreshold, contactDist);
	manifold.refreshContactPoints(aToB, projectBreakingThreshold, refreshDistance);
	
	const PxU32 newContacts = manifold.mNumContacts;

	const bool bLostContacts = (newContacts != initialContacts);
	//const bool bLostContacts = ((initialContacts == 0) || (newContacts != initialContacts));

	PxGJKStatus status = manifold.mNumContacts > 0 ? GJK_UNDEFINED : GJK_NON_INTERSECT;

		
	Vec3V closestA(zeroV), closestB(zeroV);
	Vec3V normal(zeroV); // from a to b
	const FloatV zero = FZero();
	FloatV penDep = zero;


	if(bLostContacts || manifold.invalidate2(curRTrans, minMargin))
	{
		
		const QuatV vQuat = QuatVLoadU(&shapeConvex.scale.rotation.x);  
	
		Gu::ConvexHullV convexHull(hullData, zeroV, vScale, vQuat);
		convexHull.setMargin(zero);
		//transform capsule into the local space of convexHull
		const Gu::CapsuleV capsule(aToB.p, sphereRadius);

		const bool idtScale0 = shapeConvex.scale.isIdentity();
		if(idtScale0)
		{
			  
			status = Gu::GJKLocalPenetration(capsule, *PX_CONVEX_TO_NOSCALECONVEX(&convexHull), contactDist, closestA, closestB, normal, penDep, manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints, true);
		}
		else
		{
			status = Gu::GJKLocalPenetration(capsule, convexHull, contactDist, closestA, closestB, normal, penDep, manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints, true);

		}

		if(status == EPA_CONTACT)
		{
			status= Gu::EPALocalPenetration(capsule, convexHull, closestA, closestB, normal, penDep,
				manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints);

			const Vec3V v = V3Sub(closestB, closestA);
			const FloatV sqLength = V3Dot(v, v);
			
			if(FAllEq(sqLength, zero))
			{
				normal = V3UnitY();
				penDep = zero;
			}
			else
			{
				const FloatV length = FSqrt(sqLength);
				penDep = FNeg(length);
				normal = V3ScaleInv(v, length);
				if(status == EPA_FAIL)
				{
					normal = V3Neg(normal);
				}
			}
			status = GJK_CONTACT;
		}
	
		
		manifold.setRelativeTransform(curRTrans);
	}

	if(status)
	{
		Gu::PersistentContact& p = manifold.getContactPoint(0);

		if(status == GJK_CONTACT)
		{
			//p.mLocalPointA = aToB.transformInv(closestA);//curRTrans.transformInv(closestA);
			p.mLocalPointA = zeroV;//sphere center
			p.mLocalPointB = closestB;
			p.mLocalNormalPen = V4SetW(Vec4V_From_Vec3V(normal), penDep);
			manifold.mNumContacts =1;

			Gu::ContactPoint& contact = contactBuffer.contacts[contactBuffer.count++];
			//const Vec3V worldP1 = transf1.transform(closestB);
			//transform normal to world space
			normal = transf1.rotate(normal);
			const Vec3V worldP = V3NegScaleSub(normal, sphereRadius, transf0.p);
			penDep = FSub(penDep, sphereRadius);
			
			V4StoreA(Vec4V_From_Vec3V(normal), (PxF32*)&contact.normal.x);
			V4StoreA(Vec4V_From_Vec3V(worldP), (PxF32*)&contact.point.x);
			FStore(penDep, &contact.separation);

			contact.internalFaceIndex0 = PXC_CONTACT_NO_FACE_INDEX;
			contact.internalFaceIndex1 = PXC_CONTACT_NO_FACE_INDEX;
			
		}
		else
		{
			/*const Vec3V worldP = transf1.transform(p.mLocalPointB);*/
			normal = transf1.rotate(Vec3V_From_Vec4V(p.mLocalNormalPen));
			const Vec3V worldP = V3NegScaleSub(normal, sphereRadius, transf0.p);
			penDep = FSub(V4GetW(p.mLocalNormalPen), sphereRadius);
		
			Gu::ContactPoint& contact = contactBuffer.contacts[contactBuffer.count++];
			V4StoreA(Vec4V_From_Vec3V(normal), (PxF32*)&contact.normal.x);
			V4StoreA(Vec4V_From_Vec3V(worldP), (PxF32*)&contact.point.x);
			FStore(penDep, &contact.separation);

			contact.internalFaceIndex0 = PXC_CONTACT_NO_FACE_INDEX;
			contact.internalFaceIndex1 = PXC_CONTACT_NO_FACE_INDEX;
			
		}
		return true;

		//manifold.drawManifold(&context, transf0, transf1, status == GJK_CONTACT);

	}

	return false;

}
}//Gu
}//phyxs

