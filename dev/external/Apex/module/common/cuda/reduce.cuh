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

#ifndef __REDUCE_CUH__
#define __REDUCE_CUH__

#include "common.cuh"
#include <float.h>

template <typename T>
struct AddOP
{
	static __device__ T apply(T a, T b) { return a + b; }
	static __device__ T identity() { return T(); }
};

typedef AddOP<unsigned int> AddOPu;
typedef AddOP<float> AddOPf;

struct MinOPf
{
	static __device__ float apply(float a, float b) { return fmin(a, b); }
	static __device__ float identity() { return +FLT_MAX; }
};

struct MaxOPf
{
	static __device__ float apply(float a, float b) { return fmax(a, b); }
	static __device__ float identity() { return -FLT_MAX; }
};

struct MaxOPu
{
	static __device__ float apply(float a, float b) { return umax(a, b); }
	static __device__ float identity() { return 0; }
};

template <typename T, class OP>
__device__ void reduceWarp(volatile T* sdata)
{
	unsigned int idx = threadIdx.x;
	if ((idx & (WARP_SIZE-1)) < 16)
	{
		sdata[idx] = OP::apply(sdata[idx], sdata[idx + 16]); 
		sdata[idx] = OP::apply(sdata[idx], sdata[idx +  8]);
		sdata[idx] = OP::apply(sdata[idx], sdata[idx +  4]);
		sdata[idx] = OP::apply(sdata[idx], sdata[idx +  2]);
		sdata[idx] = OP::apply(sdata[idx], sdata[idx +  1]);
	}
}

template <typename T, class OP, unsigned int WarpsPerBlock>
__device__ void reduceBlock(volatile T* sdata, T* g_odata)
{
	unsigned int idx = threadIdx.x;
	if (idx < WARP_SIZE) 
	{
		sdata[idx] = (idx < WarpsPerBlock) ? sdata[idx << LOG2_WARP_SIZE] : OP::identity();
		reduceWarp<T, OP>(sdata);
	}
	if (idx == 0) {
		g_odata[blockIdx.x] = sdata[0];
	}
}

template <typename T, class OP, unsigned int WarpsPerBlock>
__device__ void reduceSingleBlock(volatile T* sdata)
{
	unsigned int idx = threadIdx.x;
	if (idx < WARP_SIZE) 
	{
		sdata[idx] = (idx < WarpsPerBlock) ? sdata[idx << LOG2_WARP_SIZE] : OP::identity();
		reduceWarp<T, OP>(sdata);
	}
}

template <typename T, class OP>
__device__ void reduceGrid(volatile T* sdata, T* g_odata, unsigned int gridSize = gridDim.x)
{
	unsigned int idx = threadIdx.x;
	if (idx < WARP_SIZE)
	{
		sdata[idx] = (idx < gridSize) ? g_odata[idx] : OP::identity();
		reduceWarp<T, OP>(sdata);
	}
}

#endif __REDUCE_CUH__
