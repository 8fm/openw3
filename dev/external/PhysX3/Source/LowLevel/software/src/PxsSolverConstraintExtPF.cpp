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


#include "PxPreprocessor.h"
#include "PsVecMath.h"
#include "PxsSolverConstraintExt.h"
#include "PxcSolverConstraintDesc.h"
#include "PxcSolverConstraint1D.h"
#include "PxsSolverContact.h"
#include "PxcSolverContactPF.h"
#include "PxcArticulationHelper.h"
#include "PxcNpWorkUnit.h"
#include "PxsMaterialManager.h"
#include "PxsMaterialCombiner.h"

using namespace physx;
using namespace Gu;

// constraint-gen only, since these use getVelocityFast methods
// which aren't valid during the solver phase

namespace physx
{


bool setupFinalizeExtSolverConstraintsCoulomb(PxcNpWorkUnit& n,
						    const ContactBuffer& buffer,
							const PxcCorrelationBufferCoulomb& c,
							const PxTransform& bodyFrame0,
							const PxTransform& bodyFrame1,
							bool /*perPointFriction*/,
							PxU8* workspace,
							PxReal invDt,
							PxReal bounceThreshold,
							PxsSolverExtBody& b0,
							PxsSolverExtBody& b1,
							PxU32 frictionCountPerPoint,
							PxReal invMassScale0, PxReal invInertiaScale0, 
							PxReal invMassScale1, PxReal invInertiaScale1)	
{
	// NOTE II: the friction patches are sparse (some of them have no contact patches, and
	// therefore did not get written back to the cache) but the patch addresses are dense,
	// corresponding to valid patches

	PxU8* PX_RESTRICT ptr = workspace;
	const FloatV zero=FZero();

	const FloatV restDistance = FLoad(n.restDistance); 

	Ps::prefetchLine(c.contactID);
	Ps::prefetchLine(c.contactID, 128);

	bool useExtContacts = (n.flags & (PxcNpWorkUnitFlag::eARTICULATION_BODY0|PxcNpWorkUnitFlag::eARTICULATION_BODY1))!=0;

	const PxU32 frictionPatchCount = c.frictionPatchCount;
	const bool staticBody = ((n.flags & PxcNpWorkUnitFlag::eDYNAMIC_BODY1) == 0);

	const PxU32 pointStride = useExtContacts ? sizeof(PxcSolverContactExt) : sizeof(PxcSolverContact);
	const PxU32 frictionStride = useExtContacts ? sizeof(PxcSolverFrictionExt) : sizeof(PxcSolverFriction);
	const PxU8 pointHeaderType = Ps::to8(useExtContacts ? PXS_SC_TYPE_EXT_CONTACT : (staticBody ? PXS_SC_TYPE_STATIC_CONTACT : PXS_SC_TYPE_RB_CONTACT));
	const PxU8 frictionHeaderType = Ps::to8(useExtContacts ? PXS_SC_TYPE_EXT_FRICTION : (staticBody ? PXS_SC_TYPE_STATIC_FRICTION : PXS_SC_TYPE_FRICTION));

	PxReal d0 = n.dominance0 * invMassScale0;
	PxReal d1 = n.dominance1 * invMassScale1;
	PxReal angD0 = n.dominance0 * invInertiaScale0;
	PxReal angD1 = n.dominance1 * invInertiaScale1;

	for(PxU32 i=0;i< frictionPatchCount;i++)
	{
		const PxU32 contactCount = c.frictionPatchContactCounts[i];
		if(contactCount == 0)
			continue;

		const Gu::ContactPoint* contactBase = buffer.contacts + c.contactPatches[c.correlationListHeads[i]].start;

		const PxcFrictionPatchCoulomb& frictionPatch = c.frictionPatches[i];

		const Vec3V normalV = Ps::aos::V3LoadU(frictionPatch.normal);
		const PxVec3 normal = frictionPatch.normal;

		const PxReal combinedRestitution = contactBase->restitution;
	
		
		PxcSolverContactCoulombHeader* PX_RESTRICT header = reinterpret_cast<PxcSolverContactCoulombHeader* PX_RESTRICT>(ptr);
		ptr += sizeof(PxcSolverContactCoulombHeader);

		Ps::prefetchLine(ptr, 128);
		Ps::prefetchLine(ptr, 256);
		Ps::prefetchLine(ptr, 384);


		header->numNormalConstr		= (PxU8)contactCount;
		header->type				= pointHeaderType;
		header->setRestitution(combinedRestitution);

		header->setDominance0(d0);
		header->setDominance1(d1);
		header->angDom0 = angD0;
		header->angDom1 = angD1;
		
		header->setNormal(normalV);
		
		for(PxU32 patch=c.correlationListHeads[i]; 
			patch!=PxcCorrelationBuffer::LIST_END; 
			patch = c.contactPatches[patch].next)
		{
			const PxU32 count = c.contactPatches[patch].count;
			const Gu::ContactPoint* contactBase = buffer.contacts + c.contactPatches[patch].start;
				
			PxU8* p = ptr;
			for(PxU32 j=0;j<count;j++)
			{
				const Gu::ContactPoint& contact = contactBase[j];

				PxcSolverContactExt* PX_RESTRICT solverContact = reinterpret_cast<PxcSolverContactExt* PX_RESTRICT>(p);
				p += pointStride;

				const FloatV separation = FLoad(contact.separation);

				PxVec3 ra = contact.point - bodyFrame0.p; 
				PxVec3 rb = contact.point - bodyFrame1.p; 

				Vec3V targetVel = V3LoadU(contact.targetVel);
				const FloatV maxImpulse = FLoad(contact.maxImpulse);

				solverContact->scaledBiasX_targetVelocityY_maxImpulseZ = V3Merge(FSub(separation, restDistance), V3Dot(normalV,targetVel), maxImpulse);

				//TODO - should we do cross only in vector land and then store. Could cause a LHS but probably no worse than
				//what we already have (probably has a LHS from converting from vector to scalar above)
				const PxVec3 raXn = ra.cross(normal);
				const PxVec3 rbXn = rb.cross(normal);

				Cm::SpatialVector deltaV0, deltaV1;

				PxReal unitResponse = getImpulseResponse(b0, Cm::SpatialVector(normal, raXn), deltaV0, d0, angD0,
														 b1, Cm::SpatialVector(-normal, -rbXn), deltaV1, d1, angD1);

				const PxReal vrel = b0.projectVelocity(normal, raXn)
								  - b1.projectVelocity(normal, rbXn);

				solverContact->raXnXYZ_appliedForceW = V4SetW(Vec4V_From_Vec3V(V3LoadU(raXn)), zero);
				solverContact->rbXnXYZ_velMultiplierW = V4SetW(Vec4V_From_Vec3V(V3LoadU(rbXn)), zero);

				completeContactPoint(*solverContact, unitResponse, vrel, invDt, header->restitution, bounceThreshold);

				solverContact->setDeltaVA(deltaV0.linear, deltaV0.angular);
				solverContact->setDeltaVB(deltaV1.linear, deltaV1.angular);


			}			
			ptr = p;
		}
	}

	//construct all the frictions

	PxU8* PX_RESTRICT ptr2 = workspace;

	bool hasFriction = false;

	for(PxU32 i=0;i< frictionPatchCount;i++)
	{
		const PxU32 contactCount = c.frictionPatchContactCounts[i];
		if(contactCount == 0)
			continue;

		PxcSolverContactCoulombHeader* header = reinterpret_cast<PxcSolverContactCoulombHeader* PX_RESTRICT>(ptr2); 
		header->frictionOffset = PxU16(ptr - ptr2);
		ptr2 += sizeof(PxcSolverContactCoulombHeader) + header->numNormalConstr * pointStride;

		PxVec3 normal = c.frictionPatches[i].normal;

		const Gu::ContactPoint* contactBase = buffer.contacts + c.contactPatches[c.correlationListHeads[i]].start;

		const PxReal staticFriction = contactBase->staticFriction;
		const PxU32 disableStrongFriction = contactBase->internalFaceIndex1 & PxMaterialFlag::eDISABLE_FRICTION;
		const bool haveFriction = (disableStrongFriction == 0);
	
		PxcSolverFrictionHeader* frictionHeader = (PxcSolverFrictionHeader*)ptr;
		frictionHeader->numNormalConstr = Ps::to8(c.frictionPatchContactCounts[i]);
		frictionHeader->numFrictionConstr = Ps::to8(haveFriction ? c.frictionPatches[i].numConstraints : 0);
		ptr += sizeof(PxcSolverFrictionHeader);
		ptr += frictionHeader->getAppliedForcePaddingSize(c.frictionPatchContactCounts[i]);
		Ps::prefetchLine(ptr, 128);
		Ps::prefetchLine(ptr, 256);
		Ps::prefetchLine(ptr, 384);

		PxVec3 t0, t1;

		PxVec3 tFallback[2];
		Ps::normalToTangents(normal, tFallback[0], tFallback[1]);

		if(haveFriction)
		{
			hasFriction = true;
			frictionHeader->setStaticFriction(staticFriction);
			frictionHeader->setDominance0(n.dominance0);
			frictionHeader->setDominance1(n.dominance1);
			frictionHeader->angDom0 = angD0;
			frictionHeader->angDom1 = angD1;
			frictionHeader->type			= frictionHeaderType;
			
			PxU32 totalPatchContactCount = 0;
		
			for(PxU32 patch=c.correlationListHeads[i]; 
				patch!=PxcCorrelationBuffer::LIST_END; 
				patch = c.contactPatches[patch].next)
			{
				const PxU32 count = c.contactPatches[patch].count;
				const PxU32 start = c.contactPatches[patch].start;
				const Gu::ContactPoint* contactBase = buffer.contacts + start;
					
				PxU8* p = ptr;

				

				for(PxU32 j =0; j < count; j++)
				{
					const PxU32 contactId = totalPatchContactCount + j;
					const Gu::ContactPoint& contact = contactBase[j];
					const PxVec3 ra = contact.point - bodyFrame0.p;
					const PxVec3 rb = contact.point - bodyFrame1.p;

					const PxVec3 targetVel = contact.targetVel;
					const PxVec3 pVRa = b0.getLinVel() + b0.getAngVel().cross(ra);
					const PxVec3 pVRb = b1.getLinVel() + b1.getAngVel().cross(rb);
					const PxVec3 vrel = pVRa - pVRb;

					for(PxU32 k = 0; k < frictionCountPerPoint; ++k)
					{
						PxcSolverFrictionExt* PX_RESTRICT f0 = reinterpret_cast<PxcSolverFrictionExt* PX_RESTRICT>(p);
						p += frictionStride;
						f0->contactIndex = contactId;

						t0 = vrel - normal * normal.dot(vrel);
						const PxReal ll = t0.magnitudeSquared();
						if(ll > 0.00001f)
							t0 *= PxRecipSqrt(ll);
						else
							t0 = tFallback[j&1];
						PxVec3 raXn = ra.cross(t0); 
						PxVec3 rbXn = rb.cross(t0); 
						Cm::SpatialVector deltaV0, deltaV1;
						PxReal unitResponse = getImpulseResponse(b0, Cm::SpatialVector(t0, raXn), deltaV0, d0, angD0,
																 b1, Cm::SpatialVector(-t0, -rbXn), deltaV1, d1, angD1);

						f0->setVelMultiplier(FLoad(unitResponse>0.0f ? 1.f/unitResponse : 0.0f));
						f0->setRaXn(raXn);
						f0->setRbXn(rbXn);
						f0->setNormal(t0);
						f0->setAppliedForce(0.0f);
						f0->setDeltaVA(deltaV0.linear, deltaV0.angular);
						f0->setDeltaVB(deltaV1.linear, deltaV1.angular);
					}					
				}

				totalPatchContactCount += c.contactPatches[patch].count;
				
				ptr = p;	
			}
		}
	}
	//PX_ASSERT(ptr - workspace == n.solverConstraintSize);
	return hasFriction;
}


}
