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

#include "PxPhysXCommonConfig.h"

#if PX_SUPPORT_GPU_PHYSX

#include "windows/PxsRigidBodyAccessGpu.h"
#include "PxvGeometry.h"
#include "PxvDynamics.h"
#include "PxsBodyTransformVault.h"

using namespace physx;

void PxsRigidBodyAccessGpu::copyShapeProperties(ShapeProperties& shapeProperties, const PxvShapeHandle shape, const PxvBodyHandle body) const
{
	const PxsShapeCore* shapeCore = reinterpret_cast<const PxsShapeCore*>(shape);
	*shapeProperties.geometry = shapeCore->geometry;
	
	const PxsRigidCore* rigidCore = reinterpret_cast<const PxsRigidCore*>(body);
	*shapeProperties.ownerToWorld = rigidCore->body2World;
	*shapeProperties.shapeToOwner = shapeCore->transform;
}

void PxsRigidBodyAccessGpu::copyBodyProperties(BodyProperties& bodyProperties, const PxvBodyHandle* bodies, PxU32 numBodies) const
{
	const PxsBodyCore*const* bodyIt = reinterpret_cast<const PxsBodyCore*const*>(bodies);
	PxStrideIterator<PxTransform> currentTransformIt(bodyProperties.currentTransforms);
	PxStrideIterator<PxTransform> previousTransformIt(bodyProperties.previousTransforms);
	PxStrideIterator<PxVec3> linearVelocityIt(bodyProperties.linearVelocities);
	PxStrideIterator<PxVec3> angularVelocityIt(bodyProperties.angularVelocities);
	PxStrideIterator<PxTransform> body2ActorTransformIt(bodyProperties.body2ActorTransforms);
	PxStrideIterator<PxvBodyHandle> bodyHandleIt(bodyProperties.cpuBodyHandle);

	for (PxU32 i = 0; i < numBodies; ++i)
	{
		const PxsBodyCore& body = **bodyIt;
		*currentTransformIt = body.body2World;
		const PxTransform* preTransform = mTransformVault.getTransform(body);
		if(preTransform)
		{
			*previousTransformIt = *preTransform;
			*linearVelocityIt = body.linearVelocity;
			*angularVelocityIt = body.angularVelocity;
			*body2ActorTransformIt = body.body2Actor;
			*bodyHandleIt = (PxvBodyHandle)*bodyIt;
		}
		else
		{
			PX_ASSERT(0);
			*previousTransformIt = PxTransform(PxIdentity);
			*linearVelocityIt = PxVec3(0.f);
			*angularVelocityIt =  PxVec3(0.f);
			*body2ActorTransformIt = PxTransform(PxIdentity);
			*bodyHandleIt = NULL;
		}

		++bodyIt;
		++currentTransformIt;
		++previousTransformIt;
		++linearVelocityIt;
		++angularVelocityIt;
		++body2ActorTransformIt;
		++bodyHandleIt;
	}
}

#endif