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


#ifndef PXS_SOLVERCORE_H
#define PXS_SOLVERCORE_H

#include "PxvConfig.h"
#include "PsArray.h"

namespace physx
{

struct PxcSolverBody;
struct PxcSolverBodyData;
struct PxcThresholdStreamElement;
struct PxcSolverConstraintDesc;
struct PxcArticulationSolverDesc;
struct PxsSolverConstraint;
struct PxsConstraintBatchHeader;
struct PxcSolverContext;
class PxsArticulation;


typedef void (*WriteBackMethod)(const PxcSolverConstraintDesc& desc, PxcSolverContext& cache, PxcSolverBodyData& sbd0, PxcSolverBodyData& sbd1);
typedef void (*SolveMethod)(const PxcSolverConstraintDesc& desc, PxcSolverContext& cache);
typedef void (*SolveBlockMethod)(const PxcSolverConstraintDesc* desc, const PxU32 constraintCount, PxcSolverContext& cache);
typedef void (*SolveWriteBackBlockMethod)(const PxcSolverConstraintDesc* desc, const PxU32 constraintCount, PxcSolverContext& cache,
										  PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxI32* outThresholdPairs);
typedef void (*WriteBackBlockMethod)(const PxcSolverConstraintDesc* desc, const PxU32 constraintCount, PxcSolverContext& cache,
										  PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxI32* outThresholdPairs);


/*!
Interface to constraint solver cores

*/    
class PxsSolverCore
{
public:
	virtual void destroyV() = 0;

	/*
	solves dual problem exactly by GS-iterating until convergence stops
	only uses regular velocity vector for storing results, and backs up initial state, which is restored.
	the solution forces are saved in a vector.

	state should not be stored, this function is safe to call from multiple threads.
	*/
	virtual void solveV
		(const PxReal dt, const PxU32 positionIterations, const PxU32 velocityIterations, 
		 PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT atomDataList, const PxU32 solverBodyOffset, const PxU32 atomListSize,
		 PxcArticulationSolverDesc *PX_RESTRICT articulationListStart, const PxU32 articulationListSize,
		 PxcSolverConstraintDesc* PX_RESTRICT constraintList, const PxU32 constraintListSize,
		 PxcSolverConstraintDesc* PX_RESTRICT frictionConstraintList, const PxU32 frictionConstraintConstraintListSize,
		 Cm::SpatialVector* PX_RESTRICT motionVelocityArray) 
		 const = 0;

	virtual void solveVCoulomb
		(const PxReal dt, const PxU32 positionIterations, const PxU32 velocityIterations, 
		 PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT atomDataList, const PxU32 solverBodyOffset, const PxU32 atomListSize,
		 PxcArticulationSolverDesc *PX_RESTRICT articulationListStart, const PxU32 articulationListSize,
		 PxcSolverConstraintDesc* PX_RESTRICT constraintList, const PxU32 constraintListSize,
		 PxcSolverConstraintDesc* PX_RESTRICT frictionConstraintList, const PxU32 frictionConstraintConstraintListSize,
		 Cm::SpatialVector* PX_RESTRICT motionVelocityArray) 
		 const = 0;

	virtual void solveVParallelAndWriteBack
		(const PxReal dt, const PxU32 _positionIterations, const PxU32 _velocityIterations, 
		 PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT atomDataList, const PxU32 solverBodyOffset, const PxU32 _atomListSize,
		 PxcArticulationSolverDesc* PX_RESTRICT articulationListStart, const PxU32 articulationListSize,
		 PxcSolverConstraintDesc* PX_RESTRICT constraintList, const PxU32 constraintListSize,
		 PxI32* pConstraintIndex, PxI32* pAtomListIndex,
		 PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxI32* outThresholdPairs,
		 Ps::Array<PxsConstraintBatchHeader>& constraintBatchHeaders, Ps::Array<PxsConstraintBatchHeader>& frictionBatchHeaders,
		 Cm::SpatialVector* PX_RESTRICT motionVelocityArray, PxI32& normalIterations) const = 0;

