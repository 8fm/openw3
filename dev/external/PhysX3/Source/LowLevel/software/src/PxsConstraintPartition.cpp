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

#include "PxsConstraintPartition.h"
#include "PxcSolverConstraintTypes.h"

#define INTERLEAVE_SELF_CONSTRAINTS 1


// todo: unify with PxcDynamics.cpp
#define SPU_CONSTRAINT_PARTITIONING 1
using namespace physx;

PX_FORCE_INLINE PxU32 getArticulationIndex(const uintptr_t eaFsData, const uintptr_t* eas, const PxU32 numEas)
{
	PxU32 index=0xffffffff;
	for(PxU32 i=0;i<numEas;i++)
	{
		if(eas[i]==eaFsData)
		{
			index=i;
			break;
		}
	}
	PX_ASSERT(index!=0xffffffff);
	return index;
}


//#if PX_CONSTRAINT_PARTITIONING || !SPU_CONSTRAINT_PARTITIONING
//#define MAX_NUM_CONSTRAINTS_PER_PARTITION 1024 
#define MAX_NUM_PARTITIONS 32



namespace
{

void ClassifyConstraintDesc(const PxcSolverConstraintDesc* descs, const PxU32 numConstraints, const uintptr_t eaAtoms, const PxU32 numAtoms, 
							PxU32* numConstraintsPerPartition, PxcSolverConstraintDesc* /*eaOrderedConstraintDescriptors*/, 
							PxU32& /*lastElement*/)
{
	const PxcSolverConstraintDesc* desc = descs;
	const PxU32 numConstraintsMin1 = numConstraints - 1;
	for(PxU32 i = 0; i < numConstraints; ++i, desc++)
	{
		
		const PxU32 prefetchOffset = PxMin(numConstraintsMin1 - i, 4u);
		Ps::prefetchLine(desc[prefetchOffset].constraint);
		Ps::prefetchLine(desc[prefetchOffset].bodyA);
		Ps::prefetchLine(desc[prefetchOffset].bodyB);
		Ps::prefetchLine(desc + 8);
		uintptr_t indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
		uintptr_t indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);

		//If it's flagged as PXS_SC_TYPE_RB_CONTACT and has a "static body", it's actually a kinematic so need to process first!
		const bool notContainsStatic = (indexA < numAtoms && indexB < numAtoms);// || *desc->constraint == PXS_SC_TYPE_RB_CONTACT;
		
		if(notContainsStatic)
		{
			const bool bodyAStatic = indexA >= numAtoms;
			const bool bodyBStatic = indexB >= numAtoms;

			PxU32* PX_RESTRICT consPerPart = numConstraintsPerPartition;
			//PxU32* PX_RESTRICT consPerSubPart = (*desc->constraint == PXS_SC_TYPE_RB_1D) ? numSolve1DsPerPartition : numNonSolve1DsPerPartition;

			/*PxU32 partitionsA=partitionArray[indexA];
			PxU32 partitionsB=partitionArray[indexB];*/
			PxU32 partitionsA=bodyAStatic ? 0 : desc->bodyA->solverProgress;
			PxU32 partitionsB=bodyBStatic ? 0 : desc->bodyB->solverProgress;
			//PX_ASSERT(partitionsA!=0xFFFFFFFF || partitionsB!=0xFFFFFFFF);

			PxU32 availablePartition;
			//do
			{
				const PxU32 combinedMask = (~partitionsA & ~partitionsB);
				availablePartition = combinedMask == 0 ? MAX_NUM_PARTITIONS : Ps::lowestSetBit(combinedMask);
				if(availablePartition == MAX_NUM_PARTITIONS)
				{
					//eaOrderedConstraintDescriptors[--lastElement] = *desc;
					continue;
				}

				partitionsA |= 1 << availablePartition;
				partitionsB |= 1 << availablePartition;
			}
			//while(consPerPart[availablePartition] >= MAX_NUM_CONSTRAINTS_PER_PARTITION);
			//No longer needed...we can have an

			if(!bodyAStatic)
				desc->bodyA->solverProgress = partitionsA;
			if(!bodyBStatic)
				desc->bodyB->solverProgress = partitionsB;

			if(availablePartition < MAX_NUM_PARTITIONS)
			{
				consPerPart[availablePartition]++;
				//consPerSubPart[availablePartition]++;
			}
		}
	}
}


struct ArticulatedSelfConstraintIds
{
	PxU16 articulatedSelfConstraintIds[MAX_NUM_SPU_ARTICULATIONS][MAX_NUM_SPU_ARTICULED_SELFCONSTRAINTS];

	/*PxU16* PX_RESTRICT operator [] (const uintptr_t index)
	{
		return &articulatedSelfConstraintIds[index][0];
	}*/
};

void ClassifyConstraintDescWithArticulations(const PxcSolverConstraintDesc* descs, const PxU32 numConstraints, const uintptr_t eaAtoms, const PxU32 numAtoms, 
											 PxU32* numConstraintsPerPartition,
											 uintptr_t* eaFsDatas, const PxU32 numArticulations, PxcSolverConstraintDesc* /*eaOrderedConstraintDescriptors*/, 
											 PxU32& /*lastElement*/)
{

	const PxU32 numAtomsPlusArtics = numAtoms + numArticulations;
	PX_UNUSED(numAtomsPlusArtics);
	const PxcSolverConstraintDesc* desc = descs;
	const PxU32 numConstraintsMin1 = numConstraints - 1;
	for(PxU32 i = 0; i < numConstraints; ++i, desc++)
	{
		const PxU32 prefetchOffset = PxMin(numConstraintsMin1 - i, 4u);
		Ps::prefetchLine(desc[prefetchOffset].constraint);
		Ps::prefetchLine(desc[prefetchOffset].bodyA);
		Ps::prefetchLine(desc[prefetchOffset].bodyB);
		Ps::prefetchLine(desc + 8);

		uintptr_t indexA,indexB;
		bool bBodyA=true;
		bool bBodyB=true;

		if(PxcSolverConstraintDesc::NO_LINK==desc->linkIndexA && PxcSolverConstraintDesc::NO_LINK==desc->linkIndexB)
		{
			//Two rigid bodies.
			indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
			indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);
			bBodyA=(indexA<numAtoms);
			bBodyB=(indexB<numAtoms);
		}
		else if(desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK && desc->linkIndexB != PxcSolverConstraintDesc::NO_LINK && desc->bodyA==desc->bodyB)
		{
			//Self-constraint of an articulated body.
			//We will deal with these at the very end after 
#if INTERLEAVE_SELF_CONSTRAINTS
			indexA = indexB = getArticulationIndex((uintptr_t)desc->articulationA, eaFsDatas, numArticulations);
			PX_ASSERT(indexA<numArticulations);
#else
			continue;
#endif
		}
		else if(desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK && desc->linkIndexB != PxcSolverConstraintDesc::NO_LINK)
		{
			//Two separate articulated bodies.
			indexA=numAtoms+getArticulationIndex((uintptr_t)desc->articulationA,eaFsDatas,numArticulations);
			indexB=numAtoms+getArticulationIndex((uintptr_t)desc->articulationB,eaFsDatas,numArticulations);
			PX_ASSERT(indexA<numAtomsPlusArtics);
			PX_ASSERT(indexB<numAtomsPlusArtics);
		}
		else if(desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK)
		{
			//One articulated, one rigid body.
			indexA=numAtoms+getArticulationIndex((uintptr_t)desc->articulationA,eaFsDatas,numArticulations);
			PX_ASSERT(indexA<numAtomsPlusArtics);
			indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);
			bBodyB=(indexB<numAtoms);
		}
		else 
		{
			//One articulated, one rigid body.
			PX_ASSERT(desc->articulationBLength);
			indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
			indexB=numAtoms+getArticulationIndex((uintptr_t)desc->articulationB,eaFsDatas,numArticulations);
			PX_ASSERT(indexB<numAtomsPlusArtics);
			bBodyA=(indexA<numAtoms);
		}

		//Don't consider articulated self-collision.  We'll do that at the end. 
		if(bBodyA && bBodyB)// || (*desc->constraint == PXS_SC_TYPE_RB_CONTACT))
		{
			//PX_ASSERT(partitionsA!=0xFFFFFFFF || partitionsB!=0xFFFFFFFF);

			PxU32 partitionsA = bBodyA ? desc->bodyA->solverProgress : 0;
			PxU32 partitionsB = bBodyB ? desc->bodyB->solverProgress : 0;

			PxU32* consPerPart = numConstraintsPerPartition;
			//PxU32* consPerSubPart = (*desc->constraint == PXS_SC_TYPE_RB_1D || *desc->constraint == PXS_SC_TYPE_EXT_1D) ? numSolve1DsPerPartition : numNonSolve1DsPerPartition;

			PxU32 availablePartition;
			//do
			{
				const PxU32 combinedMask = (~partitionsA & ~partitionsB);
				availablePartition = combinedMask == 0 ? MAX_NUM_PARTITIONS : Ps::lowestSetBit(combinedMask);
				if(availablePartition == MAX_NUM_PARTITIONS)
				{
					//partition = availablePartition;
					//eaOrderedConstraintDescriptors[--lastElement] = *desc;
					continue;
				}

				partitionsA |= 1 << availablePartition;
				partitionsB |= 1 << availablePartition;
			}
			//while(consPerPart[availablePartition] >= MAX_NUM_CONSTRAINTS_PER_PARTITION);

			if(bBodyA)
				desc->bodyA->solverProgress = partitionsA;
			if(bBodyB)
				desc->bodyB->solverProgress = partitionsB;

			if(availablePartition < MAX_NUM_PARTITIONS)
			{
				consPerPart[availablePartition]++;
				//consPerSubPart[availablePartition]++;
			}
		}
	}
}


