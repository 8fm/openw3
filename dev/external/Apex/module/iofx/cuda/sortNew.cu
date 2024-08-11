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
#include "include/sortNew.h"

//assuming that the number of warps per block is less or equal to 32
const unsigned int MAX_WARPS_PER_BLOCK = 32;

#ifndef __CUDA_ARCH__
#define __CUDA_ARCH__ 0
#endif

//--------------------------------------------------------------------------------------------------

typedef unsigned int uint;

const uint KEY_BITS_PER_STEP = RADIX_SORT_NBITS;
const uint KEY_DIGITS_PER_STEP = (1U << KEY_BITS_PER_STEP);

#define DEBUG_RADIX_SORT 0
#define KEPLER_BUG_FIX 1

//--------------------------------------------------------------------------------------------------

template <uint VectorSize>
struct VectorTT
{
};

template <>
struct VectorTT<1>
{
	static const uint Log2Size = 0;
	static const uint Size = 1;
	typedef uint AccessType;
};
template <>
struct VectorTT<2>
{
	static const uint Log2Size = 1;
	static const uint Size = 2;
	typedef uint2 AccessType;
};
template <>
struct VectorTT<4>
{
	static const uint Log2Size = 2;
	static const uint Size = 4;
	typedef uint4 AccessType;
};

//--------------------------------------------------------------------------------------------------

template <bool doFlip>
struct KeyTraits
{
};

template <>
struct KeyTraits<false>
{
	static const uint OutOfRangeValue = 0xFFFFFFFFu;

	static inline __device__ uint flip(uint f) { return f; }
	static inline __device__ uint unflip(uint f) { return f; }
};

template <>
struct KeyTraits<true>
{
	static const uint OutOfRangeValue = 0x7FFFFFFFu;

	// ================================================================================================
	// Flip a float for sorting
	//  finds SIGN of fp number.
	//  if it's 1 (negative float), it flips all bits
	//  if it's 0 (positive float), it flips the sign only
	// ================================================================================================
	static inline __device__ uint flip(uint f)
	{
		const uint mask = -int(f >> 31) | 0x80000000;
		return f ^ mask;
	}
	// ================================================================================================
	// flip a float back (invert FloatFlip)
	//  signed was flipped from above, so:
	//  if sign is 1 (negative), it flips the sign bit back
	//  if sign is 0 (positive), it flips all bits back
	// ================================================================================================
	static inline __device__ uint unflip(uint f)
	{
		const uint mask = ((f >> 31) - 1) | 0x80000000;
		return f ^ mask;
	}
};

//--------------------------------------------------------------------------------------------------

#if __CUDA_ARCH__ >= 300

static __device__ __inline__ uint reduceWarp(uint val, volatile uint* sdata)
{
	uint res = val;

	asm volatile (
"{"
"	.reg .u32 tmp;"
"	shfl.bfly.b32 tmp, %0, 0x10, 0x1f;"
"	add.u32 %0, tmp, %0;"
"	shfl.bfly.b32 tmp, %0, 0x08, 0x1f;"
"	add.u32 %0, tmp, %0;"
"	shfl.bfly.b32 tmp, %0, 0x04, 0x1f;"
"	add.u32 %0, tmp, %0;"
"	shfl.bfly.b32 tmp, %0, 0x02, 0x1f;"
"	add.u32 %0, tmp, %0;"
"	shfl.bfly.b32 tmp, %0, 0x01, 0x1f;"
"	add.u32 %0, tmp, %0;"
"}"
	: "+r"(res) : );

	return res;
}

static __device__ __inline__ uint scanWarp(uint val, volatile uint* sdata)
{
	uint ret = val;

	asm volatile (
"{"
"	.reg .u32 tmp;"
"	.reg .pred p;"
"	shfl.up.b32 tmp|p, %0, 0x1, 0x0;"
"@p add.u32 %0, tmp, %0;"
"	shfl.up.b32 tmp|p, %0, 0x2, 0x0;"
"@p add.u32 %0, tmp, %0;"
"	shfl.up.b32 tmp|p, %0, 0x4, 0x0;"
"@p add.u32 %0, tmp, %0;"
"	shfl.up.b32 tmp|p, %0, 0x8, 0x0;"
"@p add.u32 %0, tmp, %0;"
"	shfl.up.b32 tmp|p, %0, 0x10, 0x0;"
"@p add.u32 %0, tmp, %0;"
"}"
	: "+r"(ret) : );

	return ret;
}

