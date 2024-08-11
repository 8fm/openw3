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


#ifndef PXD_SHADER_H
#define PXD_SHADER_H

#include "PxvConfig.h"
#include "PxvDynamics.h"

#include "PxVec3.h"
#include "PxTransform.h"
#include "PxConstraint.h"

namespace physx
{

class PxsRigidBody;


PX_ALIGN_PREFIX(16)
struct PxsConstraintWriteback
{
	PxVec3	linearImpulse;
	PxU32	broken;
	PxVec3  angularImpulse;
	PxU32	pad;
}
PX_ALIGN_SUFFIX(16);


PX_ALIGN_PREFIX(16)
struct PxsConstraint
{
public:
	static const PxU32 MAX_CONSTRAINTS = 12;

	PxReal								linBreakForce;														//0
	PxReal								angBreakForce;														//4
	PxU32								solverPrepSpuByteSize;												//8
	PxU32								constantBlockSize;													//12

	PxConstraintSolverPrep				solverPrep;															//16
	PxConstraintProject					project;															//20
	void*								solverPrepSpu;														//24
	void*								constantBlock;														//28

	PxsConstraintWriteback*				writeback;															//32
	PxsRigidBody*						body0;																//36
	PxsRigidBody*						body1;																//40

	// lifted here from the bodies to eliminate an extra level of pointer chasing on SPU

	PxsBodyCore*						bodyCore0;															//44
	PxsBodyCore*						bodyCore1;															//48
	PxU32								flags;																//52
}
PX_ALIGN_SUFFIX(16);

#ifndef PX_X64
PX_COMPILE_TIME_ASSERT(64==sizeof(PxsConstraint));
#endif

}

#endif
