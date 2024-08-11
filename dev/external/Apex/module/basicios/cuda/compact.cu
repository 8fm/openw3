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
#include "include/compact.h"


inline __device__ unsigned int fetchHoleScan(unsigned int index, unsigned int& flag)
{
	const unsigned int holeScan = tex1Dfetch(KERNEL_TEX_REF(CompactScanSum), index);
	flag = (holeScan >> HOLE_SCAN_FLAG_BIT);
	return (holeScan & HOLE_SCAN_MASK); //inclusive
}
inline __device__ unsigned int fetchOutputScan(unsigned int index, unsigned int start, unsigned int holesBeforeStart, unsigned int& flag)
{
	unsigned int holeScan = fetchHoleScan(index, flag);
	if (index < start)
	{
		return holeScan; //inclusive
	}
	else
	{
		flag ^= 1;
		const unsigned int nonHoleScan = (index + 1) - holeScan; //inclusive
		const unsigned int nonHolesBeforeStart = start - holesBeforeStart;
		return holesBeforeStart + (nonHoleScan - nonHolesBeforeStart); //inclusive
	}
}

BOUND_KERNEL_BEG(COMPACT_WARPS_PER_BLOCK, compactKernel,
	unsigned int targetCount, unsigned int totalCount, unsigned int injectorCount, unsigned int* g_outIndices, unsigned int* g_outCount, unsigned int* g_injCounters
)
	const unsigned int start = targetCount;

	__shared__ unsigned int holesBeforeStart;
	if (threadIdx.x == 0) {
		unsigned int flag;
		holesBeforeStart = (start > 0) ? fetchHoleScan(start-1, flag) : 0;
		if (blockIdx.x == 0) {
			g_outCount[0] = holesBeforeStart;
		}
	}
	__syncthreads();

	{
		const unsigned int idx = threadIdx.x;
		const unsigned int warpIdx = (idx >> LOG2_WARP_SIZE);
		const unsigned int idxInWarp = idx & (WARP_SIZE-1);

		const unsigned int CountPerBlock = (totalCount + gridDim.x-1) / gridDim.x;

		const unsigned int DataWarpsPerBlock = (CountPerBlock + WARP_SIZE-1) / WARP_SIZE;
		const unsigned int WarpBorder = DataWarpsPerBlock % WarpsPerBlock;
		const unsigned int WarpFactor = DataWarpsPerBlock / WarpsPerBlock;

		const unsigned int WarpSelect = (warpIdx < WarpBorder) ? 1 : 0;
		const unsigned int WarpCount = WarpFactor + WarpSelect;
		const unsigned int WarpOffset = warpIdx * WarpCount + WarpBorder * (1 - WarpSelect);

		const unsigned int blockBeg = blockIdx.x * CountPerBlock;
		const unsigned int blockEnd = min(blockBeg + CountPerBlock, totalCount);

		const unsigned int warpBeg = blockBeg + (WarpOffset << LOG2_WARP_SIZE);
		const unsigned int warpEnd = min(warpBeg + (WarpCount << LOG2_WARP_SIZE), blockEnd);

		const unsigned int Log2BufferSize = (LOG2_WARP_SIZE + 1);
		const unsigned int BufferSize = (1 << Log2BufferSize);

		__shared__ volatile unsigned int sdata[WARP_SIZE * WarpsPerBlock];

		__shared__ volatile unsigned int buffer[BufferSize*WarpsPerBlock];
		__shared__ volatile unsigned int outputBeg[WarpsPerBlock];

		if (warpBeg < warpEnd)
		{
			if (idxInWarp == 0) {
				unsigned int flag;
				outputBeg[warpIdx] = (warpBeg > 0) ? fetchOutputScan(warpBeg-1, start, holesBeforeStart, flag) : 0;
			}

			unsigned int bufferBeg = outputBeg[warpIdx] & (WARP_SIZE-1);
			unsigned int bufferEnd = bufferBeg;

			if (idxInWarp == 0) {
				outputBeg[warpIdx] &= ~(WARP_SIZE-1);
			}

			for (unsigned int i = warpBeg; i < warpEnd; i += WARP_SIZE)
			{
				unsigned int inputPos = i + idxInWarp;
				if (inputPos < warpEnd)
				{
					unsigned int flag;
					unsigned int outputPos = fetchOutputScan(inputPos, start, holesBeforeStart, flag);
					unsigned int bufferPos = outputPos - outputBeg[warpIdx];

					sdata[idx] = bufferPos;
					if (flag)
					{
						bufferPos -= 1; //inclusive -> exclusive
						bufferPos += (bufferBeg & WARP_SIZE);
						bufferPos &= (BufferSize-1);

						buffer[(warpIdx << Log2BufferSize) + bufferPos] = inputPos;
					}
				}

				unsigned int endOfWarp = (min(i + WARP_SIZE, warpEnd)-1 - warpBeg) & (WARP_SIZE-1);
				bufferEnd = (bufferBeg & WARP_SIZE) + sdata[(warpIdx << LOG2_WARP_SIZE) + endOfWarp];
				bufferEnd &= (BufferSize-1);

				if ((bufferBeg & WARP_SIZE) != (bufferEnd & WARP_SIZE))
				{
					if (idxInWarp >= (bufferBeg & (WARP_SIZE-1)) ) {
						g_outIndices[outputBeg[warpIdx] + idxInWarp] = buffer[(warpIdx << Log2BufferSize) + (bufferBeg & WARP_SIZE) + idxInWarp];
					}
					bufferBeg = (bufferEnd & WARP_SIZE);
					if (idxInWarp == 0) {
						outputBeg[warpIdx] += WARP_SIZE;
					}
				}
			}

			if ( idxInWarp >= (bufferBeg & (WARP_SIZE-1)) && idxInWarp < (bufferEnd & (WARP_SIZE-1)) ) {
				g_outIndices[outputBeg[warpIdx] + idxInWarp] = buffer[(warpIdx << Log2BufferSize) + (bufferBeg & WARP_SIZE) + idxInWarp];
			}
		}
	}

	if (injectorCount > HISTOGRAM_SIMULATE_BIN_COUNT)
	{
		for (physx::PxU32 pos = BlockSize*blockIdx.x + threadIdx.x; pos < injectorCount; pos += BlockSize*gridDim.x)
		{
			g_injCounters[ pos ] = 0;
		}
	}
BOUND_KERNEL_END()
