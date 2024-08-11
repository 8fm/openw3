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

#ifndef RAND_STATE_HELPERS_H
#define RAND_STATE_HELPERS_H

#include "PxTask.h"
#include "ApexMirroredArray.h"

namespace physx
{
namespace apex
{

// For CUDA PRNG: host part
PX_INLINE void InitDevicePRNGs(
    NiApexScene& scene,
    unsigned int blockSize,
    LCG_PRNG& threadLeap,
    LCG_PRNG& gridLeap,
    ApexMirroredArray<LCG_PRNG>& blockPRNGs)
{
	threadLeap = LCG_PRNG::getDefault().leapFrog(16);

	LCG_PRNG randBlock = LCG_PRNG::getIdentity();
	LCG_PRNG randBlockLeap = threadLeap.leapFrog(blockSize);

	const PxU32 numBlocks = 32; //Max Multiprocessor count
	blockPRNGs.setSize(numBlocks, ApexMirroredPlace::CPU_GPU);
	for (PxU32 i = 0; i < numBlocks; ++i)
	{
		blockPRNGs[i] = randBlock;
		randBlock *= randBlockLeap;
	}
	gridLeap = randBlock;

	{
		physx::PxTaskManager* tm = scene.getTaskManager();
		physx::PxCudaContextManager* ctx = tm->getGpuDispatcher()->getCudaContextManager();

		physx::PxScopedCudaLock s(*ctx);

		PxGpuCopyDesc desc;
		blockPRNGs.copyHostToDeviceDesc(desc, 0, 0);
		tm->getGpuDispatcher()->launchCopyKernel(&desc, 1, 0);
	}
}

}
} // physx::apex::

#endif