//Iterates through the constraint array, merging static constraints to the end of the ordered constraint list. This is done by assigning to partitions but no actual 
//copying/recording of indices is made at this point
void MergeBackStaticConstraints(PxcSolverConstraintDesc* descs, const PxU32 numConstraints, const uintptr_t eaAtoms, const PxU32 numAtoms,
								PxU32* numConstraintsPerPartition,
								PxcSolverConstraintDesc* /*eaOrderedConstraintDescriptors*/, PxU32& /*lastElement*/)
{
	PxcSolverConstraintDesc* desc = descs;
	const PxU32 numConstraintsMin1 = numConstraints - 1;
	for(PxU32 i = 0; i < numConstraints; ++i, desc++)
	{
		const PxU32 prefetchOffset = PxMin(numConstraintsMin1 - i, 4u);
		Ps::prefetchLine(desc[prefetchOffset].constraint);
		Ps::prefetchLine(desc[prefetchOffset].bodyA);
		Ps::prefetchLine(desc[prefetchOffset].bodyB);
		Ps::prefetchLine(desc + 8);

		uintptr_t indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
		uintptr_t indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);

		const bool notContainsStatic = (indexA < numAtoms && indexB < numAtoms);// || *desc->constraint == PXS_SC_TYPE_RB_CONTACT;

		if(!notContainsStatic)
		{
			const bool bodyAStatic = indexA >= numAtoms;
			const bool bodyBStatic = indexB >= numAtoms;

			PxU32* PX_RESTRICT consPerPart = numConstraintsPerPartition;
			//PxU32* PX_RESTRICT consPerSubPart = (*desc->constraint == PXS_SC_TYPE_RB_1D) ? numSolve1DsPerPartition : numNonSolve1DsPerPartition;

			PxU32 partitionsA=(indexA<numAtoms) ? desc->bodyA->solverProgress : 0xFFFFFFFF;
			PxU32 partitionsB=(indexB<numAtoms) ? desc->bodyB->solverProgress : 0xFFFFFFFF;
			//PX_ASSERT(partitionsA!=0xFFFFFFFF || partitionsB!=0xFFFFFFFF);

			PxU32 availablePartition;
			//do
			{
				if(indexA >= numAtoms)
				{
					availablePartition=partitionsB == 0 ? 0 : Ps::highestSetBit(partitionsB)+1;
				}
				else
				{
					availablePartition=partitionsA == 0 ? 0 : Ps::highestSetBit(partitionsA)+1;
				}
				if(availablePartition == MAX_NUM_PARTITIONS)
				{
					continue;
				}

				partitionsA |= 1 << availablePartition;
				partitionsB |= 1 << availablePartition;
			}
			//while(consPerPart[availablePartition] >= MAX_NUM_CONSTRAINTS_PER_PARTITION);

			if(!bodyAStatic)
				desc->bodyA->solverProgress = partitionsA;
			if(!bodyBStatic)
				desc->bodyB->solverProgress = partitionsB;
			if(availablePartition < MAX_NUM_PARTITIONS)
			{
				consPerPart[availablePartition]++;
				//consPerSubPart[availablePartition]++;
			}
		}
	}
}

//Iterates through the constraint array, merging static constraints to the end of the ordered constraint list. This is done by assigning to partitions but no actual 
//copying/recording of indices is made at this point
void MergeBackStaticConstraintsWithArticulations(PxcSolverConstraintDesc* descs, const PxU32 numConstraints, const uintptr_t eaAtoms, const PxU32 numAtoms, 
												 PxU32* PX_RESTRICT numConstraintsPerPartition, PxcSolverConstraintDesc* /*eaOrderedConstraintDescriptors*/, 
												 PxU32& /*lastElement*/, uintptr_t* eaFsDatas, PxU32 numArticulations)
{

	const PxU32 numAtomsPlusArtics = numAtoms + numArticulations;
	PX_UNUSED(numAtomsPlusArtics);
	PxcSolverConstraintDesc* desc = descs;
	const PxU32 numConstraintsMin1 = numConstraints - 1;
	for(PxU32 i = 0; i < numConstraints; ++i, desc++)
	{
		const PxU32 prefetchOffset = PxMin(numConstraintsMin1 - i, 4u);
		Ps::prefetchLine(desc[prefetchOffset].constraint);
		Ps::prefetchLine(desc[prefetchOffset].bodyA);
		Ps::prefetchLine(desc[prefetchOffset].bodyB);
		Ps::prefetchLine(desc + 8);

		uintptr_t indexA,indexB;
		bool bBodyA=true;
		bool bBodyB=true;
		if(PxcSolverConstraintDesc::NO_LINK==desc->linkIndexA && PxcSolverConstraintDesc::NO_LINK==desc->linkIndexB)
		{
			//Two rigid bodies.
			indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
			indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);
			bBodyA=(indexA<numAtoms);
			bBodyB=(indexB<numAtoms);
		}
		else if(desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK && desc->linkIndexB != PxcSolverConstraintDesc::NO_LINK && desc->bodyA==desc->bodyB)
		{
#if INTERLEAVE_SELF_CONSTRAINTS
			continue;
#else
			indexA = indexB = getArticulationIndex((uintptr_t)desc->articulationA, eaFsDatas, numArticulations);
			PX_ASSERT(indexA<numArticulations);
			bBodyA=false;
			bBodyB=false;
#endif
		}
		else if(desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK && desc->linkIndexB != PxcSolverConstraintDesc::NO_LINK)
		{
			continue;
		}
		else if(desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK)
		{
			//One articulated, one rigid body.
			indexA=numAtoms+getArticulationIndex((uintptr_t)desc->articulationA,eaFsDatas,numArticulations);
			PX_ASSERT(indexA<numAtomsPlusArtics);
			indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);
			bBodyB=(indexB<numAtoms);
		}
		else 
		{
			//One articulated, one rigid body.
			PX_ASSERT(desc->articulationBLength);
			indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
			indexB=numAtoms+getArticulationIndex((uintptr_t)desc->articulationB,eaFsDatas,numArticulations);
			PX_ASSERT(indexB<numAtomsPlusArtics);
			bBodyA=(indexA<numAtoms);
		}

		//if one of these is false, then we have a static constraint
		if((!bBodyA || !bBodyB))// && !(*desc->constraint == PXS_SC_TYPE_RB_CONTACT))
		{
			PxU32* consPerPart = numConstraintsPerPartition;
			//PxU32* consPerSubPart = (*desc->constraint == PXS_SC_TYPE_RB_1D || *desc->constraint == PXS_SC_TYPE_EXT_1D) ? numSolve1DsPerPartition : numNonSolve1DsPerPartition;

			PxU32 partitionsA = bBodyA ? desc->bodyA->solverProgress : 0xffffffff;
			PxU32 partitionsB = bBodyB ? desc->bodyB->solverProgress : 0xffffffff;

			//PX_ASSERT(partitionsA!=0xFFFFFFFF || partitionsB!=0xFFFFFFFF);

			PxU32 availablePartition;
			//do
			{
				if(bBodyB)
				{
					availablePartition=partitionsB == 0 ? 0 : Ps::highestSetBit(partitionsB)+1;
				}
				else
				{
					availablePartition=partitionsA == 0 ? 0 : Ps::highestSetBit(partitionsA)+1;
				}
				if(availablePartition == MAX_NUM_PARTITIONS || !(bBodyA || bBodyB))
				{
					//eaOrderedConstraintDescriptors[lastElement--] = *desc;
					continue;
				}

				partitionsA |= 1 << availablePartition;
				partitionsB |= 1 << availablePartition;
			}
			//while(consPerPart[availablePartition] >= MAX_NUM_CONSTRAINTS_PER_PARTITION);

			if(bBodyA)
				desc->bodyA->solverProgress = partitionsA;
			if(bBodyB)
				desc->bodyB->solverProgress = partitionsB;

			consPerPart[availablePartition]++;
			//consPerSubPart[availablePartition]++;
		}
	}
}
 

