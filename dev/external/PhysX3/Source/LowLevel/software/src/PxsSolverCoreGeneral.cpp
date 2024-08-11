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

#include "PsAllocator.h"
#include "PxPreprocessor.h"
//#ifndef PX_X360 //

#include <new>
#include <stdio.h>
#include "CmPhysXCommon.h"
#include "PxcSolverBody.h"
#include "PxcSolverConstraintDesc.h"
#include "PxcSolverConstraint1D.h"
#include "PxcSolverContact.h"
#include "PxcThresholdStreamElement.h"
#include "PxsSolverCoreGeneral.h"
#include "PxcArticulationHelper.h"
#include "PsAtomic.h"
#include "PsIntrinsics.h"
#include "PxcArticulationPImpl.h"

using namespace physx;

#ifdef PX_WINDOWS
#define PX_PROFILE_SOLVE_STALLS 0

PX_FORCE_INLINE PxU64 readTimer()
{
	return __rdtsc();
}

#endif


PxsSolveVMethod physx::solveV[3] = 
{
	solveVDefaultFriction,
	solveVCoulombFriction,
	solveVCoulombFriction
};

PxsSolveVBlockMethod physx::solveVBlock[3] = 
{
	solveVBlockDefaultFriction,
	solveVBlockCoulombFriction,
	solveVBlockCoulombFriction,
};

void physx::solveVDefaultFriction(SOLVEV_METHOD_ARGS)
{
	solverCore->solveV(dt, positionIterations, velocityIterations, atomListStart, atomDataList, solverBodyOffset, atomListSize,
		articulationListStart, articulationListSize, contactConstraintList, contactConstraintListSize, frictionConstraintList, frictionConstraintListSize,
		motionVelocityArray);
}

void physx::solveVCoulombFriction(SOLVEV_METHOD_ARGS)
{
	solverCore->solveVCoulomb(dt, positionIterations, velocityIterations, atomListStart, atomDataList, solverBodyOffset, atomListSize,
		articulationListStart, articulationListSize, contactConstraintList, contactConstraintListSize, frictionConstraintList, frictionConstraintListSize,
		motionVelocityArray);
}

void physx::solveVBlockDefaultFriction(SOLVEV_BLOCK_METHOD_ARGS)
{
	solverCore->solveV_Blocks(dt, positionIterations, velocityIterations, atomListStart,	atomDataList,solverBodyOffset, atomListSize, 
		articulationListStart, articulationListSize, contactConstraintList,	contactConstraintListSize,	frictionConstraintList, frictionConstraintConstraintListSize,
		contactConstraintBatchHeaders,frictionConstraintBatchHeaders, motionVelocityArray, thresholdStream,	thresholdStreamLength, outThresholdPairs);
}

void physx::solveVBlockCoulombFriction(SOLVEV_BLOCK_METHOD_ARGS)
{
	solverCore->solveVCoulomb_Blocks(dt, positionIterations, velocityIterations, atomListStart,	atomDataList,solverBodyOffset, atomListSize, 
		articulationListStart, articulationListSize, contactConstraintList,	contactConstraintListSize,	frictionConstraintList, frictionConstraintConstraintListSize,
		contactConstraintBatchHeaders,frictionConstraintBatchHeaders, motionVelocityArray, thresholdStream,	thresholdStreamLength, outThresholdPairs);
}


void physx::PxsSolverCoreRegisterArticulationFns()
{
	gVTableSolve[PXS_SC_TYPE_EXT_CONTACT] = solveExtContact;
	gVTableSolve[PXS_SC_TYPE_EXT_1D] = solveExt1D;

	gVTableSolveConclude[PXS_SC_TYPE_EXT_CONTACT] = solveConcludeExtContact;
	gVTableSolveConclude[PXS_SC_TYPE_EXT_1D] = solveConcludeExt1D;

	gVTableSolveBlock[PXS_SC_TYPE_EXT_CONTACT] = solveExtContactBlock;
	gVTableSolveBlock[PXS_SC_TYPE_EXT_1D] = solveExt1DBlock;

	gVTableSolveWriteBackBlock[PXS_SC_TYPE_EXT_CONTACT] = solveExtContactBlockWriteBack;
	gVTableSolveWriteBackBlock[PXS_SC_TYPE_EXT_1D] = solveExt1DBlockWriteBack;
	gVTableSolveConcludeBlock[PXS_SC_TYPE_EXT_CONTACT] = solveExtContactConcludeBlock;
	gVTableSolveConcludeBlock[PXS_SC_TYPE_EXT_1D] = solveExt1DConcludeBlock;


	gVTableSolveCoulomb[PXS_SC_TYPE_EXT_CONTACT] = solveExtContactCoulomb;
	gVTableSolveCoulomb[PXS_SC_TYPE_EXT_1D] = solveExt1D;

	gVTableSolveConcludeCoulomb[PXS_SC_TYPE_EXT_CONTACT] = solveConcludeExtContactCoulomb;
	gVTableSolveConcludeCoulomb[PXS_SC_TYPE_EXT_1D] = solveConcludeExt1D;

	gVTableSolveBlockCoulomb[PXS_SC_TYPE_EXT_CONTACT] = solveExtContactCoulombBlock;
	gVTableSolveBlockCoulomb[PXS_SC_TYPE_EXT_1D] = solveExt1DBlock;

	gVTableSolveWriteBackBlockCoulomb[PXS_SC_TYPE_EXT_CONTACT] = solveExtContactCoulombBlockWriteBack;
	gVTableSolveWriteBackBlockCoulomb[PXS_SC_TYPE_EXT_1D] = solveExt1DBlockWriteBack;
	gVTableSolveConcludeBlockCoulomb[PXS_SC_TYPE_EXT_CONTACT] = solveExtContactCoulombConcludeBlock;
	gVTableSolveConcludeBlockCoulomb[PXS_SC_TYPE_EXT_1D] = solveExt1DConcludeBlock;

	gVTableSolveCoulomb[PXS_SC_TYPE_EXT_FRICTION] = solveExtFriction;
	gVTableSolveBlockCoulomb[PXS_SC_TYPE_EXT_FRICTION] = solveExtFrictionBlock;
	gVTableSolveWriteBackBlockCoulomb[PXS_SC_TYPE_EXT_FRICTION] = solveExtFrictionBlockWriteBack;
	gVTableSolveConcludeBlockCoulomb[PXS_SC_TYPE_EXT_FRICTION] = solveExtFrictionBlock;

}

PxsSolverCoreGeneral* PxsSolverCoreGeneral::create()
{
	PxsSolverCoreGeneral* scg = reinterpret_cast<PxsSolverCoreGeneral*>(
		PX_ALLOC(sizeof(PxsSolverCoreGeneral), PX_DEBUG_EXP("PxsSolverCoreGeneral")));

	if(scg)
		new (scg) PxsSolverCoreGeneral;

	return scg;
}

void PxsSolverCoreGeneral::destroyV()
{
	this->~PxsSolverCoreGeneral();
	PX_FREE(this);
}

