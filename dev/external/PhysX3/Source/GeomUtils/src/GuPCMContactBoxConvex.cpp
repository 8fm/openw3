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

#include "GuGJKPenetration.h"
#include "GuEPA.h"
//#include "GuGJKPenetrationWrapper.h"
#include "GuEPAPenetrationWrapper.h"
#include "GuVecBox.h"
#include "GuVecShrunkBox.h"
#include "GuVecShrunkConvexHull.h"
#include "GuVecShrunkConvexHullNoScale.h"
#include "GuVecConvexHull.h"
#include "GuVecConvexHullNoScale.h"
#include "GuGeometryUnion.h"

#include "GuContactMethodImpl.h"
#include "GuPCMContactGen.h"
#include "GuPCMShapeConvex.h"
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

//#pragma optimize("", off)

static bool fullContactsGenerationBoxConvex(const PxVec3& halfExtents, const Gu::BoxV& box, Gu::ConvexHullV& convexHull, const Ps::aos::PsTransformV& transf0, const Ps::aos::PsTransformV& transf1, 
									Gu::PersistentContact* manifoldContacts, PxU32& numContacts, Gu::ContactBuffer& contactBuffer, Gu::PersistentContactManifold& manifold, Ps::aos::Vec3VArg normal, const Ps::aos::FloatVArg contactDist, const bool idtScale, const bool doOverlapTest)
{
	using namespace Ps::aos;
	Gu::PolygonalData polyData0;
	PCMPolygonalBox polyBox0(halfExtents);
	polyBox0.getPolygonalData(&polyData0);
	polyData0.mPolygonVertexRefs = gPCMBoxPolygonData;
	
	Gu::PolygonalData polyData1;
	getPCMConvexData(convexHull.hullData, idtScale, polyData1);
	
	Mat33V identity =  M33Identity();
	SupportLocalImpl<BoxV> map0(box, transf0, identity, identity, true);
	//SupportLocalImpl<ConvexHullV> map1(convexHull, transf1, convexHull.vertex2Shape, convexHull.shape2Vertex);

	PxU8 buff1[sizeof(SupportLocalImpl<ConvexHullV>)];

	SupportLocal* map1 = (idtScale ? (SupportLocal*)PX_PLACEMENT_NEW(buff1, SupportLocalImpl<ConvexHullNoScaleV>)((ConvexHullNoScaleV&)convexHull, transf1, convexHull.vertex2Shape, convexHull.shape2Vertex, idtScale) : 
		(SupportLocal*)PX_PLACEMENT_NEW(buff1, SupportLocalImpl<ConvexHullV>)(convexHull, transf1, convexHull.vertex2Shape, convexHull.shape2Vertex, idtScale));

	//If the origContacts == 1, which means GJK generate  one contact
	PxU32 origContacts = numContacts;
	if(generateFullContactManifold(polyData0, polyData1, &map0, map1, manifoldContacts, numContacts, contactDist, normal, doOverlapTest))
	{
		if(numContacts > 0)
		{
			//PxU32 currentContact = numContacts;
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

			//manifold.drawManifold(*gRenderOutPut, transf0, transf1);
		
		/*	PxVec3 normalf;
			V3StoreU(worldNormal, normalf);
			PX_ASSERT((normalf.magnitude() - 1) < 1e-3f);*/
			//add the manifold contacts;
			manifold.addManifoldContactsToContactBuffer(contactBuffer, worldNormal, transf1);
		}
		return true;
	}

	return false;

}

