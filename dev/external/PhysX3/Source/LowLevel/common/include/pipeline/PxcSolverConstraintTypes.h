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


#ifndef PXC_SOLVERCONSTRAINTTYPES_H
#define PXC_SOLVERCONSTRAINTTYPES_H

#include "PxvConfig.h"
#include "PxSimpleTypes.h"

namespace physx
{

enum SolverConstraintType
{
	PXS_SC_TYPE_NONE = 0,
	PXS_SC_TYPE_RB_CONTACT,		// RB-only contact
	PXS_SC_TYPE_RB_1D,			// RB-only 1D constraint
	PXS_SC_TYPE_EXT_CONTACT,	// contact involving articulations
	PXS_SC_TYPE_EXT_1D,			// 1D constraint involving articulations
	PXS_SC_TYPE_STATIC_CONTACT,	// RB-only contact where body b is static
	PXS_SC_TYPE_NOFRICTION_RB_CONTACT, //RB-only contact with no friction patch
	PXS_SC_TYPE_BLOCK_RB_CONTACT,
	PXS_SC_TYPE_BLOCK_STATIC_RB_CONTACT,
	PXS_SC_TYPE_BLOCK_1D,
	PXS_SC_TYPE_FRICTION,
	PXS_SC_TYPE_STATIC_FRICTION,
	PXS_SC_TYPE_EXT_FRICTION,
	PXS_SC_TYPE_BLOCK_FRICTION,
	PXS_SC_TYPE_BLOCK_STATIC_FRICTION,
	PXS_SC_CONSTRAINT_TYPE_COUNT //Count of the number of different constraint types in the solver
};

enum SolverDominanceFlags
{
	PXS_SC_FLAG_0_IS_PUSHABLE		= (1<<0),
	PXS_SC_FLAG_1_IS_PUSHABLE		= (1<<1)
};

enum SolverConstraintFlags
{
	PXS_SC_FLAG_KEEP_BIAS			= (1<<0),
	PXS_SC_FLAG_OUTPUT_FORCE		= (1<<1)
};

/*
PX_FORCE_INLINE float dominance0(PxU8 flags)
{
	return (flags&PXS_SC_FLAG_0_IS_PUSHABLE) ? 1.0f : 0.0f;
}

PX_FORCE_INLINE float dominance1(PxU8 flags) 
{
	return (flags&PXS_SC_FLAG_1_IS_PUSHABLE) ? 1.0f : 0.0f;
}
*/
}

#endif
