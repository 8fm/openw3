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
#include "scan.cuh"

#include "include/common.h"
using namespace physx::apex;
using namespace physx::apex::iofx;
#include "include/sort.h"


typedef unsigned int uint;

//--------------------------------------------------------------------------------------------------

// ================================================================================================
// Flip a float for sorting
//  finds SIGN of fp number.
//  if it's 1 (negative float), it flips all bits
//  if it's 0 (positive float), it flips the sign only
// ================================================================================================
template <bool doFlip>
inline __device__ uint floatFlip(uint f)
{
	if (doFlip)
	{
		uint mask = -int(f >> 31) | 0x80000000;
		return f ^ mask;
	}
	else
		return f;
}

// ================================================================================================
// flip a float back (invert FloatFlip)
//  signed was flipped from above, so:
//  if sign is 1 (negative), it flips the sign bit back
//  if sign is 0 (positive), it flips all bits back
// ================================================================================================
template <bool doFlip>
inline __device__ uint floatUnflip(uint f)
{
	if (doFlip)
	{
		uint mask = ((f >> 31) - 1) | 0x80000000;
		return f ^ mask;
	}
	else
		return f;
}

//--------------------------------------------------------------------------------------------------

template<class T, int maxlevel>
inline __device__ T scanwarp(T val, volatile T* sData)
{
	int idx = 2 * threadIdx.x - (threadIdx.x & (WARP_SIZE - 1));
	sData[idx] = 0;
	idx += WARP_SIZE;
	sData[idx] = val;

	if (0 <= maxlevel) { sData[idx] += sData[idx - 1]; }
	if (1 <= maxlevel) { sData[idx] += sData[idx - 2]; }
	if (2 <= maxlevel) { sData[idx] += sData[idx - 4]; }
	if (3 <= maxlevel) { sData[idx] += sData[idx - 8]; }
	if (4 <= maxlevel) { sData[idx] += sData[idx -16]; }

	return sData[idx] - val;  // convert inclusive -> exclusive
}

inline __device__ uint4 scan4(uint4 idata, volatile uint* sData)
{
	uint idx = threadIdx.x;

	uint4 val4 = idata;
	uint sum[3];
	sum[0] = val4.x;
	sum[1] = val4.y + sum[0];
	sum[2] = val4.z + sum[1];

	uint val = val4.w + sum[2];

	val = scanwarp<uint, 4>(val, sData);
	__syncthreads();

	if ((idx & (WARP_SIZE - 1)) == WARP_SIZE - 1)
	{
		sData[idx >> LOG2_WARP_SIZE] = val + val4.w + sum[2];
	}
	__syncthreads();

	if (idx < WARP_SIZE)
	{
		sData[idx] = scanwarp<uint, 4>(sData[idx], sData);
	}
	__syncthreads();

	val += sData[idx >> LOG2_WARP_SIZE];

	val4.x = val;
	val4.y = val + sum[0];
	val4.z = val + sum[1];
	val4.w = val + sum[2];

	return val4;
}

//--------------------------------------------------------------------------------------------------


template <uint BlockSize>
inline __device__ uint rank(volatile uint* sdata, uint pred)
{
	uint idx = threadIdx.x;

	uint scanCount = scanBlock<uint, AddOP<uint> >(sdata, pred);

	__shared__ uint totalCount;
	if (idx == BlockSize - 1)
	{
		totalCount = scanCount + pred;
	}
	__syncthreads();

	return pred ? scanCount : (totalCount + idx - scanCount);
}


template <uint BlockSize, uint nbits>
inline __device__ void radixSortBlock(volatile uint* sKeys, volatile uint* sValues, uint &key, uint &value, uint startbit)
{
	uint idx = threadIdx.x;

	#pragma unroll
	for(uint shift = startbit; shift < (startbit + nbits); ++shift)
	{
		uint lsb = ((key >> shift) & 0x1) ^ 0x01;

		uint r = rank<BlockSize>(sKeys, lsb);

		sKeys[r] = key;
		sValues[r] = value;

		__syncthreads();

		key = sKeys[idx];
		value = sValues[idx];

		__syncthreads();
	}
}


//--------------------------------------------------------------------------------------------------
#define FIND_RADIX_OFFSETS(sdata) \
		sdata[idx] = (pos < blockEnd) ? ((key >> startbit) & 0xF) : UINT_MAX; \
		if (idx == 0) sdata[BlockSize] = UINT_MAX; \
		if (idx < 16) sRadixStart[idx] = sRadixEnd[idx] = 0; \
		__syncthreads(); \
		if (sdata[idx] != sdata[idx + 1]) { \
			sRadixEnd[sdata[idx]] = idx + 1; \
			if (sdata[idx + 1] < 16) sRadixStart[sdata[idx + 1]] = idx + 1; \
		} \
		__syncthreads(); \

