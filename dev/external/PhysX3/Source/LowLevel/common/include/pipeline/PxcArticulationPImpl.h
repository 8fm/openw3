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



#ifndef PXC_ARTICULATION_INTERFACE_H
#define PXC_ARTICULATION_INTERFACE_H

#include "PxcArticulation.h"
#include "PxcArticulationHelper.h"

namespace physx
{

struct PxcArticulationSolverDesc;
struct PxcSolverConstraintDesc;
class PxcConstraintBlockStream;
class PxcScratchAllocator;

class PxcArticulationPImpl
{
public:

	typedef PxU32 (*ComputeUnconstrainedVelocitiesFn)(const PxcArticulationSolverDesc& desc,
													 PxReal dt,
													 PxcConstraintBlockStream& stream,
													 PxcSolverConstraintDesc* constraintDesc,
													 PxU32& acCount,
													 Cm::EventProfiler& profiler,
													 PxsConstraintBlockManager& constraintBlockManager);

	typedef void (*UpdateBodiesFn)(const PxcArticulationSolverDesc& desc,
								   PxReal dt);

	typedef void (*SaveVelocityFn)(const PxcArticulationSolverDesc &m);

	static ComputeUnconstrainedVelocitiesFn sComputeUnconstrainedVelocities;
	static UpdateBodiesFn sUpdateBodies;
	static SaveVelocityFn sSaveVelocity;

	static PxU32 computeUnconstrainedVelocities(const PxcArticulationSolverDesc& desc,
										   PxReal dt,
										   PxcConstraintBlockStream& stream,
										   PxcSolverConstraintDesc* constraintDesc,
										   PxU32& acCount,
										   Cm::EventProfiler& profiler,
										   PxcScratchAllocator&,
										   PxsConstraintBlockManager& constraintBlockManager)
	{
		PX_ASSERT(sComputeUnconstrainedVelocities);
		if(sComputeUnconstrainedVelocities)
			return (sComputeUnconstrainedVelocities)(desc, dt, stream, constraintDesc, acCount, profiler, constraintBlockManager);
		else
			return 0;
	}

	static void	updateBodies(const PxcArticulationSolverDesc& desc,
						 PxReal dt)
	{
		PX_ASSERT(sUpdateBodies);
		if(sUpdateBodies)
			(*sUpdateBodies)(desc, dt);
	}

	static void	saveVelocity(const PxcArticulationSolverDesc& desc)
	{
		PX_ASSERT(sSaveVelocity);
		if(sSaveVelocity)
			(*sSaveVelocity)(desc);
	}
};


}
#endif