static __device__ __inline__ uint scanWarpWithTotal(uint val, uint& total, volatile uint* sdata)
{
	uint ret = val;
	uint out;

	asm volatile (
"{	.reg .u32 tmp;"
"	.reg .pred p;"
"	shfl.up.b32 tmp|p, %0, 0x1, 0x0;"
"@p add.u32 %0, tmp, %0;"
"	shfl.up.b32 tmp|p, %0, 0x2, 0x0;"
"@p add.u32 %0, tmp, %0;"
"	shfl.up.b32 tmp|p, %0, 0x4, 0x0;"
"@p add.u32 %0, tmp, %0;"
"	shfl.up.b32 tmp|p, %0, 0x8, 0x0;"
"@p add.u32 %0, tmp, %0;"
"	shfl.up.b32 tmp|p, %0, 0x10, 0x0;"
"@p add.u32 %0, tmp, %0;"
"	shfl.idx.b32 %1, %0, 0x1f, 0x1f;"
"}"
	: "+r"(ret), "=r"(out) : );

	total = out;
	return ret;
}

static __device__ __inline__ uint SHFL(uint val, uint idx)
{
	uint res;

	asm volatile (
"	shfl.idx.b32 %0, %1, %2, 0x1f;"
	: "=r"(res) : "r"(val), "r"(idx) );

	return res;
}

#else

static __device__ __inline__ uint reduceWarp(uint val, volatile uint* sdata)
{
	unsigned int idx = threadIdx.x;
	sdata[idx] = val;
	if ((idx & (WARP_SIZE-1)) < 16)
	{
		sdata[idx] = sdata[idx] + sdata[idx + 16]; 
		sdata[idx] = sdata[idx] + sdata[idx +  8];
		sdata[idx] = sdata[idx] + sdata[idx +  4];
		sdata[idx] = sdata[idx] + sdata[idx +  2];
		sdata[idx] = sdata[idx] + sdata[idx +  1];
	}
	unsigned int res = sdata[idx & ~(WARP_SIZE-1)];
	return res;
}

static __device__ __inline__ uint scanWarp(uint val, volatile uint* sdata)
{
	const unsigned int idx = threadIdx.x;
	const unsigned int idxInWarp = idx & (WARP_SIZE-1);
	const unsigned int warpIdx = (idx >> LOG2_WARP_SIZE);

	//setup scan
	uint scanIdx = (warpIdx << (LOG2_WARP_SIZE + 1)) + idxInWarp;
	sdata[scanIdx] = 0;
	scanIdx += WARP_SIZE;
	sdata[scanIdx] = val;

	sdata[scanIdx] += sdata[scanIdx -  1]; 
	sdata[scanIdx] += sdata[scanIdx -  2]; 
	sdata[scanIdx] += sdata[scanIdx -  4]; 
	sdata[scanIdx] += sdata[scanIdx -  8]; 
	sdata[scanIdx] += sdata[scanIdx - 16];

	return sdata[scanIdx];
}

static __device__ __inline__ uint scanWarpWithTotal(uint val, uint& total, volatile uint* sdata)
{
	const unsigned int idx = threadIdx.x;
	const unsigned int idxInWarp = idx & (WARP_SIZE-1);
	const unsigned int warpIdx = (idx >> LOG2_WARP_SIZE);

	//setup scan
	uint scanIdx = (warpIdx << (LOG2_WARP_SIZE + 1)) + idxInWarp;
	sdata[scanIdx] = 0;
	scanIdx += WARP_SIZE;
	sdata[scanIdx] = val;

	sdata[scanIdx] += sdata[scanIdx -  1]; 
	sdata[scanIdx] += sdata[scanIdx -  2]; 
	sdata[scanIdx] += sdata[scanIdx -  4]; 
	sdata[scanIdx] += sdata[scanIdx -  8]; 
	sdata[scanIdx] += sdata[scanIdx - 16];

	total = sdata[scanIdx | (WARP_SIZE-1)];
	return sdata[scanIdx];
}

#endif

//--------------------------------------------------------------------------------------------------

inline __device__ void scanBlockInSMem(uint scanCount, uint* scanArray, volatile uint* sdata)
{
	const unsigned int idx = threadIdx.x;
	const unsigned int warpIdx = (idx >> LOG2_WARP_SIZE);
	const unsigned int idxInWarp = idx & (WARP_SIZE-1);

	const uint scanWarps = (scanCount + WARP_SIZE-1) >> LOG2_WARP_SIZE;

	uint scanRes;

	__shared__ volatile uint sScanForWarp[MAX_WARPS_PER_BLOCK];
	if (warpIdx < scanWarps)
	{
		uint scanVal = (idx < scanCount) ? scanArray[idx] : 0;
		scanRes = scanWarp(scanVal, sdata);

		if (idxInWarp == WARP_SIZE-1)
		{
			sScanForWarp[warpIdx] = scanRes;
		}
		scanRes -= scanVal; //make scan exclusive
	}
	__syncthreads();

	//1 warp scan
	if (idx < WARP_SIZE)
	{
		uint warpScanVal = (idx < scanWarps) ? sScanForWarp[idx] : 0;
		uint warpScanRes = scanWarp(warpScanVal, sdata);

		if (idxInWarp == WARP_SIZE-1)
		{
			//store total scan at the end of scanArray
			scanArray[scanCount] = warpScanRes;
		}
		warpScanRes -= warpScanVal; //make scan exclusive
		sScanForWarp[idx] = warpScanRes; 
	}
	__syncthreads();

	if (idx < scanCount)
	{
		scanArray[idx] = scanRes + sScanForWarp[warpIdx];
	}
}

