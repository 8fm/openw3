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

#define FIX_FOR_KEPLER 1

#include "blocksync.cuh"

#include "include/common.h"
#include "include/scan.h"

inline __device__ void reduceWarp(unsigned int idx, volatile unsigned int* sdata)
{
	sdata[idx] += sdata[idx + 16];
	sdata[idx] += sdata[idx +  8];
	sdata[idx] += sdata[idx +  4];
	sdata[idx] += sdata[idx +  2];
	sdata[idx] += sdata[idx +  1];
}

inline __device__ void bitScanWarp(unsigned int idx, unsigned idxInWarp, volatile unsigned int* sdata)
{
	sdata[idx] <<= idxInWarp;
	reduceWarp(idx, sdata);

	unsigned int result = sdata[ idx & ~(WARP_SIZE-1) ];

	sdata[idx] = __popc( result & (0xFFFFFFFFU >> (31-idxInWarp)) );
}

inline __device__ int evalBin(float benefit, float benefitMin, float benefitMax)
{
	return (benefit > -FLT_MAX) ? ((benefit - benefitMin) * HISTOGRAM_BIN_COUNT / (benefitMax - benefitMin)) : -1;
}

inline __device__ unsigned int condition(int bin, int boundBin)
{
	return (bin < boundBin) ? 1 : 0;
}
inline __device__ unsigned int condition1(int bin, int boundBin)
{
	return (bin == boundBin) ? 1 : 0;
}


#define SCAN_LOOP_COMMON(whole) \
	sdata[idx] = marker; \
	bitScanWarp(idx, idxInWarp, sdata); \
	if (whole || pos < warpEnd) g_indices[pos] = (prefix + sdata[idx]) | (marker << HOLE_SCAN_FLAG_BIT); \
	if (whole) prefix += sdata[(warpIdx << LOG2_WARP_SIZE) + WARP_SIZE-1];

#define SCAN_LOOP_1(whole) \
	unsigned int marker = 0; \
	if (whole || pos < warpEnd) { \
		float benefit = g_benefits[pos]; \
		int bin = evalBin(benefit, benefitMin, benefitMax); \
		marker = condition(bin, markBin); \
	} \
	SCAN_LOOP_COMMON(whole)

#define SCAN_LOOP_2(whole) \
	unsigned int marker = 0; \
	if (whole || pos < warpEnd) { \
		float benefit = g_benefits[pos]; \
		int bin = evalBin(benefit, benefitMin, benefitMax); \
		marker = condition(bin, boundBin); \
		marker |= condition1(bin, boundBin) << 1; \
	} \
	if (prefix1 < boundCount) \
	{ \
		sdata1[idx] = (marker >> 1); \
		bitScanWarp(idx, idxInWarp, sdata1); \
		marker |= (marker >> 1) & ((prefix1 + sdata1[idx] <= boundCount) ? 1 : 0); \
		if (whole) prefix1 += sdata1[(warpIdx << LOG2_WARP_SIZE) + WARP_SIZE-1]; \
	} \
	marker &= 1; \
	SCAN_LOOP_COMMON(whole)


template <int WarpsPerBlock, int BlockSize>
inline __device__ void scan1(unsigned int count,
	float benefitMin, float benefitMax, unsigned int* g_indices, const float* g_benefits,
	unsigned int* g_boundParams, unsigned int* g_tmpCounts, unsigned int* g_tmpCounts1,
	volatile unsigned int* sdata, volatile unsigned int* sdata1,
	const unsigned int warpBeg, const unsigned int warpEnd)
{
	const unsigned int idx = threadIdx.x;
	const unsigned int idxInWarp = idx & (WARP_SIZE-1);

	__shared__ int boundBin;
	if (idx == 0)
	{
		boundBin = g_boundParams[1];
	}
	__syncthreads();

	sdata[idx] = 0;
	sdata1[idx] = 0;

	if (warpBeg < warpEnd)
	{
		//accum
		for (unsigned int i = warpBeg + idxInWarp; i < warpEnd; i += WARP_SIZE)
		{
			float benefit = g_benefits[i];

			int bin = evalBin(benefit, benefitMin, benefitMax);
			sdata[idx] += condition(bin, boundBin);
			sdata1[idx] += condition1(bin, boundBin);
		}
		//reduce warp
		reduceWarp(idx, sdata);
		reduceWarp(idx, sdata1);
	}

	__syncthreads();

	if (idx < WarpsPerBlock)
	{
		g_tmpCounts[blockIdx.x * WarpsPerBlock + idx] = sdata[idx << LOG2_WARP_SIZE];
		g_tmpCounts1[blockIdx.x * WarpsPerBlock + idx] = sdata1[idx << LOG2_WARP_SIZE];
		__threadfence();
	}
}


