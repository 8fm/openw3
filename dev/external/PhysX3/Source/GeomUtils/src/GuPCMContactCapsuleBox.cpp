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
#include "GuVecBox.h"
#include "GuVecCapsule.h"
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

static bool fullContactsGenerationCapsuleBox(const Gu::CapsuleV& capsule, const Gu::BoxV& box, const PxVec3 halfExtents,  const Ps::aos::PsMatTransformV& aToB, const Ps::aos::PsTransformV& transf0,const Ps::aos::PsTransformV& transf1,
								Gu::PersistentContact* manifoldContacts, PxU32& numContacts, Gu::ContactBuffer& contactBuffer, Gu::PersistentContactManifold& manifold, Ps::aos::Vec3VArg normal, const Ps::aos::FloatVArg contactDist, const bool doOverlapTest)
{
	//PX_UNUSED(transf0);

	using namespace Ps::aos;
	Gu::PolygonalData polyData;
	PCMPolygonalBox polyBox(halfExtents);
	polyBox.getPolygonalData(&polyData);

	Mat33V identity = M33Identity();
	SupportLocalImpl<BoxV> map(box, transf1, identity, identity);
	

	PxU32 origContacts = numContacts;
	if(generateCapsuleBoxFullContactManifold(capsule, box,polyData, &map, aToB,  manifoldContacts, numContacts, contactDist, normal, doOverlapTest))
	{
		//EPA has contacts and we have new contacts, we discard the EPA contacts
		if(origContacts != 0 && numContacts != origContacts)
		{
			numContacts--;
			manifoldContacts++;
		}

		manifold.addBatchManifoldContacts2(manifoldContacts, numContacts);
		
		normal = transf1.rotate(normal);
		
		//manifold.drawManifold(*gRenderOutPut, transf0, transf1);
		//manifold.addManifoldContactsToContactBuffer(contactBuffer, normal, transf1);
		manifold.addManifoldContactsToContactBuffer(contactBuffer, normal, transf0, capsule.radius);

		return numContacts > 0;
	}

	return false;

}


bool pcmContactCapsuleBox(GU_CONTACT_METHOD_ARGS)
{
	using namespace Ps::aos;

	Gu::PersistentContactManifold& manifold = cache.getManifold();
	/*Ps::prefetchLine(&manifold);
	Ps::prefetchLine(&manifold, 128);*/
	Ps::prefetchLine(&manifold, 256);
	const PxCapsuleGeometry& shapeCapsule = shape0.get<const PxCapsuleGeometry>();
	const PxBoxGeometry& shapeBox = shape1.get<const PxBoxGeometry>();

	PX_ASSERT(transform1.q.isSane());
	PX_ASSERT(transform0.q.isSane());  

	const Vec3V zeroV = V3Zero();
	const Vec3V boxExtents = V3LoadU(shapeBox.halfExtents);

	const QuatV q0 = QuatVLoadA(&transform0.q.x);
	const Vec3V p0 = V3LoadA(&transform0.p.x);

	const QuatV q1 = QuatVLoadA(&transform1.q.x);
	const Vec3V p1 = V3LoadA(&transform1.p.x);

	const FloatV contactDist = FLoad(contactDistance);

	const PsTransformV transf0(p0, q0);
	const PsTransformV transf1(p1, q1);

	const PsTransformV curRTrans = transf1.transformInv(transf0);

	const FloatV capsuleRadius = FLoad(shapeCapsule.radius);
	const FloatV capsuleHalfHeight = FLoad(shapeCapsule.halfHeight);

	const PxU32 initialContacts = manifold.mNumContacts;

	const FloatV boxMargin = Gu::CalculatePCMBoxMargin(boxExtents);
	
	const FloatV minMargin = FMin(boxMargin, capsuleRadius);

	//const FloatV distanceBreakingThreshold = FMul(minMargin, FloatV_From_F32(0.25f));
	const FloatV projectBreakingThreshold = FMul(minMargin, FLoad(0.8f));
	
	//manifold.refreshContactPoints(curRTrans, projectBreakingThreshold, contactDist);
	const FloatV refreshDist = FAdd(contactDist, capsuleRadius);
	manifold.refreshContactPoints(curRTrans, projectBreakingThreshold, refreshDist);

	const PxU32 newContacts = manifold.mNumContacts;
	const bool bLostContacts = (newContacts != initialContacts);//((initialContacts == 0) || (newContacts != initialContacts));

	PxGJKStatus status = manifold.mNumContacts > 0 ? GJK_UNDEFINED : GJK_NON_INTERSECT;

	Vec3V closestA(zeroV), closestB(zeroV);
	Vec3V normal(zeroV); // from a to b
	const FloatV zero = FZero();
	FloatV penDep = zero;

	//gRenderOutPut = cache.mRenderOutput;
	//manifold.mNumContacts = 0;

	if(bLostContacts || manifold.invalidate2(curRTrans, minMargin))	
	{

		manifold.setRelativeTransform(curRTrans);
		const PsMatTransformV aToB(curRTrans);
		
		//const Gu::BoxV box(transf1.p, boxExtents);
		Gu::BoxV box(zeroV, boxExtents);
		box.setMargin(zero);
		//transform capsule into the local space of box
		const Gu::CapsuleV capsule(aToB.p, aToB.rotate(V3Scale(V3UnitX(), capsuleHalfHeight)), capsuleRadius);

		
		status =  Gu::GJKLocalPenetration(capsule, box, contactDist, closestA, closestB, normal, penDep, manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints, true);

		Gu::PersistentContact* manifoldContacts = PX_CP_TO_PCP(contactBuffer.contacts);
		PxU32 numContacts = 0;
		bool doOverlapTest = false;
		if(status == GJK_NON_INTERSECT)
		{
			return false;
		}
		else if(status == GJK_DEGENERATE)
		{
			return fullContactsGenerationCapsuleBox(capsule, box, shapeBox.halfExtents,  aToB, transf0, transf1, manifoldContacts, numContacts, contactBuffer, manifold, normal, contactDist, true);
		}
		else 
		{
			if(status == EPA_CONTACT)
			{
				status= Gu::EPALocalPenetration(capsule, box, closestA, closestB, normal, penDep,
				manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints);

				if(status == EPA_CONTACT)
				{
					//status == EPA_CONTACT
					const Vec3V v = V3Sub(closestB, closestA);
					const FloatV length = V3Length(v);
					normal = V3ScaleInv(v, length);
					penDep = FNeg(length);
					/*penDep = FSub(FNeg(length), capsuleRadius);
					closestA = V3NegScaleSub(normal, capsuleRadius, closestA);*/

					
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

				return fullContactsGenerationCapsuleBox(capsule, box, shapeBox.halfExtents,  aToB, transf0, transf1, manifoldContacts, numContacts, contactBuffer, manifold, normal, contactDist, doOverlapTest);
			}
			else
			{
				//manifold.drawManifold(*gRenderOutPut, transf0, transf1);
				
				//The contacts is either come from GJK or EPA
				const FloatV replaceBreakingThreshold = FMul(minMargin, FLoad(0.1f));
				const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(normal), penDep);
				manifold.addManifoldPoint2(curRTrans.transformInv(closestA), closestB, localNormalPen, replaceBreakingThreshold);
				
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
}
}
