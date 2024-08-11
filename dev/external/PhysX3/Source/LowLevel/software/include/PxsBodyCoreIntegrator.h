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


#ifndef PXS_BODYCORE_INTEGRATOR_H
#define PXS_BODYCORE_INTEGRATOR_H

#include "CmPhysXCommon.h"
#include "PxvDynamics.h"
#include "PsMathUtils.h"

namespace physx
{

PX_FORCE_INLINE void bodyCoreComputeUnconstrainedVelocity
(PxsBodyCore& core, const PxVec3& linearAccel, const PxVec3& angularAccel, const PxReal dt)
{
	//TODO - rewrite this with simd instructions.

	//Grab what we need from the body.
	PxVec3 linearVelocity=core.linearVelocity;
	PxVec3 angularVelocity=core.angularVelocity;
	const PxReal linearDamping=core.linearDamping;
	const PxReal angularDamping=core.angularDamping;
	const PxReal maxLinearVelocitySq=core.maxLinearVelocitySq;
	const PxReal maxAngularVelocitySq=core.maxAngularVelocitySq;

	//Multiply everything that needs multiplied by dt to improve code generation.
	const PxVec3 linearAccelTimesDT=linearAccel*dt;
	const PxVec3 angularAccelTimesDT=angularAccel*dt;
	const PxReal linearDampingTimesDT=linearDamping*dt;
	const PxReal angularDampingTimesDT=angularDamping*dt;
	const PxReal oneMinusLinearDampingTimesDT=1.0f-linearDampingTimesDT;
	const PxReal oneMinusAngularDampingTimesDT=1.0f-angularDampingTimesDT;

	//TODO context-global gravity
	linearVelocity += linearAccelTimesDT;
	angularVelocity += angularAccelTimesDT;

	//Apply damping.
	const PxReal linVelMultiplier = physx::intrinsics::fsel(oneMinusLinearDampingTimesDT, oneMinusLinearDampingTimesDT, 0.0f);
	const PxReal angVelMultiplier = physx::intrinsics::fsel(oneMinusAngularDampingTimesDT, oneMinusAngularDampingTimesDT, 0.0f);
	linearVelocity*=linVelMultiplier;
	angularVelocity*=angVelMultiplier;

	// Clamp velocity
	const PxReal linVelSq = linearVelocity.magnitudeSquared();
	if(linVelSq > maxLinearVelocitySq)
	{
		linearVelocity *= PxSqrt(maxLinearVelocitySq / linVelSq);
	}
	const PxReal angVelSq = angularVelocity.magnitudeSquared();
	if(angVelSq > maxAngularVelocitySq)
	{
		angularVelocity *= PxSqrt(maxAngularVelocitySq / angVelSq);
	}

	//Update the body.
	core.linearVelocity=linearVelocity;
	core.angularVelocity=angularVelocity;
}

PX_FORCE_INLINE void integrateCore(PxsBodyCore& core, Cm::SpatialVector& motionVelocity, const PxF32 dt) 
{
	// Integrate linear part
	core.body2World.p += motionVelocity.linear * dt;
	PX_ASSERT(core.body2World.p.isFinite());

	// Integrate the rotation using closed form quaternion integrator
	PxReal w = motionVelocity.angular.magnitudeSquared();

	if(w != 0.0f)
	{
		w = PxSqrt(w);
		// Perform a post-solver clamping
		// TODO(dsequeira): ignore this for the moment
			//just clamp motionVel to half float-range
		const PxReal maxW = 1e+7f;		//Should be about sqrt(PX_MAX_REAL/2) or smaller
		if(w > maxW)
		{
			motionVelocity.angular = motionVelocity.angular.getNormalized() * maxW;
			w = maxW;
		}

		if (w != 0.0f)
		{
			const PxReal v = dt * w * 0.5f;
			PxReal s, q;
			Ps::sincos(v, s, q);
			s /= w;

			const PxVec3 pqr = motionVelocity.angular * s;
			const PxQuat quatVel(pqr.x, pqr.y, pqr.z, 0);
			PxQuat result = quatVel * core.body2World.q;

			result += core.body2World.q * q;

			core.body2World.q = result.getNormalized();
			PX_ASSERT(core.body2World.q.isSane());
			PX_ASSERT(core.body2World.q.isFinite());
		}
	}
}

}

#endif //PXS_BODYCORE_INTEGRATOR_H
