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


#include "PxsThreadContext.h"
#include "PxsContext.h"

using namespace physx;

PxsThreadContext::PxsThreadContext(PxsContext* context):
	PxcNpThreadContext(	context->getMeshContactMargin(),
						context->getCorrelationDistance(),
						context->getToleranceLength(),
						context->getRenderOutput(),
						context->getNpMemBlockPool(),
						context->getCreateContactStream()),
					   mNumDifferentBodyConstraints(0),
	mNumSelfConstraints(0),
	
	mNumSelfConstraintBlocks(0),
	bodyCoreArray(PX_DEBUG_EXP("PxsThreadContext::bodyCoreArray")),
	accelerationArray(PX_DEBUG_EXP("PxsThreadContext::accelerationArray")),
	motionVelocityArray(PX_DEBUG_EXP("PxsThreadContext::motionVelocityArray")),
	contactConstraintDescArray(PX_DEBUG_EXP("PxsThreadContext::solverContactConstraintArray")),
	frictionConstraintDescArray(PX_DEBUG_EXP("PxsThreadContext::solverFrictionConstraintArray")),
	orderedContactConstraints(PX_DEBUG_EXP("PxsThreadContext::orderedContactContraintArray")),
	//orderedFrictionConstraints(PX_DEBUG_EXP("PxsThreadContext::orderedFrictionContraintArray")),
	contactConstraintBatchHeaders(PX_DEBUG_EXP("PxsThreadContext::contactConstraintBatchHeaders")),
	frictionConstraintBatchHeaders(PX_DEBUG_EXP("PxsThreadContext::frictionConstraintBatchHeaders")),
	mAxisConstraintCount(0),
	mSuccessfulSpuConstraintPartition(false),
	mSelfConstraintBlocks(NULL),
	mMaxPartitions(0),
	mMaxSolverPositionIterations(0),
	mMaxSolverVelocityIterations(0),
	mThresholdPairCount(0),
	mMaxArticulationLength(0),
	mContactDescPtr(NULL),
	mFrictionDescPtr(NULL),
	mArticulations(PX_DEBUG_EXP("PxsThreadContext::articulations")),
	mLocalNewTouchCount(0), 
	mLocalLostTouchCount(0)
  {
#if PX_ENABLE_SIM_STATS
	  mThreadSimStats.clear();
#endif
  }

void PxsThreadContext::resizeArrays(PxU32 bodyCount, PxU32 cmCount, PxU32 contactConstraintDescCount, PxU32 frictionConstraintDescCount, PxU32 articulationCount)
{
	// resize resizes smaller arrays to the exact target size, which can generate a lot of churn

	//bodyCoreArray.clear();
	bodyCoreArray.forceSize_Unsafe(0);
	bodyCoreArray.reserve(PxMax<PxU32>(Ps::nextPowerOfTwo(bodyCount), 64));
	bodyCoreArray.forceSize_Unsafe(bodyCount);
	//bodyCoreArray.resize(bodyCount);


	//accelerationArray.clear();
	accelerationArray.forceSize_Unsafe(0);
	accelerationArray.reserve((bodyCount+63)&~63);
	accelerationArray.forceSize_Unsafe(bodyCount);
	//accelerationArray.resize(bodyCount);

	//motionVelocityArray.clear();
	motionVelocityArray.forceSize_Unsafe(0);
	motionVelocityArray.reserve((bodyCount+63)&~63);
	motionVelocityArray.forceSize_Unsafe(bodyCount);
	//motionVelocityArray.resize(bodyCount);


	//constraintDescArray.clear();
	contactConstraintDescArray.forceSize_Unsafe(0);
	contactConstraintDescArray.reserve((contactConstraintDescCount+63)&~63);
	//constraintDescArray.resize(constraintDescCount);

	orderedContactConstraints.forceSize_Unsafe(0);
	orderedContactConstraints.reserve((contactConstraintDescCount + 66) & ~63);


	frictionConstraintDescArray.forceSize_Unsafe(0);
	frictionConstraintDescArray.reserve((frictionConstraintDescCount+63)&~63);

	// will be sized according to exact number of descs
	//orderedConstraints.clear();
	//orderedContactConstraints.forceSize_Unsafe(0);
	//orderedFrictionConstraints.forceSize_Unsafe(0);

	//mThresholdStream.clear();
	mThresholdStream.forceSize_Unsafe(0);
	mThresholdStream.reserve(PxMax<PxU32>(Ps::nextPowerOfTwo(cmCount), 64));
	mThresholdStream.forceSize_Unsafe(cmCount);
	//mThresholdStream.resize(cmCount);

	//mArticulations.clear();
	mArticulations.forceSize_Unsafe(0);
	mArticulations.reserve(PxMax<PxU32>(Ps::nextPowerOfTwo(articulationCount), 16));
	//mArticulations.resize(articulationCount);
	mArticulations.forceSize_Unsafe(articulationCount);

	mContactDescPtr = contactConstraintDescArray.begin();
	mFrictionDescPtr = frictionConstraintDescArray.begin();
}


void PxsThreadContext::reset(PxU32 shapeCount, PxU32 cmCount)
{
	// TODO: move these to the PxcNpThreadContext
	mFrictionPatchStreamPair.reset();
	mConstraintBlockStream.reset();
	mContactBlockStream.reset();
	mNpCacheStreamPair.reset();

	mLocalChangeTouch.clear();
	mLocalChangeTouch.resize(cmCount);
	mLocalNewTouchCount = 0;
	mLocalLostTouchCount = 0;

	mLocalChangedShapes.clear();
	mLocalChangedShapes.resize(shapeCount);
	mAccumulatedThresholdStream.clear();

	mContactDescPtr = contactConstraintDescArray.begin();
	mFrictionDescPtr = frictionConstraintDescArray.begin();

	mThresholdPairCount = 0;
	mAxisConstraintCount = 0;
	mMaxSolverPositionIterations = 0;
	mMaxSolverVelocityIterations = 0;
	mSuccessfulSpuConstraintPartition = false;
	mNumDifferentBodyConstraints = 0;
	mNumSelfConstraints = 0;
	mSelfConstraintBlocks = NULL;
	mNumSelfConstraintBlocks = 0;

	orderedContactConstraints.clear();
	//orderedFrictionConstraints.clear();
}

