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


#ifndef PXS_CONSTRAINTPARTITION_H
#define PXS_CONSTRAINTPARTITION_H

#include "PxsDynamics.h"



namespace physx
{
struct ConstraintPartitionArgs
{
	enum
	{
		eMAX_NUM_ATOMS = 8192,
	};   

	//Input
	PxcSolverBody*							mAtoms;
	PxU32									mNumAtoms;
	PxcArticulationSolverDesc*				mArticulationPtrs;
	PxU32									mNumArticulationPtrs;
	PxcSolverConstraintDesc*				mContactConstraintDescriptors;
	PxU32									mNumContactConstraintDescriptors;
	PxcSolverConstraintDesc*				mFrictionConstraintDescriptors;
	PxU32									mNumFrictionConstraintDescriptors;
	//output
	PxcSolverConstraintDesc*				mOrderedContactConstraintDescriptors;
	PxcSolverConstraintDesc*				mOrderedFrictionConstraintDescriptors;
	PxcFsSelfConstraintBlock*				mSelfConstraintBlocks;
	PxU32									mNumSelfConstraintBlocks;
	PxU32									mNumDifferentBodyConstraints;
	PxU32									mNumSelfConstraints;
	Ps::Array<PxsConstraintBatchHeader>*	contactConstraintBatchHeader;
	Ps::Array<PxsConstraintBatchHeader>*	frictionConstraintBatchHeader;
	PxU32*									mConstraintsPerPartition;
	PxU32*									mFrictionConstraintsPerPartition;
};

//PxU32 PartitionConstraints(ConstraintPartitionArgs& args);
PxU32 PartitionContactConstraints(ConstraintPartitionArgs& args);
PxU32 PartitionFrictionConstraints(ConstraintPartitionArgs& args);

void ConstructBatchHeaders(PxcSolverConstraintDesc* eaOrderedConstraintDescriptors, const PxU32 numConstraintDescriptors, const PxU32* PX_RESTRICT constraintsPerPartition, 
						   Ps::Array<PxsConstraintBatchHeader>& batches);


PxU32 PostProcessConstraintPartitioning(PxcSolverBody* atoms, const PxU32 numAtoms, PxcArticulationSolverDesc* articulationDescs, const PxU32 numArticulations,
									   PxcSolverConstraintDesc* eaOrderedConstraintDescriptors, const PxU32 numConstraintDescriptors, 
									   PxcFsSelfConstraintBlock* selfConstraintBlocks, PxU32 numSelfConstraintBlocks);

void PostProcessFrictionConstraintPartitioning(PxcSolverBody* atoms, const PxU32 numAtoms, PxcArticulationSolverDesc* articulationDescs, const PxU32 numArticulations,
									   PxcSolverConstraintDesc* eaOrderedConstraintDescriptors, const PxU32 numConstraintDescriptors,
									    PxcFsSelfConstraintBlock* selfConstraintBlocks, PxU32 numSelfConstraintBlocks);

} // namespace physx



#endif // PXS_CONSTRAINT_PARTITIONING_H  