inline __device__ uint4 scan4(uint scanCount, uint4 idata, volatile uint* sData, uint* pOutTotal = 0)
{    
	const unsigned int idx = threadIdx.x;
	const unsigned int warpIdx = (idx >> LOG2_WARP_SIZE);
	const unsigned int idxInWarp = idx & (WARP_SIZE-1);

	const uint scanWarps = (scanCount + WARP_SIZE-1) >> LOG2_WARP_SIZE;

	__shared__ volatile uint sScanForWarp[MAX_WARPS_PER_BLOCK];

	uint4 val4 = idata;
	uint res;
	uint sum[3];
	if (warpIdx < scanWarps)
	{
		sum[0] = val4.x;
		sum[1] = val4.y + sum[0];
		sum[2] = val4.z + sum[1];

		uint val = val4.w + sum[2];
		res = scanWarp(val, sData);

		if (idxInWarp == WARP_SIZE-1)
		{
			sScanForWarp[warpIdx] = res;
		}
		res -= val; //make scan exclusive
	}
	__syncthreads();

	//1 warp scan
	if (idx < WARP_SIZE)
	{
		uint warpScanVal = (idx < scanWarps) ? sScanForWarp[idx] : 0;
		uint warpScanRes = scanWarp(warpScanVal, sData);

		if (idxInWarp == WARP_SIZE-1)
		{
			if (pOutTotal != 0)
			{
				*pOutTotal = warpScanRes;
			}
		}
		warpScanRes -= warpScanVal; //make scan exclusive
		sScanForWarp[idx] = warpScanRes; 
	}
	__syncthreads();

	if (warpIdx < scanWarps)
	{
		res += sScanForWarp[warpIdx];
		val4.x = res;
		val4.y = res + sum[0];
		val4.z = res + sum[1];
		val4.w = res + sum[2];
	}
	return val4;
}

//--------------------------------------------------------------------------------------------------

template <uint WarpsPerBlock, uint VectorSize, uint WarpStride, bool FullBlock>
inline __device__ void localSortStep(uint count, uint startBit, uint key[VectorSize], uint value[VectorSize], 
							  uint* sKeys, uint* sValues, uint* sWarpCounters, volatile uint* sdata,
							  uint remainWarps)
{
	const unsigned int ElemsPerThread = VectorTT<VectorSize>::Size;
	typedef typename VectorTT<VectorSize>::AccessType ElemAccessType;

	const unsigned int idx = threadIdx.x;
	const unsigned int warpIdx = (idx >> LOG2_WARP_SIZE);
	const unsigned int idxInWarp = idx & (WARP_SIZE-1);

	if (!(FullBlock || warpIdx < remainWarps))
	{
		//fill gaps for unused warps in sWarpCounters with 0 for correct scan later
		if (idxInWarp < KEY_DIGITS_PER_STEP)
		{
			sWarpCounters[idxInWarp * WarpStride + warpIdx] = 0;
		}
	}
	//fill gaps out of warps in sWarpCounters with 0 for correct scan later
	if (WarpStride > WarpsPerBlock && idx < KEY_DIGITS_PER_STEP)
	{
		sWarpCounters[idx * WarpStride + WarpsPerBlock] = 0;
	}

	uint keyDigit[ElemsPerThread];
	uint keyOffset[ElemsPerThread];

	if (FullBlock || warpIdx < remainWarps)
	{
		#pragma unroll
		for (int i = 0; i < ElemsPerThread; ++i)
		{
			keyDigit[i] = ((key[i] >> startBit) & (KEY_DIGITS_PER_STEP - 1));
			keyOffset[i] = 0;
		}

		#pragma unroll
		for (uint bit = 0; bit < KEY_DIGITS_PER_STEP; bit += 4)
		{
			//seq. reduce
			uint scanVal = 0;
			#pragma unroll
			for (int i = 0; i < ElemsPerThread; ++i)
			{
				scanVal += (1u << ((keyDigit[i] - bit) << 3)); //in PTX shifts are clamped to 32, so it's ok here
			}

			uint scanTotal;
			uint scanRes = scanWarpWithTotal(scanVal, scanTotal, sdata);

			if (idxInWarp < 4)
			{
				sWarpCounters[(bit + idxInWarp)*WarpStride + warpIdx] = ((scanTotal >> (idxInWarp << 3)) & 0xFF);
			}
			scanRes -= scanVal; //makes scan exclusive

			#pragma unroll
			for (int i = 0; i < ElemsPerThread; ++i)
			{
				keyOffset[i] |= ((scanRes >> ((keyDigit[i] - bit) << 3)) & 0xFF); //in PTX shifts are clamped to 32, so it's ok here
			}
		}
	}
	__syncthreads();

	const uint scanCount = KEY_DIGITS_PER_STEP * WarpStride;
	scanBlockInSMem(scanCount, sWarpCounters, sdata);
	__syncthreads();

	if (FullBlock || warpIdx < remainWarps)
	{
#if (__CUDA_ARCH__ >= 300)
		uint keyOffsetForWarp;
		if (idxInWarp < KEY_DIGITS_PER_STEP) keyOffsetForWarp = sWarpCounters[idxInWarp*WarpStride + warpIdx];
#endif
		//seq. exclusive scan
		const uint AccumCount = (KEY_DIGITS_PER_STEP / 16);
		uint accum[AccumCount];
		#pragma unroll
		for (int k = 0; k < AccumCount; ++k) accum[k] = 0;

		#pragma unroll
		for (int i = 0; i < ElemsPerThread; ++i)
		{
			uint digit = keyDigit[i];

			uint keyLocalOffset = keyOffset[i];
			#pragma unroll
			for (int k = 0; k < AccumCount; ++k) keyLocalOffset += ((accum[k] >> ((digit - k*16) << 1)) & 3);

#if (__CUDA_ARCH__ >= 300)
			keyLocalOffset += SHFL(keyOffsetForWarp, digit);
#else
			keyLocalOffset += sWarpCounters[digit*WarpStride + warpIdx];
#endif

			sKeys[keyLocalOffset] = key[i];
#if !DEBUG_RADIX_SORT
			sValues[keyLocalOffset] = value[i];
#else
			sValues[idx*ElemsPerThread + i] = keyLocalOffset;
#endif

			#pragma unroll
			for (int k = 0; k < AccumCount; ++k) accum[k] += (1u << ((digit - k*16) << 1));
		}
	}
}

