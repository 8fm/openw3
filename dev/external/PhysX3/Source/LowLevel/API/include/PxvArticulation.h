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


#ifndef PXD_ARTICULATION_H
#define PXD_ARTICULATION_H

#include "PxVec3.h"
#include "PxQuat.h"
#include "PxTransform.h"
#include "PxvConfig.h"
#include "PsVecMath.h"
#include "CmUtils.h"

namespace physx
{

class PxsContext;
class PxsRigidBody;
class PxcRigidBody;
struct PxsBodyCore;
struct PxcFsData;
struct PxcSIMDSpatial;


struct PxsArticulationCore
{
	PxU32		internalDriveIterations;
	PxU32		externalDriveIterations;
	PxU32		maxProjectionIterations;
	PxU16		solverIterationCounts; //KS - made a U16 so that it matches PxsRigidCore
	PxReal		separationTolerance;
	PxReal		sleepThreshold;
	PxReal		wakeCounter;
};

struct PxsArticulationJointCore
{
	// attachment points
	PxTransform		parentPose;
	PxTransform		childPose;

	// drive model
	PxQuat			targetPosition;
	PxVec3			targetVelocity; 

	PxReal			spring;
	PxReal			damping;

	PxReal			solverSpring;
	PxReal			solverDamping;

	PxReal			internalCompliance;
	PxReal			externalCompliance;

	// limit model

	PxReal			swingYLimit;
	PxReal			swingZLimit;
	PxReal			swingLimitContactDistance;
	bool			swingLimited;

	PxReal			tangentialStiffness;
	PxReal			tangentialDamping;

	PxReal			twistLimitHigh;
	PxReal			twistLimitLow;
	PxReal			twistLimitContactDistance;
	bool			twistLimited;

	PxReal			tanQSwingY;
	PxReal			tanQSwingZ;
	PxReal			tanQSwingPad;
	PxReal			tanQTwistHigh;
	PxReal			tanQTwistLow;
	PxReal			tanQTwistPad;

	PxsArticulationJointCore()
	{
		Cm::markSerializedMem(this, sizeof(PxsArticulationJointCore));
		parentPose = PxTransform(PxIdentity);
		childPose = PxTransform(PxIdentity);
		internalCompliance = 0;
		externalCompliance = 0;
		swingLimitContactDistance = 0.05f;
		twistLimitContactDistance = 0.05f;
	}
// PX_SERIALIZATION
	PxsArticulationJointCore(const PxEMPTY&)	{}
//~PX_SERIALIZATION

};

#define PXS_ARTICULATION_LINK_NONE 0xffffffff

typedef PxU64 PxcArticulationBitField;

struct PxsArticulationLink
{
	PxcArticulationBitField			children;		// child bitmap
	PxcArticulationBitField			pathToRoot;		// path to root, including link and root
	PxcRigidBody*					body;
	PxsBodyCore*					bodyCore;
	const PxsArticulationJointCore*	inboundJoint;
	PxU32							parent;
};


typedef size_t PxsArticulationLinkHandle;

struct PxcArticulationSolverDesc
{
	PxcFsData*					fsData;
	const PxsArticulationLink*	links;
	PxcSIMDSpatial*				motionVelocity;	
	PxTransform*				poses;
	physx::shdfnd::aos::Mat33V* externalLoads;
	physx::shdfnd::aos::Mat33V* internalLoads;
	const PxsArticulationCore*	core;
	char*						scratchMemory;
	PxU16						totalDataSize;
	PxU16						solverDataSize;
	PxU16						linkCount;
	PxU16						scratchMemorySize;
};

}

#endif
