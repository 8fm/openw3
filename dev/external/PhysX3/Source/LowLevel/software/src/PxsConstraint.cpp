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


#include "PxsConstraint.h"
#include "PxsDynamics.h"
#include "PxsContext.h"
#include "PxsRigidBody.h"

using namespace physx;

PxU32 physx::PxsSetupSolverConstraint(const PxsConstraint* constraint,
									  PxcSolverBodyData* solverBodyDataArray,
									  PxcConstraintBlockStream& blockStream,
									  PxReal dt, PxReal recipdt,
									  PxcSolverConstraintDesc& desc,
									  PxsConstraintBlockManager& constraintBlockManager)
{
	// LL shouldn't see broken constraints
	PX_ASSERT(!constraint->writeback->broken);

	const PxConstraintSolverPrep solverPrep = constraint->solverPrep;
	const void* constantBlock = constraint->constantBlock;
	const PxU32 constantBlockByteSize = constraint->constantBlockSize;
	const PxTransform& pose0 = (constraint->body0 ? constraint->body0->getPose() : PxTransform(PxIdentity));
	const PxTransform& pose1 = (constraint->body1 ? constraint->body1->getPose() : PxTransform(PxIdentity));
	const PxcSolverBody* sbody0 = desc.bodyA;
	const PxcSolverBody* sbody1 = desc.bodyB;
	PxcSolverBodyData* sbodyData0 = &solverBodyDataArray[desc.linkIndexA != PxcSolverConstraintDesc::NO_LINK ? 0 : desc.bodyADataIndex];
	PxcSolverBodyData* sbodyData1 = &solverBodyDataArray[desc.linkIndexB != PxcSolverConstraintDesc::NO_LINK ? 0 : desc.bodyBDataIndex];

	return PxsConstraintHelper::setupSolverConstraint
		(pose0, pose1, sbody0, sbody1, sbodyData0, sbodyData1,
		solverPrep, constantBlock, constantBlockByteSize,
		dt, recipdt,
		desc, *constraint, blockStream, constraintBlockManager);
}

void physx::PxsConstraintProject(const PxsConstraint* constraint, bool projectToBody0, PxReal /*dt*/)
{
	PxsRigidBody* body0 = constraint->body0, 
				* body1 = constraint->body1;

	PxTransform body0ToWorld = body0 ? body0->getPose() : PxTransform(PxIdentity);
	PxTransform body1ToWorld = body1 ? body1->getPose() : PxTransform(PxIdentity);

	(*constraint->project)(constraint->constantBlock, body0ToWorld, body1ToWorld, projectToBody0);

	if(projectToBody0)
	{
		PX_ASSERT(body1);
		body1->setPose(body1ToWorld);
	}
	else
	{
		PX_ASSERT(body0);
		body0->setPose(body0ToWorld);
	}
}