	virtual void solveVCoulombParallelAndWriteBack
		(const PxReal dt, const PxU32 _positionIterations, const PxU32 _velocityIterations, 
		 PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT atomDataList, const PxU32 solverBodyOffset, const PxU32 _atomListSize,
		 PxcArticulationSolverDesc* PX_RESTRICT articulationListStart, const PxU32 articulationListSize,
		 PxcSolverConstraintDesc* PX_RESTRICT contactConstraintList, const PxU32 contactConstraintListSize,
		 PxcSolverConstraintDesc* PX_RESTRICT frictionConstraintList, const PxU32 frictionConstraintConstraintListSize,
		 PxI32* pConstraintIndex, PxI32* pFrictionConstraintIndex, PxI32* pAtomListIndex,
		 PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxI32* outThresholdPairs,
		 Ps::Array<PxsConstraintBatchHeader>& constraintBatchHeaders, Ps::Array<PxsConstraintBatchHeader>& frictionConstraintBatches,
		 Cm::SpatialVector* PX_RESTRICT motionVelocityArray, PxI32& normalIterations, PxI32& frictionIterations) const = 0;


	virtual void solveV_Blocks 
		(const PxReal dt, const PxU32 positionIterations, const PxU32 velocityIterations, 
		 PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT atomDataList, const PxU32 solverBodyOffset, const PxU32 atomListSize,
		 PxcArticulationSolverDesc* PX_RESTRICT articulationListStart, const PxU32 articulationListSize,
		 PxcSolverConstraintDesc* PX_RESTRICT contactConstraintList,  const PxU32 constraintListSize,
		 PxcSolverConstraintDesc* PX_RESTRICT frictionConstraintList, const PxU32 frictionConstraintConstraintListSize,
		 Ps::Array<PxsConstraintBatchHeader>& contactConstraintBatchHeaders, Ps::Array<PxsConstraintBatchHeader>& frictionConstraintBatchHeaders,
		 Cm::SpatialVector* PX_RESTRICT motionVelocityArray, PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxU32& outThresholdPairs) const = 0;


	virtual void solveVCoulomb_Blocks
		(const PxReal dt, const PxU32 positionIterations, const PxU32 velocityIterations, 
		 PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT atomDataList, const PxU32 solverBodyOffset, const PxU32 atomListSize,
		 PxcArticulationSolverDesc* PX_RESTRICT articulationListStart, const PxU32 articulationListSize,
		 PxcSolverConstraintDesc* PX_RESTRICT contactConstraintList, const PxU32 contactConstraintListSize,
		 PxcSolverConstraintDesc* PX_RESTRICT frictionConstraintList, const PxU32 frictionConstraintListSize,
		 Ps::Array<PxsConstraintBatchHeader>& contactConstraintBatchHeaders, Ps::Array<PxsConstraintBatchHeader>& frictionConstraintBatchHeaders,
		 Cm::SpatialVector* PX_RESTRICT motionVelocityArray,PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxU32& outThresholdPairs) const = 0;

	virtual void writeBackV
		(const PxcSolverConstraintDesc* PX_RESTRICT constraintList, const PxU32 constraintListSize,
	 	 PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxU32& outThresholdPairs,
		 PxcSolverBodyData* atomListData, WriteBackMethod writeBackTable[]) const = 0;

	virtual void writeBackV
		(const PxcSolverConstraintDesc* PX_RESTRICT constraintList, const PxU32 constraintListSize, PxsConstraintBatchHeader* contactConstraintBatches, const PxU32 numConstraintBatches,
	 	 PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxU32& outThresholdPairs,
		 PxcSolverBodyData* atomListData, WriteBackBlockMethod writeBackTable[]) const = 0;
};

}

#endif