template <uint VectorSize, bool doFlip, bool FullBlock>
inline __device__ void readKeyAndValue(uint count, uint* inpKeys, uint* inpValues, uint key[VectorSize], uint value[VectorSize], uint remainWarps, uint blockPos)
{
	const unsigned int Log2ElemsPerThread = VectorTT<VectorSize>::Log2Size;
	const unsigned int ElemsPerThread = VectorTT<VectorSize>::Size;
	typedef typename VectorTT<VectorSize>::AccessType ElemAccessType;

	const unsigned int idx = threadIdx.x;
	const unsigned int warpIdx = (idx >> LOG2_WARP_SIZE);

	if (FullBlock || warpIdx < remainWarps)
	{
		uint inpPos = blockPos + idx;
		if (FullBlock || inpPos < (count >> Log2ElemsPerThread))
		{
			*((ElemAccessType*)key) = ((ElemAccessType*)inpKeys)[inpPos];
			*((ElemAccessType*)value) = ((ElemAccessType*)inpValues)[inpPos];
		}
		else
		{
			if (VectorSize == 1)
			{
				key[0] = KeyTraits<doFlip>::OutOfRangeValue;
			}
			else
			{
				inpPos <<= Log2ElemsPerThread;
				#pragma unroll
				for (int i = 0; i < ElemsPerThread; ++i, ++inpPos)
				{
					key[i] = KeyTraits<doFlip>::OutOfRangeValue;
					if (inpPos < count)
					{
						key[i] = inpKeys[inpPos];
						value[i] = inpValues[inpPos];
					}
				}
			}
		}

		if (doFlip)
		{
			#pragma unroll
			for (int i = 0; i < ElemsPerThread; ++i)
			{
				key[i] = KeyTraits<true>::flip(key[i]);
			}
		}
	}
}

