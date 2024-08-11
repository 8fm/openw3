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

#include "blocksync.cuh"
#include "reduce.cuh"

#include "include/common.h"
#include "include/reduce.h"


template <int WarpsPerBlock, int BlockSize>
inline __device__ void reduce1(unsigned int count, float* g_benefit, float4* g_output,
	unsigned int			*g_tmpActiveCount,
	float					*g_tmpBenefitSum,
	float					*g_tmpBenefitMin,
	float					*g_tmpBenefitMax,
	volatile unsigned int	*sdataActiveCount,
	volatile float			*sdataBenefitSum,
	volatile float			*sdataBenefitMin,
	volatile float			*sdataBenefitMax)
{
	unsigned int idx = threadIdx.x;

	sdataActiveCount[idx] = AddOPu::identity();
	sdataBenefitSum[idx]  = AddOPf::identity();
	sdataBenefitMin[idx]  = MinOPf::identity();
	sdataBenefitMax[idx]  = MaxOPf::identity();

	for (unsigned int pos = BlockSize*blockIdx.x + idx; pos < count; pos += BlockSize*gridDim.x)
	{
		float benefit = g_benefit[pos];
		if (benefit != -FLT_MAX)
		{
			sdataActiveCount[idx] = AddOPu::apply(sdataActiveCount[idx], 1);
			sdataBenefitSum[idx]  = AddOPf::apply(sdataBenefitSum[idx], benefit);
			sdataBenefitMin[idx]  = MinOPf::apply(sdataBenefitMin[idx], benefit);
			sdataBenefitMax[idx]  = MaxOPf::apply(sdataBenefitMax[idx], benefit);
		}
	}

	//don't need to synch because we use whole WARPs here
	reduceWarp<unsigned int, AddOPu>(sdataActiveCount);
	reduceWarp<float, AddOPf>(sdataBenefitSum);
	reduceWarp<float, MinOPf>(sdataBenefitMin);
	reduceWarp<float, MaxOPf>(sdataBenefitMax);

	//merge all warps for block
	__syncthreads();

	reduceBlock<unsigned int, AddOPu, WarpsPerBlock>(sdataActiveCount, g_tmpActiveCount);
	reduceBlock<float, AddOPf, WarpsPerBlock>(sdataBenefitSum,  g_tmpBenefitSum);
	reduceBlock<float, MinOPf, WarpsPerBlock>(sdataBenefitMin,  g_tmpBenefitMin);
	reduceBlock<float, MaxOPf, WarpsPerBlock>(sdataBenefitMax,  g_tmpBenefitMax);

	if (idx == 0) {
		__threadfence(); //only one write per block
	}
}

template <int WarpsPerBlock, int BlockSize>
inline __device__ void reduce2(float* g_benefit, float4* g_output,
	unsigned int			*g_tmpActiveCount,
	float					*g_tmpBenefitSum,
	float					*g_tmpBenefitMin,
	float					*g_tmpBenefitMax,
	volatile unsigned int	*sdataActiveCount,
	volatile float			*sdataBenefitSum,
	volatile float			*sdataBenefitMin,
	volatile float			*sdataBenefitMax,
	unsigned int gridSize)
{
	reduceGrid<unsigned int, AddOPu>(sdataActiveCount, g_tmpActiveCount, gridSize);
	reduceGrid<float, AddOPf>(sdataBenefitSum,  g_tmpBenefitSum, gridSize);
	reduceGrid<float, MinOPf>(sdataBenefitMin,  g_tmpBenefitMin, gridSize);
	reduceGrid<float, MaxOPf>(sdataBenefitMax,  g_tmpBenefitMax, gridSize);

	if (threadIdx.x == 0)
	{
		g_output[0] = make_float4(__int_as_float( sdataActiveCount[0] ),
			sdataBenefitSum[0],
			sdataBenefitMin[0],
			sdataBenefitMax[0]
		);
	}
}

#define REDUCE_KERNEL_SETUP() \
	unsigned int* g_tmpActiveCount = g_tmp; \
	float* g_tmpBenefitSum = (float*)(g_tmp + WARP_SIZE); \
	float* g_tmpBenefitMin = (float*)(g_tmp + WARP_SIZE*2); \
	float* g_tmpBenefitMax = (float*)(g_tmp + WARP_SIZE*3); \
	__shared__ volatile unsigned int	sdataActiveCount[BlockSize]; \
	__shared__ volatile float			sdataBenefitSum[BlockSize]; \
	__shared__ volatile float			sdataBenefitMin[BlockSize]; \
	__shared__ volatile float			sdataBenefitMax[BlockSize];

SYNC_KERNEL_BEG(REDUCE_WARPS_PER_BLOCK, reduceKernel,
	unsigned int count, float* g_benefit,
	float4* g_output, unsigned int* g_tmp
)
	REDUCE_KERNEL_SETUP()

	reduce1<WarpsPerBlock, BlockSize>(count, g_benefit, g_output,
		g_tmpActiveCount, g_tmpBenefitSum, g_tmpBenefitMin, g_tmpBenefitMax,
		sdataActiveCount, sdataBenefitSum, sdataBenefitMin, sdataBenefitMax);

	BLOCK_SYNC_BEGIN()

	reduce2<WarpsPerBlock, BlockSize>(g_benefit, g_output,
		g_tmpActiveCount, g_tmpBenefitSum, g_tmpBenefitMin, g_tmpBenefitMax,
		sdataActiveCount, sdataBenefitSum, sdataBenefitMin, sdataBenefitMax,
		gridDim.x);

	BLOCK_SYNC_END()

SYNC_KERNEL_END()

BOUND_KERNEL_BEG(REDUCE_WARPS_PER_BLOCK, reduce1Kernel,
	float* g_benefit, float4* g_output, unsigned int* g_tmp
)
	REDUCE_KERNEL_SETUP()

	reduce1<WarpsPerBlock, BlockSize>(_threadCount, g_benefit, g_output,
		g_tmpActiveCount, g_tmpBenefitSum, g_tmpBenefitMin, g_tmpBenefitMax,
		sdataActiveCount, sdataBenefitSum, sdataBenefitMin, sdataBenefitMax);

BOUND_KERNEL_END()

BOUND_KERNEL_BEG(REDUCE_WARPS_PER_BLOCK, reduce2Kernel,
	float* g_benefit, float4* g_output, unsigned int* g_tmp,
	unsigned int gridSize
)
	REDUCE_KERNEL_SETUP()

	reduce2<WarpsPerBlock, BlockSize>(g_benefit, g_output,
		g_tmpActiveCount, g_tmpBenefitSum, g_tmpBenefitMin, g_tmpBenefitMax,
		sdataActiveCount, sdataBenefitSum, sdataBenefitMin, sdataBenefitMax,
		gridSize);

BOUND_KERNEL_END()