void PxsSolverCoreGeneral::solveV
(const PxReal /*dt*/, const PxU32 positionIterations, const PxU32 velocityIterations, 
 PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT /*atomDataList*/, const PxU32 /*solverBodyOffset*/, const PxU32 atomListSize,
 PxcArticulationSolverDesc* PX_RESTRICT articulationListStart, const PxU32 articulationListSize,
 PxcSolverConstraintDesc* PX_RESTRICT contactConstraintList, const PxU32 contactConstraintListSize,
 PxcSolverConstraintDesc* PX_RESTRICT /*frictionConstraintList*/, const PxU32 /*frictionConstraintListSize*/,
 Cm::SpatialVector* PX_RESTRICT motionVelocityArray) const
{
	PxcSolverContext cache;

	PX_ASSERT(velocityIterations >= 1);
	PX_ASSERT(positionIterations >= 1);
	cache.writeBackIteration = false;

	//0-(n-1) iterations
	for (PxI32 iteration = positionIterations; iteration > 0; iteration--)	//decreasing positive numbers == position iters
	{
		cache.doFriction = iteration<=3;
		for(PxU32 i=0;i<contactConstraintListSize;i++)
			gVTableSolve[*contactConstraintList[i].constraint](contactConstraintList[i], cache);
	}


	for (PxU32 baIdx = 0; baIdx < atomListSize; baIdx++)
	{
		Cm::SpatialVector& motionVel = motionVelocityArray[baIdx];
		const PxcSolverBody& atom = atomListStart[baIdx];
		motionVel.linear = atom.linearVelocity;
		motionVel.angular = atom.angularVelocity;
		PX_ASSERT(motionVel.linear.isFinite());
		PX_ASSERT(motionVel.angular.isFinite());
	}

	for (PxU32 i = 0; i < articulationListSize; i++)
		PxcArticulationPImpl::saveVelocity(articulationListStart[i]);

	for(PxU32 i=0;i<contactConstraintListSize;i++)
		gVTableConclude[*contactConstraintList[i].constraint](contactConstraintList[i], cache);

	for (PxI32 iteration = 1-velocityIterations; iteration <= 0; iteration++)	//nonpositive iterations == velocity iters
	{		
		for(PxU32 i=0;i<contactConstraintListSize;i++)
			gVTableSolve[*contactConstraintList[i].constraint](contactConstraintList[i], cache);
	}
}

void PxsSolverCoreGeneral::solveVCoulomb
(const PxReal /*dt*/, const PxU32 positionIterations, const PxU32 velocityIterations, 
 PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT /*atomDataList*/, const PxU32 /*solverBodyOffset*/, const PxU32 atomListSize,
 PxcArticulationSolverDesc* PX_RESTRICT articulationListStart, const PxU32 articulationListSize,
 PxcSolverConstraintDesc* PX_RESTRICT contactConstraintList, const PxU32 contactConstraintListSize,
 PxcSolverConstraintDesc* PX_RESTRICT frictionConstraintList, const PxU32 frictionConstraintListSize,
 Cm::SpatialVector* PX_RESTRICT motionVelocityArray) const
{
	PxcSolverContext cache;

	PX_ASSERT(velocityIterations >= 1);
	PX_ASSERT(positionIterations >= 1);


	//0-(n-1) iterations
	for (PxI32 iteration = positionIterations; iteration > 0; iteration--)	//decreasing positive numbers == position iters
	{
		for(PxU32 i=0;i<contactConstraintListSize;i++)
			gVTableSolveCoulomb[*contactConstraintList[i].constraint](contactConstraintList[i], cache);
	}

	const PxI32 count = positionIterations*2;
	for (PxI32 iteration = count; iteration > 0; iteration--)
	{
		for(PxU32 i = 0; i < frictionConstraintListSize; ++i)
			gVTableSolveCoulomb[*frictionConstraintList[i].constraint](frictionConstraintList[i], cache);
	}

	for (PxU32 baIdx = 0; baIdx < atomListSize; baIdx++)
	{
		Cm::SpatialVector& motionVel = motionVelocityArray[baIdx];
		const PxcSolverBody& atom = atomListStart[baIdx];
		motionVel.linear = atom.linearVelocity;
		motionVel.angular = atom.angularVelocity;
		PX_ASSERT(motionVel.linear.isFinite());
		PX_ASSERT(motionVel.angular.isFinite());
	}

	for (PxU32 i = 0; i < articulationListSize; i++)
		PxcArticulationPImpl::saveVelocity(articulationListStart[i]);

	for(PxU32 i=0;i<contactConstraintListSize;i++)
		gVTableConcludeCoulomb[*contactConstraintList[i].constraint](contactConstraintList[i], cache);

	for (PxI32 iteration = 1-velocityIterations; iteration <= 0; iteration++)	//nonpositive iterations == velocity iters
	{		
		for(PxU32 i=0;i<contactConstraintListSize;i++)
			gVTableSolveCoulomb[*contactConstraintList[i].constraint](contactConstraintList[i], cache);

		for(PxU32 i = 0; i < frictionConstraintListSize; ++i)
			gVTableSolveCoulomb[*frictionConstraintList[i].constraint](frictionConstraintList[i], cache);
	}


}

inline void BusyWaitState(volatile PxU32* state, const PxU32 requiredState)
{
	while(requiredState != *state );
}

inline void WaitBodyRequiredState(PxU32* state, const PxU32 requiredState
#if PX_PROFILE_SOLVE_STALLS
								  , PxU64& stallCount
#endif
								  )
{
	if(*state != requiredState)
	{
#if PX_PROFILE_SOLVE_STALLS
		PxU64 startStall = readTimer();
#endif
		BusyWaitState(state, requiredState);
#if PX_PROFILE_SOLVE_STALLS
		PxU64 endStall = readTimer();
		stallCount += (endStall - startStall);
#endif
	}
}

inline void BusyWaitStates(volatile PxU32* stateA, volatile PxU32* stateB, const PxU32 requiredStateA, const PxU32 requiredStateB)
{
	while(*stateA != requiredStateA);
	while(*stateB != requiredStateB);
}


PX_FORCE_INLINE void WaitBodyABodyBRequiredState(const PxcSolverConstraintDesc& desc, const PxU32 iterationA, const PxU32 iterationB
#if PX_PROFILE_SOLVE_STALLS
																	   ,PxU64& stallCount
#endif
																	   )
{

	//if(desc.bodyASolverProgress == 33 || desc.bodyBSolverProgress == 33)
	//{
	//	DebugBreak();
	//}

	

	PxcSolverBody* PX_RESTRICT pBodyA = desc.bodyA;
	PxcSolverBody* PX_RESTRICT pBodyB = desc.bodyB;

	const PxU32 requiredProgressA=(desc.bodyASolverProgress == 0xFFFF) ? 0xFFFF : desc.bodyASolverProgress + iterationA * pBodyA->maxSolverNormalProgress + iterationB * pBodyA->maxSolverFrictionProgress;
	const PxU32 requiredProgressB=(desc.bodyBSolverProgress == 0xFFFF) ? 0xFFFF : desc.bodyBSolverProgress + iterationA * pBodyB->maxSolverNormalProgress + iterationB * pBodyB->maxSolverFrictionProgress;
	PX_ASSERT(requiredProgressA!=0xFFFFFFFF || requiredProgressB!=0xFFFFFFFF);

	const PxU32 solverProgressA = pBodyA->solverProgress;
	const PxU32 solverProgressB = pBodyB->solverProgress;	

	if(solverProgressA != requiredProgressA || solverProgressB != requiredProgressB)
	{
#if PX_PROFILE_SOLVE_STALLS
		PxU64 startCount = readTimer();
#endif
		BusyWaitStates(&pBodyA->solverProgress, &pBodyB->solverProgress, requiredProgressA, requiredProgressB);
#if PX_PROFILE_SOLVE_STALLS
		PxU64 endCount = readTimer();
		stallCount += (endCount - startCount);
#endif
	}

	
}