inline __device__ void scanWarp(unsigned int scanIdx, volatile unsigned int* sdata)
{
	sdata[scanIdx] += sdata[scanIdx -  1];
	sdata[scanIdx] += sdata[scanIdx -  2];
	sdata[scanIdx] += sdata[scanIdx -  4];
	sdata[scanIdx] += sdata[scanIdx -  8];
	sdata[scanIdx] += sdata[scanIdx - 16]; 
}


template <int WarpsPerBlock, int BlockSize>
inline __device__ void scan2(
	float benefitMin, float benefitMax, unsigned int* g_indices, const float* g_benefits,
	unsigned int* g_boundParams, unsigned int* g_tmpCounts, unsigned int* g_tmpCounts1,
	volatile unsigned int* sdata, volatile unsigned int* sdata1,
	unsigned int gridSize)
{
	const unsigned int idx = threadIdx.x;
	const unsigned int WarpsPerGrid = WarpsPerBlock * gridSize;

#if FIX_FOR_KEPLER
	const unsigned int idxInWarp = idx & (WARP_SIZE-1);
	const unsigned int warpIdx = (idx >> LOG2_WARP_SIZE);

	unsigned int val = (idx < WarpsPerGrid) ? g_tmpCounts[idx] : 0;
	unsigned int val1 = (idx < WarpsPerGrid) ? g_tmpCounts1[idx] : 0;

	//setup scan
	int scanIdx = (warpIdx << (LOG2_WARP_SIZE + 1)) + idxInWarp;
	sdata[scanIdx] = 0;
	sdata1[scanIdx] = 0;
	scanIdx += WARP_SIZE;
	sdata[scanIdx] = val;
	sdata1[scanIdx] = val1;

	scanWarp(scanIdx, sdata);
	scanWarp(scanIdx, sdata1);

	//__syncthreads();

	val = sdata[scanIdx];
	val1 = sdata1[scanIdx];

	__syncthreads();

	if (idxInWarp == WARP_SIZE-1)
	{
		sdata[warpIdx + WARP_SIZE] = val;
		sdata1[warpIdx + WARP_SIZE] = val1;
	}
	__syncthreads();

	if (idx < WARP_SIZE)
	{
		scanWarp(scanIdx, sdata);
		scanWarp(scanIdx, sdata1);
	}
	__syncthreads();

	val += sdata[warpIdx + WARP_SIZE - 1];
	val1 += sdata1[warpIdx + WARP_SIZE - 1];

	if (idx < WarpsPerGrid) {
		g_tmpCounts[idx] = val;
		g_tmpCounts1[idx] = val1;
		__threadfence();
	}
#else
	//do prefix sum (TODO: optimize for warps)
	sdata[idx] = (idx < WarpsPerGrid) ? g_tmpCounts[idx] : 0;
	sdata1[idx] = (idx < WarpsPerGrid) ? g_tmpCounts1[idx] : 0;

	int pout = 0;
	int pin = 1;

	#pragma unroll
	for (int offset = 1; offset < BlockSize; offset *= 2)
	{
		pout = 1 - pout;
		pin  = 1 - pout;

		sdata[pout*BlockSize + idx] = sdata[pin*BlockSize + idx];
		if (idx >= offset) sdata[pout*BlockSize + idx] += sdata[pin*BlockSize + idx - offset];

		sdata1[pout*BlockSize + idx] = sdata1[pin*BlockSize + idx];
		if (idx >= offset) sdata1[pout*BlockSize + idx] += sdata1[pin*BlockSize + idx - offset];

		__syncthreads();
	}

	if (idx < WarpsPerGrid) {
		g_tmpCounts[idx] = sdata[pout*BlockSize + idx];
		g_tmpCounts1[idx] = sdata1[pout*BlockSize + idx];
		__threadfence();
	}
#endif
}