void WriteDynamicConstraintsToPartitions(const PxcSolverConstraintDesc* descs, const PxU32 numConstraints, const uintptr_t eaAtoms, const PxU32 numAtoms, 
										 PxU32* numConstraintsPerPartition, PxU32* accumulatedConstraintsPerPartition,
										 PxcSolverConstraintDesc* eaOrderedConstraintDescriptors)
{

	const PxcSolverConstraintDesc* desc = descs;
	const PxU32 numConstraintsMin1 = numConstraints - 1;
	for(PxU32 i = 0; i < numConstraints; ++i, desc++)
	{

		const PxU32 prefetchOffset = PxMin(numConstraintsMin1 - i, 4u);
		Ps::prefetchLine(desc[prefetchOffset].constraint);
		Ps::prefetchLine(desc[prefetchOffset].bodyA);
		Ps::prefetchLine(desc[prefetchOffset].bodyB);
		Ps::prefetchLine(desc + 8);

		uintptr_t indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
		uintptr_t indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);

		const bool notContainsStatic = (indexA < numAtoms && indexB < numAtoms); //|| *desc->constraint == PXS_SC_TYPE_RB_CONTACT;


		if(notContainsStatic)
		{
			PxU32 partitionsA= indexA >= numAtoms ? 0 : desc->bodyA->solverProgress;
			PxU32 partitionsB= indexB >= numAtoms ? 0 : desc->bodyB->solverProgress;

			PxU32 availablePartition;
			//do
			{
				const PxU32 combinedMask = (~partitionsA & ~partitionsB);
				availablePartition = combinedMask == 0 ? MAX_NUM_PARTITIONS : Ps::lowestSetBit(combinedMask);
				if(availablePartition == MAX_NUM_PARTITIONS)
				{
					//Should already be written back in previous phase!!!!
					const PxU32 startIndex = accumulatedConstraintsPerPartition[availablePartition];

					//const bool bIsSolve1D = *desc->constraint == PXS_SC_TYPE_RB_1D;

					const PxU32 writeIndex = numConstraintsPerPartition[availablePartition];

					eaOrderedConstraintDescriptors[startIndex + writeIndex] = *desc;
					Ps::prefetchLine(&eaOrderedConstraintDescriptors[startIndex+writeIndex], 128);

					numConstraintsPerPartition[availablePartition]++;
					continue;
				}

				partitionsA |= 1 << availablePartition;
				partitionsB |= 1 << availablePartition;
			}
			//while(consPerPart[availablePartition] >= MAX_NUM_CONSTRAINTS_PER_PARTITION);

			if(indexA < numAtoms)
				desc->bodyA->solverProgress = partitionsA;
			if(indexB < numAtoms)
				desc->bodyB->solverProgress = partitionsB;

			if(availablePartition < MAX_NUM_PARTITIONS)
			{

				//Write this thing back....
				//This is the index of the first constraint in this partition
				const PxU32 startIndex = accumulatedConstraintsPerPartition[availablePartition];

				//const bool bIsSolve1D = *desc->constraint == PXS_SC_TYPE_RB_1D;

				const PxU32 writeIndex = numConstraintsPerPartition[availablePartition];

				eaOrderedConstraintDescriptors[startIndex + writeIndex] = *desc;
				Ps::prefetchLine(&eaOrderedConstraintDescriptors[startIndex+writeIndex], 128);

				numConstraintsPerPartition[availablePartition]++;
			}
		}
	}
}

