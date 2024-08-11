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


#include "common.cuh"

#include "include/common.h"
using namespace physx::apex;
using namespace physx::apex::iofx;
#include "include/actorRanges.h"
#include "NiIofxManager.h"


BOUND_KERNEL_BEG( ACTOR_RANGE_WARPS_PER_BLOCK, actorRangeKernel,
	const physx::PxU32* sortedActorID, physx::PxU32 numActorIDs,
	physx::PxU32* actorStart, physx::PxU32* actorEnd, physx::PxU32* actorVisibleEnd,
	const physx::PxU32* sortedStateID
)
	__shared__ physx::PxU32 sdata[BlockSize + 1];
	__shared__ physx::PxU32 sdataVisible[BlockSize + 1];

	const physx::PxU32 idx = threadIdx.x;

	const physx::PxU32 outputCount = _threadCount;
	for (unsigned int outputBeg = BlockSize * blockIdx.x; outputBeg < outputCount; outputBeg += BlockSize * gridDim.x)
	{
		const unsigned int outputEnd = min(outputBeg + BlockSize, outputCount);
		const unsigned int output = outputBeg + idx;

		sdata[idx] = (output < outputEnd) ? (sortedActorID[output] >> NiIofxActorID::ASSETS_PER_MATERIAL_BITS) : UINT_MAX;
		sdataVisible[idx] = (output < outputEnd) ? (sortedStateID[output] >> 31) : UINT_MAX;
		if (idx == 0) {
			sdata[BlockSize] = (outputEnd < outputCount) ? (sortedActorID[outputEnd] >> NiIofxActorID::ASSETS_PER_MATERIAL_BITS) : UINT_MAX;
			sdataVisible[BlockSize] = (outputEnd < outputCount) ? (sortedStateID[outputEnd] >> 31) : UINT_MAX;
		}
		__syncthreads();

		if (output < outputEnd)
		{
			const physx::PxU32 currActorIndex = sdata[idx];
			const physx::PxU32 nextActorIndex = sdata[idx + 1];
			if (nextActorIndex != currActorIndex)
			{
				if (nextActorIndex != UINT_MAX)
				{
					actorStart[nextActorIndex] = output + 1;
					if (sdataVisible[idx + 1] != 0)
					{
						actorVisibleEnd[nextActorIndex] = output + 1;
					}
				}
				if (currActorIndex != UINT_MAX)
				{
					actorEnd[currActorIndex] = output + 1;
					if (sdataVisible[idx] == 0)
					{
						actorVisibleEnd[currActorIndex] = output + 1;
					}
				}
			}
			else if (sdataVisible[idx] != sdataVisible[idx + 1])
			{
				if (currActorIndex != UINT_MAX)
				{
					actorVisibleEnd[currActorIndex] = output + 1;
				}
			}
		}
	}
BOUND_KERNEL_END()
