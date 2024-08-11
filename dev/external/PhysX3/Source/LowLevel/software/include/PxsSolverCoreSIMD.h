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


#ifndef PXS_SOLVERCORESIMD_H
#define PXS_SOLVERCORESIMD_H

#include "PxsSolverCore.h"

namespace physx
{

class PxsSolverCoreSIMD : public PxsSolverCore
{
public:
	static PxsSolverCoreSIMD* create();

	// Implements PxsSolverCore
	virtual void destroyV();

	//! Solve
	virtual void solveV
		(const PxReal dt, const PxU32 positionIterations, const PxU32 velocityIterations, 
		PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT atomDataList, const PxU32 solverBodyOffset, const PxU32 atomListSize,
		PxcArticulationSolverDesc *PX_RESTRICT articulationListStart, const PxU32 articulationListSize,
		PxcSolverConstraintDesc* PX_RESTRICT constraintList, const PxU32 constraintListSize,
		PxcSolverConstraintDesc* PX_RESTRICT frictionConstraintList, const PxU32 frictionConstraintConstraintListSize,
		Cm::SpatialVector* PX_RESTRICT motionVelocityArray) 
		const;

	//~Implements PxsSolverCore
};

}

#endif