template <int WarpsPerBlock, int BlockSize>
inline __device__ void scan3(unsigned int count,
	float benefitMin, float benefitMax, unsigned int* g_indices, const float* g_benefits,
	unsigned int* g_boundParams, unsigned int* g_tmpCounts, unsigned int* g_tmpCounts1,
	volatile unsigned int* sdata, volatile unsigned int* sdata1,
	const unsigned int warpBeg, const unsigned int warpEnd)
{
	const unsigned int idx = threadIdx.x;
	const unsigned int idxInWarp = idx & (WARP_SIZE-1);
	const unsigned int warpIdx = (idx >> LOG2_WARP_SIZE);

	__shared__ unsigned int sCounts[WarpsPerBlock+1];
	__shared__ unsigned int sCounts1[WarpsPerBlock+1];

	__shared__ unsigned int boundCount;
	__shared__ int          boundBin;
	if (idx == 0)
	{
		boundCount = g_boundParams[0];
		boundBin   = g_boundParams[1];

		sCounts[0]  = (blockIdx.x > 0) ? g_tmpCounts[blockIdx.x * WarpsPerBlock - 1] : 0;
		sCounts1[0] = (blockIdx.x > 0) ? g_tmpCounts1[blockIdx.x * WarpsPerBlock - 1] : 0;
	}
	if (idx < WarpsPerBlock)
	{
		sCounts[idx+1]  = g_tmpCounts[blockIdx.x * WarpsPerBlock + idx];
		sCounts1[idx+1] = g_tmpCounts1[blockIdx.x * WarpsPerBlock + idx];
	}
	__syncthreads();

	if (warpBeg < warpEnd)
	{
		unsigned int prefix = sCounts[warpIdx];
		unsigned int prefix1 = sCounts1[warpIdx];

		if (prefix1 >= boundCount || boundCount >= sCounts1[warpIdx+1])
		{
			prefix += min(prefix1, boundCount);
			int markBin = (prefix1 >= boundCount) ? boundBin : (boundBin + 1);

			unsigned int pos;
			for (pos = warpBeg + idxInWarp; pos < (warpEnd & ~(WARP_SIZE-1)); pos += WARP_SIZE)
			{
				SCAN_LOOP_1(true)
			}
			if ((warpEnd & (WARP_SIZE-1)) > 0)
			{
				SCAN_LOOP_1(false)
			}
		}
		else
		{
			prefix += prefix1;

			unsigned int pos;
			for (pos = warpBeg + idxInWarp; pos < (warpEnd & ~(WARP_SIZE-1)); pos += WARP_SIZE)
			{
				SCAN_LOOP_2(true)
			}
			if ((warpEnd & (WARP_SIZE-1)) > 0)
			{
				SCAN_LOOP_2(false)
			}
		}
	}
}

#define SCAN_KERNEL_SETUP(count) \
	const unsigned int DataWarpsPerGrid = ((count + WARP_SIZE-1) >> LOG2_WARP_SIZE); \
	const unsigned int DataWarpsPerBlock = (DataWarpsPerGrid + gridDim.x-1) / gridDim.x; \
	const unsigned int DataCountPerBlock = (DataWarpsPerBlock << LOG2_WARP_SIZE); \
	const unsigned int WarpBorder = DataWarpsPerBlock % WarpsPerBlock; \
	const unsigned int WarpFactor = DataWarpsPerBlock / WarpsPerBlock; \
	const unsigned int warpIdx = (threadIdx.x >> LOG2_WARP_SIZE); \
	const unsigned int blockBeg = blockIdx.x * DataCountPerBlock; \
	const unsigned int blockEnd = min(blockBeg + DataCountPerBlock, count); \
	const unsigned int WarpSelect = (warpIdx < WarpBorder) ? 1 : 0; \
	const unsigned int WarpCount = WarpFactor + WarpSelect; \
	const unsigned int WarpOffset = warpIdx * WarpCount + WarpBorder * (1 - WarpSelect); \
	const unsigned int warpBeg = blockBeg + (WarpOffset << LOG2_WARP_SIZE); \
	const unsigned int warpEnd = min(warpBeg + (WarpCount << LOG2_WARP_SIZE), blockEnd); \
	__shared__ volatile unsigned int sdata[BlockSize * 2]; \
	__shared__ volatile unsigned int sdata1[BlockSize * 2];

