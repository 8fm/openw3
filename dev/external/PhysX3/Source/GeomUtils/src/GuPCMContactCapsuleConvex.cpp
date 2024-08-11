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
#include "GuPCMContactGen.h"
#include "GuPCMShapeConvex.h"

#ifdef	PCM_LOW_LEVEL_DEBUG
#include "CmRenderOutput.h"
extern physx::Cm::RenderOutput* gRenderOutPut;
#endif

using namespace physx;
using namespace Gu;

namespace physx
{
namespace Gu
{

static bool fullContactsGenerationCapsuleConvex(const Gu::CapsuleV& capsule, const Gu::ConvexHullV& convexHull,  const Ps::aos::PsMatTransformV& aToB, const Ps::aos::PsTransformV& transf0,const Ps::aos::PsTransformV& transf1,
								Gu::PersistentContact* manifoldContacts, PxU32& numContacts, Gu::ContactBuffer& contactBuffer, const bool idtScale, Gu::PersistentContactManifold& manifold, Ps::aos::Vec3VArg normal, const Ps::aos::FloatVArg contactDist, const bool doOverlapTest)
{

	using namespace Ps::aos;
	Gu::PolygonalData polyData;
	getPCMConvexData(convexHull.hullData, idtScale, polyData);

	PxU8 buff[sizeof(SupportLocalImpl<ConvexHullV>)];
	SupportLocal* map = (idtScale ? (SupportLocal*)PX_PLACEMENT_NEW(buff, SupportLocalImpl<ConvexHullNoScaleV>)((ConvexHullNoScaleV&)convexHull, transf1, convexHull.vertex2Shape, convexHull.shape2Vertex, idtScale) : 
	(SupportLocal*)PX_PLACEMENT_NEW(buff, SupportLocalImpl<ConvexHullV>)(convexHull, transf1, convexHull.vertex2Shape, convexHull.shape2Vertex, idtScale));

	/*Gu::PersistentContact* manifoldContacts = PX_CP_TO_PCP(contactBuffer.contacts);
	PxU32 numContacts = 0;*/

	PxU32 origContacts = numContacts;
	if(generateFullContactManifold(capsule, polyData, map, aToB, manifoldContacts, numContacts, contactDist, normal, doOverlapTest))
	{
		//EPA has contacts and we have new contacts, we discard the EPA contacts
		if(origContacts != 0 && numContacts != origContacts)
		{
			numContacts--;
			manifoldContacts++;
		}
		manifold.addBatchManifoldContacts2(manifoldContacts, numContacts);
		//transform normal into the world space
		normal = transf1.rotate(normal);
		manifold.addManifoldContactsToContactBuffer(contactBuffer, normal, transf0, capsule.radius);
		//manifold.addManifoldContactsToContactBuffer(contactBuffer, normal, transf1);
		//manifold.drawManifold(*gRenderOutPut, transf0, transf1);

		return numContacts > 0;
	}
	return false;

}


bool pcmContactCapsuleConvex(GU_CONTACT_METHOD_ARGS)
{
	using namespace Ps::aos;

	const PxConvexMeshGeometryLL& shapeConvex = shape1.get<const PxConvexMeshGeometryLL>();
	const PxCapsuleGeometry& shapeCapsule = shape0.get<const PxCapsuleGeometry>();

	Gu::PersistentContactManifold& manifold = cache.getManifold();

	Ps::prefetchLine(shapeConvex.hullData);

		
	PX_ASSERT(transform1.q.isSane());
	PX_ASSERT(transform0.q.isSane());

	const Vec3V zeroV = V3Zero();

	const QuatV q0 = QuatVLoadA(&transform0.q.x);
	const Vec3V p0 = V3LoadA(&transform0.p.x);

	const QuatV q1 = QuatVLoadA(&transform1.q.x);
	const Vec3V p1 = V3LoadA(&transform1.p.x);

	const Vec3V vScale = V3LoadU(shapeConvex.scale.scale);

	const FloatV contactDist = FLoad(contactDistance);
	const FloatV capsuleHalfHeight = FLoad(shapeCapsule.halfHeight);
	const FloatV capsuleRadius = FLoad(shapeCapsule.radius);
	const Gu::ConvexHullData* hullData =shapeConvex.hullData;
	
	//Transfer A into the local space of B
	const PsTransformV transf0(p0, q0);
	const PsTransformV transf1(p1, q1);
	const PsTransformV curRTrans(transf1.transformInv(transf0));
	//const PsTransformV curRTrans(transform1.transformInv(transform0));
	const PsMatTransformV aToB(curRTrans);
	//const PsTransformV transf1(p1, q1);

	const FloatV convexMargin = Gu::CalculatePCMConvexMargin(hullData, vScale);
	const FloatV capsuleMinMargin = Gu::CalculateCapsuleMinMargin(capsuleRadius);
	const FloatV minMargin = FMin(convexMargin, capsuleMinMargin);
	
	const PxU32 initialContacts = manifold.mNumContacts;
	const FloatV projectBreakingThreshold = FMul(minMargin, FLoad(0.8f));
	const FloatV refreshDist = FAdd(contactDist, capsuleRadius);
	manifold.refreshContactPoints(aToB,  projectBreakingThreshold, refreshDist);
	//manifold.refreshContactPoints(aToB,  projectBreakingThreshold, contactDist);

	const PxU32 newContacts = manifold.mNumContacts;
	const bool bLostContacts = (newContacts != initialContacts);

	PxGJKStatus status = manifold.mNumContacts > 0 ? GJK_UNDEFINED : GJK_NON_INTERSECT;

	Vec3V closestA(zeroV), closestB(zeroV), normal(zeroV); // from a to b
	const FloatV zero = FZero();
	FloatV penDep = zero;
	//manifold.numContacts = 0;
	//gRenderOutPut = cache.mRenderOutput;
	
	if(bLostContacts || manifold.invalidate2(curRTrans, minMargin))
	{

		manifold.setRelativeTransform(curRTrans);
		const QuatV vQuat = QuatVLoadU(&shapeConvex.scale.rotation.x);  
		Gu::ConvexHullV convexHull(hullData, zeroV, vScale, vQuat);
		convexHull.setMargin(zero);

		//transform capsule(a) into the local space of convexHull(b)
		const Gu::CapsuleV capsule(aToB.p, aToB.rotate(V3Scale(V3UnitX(), capsuleHalfHeight)), capsuleRadius);

		const bool idtScale = shapeConvex.scale.isIdentity();
		if(idtScale)
		{
			
			status = Gu::GJKLocalPenetration(capsule, *PX_CONVEX_TO_NOSCALECONVEX(&convexHull), contactDist, closestA, closestB, normal, penDep, manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints, true);
		}
		else
		{
			status = Gu::GJKLocalPenetration(capsule, convexHull, contactDist, closestA, closestB, normal, penDep, manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints, true);

		}

		Gu::PersistentContact* manifoldContacts = PX_CP_TO_PCP(contactBuffer.contacts);
		PxU32 numContacts = 0;
		bool doOverlapTest = false;
		if(status == GJK_NON_INTERSECT)
		{
			return false;
		}
		else if(status == GJK_DEGENERATE)
		{
			return fullContactsGenerationCapsuleConvex(capsule, convexHull, aToB, transf0, transf1, manifoldContacts, numContacts, contactBuffer, idtScale, manifold, normal, contactDist, true);
		}
		else 
		{
			if(status == EPA_CONTACT)
			{
				//status == EPA_CONTACT
				status= Gu::EPALocalPenetration(capsule, convexHull, closestA, closestB, normal, penDep,
				manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints);
				
				if(status == EPA_CONTACT)
				{
					const Vec3V v = V3Sub(closestB, closestA);
					const FloatV length = V3Length(v);
					normal = V3ScaleInv(v, length);
					penDep = FNeg(length);

					const Vec3V localPointA = aToB.transformInv(closestA);//curRTrans.transformInv(closestA);
					const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(normal), penDep);
					//Add contact to contact stream
					manifoldContacts[numContacts].mLocalPointA = localPointA;
					manifoldContacts[numContacts].mLocalPointB = closestB;
					manifoldContacts[numContacts++].mLocalNormalPen = localNormalPen;

				}
				else
				{
					doOverlapTest = true;
				}
			}
			
		
			if(initialContacts == 0 || bLostContacts || doOverlapTest)
			{
				return fullContactsGenerationCapsuleConvex(capsule, convexHull, aToB, transf0, transf1, manifoldContacts, numContacts, contactBuffer, idtScale, manifold, normal, contactDist, doOverlapTest);
			}
			else
			{
				
				//manifold.drawManifold(*gRenderOutPut, transf0, transf1);
				//This contact is either come from GJK or EPA
				const FloatV replaceBreakingThreshold = FMul(minMargin, FLoad(0.05f));
				const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(normal), penDep);
				manifold.addManifoldPoint2(aToB.transformInv(closestA), closestB, localNormalPen, replaceBreakingThreshold);
				normal = transf1.rotate(normal);
				manifold.addManifoldContactsToContactBuffer(contactBuffer, normal, transf0, capsuleRadius);
				//manifold.addManifoldContactsToContactBuffer(contactBuffer, normal, transf1);

				return true;
			}
		}	
	}
	else
	{
		normal = manifold.getWorldNormal(transf1);
		//manifold.addManifoldContactsToContactBuffer(contactBuffer, normal, transf1);
		manifold.addManifoldContactsToContactBuffer(contactBuffer, normal, transf0, capsuleRadius);
		//manifold.drawManifold(*gRenderOutPut, transf0, transf1);
		return manifold.getNumContacts() > 0;
	}
}

}//Gu
}//physx
