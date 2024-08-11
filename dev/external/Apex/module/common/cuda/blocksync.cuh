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

#ifndef __BLOCK_SYNC_CUH__
#define __BLOCK_SYNC_CUH__

#include "common.cuh"

__device__ volatile unsigned int blockSyncFlags[MULTIPROCESSOR_MAX_COUNT];

#define BLOCK_SYNC_BEGIN() \
	__shared__ volatile unsigned int s_blockSyncFlags[WARP_SIZE]; \
	if (blockIdx.x == 0) \
	{ \
		if (threadIdx.x < WARP_SIZE) { \
			do	{ \
				s_blockSyncFlags[threadIdx.x] = (threadIdx.x < MULTIPROCESSOR_MAX_COUNT) ? \
					blockSyncFlags[threadIdx.x] : 0; \
				if (threadIdx.x < 16) { \
					s_blockSyncFlags[threadIdx.x] += s_blockSyncFlags[threadIdx.x + 16]; \
					s_blockSyncFlags[threadIdx.x] += s_blockSyncFlags[threadIdx.x +  8]; \
					s_blockSyncFlags[threadIdx.x] += s_blockSyncFlags[threadIdx.x +  4]; \
					s_blockSyncFlags[threadIdx.x] += s_blockSyncFlags[threadIdx.x +  2]; \
					s_blockSyncFlags[threadIdx.x] += s_blockSyncFlags[threadIdx.x +  1]; \
				} \
			} \
			while (s_blockSyncFlags[0] + 1 < gridDim.x); \
		} \
		__syncthreads();

#define BLOCK_SYNC_END() \
		__syncthreads(); \
		if (threadIdx.x < MULTIPROCESSOR_MAX_COUNT) { \
			blockSyncFlags[threadIdx.x] = 0; \
			__threadfence(); \
		} \
		__syncthreads(); \
	} \
	else \
	{ \
		__syncthreads(); \
		if (threadIdx.x == blockIdx.x) { \
			blockSyncFlags[threadIdx.x] = 1; \
			__threadfence(); \
			while (blockSyncFlags[threadIdx.x] != 0) ; \
		} \
		__syncthreads(); \
	}

#endif //__BLOCK_SYNC_CUH__