template <uint WarpsPerBlock, uint VectorSize, bool doFlip, uint WarpStride, bool FullBlock>
inline __device__ void localSortStepBlock(uint count, uint startBit, uint* inpKeys, uint* inpValues, uint* outKeys, uint* outValues,
							  uint* sKeys, uint* sValues, uint* sWarpCounters, volatile uint* sdata, uint* sCounters,
							  uint remainDataCount, uint blockPos)
{
	const unsigned int Log2ElemsPerThread = VectorTT<VectorSize>::Log2Size;
	const unsigned int ElemsPerThread = VectorTT<VectorSize>::Size;
	typedef typename VectorTT<VectorSize>::AccessType ElemAccessType;

	const unsigned int idx = threadIdx.x;

	const unsigned int Log2DataWarpSize = (LOG2_WARP_SIZE + Log2ElemsPerThread);
	const unsigned int DataWarpSize = (1 << Log2DataWarpSize);

	uint remainWarps = (remainDataCount + (DataWarpSize-1)) >> Log2DataWarpSize;

	uint key[ElemsPerThread];
	uint value[ElemsPerThread];

	readKeyAndValue<VectorSize, doFlip, FullBlock>(count, inpKeys, inpValues, key, value, remainWarps, blockPos);

	localSortStep<WarpsPerBlock, VectorSize, WarpStride, FullBlock>(count, startBit, key, value, sKeys, sValues, sWarpCounters, sdata, remainWarps);

	__shared__ int sGlobalOffsets[KEY_DIGITS_PER_STEP];
	if (idx < KEY_DIGITS_PER_STEP)
	{
		uint radixStart = sWarpCounters[idx*WarpStride];
		uint radixEnd = sWarpCounters[(idx+1)*WarpStride];

		sGlobalOffsets[idx] = sCounters[idx] - radixStart;
		sCounters[idx] += (radixEnd - radixStart);
	}
	__syncthreads();

//#if __CUDA_ARCH__ >= 200
#if 1

#if (__CUDA_ARCH__ >= 300) && !KEPLER_BUG_FIX
	int globalOffsetForWarp = 0;
	const unsigned int idxInWarp = idx & (WARP_SIZE-1);
	if (idxInWarp < KEY_DIGITS_PER_STEP) globalOffsetForWarp = sGlobalOffsets[idxInWarp];
#endif
	const unsigned int BlockSize = (WarpsPerBlock << LOG2_WARP_SIZE);
	for (int outIdx = idx; outIdx < remainDataCount; outIdx += BlockSize)
	{
		uint _key = sKeys[outIdx];
		uint _value = sValues[outIdx];
		uint digit = ((_key >> startBit) & (KEY_DIGITS_PER_STEP - 1));
#if (__CUDA_ARCH__ >= 300) && !KEPLER_BUG_FIX
		int globalOffset = SHFL(globalOffsetForWarp, digit);
#else
		int globalOffset = sGlobalOffsets[digit];
#endif

		uint outPos = globalOffset + outIdx;
#if DEBUG_RADIX_SORT
		if (outPos >= 0 && outPos < count)
#endif
		outKeys[outPos] = KeyTraits<doFlip>::unflip(_key);
#if !DEBUG_RADIX_SORT
		outValues[outPos] = _value;
#else
		outValues[blockPos*ElemsPerThread + outIdx] = globalOffset;//_value;
#endif
	}

#if 0
	//manual coalescing full-warp
	const unsigned int warpIdx = (idx >> LOG2_WARP_SIZE);
	for (uint digit = warpIdx; digit < KEY_DIGITS_PER_STEP; digit += WarpsPerBlock)
	{
		const uint warpOffset = idx & (WARP_SIZE-1);

		const uint radixStart = sWarpCounters[digit * WarpStride];
		const uint radixEnd = sWarpCounters[(digit + 1) * WarpStride];

		const uint endPos = sCounters[digit];
		const uint startPos = endPos - (radixEnd - radixStart);

		const uint leadingInvalid = startPos & (WARP_SIZE-1);

		uint outOffset = (startPos & ~(WARP_SIZE-1)) + warpOffset;
		if (warpOffset >= leadingInvalid && outOffset < endPos)
		{
			uint inOffset0 = radixStart + (warpOffset - leadingInvalid);
			outKeys[outOffset]   = KeyTraits<doFlip>::unflip(sKeys[inOffset0]);
			outValues[outOffset] = sValues[inOffset0];
		}

		outOffset += WARP_SIZE;
		uint inOffset  = radixStart + (WARP_SIZE - leadingInvalid) + warpOffset;

		for (; outOffset < endPos; outOffset += WARP_SIZE, inOffset += WARP_SIZE)
		{
			outKeys[outOffset]   = KeyTraits<doFlip>::unflip(sKeys[inOffset]);
			outValues[outOffset] = sValues[inOffset];
		}
	}
#endif

#else
	//manual coalescing half-warp for Tesla
	const uint halfWarpIdx = (idx >> 4);
	for (uint digit = halfWarpIdx; digit < KEY_DIGITS_PER_STEP; digit += WarpsPerBlock)
	{
		const uint halfWarpOffset = (idx & 0xF);

		const uint radixStart = sWarpCounters[digit * WarpStride];
		const uint radixEnd = sWarpCounters[(digit + 1) * WarpStride];

		const uint endPos = sCounters[digit];
		const uint startPos = endPos - (radixEnd - radixStart);

		const uint leadingInvalid = (startPos & 0xF);

		uint outOffset = (startPos & ~0xF) + halfWarpOffset;
		if (halfWarpOffset >= leadingInvalid && outOffset < endPos)
		{
			uint inOffset0 = radixStart + (halfWarpOffset - leadingInvalid);
			outKeys[outOffset]   = KeyTraits<doFlip>::unflip(sKeys[inOffset0]);
			outValues[outOffset] = sValues[inOffset0];
		}

		outOffset += 16;
		uint inOffset  = radixStart + (16 - leadingInvalid) + halfWarpOffset;

		for (; outOffset < endPos; outOffset += 16, inOffset += 16)
		{
			outKeys[outOffset]   = KeyTraits<doFlip>::unflip(sKeys[inOffset]);
			outValues[outOffset] = sValues[inOffset];
		}
	}
#endif
}