//PX_FORCE_INLINE void WaitBodyABodyBRequiredState(const PxcSolverConstraintDesc& desc, const PxU32 iteration)
//{
//	PxcSolverBody* PX_RESTRICT pBodyA = desc.bodyA;
//	PxcSolverBody* PX_RESTRICT pBodyB = desc.bodyB;
//
//	const PxU32 requiredProgressA=(desc.bodyASolverProgress == 0xFFFF) ? 0xFFFF : desc.bodyASolverProgress + iteration * pBodyA->maxSolverNormalProgress;
//	const PxU32 requiredProgressB=(desc.bodyBSolverProgress == 0xFFFF) ? 0xFFFF : desc.bodyBSolverProgress + iteration * pBodyB->maxSolverNormalProgress;
//	PX_ASSERT(requiredProgressA!=0xFFFFFFFF || requiredProgressB!=0xFFFFFFFF);
//
//	const PxU32 solverProgressA = pBodyA->solverProgress;
//	const PxU32 solverProgressB = pBodyB->solverProgress;
//
//	if(solverProgressA != requiredProgressA || solverProgressB != requiredProgressB)
//	{
//		BusyWaitStates(&pBodyA->solverProgress, &pBodyB->solverProgress, requiredProgressA, requiredProgressB);
//	}
//}

PX_FORCE_INLINE void IncrementBodyProgress(const PxcSolverConstraintDesc& desc)
{
	PxcSolverBody* PX_RESTRICT pBodyA = desc.bodyA;
	PxcSolverBody* PX_RESTRICT pBodyB = desc.bodyB;

	const PxU32 maxProgressA = pBodyA->maxSolverNormalProgress;
	const PxU32 maxProgressB = pBodyB->maxSolverNormalProgress;

	/*const PxU32 requiredProgressA=(desc.bodyASolverProgress == 0xFFFF) ? 0xFFFF : desc.bodyASolverProgress + iteration * pBodyA->maxSolverProgress;
	const PxU32 requiredProgressB=(desc.bodyBSolverProgress == 0xFFFF) ? 0xFFFF : desc.bodyBSolverProgress + iteration * pBodyB->maxSolverProgress;*/

	//NB - this approach removes the need for an imul (which is a non-pipeline instruction on PPC chips)
	const PxU32 requiredProgressA=(maxProgressA == 0xFFFF) ? 0xFFFF : pBodyA->solverProgress + 1;
	const PxU32 requiredProgressB=(maxProgressB == 0xFFFF) ? 0xFFFF : pBodyB->solverProgress + 1;

	//Ensure that the writes have completed before we write the signal back
	//Ps::memoryBarrier();

	volatile PxU32* solveProgressA = &pBodyA->solverProgress;
	volatile PxU32* solveProgressB = &pBodyB->solverProgress;

	*solveProgressA=requiredProgressA;
	*solveProgressB=requiredProgressB;

}


class PxsBatchIterator
{
public:
	Ps::Array<PxsConstraintBatchHeader>& constraintBatchHeaders;
	PxU32 mCurrentIndex;

	PxsBatchIterator(Ps::Array<PxsConstraintBatchHeader>& _constraintBatchHeaders) : constraintBatchHeaders(_constraintBatchHeaders),
		mCurrentIndex(0)
	{
	}

	PX_FORCE_INLINE PxsConstraintBatchHeader& GetCurrentHeader(const PxU32 constraintIndex)
	{
		PxU32 currentIndex = mCurrentIndex;
		while((constraintIndex - constraintBatchHeaders[currentIndex].mStartIndex) >= constraintBatchHeaders[currentIndex].mStride)
			currentIndex = (currentIndex + 1)%constraintBatchHeaders.size();
		Ps::prefetchLine(&constraintBatchHeaders[currentIndex], 128);
		mCurrentIndex = currentIndex;
		return constraintBatchHeaders[currentIndex];
	}
private:
	PxsBatchIterator& operator=(const PxsBatchIterator&);
};

#if PX_USE_BLOCK_SOLVER

template<bool bWaitIncrement>
void SolveBlockParallel	(PxcSolverConstraintDesc* PX_RESTRICT constraintList, const PxU32 batchCount, const PxI32 index,  
						 const PxI32 headerCount, PxcSolverContext& cache, PxsBatchIterator& iterator,
						 SolveBlockMethod solveTable[], const PxI32 normalIteration, const PxI32 frictionIteration,
						 const PxU32 iteration
#if PX_PROFILE_SOLVE_STALLS
						, PxU64* stallCount = NULL
#endif
						)
{
	//Can solve in one block as long as
	//An object is only in the block once
	//and the constraints are of the same type
	
	//1 - determine if all the indices (endIndex - index) is in the same iteration...if this is the case, go down a different codepath...

	//const PxI32 iterationA = index/constraintListSize;
	const PxI32 indA = index - (iteration * headerCount);

	PxsConstraintBatchHeader* PX_RESTRICT headers = iterator.constraintBatchHeaders.begin();


	//This must be guaranteed by the calling code!
	//PX_ASSERT((headerCount - indA) >= ((PxI32)headerCount-1));

	//PxI32 i = indA;
	const PxI32 endIndex = indA + (PxI32)batchCount;
	for(PxI32 i = indA; i < endIndex; ++i)
	{
		PxsConstraintBatchHeader& header = headers[i];
		//const PxI32 batchEnd = header.mStartIndex + header.mStride;

		const PxI32 numToGrab = header.mStride;
		const PxI32 remainder = header.mStride - 1;
		PxcSolverConstraintDesc* PX_RESTRICT block = &constraintList[header.mStartIndex];
		
		
		for(PxI32 b = 0; b < numToGrab; ++b)
		{
			const PxI32 prefetch = PxMin(b+2, remainder);
			Ps::prefetchLine(block[b].constraint, 0);
			Ps::prefetchLine(block[b].constraint, 128);
			Ps::prefetchLine(block[b].constraint, 256);
			Ps::prefetchLine(block[prefetch].bodyA);
			Ps::prefetchLine(block[prefetch].bodyB);
			Ps::prefetchLine(&block[b], 256);
			//WaitBodyABodyBRequiredNormalState(block[b], iterationA);
			if(bWaitIncrement)
				WaitBodyABodyBRequiredState(block[b], normalIteration, frictionIteration
#if PX_PROFILE_SOLVE_STALLS
				, *stallCount
#endif
				);
		}
		//OK. We have a number of constraints to run...
		solveTable[header.mConstraintType](block, numToGrab, cache);

		//Increment body progresses
		if(bWaitIncrement)
		{
#if  1 //!(defined(PX_X86) || defined(PX_X64))
			//KS - with x86/64 hardware, volatile writes automatically act as memory barriers
			Ps::memoryBarrier();
#endif
			for(PxI32 j = 0; j < numToGrab; ++j)
			{
				IncrementBodyProgress(block[j]);	
			}
		}
	}
}