PxU32 WriteDynamicConstraintsToPartitionsWithArticulations(const PxcSolverConstraintDesc* descs, const PxU32 numConstraints, const uintptr_t eaAtoms, const PxU32 numAtoms, 
														   PxU32* PX_RESTRICT numConstraintsPerPartition, PxU32* accumulatedConstraintsPerPartition,
														   PxcSolverConstraintDesc* eaOrderedConstraintDescriptors,
														   uintptr_t* eaFsDatas, const PxU32 numArticulations)
{
	const PxcSolverConstraintDesc* desc = descs;
	const PxU32 numAtomsPlusArtics = numAtoms + numArticulations;
	PX_UNUSED(numAtomsPlusArtics);
	PxU32 numWrittenConstraints = 0;
	const PxU32 numConstraintsMin1 = numConstraints - 1;
	for(PxU32 i = 0; i < numConstraints; ++i, desc++)
	{

		const PxU32 prefetchOffset = PxMin(numConstraintsMin1 - i, 4u);
		Ps::prefetchLine(desc[prefetchOffset].constraint);
		Ps::prefetchLine(desc[prefetchOffset].bodyA);
		Ps::prefetchLine(desc[prefetchOffset].bodyB);
		Ps::prefetchLine(desc + 8);

		uintptr_t indexA,indexB;
		bool bBodyA=true;
		bool bBodyB=true;
		if(PxcSolverConstraintDesc::NO_LINK==desc->linkIndexA && PxcSolverConstraintDesc::NO_LINK==desc->linkIndexB)
		{
			//Two rigid bodies.
			indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
			indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);
			bBodyA=(indexA<numAtoms);
			bBodyB=(indexB<numAtoms);
		}
		else if(desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK && desc->linkIndexB != PxcSolverConstraintDesc::NO_LINK && desc->bodyA==desc->bodyB)
		{
#if INTERLEAVE_SELF_CONSTRAINTS
			indexA = indexB = getArticulationIndex((uintptr_t)desc->articulationA, eaFsDatas, numArticulations);
			PX_ASSERT(indexA<numArticulations);
#else
			continue;
#endif
		}
		else if(desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK && desc->linkIndexB != PxcSolverConstraintDesc::NO_LINK)
		{
			//2 separate articulations!!!
			indexA=numAtoms+getArticulationIndex((uintptr_t)desc->articulationA,eaFsDatas,numArticulations);
			indexB=numAtoms+getArticulationIndex((uintptr_t)desc->articulationB,eaFsDatas,numArticulations);
			PX_ASSERT(indexA<numAtomsPlusArtics);
			PX_ASSERT(indexB<numAtomsPlusArtics);
		}
		else if(desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK)
		{
			//One articulated, one rigid body.
			indexA=numAtoms+getArticulationIndex((uintptr_t)desc->articulationA,eaFsDatas,numArticulations);
			PX_ASSERT(indexA<numAtomsPlusArtics);
			indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);
			bBodyB=(indexB<numAtoms);
		}
		else 
		{
			//One articulated, one rigid body.
			PX_ASSERT(desc->articulationBLength);
			indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
			indexB=numAtoms+getArticulationIndex((uintptr_t)desc->articulationB,eaFsDatas,numArticulations);
			PX_ASSERT(indexB<numAtomsPlusArtics);
			bBodyA=(indexA<numAtoms);
		}

		//if one of these is false, then we have a static constraint
		if((bBodyA && bBodyB))// || (*desc->constraint == PXS_SC_TYPE_RB_CONTACT))
		{

			PxU32 partitionsA= bBodyA ? desc->bodyA->solverProgress : 0;
			PxU32 partitionsB= bBodyB ? desc->bodyB->solverProgress : 0;

			PxU32 availablePartition;
			//do
			{
				const PxU32 combinedMask = (~partitionsA & ~partitionsB);
				availablePartition = combinedMask == 0 ? MAX_NUM_PARTITIONS : Ps::lowestSetBit(combinedMask);
				if(availablePartition == MAX_NUM_PARTITIONS)
				{
					const PxU32 startIndex = accumulatedConstraintsPerPartition[availablePartition];

					const PxU32 writeIndex = numConstraintsPerPartition[availablePartition];

					eaOrderedConstraintDescriptors[startIndex + writeIndex] = *desc;

					++numWrittenConstraints;

					numConstraintsPerPartition[availablePartition]++;
					continue;
				}

				partitionsA |= 1 << availablePartition;
				partitionsB |= 1 << availablePartition;
			}
			//while(consPerPart[availablePartition] >= MAX_NUM_CONSTRAINTS_PER_PARTITION);

			if(bBodyA)
				desc->bodyA->solverProgress = partitionsA;
			if(bBodyB)
				desc->bodyB->solverProgress = partitionsB;

			if(availablePartition < MAX_NUM_PARTITIONS)
			{

				//Write this thing back....
				//This is the index of the first constraint in this partition
				const PxU32 startIndex = accumulatedConstraintsPerPartition[availablePartition];

				const PxU32 writeIndex = numConstraintsPerPartition[availablePartition];

				eaOrderedConstraintDescriptors[startIndex + writeIndex] = *desc;

				++numWrittenConstraints;

				numConstraintsPerPartition[availablePartition]++;
			}
		}
	}
	return numWrittenConstraints;
}

//#pragma optimize("", off)
void WriteStaticConstraintsToPartitions(const PxcSolverConstraintDesc* descs, const PxU32 numConstraints, const uintptr_t eaAtoms, const PxU32 numAtoms, 
										PxU32* numConstraintsPerPartition, PxU32* accumulatedConstraintsPerPartition,
										PxcSolverConstraintDesc* eaOrderedConstraintDescriptors)
{
	const PxcSolverConstraintDesc* desc = descs;
	const PxU32 numConstraintsMin1 = numConstraints - 1;
	for(PxU32 i = 0; i < numConstraints; ++i, desc++)
	{
		const PxU32 prefetchOffset = PxMin(numConstraintsMin1 - i, 4u);
		Ps::prefetchLine(desc[prefetchOffset].constraint);
		Ps::prefetchLine(desc[prefetchOffset].bodyA);
		Ps::prefetchLine(desc[prefetchOffset].bodyB);
		Ps::prefetchLine(desc + 8);

		uintptr_t indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
		uintptr_t indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);

		const bool notContainsStatic = (indexA < numAtoms && indexB < numAtoms);// || *desc->constraint == PXS_SC_TYPE_RB_CONTACT;

		if(!notContainsStatic)
		{
			PxU32 partitionsA= indexA < numAtoms ? desc->bodyA->solverProgress : 0xffffffff;
			PxU32 partitionsB= indexB < numAtoms ? desc->bodyB->solverProgress : 0xffffffff;

			PxU32 availablePartition;
			//do
			{
				if(indexA >= numAtoms)
				{
					availablePartition=partitionsB == 0 ? 0 : Ps::highestSetBit(partitionsB)+1;
				}
				else
				{
					availablePartition=partitionsA == 0 ? 0 : Ps::highestSetBit(partitionsA)+1;
				}
				if(availablePartition == MAX_NUM_PARTITIONS)
				{
					//Should already be written back in previous phase!!!!
					const PxU32 startIndex = accumulatedConstraintsPerPartition[availablePartition];

					//	const bool bIsSolve1D = *desc->constraint == PXS_SC_TYPE_RB_1D;

					/*const PxU32 writeIndex = bIsSolve1D ? numSolve1DsWrittenPerPartition[availablePartition] : numSolve1DsPerPartition[availablePartition] 
					+ numNonSolve1DsWrittenPerPartition[availablePartition];*/

					const PxU32 writeIndex = numConstraintsPerPartition[availablePartition];

					eaOrderedConstraintDescriptors[startIndex + writeIndex] = *desc;
					Ps::prefetchLine(&eaOrderedConstraintDescriptors[startIndex + writeIndex], 128);

					numConstraintsPerPartition[availablePartition]++;
					continue;
				}

				partitionsA |= 1 << availablePartition;
				partitionsB |= 1 << availablePartition;
			}
			//while(consPerPart[availablePartition] >= MAX_NUM_CONSTRAINTS_PER_PARTITION);

			if(indexA < numAtoms)
				desc->bodyA->solverProgress = partitionsA;
			if(indexB < numAtoms)
				desc->bodyB->solverProgress = partitionsB;

			if(availablePartition < MAX_NUM_PARTITIONS)
			{
				//Write this thing back....
				//This is the index of the first constraint in this partition
				const PxU32 startIndex = accumulatedConstraintsPerPartition[availablePartition];

			//	const bool bIsSolve1D = *desc->constraint == PXS_SC_TYPE_RB_1D;

				/*const PxU32 writeIndex = bIsSolve1D ? numSolve1DsWrittenPerPartition[availablePartition] : numSolve1DsPerPartition[availablePartition] 
				+ numNonSolve1DsWrittenPerPartition[availablePartition];*/

				const PxU32 writeIndex = numConstraintsPerPartition[availablePartition];

				eaOrderedConstraintDescriptors[startIndex + writeIndex] = *desc;
				Ps::prefetchLine(&eaOrderedConstraintDescriptors[startIndex + writeIndex], 128);

				numConstraintsPerPartition[availablePartition]++;
			}
		}
	}
}