bool pcmContactBoxConvex(GU_CONTACT_METHOD_ARGS)
{
	using namespace Ps::aos;

	const PxConvexMeshGeometryLL& shapeConvex = shape1.get<const PxConvexMeshGeometryLL>();
	const PxBoxGeometry& shapeBox = shape0.get<const PxBoxGeometry>();
	
	Gu::PersistentContactManifold& manifold = cache.getManifold();
	Ps::prefetchLine(shapeConvex.hullData);
	
	PX_ASSERT(transform1.q.isSane());
	PX_ASSERT(transform0.q.isSane());

	const Vec3V zeroV = V3Zero();

	const QuatV q0 = QuatVLoadA(&transform0.q.x);
	const Vec3V p0 = V3LoadA(&transform0.p.x);

	const QuatV q1 = QuatVLoadA(&transform1.q.x);
	const Vec3V p1 = V3LoadA(&transform1.p.x);

	const FloatV contactDist = FLoad(contactDistance);
	const Vec3V boxExtents = V3LoadU(shapeBox.halfExtents);

	const Vec3V vScale = V3LoadU(shapeConvex.scale.scale);
	
	
	//Transfer A into the local space of B
	const PsTransformV transf0(p0, q0);
	const PsTransformV transf1(p1, q1);
	const PsTransformV curRTrans(transf1.transformInv(transf0));
	//const PsTransformV curRTrans(transform1.transformInv(transform0));
	const PsMatTransformV aToB(curRTrans);
	
	const Gu::ConvexHullData* hullData = shapeConvex.hullData;
	const FloatV convexMargin = Gu::CalculatePCMConvexMargin(hullData, vScale);
	const FloatV boxMargin = Gu::CalculatePCMBoxMargin(boxExtents);

	const FloatV minMargin = FMin(convexMargin, boxMargin);//FMin(boxMargin, convexMargin);
	const FloatV projectBreakingThreshold = FMul(minMargin, FLoad(0.8f));
	//This is needed when we just produce one point of contacts
	//const FloatV projectBreakingThreshold = FMul(minMargin, FloatV_From_F32(1.3f));
	const FloatV replaceBreakingThreshold = FMul(minMargin, FLoad(0.05f));
	//const FloatV replaceBreakingThreshold = FMul(minMargin, FloatV_From_F32(0.15f));
	const PxU32 initialContacts = manifold.mNumContacts;

	manifold.refreshContactPoints(aToB, projectBreakingThreshold, contactDist);  
	
	//After the refresh contact points, the numcontacts in the manifold will be changed
	const PxU32 newContacts = manifold.mNumContacts;

	//const bool bLostContacts = ((initialContacts > 0) & (newContacts != initialContacts));
	const bool bLostContacts = (newContacts != initialContacts);

	PxGJKStatus status = manifold.mNumContacts > 0 ? GJK_UNDEFINED : GJK_NON_INTERSECT;

	Vec3V closestA(zeroV), closestB(zeroV), normal(zeroV); // from a to b
	FloatV penDep = FZero(); 

	//gRenderOutPut = cache.mRenderOutput;
	//manifold.numContacts = 0;
	
	if(bLostContacts || manifold.invalidate(curRTrans, minMargin))	
	{
	
		const QuatV vQuat = QuatVLoadU(&shapeConvex.scale.rotation.x);
		Gu::ShrunkConvexHullV convexHull(hullData, zeroV, vScale, vQuat);
		Gu::ShrunkBoxV box(zeroV, boxExtents);
		const bool idtScale = shapeConvex.scale.isIdentity();

		if(idtScale)
		{
			
			/*status = Gu::GJKRelativePenetration(box, *PX_SCONVEX_TO_NOSCALECONVEX(&convexHull), aToB, contactDist, closestA, closestB, normal, penDep,
				manifold.aIndice, manifold.bIndice, manifold.numWarmStartPoints);*/

			status = Gu::gjkRelativePenetration(box, *PX_SCONVEX_TO_NOSCALECONVEX(&convexHull), aToB, contactDist, closestA, closestB, normal, penDep,
				manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints);
		}
		else
		{/*
			status = Gu::GJKRelativePenetration(box, convexHull, aToB, contactDist, closestA, closestB, normal, penDep,
				manifold.aIndice, manifold.bIndice, manifold.numWarmStartPoints);*/
			status = Gu::gjkRelativePenetration(box, convexHull, aToB, contactDist, closestA, closestB, normal, penDep,
				manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints);

		}

		manifold.setRelativeTransform(curRTrans); 

		Gu::PersistentContact* manifoldContacts = PX_CP_TO_PCP(contactBuffer.contacts);
		PxU32 numContacts = 0;
		if(status == GJK_DEGENERATE)
		{
			return fullContactsGenerationBoxConvex(shapeBox.halfExtents, box, convexHull, transf0, transf1, manifoldContacts, numContacts, contactBuffer, manifold, normal, contactDist, idtScale, true);
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
					manifoldContacts[numContacts].mLocalNormalPen = localNormalPen;

					//Add contact to manifold
					manifold.addManifoldPoint(aToB.transformInv(closestA), closestB, localNormalPen, replaceBreakingThreshold);

				}
				else //if(status == EPA_CONTACT)
				{
					PX_ASSERT(status == EPA_CONTACT);
					/*status= Gu::EPARelativePenetration((Gu::BoxV&)box, (Gu::ConvexHullV&)convexHull, aToB,  closestA, closestB, normal, penDep,
						manifold.aIndice, manifold.bIndice, manifold.numWarmStartPoints);  */
					EPASupportMapPairRelativeImpl<Gu::BoxV, Gu::ConvexHullV> supportMap((Gu::BoxV&)box, (Gu::ConvexHullV&)convexHull, aToB);
					status= epaPenetration((Gu::BoxV&)box, (Gu::ConvexHullV&)convexHull, &supportMap, manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints, 
						closestA, closestB, normal, penDep); 
					if(status == EPA_CONTACT)
					{
						const Vec3V v1 = V3Sub(closestB, closestA);  
						const FloatV dist = V3Length(v1);
						normal = V3ScaleInv(v1, dist);
						penDep = FNeg(dist);

						const Vec3V localPointA = aToB.transformInv(closestA);//curRTrans.transformInv(closestA);
						const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(normal), penDep);

						//Add contact to contact stream
						manifoldContacts[numContacts].mLocalPointA = localPointA;
						manifoldContacts[numContacts].mLocalPointB = closestB;
						manifoldContacts[numContacts].mLocalNormalPen = localNormalPen;

						//Add contact to manifold
						manifold.addManifoldPoint(localPointA, closestB, localNormalPen, replaceBreakingThreshold);
					}
					else
					{
						doOverlapTest = true;
					}
				}

					
				if(manifold.mNumContacts < initialContacts || doOverlapTest)//bLostContacts || doOverlapTest)
				{
					numContacts++;
					return fullContactsGenerationBoxConvex(shapeBox.halfExtents, box, convexHull, transf0, transf1, manifoldContacts, numContacts,  contactBuffer, manifold, normal, contactDist, idtScale, doOverlapTest);
				}
				else
				{
					//Add contact to manifold
					//manifold.addManifoldPoint(manifoldContacts[numContacts].mLocalPointA, manifoldContacts[numContacts].mLocalPointB, manifoldContacts[numContacts].mLocalNormalPen, replaceBreakingThreshold);

					//const Vec3V worldNormal = manifold.getWorldNormal(transf1);//transf1.rotate(normal);
					const Vec3V worldNormal = transf1.rotate(normal);
					manifold.addManifoldContactsToContactBuffer(contactBuffer, worldNormal, transf1);

					//manifold.drawManifold(*gRenderOutPut, transf0, transf1);
					return true;
				}
			}
			else
			{
				//generate full manifold
				if(status == GJK_CONTACT)
				{
					const Vec3V localPointA = aToB.transformInv(closestA);//curRTrans.transformInv(closestA);
					const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(normal), penDep);

					//Add contact to contact stream
					manifoldContacts[numContacts].mLocalPointA = localPointA;
					manifoldContacts[numContacts].mLocalPointB = closestB;
					manifoldContacts[numContacts++].mLocalNormalPen = localNormalPen;
					//manifold.addManifoldPoint(aToB.transformInv(closestA), closestB, localNormalPen, replaceBreakingThreshold);
				}
				bool doOverlapTest = (status == EPA_CONTACT) ? true : false;
				return fullContactsGenerationBoxConvex(shapeBox.halfExtents, box, convexHull, transf0, transf1, manifoldContacts, numContacts,  contactBuffer, manifold, normal, contactDist, idtScale, doOverlapTest);
			}
			
		} 
	}
	else
	{
		const Vec3V worldNormal =  manifold.getWorldNormal(transf1);
		manifold.addManifoldContactsToContactBuffer(contactBuffer, worldNormal, transf1);
		//manifold.drawManifold(*gRenderOutPut, transf0, transf1);
		return manifold.getNumContacts()>0;
	}

}

}//Gu
}//physx
