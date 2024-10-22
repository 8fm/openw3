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


// suppress LNK4221
#include "PxPreprocessor.h"
PX_DUMMY_SYMBOL

#include "PsVecMath.h"
#ifndef PX_SUPPORT_SIMD

#include "PxcSolverBody.h"
#include "PxcSolverContact.h"
#include "PxcSolverConstraintTypes.h"
#include "PxcSolverConstraintDesc.h"
#include "PsMathUtils.h"
#include "PxsSolverContact.h"

void finalizeContact(const PxcSolverConstraintDesc& desc, 
					 const PxReal invDt,
					 PxReal bounceThreshold)
{
	const PxcSolverBody* PX_RESTRICT b0 = desc.bodyA;
	const PxcSolverBody* PX_RESTRICT b1 = desc.bodyB;

	PxU8* PX_RESTRICT ptr = desc.constraint;
	const PxU32 length = getConstraintLength(desc);

	PxU8* p = ptr;
	while(p < ptr + length)
	{
		PxcSolverContactHeader* PX_RESTRICT header = reinterpret_cast<PxcSolverContactHeader* PX_RESTRICT>(p);
		p += sizeof(PxcSolverContactHeader);

		PxReal d0 = header->getDominance0PxF32();
		PxReal d1 = header->getDominance1PxF32();


		PxcSolverContactPoint* PX_RESTRICT point = reinterpret_cast<PxcSolverContactPoint* PX_RESTRICT>(p);

		for(PxU8 j=0;j<header->numNormalConstr;j++)
		{
			PxcSolverContactPoint& c = point[j];

			PxReal unitResponse = b0->getResponse(c.getNormalPxVec3(), c.getRaXnPxVec3()) * d0 
				                + b1->getResponse(c.getNormalPxVec3(), c.getRbXnPxVec3()) * d1;

			const PxReal vrel = b0->projectVelocity(c.getNormalPxVec3(), c.getRaXnPxVec3())
							  - b1->projectVelocity(c.getNormalPxVec3(), c.getRbXnPxVec3());

			completeContactPoint(c, unitResponse, vrel, invDt,
				header->restitution, bounceThreshold);
			
		}

		p += sizeof(PxcSolverContactPoint)*header->numNormalConstr;

		PxcSolverContactFriction* PX_RESTRICT friction = reinterpret_cast<PxcSolverContactFriction* PX_RESTRICT>(p);

		PxVec3 t0, t1;
		//computeFrictionTangents(b0->linearVelocity - b1->linearVelocity, point[0].normal, t0, t1);
		computeFrictionTangents(b0->linearVelocity - b1->linearVelocity, point[0].getNormalPxVec3(), t0, t1);

		for(PxU8 j=0;j<header->numFrictionConstr;j+=2)
		{
			PxcSolverContactFriction* f = friction+j;

			completeFrictionRow(f[0], t0, invDt);
			completeFrictionRow(f[1], t1, invDt);
		
			PxReal unitResponse;

			unitResponse = b0->getResponse(t0, f[0].getRaXnPxVec3()) * d0 
				         + b1->getResponse(t0, f[0].getRbXnPxVec3()) * d1;

			f[0].setVelMultiplier(unitResponse ? 0.8f/unitResponse : 0.0f);

			unitResponse = b0->getResponse(t1, f[1].getRaXnPxVec3()) * d0 
						 + b1->getResponse(t1, f[1].getRbXnPxVec3()) * d1;

			f[1].setVelMultiplier(unitResponse ? 0.8f/unitResponse : 0.0f);

		}

		p += sizeof(PxcSolverContactFriction)*header->numFrictionConstr;
	}
	PX_ASSERT(p == ptr + length);
}



#endif