template <uint WarpStride>
inline __device__ void processReduce(uint accum[KEY_DIGITS_PER_STEP >> 2], volatile uint* sData, volatile uint* sWarpCounters)
{
	const unsigned int idx = threadIdx.x;
	const unsigned int warpIdx = (idx >> LOG2_WARP_SIZE);
	const unsigned int idxInWarp = idx & (WARP_SIZE-1);

	#pragma unroll
	for (uint keyDigit = 0; keyDigit < KEY_DIGITS_PER_STEP; ++keyDigit)
	{
		uint val = (accum[keyDigit >> 2] >> ((keyDigit & 3) << 3)) & 0xFF;

		uint res = reduceWarp(val, sData);

		if (idxInWarp == 0)
		{
			sWarpCounters[keyDigit*WarpStride + warpIdx] += res;
		}
	}
}

template <uint WarpsPerBlock, uint VectorSize, bool doFlip>
inline __device__ void radixSortStep1(uint count, uint startBit, uint* inpKeys, uint* inpValues, uint* outKeys, uint* outValues, uint* tempScan)
{
	const unsigned int Log2ElemsPerThread = VectorTT<VectorSize>::Log2Size;
	const unsigned int ElemsPerThread = VectorTT<VectorSize>::Size;
	typedef typename VectorTT<VectorSize>::AccessType ElemAccesType;

	const unsigned int Log2DataWarpSize = (LOG2_WARP_SIZE + Log2ElemsPerThread);
	const unsigned int DataWarpSize = (1 << Log2DataWarpSize);

	const unsigned int GridDataWarpsCount = ((count + DataWarpSize-1) >> Log2DataWarpSize);
	const unsigned int DataWarpsResidue = (GridDataWarpsCount % gridDim.x);
	const unsigned int DataWarpsExtra = (blockIdx.x < DataWarpsResidue) ? 1 : 0;
	const unsigned int DataWarpsCount = (GridDataWarpsCount / gridDim.x) + DataWarpsExtra;
	const unsigned int DataWarpsOffset = blockIdx.x * DataWarpsCount + DataWarpsResidue * (1 - DataWarpsExtra);

	const unsigned int BlockSize = (WarpsPerBlock << LOG2_WARP_SIZE);

	const unsigned int idx = threadIdx.x;
	const unsigned int warpIdx = (idx >> LOG2_WARP_SIZE);
	const unsigned int idxInWarp = idx & (WARP_SIZE-1);


	//temp shared memory for scan
	__shared__ volatile uint sdata[BlockSize*2];

	const uint WarpStride = WarpsPerBlock + 1;
	__shared__ volatile uint sWarpCounters[KEY_DIGITS_PER_STEP * WarpStride];

	if (idxInWarp < KEY_DIGITS_PER_STEP)
	{
		sWarpCounters[idxInWarp*WarpStride + warpIdx] = 0;
	}

	uint accum[KEY_DIGITS_PER_STEP >> 2];
	uint accumCount = 0;
	#pragma unroll
	for (uint i = 0; i < (KEY_DIGITS_PER_STEP >> 2); ++i)
	{
		accum[i] = 0;
	}

	uint blockPos = (DataWarpsOffset << LOG2_WARP_SIZE);
	for (int remainWarps = DataWarpsCount; remainWarps > 0; remainWarps -= WarpsPerBlock, blockPos += BlockSize)
	{
		uint key[ElemsPerThread];

		if (warpIdx < remainWarps)
		{
			uint inpPos = blockPos + idx;
			if (inpPos < (count >> Log2ElemsPerThread))
			{
				*((ElemAccesType*)key) = ((ElemAccesType*)inpKeys)[inpPos];
			}
			else
			{
				inpPos <<= Log2ElemsPerThread;
				#pragma unroll
				for (int i = 0; i < ElemsPerThread; ++i, ++inpPos)
				{
					key[i] = KeyTraits<doFlip>::OutOfRangeValue;
					if (inpPos < count)
					{
						key[i] = inpKeys[inpPos];
					}
				}
			}

			#pragma unroll
			for (int i = 0; i < ElemsPerThread; ++i)
			{
				uint keyDigit = ((KeyTraits<doFlip>::flip(key[i]) >> startBit) & (KEY_DIGITS_PER_STEP - 1));

				#pragma unroll
				for (uint bit = 0; bit < KEY_DIGITS_PER_STEP; bit += 4)
				{
					accum[bit >> 2] += (1u << ((keyDigit - bit) << 3)); //in PTX shifts are clamped to 32, so it's ok here
				}
			}

			accumCount += ElemsPerThread;
			//check overflow
			if (accumCount + ElemsPerThread > 0xFF)
			{
				processReduce<WarpStride>(accum, sdata, sWarpCounters);

				accumCount = 0;
				#pragma unroll
				for (uint i = 0; i < (KEY_DIGITS_PER_STEP >> 2); ++i)
				{
					accum[i] = 0;
				}
			}
		}
	}
	if (accumCount > 0)
	{
		processReduce<WarpStride>(accum, sdata, sWarpCounters);
	}
	__syncthreads();

	#pragma unroll
	for (uint digit = warpIdx; digit < KEY_DIGITS_PER_STEP; digit += WarpsPerBlock)
	{
		uint val = (idxInWarp < WarpsPerBlock) ? sWarpCounters[digit*WarpStride + idxInWarp] : 0;
		uint res = reduceWarp(val, sdata);

		if (idxInWarp == 0)
		{
			tempScan[gridDim.x*digit + blockIdx.x] = res;
		}
	}
}