SYNC_KERNEL_BEG(SCAN_WARPS_PER_BLOCK, scanKernel, unsigned int count,
	float benefitMin, float benefitMax, unsigned int* g_indices, const float* g_benefits, unsigned int* g_boundParams, unsigned int* g_tmpCounts, unsigned int* g_tmpCounts1
)
	SCAN_KERNEL_SETUP(count)

	scan1<WarpsPerBlock, BlockSize>(count,
		benefitMin, benefitMax, g_indices, g_benefits,
		g_boundParams, g_tmpCounts, g_tmpCounts1,
		sdata, sdata1,
		warpBeg, warpEnd);

	BLOCK_SYNC_BEGIN()

	scan2<WarpsPerBlock, BlockSize>(
		benefitMin, benefitMax, g_indices, g_benefits,
		g_boundParams, g_tmpCounts, g_tmpCounts1,
		sdata, sdata1,
		gridDim.x);
		
	BLOCK_SYNC_END()

	scan3<WarpsPerBlock, BlockSize>(count,
		benefitMin, benefitMax, g_indices, g_benefits,
		g_boundParams, g_tmpCounts, g_tmpCounts1,
		sdata, sdata1,
		warpBeg, warpEnd);

SYNC_KERNEL_END()

BOUND_KERNEL_BEG(SCAN_WARPS_PER_BLOCK, scan1Kernel,
	float benefitMin, float benefitMax, unsigned int* g_indices, const float* g_benefits, unsigned int* g_boundParams, unsigned int* g_tmpCounts, unsigned int* g_tmpCounts1
)
	SCAN_KERNEL_SETUP(_threadCount)

	scan1<WarpsPerBlock, BlockSize>(_threadCount,
		benefitMin, benefitMax, g_indices, g_benefits,
		g_boundParams, g_tmpCounts, g_tmpCounts1,
		sdata, sdata1,
		warpBeg, warpEnd);

BOUND_KERNEL_END()

BOUND_KERNEL_BEG(SCAN_WARPS_PER_BLOCK, scan2Kernel,
	float benefitMin, float benefitMax, unsigned int* g_indices, const float* g_benefits, unsigned int* g_boundParams, unsigned int* g_tmpCounts, unsigned int* g_tmpCounts1,
	unsigned int gridSize
)
	__shared__ volatile unsigned int sdata[BlockSize * 2];
	__shared__ volatile unsigned int sdata1[BlockSize * 2];

	scan2<WarpsPerBlock, BlockSize>(
		benefitMin, benefitMax, g_indices, g_benefits,
		g_boundParams, g_tmpCounts, g_tmpCounts1,
		sdata, sdata1,
		gridSize);

BOUND_KERNEL_END()

BOUND_KERNEL_BEG(SCAN_WARPS_PER_BLOCK, scan3Kernel,
	float benefitMin, float benefitMax, unsigned int* g_indices, const float* g_benefits, unsigned int* g_boundParams, unsigned int* g_tmpCounts, unsigned int* g_tmpCounts1
)
	SCAN_KERNEL_SETUP(_threadCount)

	scan3<WarpsPerBlock, BlockSize>(_threadCount,
		benefitMin, benefitMax, g_indices, g_benefits,
		g_boundParams, g_tmpCounts, g_tmpCounts1,
		sdata, sdata1,
		warpBeg, warpEnd);

BOUND_KERNEL_END()
