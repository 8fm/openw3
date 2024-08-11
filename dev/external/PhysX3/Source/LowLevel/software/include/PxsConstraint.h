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


#ifndef PXS_CONSTRAINTSHADER_H
#define PXS_CONSTRAINTSHADER_H

#include "PxvShader.h"

#include "PxcSolverConstraintTypes.h"
#include "PxcSolverConstraint1D.h"
#include "PxcSolverConstraint1D4.h"
#include "PxsIslandManagerAux.h"
#include "PxcSolverConstraintDesc.h"

namespace physx
{

class PxsContext;
class PxsDynamicsContext;
class PxcConstraintBlockStream;
class SpuNpMemBlock;
struct PxcSolverConstraintDesc;
struct PxcSolverBody;
struct PxcSolverBodyData;
class PxsConstraintBlockManager;

//A struct used to construct a block of solver constraints
struct PxcSolverConstraint4Desc
{
	const PxsConstraint* constraint;
	const PxTransform* pose0;
	const PxTransform* pose1;
	const PxcSolverBody* solverBody0;
	const PxcSolverBody* solverBody1;
	PxcSolverBodyData* sBodyData0;
	PxcSolverBodyData* sBodyData1;
	PxConstraintSolverPrep solverPrep;
	const void* constantBlock;
	PxU32 constantBlockByteSize;
	PxcSolverConstraintDesc* desc;
	PxU32 startRowIndex;
	PxU32 numRows;
};

PxcSolverConstraintPrepState::Enum setupSolverConstraint4
		(PxcSolverConstraint4Desc* PX_RESTRICT constraintDescs,
		 const PxReal dt, const PxReal recipdt,
#ifdef __SPU__
		 SpuNpMemBlock& blockStream, uintptr_t* eaConstraintData);
#else
		 PxcConstraintBlockStream& blockStream, PxsConstraintBlockManager& constraintBlockManager);
#endif




PxU32 PxsSetupSolverConstraint(const PxsConstraint* constraint,
							   PxcSolverBodyData* solverBodyDataArray,
							   PxcConstraintBlockStream& blockStream,
							   PxReal dt, PxReal invdt,
							   PxcSolverConstraintDesc& desc,
							   PxsConstraintBlockManager& constraintBlockManager);


void PxsConstraintProject(const PxsConstraint* constraint, bool projectToBody0, PxReal dt);


class PxsConstraintHelper
{
public:

	static PxU32 setupSolverConstraint
		(const PxTransform& pose0, const PxTransform& pose1, const PxcSolverBody* solverBody0, const PxcSolverBody* solverBody1,
		 PxcSolverBodyData* sBodyData0, PxcSolverBodyData* sBodyData1,
		 const PxConstraintSolverPrep solverPrep, const void* constantBlock, const PxU32 constantBlockByteSize,
		 const PxReal dt, const PxReal recipdt, 
		 PxcSolverConstraintDesc& desc, const PxsConstraint& constraint,
#ifdef __SPU__
		 SpuNpMemBlock& blockStream, uintptr_t* eaConstraintData);
#else
		 PxcConstraintBlockStream& blockStream, PxsConstraintBlockManager& constraintBlockManager);
#endif
};


PX_INLINE void Px1DConstraintInit(Px1DConstraint& c)
{
	c.linear0 = PxVec3(0,0,0);	
	c.angular0 = PxVec3(0,0,0);

	c.linear1 = PxVec3(0,0,0);	
	c.angular1 = PxVec3(0,0,0);

	c.geometricError = 0;
	c.velocityTarget = 0;
	c.forInternalUse = 0;

	c.minImpulse = -PX_MAX_REAL;
	c.maxImpulse = PX_MAX_REAL;

	c.mods.spring.stiffness = 0;
	c.mods.spring.damping = 0;
		
	c.flags = Px1DConstraintFlags();
	c.solveHint = PxConstraintSolveHint::eNONE;
}

}

#endif