template <uint WarpsPerBlock, uint VectorSize, bool doFlip>
inline __device__ void radixSortStep2(int count, int blockCount, uint* tempScan)
{
	const unsigned int idx = threadIdx.x;
	const unsigned int ScanCount = ((blockCount * KEY_DIGITS_PER_STEP) >> 2);

	const unsigned int BlockSize = (WarpsPerBlock << LOG2_WARP_SIZE);
	__shared__ volatile uint sdata[BlockSize*2];

	uint4 val = (idx < ScanCount) ? ((uint4*)tempScan)[idx] : make_uint4(0, 0, 0, 0);

	val = scan4(ScanCount, val, sdata);

	if (idx < ScanCount)
	{
		val.x = min(val.x, count);
		val.y = min(val.y, count);
		val.z = min(val.z, count);
		val.w = min(val.w, count);
		((uint4*)tempScan)[idx] = val;
	}
}

template <uint WarpsPerBlock, uint VectorSize, bool doFlip>
inline __device__ void radixSortStep3(uint count, uint startBit, uint* inpKeys, uint* inpValues, uint* outKeys, uint* outValues, uint* tempScan)
{
	const unsigned int Log2ElemsPerThread = VectorTT<VectorSize>::Log2Size;
	typedef typename VectorTT<VectorSize>::AccessType ElemAccessType;

	const unsigned int Log2DataWarpSize = (LOG2_WARP_SIZE + Log2ElemsPerThread);
	const unsigned int DataWarpSize = (1 << Log2DataWarpSize);

	const unsigned int GridDataWarpsCount = ((count + DataWarpSize-1) >> Log2DataWarpSize);
	const unsigned int DataWarpsResidue = (GridDataWarpsCount % gridDim.x);
	const unsigned int DataWarpsExtra = (blockIdx.x < DataWarpsResidue) ? 1 : 0;
	const unsigned int DataWarpsCount = (GridDataWarpsCount / gridDim.x) + DataWarpsExtra;
	const unsigned int DataWarpsOffset = blockIdx.x * DataWarpsCount + DataWarpsResidue * (1 - DataWarpsExtra);

	if (DataWarpsCount == 0)
	{
		return;
	}

	const unsigned int BlockSize = (WarpsPerBlock << LOG2_WARP_SIZE);
	const unsigned int BlockDataSize = (WarpsPerBlock << Log2DataWarpSize);

	const unsigned int idx = threadIdx.x;

	const uint WarpStride = WarpsPerBlock + 1; //+1 here is to avoid shared memory bank conflicts
	__shared__ uint sWarpCounters[KEY_DIGITS_PER_STEP * WarpStride + 1]; //+1 here is to store total scan at the end

	__shared__ uint sCounters[KEY_DIGITS_PER_STEP];
	if (idx < KEY_DIGITS_PER_STEP)
	{
		sCounters[idx] = tempScan[gridDim.x*idx + blockIdx.x];
	}

	//temp shared memory for scan
	__shared__ volatile uint sdata[BlockSize*2];

	__shared__ uint sKeys[BlockDataSize];
	__shared__ uint sValues[BlockDataSize];

#if DEBUG_RADIX_SORT
	for (int i = idx; i < BlockDataSize; i += BlockSize)
	{
		sKeys[i] = -2;
		sValues[i] = -2;
	}
	__syncthreads();
#endif


	uint blockBeg = (DataWarpsOffset << Log2DataWarpSize);
	uint blockEnd = min(blockBeg + (DataWarpsCount << Log2DataWarpSize), count);
	int remainDataCount = (blockEnd - blockBeg);

	uint blockPos = (DataWarpsOffset << LOG2_WARP_SIZE);

	for (; remainDataCount >= BlockDataSize; remainDataCount -= BlockDataSize, blockPos += BlockSize)
	{
		localSortStepBlock<WarpsPerBlock, VectorSize, doFlip, WarpStride, true>(
			count, startBit, inpKeys, inpValues, outKeys, outValues, 
			sKeys, sValues, sWarpCounters, sdata, sCounters,
			BlockDataSize, blockPos);
	}
	if (remainDataCount > 0)
	{
		localSortStepBlock<WarpsPerBlock, VectorSize, doFlip, WarpStride, false>(
			count, startBit, inpKeys, inpValues, outKeys, outValues, 
			sKeys, sValues, sWarpCounters, sdata, sCounters,
			remainDataCount, blockPos);
	}
}