template<bool bWaitIncrement>
void SolveWriteBackBlockParallel (PxcSolverConstraintDesc* PX_RESTRICT constraintList, const PxU32 batchCount, const PxI32 index,  const PxI32 headerCount, PxcSolverContext& cache,
								  PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxI32* outThresholdPairs, PxsBatchIterator& iterator,
								  const PxU32 normalIteration, const PxU32 frictionIteration, const PxU32 iteration, SolveWriteBackBlockMethod solveTable[]
#if PX_PROFILE_SOLVE_STALLS
								  ,PxU64* stallCount = NULL
#endif
								  )
{
	//Can solve in one block as long as
	//An object is only in the block once
	//and the constraints are of the same type
	
	//1 - determine if all the indices (endIndex - index) is in the same iteration...if this is the case, go down a different codepath...

	//const PxI32 iterationA = index/constraintListSize;
	const PxI32 indA = index - (iteration * headerCount);

	PxsConstraintBatchHeader* PX_RESTRICT headers = iterator.constraintBatchHeaders.begin();


//	PxI32 i = indA;
	const PxI32 endIndex = indA + (PxI32)batchCount;
	for(PxI32 i = indA; i < endIndex; ++i)
	{
		PxsConstraintBatchHeader& header = headers[i];
		//const PxI32 batchEnd = header.mStartIndex + header.mStride;

		const PxI32 numToGrab = header.mStride;
		const PxI32 remainder = header.mStride - 1;
		PxcSolverConstraintDesc* PX_RESTRICT block = &constraintList[header.mStartIndex];
		
		
		for(PxI32 b = 0; b < numToGrab; ++b)
		{
			const PxI32 prefetch = PxMin(b+2, remainder);
			Ps::prefetchLine(block[b].constraint, 0);
			Ps::prefetchLine(block[b].constraint, 128);
			Ps::prefetchLine(block[b].constraint, 256);
			Ps::prefetchLine(block[prefetch].bodyA);
			Ps::prefetchLine(block[prefetch].bodyB);
			Ps::prefetchLine(&block[b], 256);
			//WaitBodyABodyBRequiredNormalState(block[b], iterationA);
			if(bWaitIncrement)
				WaitBodyABodyBRequiredState(block[b], normalIteration, frictionIteration
#if PX_PROFILE_SOLVE_STALLS
				, *stallCount
#endif
				);
		}
		//OK. We have a number of constraints to run...
		solveTable[header.mConstraintType](block, numToGrab, cache, thresholdStream, thresholdStreamLength, outThresholdPairs);

		//Increment body progresses
		if(bWaitIncrement)
		{
#if  1 //!(defined(PX_X86) || defined(PX_X64))
			Ps::memoryBarrier();
#endif
			for(PxI32 j = 0; j < numToGrab; ++j)
			{
				IncrementBodyProgress(block[j]);	
			}
		}
	}
}


#else

void SolveBlockParallel	(const PxcSolverConstraintDesc* PX_RESTRICT constraintList, const PxU32 constraintCount, const PxI32 index,  const PxI32 constraintListSize, 
						 PxcSolverContext& cache, PxsBatchIterator&, SolveMethod* solveTable)
{
	PxI32 endIndex = index + constraintCount;

	for(PxI32 i = index; i < endIndex; ++i)
	{
		const PxI32 iteration = i/constraintListSize;
		PxI32 a = i - (iteration * constraintListSize);
		WaitBodyABodyBRequiredState(constraintList[a], iteration);
		solveTable[*constraintList[a].constraint](constraintList[a], cache);
		Ps::memoryBarrier();
		IncrementBodyProgress(constraintList[a]);	
	}
}

void SolveWriteBackBlockParallel (const PxcSolverConstraintDesc* PX_RESTRICT constraintList, const PxU32 constraintCount, const PxI32 index,  const PxI32 constraintListSize, PxcSolverContext& cache,
								  PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxI32* outThresholdPairs, PxsBatchIterator&)
{	PxI32 endIndex = index + constraintCount;

	for(PxI32 i = index; i < endIndex; ++i)
	{
		const PxI32 iteration = i/constraintListSize;
		PxI32 a = i - (iteration * constraintListSize);
		WaitBodyABodyBRequiredState(constraintList[a], iteration);
		gVTableSolve[*constraintList[a].constraint](constraintList[a], cache);
		gVTableWriteBack[*constraintList[a].constraint](constraintList[a], cache);
		Ps::memoryBarrier();
		IncrementBodyProgress(constraintList[a]);	

		if(cache.mThresholdStreamIndex == cache.mThresholdStreamLength)
		{
			//Write back to global buffer
			PxI32 threshIndex = physx::shdfnd::atomicAdd(outThresholdPairs, cache.mThresholdStreamIndex) - cache.mThresholdStreamIndex;
			for(PxU32 a = 0; a < cache.mThresholdStreamIndex; ++a)
			{
				thresholdStream[a + threshIndex] = cache.mThresholdStream[a];
			}
			cache.mThresholdStreamIndex = 0;
		}

	}
}
#endif

void PxsSolverCoreGeneral::solveV_Blocks
(const PxReal /*dt*/, const PxU32 positionIterations, const PxU32 velocityIterations, 
 PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT atomDataList, const PxU32 /*solverBodyOffset*/, const PxU32 atomListSize,
 PxcArticulationSolverDesc* PX_RESTRICT articulationListStart, const PxU32 articulationListSize,
 PxcSolverConstraintDesc* PX_RESTRICT contactConstraintList, const PxU32 contactConstraintListSize,
 PxcSolverConstraintDesc* PX_RESTRICT /*frictionConstraintList*/, const PxU32 /*frictionConstraintListSize*/,
 Ps::Array<PxsConstraintBatchHeader>& contactConstraintBatchHeaders, Ps::Array<PxsConstraintBatchHeader>& frictionConstraintBatchHeaders,
 Cm::SpatialVector* PX_RESTRICT motionVelocityArray,PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxU32& outThresholdPairs) const
{
	PxcSolverContext cache;
	cache.solverBodyArray = atomDataList;
	cache.mThresholdStream			= thresholdStream;
	//Add 4 to avoid a hitting the store case in the writeBack code because this is unnecessary when writing directly to the buffer
	cache.mThresholdStreamLength	= thresholdStreamLength + 4;
	cache.mThresholdStreamIndex		= 0;
	cache.writeBackIteration = false;

	PxI32 batchCount = contactConstraintBatchHeaders.size();

	PX_ASSERT(velocityIterations >= 1);
	PX_ASSERT(positionIterations >= 1);

	if(contactConstraintListSize == 0)
	{
		for (PxU32 baIdx = 0; baIdx < atomListSize; baIdx++)
		{
			Cm::SpatialVector& motionVel = motionVelocityArray[baIdx];
			PxcSolverBody& atom = atomListStart[baIdx];
			motionVel.linear = atom.linearVelocity;
			motionVel.angular = atom.angularVelocity;
		}

		for (PxU32 i = 0; i < articulationListSize; i++)
			PxcArticulationPImpl::saveVelocity(articulationListStart[i]);

		return;
	}

	PxsBatchIterator contactIterator(contactConstraintBatchHeaders);
	PxsBatchIterator frictionIterator(frictionConstraintBatchHeaders);


	//0-(n-1) iterations
	PxI32 normalIter = 0;
	PxI32 frictionIter = 0;
	for (PxI32 iteration = positionIterations; iteration > 0; iteration--)	//decreasing positive numbers == position iters
	{
		cache.doFriction = iteration<=3;

		SolveBlockParallel<false>(contactConstraintList, batchCount, normalIter * batchCount, batchCount, 
			cache, contactIterator, iteration == 1 ? gVTableSolveConcludeBlock : gVTableSolveBlock, normalIter, frictionIter, normalIter);
		++normalIter;
	
	}

	for (PxU32 baIdx = 0; baIdx < atomListSize; baIdx++)
	{
		const PxcSolverBody& atom = atomListStart[baIdx];
		Cm::SpatialVector& motionVel = motionVelocityArray[baIdx];
		motionVel.linear = atom.linearVelocity;
		motionVel.angular = atom.angularVelocity;
	}
	

	for (PxU32 i = 0; i < articulationListSize; i++)
		PxcArticulationPImpl::saveVelocity(articulationListStart[i]);


	const PxI32 velItersMinOne = ((PxI32)velocityIterations) - 1;

	PxI32 iteration = 0;

	for(; iteration < velItersMinOne; ++iteration)
	{	

		SolveBlockParallel<false>(contactConstraintList, batchCount, normalIter * batchCount, batchCount, 
			cache, contactIterator, gVTableSolveBlock, normalIter, frictionIter, normalIter);
		++normalIter;

	}

	PxI32 threshPairOut = (PxI32)outThresholdPairs;
	cache.writeBackIteration = true;
	for(; iteration < (PxI32)velocityIterations; ++iteration)
	{
	/*	SolveWriteBackBlockParallel(constraintList, constraintListSize, iter *  constraintListSize, 
				constraintListSize, cache, thresholdStream, thresholdStreamLength, &threshPairOut, iterator);*/

		SolveWriteBackBlockParallel<false>(contactConstraintList, batchCount, normalIter * batchCount, batchCount,
			cache, thresholdStream, thresholdStreamLength, &threshPairOut, contactIterator, normalIter,
				frictionIter, normalIter, gVTableSolveWriteBackBlock);
		++normalIter;

	}

	outThresholdPairs = (PxU32)cache.mThresholdStreamIndex;
}