//--------------------------------------------------------------------------------------------------

template <uint BlockSize, uint nbits, bool doFlip>
inline __device__ void radixSortStep1(uint *keys, uint *values, uint *tempKeys, uint *tempValues, uint count, uint startbit, uint* g_temp,
									  volatile uint* sKeys, volatile uint* sValues, const uint blockBeg, const uint blockEnd)
{
	__shared__ uint sRadixStart[16];
	__shared__ uint sRadixEnd[16];
	__shared__ uint sCounters[16];

	const unsigned int idx = threadIdx.x;
	if (idx < 16) sCounters[idx] = 0;

	//sort blocks
	for (uint blockPos = blockBeg; blockPos < blockEnd; blockPos += BlockSize)
	{
		uint pos = blockPos + idx;

		uint key = UINT_MAX;
		uint value = UINT_MAX;
		if (pos < blockEnd)
		{
			key = floatFlip<doFlip>(keys[pos]);
			value = values[pos];
		}

		__syncthreads();
		radixSortBlock<BlockSize, nbits> (sKeys, sValues, key, value, startbit);

		if (pos < blockEnd) 
		{
			tempKeys[pos] = key;
			tempValues[pos] = value;
		}

		FIND_RADIX_OFFSETS(sKeys)

		if (idx < 16) sCounters[idx] += (sRadixEnd[idx] - sRadixStart[idx]);
	}

	__syncthreads();
	if (idx < 16)
	{
		g_temp[gridDim.x*idx + blockIdx.x] = sCounters[idx];
	}
}

inline __device__ void radixSortStep2(uint* g_temp, volatile uint* sdata, uint gridSize)
{
	const uint ScanCount = gridSize * (16 >> 2);

	const unsigned int idx = threadIdx.x;
	uint4 val = (idx < ScanCount) ? ((uint4*)g_temp)[idx] : make_uint4(0, 0, 0, 0);

	val = scan4(val, sdata);

	if (idx < ScanCount) {
		((uint4*)g_temp)[idx] = val;
		__threadfence();
	}
}

template <uint BlockSize, uint nbits, bool doFlip>
inline __device__ void radixSortStep3(uint *keys, uint *values, uint *tempKeys, uint *tempValues, uint count, uint startbit, uint* g_temp,
									  volatile uint* sKeys, volatile uint* sValues, const uint blockBeg, const uint blockEnd)
{
	__shared__ uint sRadixStart[16];
	__shared__ uint sRadixEnd[16];
	__shared__ uint sCounters[16];

	const unsigned int idx = threadIdx.x;
	if (idx < 16)
	{
		sCounters[idx] = g_temp[gridDim.x*idx + blockIdx.x];
	}

	//reorder data in blocks
	for (uint blockPos = blockBeg; blockPos < blockEnd; blockPos += BlockSize)
	{
		uint pos = blockPos + idx;

		uint key = (pos < blockEnd) ? tempKeys[pos] : UINT_MAX;

		FIND_RADIX_OFFSETS(sKeys)

		sKeys[idx] = key;
		sValues[idx] = (pos < blockEnd) ? tempValues[pos] : UINT_MAX;
		__syncthreads();

#if !defined(__CUDA_ARCH__) || (__CUDA_ARCH__ >= 120)
		if (pos < blockEnd)
		{
			uint radix = (sKeys[idx] >> startbit) & 0xF;
			uint globalOffset = sCounters[radix] + idx - sRadixStart[radix];

			keys[globalOffset]   = floatUnflip<doFlip>(sKeys[idx]);
			values[globalOffset] = sValues[idx];
		}
#else
		//manual coalescing
		const uint halfWarpID = idx >> 4;
		if (halfWarpID < 16)
		{
			const uint halfWarpOffset = idx & 0xF;

			const uint startPos = sCounters[halfWarpID];
			const uint endPos   = startPos + (sRadixEnd[halfWarpID] - sRadixStart[halfWarpID]);

			const uint leadingInvalid = startPos & 0xF;

			uint outOffset = (startPos & ~0xF) + halfWarpOffset;
			if (halfWarpOffset >= leadingInvalid && outOffset < endPos)
			{
				uint inOffset0 = sRadixStart[halfWarpID] + (halfWarpOffset - leadingInvalid);
				keys[outOffset]   = floatUnflip<doFlip>(sKeys[inOffset0]);
				values[outOffset] = sValues[inOffset0];
			}

			outOffset += 16;
			uint inOffset  = sRadixStart[halfWarpID] + (16 - leadingInvalid) + halfWarpOffset;

			for (; outOffset < endPos; outOffset += 16, inOffset += 16)
			{
				keys[outOffset]   = floatUnflip<doFlip>(sKeys[inOffset]);
				values[outOffset] = sValues[inOffset];
			}
		}
#endif
		__syncthreads();

		if (idx < 16) sCounters[idx] += (sRadixEnd[idx] - sRadixStart[idx]);
	}
}

