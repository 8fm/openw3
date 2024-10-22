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


#pragma once

#include "PxvConfig.h"
#include "PxcSolverConstraintTypes.h"
#include "PxVec3.h"
#include "PxcSolverConstraint1D.h"
#include "PxcSolverBody.h"
#include "PxConstraintDesc.h"

namespace physx
{

PX_FORCE_INLINE void reset(PxsSolverConstraintWriteback& w)
{
	w.linear	= PxVec3(0);
	w.angular	= PxVec3(0);
	w.vForce	= 0.0f;
	w.flags		= 0;
}

PX_FORCE_INLINE void init(PxcSolverConstraint1DHeader& h, 
						  PxU8 count, 
						  bool isExtended,
						  const PxConstraintInvMassScale& ims)
{
	h.type			= PxU8(isExtended ? PXS_SC_TYPE_EXT_1D : PXS_SC_TYPE_RB_1D);
	h.count			= count;
	h.dominance		= 0;
	h.linearInvMassScale0	= ims.linear0;
	h.angularInvMassScale0	= ims.angular0;
	h.linearInvMassScale1	= -ims.linear1;
	h.angularInvMassScale1	= -ims.angular1;
}

PX_FORCE_INLINE void init(PxcSolverConstraint1D& c,
						  const PxVec3& _linear0, const PxVec3& _linear1, 
						  const PxVec3& _angular0, const PxVec3& _angular1,
						  PxReal _minImpulse, PxReal _maxImpulse)
{
	PX_ASSERT(_linear0.isFinite());
	PX_ASSERT(_linear1.isFinite());
	c.lin0					= _linear0;
	c.lin1					= _linear1;
	c.ang0					= _angular0;
	c.ang1					= _angular1;
	c.minImpulse			= _minImpulse;
	c.maxImpulse			= _maxImpulse;
	c.flags					= 0;
	c.appliedForce			= 0;
}

PX_FORCE_INLINE bool needsNormalVel(const Px1DConstraint &c)
{
	return c.flags & Px1DConstraintFlag::eRESTITUTION
		|| (c.flags & Px1DConstraintFlag::eSPRING && c.flags & Px1DConstraintFlag::eACCELERATION_SPRING);
}

PX_FORCE_INLINE void setSolverConstants(PxReal& constant,
										PxReal& unbiasedConstant,
										PxReal& velMultiplier,
										PxReal& impulseMultiplier,
										const Px1DConstraint& c,
										PxReal normalVel,
										PxReal unitResponse,
										PxReal erp,
										PxReal dt,
										PxReal recipdt)
{
	PxReal recipResponse = physx::intrinsics::fsel(-PxAbs(unitResponse), 0.0f, 1.0f/unitResponse);
	PxReal geomError = c.geometricError * erp;

	if(c.flags & Px1DConstraintFlag::eSPRING)
	{
		PxReal a = dt * dt * c.mods.spring.stiffness + dt * c.mods.spring.damping;
		PxReal b = dt * (c.mods.spring.damping * c.velocityTarget - c.mods.spring.stiffness * geomError);

		if(c.flags & Px1DConstraintFlag::eACCELERATION_SPRING)
		{	
			PxReal x = 1.0f/(1.0f+a);
			constant = unbiasedConstant = x * recipResponse * b;
			velMultiplier = -x * recipResponse * a;
			impulseMultiplier = 1.0f-x;
		}
		else
		{
			PxReal x = 1.0f/(1.0f+a*unitResponse);
			constant = unbiasedConstant = x * b;
			velMultiplier = -x*a;
			impulseMultiplier = 1.0f-x;
		}
	}
	else
	{
		velMultiplier = -recipResponse;
		impulseMultiplier = 1.0f;

		if(c.flags & Px1DConstraintFlag::eRESTITUTION && -normalVel>c.mods.bounce.velocityThreshold)
		{
			unbiasedConstant = constant = recipResponse * c.mods.bounce.restitution*-normalVel;
		}
		else
		{
			// see usage of 'for internal use' in orthogonalize()
			constant = recipResponse * (c.velocityTarget - c.geometricError*recipdt);
			unbiasedConstant = recipResponse * (c.velocityTarget - c.forInternalUse*recipdt);
		}
	}
}




}
