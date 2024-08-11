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



#ifndef PXC_SOLVERBODY_H
#define PXC_SOLVERBODY_H

#include "PxvConfig.h"
#include "PxSimpleTypes.h"
#include "PxVec3.h"
#include "PxMat33.h"

namespace physx
{

class PxsRigidBody;
class PxsArticulation;

struct PxcSolverBody;

struct PxcSolverBodyData
{
	PxMat33    invInertia;      // 36 inverse inertia in world space
	PxReal    reportThreshold;    // 40 contact force threshold
	PxsRigidBody*  originalBody;     // 44 or 48 depending on ptr size
	PxcSolverBody*  solverBody;      // 48 or 56 depending on ptr size
	PxReal    invMass;      // 52 or 60 inverse mass

	PxU8    pad[20-(sizeof(void *) + sizeof(void*))];
};

PX_COMPILE_TIME_ASSERT(sizeof(PxcSolverBodyData) == 64);


struct PxcSolverBody
{
	PxVec3				linearVelocity;					// 12 post-solver linear velocity in world space
	PxU16				maxSolverNormalProgress;
	PxU16				maxSolverFrictionProgress;
					
	PxVec3				angularVelocity;				// 28 post-solver angular velocity in world space	
	
	PxU32				solverProgress;					// 16

	PX_FORCE_INLINE void getResponse(const PxVec3& linImpulse, const PxVec3& angImpulse,
								     PxVec3& linDeltaV, PxVec3& angDeltaV, const PxcSolverBodyData& _solverBodyData) const
	{
		linDeltaV = linImpulse * _solverBodyData.invMass;
		angDeltaV = _solverBodyData.invInertia * angImpulse;
	}

	PX_FORCE_INLINE	PxReal getResponse(const PxVec3& normal, const PxVec3& rXn, const PxcSolverBodyData& _solverBodyData)	const
	{
		return (normal.magnitudeSquared() * _solverBodyData.invMass) + rXn.dot(_solverBodyData.invInertia * rXn);
	}

	PX_FORCE_INLINE	PxReal getResponse(const PxVec3& normal, const PxVec3& rXn, 
									   PxVec3& angDeltaV, const PxcSolverBodyData& _solverBodyData,
									   PxReal invMassScale, PxReal invInertiaScale)	const
	{
		const PxVec3 angDelta = (_solverBodyData.invInertia * rXn) * invInertiaScale;
		angDeltaV = angDelta;
		return (normal.magnitudeSquared() * _solverBodyData.invMass * invMassScale) + rXn.dot(angDelta);
	}

	PX_FORCE_INLINE PxReal projectVelocity(const PxVec3& lin, const PxVec3& ang)	const
	{
		return linearVelocity.dot(lin) + angularVelocity.dot(ang);
	}


	PX_FORCE_INLINE	void applyImpulse(const PxVec3& lin, const PxVec3& ang, const PxcSolverBodyData& _solverBodyData)
	{
		linearVelocity += lin * _solverBodyData.invMass;
		angularVelocity += _solverBodyData.invInertia * ang;
		PX_ASSERT(linearVelocity.isFinite());
		PX_ASSERT(angularVelocity.isFinite());
	}

	/*PX_FORCE_INLINE	void saveVelocity(PxcSolverBodyData& _solverBodyData)
	{
		PX_ASSERT(linearVelocity.isFinite());
		PX_ASSERT(angularVelocity.isFinite());
		_solverBodyData.motionLinearVelocity = linearVelocity;
		_solverBodyData.motionAngularVelocity = angularVelocity;
	}*/
};

#define SOLVER_BODY_SOLVER_PROGRESS_OFFSET 28
#define SOLVER_BODY_MAX_SOLVER_PROGRESS_OFFSET 12
#define SOLVER_BODY_MAX_SOLVER_FRICTION_PROGRESS_OFFSET 14

PX_COMPILE_TIME_ASSERT(sizeof(PxcSolverBody) == 32);

}

#endif //PXC_SOLVERBODY_H
