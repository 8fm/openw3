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

#ifndef PXC_SOLVERCONSTRAINT1D4_H
#define PXC_SOLVERCONSTRAINT1D4_H


#include "PxvConfig.h"
#include "PxcSolverConstraintTypes.h"
#include "PxVec3.h"
#include "PxcArticulation.h"
#include "PxcSolverConstraint1D.h"

namespace physx
{
struct PxcSolverConstraint1DHeader4
{
	PxU8	type;			// enum SolverConstraintType - must be first byte
	PxU8	pad0[3];
	//These counts are the max of the 4 sets of data.
	//When certain pairs have fewer constraints than others, they are padded with 0s so that no work is performed but 
	//calculations are still shared (afterall, they're computationally free because we're doing 4 things at a time in SIMD)
	PxU32	count;
	PxU8	count0, count1, count2, count3;
	PxU32	pad[1];

	Vec4V	linBreakImpulse;
	Vec4V	angBreakImpulse;
	Vec4V	invMass0Dom0;
	Vec4V	invMass1Dom1;

	Vec4V	body0WorkOffsetX;
	Vec4V	body0WorkOffsetY;
	Vec4V	body0WorkOffsetZ;
};

struct PxcSolverConstraint1DBase4 
{
public:
	Vec4V		lin0X;
	Vec4V		lin0Y;
	Vec4V		lin0Z;
	Vec4V		ang0X;
	Vec4V		ang0Y;
	Vec4V		ang0Z;
	Vec4V		ang0InvInertiaX;
	Vec4V		ang0InvInertiaY;
	Vec4V		ang0InvInertiaZ;
	Vec4V		constant;
	Vec4V		unbiasedConstant;
	Vec4V		velMultiplier;
	Vec4V		impulseMultiplier;
	Vec4V		minImpulse;
	Vec4V		maxImpulse;
	Vec4V		appliedForce;
	PxU32		flags[4];
};

PX_COMPILE_TIME_ASSERT(sizeof(PxcSolverConstraint1DBase4) == 272);

struct PxcSolverConstraint1DDynamic4 : public PxcSolverConstraint1DBase4
{
	Vec4V		lin1X;
	Vec4V		lin1Y;
	Vec4V		lin1Z;
	Vec4V		ang1X;
	Vec4V		ang1Y;
	Vec4V		ang1Z;
	Vec4V		ang1InvInertia1X;
	Vec4V		ang1InvInertia1Y;
	Vec4V		ang1InvInertia1Z;
};
PX_COMPILE_TIME_ASSERT(sizeof(PxcSolverConstraint1DDynamic4) == 416);

}

#endif