PxU32 WriteStaticConstraintsToPartitionsWithArticulations(const PxcSolverConstraintDesc* descs, const PxU32 numConstraints, const uintptr_t eaAtoms, const PxU32 numAtoms, 
														  PxU32* numConstraintsPerPartition, PxU32* accumulatedConstraintsPerPartition,
														  PxcSolverConstraintDesc* eaOrderedConstraintDescriptors,
														  uintptr_t* eaFsDatas, const PxU32 numArticulations)
{
	PxU32 numWrittenConstraints = 0;
	const PxU32 numAtomsPlusArtics = numAtoms + numArticulations;
	PX_UNUSED(numAtomsPlusArtics);
	const PxcSolverConstraintDesc* desc = descs;
	const PxU32 numConstraintsMin1 = numConstraints -1;
	for(PxU32 i = 0; i < numConstraints; ++i, desc++)
	{
		const PxU32 prefetchOffset = PxMin(numConstraintsMin1 - i, 4u);
		Ps::prefetchLine(desc[prefetchOffset].constraint);
		Ps::prefetchLine(desc[prefetchOffset].bodyA);
		Ps::prefetchLine(desc[prefetchOffset].bodyB);
		Ps::prefetchLine(desc + 8);

		uintptr_t indexA,indexB;
		bool bBodyA=true;
		bool bBodyB=true;
		if(PxcSolverConstraintDesc::NO_LINK==desc->linkIndexA && PxcSolverConstraintDesc::NO_LINK==desc->linkIndexB)
		{
			//Two rigid bodies.
			indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
			indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);
			bBodyA=(indexA<numAtoms);
			bBodyB=(indexB<numAtoms);
		}
		else if(desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK && desc->linkIndexB != PxcSolverConstraintDesc::NO_LINK && desc->bodyA==desc->bodyB)
		{
#if INTERLEAVE_SELF_CONSTRAINTS
			continue;
#else
			indexA = indexB = getArticulationIndex((uintptr_t)desc->articulationA, eaFsDatas, numArticulations);
			PX_ASSERT(indexA<numArticulations);
			bBodyA=false;
			bBodyB=false;
#endif
		}
		else if(desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK && desc->linkIndexB != PxcSolverConstraintDesc::NO_LINK)
		{
			continue;
		}
		else if(desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK)
		{
			//One articulated, one rigid body.
			indexA=numAtoms+getArticulationIndex((uintptr_t)desc->articulationA,eaFsDatas,numArticulations);
			PX_ASSERT(indexA<numAtomsPlusArtics);
			indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);
			bBodyB=(indexB<numAtoms);
		}
		else 
		{
			//One articulated, one rigid body.
			PX_ASSERT(desc->articulationBLength);
			indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
			indexB=numAtoms+getArticulationIndex((uintptr_t)desc->articulationB,eaFsDatas,numArticulations);
			PX_ASSERT(indexB<numAtomsPlusArtics);
			bBodyA=(indexA<numAtoms);
		}

		//if one of these is false, then we have a static constraint
		if((!bBodyA || !bBodyB))// && !(*desc->constraint == PXS_SC_TYPE_RB_CONTACT))
		{
			PxU32 partitionsA = bBodyA ? desc->bodyA->solverProgress : 0xffffffff;
			PxU32 partitionsB = bBodyB ? desc->bodyB->solverProgress : 0xffffffff;

			PxU32 availablePartition;
			//do
			{
				if(bBodyB)
				{
					availablePartition=partitionsB == 0 ? 0 : Ps::highestSetBit(partitionsB)+1;
				}
				else
				{
					availablePartition=partitionsA == 0 ? 0 : Ps::highestSetBit(partitionsA)+1;
				}
				if(availablePartition == MAX_NUM_PARTITIONS || 
					!(bBodyA || bBodyB))
				{
					const PxU32 startIndex = accumulatedConstraintsPerPartition[availablePartition];

					//const bool bIsSolve1D = *desc->constraint == PXS_SC_TYPE_RB_1D || *desc->constraint == PXS_SC_TYPE_EXT_1D;

					/*const PxU32 writeIndex = bIsSolve1D ? numSolve1DsWrittenPerPartition[availablePartition] : numSolve1DsPerPartition[availablePartition] 
					+ numNonSolve1DsWrittenPerPartition[availablePartition];*/
					const PxU32 writeIndex = numConstraintsPerPartition[availablePartition];

					eaOrderedConstraintDescriptors[startIndex + writeIndex] = *desc;
					++numWrittenConstraints;

					numConstraintsPerPartition[availablePartition]++;
					continue;
				}

				partitionsA |= 1 << availablePartition;
				partitionsB |= 1 << availablePartition;
			}
			//while(consPerPart[availablePartition] >= MAX_NUM_CONSTRAINTS_PER_PARTITION);

			if(bBodyA)
				desc->bodyA->solverProgress = partitionsA;
			if(bBodyB)
				desc->bodyB->solverProgress = partitionsB;

			if(availablePartition < MAX_NUM_PARTITIONS)
			{
				//Write this thing back....
				//This is the index of the first constraint in this partition
				const PxU32 startIndex = accumulatedConstraintsPerPartition[availablePartition];

				//const bool bIsSolve1D = *desc->constraint == PXS_SC_TYPE_RB_1D || *desc->constraint == PXS_SC_TYPE_EXT_1D;

				/*const PxU32 writeIndex = bIsSolve1D ? numSolve1DsWrittenPerPartition[availablePartition] : numSolve1DsPerPartition[availablePartition] 
				+ numNonSolve1DsWrittenPerPartition[availablePartition];*/
				const PxU32 writeIndex = numConstraintsPerPartition[availablePartition];

				eaOrderedConstraintDescriptors[startIndex + writeIndex] = *desc;
				++numWrittenConstraints;

				numConstraintsPerPartition[availablePartition]++;
			}
		}
	}
	return numWrittenConstraints;
}

}