void PxsSolverCoreGeneral::solveVCoulomb_Blocks
(const PxReal /*dt*/, const PxU32 positionIterations, const PxU32 velocityIterations, 
 PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT atomDataList, const PxU32 /*solverBodyOffset*/, const PxU32 atomListSize,
 PxcArticulationSolverDesc* PX_RESTRICT articulationListStart, const PxU32 articulationListSize,
 PxcSolverConstraintDesc* PX_RESTRICT contactConstraintList, const PxU32 contactConstraintListSize,
 PxcSolverConstraintDesc* PX_RESTRICT frictionConstraintList, const PxU32 frictionConstraintListSize,
 Ps::Array<PxsConstraintBatchHeader>& contactConstraintBatchHeaders, Ps::Array<PxsConstraintBatchHeader>& frictionConstraintBatchHeaders,
 Cm::SpatialVector* PX_RESTRICT motionVelocityArray,PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxU32& outThresholdPairs) const
{
	PxcSolverContext cache;
	cache.solverBodyArray = atomDataList;
	cache.mThresholdStream			= thresholdStream;
	//Add 4 to avoid a hitting the store case in the writeBack code because this is unnecessary when writing directly to the buffer
	cache.mThresholdStreamLength	= thresholdStreamLength + 4;
	cache.mThresholdStreamIndex		= 0;
	cache.writeBackIteration = false;

	PX_ASSERT(velocityIterations >= 1);
	PX_ASSERT(positionIterations >= 1);

	if(contactConstraintListSize == 0)
	{
		for (PxU32 baIdx = 0; baIdx < atomListSize; baIdx++)
		{
			Cm::SpatialVector& motionVel = motionVelocityArray[baIdx];
			PxcSolverBody& atom = atomListStart[baIdx];
			motionVel.linear = atom.linearVelocity;
			motionVel.angular = atom.angularVelocity;
		}

		for (PxU32 i = 0; i < articulationListSize; i++)
			PxcArticulationPImpl::saveVelocity(articulationListStart[i]);

		return;
	}

	PxsBatchIterator contactIterator(contactConstraintBatchHeaders);
	PxsBatchIterator frictionIterator(frictionConstraintBatchHeaders);

	PxI32 batchCount = contactConstraintBatchHeaders.size();
	PxI32 frictionBatchCount = frictionConstraintBatchHeaders.size();


	//0-(n-1) iterations
	PxI32 normalIter = 0;
	PxI32 frictionIter = 0;
	for (PxI32 iteration = positionIterations; iteration > 0; iteration--)	//decreasing positive numbers == position iters
	{

		SolveBlockParallel<false>(contactConstraintList, batchCount, normalIter * batchCount, batchCount, 
			cache, contactIterator, iteration == 1 ? gVTableSolveConcludeBlockCoulomb : gVTableSolveBlockCoulomb, normalIter, frictionIter, normalIter);
		++normalIter;
	
	}

	if(frictionConstraintListSize>0)
	{
		const PxI32 numIterations = positionIterations * 2;
		for (PxI32 iteration = numIterations; iteration > 0; iteration--)	//decreasing positive numbers == position iters
		{
			SolveBlockParallel<false>(frictionConstraintList, frictionBatchCount, frictionIter * frictionBatchCount, frictionBatchCount, 
				cache, frictionIterator, iteration == 1 ? gVTableSolveConcludeBlockCoulomb : gVTableSolveBlockCoulomb, normalIter, frictionIter, frictionIter);
			++frictionIter;
		}
	}

	for (PxU32 baIdx = 0; baIdx < atomListSize; baIdx++)
	{
		const PxcSolverBody& atom = atomListStart[baIdx];
		Cm::SpatialVector& motionVel = motionVelocityArray[baIdx];
		motionVel.linear = atom.linearVelocity;
		motionVel.angular = atom.angularVelocity;
	}
	

	for (PxU32 i = 0; i < articulationListSize; i++)
		PxcArticulationPImpl::saveVelocity(articulationListStart[i]);


	const PxI32 velItersMinOne = ((PxI32)velocityIterations) - 1;

	PxI32 iteration = 0;

	for(; iteration < velItersMinOne; ++iteration)
	{	

		SolveBlockParallel<false>(contactConstraintList, batchCount, normalIter * batchCount, batchCount, 
			cache, contactIterator, gVTableSolveBlockCoulomb, normalIter, frictionIter, normalIter);
		++normalIter;

		if(frictionConstraintListSize)
		{
			SolveBlockParallel<false>(frictionConstraintList, frictionBatchCount, frictionIter * frictionBatchCount, frictionBatchCount, 
				cache, frictionIterator, gVTableSolveBlockCoulomb, normalIter, frictionIter, frictionIter);
			++frictionIter;
		}
	}



	PxI32 threshPairOut = (PxI32)outThresholdPairs;
	cache.writeBackIteration = true;
	for(; iteration < (PxI32)velocityIterations; ++iteration)
	{
	/*	SolveWriteBackBlockParallel(constraintList, constraintListSize, iter *  constraintListSize, 
				constraintListSize, cache, thresholdStream, thresholdStreamLength, &threshPairOut, iterator);*/

		SolveWriteBackBlockParallel<false>(contactConstraintList, batchCount, normalIter * batchCount, batchCount, 
				cache, thresholdStream, thresholdStreamLength, &threshPairOut, contactIterator, normalIter,
				frictionIter, normalIter, gVTableSolveWriteBackBlockCoulomb);
		++normalIter;

		if(frictionConstraintListSize)
		{
			SolveWriteBackBlockParallel<false>(frictionConstraintList, frictionBatchCount, frictionIter * frictionBatchCount, frictionBatchCount,
						cache, thresholdStream, thresholdStreamLength, &threshPairOut, frictionIterator, normalIter,
						frictionIter, frictionIter, gVTableSolveWriteBackBlockCoulomb);
				++frictionIter;
		}

	}

	outThresholdPairs = (PxU32)cache.mThresholdStreamIndex;
}