#define SORT_STEP_KERNEL_SETUP(count) \
	const unsigned int DataWarpsPerGrid = ((count + WARP_SIZE-1) >> LOG2_WARP_SIZE); \
	const unsigned int DataWarpsPerBlock = (DataWarpsPerGrid + gridDim.x-1) / gridDim.x; \
	const unsigned int DataCountPerBlock = (DataWarpsPerBlock << LOG2_WARP_SIZE); \
	const unsigned int blockBeg = blockIdx.x * DataCountPerBlock; \
	const unsigned int blockEnd = min(blockBeg + DataCountPerBlock, count); \
	__shared__ volatile uint sdata[BlockSize * 2]; \
	volatile uint* sKeys = sdata; \
	volatile uint* sValues = sdata + BlockSize;

template <uint BlockSize, uint nbits, bool doFlip>
__device__ void radixSortStep(uint *keys, uint *values, uint *tempKeys, uint *tempValues, uint count, uint startbit, uint* g_temp)
{
	SORT_STEP_KERNEL_SETUP(count)

	radixSortStep1<BlockSize, nbits, doFlip>(
		keys, values, tempKeys, tempValues, count, startbit, g_temp,
		sKeys, sValues, blockBeg, blockEnd);
	__threadfence();

	BLOCK_SYNC_BEGIN()

	radixSortStep2(g_temp, sdata, gridDim.x);

	BLOCK_SYNC_END()

	radixSortStep3<BlockSize, nbits, doFlip>(
		keys, values, tempKeys, tempValues, count, startbit, g_temp,
		sKeys, sValues, blockBeg, blockEnd);

	{
		__threadfence();
		BLOCK_SYNC_BEGIN()
		BLOCK_SYNC_END()
	}
}

//--------------------------------------------------------------------------------------------------

SYNC_KERNEL_BEG(SORT_WARPS_PER_BLOCK, radixSortKernel, uint numElements,
	uint *keys, uint *values, uint *tempKeys, uint *tempValues, uint* g_temp, uint keyBits, uint startbit0
)
	for (uint startbit = startbit0; startbit < startbit0 + keyBits; startbit += RADIX_SORT_NBITS) {
		radixSortStep<BlockSize, RADIX_SORT_NBITS, false> (keys, values, tempKeys, tempValues, numElements, startbit, g_temp);
	}
SYNC_KERNEL_END()

BOUND_KERNEL_BEG(SORT_WARPS_PER_BLOCK, radixSortStep1Kernel,
	uint *keys, uint *values, uint *tempKeys, uint *tempValues, uint* g_temp, uint startBit
)
	SORT_STEP_KERNEL_SETUP(_threadCount)

	radixSortStep1<BlockSize, RADIX_SORT_NBITS, false>(
		keys, values, tempKeys, tempValues, _threadCount, startBit, g_temp,
		sKeys, sValues, blockBeg, blockEnd);

BOUND_KERNEL_END()

BOUND_KERNEL_BEG(SORT_WARPS_PER_BLOCK, radixSortStep2Kernel,
	uint* g_temp, uint gridSize
)
	__shared__ volatile uint sdata[BlockSize * 2];

	radixSortStep2(g_temp, sdata, gridSize);

BOUND_KERNEL_END()

BOUND_KERNEL_BEG(SORT_WARPS_PER_BLOCK, radixSortStep3Kernel,
	uint *keys, uint *values, uint *tempKeys, uint *tempValues, uint* g_temp, uint startBit
)
	SORT_STEP_KERNEL_SETUP(_threadCount)

	radixSortStep3<BlockSize, RADIX_SORT_NBITS, false>(
		keys, values, tempKeys, tempValues, _threadCount, startBit, g_temp,
		sKeys, sValues, blockBeg, blockEnd);

BOUND_KERNEL_END()