namespace physx
{


PxU32 PartitionContactConstraints(ConstraintPartitionArgs& args) 
{
	//PIX_PROFILE_ZONE(PartitionContact_Constraint);
	PxU32 maxPartition = 0;
	//Unpack the input data.
	const PxU32 numAtoms=args.mNumAtoms;
	const uintptr_t eaAtoms=(uintptr_t)args.mAtoms;
	const PxU32	numArticulations=args.mNumArticulationPtrs;
	const PxcArticulationSolverDesc* articulationDescs=args.mArticulationPtrs;
	const PxU32 numConstraintDescriptors=args.mNumContactConstraintDescriptors;

	PxcSolverConstraintDesc* eaConstraintDescriptors=args.mContactConstraintDescriptors;
	PxcSolverConstraintDesc* eaOrderedConstraintDescriptors=args.mOrderedContactConstraintDescriptors;
	//PxcFsSelfConstraintBlock* eaSelfConstraintBlocks=args.mSelfConstraintBlocks;

	//Ps::Array<PxsConstraintBatchHeader>& batchHeaders = *args.contactConstraintBatchHeader;
	//batchHeaders.forceSize_Unsafe(0);

	//const PxU32 numAtomsPlusArtics=numAtoms+numArticulations;

	//Set the number of constraints per partition to zero.
	PxU32 numConstraintsPerPartition[MAX_NUM_PARTITIONS+1];
	PxMemZero(numConstraintsPerPartition, sizeof(numConstraintsPerPartition));

	for(PxU32 a = 0; a < numAtoms; ++a)
	{
		PxcSolverBody& body = args.mAtoms[a];
		Ps::prefetchLine(&args.mAtoms[a], 256);
		body.solverProgress = 0;
		body.maxSolverNormalProgress = 0;
	}

	PxU32 numOrderedConstraints=0;	

	PxU32 lastElement = numConstraintDescriptors;

	PxU32 numSelfConstraintBlocks=0;

   
	if(numArticulations == 0)
	{
		ClassifyConstraintDesc(eaConstraintDescriptors, numConstraintDescriptors, eaAtoms, numAtoms, numConstraintsPerPartition,
			eaOrderedConstraintDescriptors, lastElement);

		MergeBackStaticConstraints(eaConstraintDescriptors, numConstraintDescriptors, eaAtoms, numAtoms, numConstraintsPerPartition,
			eaOrderedConstraintDescriptors, lastElement);

		//const PxU32 numElementsInPartition32 = numConstraintDescriptors - lastElement;

		PxU32 accumulatedConstraintsPerPartition[MAX_NUM_PARTITIONS+1];
		PxU32 accumulation = 0;
		for(PxU32 a = 0; a < MAX_NUM_PARTITIONS+1; ++a)
		{
			accumulatedConstraintsPerPartition[a] = accumulation;
			accumulation += numConstraintsPerPartition[a];
			numConstraintsPerPartition[a] = 0;
		}

		for(PxU32 a = 0; a < numAtoms; ++a)
		{
			PxcSolverBody& body = args.mAtoms[a];
			Ps::prefetchLine(&args.mAtoms[a], 256);
			body.solverProgress = 0;
		}

		WriteDynamicConstraintsToPartitions(eaConstraintDescriptors, numConstraintDescriptors, eaAtoms, numAtoms, numConstraintsPerPartition,
			accumulatedConstraintsPerPartition, eaOrderedConstraintDescriptors);

		WriteStaticConstraintsToPartitions(eaConstraintDescriptors, numConstraintDescriptors, eaAtoms, numAtoms, numConstraintsPerPartition,
			accumulatedConstraintsPerPartition, eaOrderedConstraintDescriptors);

		numOrderedConstraints = numConstraintDescriptors;

	}
	else
	{
		PX_ALLOCA(_eaFsData, uintptr_t, numArticulations);
		uintptr_t* eaFsDatas = _eaFsData;
		for(PxU32 i=0;i<numArticulations;i++)
		{
			PxcFsData* data = articulationDescs[i].fsData;
			eaFsDatas[i]=(uintptr_t)data;
			data->solverProgress = 0;
			data->maxSolverFrictionProgress = 0;
			data->maxSolverNormalProgress = 0;
		}

		ClassifyConstraintDescWithArticulations(eaConstraintDescriptors, numConstraintDescriptors, eaAtoms, numAtoms, numConstraintsPerPartition,
			eaFsDatas, numArticulations,eaOrderedConstraintDescriptors, lastElement);

		MergeBackStaticConstraintsWithArticulations(eaConstraintDescriptors, numConstraintDescriptors, eaAtoms, numAtoms, 
			numConstraintsPerPartition, eaOrderedConstraintDescriptors, lastElement, eaFsDatas, numArticulations);

		//const PxU32 numElementsInPartition32 = numConstraintDescriptors - lastElement;

		lastElement = numConstraintDescriptors;


		PxU32 accumulatedConstraintsPerPartition[MAX_NUM_PARTITIONS+1];
		PxU32 accumulation = 0;
		for(PxU32 a = 0; a < MAX_NUM_PARTITIONS+1; ++a)
		{
			accumulatedConstraintsPerPartition[a] = accumulation;
			accumulation += numConstraintsPerPartition[a];
			numConstraintsPerPartition[a] = 0;
		}
		
		for(PxU32 a = 0; a < numAtoms; ++a)
		{
			PxcSolverBody& body = args.mAtoms[a];
			Ps::prefetchLine(&args.mAtoms[a], 256);
			body.solverProgress = 0;
		}

		for(PxU32 i=0;i<numArticulations;i++)
		{
			PxcFsData* data = articulationDescs[i].fsData;
			data->solverProgress = 0;
		}

		numOrderedConstraints = WriteDynamicConstraintsToPartitionsWithArticulations(eaConstraintDescriptors, numConstraintDescriptors, eaAtoms, numAtoms, numConstraintsPerPartition,
			accumulatedConstraintsPerPartition, eaOrderedConstraintDescriptors, eaFsDatas, numArticulations);

		numOrderedConstraints += WriteStaticConstraintsToPartitionsWithArticulations(eaConstraintDescriptors, numConstraintDescriptors, eaAtoms, numAtoms, numConstraintsPerPartition,
			accumulatedConstraintsPerPartition, eaOrderedConstraintDescriptors, eaFsDatas, numArticulations);

		//final bit (special to articulations) is to merge the articulations together with the constraints...

		//numOrderedConstraints = numConstraintDescriptors;

	}

	const PxU32 numConstraintsDifferentBodies=numOrderedConstraints;

	PX_ASSERT(numConstraintsDifferentBodies == numConstraintDescriptors);

	//Now handle the articulated self-constraints.
	PxU32 totalConstraintCount = numConstraintsDifferentBodies;

	//OK. Allocate the buffer of constraintBatchHeader
	//This is an estimate of how large it should be...it shouldn't be bigger than this but there's always a chance...
	//batchHeaders.reserve(256);		

	//ConstructBatchHeaders(eaOrderedConstraintDescriptors, numConstraintDescriptors, numConstraintsPerPartition, batchHeaders);

	args.mNumSelfConstraintBlocks=numSelfConstraintBlocks;

	//Dma back the number of self-constraints and the number of standard constraints involving different bodies.
	args.mNumDifferentBodyConstraints=numConstraintsDifferentBodies;
	args.mNumSelfConstraints=totalConstraintCount-numConstraintsDifferentBodies;

	for(PxU32 a = 0; a < MAX_NUM_PARTITIONS; ++a)
	{
		if(numConstraintsPerPartition[a] > 0)
			maxPartition = a;
	}

	for(PxU32 a = 0; a < MAX_NUM_PARTITIONS+1; ++a)
	{
		args.mConstraintsPerPartition[a] = numConstraintsPerPartition[a];
	}

	return maxPartition;
}


void ConstructBatchHeaders(PxcSolverConstraintDesc* eaOrderedConstraintDescriptors, const PxU32 numConstraintDescriptors, const PxU32* PX_RESTRICT constraintsPerPartition, 
						   Ps::Array<PxsConstraintBatchHeader>& batches)
{
	batches.forceSize_Unsafe(0);
	//OK - we have an array of constraint partitions, stating how many partitions we have....
	if(numConstraintDescriptors == 0)
		return;
	batches.reserve(256); //reserve a minimum of 256 headers
	PxcSolverConstraintDesc* desc = eaOrderedConstraintDescriptors;

	PxU32 accumulation = 0;
	for(PxU32 a = 0; a < 32; ++a)
	{
		//Loop through the partitions...identifying constraint batches
		if(constraintsPerPartition[a])
		{
			PxsConstraintBatchHeader batch;
			batch.mStartIndex = accumulation;
			batch.mConstraintType = *desc->constraint;
			const PxU32 constraintsPerPartitionMin1 = constraintsPerPartition[a]-1;
			for(PxU32 i = 0; i < constraintsPerPartition[a]; ++i, desc++)
			{
				const PxU32 prefetchIndex = PxMin(constraintsPerPartitionMin1-i, 4u);
				Ps::prefetchLine(desc[prefetchIndex].constraint);
				Ps::prefetchLine(desc, 256);
				if((*desc->constraint) != batch.mConstraintType)
				{
					batch.mStride = Ps::to16((accumulation + i) - batch.mStartIndex);
					batches.pushBack(batch);
					batch.mStartIndex = accumulation + i;
					batch.mConstraintType = *desc->constraint;
				}
			}
			accumulation += constraintsPerPartition[a];
			if(accumulation > batch.mStartIndex)
			{
				batch.mStride = Ps::to16(accumulation - batch.mStartIndex);
				batches.pushBack(batch);
			}
		}
	}
	//Remainder...
	for(PxU32 a = accumulation; a < numConstraintDescriptors; ++a)
	{
		PxsConstraintBatchHeader batch;
		batch.mStartIndex = a;
		batch.mConstraintType = *eaOrderedConstraintDescriptors[a].constraint;
		batch.mStride = 1;
		batches.pushBack(batch);
	}
}

} // namespace physx
 
//#endif // PX_CONSTRAINT_PARTITIONINC

namespace physx
{

PxU32 UpdateAtomProgresses(PxcSolverConstraintDesc* eaOrderedConstraintDescriptors, const PxU32 numConstraintDescriptors, const uintptr_t eaAtoms, const PxU32 numAtoms)
{
	PxU16 maxAtomProgress = 0;
	PxcSolverConstraintDesc* desc = eaOrderedConstraintDescriptors;
	const PxU32 numConstraintMin1 = numConstraintDescriptors - 1;
	for(PxU32 a = 0; a < numConstraintDescriptors; ++a, desc++)
	{
		const PxU16 prefetchOffset = Ps::to16(PxMin(numConstraintMin1 - a, 4u));
		Ps::prefetchLine(desc[prefetchOffset].bodyA);
		Ps::prefetchLine(desc[prefetchOffset].bodyB);
		Ps::prefetchLine(desc + 8);

		uintptr_t indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
		uintptr_t indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);

		const PxU16 solverProgressA = indexA < numAtoms ? desc->bodyA->maxSolverNormalProgress : 0xffff;
		const PxU16 solverProgressB = indexB < numAtoms ? desc->bodyB->maxSolverNormalProgress : 0xffff;

		desc->bodyASolverProgress = solverProgressA;
		desc->bodyBSolverProgress = solverProgressB;

		maxAtomProgress = PxMax(maxAtomProgress, PxMax((PxU16)(solverProgressA+1u), (PxU16)(solverProgressB+1u)));


		if(indexA < numAtoms)
			desc->bodyA->maxSolverNormalProgress = solverProgressA + 1;
		if(indexB < numAtoms)
			desc->bodyB->maxSolverNormalProgress = solverProgressB + 1;
	}
	return maxAtomProgress;
}