void PxsSolverCoreGeneral::solveVParallelAndWriteBack
(const PxReal /*dt*/, const PxU32 _positionIterations, const PxU32 _velocityIterations, 
 PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT atomDataList, const PxU32 /*solverBodyOffset*/, const PxU32 _atomListSize,
 PxcArticulationSolverDesc* PX_RESTRICT articulationListStart, const PxU32 _articulationListSize,
 PxcSolverConstraintDesc* PX_RESTRICT contactConstraintList, const PxU32 /*contactConstraintListSize*/,
 PxI32* pConstraintIndex, PxI32* pAtomListIndex,
 PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxI32* outThresholdPairs,
 Ps::Array<PxsConstraintBatchHeader>& contactConstraintBatches, Ps::Array<PxsConstraintBatchHeader>& /*frictionConstraintBatches*/,
 Cm::SpatialVector* PX_RESTRICT motionVelocityArray, PxI32& _normalIterations) const
{
#if PX_PROFILE_SOLVE_STALLS
	PxU64 startTime = readTimer();

	PxU64 stallCount = 0;
#endif

	PxcSolverContext cache;
	cache.solverBodyArray = atomDataList;

	const PxI32 UnrollCount = 4;
	const PxI32 SaveUnrollCount = 64;

	const PxI32 TempThresholdStreamSize = 32;
	PxcThresholdStreamElement tempThresholdStream[TempThresholdStreamSize];


	const PxI32 batchCount = contactConstraintBatches.size();
	cache.mThresholdStream = tempThresholdStream;
	cache.mThresholdStreamLength = TempThresholdStreamSize;
	cache.mThresholdStreamIndex = 0;
	cache.writeBackIteration = false;

	const PxI32 positionIterations = (PxI32)_positionIterations;
	const PxI32 velocityIterations = (PxI32)_velocityIterations;
	PX_UNUSED(velocityIterations);
	const PxI32 atomListSize = (PxI32)_atomListSize;
	const PxI32 articulationListSize = (PxI32)_articulationListSize;

	PX_ASSERT(velocityIterations >= 1);
	PX_ASSERT(positionIterations >= 1);

	PxI32 endIndexCount = UnrollCount;
	PxI32 index = physx::shdfnd::atomicAdd(pConstraintIndex, UnrollCount) - UnrollCount;
	
	PxsBatchIterator contactIter(contactConstraintBatches);


	PxI32 maxNormalIndex = 0;
	PxI32 normalIteration = 0;
	PxI32 frictionIteration = 0;
	PxU32 a = 0;
	for(PxU32 i = 0; i < 2; ++i)
	{
		physx::SolveBlockMethod* solveTable = i == 0 ? gVTableSolveBlock : gVTableSolveConcludeBlock;
		for(; a < positionIterations - 1 + i; ++a)
		{
			maxNormalIndex += batchCount;
			cache.doFriction = (positionIterations - a) <= 3;
			while(index < maxNormalIndex)
			{
				const PxI32 remainder = PxMin(maxNormalIndex - index, endIndexCount);
				SolveBlockParallel<true>(contactConstraintList, remainder, index, batchCount, cache, contactIter, solveTable, 
					normalIteration, frictionIteration, normalIteration
#if PX_PROFILE_SOLVE_STALLS
					, &stallCount
#endif
					);
				index += remainder;
				endIndexCount -= remainder;
				if(endIndexCount == 0)
				{
					endIndexCount = UnrollCount;
					index = physx::shdfnd::atomicAdd(pConstraintIndex, UnrollCount) - UnrollCount;
				}
			}
			++normalIteration;
		}

	}

	//Save velocity - articulated
	PxI32 endIndexCount2 = SaveUnrollCount;
	PxI32 index2 = physx::shdfnd::atomicAdd(pAtomListIndex, SaveUnrollCount) - SaveUnrollCount;

	{
		while(index2 < articulationListSize)
		{
			const PxI32 remainder = PxMin(SaveUnrollCount, (articulationListSize - index2));
			endIndexCount2 -= remainder;
			for(PxI32 a = 0; a < remainder; ++a, ++index2)
			{
				PxcFsData* fs = articulationListStart[index2].fsData;
				const PxU32 requiredProgress = normalIteration * fs->maxSolverNormalProgress + frictionIteration * fs->maxSolverFrictionProgress;
				WaitBodyRequiredState(&fs->solverProgress, requiredProgress
#if PX_PROFILE_SOLVE_STALLS
					, stallCount
#endif
					);
				PxcArticulationPImpl::saveVelocity(articulationListStart[index2]);
				Ps::memoryBarrier();
				fs->solverProgress += fs->maxSolverNormalProgress;
			}
			if(endIndexCount2 == 0)
			{
				index2 = physx::shdfnd::atomicAdd(pAtomListIndex, SaveUnrollCount) - SaveUnrollCount;
				endIndexCount2 = SaveUnrollCount;
			}
		}

		index2 -= articulationListSize;

		//save velocity
		
		while(index2 < atomListSize)
		{
			const PxI32 remainder = PxMin(endIndexCount2, (atomListSize - index2));
			endIndexCount2 -= remainder;
			for(PxI32 a = 0; a < remainder; ++a, ++index2)
			{
				Ps::prefetchLine(&atomListStart[index2 + 8]);
				Ps::prefetchLine(&motionVelocityArray[index2 + 8]);
				PxcSolverBody& b = atomListStart[index2];
				Cm::SpatialVector& motionVel = motionVelocityArray[index2];
				const PxU32 requiredProgress = normalIteration * b.maxSolverNormalProgress + frictionIteration * b.maxSolverFrictionProgress;
				WaitBodyRequiredState(&b.solverProgress, requiredProgress 
#if PX_PROFILE_SOLVE_STALLS
					,stallCount
#endif
					);
				motionVel.linear = b.linearVelocity;
				motionVel.angular = b.angularVelocity;
				PX_ASSERT(motionVel.linear.isFinite());
				PX_ASSERT(motionVel.angular.isFinite());
				Ps::memoryBarrier();
				b.solverProgress += b.maxSolverNormalProgress;
			}
			
			//Branch not required because this is the last time we use this atomic variable
			//if(index2 < articulationListSizePlusAtomListSize)
			{
				index2 = physx::shdfnd::atomicAdd(pAtomListIndex, SaveUnrollCount) - SaveUnrollCount - articulationListSize;
				endIndexCount2 = SaveUnrollCount;
			}
		}
		normalIteration++;
	}

	a = 0;
	for(; a < _velocityIterations-1; ++a)
	{
		maxNormalIndex += batchCount;
		
		while(index < maxNormalIndex)
		{
			const PxI32 remainder = PxMin(maxNormalIndex - index, endIndexCount);
			SolveBlockParallel<true>(contactConstraintList, remainder, index + batchCount, batchCount, cache, contactIter, gVTableSolveBlock, 
				normalIteration, 0, normalIteration
#if PX_PROFILE_SOLVE_STALLS
				,&stallCount
#endif
				);
			index += remainder;
			endIndexCount -= remainder;
			if(endIndexCount == 0)
			{
				endIndexCount = UnrollCount;
				index = physx::shdfnd::atomicAdd(pConstraintIndex, UnrollCount) - UnrollCount;
			}
		}
		++normalIteration;
	}

	//Last iteration - do writeback as well!
	//for(; a < velocityIterations; ++a)
	cache.writeBackIteration = true;
	{
		maxNormalIndex += batchCount;
		
		while(index < maxNormalIndex)
		{
			const PxI32 remainder = PxMin(maxNormalIndex - index, endIndexCount);
			SolveWriteBackBlockParallel<true>(contactConstraintList, remainder, index + batchCount, 
				batchCount, cache, thresholdStream, thresholdStreamLength, outThresholdPairs, 
				contactIter, normalIteration, 0, normalIteration, gVTableSolveWriteBackBlock 
#if PX_PROFILE_SOLVE_STALLS
				,&stallCount
#endif
				);

			index += remainder;
			endIndexCount -= remainder;
			if(endIndexCount == 0)
			{
				endIndexCount = UnrollCount;
				index = physx::shdfnd::atomicAdd(pConstraintIndex, UnrollCount) - UnrollCount;
			}
		}
		++normalIteration;

	}
	_normalIterations = normalIteration;

#if PX_PROFILE_SOLVE_STALLS
	PxU64 endTime = readTimer();
	PxReal totalTime = endTime - startTime;
	PxReal stallTime = stallCount;
	PxReal stallRatio = stallTime/totalTime;
	if(stallRatio > 0.2f)
		printf("Warning -- percentage time stalled = %f\n", stallRatio * 100.f);
#endif


}


