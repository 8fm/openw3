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


#include "CmUtils.h"
#include "PxcSolverBody.h"
#include "PxsRigidBody.h"
#include "PxvDynamics.h"

namespace physx
{

void copyToSolverBody(PxcSolverBody& solverBody, PxcSolverBodyData& data, const PxsBodyCore& core, PxsRigidBody& body)
{
	data.solverBody = &solverBody;
	data.originalBody = &body;

	// Copy simple properties
	solverBody.linearVelocity = core.linearVelocity;
	solverBody.angularVelocity = core.angularVelocity;

	PX_ASSERT(core.linearVelocity.isFinite());
	PX_ASSERT(core.angularVelocity.isFinite());
	/*data.motionLinearVelocity = core.linearVelocity;
	data.motionAngularVelocity = core.angularVelocity;*/

	data.invMass = core.inverseMass;

	Cm::transformInertiaTensor(core.inverseInertia, PxMat33(core.body2World.q), data.invInertia);

	data.reportThreshold = core.contactReportThreshold;
}

void writeBackSolverBody(PxcSolverBody& solverBody, PxcSolverBodyData& solverBodyData, PxsBodyCore& core, Cm::SpatialVector&)
{
	PX_ASSERT(solverBodyData.originalBody);

	core.linearVelocity = solverBody.linearVelocity;
	core.angularVelocity = solverBody.angularVelocity;

	/*PX_ASSERT(solverBodyData.motionLinearVelocity.isFinite());
	PX_ASSERT(solverBodyData.motionAngularVelocity.isFinite());
	motionVelocity.linear = solverBodyData.motionLinearVelocity;
	motionVelocity.angular = solverBodyData.motionAngularVelocity;*/

	solverBodyData.originalBody = NULL; 
}

}