void UpdateAtomFrictionProgresses(PxcSolverConstraintDesc* eaOrderedConstraintDescriptors, const PxU32 numConstraintDescriptors, const uintptr_t eaAtoms, const PxU32 numAtoms)
{
	PxcSolverConstraintDesc* desc = eaOrderedConstraintDescriptors;
	const PxU32 numConstraintMin1 = numConstraintDescriptors - 1;
	for(PxU32 a = 0; a < numConstraintDescriptors; ++a, desc++)
	{
		const PxU32 prefetchOffset = PxMin(numConstraintMin1 - a, 4u);
		Ps::prefetchLine(desc[prefetchOffset].bodyA);
		Ps::prefetchLine(desc[prefetchOffset].bodyB);
		Ps::prefetchLine(desc + 8);

		uintptr_t indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
		uintptr_t indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);

		const PxU16 solverProgressA = indexA < numAtoms ? desc->bodyA->maxSolverFrictionProgress : 0xffff;
		const PxU16 solverProgressB = indexB < numAtoms ? desc->bodyB->maxSolverFrictionProgress : 0xffff;

		desc->bodyASolverProgress = solverProgressA;
		desc->bodyBSolverProgress = solverProgressB;

		if(indexA < numAtoms)
			desc->bodyA->maxSolverFrictionProgress = solverProgressA + 1;
		if(indexB < numAtoms)
			desc->bodyB->maxSolverFrictionProgress = solverProgressB + 1;
	}
}

PxU32 UpdateAtomProgressesWithArticulations(PxcSolverConstraintDesc* eaOrderedConstraintDescriptors, const PxU32 numConstraintDescriptors, const uintptr_t eaAtoms, 
										   const PxU32 numAtoms, uintptr_t* eaFsDatas, const PxU32 numArticulations)
{
	//const PxU32 numAtomsPlusArtics = numAtoms + numArticulations;
	PxcSolverConstraintDesc* desc = eaOrderedConstraintDescriptors;
	const PxU32 numConstraintsMin1 = numConstraintDescriptors - 1;
	PxU16 maxProgress = 0;
	for(PxU32 a = 0; a < numConstraintDescriptors; ++a, desc++)
	{
		const PxU32 prefetchOffset = PxMin(numConstraintsMin1 - a, 4u);
		Ps::prefetchLine(desc[prefetchOffset].bodyA);
		Ps::prefetchLine(desc[prefetchOffset].bodyB);
		Ps::prefetchLine(desc + 8);

		uintptr_t indexA;
		uintptr_t indexB;

		bool bBodyA = true;
		bool bBodyB = true;

		if(PxcSolverConstraintDesc::NO_LINK==desc->linkIndexA && PxcSolverConstraintDesc::NO_LINK==desc->linkIndexB)
		{
			//Two rigid bodies.
			indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
			indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);
			bBodyA=(indexA<numAtoms);
			bBodyB=(indexB<numAtoms);
		}
		else if(desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK && desc->linkIndexB != PxcSolverConstraintDesc::NO_LINK && desc->bodyA!=desc->bodyB)
		{
			indexA=numAtoms+getArticulationIndex((uintptr_t)desc->articulationA, eaFsDatas, numArticulations);
			indexB=numAtoms+getArticulationIndex((uintptr_t)desc->articulationB, eaFsDatas, numArticulations);
			PX_ASSERT(indexA<(numAtoms + numArticulations));
			PX_ASSERT(indexB<(numAtoms + numArticulations));
		}
		else if (desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK && desc->linkIndexB != PxcSolverConstraintDesc::NO_LINK && desc->bodyA == desc->bodyB)
		{
			indexA = indexB =numAtoms+getArticulationIndex((uintptr_t)desc->articulationA, eaFsDatas, numArticulations);
			//bBodyB = false;
		}
		else if(desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK)
		{
			//One articulated, one rigid body.
			indexA=numAtoms+getArticulationIndex((uintptr_t)desc->articulationA, eaFsDatas, numArticulations);
			indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);
			PX_ASSERT(indexA<(numAtoms + numArticulations));
			bBodyB=(indexB<numAtoms);
		}
		else 
		{
			//One articulated, one rigid body.
			indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
			indexB=numAtoms+getArticulationIndex((uintptr_t)desc->articulationB, eaFsDatas, numArticulations);
			bBodyA=(indexA<numAtoms);
		}

		const PxU16 bodyASolverProgress = bBodyA ? desc->bodyA->maxSolverNormalProgress : 0xffff;
		const PxU16 bodyBSolverProgress = bBodyB ? desc->bodyB->maxSolverNormalProgress : 0xffff;
		desc->bodyASolverProgress = bodyASolverProgress;
		desc->bodyBSolverProgress = bodyBSolverProgress;

		PX_ASSERT(desc->bodyA->solverProgress == 0xffff || desc->bodyA->maxSolverNormalProgress != 0xffff);
		PX_ASSERT(desc->bodyB->solverProgress == 0xffff || desc->bodyB->maxSolverNormalProgress != 0xffff);

		maxProgress = PxMax(maxProgress, PxMax((PxU16)(bodyASolverProgress + 1u), (PxU16)(bodyBSolverProgress + 1u)));

		if(bBodyA)
			desc->bodyA->maxSolverNormalProgress = bodyASolverProgress + 1;
		if(bBodyB)
			desc->bodyB->maxSolverNormalProgress = bodyBSolverProgress + 1;
	}
	return maxProgress;
}

