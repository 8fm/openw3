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


#include "CmPhysXCommon.h"
#include "PsBitUtils.h"

#include "PxcSolverConstraintDesc.h"
#include "PxcSolverConstraint1D.h"
#include "PxcSolverContact.h"

#include "PxsSolverConstraintDesc.h"
#include "PxcSolverBody.h"

using namespace physx;

void partitionConstraints
(PxcSolverBody* PX_RESTRICT atoms, const PxU32 numAtoms, 
 const PxcSolverConstraintDesc* PX_RESTRICT constraintDescriptors, const PxU32 numConstraintDescriptors,
 PxU32** PX_RESTRICT constraintPartitions, PxU32* numConstraintsPerPartition, 
 const PxU32 numConstraintPartitions, const PxU32 constraintPartitionCapacity,
 PxcSolverConstraintDesc* PX_RESTRICT orderedConstraints)
{
	PX_ASSERT(32==numConstraintPartitions);
	PX_ASSERT(constraintPartitionCapacity==numConstraintDescriptors);
	PX_UNUSED(constraintPartitionCapacity);

	//Set the number of constraints per partition back to zero.
	for(PxU32 j=0;j<numConstraintPartitions;j++)
	{
		numConstraintsPerPartition[j]=0;
	}

	//Rest atom progress back to zero.
	for(PxU32 j=0;j<numAtoms;j++)
	{
		atoms[j].solverProgress=0;
	}
	//Sort the constraints into 32 partitions.
	for(PxU32 j=0;j<numConstraintDescriptors;j++)
	{
		//Compute an available partition for the two bodies.
		const PxcSolverConstraintDesc& constraintDesc=constraintDescriptors[j];
		PxcSolverBody* bodyA=constraintDesc.bodyA;
		PxcSolverBody* bodyB=constraintDesc.bodyB;
		PxU32 solverProgressA=bodyA->solverProgress;
		PxU32 solverProgressB=bodyB->solverProgress;
		PX_ASSERT(solverProgressA!=MAX_PERMITTED_SOLVER_PROGRESS || solverProgressB!=MAX_PERMITTED_SOLVER_PROGRESS);
		PxU32 availablePartition;
		if(0==~solverProgressA ||  0==~solverProgressB)
		{
			const PxU32 sum=(~solverProgressA + ~solverProgressB);
			availablePartition=Ps::lowestSetBit(sum);
		}
		else
		{
			availablePartition=Ps::lowestSetBit(~solverProgressA & ~solverProgressB);
			PX_ASSERT(availablePartition<numConstraintPartitions);
		}

		//Add the constraint to the available partition.
		PX_ASSERT(availablePartition<numConstraintPartitions);
		PxU32* availableConstraintPartition=constraintPartitions[availablePartition];
		PxU32 availableConstraintPartitionSize=numConstraintsPerPartition[availablePartition];
		PX_ASSERT(availableConstraintPartitionSize<constraintPartitionCapacity);
		availableConstraintPartition[availableConstraintPartitionSize]=j;
		availableConstraintPartitionSize++;
		numConstraintsPerPartition[availablePartition]=availableConstraintPartitionSize;

		//Tell the solver bodies that they've been added to the available partition.
		solverProgressA |= (1<<availablePartition);
		solverProgressB |= (1<<availablePartition);
		bodyA->solverProgress=solverProgressA;
		bodyB->solverProgress=solverProgressB;
	}
	//Add all the partitioned constraints back to a single array.
	PxU32 orderedConstraintCount=0;
	for(PxU32 k=0;k<32;k++)
	{
		PxU32* constraintPartition=constraintPartitions[k]; 
		const PxU32 constraintPartitionSize=numConstraintsPerPartition[k];
		for(PxU32 j=0;j<constraintPartitionSize;j++)
		{
			const PxU32 constraintId=constraintPartition[j];
			orderedConstraints[orderedConstraintCount]=constraintDescriptors[constraintId];
			orderedConstraintCount++;
		}
	}
	PX_ASSERT(orderedConstraintCount==numConstraintDescriptors);

	//Rest atom progress back to zero.
	for(PxU32 j=0;j<numAtoms;j++)
	{
		PX_ASSERT(atoms[j].solverProgress!=MAX_PERMITTED_SOLVER_PROGRESS);
		atoms[j].solverProgress=0;
	}
	//Reset all constraint progress back to zero.
	for(PxU32 j=0;j<numConstraintDescriptors;j++)
	{
		PxcSolverConstraintDesc& constraintDesc=orderedConstraints[j];
		constraintDesc.bodyASolverProgress = 0;
		constraintDesc.bodyBSolverProgress = 0;
	}
	//Set the constraint order dependencies of the solver.
	for(PxU32 j=0;j<numConstraintDescriptors;j++)
	{
		PxcSolverConstraintDesc& constraintDesc=orderedConstraints[j];

		PxcSolverBody* bodyA=constraintDesc.bodyA;
		PxcSolverBody* bodyB=constraintDesc.bodyB;

		PxU32 solverProgressA = bodyA->solverProgress;
		PxU32 solverProgressB = bodyB->solverProgress;
		constraintDesc.bodyASolverProgress = Ps::to16(solverProgressA);
		constraintDesc.bodyBSolverProgress = Ps::to16(solverProgressB);

		bodyA->solverProgress=(solverProgressA==MAX_PERMITTED_SOLVER_PROGRESS ? MAX_PERMITTED_SOLVER_PROGRESS : solverProgressA+1);
		bodyB->solverProgress=(solverProgressB==MAX_PERMITTED_SOLVER_PROGRESS ? MAX_PERMITTED_SOLVER_PROGRESS : solverProgressB+1);
	}
	//Rest atom progress back to zero.
	for(PxU32 j=0;j<numAtoms;j++)
	{
		PX_ASSERT(atoms[j].solverProgress!=MAX_PERMITTED_SOLVER_PROGRESS);
		atoms[j].maxSolverNormalProgress= Ps::to16(atoms[j].solverProgress);
		atoms[j].solverProgress=0;
	}
}