void PxsSolverCoreGeneral::solveVCoulombParallelAndWriteBack
(const PxReal /*dt*/, const PxU32 _positionIterations, const PxU32 _velocityIterations, 
 PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT atomDataList, const PxU32 /*solverBodyOffset*/, const PxU32 _atomListSize,
 PxcArticulationSolverDesc* PX_RESTRICT articulationListStart, const PxU32 _articulationListSize,
 PxcSolverConstraintDesc* PX_RESTRICT contactConstraintList, const PxU32 /*contactConstraintListSize*/,
 PxcSolverConstraintDesc* PX_RESTRICT frictionConstraintList, const PxU32 /*frictionConstraintListSize*/,
 PxI32* pConstraintIndex, PxI32* pFrictionConstraintIndex, PxI32* pAtomListIndex,
 PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxI32* outThresholdPairs,
 Ps::Array<PxsConstraintBatchHeader>& contactConstraintBatches, Ps::Array<PxsConstraintBatchHeader>& frictionConstraintBatches,
 Cm::SpatialVector* PX_RESTRICT motionVelocityArray, PxI32& _normalIterations, PxI32& _frictionIterations) const
{
#if PX_PROFILE_SOLVE_STALLS
	PxU64 stallCount = 0;
#endif
	PxcSolverContext cache;
	cache.solverBodyArray = atomDataList;

	const PxI32 UnrollCount = 32;
	const PxI32 SaveUnrollCount = 64;

	const PxI32 TempThresholdStreamSize = 32;
	PxcThresholdStreamElement tempThresholdStream[TempThresholdStreamSize];


	const PxI32 batchCount = contactConstraintBatches.size();
	const PxI32 frictionBatchCount = frictionConstraintBatches.size();
	cache.mThresholdStream = tempThresholdStream;
	cache.mThresholdStreamLength = TempThresholdStreamSize;
	cache.mThresholdStreamIndex = 0;

	const PxI32 positionIterations = (PxI32)_positionIterations;
	const PxI32 velocityIterations = (PxI32)_velocityIterations;
	PX_UNUSED(velocityIterations);
	const PxI32 atomListSize = (PxI32)_atomListSize;
	const PxI32 articulationListSize = (PxI32)_articulationListSize;

	PX_ASSERT(velocityIterations >= 1);
	PX_ASSERT(positionIterations >= 1);

	PxI32 endIndexCount = UnrollCount;
	PxI32 index = physx::shdfnd::atomicAdd(pConstraintIndex, UnrollCount) - UnrollCount;
	PxI32 frictionIndex = physx::shdfnd::atomicAdd(pFrictionConstraintIndex, UnrollCount) - UnrollCount;
	
	//PxI32 maxContactIndex = PxMax(positionIterations - 3, 0) * contactConstraintListSize;

	PxsBatchIterator contactIter(contactConstraintBatches);
	PxsBatchIterator frictionIter(frictionConstraintBatches);


	PxI32 maxNormalIndex = 0;
	PxI32 frictionEndIndexCount = UnrollCount;
	PxI32 maxFrictionIndex = 0;

	PxI32 normalIteration = 0;
	PxI32 frictionIteration = 0;
	PxU32 a = 0;
	for(PxU32 i = 0; i < 2; ++i)
	{
		physx::SolveBlockMethod* solveTable = i == 0 ? gVTableSolveBlockCoulomb : gVTableSolveConcludeBlockCoulomb;
		for(; a < positionIterations - 1 + i; ++a)
		{
			maxNormalIndex += batchCount;
			while(index < maxNormalIndex)
			{
				const PxI32 remainder = PxMin(maxNormalIndex - index, endIndexCount);
				SolveBlockParallel<true>(contactConstraintList, remainder, index, batchCount, cache, contactIter, solveTable, 
					normalIteration, frictionIteration, normalIteration);
				index += remainder;
				endIndexCount -= remainder;
				if(endIndexCount == 0)
				{
					endIndexCount = UnrollCount;
					index = physx::shdfnd::atomicAdd(pConstraintIndex, UnrollCount) - UnrollCount;
				}
			}
			++normalIteration;
		}

	}


	for(PxU32 i = 0; i < 2; ++i)
	{
		physx::SolveBlockMethod* solveTable = i == 0 ? gVTableSolveBlockCoulomb : gVTableSolveConcludeBlockCoulomb;
		const PxI32 numIterations = positionIterations *2;
		for(; a <  numIterations - 1 + i; ++a)
		{
			maxFrictionIndex += frictionBatchCount;
			while(frictionIndex < maxFrictionIndex)
			{
				const PxI32 remainder = PxMin(maxFrictionIndex - frictionIndex, frictionEndIndexCount);
				SolveBlockParallel<true>(frictionConstraintList, remainder, frictionIndex, frictionBatchCount, cache, frictionIter, 
					solveTable, normalIteration, frictionIteration, frictionIteration);
				frictionIndex += remainder;
				frictionEndIndexCount -= remainder;
				if(frictionEndIndexCount == 0)
				{
					frictionEndIndexCount = UnrollCount;
					frictionIndex  = physx::shdfnd::atomicAdd(pFrictionConstraintIndex, UnrollCount) - UnrollCount;
				}
			}
			++frictionIteration;
			
		}

	}

	//Save velocity - articulated
	PxI32 endIndexCount2 = SaveUnrollCount;
	PxI32 index2 = physx::shdfnd::atomicAdd(pAtomListIndex, SaveUnrollCount) - SaveUnrollCount;

	{
		while(index2 < articulationListSize)
		{
			const PxI32 remainder = PxMin(SaveUnrollCount, (articulationListSize - index2));
			endIndexCount2 -= remainder;
			for(PxI32 a = 0; a < remainder; ++a, ++index2)
			{
				PxcFsData* fs = articulationListStart[index2].fsData;
				const PxU32 requiredProgress = normalIteration * fs->maxSolverNormalProgress + frictionIteration * fs->maxSolverFrictionProgress;
				WaitBodyRequiredState(&fs->solverProgress, requiredProgress
#if PX_PROFILE_SOLVE_STALLS
					, stallCount
#endif
					);
				PxcArticulationPImpl::saveVelocity(articulationListStart[index2]);
				Ps::memoryBarrier();
				fs->solverProgress += fs->maxSolverNormalProgress;
			}
			if(endIndexCount2 == 0)
			{
				index2 = physx::shdfnd::atomicAdd(pAtomListIndex, SaveUnrollCount) - SaveUnrollCount;
				endIndexCount2 = SaveUnrollCount;
			}
		}

		index2 -= articulationListSize;

		//save velocity
		
		while(index2 < atomListSize)
		{
			const PxI32 remainder = PxMin(endIndexCount2, (atomListSize - index2));
			endIndexCount2 -= remainder;
			for(PxI32 a = 0; a < remainder; ++a, ++index2)
			{
				Ps::prefetchLine(&atomListStart[index2 + 8]);
				Ps::prefetchLine(&motionVelocityArray[index2 + 8]);
				PxcSolverBody& b = atomListStart[index2];
				Cm::SpatialVector& motionVel = motionVelocityArray[index2];
				const PxU32 requiredProgress = normalIteration * b.maxSolverNormalProgress + frictionIteration * b.maxSolverFrictionProgress;
				WaitBodyRequiredState(&b.solverProgress, requiredProgress
#if PX_PROFILE_SOLVE_STALLS
					, stallCount
#endif
					);
				motionVel.linear = b.linearVelocity;
				motionVel.angular = b.angularVelocity;
				PX_ASSERT(motionVel.linear.isFinite());
				PX_ASSERT(motionVel.angular.isFinite());
				Ps::memoryBarrier();
				b.solverProgress += b.maxSolverNormalProgress;
			}
			
			//Branch not required because this is the last time we use this atomic variable
			//if(index2 < articulationListSizePlusAtomListSize)
			{
				index2 = physx::shdfnd::atomicAdd(pAtomListIndex, SaveUnrollCount) - SaveUnrollCount - articulationListSize;
				endIndexCount2 = SaveUnrollCount;
			}
		}
		normalIteration++;
	}

	a = 0;
	for(; a < _velocityIterations-1; ++a)
	{
		maxNormalIndex += batchCount;
		
		while(index < maxNormalIndex)
		{
			const PxI32 remainder = PxMin(maxNormalIndex - index, endIndexCount);
			SolveBlockParallel<true>(contactConstraintList, remainder, index + batchCount, batchCount, cache, contactIter, gVTableSolveBlockCoulomb, normalIteration, frictionIteration, normalIteration);
			index += remainder;
			endIndexCount -= remainder;
			if(endIndexCount == 0)
			{
				endIndexCount = UnrollCount;
				index = physx::shdfnd::atomicAdd(pConstraintIndex, UnrollCount) - UnrollCount;
			}
		}
		++normalIteration;

		maxFrictionIndex += frictionBatchCount;
		while(frictionIndex < maxFrictionIndex)
		{
			const PxI32 remainder = PxMin(maxFrictionIndex - frictionIndex, frictionEndIndexCount);
			SolveBlockParallel<true>(frictionConstraintList, remainder, frictionIndex, frictionBatchCount, cache, frictionIter, gVTableSolveBlockCoulomb, normalIteration, frictionIteration, frictionIteration);
			frictionIndex += remainder;
			frictionEndIndexCount -= remainder;
			if(frictionEndIndexCount == 0)
			{
				frictionEndIndexCount = UnrollCount;
				frictionIndex  = physx::shdfnd::atomicAdd(pFrictionConstraintIndex, UnrollCount) - UnrollCount;
			}
		}
		++frictionIteration;
	}

	{
		maxNormalIndex += batchCount;
		
		while(index < maxNormalIndex)
		{
			const PxI32 remainder = PxMin(maxNormalIndex - index, endIndexCount);
			SolveWriteBackBlockParallel<true>(contactConstraintList, remainder, index + batchCount, 
				batchCount, cache, thresholdStream, thresholdStreamLength, outThresholdPairs, 
				contactIter, normalIteration, frictionIteration, normalIteration, gVTableSolveWriteBackBlockCoulomb);

			index += remainder;
			endIndexCount -= remainder;
			if(endIndexCount == 0)
			{
				endIndexCount = UnrollCount;
				index = physx::shdfnd::atomicAdd(pConstraintIndex, UnrollCount) - UnrollCount;
			}
		}
		++normalIteration;

		maxFrictionIndex += frictionBatchCount;
		while(frictionIndex < maxFrictionIndex)
		{
			const PxI32 remainder = PxMin(maxFrictionIndex - frictionIndex, frictionEndIndexCount);
			//SolveWriteBackBlockParallel(frictionConstraintList, remainder, index, frictionConstraintListSize, cache, frictionIter, gVTableSolveBlock, normalIteration, frictionIteration, frictionIteration);
			SolveWriteBackBlockParallel<true>(frictionConstraintList, remainder, frictionIndex, frictionBatchCount, cache, thresholdStream, 
				thresholdStreamLength, outThresholdPairs, frictionIter, normalIteration, frictionIteration, frictionIteration, gVTableSolveWriteBackBlockCoulomb);

			frictionIndex += remainder;
			frictionEndIndexCount -= remainder;
			if(frictionEndIndexCount == 0)
			{
				frictionEndIndexCount = UnrollCount;
				frictionIndex  = physx::shdfnd::atomicAdd(pFrictionConstraintIndex, UnrollCount) - UnrollCount;
			}
		}
		++frictionIteration;
	}

	_normalIterations = normalIteration;
	_frictionIterations = frictionIteration;
}