void UpdateAtomFrictionProgressesWithArticulations(PxcSolverConstraintDesc* eaOrderedConstraintDescriptors, const PxU32 numConstraintDescriptors, const uintptr_t eaAtoms, 
										   const PxU32 numAtoms, uintptr_t* eaFsDatas, const PxU32 numArticulations)
{
	const PxU32 numAtomsPlusArtics = numAtoms + numArticulations;
	PX_UNUSED(numAtomsPlusArtics);
	PxcSolverConstraintDesc* desc = eaOrderedConstraintDescriptors;
	const PxU32 numConstraintDescMin1 = numConstraintDescriptors - 1;
	for(PxU32 a = 0; a < numConstraintDescriptors; ++a, desc++)
	{
		const PxU32 prefetchOffset = PxMin(numConstraintDescMin1 - a, 4u);
		Ps::prefetchLine(desc[prefetchOffset].bodyA);
		Ps::prefetchLine(desc[prefetchOffset].bodyB);
		Ps::prefetchLine(desc + 8);

		uintptr_t indexA;
		uintptr_t indexB;

		bool bBodyA = true;
		bool bBodyB = true;

		if(PxcSolverConstraintDesc::NO_LINK==desc->linkIndexA && PxcSolverConstraintDesc::NO_LINK==desc->linkIndexB)
		{
			//Two rigid bodies.
			indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
			indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);
			bBodyA=(indexA<numAtoms);
			bBodyB=(indexB<numAtoms);
		}
		else if(desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK && desc->linkIndexB != PxcSolverConstraintDesc::NO_LINK && desc->bodyA!=desc->bodyB)
		{
			indexA=numAtoms+getArticulationIndex((uintptr_t)desc->articulationA, eaFsDatas, numArticulations);
			indexB=numAtoms+getArticulationIndex((uintptr_t)desc->articulationB, eaFsDatas, numArticulations);
			PX_ASSERT(indexA<numAtomsPlusArtics);
			PX_ASSERT(indexB<numAtomsPlusArtics);
		}
		else if (desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK && desc->linkIndexB != PxcSolverConstraintDesc::NO_LINK && desc->bodyA == desc->bodyB)
		{
			indexA = indexB = numAtoms + getArticulationIndex((uintptr_t)desc->articulationA, eaFsDatas, numArticulations);
		}
		else if(desc->linkIndexA != PxcSolverConstraintDesc::NO_LINK)
		{
			//One articulated, one rigid body.
			indexA=numAtoms+getArticulationIndex((uintptr_t)desc->articulationA, eaFsDatas, numArticulations);
			indexB=((uintptr_t)desc->bodyB - eaAtoms)/sizeof(PxcSolverBody);
			PX_ASSERT(indexA<numAtomsPlusArtics);
			bBodyB=(indexB<numAtoms);
		}
		else 
		{
			//One articulated, one rigid body.
			indexA=((uintptr_t)desc->bodyA - eaAtoms)/sizeof(PxcSolverBody);
			indexB=numAtoms+getArticulationIndex((uintptr_t)desc->articulationB, eaFsDatas, numArticulations);
			bBodyA=(indexA<numAtoms);
		}

		desc->bodyASolverProgress = bBodyA ? desc->bodyA->maxSolverFrictionProgress : 0xffff;
		desc->bodyBSolverProgress = bBodyB ? desc->bodyB->maxSolverFrictionProgress : 0xffff;

		if(bBodyA)
			desc->bodyA->maxSolverFrictionProgress = desc->bodyASolverProgress + 1;
		if(bBodyB)
			desc->bodyB->maxSolverFrictionProgress = desc->bodyBSolverProgress + 1;
	}
}


PxU32 PostProcessConstraintPartitioning(PxcSolverBody* atoms, const PxU32 numAtoms, PxcArticulationSolverDesc* articulationDescs, const PxU32 numArticulations,
									   PxcSolverConstraintDesc* eaOrderedConstraintDescriptors, const PxU32 numConstraintDescriptors, 
									   PxcFsSelfConstraintBlock* selfConstraintBlocks, PxU32 numSelfConstraintBlocks)
{
	const uintptr_t eaAtoms=(uintptr_t)atoms;

	PxU32 totalConstraintCount = numConstraintDescriptors;

	for(PxU32 i=0;i<numAtoms;++i)
	{
		//Process the current body.
		Ps::prefetchLine(&atoms[i], 256);
		PxcSolverBody& body = atoms[i];
		body.solverProgress = 0;
		body.maxSolverNormalProgress = 0;
	}


	PxU32 maxProgress = 0;
	if(numArticulations == 0)
	{
		maxProgress = UpdateAtomProgresses(eaOrderedConstraintDescriptors, numConstraintDescriptors, eaAtoms, numAtoms);
	}
	else
	{
		PX_ALLOCA(_eaFsData, uintptr_t, numArticulations);
		uintptr_t* eaFsDatas = _eaFsData;
		for(PxU32 i=0;i<numArticulations;i++)
		{
			PxcFsData* data = articulationDescs[i].fsData;
			eaFsDatas[i]=(uintptr_t)data;
			data->solverProgress = 0;
			data->maxSolverFrictionProgress = 0;
			data->maxSolverNormalProgress = 0;
		}

		maxProgress = UpdateAtomProgressesWithArticulations(eaOrderedConstraintDescriptors, numConstraintDescriptors, eaAtoms, numAtoms, eaFsDatas, numArticulations);

		//Now do self-constraints

		//Fix up progresses
		for(PxU32 a = 0; a < numSelfConstraintBlocks; ++a)
		{
			totalConstraintCount += selfConstraintBlocks[a].numSelfConstraints;
			if(selfConstraintBlocks[a].numSelfConstraints>0)
			{
				PxcFsData* data = (PxcFsData*)selfConstraintBlocks[a].eaFsData;
				PxU16 requiredProgress = data->maxSolverNormalProgress;

				for(PxU32 b = 0; b < selfConstraintBlocks[a].numSelfConstraints; ++b)
				{
					PxcSolverConstraintDesc& desc = eaOrderedConstraintDescriptors[selfConstraintBlocks[a].startId + b];
					desc.bodyASolverProgress = requiredProgress;
					desc.bodyBSolverProgress = requiredProgress;
					//requiredProgress++;
				}
				data->maxSolverNormalProgress++;
			}
		}
	}

	//Copy 1st 3 constraints to the end of the array to enable SOA to block together objects in different iterations.
	PxU32 numToCopy = PxMin(3u, totalConstraintCount);
	for(PxU32 a = 0; a < numToCopy; ++a)
	{
		eaOrderedConstraintDescriptors[a+totalConstraintCount] = eaOrderedConstraintDescriptors[a];
	}
	return maxProgress;
}

void PostProcessFrictionConstraintPartitioning(PxcSolverBody* atoms, const PxU32 numAtoms, PxcArticulationSolverDesc* articulationDescs, const PxU32 numArticulations,
									   PxcSolverConstraintDesc* eaOrderedConstraintDescriptors, const PxU32 numConstraintDescriptors,
									    PxcFsSelfConstraintBlock* selfConstraintBlocks, PxU32 numSelfConstraintBlocks)
{
	const uintptr_t eaAtoms=(uintptr_t)atoms;

	PxU32 totalConstraintCount = numConstraintDescriptors;

	for(PxU32 i=0;i<numAtoms;++i)
	{
		//Process the current body.
		Ps::prefetchLine(&atoms[i], 256);
		PxcSolverBody& body = atoms[i];
		body.solverProgress = 0;
		body.maxSolverFrictionProgress = 0;
	}

	if(numArticulations == 0)
	{
		UpdateAtomFrictionProgresses(eaOrderedConstraintDescriptors, numConstraintDescriptors, eaAtoms, numAtoms);
	}
	else
	{
		PX_ALLOCA(_eaFsData, uintptr_t, numArticulations);
		uintptr_t* eaFsDatas = _eaFsData;

		for(PxU32 i=0;i<numArticulations;i++)
		{
			PxcFsData* data = articulationDescs[i].fsData;
			eaFsDatas[i]=(uintptr_t)data;
			data->solverProgress = 0;
			data->maxSolverFrictionProgress = 0;
		}

		UpdateAtomFrictionProgressesWithArticulations(eaOrderedConstraintDescriptors, numConstraintDescriptors, eaAtoms, numAtoms, eaFsDatas, numArticulations);

		for(PxU32 a = 0; a < numSelfConstraintBlocks; ++a)
		{
			totalConstraintCount += selfConstraintBlocks[a].numSelfConstraints;
			if(selfConstraintBlocks[a].numSelfConstraints>0)
			{
				PxcFsData* data = (PxcFsData*)selfConstraintBlocks[a].eaFsData;
				PxU16 requiredProgress = data->maxSolverFrictionProgress;

				for(PxU32 b = 0; b < selfConstraintBlocks[a].numSelfConstraints; ++b)
				{
					PxcSolverConstraintDesc& desc = eaOrderedConstraintDescriptors[selfConstraintBlocks[a].startId + b];
					desc.bodyASolverProgress = requiredProgress;
					desc.bodyBSolverProgress = requiredProgress;
					//requiredProgress++;
				}
				data->maxSolverFrictionProgress++;
			}
		}

	}

	////Copy 1st 3 constraints to the end of the array to enable SOA to block together objects in different iterations.
	//PxU32 numToCopy = PxMin(3u, totalConstraintCount);
	//for(PxU32 a = 0; a < numToCopy; ++a)
	//{
	//	eaOrderedConstraintDescriptors[a+totalConstraintCount] = eaOrderedConstraintDescriptors[a];
	//}

}
}
