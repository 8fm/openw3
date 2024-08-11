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
#include "GuVecConvexHull.h"
#include "GuVecShrunkConvexHull.h"
#include "GuVecShrunkConvexHullNoScale.h"
#include "GuVecConvexHullNoScale.h"
#include "GuGeometryUnion.h"   

#include "GuContactMethodImpl.h"
#include "GuPCMShapeConvex.h"
#include "GuPCMContactGen.h"
#include "GuContactBuffer.h"

using namespace physx;
using namespace Gu;

#ifdef	PCM_LOW_LEVEL_DEBUG
#include "CmRenderOutput.h"
extern physx::Cm::RenderOutput* gRenderOutPut;
#endif

namespace physx
{
namespace Gu
{


static bool fullContactsGenerationConvexConvex(const Gu::ConvexHullV& convexHull0, Gu::ConvexHullV& convexHull1, const Ps::aos::PsTransformV& transf0, const Ps::aos::PsTransformV& transf1, 
											   const bool idtScale0, const bool idtScale1, Gu::PersistentContact* manifoldContacts, PxU32& numContacts, Gu::ContactBuffer& contactBuffer, Gu::PersistentContactManifold& manifold, Ps::aos::Vec3VArg normal, 
											   const Ps::aos::FloatVArg contactDist,  const bool doOverlapTest)
{
	using namespace Ps::aos;
	Gu::PolygonalData polyData0, polyData1;
	getPCMConvexData(convexHull0.hullData, idtScale0, polyData0);
	getPCMConvexData(convexHull1.hullData, idtScale1, polyData1);

	PxU8 buff0[sizeof(SupportLocalImpl<ConvexHullV>)];
	PxU8 buff1[sizeof(SupportLocalImpl<ConvexHullV>)];

	SupportLocal* map0 = (idtScale0 ? (SupportLocal*)PX_PLACEMENT_NEW(buff0, SupportLocalImpl<ConvexHullNoScaleV>)((ConvexHullNoScaleV&)convexHull0, transf0, convexHull0.vertex2Shape, convexHull0.shape2Vertex, idtScale0) : 
		(SupportLocal*)PX_PLACEMENT_NEW(buff0, SupportLocalImpl<ConvexHullV>)(convexHull0, transf0, convexHull0.vertex2Shape, convexHull0.shape2Vertex, idtScale0));

	SupportLocal* map1 = (idtScale1 ? (SupportLocal*)PX_PLACEMENT_NEW(buff1, SupportLocalImpl<ConvexHullNoScaleV>)((ConvexHullNoScaleV&)convexHull1, transf1, convexHull1.vertex2Shape, convexHull1.shape2Vertex, idtScale1) : 
		(SupportLocal*)PX_PLACEMENT_NEW(buff1, SupportLocalImpl<ConvexHullV>)(convexHull1, transf1, convexHull1.vertex2Shape, convexHull1.shape2Vertex, idtScale1));


	//If the origContacts == 1, which means GJK generate  one contact
	PxU32 origContacts = numContacts;

	if(generateFullContactManifold(polyData0, polyData1, map0, map1, manifoldContacts, numContacts, contactDist, normal, doOverlapTest))
	{
		if(numContacts > 0)
		{

			//if we already have a gjk contacts, but the full manifold generate contacts, we need to drop the gjk contact, because the normal
			//will be different
			if(numContacts != origContacts && origContacts != 0)
			{
				numContacts--;
				manifoldContacts++;
			}
		
			//reduce contacts
			manifold.addBatchManifoldContacts(manifoldContacts, numContacts);

			const Vec3V worldNormal =  manifold.getWorldNormal(transf1);
			
			/*PxVec3 normalf;
			V3StoreU(worldNormal, normalf);
			PX_ASSERT(normalf.magnitude() < 1e-3f);*/
			//add the manifold contacts;
			manifold.addManifoldContactsToContactBuffer(contactBuffer, worldNormal, transf1);
			return true;
		
		}
		return false;
	}

	return false;

}

bool pcmContactConvexConvex(GU_CONTACT_METHOD_ARGS)
{
	using namespace Ps::aos;
	const PxConvexMeshGeometryLL& shapeConvex0 = shape0.get<const PxConvexMeshGeometryLL>();
	const PxConvexMeshGeometryLL& shapeConvex1 = shape1.get<const PxConvexMeshGeometryLL>();
	Gu::PersistentContactManifold& manifold = cache.getManifold();
	
	//gRenderOutPut = cache.mRenderOutput;

	Ps::prefetchLine(shapeConvex0.hullData);
	Ps::prefetchLine(shapeConvex1.hullData);


	PX_ASSERT(transform1.q.isSane());
	PX_ASSERT(transform0.q.isSane());

	const FloatV zero = FZero();
	const Vec3V zeroV = V3Zero();

	const QuatV q0 = QuatVLoadA(&transform0.q.x);
	const Vec3V p0 = V3LoadA(&transform0.p.x);

	const QuatV q1 = QuatVLoadA(&transform1.q.x);
	const Vec3V p1 = V3LoadA(&transform1.p.x);

	const Vec3V vScale0 = V3LoadU(shapeConvex0.scale.scale);
	const Vec3V vScale1 = V3LoadU(shapeConvex1.scale.scale);
	const FloatV contactDist = FLoad(contactDistance);

	//Transfer A into the local space of B
	const PsTransformV transf0(p0, q0);
	const PsTransformV transf1(p1, q1);
	const PsTransformV curRTrans(transf1.transformInv(transf0));

	//const PsTransformV curRTrans(transform1.transformInv(transform0));
	const PsMatTransformV aToB(curRTrans);
	
	const Gu::ConvexHullData* hullData0 = shapeConvex0.hullData;
	const Gu::ConvexHullData* hullData1 = shapeConvex1.hullData;

	//const PsTransformV transf1(p1, q1); 
	const FloatV convexMargin0 = Gu::CalculatePCMConvexMargin(hullData0, vScale0);
	const FloatV convexMargin1 = Gu::CalculatePCMConvexMargin(hullData1, vScale1);
	
	const PxU32 initialContacts = manifold.mNumContacts;
	
	const FloatV minMargin = FMin(convexMargin0, convexMargin1);
	//const FloatV distanceBreakingThreshold = FMul(minMargin, FloatV_From_F32(0.05f));
	const FloatV projectBreakingThreshold = FMul(minMargin, FLoad(0.8f));
	//const FloatV projectBreakingThreshold = FMul(minMargin, FloatV_From_F32(0.2f));
	const FloatV replaceBreakingThreshold = FMul(minMargin, FLoad(0.05f));
	
	manifold.refreshContactPoints(aToB, projectBreakingThreshold, contactDist);
	const PxU32 newContacts = manifold.mNumContacts;
	const bool bLostContacts = (newContacts != initialContacts);

	PxGJKStatus status = manifold.mNumContacts > 0 ? GJK_UNDEFINED : GJK_NON_INTERSECT;

	Vec3V closestA(zeroV), closestB(zeroV), normal(zeroV); // from a to b
	FloatV penDep = zero;
	//gRenderOutPut = cache.mRenderOutput;

	if(bLostContacts || manifold.invalidate(curRTrans, minMargin))
	{

		const bool idtScale0 = shapeConvex0.scale.isIdentity();
		const bool idtScale1 = shapeConvex1.scale.isIdentity();
		const QuatV vQuat0 = QuatVLoadU(&shapeConvex0.scale.rotation.x);
		const QuatV vQuat1 = QuatVLoadU(&shapeConvex1.scale.rotation.x);
		Gu::ShrunkConvexHullV convexHull0(hullData0, zeroV, vScale0, vQuat0);
		Gu::ShrunkConvexHullV convexHull1(hullData1, zeroV, vScale1, vQuat1);
		if(idtScale0)
		{
			if(idtScale1)
			{
				status = Gu::GJKRelativePenetration((Gu::ShrunkConvexHullNoScaleV&)convexHull0, (Gu::ShrunkConvexHullNoScaleV&)convexHull1, aToB, contactDist, closestA, closestB, normal, penDep,
								manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints);
			}
			else
			{
				status = Gu::GJKRelativePenetration((Gu::ShrunkConvexHullNoScaleV&)convexHull0, convexHull1, aToB, contactDist, closestA, closestB, normal, penDep,
							manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints);
			}
		}
		else
		{
			if(idtScale1)
			{
				status = Gu::GJKRelativePenetration(convexHull0, (Gu::ShrunkConvexHullNoScaleV&)convexHull1, aToB, contactDist, closestA, closestB, normal, penDep,
								manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints);
			}
			else
			{
				status = Gu::GJKRelativePenetration(convexHull0, convexHull1, aToB, contactDist, closestA, closestB, normal, penDep,
							manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints);
			}

		}

		manifold.setRelativeTransform(curRTrans);

	//	return fullContactsGenerationConvexConvex(convexHull0, convexHull1, transf0, transf1, idtScale0, idtScale1, contactBuffer, manifold, normal, contactDist, true);

		Gu::PersistentContact* manifoldContacts = PX_CP_TO_PCP(contactBuffer.contacts);
		PxU32 numContacts = 0;
		if(status == GJK_DEGENERATE)
		{
			return fullContactsGenerationConvexConvex(convexHull0, convexHull1, transf0, transf1, idtScale0, idtScale1, manifoldContacts, numContacts, contactBuffer, manifold, normal, contactDist, true);
		}
		else if(status == GJK_NON_INTERSECT)
		{
			return false;
		}
		else
		{
			if(manifold.mNumContacts > 0)
			{
				bool doOverlapTest  = false;
				if(status == GJK_CONTACT)
				{
			
					const Vec3V localPointA = aToB.transformInv(closestA);//curRTrans.transformInv(closestA);
					const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(normal), penDep);

					//Add contact to contact stream
					manifoldContacts[numContacts].mLocalPointA = localPointA;
					manifoldContacts[numContacts].mLocalPointB = closestB;
					manifoldContacts[numContacts++].mLocalNormalPen = localNormalPen;

					//Add contact to manifold
					manifold.addManifoldPoint(localPointA, closestB, localNormalPen, replaceBreakingThreshold);
				}
				else 
				{
					PX_ASSERT(status == EPA_CONTACT);
				
					status = Gu::EPARelativePenetration((Gu::ConvexHullV&)convexHull0, (Gu::ConvexHullV&)convexHull1, aToB,closestA, closestB, normal, penDep,
										manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints);
					if(status == EPA_CONTACT)
					{
						const Vec3V v1 = V3Sub(closestB, closestA);  
						const FloatV dist = V3Length(v1);
						normal = V3ScaleInv(v1, dist);//V3Scale(v1, FRecip(dist));
						penDep = FNeg(dist);

						
						const Vec3V localPointA = aToB.transformInv(closestA);//curRTrans.transformInv(closestA);
						const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(normal), penDep);
						
						//Add contact to contact stream
						manifoldContacts[numContacts].mLocalPointA = localPointA;
						manifoldContacts[numContacts].mLocalPointB = closestB;
						manifoldContacts[numContacts++].mLocalNormalPen = localNormalPen;

						//Add contact to manifold
						manifold.addManifoldPoint(localPointA, closestB, localNormalPen, replaceBreakingThreshold);
					}
					else
					{
						doOverlapTest = true;
					}
				}

				if(manifold.mNumContacts < initialContacts || doOverlapTest)
				{
					return fullContactsGenerationConvexConvex(convexHull0, convexHull1, transf0, transf1, idtScale0, idtScale1, manifoldContacts, numContacts, contactBuffer, manifold, normal, contactDist, doOverlapTest);
				}
				else
				{
					//normal = transf1.rotate(normal);
					const Vec3V worldNormal = manifold.getWorldNormal(transf1);
					manifold.addManifoldContactsToContactBuffer(contactBuffer, worldNormal, transf1);
					return true;
				}
			}
			else
			{
				if(status == GJK_CONTACT)
				{
					const Vec3V localPointA = aToB.transformInv(closestA);//curRTrans.transformInv(closestA);
					const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(normal), penDep);

					//Add contact to contact stream
					manifoldContacts[numContacts].mLocalPointA = localPointA;
					manifoldContacts[numContacts].mLocalPointB = closestB;
					manifoldContacts[numContacts++].mLocalNormalPen = localNormalPen;
				}
				bool doOverlapTest = (status == EPA_CONTACT) ? true : false;
				return fullContactsGenerationConvexConvex(convexHull0, convexHull1, transf0, transf1, idtScale0, idtScale1, manifoldContacts, numContacts, 
					contactBuffer, manifold, normal, contactDist, doOverlapTest);
			}
		}
	}
	else
	{
		const Vec3V worldNormal =  manifold.getWorldNormal(transf1);
		manifold.addManifoldContactsToContactBuffer(contactBuffer, worldNormal, transf1);
		return  manifold.getNumContacts()> 0;
	}
	
}
//#pragma optimize("", on)
}
}