template <uint WarpsPerBlock, uint VectorSize, bool doFlip>
inline __device__ void radixSortBlock(uint count, uint startBit0, uint startBit1, uint* inpKeys, uint* inpValues)
{
	const unsigned int Log2ElemsPerThread = VectorTT<VectorSize>::Log2Size;
	const unsigned int ElemsPerThread = VectorTT<VectorSize>::Size;
	typedef typename VectorTT<VectorSize>::AccessType ElemAccessType;

	const unsigned int Log2DataWarpSize = (LOG2_WARP_SIZE + Log2ElemsPerThread);
	const unsigned int DataWarpSize = (1 << Log2DataWarpSize);

	const unsigned int DataWarpsCount = ((count + DataWarpSize-1) >> Log2DataWarpSize);

	const unsigned int BlockSize = (WarpsPerBlock << LOG2_WARP_SIZE);
	const unsigned int BlockDataSize = (WarpsPerBlock << Log2DataWarpSize);

	const unsigned int idx = threadIdx.x;
	const unsigned int warpIdx = (idx >> LOG2_WARP_SIZE);

	const uint WarpStride = WarpsPerBlock + 1; //+1 here is to avoid shared memory bank conflicts
	__shared__ uint sWarpCounters[KEY_DIGITS_PER_STEP * WarpStride + 1]; //+1 here is to store total scan at the end

	//temp shared memory for scan
	__shared__ volatile uint sdata[BlockSize*2];

	__shared__ uint sKeys[BlockDataSize];
	__shared__ uint sValues[BlockDataSize];

	//read from memory
	uint key[ElemsPerThread];
	uint value[ElemsPerThread];

	readKeyAndValue<VectorSize, doFlip, false>(count, inpKeys, inpValues, key, value, DataWarpsCount, 0);

	for (uint startBit = startBit0; startBit < startBit1; startBit += KEY_BITS_PER_STEP)
	{
		localSortStep<WarpsPerBlock, VectorSize, WarpStride, false>(count, startBit, key, value, sKeys, sValues, sWarpCounters, sdata, DataWarpsCount);

		__syncthreads();

		if (warpIdx < DataWarpsCount)
		{
			#pragma unroll
			for (int i = 0; i < ElemsPerThread; ++i)
			{
				key[i] = sKeys[(idx << Log2ElemsPerThread) + i];
				value[i] = sValues[(idx << Log2ElemsPerThread) + i];
			}
		}
	}

	//output to memory
	if (warpIdx < DataWarpsCount)
	{
		if (doFlip)
		{
			#pragma unroll
			for (int i = 0; i < ElemsPerThread; ++i)
			{
				key[i] = KeyTraits<true>::unflip(key[i]);
			}
		}

		uint inpPos = idx;
		if (inpPos < (count >> Log2ElemsPerThread))
		{
			((ElemAccessType*)inpKeys)[inpPos] = *((ElemAccessType*)key);
			((ElemAccessType*)inpValues)[inpPos] = *((ElemAccessType*)value);
		}
		else
		{
			inpPos <<= Log2ElemsPerThread;
			#pragma unroll
			for (int i = 0; i < ElemsPerThread; ++i, ++inpPos)
			{
				if (inpPos < count)
				{
					inpKeys[inpPos] = key[i];
					inpValues[inpPos] = value[i];
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------

BOUND_KERNEL_BEG(SORT_NEW_WARPS_PER_BLOCK, newRadixSortBlockKernel,
	uint count, uint bitCount, uint startBit, uint* inpKeys, uint* inpValues
)
	radixSortBlock<WarpsPerBlock, SORT_NEW_VECTOR_SIZE, false>(count, startBit, startBit + bitCount, inpKeys, inpValues);

BOUND_KERNEL_END()

BOUND_KERNEL_BEG(SORT_NEW_WARPS_PER_BLOCK, newRadixSortStep1Kernel,
	uint count, uint startBit, uint* inpKeys, uint* inpValues, uint* outKeys, uint* outValues, uint* tempScan
)
	radixSortStep1<WarpsPerBlock, SORT_NEW_VECTOR_SIZE, false>(count, startBit, inpKeys, inpValues, outKeys, outValues, tempScan);

BOUND_KERNEL_END()

BOUND_KERNEL_BEG(SORT_NEW_WARPS_PER_BLOCK, newRadixSortStep2Kernel,
	uint count, uint blockCount, uint* tempScan
)
	radixSortStep2<WarpsPerBlock, SORT_NEW_VECTOR_SIZE, false>(count, blockCount, tempScan);

BOUND_KERNEL_END()

BOUND_KERNEL_BEG(SORT_NEW_WARPS_PER_BLOCK, newRadixSortStep3Kernel,
	uint count, uint startBit, uint* inpKeys, uint* inpValues, uint* outKeys, uint* outValues, uint* tempScan
)
	radixSortStep3<WarpsPerBlock, SORT_NEW_VECTOR_SIZE, false>(count, startBit, inpKeys, inpValues, outKeys, outValues, tempScan);

BOUND_KERNEL_END()