void PxsSolverCoreGeneral::writeBackV
(const PxcSolverConstraintDesc* PX_RESTRICT constraintList, const PxU32 constraintListSize,
 PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxU32& outThresholdPairs,
 PxcSolverBodyData* atomListData, WriteBackMethod writeBackTable[]) const
{
	PxcSolverContext cache;
	cache.solverBodyArray			= atomListData;
	cache.mThresholdStream			= thresholdStream;
	cache.mThresholdStreamLength	= thresholdStreamLength;
	cache.mThresholdStreamIndex		= 0;

	for(PxU32 i=0;i<constraintListSize;i++)
	{
		PxcSolverBodyData& sbd0 = atomListData[constraintList[i].linkIndexA != PxcSolverConstraintDesc::NO_LINK ? 0 : constraintList[i].bodyADataIndex];
		PxcSolverBodyData& sbd1 = atomListData[constraintList[i].linkIndexB != PxcSolverConstraintDesc::NO_LINK ? 0 : constraintList[i].bodyBDataIndex];
		writeBackTable[*constraintList[i].constraint](constraintList[i], cache, sbd0, sbd1);
	}

	outThresholdPairs = cache.mThresholdStreamIndex;
}

void PxsSolverCoreGeneral::writeBackV
(const PxcSolverConstraintDesc* PX_RESTRICT constraintList, const PxU32 /*constraintListSize*/, PxsConstraintBatchHeader* batchHeaders, const PxU32 numBatches,
 PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxU32& outThresholdPairs,
 PxcSolverBodyData* atomListData, WriteBackBlockMethod writeBackTable[]) const
{
	PxcSolverContext cache;
	cache.solverBodyArray			= atomListData;
	cache.mThresholdStream			= thresholdStream;
	cache.mThresholdStreamLength	= thresholdStreamLength;
	cache.mThresholdStreamIndex		= 0;

	PxI32 outThreshIndex = 0;
	for(PxU32 j = 0; j < numBatches; ++j)
	{
		PxU8 type = *constraintList[batchHeaders[j].mStartIndex].constraint;
		writeBackTable[type](constraintList + batchHeaders[j].mStartIndex,
			batchHeaders[j].mStride, cache, thresholdStream, thresholdStreamLength, &outThreshIndex);
	}

	outThresholdPairs = outThreshIndex;
}


//#endif
