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
#include "reduce.cuh"

#include "include/common.h"
using namespace physx::apex;
using namespace physx::apex::iofx;
#include "include/bbox.h"


#define DEBUG_BBOX2 0

#if DEBUG_BBOX2
	typedef AddOPf OP;
	#define OUTPUT_DEBUG_INFO(pos, a1, a2, a3, a4) ((float4*)g_positionMass)[pos] = make_float4(a1, a2, a3, a4);
#else
	typedef MinOPf OP;
	#define OUTPUT_DEBUG_INFO(pos, a1, a2, a3, a4)
#endif

#define REDUCE_STEP(sdata, pitch, n) \
	sdata[pitch*0 + idx] = OP::apply(sdata[pitch*0 + idx], sdata[pitch*0 + idx + n]); \
	sdata[pitch*1 + idx] = OP::apply(sdata[pitch*1 + idx], sdata[pitch*1 + idx + n]); \
	sdata[pitch*2 + idx] = OP::apply(sdata[pitch*2 + idx], sdata[pitch*2 + idx + n]); \
	sdata[pitch*3 + idx] = OP::apply(sdata[pitch*3 + idx], sdata[pitch*3 + idx + n]); \
	sdata[pitch*4 + idx] = OP::apply(sdata[pitch*4 + idx], sdata[pitch*4 + idx + n]); \
	sdata[pitch*5 + idx] = OP::apply(sdata[pitch*5 + idx], sdata[pitch*5 + idx + n]);

#define REDUCE_BLOCK_WHOLE_WARP() \
	if (idxInWarp < 16) { \
		REDUCE_STEP(sBlockBounds, BlockPitch, 16) \
		REDUCE_STEP(sBlockBounds, BlockPitch, 8) \
		REDUCE_STEP(sBlockBounds, BlockPitch, 4) \
		REDUCE_STEP(sBlockBounds, BlockPitch, 2) \
		REDUCE_STEP(sBlockBounds, BlockPitch, 1) \
	}

#define REDUCE_BLOCK_STEP(n) \
	if (idxInWarp + n < WARP_SIZE && sBlockActorID[idx + n] == sBlockActorID[idx]) \
	{ \
		REDUCE_STEP(sBlockBounds, BlockPitch, n) \
	}

#define REDUCE_WARP_STEP(n) \
	if (idx + n < WARP_SIZE && sWarpLastActorID[idx + n] == sWarpLastActorID[idx]) \
	{ \
		sWarpSegmentSize[idx] += sWarpSegmentSize[idx + n]; \
		REDUCE_STEP(sWarpLastBounds, WarpPitch, n) \
	}

#define INPUT_BOUNDS(dst, pitch, idx, vmin, vmax) \
	dst[pitch*0 + idx] = vmin.x; \
	dst[pitch*1 + idx] = vmin.y; \
	dst[pitch*2 + idx] = vmin.z; \
	dst[pitch*3 + idx] = -vmax.x; \
	dst[pitch*4 + idx] = -vmax.y; \
	dst[pitch*5 + idx] = -vmax.z;

#define INPUT_OP_BOUNDS(dst, pitch, idx, vmin, vmax) \
	dst[pitch*0 + idx] = OP::apply(dst[pitch*0 + idx], vmin.x); \
	dst[pitch*1 + idx] = OP::apply(dst[pitch*1 + idx], vmin.y); \
	dst[pitch*2 + idx] = OP::apply(dst[pitch*2 + idx], vmin.z); \
	dst[pitch*3 + idx] = OP::apply(dst[pitch*3 + idx], -vmax.x); \
	dst[pitch*4 + idx] = OP::apply(dst[pitch*4 + idx], -vmax.y); \
	dst[pitch*5 + idx] = OP::apply(dst[pitch*5 + idx], -vmax.z);

#define OUTPUT_BOUNDS(dst, pos, src, pitch, idx) \
	dst##MinBounds[pos] = make_float4( \
		src[pitch*0 + idx], \
		src[pitch*1 + idx], \
		src[pitch*2 + idx], 0); \
	dst##MaxBounds[pos] = make_float4( \
		-src[pitch*3 + idx], \
		-src[pitch*4 + idx], \
		-src[pitch*5 + idx], 0);

#define RESET_LAST_BOUNDS() \
	if (idxInWarp < 6) { \
		sWarpLastBounds[WarpPitch*idxInWarp + warpIdx] = OP::identity(); \
	}

#define UPDATE_LAST_BOUNDS() \
	if (idxInWarp < 6) { \
		sWarpLastBounds[WarpPitch*idxInWarp + warpIdx] = OP::apply( \
			sWarpLastBounds[WarpPitch*idxInWarp + warpIdx], sBlockBounds[BlockPitch*idxInWarp + warpFirstIdx]); \
	}

#define WRITE_LAST_BOUNDS() \
	if (firstActorID != UINT_MAX) { \
		if (idxInWarp == 0) { \
			OUTPUT_BOUNDS(g_out, lastActorID, sWarpLastBounds, WarpPitch, warpIdx) \
			OUTPUT_DEBUG_INFO(lastActorID, 0, blockIdx.x, warpIdx, pos) \
		} \
	} else { \
		firstActorID = lastActorID; \
		if (idxInWarp < 6) { \
			sWarpFirstBounds[WarpPitch*idxInWarp + warpIdx] = sWarpLastBounds[WarpPitch*idxInWarp + warpIdx]; \
		} \
	}


template <unsigned int WarpsPerBlock, unsigned int BlockSize, unsigned int BlockPitch, unsigned int WarpPitch>
inline __device__ void bbox1(
	unsigned int count,
	unsigned int* g_actorID,
	unsigned int* stateToInput,
	const float4* g_positionMass,
	float4* g_outMinBounds, float4* g_outMaxBounds,
	unsigned int* g_tmpLastActorID, unsigned int* g_tmpFirstActorID,
	float4* g_tmpLastMinBounds, float4* g_tmpFirstMinBounds,
	float4* g_tmpLastMaxBounds, float4* g_tmpFirstMaxBounds,
	volatile unsigned int* sBlockActorID, volatile float* sBlockBounds,
	volatile unsigned int* sWarpLastActorID, volatile float* sWarpLastBounds,
	volatile unsigned int* sWarpFirstActorID, volatile float* sWarpFirstBounds)
{
	const unsigned int DataWarpsPerGrid = ((count + WARP_SIZE-1) >> LOG2_WARP_SIZE);
	const unsigned int DataWarpsPerBlock = (DataWarpsPerGrid + gridDim.x-1) / gridDim.x;
	const unsigned int DataCountPerBlock = (DataWarpsPerBlock << LOG2_WARP_SIZE);

	const unsigned int WarpLimit = min(DataWarpsPerBlock, WarpsPerBlock);
	const unsigned int WarpBorder = DataWarpsPerBlock % WarpsPerBlock;
	const unsigned int WarpFactor = DataWarpsPerBlock / WarpsPerBlock;

	const unsigned int idx = threadIdx.x;
	const unsigned int warpIdx = (idx >> LOG2_WARP_SIZE);
	const unsigned int blockBeg = blockIdx.x * DataCountPerBlock;
	const unsigned int blockEnd = min(blockBeg + DataCountPerBlock, count);

	const unsigned int WarpSelect = (warpIdx < WarpBorder) ? 1 : 0;
	const unsigned int WarpCount = WarpFactor + WarpSelect;
	const unsigned int WarpOffset = warpIdx * WarpCount + WarpBorder * (1 - WarpSelect);

	const unsigned int warpBeg = blockBeg + (WarpOffset << LOG2_WARP_SIZE);
	const unsigned int warpEnd = min(warpBeg + (WarpCount << LOG2_WARP_SIZE), blockEnd);

	const unsigned int idxInWarp = idx & (WARP_SIZE-1);


	unsigned int lastActorID = UINT_MAX;
	unsigned int firstActorID = UINT_MAX;
	if (warpBeg < warpEnd)
	{
		unsigned int lastWholeWarp = 0;

		const unsigned int warpFirstIdx = idx & ~(WARP_SIZE-1);

		unsigned int pos;
		for (pos = warpBeg + idxInWarp; pos < (warpEnd & ~(WARP_SIZE-1)); pos += WARP_SIZE)
		{
			//read data
			unsigned int actorID = (g_actorID[pos] >> NiIofxActorID::ASSETS_PER_MATERIAL_BITS);
			sBlockActorID[idx] = actorID;

			const unsigned int warpFirstActorID = sBlockActorID[warpFirstIdx];
			const unsigned int warpLastActorID = sBlockActorID[warpFirstIdx + (WARP_SIZE - 1)];

#if DEBUG_BBOX2
			const float4 point = make_float4(1, 2, 3, 0);
#else
			unsigned int input = stateToInput[ pos ];
			const float4 point = tex1Dfetch(KERNEL_TEX_REF(BBoxPositions), input);
#endif
			if (lastWholeWarp)
			{
				if (warpLastActorID == lastActorID) {
					//current warp is also whole - just accum. whole warp
					INPUT_OP_BOUNDS(sBlockBounds, BlockPitch, idx, point, point)
					continue;
				}

				//we have not whole warp - reduce last whole warp
				REDUCE_BLOCK_WHOLE_WARP()
				//and update the last data
				UPDATE_LAST_BOUNDS()
				lastWholeWarp = 0;
			}

			INPUT_BOUNDS(sBlockBounds, BlockPitch, idx, point, point)

			if (warpFirstActorID == warpLastActorID) {
				//we have a whole warp
				if (warpFirstActorID != lastActorID) {
					if (lastActorID != UINT_MAX) {
						WRITE_LAST_BOUNDS()
					}
					//reset last bounds
					RESET_LAST_BOUNDS()
					lastActorID = actorID; //actorID is the same for the whole warp!
				}
				lastWholeWarp = 1;
				continue;
			}

			//reduce
			REDUCE_BLOCK_STEP(1)
			REDUCE_BLOCK_STEP(2)
			REDUCE_BLOCK_STEP(4)
			REDUCE_BLOCK_STEP(8)
			REDUCE_BLOCK_STEP(16)

			if (lastActorID == UINT_MAX) {
				RESET_LAST_BOUNDS()
				lastActorID = warpFirstActorID;
			}
			//update the last data
			if (warpFirstActorID == lastActorID) {
				UPDATE_LAST_BOUNDS()
			}
			//and write it
			WRITE_LAST_BOUNDS()

			unsigned int prevActorID = (idxInWarp > 0) ? sBlockActorID[idx - 1] : lastActorID;
			if (prevActorID != actorID) {
				if (actorID == warpLastActorID) {
					sWarpLastActorID[warpIdx] = idx;
#if FIX_FOR_KEPLER
					sWarpLastBounds[WarpPitch*0 + warpIdx] = sBlockBounds[BlockPitch*0 + idx];
					sWarpLastBounds[WarpPitch*1 + warpIdx] = sBlockBounds[BlockPitch*1 + idx];
					sWarpLastBounds[WarpPitch*2 + warpIdx] = sBlockBounds[BlockPitch*2 + idx];
					sWarpLastBounds[WarpPitch*3 + warpIdx] = sBlockBounds[BlockPitch*3 + idx];
					sWarpLastBounds[WarpPitch*4 + warpIdx] = sBlockBounds[BlockPitch*4 + idx];
					sWarpLastBounds[WarpPitch*5 + warpIdx] = sBlockBounds[BlockPitch*5 + idx];
#endif
				} else {
					OUTPUT_BOUNDS(g_out, actorID, sBlockBounds, BlockPitch, idx)
					OUTPUT_DEBUG_INFO(actorID, 1, blockIdx.x, warpIdx, pos)
				}
			}
#if !FIX_FOR_KEPLER
			//set the last data
			if (idxInWarp < 6) {
				sWarpLastBounds[WarpPitch*idxInWarp + warpIdx] = sBlockBounds[BlockPitch*idxInWarp + sWarpLastActorID[warpIdx]];
			}
#endif
			lastActorID = warpLastActorID;
		}

		if (lastWholeWarp)
		{
			//reduce last whole warp
			REDUCE_BLOCK_WHOLE_WARP()
			//and update the last data
			UPDATE_LAST_BOUNDS()
			//lastWholeWarp = 0;
		}

		//handle the last non-whole warp
		unsigned int lastWarpSize = warpEnd & (WARP_SIZE-1);
		if (lastWarpSize > 0)
		{
			unsigned int actorID = (pos < warpEnd) ? (g_actorID[pos] >> NiIofxActorID::ASSETS_PER_MATERIAL_BITS) : UINT_MAX;
			sBlockActorID[idx] = actorID;

			const unsigned int warpFirstActorID = sBlockActorID[warpFirstIdx];
			const unsigned int warpLastActorID = sBlockActorID[warpFirstIdx + (lastWarpSize - 1)];

			if (pos < warpEnd) {
				//read data
#if DEBUG_BBOX2
				const float4 point = make_float4(1, 2, 3, 0);
#else
				unsigned int input = stateToInput[ pos ];
				const float4 point = tex1Dfetch(KERNEL_TEX_REF(BBoxPositions), input);
#endif
				INPUT_BOUNDS(sBlockBounds, BlockPitch, idx, point, point)
			}

			//reduce
			REDUCE_BLOCK_STEP(1)
			REDUCE_BLOCK_STEP(2)
			REDUCE_BLOCK_STEP(4)
			REDUCE_BLOCK_STEP(8)
			REDUCE_BLOCK_STEP(16)

			if (lastActorID == UINT_MAX) {
				RESET_LAST_BOUNDS()
				lastActorID = warpFirstActorID;
			}
			if (warpFirstActorID == lastActorID) {
				//update the last data
				UPDATE_LAST_BOUNDS()
			}
			if (warpLastActorID != lastActorID) {
				WRITE_LAST_BOUNDS()

				unsigned int prevActorID = (idxInWarp > 0) ? sBlockActorID[idx - 1] : lastActorID;
				if (actorID != UINT_MAX && prevActorID != actorID) {
					if (actorID == warpLastActorID) {
						sWarpLastActorID[warpIdx] = idx;
					} else {
						OUTPUT_BOUNDS(g_out, actorID, sBlockBounds, BlockPitch, idx)
						OUTPUT_DEBUG_INFO(actorID, 1, blockIdx.x, warpIdx, pos)
					}
				}
				//set the last data
				if (idxInWarp < 6) {
					sWarpLastBounds[WarpPitch*idxInWarp + warpIdx] = sBlockBounds[BlockPitch*idxInWarp + sWarpLastActorID[warpIdx]];
				}
				lastActorID = warpLastActorID;
			}
		}
	}
	if (idxInWarp == 0) {
		sWarpLastActorID[warpIdx] = lastActorID;
		sWarpFirstActorID[warpIdx] = firstActorID;
	}
	//set the last guard values
	if (idx >= WarpLimit && idx <= WARP_SIZE)
	{
		sWarpLastActorID[idx] = UINT_MAX;
		sWarpFirstActorID[idx] = UINT_MAX;
	}
	__syncthreads();

	volatile unsigned int* sWarpSegmentSize = sBlockActorID;

	//check for empty block
	if (sWarpLastActorID[0] != UINT_MAX)
	{
		//one warp
		if (idx < WARP_SIZE)
		{
			sWarpSegmentSize[idx] = 1;

			REDUCE_WARP_STEP(1)
			REDUCE_WARP_STEP(2)
			REDUCE_WARP_STEP(4)
			REDUCE_WARP_STEP(8)
			REDUCE_WARP_STEP(16)

			float4* outMinBounds = 0;
			float4* outMaxBounds = 0;

			//combine with the first bounds of next segment
			unsigned int actorID = sWarpLastActorID[idx];
			unsigned int prevActorID = (idx > 0) ? sWarpLastActorID[idx - 1] : UINT_MAX;
			if (actorID != UINT_MAX && actorID != prevActorID)
			{
				unsigned int nextSegmentIdx = idx + sWarpSegmentSize[idx];
				if (sWarpFirstActorID[nextSegmentIdx] == actorID) {
					#pragma unroll
					for (int i = 0; i < 6; ++i) {
						sWarpLastBounds[WarpPitch*i + idx] = OP::apply(sWarpLastBounds[WarpPitch*i + idx],
							sWarpFirstBounds[WarpPitch*i + nextSegmentIdx]);
					}
					sWarpFirstActorID[nextSegmentIdx] = UINT_MAX;
				}

				if (sWarpLastActorID[nextSegmentIdx] == UINT_MAX) {
					//last segment
					outMinBounds = g_tmpLastMinBounds + blockIdx.x;
					outMaxBounds = g_tmpLastMaxBounds + blockIdx.x;
					g_tmpLastActorID[blockIdx.x] = actorID;
				}
				else
				{
					outMinBounds = g_outMinBounds + actorID;
					outMaxBounds = g_outMaxBounds + actorID;
					OUTPUT_DEBUG_INFO(actorID, 3, blockIdx.x, idx, -1)
				}
			}

			if (sWarpFirstActorID[0] == UINT_MAX && sWarpLastActorID[sWarpSegmentSize[0]] != UINT_MAX)
			{
				if (idx < 6) {
					sWarpFirstBounds[WarpPitch*idx + 0] = sWarpLastBounds[WarpPitch*idx + 0];
				}
				if (idx == 0) {
					sWarpFirstActorID[0] = sWarpLastActorID[0];
					//exclude output of the last bounds for 0 thread
					outMinBounds = outMaxBounds = 0;
				}
			}
			//output last bounds
			if (outMinBounds != 0)
			{
				OUTPUT_BOUNDS(out, 0, sWarpLastBounds, WarpPitch, idx)
			}

			//output first bounds
			actorID = sWarpFirstActorID[idx];
			if (idx == 0) {
				g_tmpFirstActorID[blockIdx.x] = actorID;
			}
			if (actorID != UINT_MAX) {
				if (idx == 0) {
					outMinBounds = g_tmpFirstMinBounds + blockIdx.x;
					outMaxBounds = g_tmpFirstMaxBounds + blockIdx.x;
				} else {
					outMinBounds = g_outMinBounds + actorID;
					outMaxBounds = g_outMaxBounds + actorID;
					OUTPUT_DEBUG_INFO(actorID, 4, blockIdx.x, idx, -1)
				}
				OUTPUT_BOUNDS(out, 0, sWarpFirstBounds, WarpPitch, idx)
			}
		}
	}
	else
	{
		if (idx == 0) {
			g_tmpLastActorID[blockIdx.x] = UINT_MAX;
			g_tmpFirstActorID[blockIdx.x] = UINT_MAX;
		}
	}
}

template <unsigned int WarpsPerBlock, unsigned int BlockSize, unsigned int BlockPitch, unsigned int WarpPitch>
inline __device__ void bbox2(
	unsigned int* g_actorID,
	unsigned int* stateToInput,
	const float4* g_positionMass,
	float4* g_outMinBounds, float4* g_outMaxBounds,
	unsigned int* g_tmpLastActorID, unsigned int* g_tmpFirstActorID,
	float4* g_tmpLastMinBounds, float4* g_tmpFirstMinBounds,
	float4* g_tmpLastMaxBounds, float4* g_tmpFirstMaxBounds,
	volatile unsigned int* sBlockActorID, volatile float* sBlockBounds,
	volatile unsigned int* sWarpLastActorID, volatile float* sWarpLastBounds,
	volatile unsigned int* sWarpFirstActorID, volatile float* sWarpFirstBounds,
	unsigned int gridSize)
{
	const unsigned int idx = threadIdx.x;

	if (idx <= WARP_SIZE)
	{
		sWarpLastActorID[idx] = (idx < gridSize) ? g_tmpLastActorID[idx] : UINT_MAX;
		sWarpFirstActorID[idx] = (idx < gridSize) ? g_tmpFirstActorID[idx] : UINT_MAX;
	}
	__syncthreads();

	volatile unsigned int* sWarpSegmentSize = sBlockActorID;

	//one warp
	if (idx < WARP_SIZE)
	{
		if (idx < gridSize)
		{
			float4 minBound = g_tmpLastMinBounds[idx];
			float4 maxBound = g_tmpLastMaxBounds[idx];
			INPUT_BOUNDS(sWarpLastBounds, WarpPitch, idx, minBound, maxBound)

			minBound = g_tmpFirstMinBounds[idx];
			maxBound = g_tmpFirstMaxBounds[idx];
			INPUT_BOUNDS(sWarpFirstBounds, WarpPitch, idx, minBound, maxBound)
		}

		sWarpSegmentSize[idx] = 1;

		REDUCE_WARP_STEP(1)
		REDUCE_WARP_STEP(2)
		REDUCE_WARP_STEP(4)
		REDUCE_WARP_STEP(8)
		REDUCE_WARP_STEP(16)

		unsigned int actorID = sWarpLastActorID[idx];
		unsigned int prevActorID = (idx > 0) ? sWarpLastActorID[idx - 1] : UINT_MAX;
		if (actorID != UINT_MAX && actorID != prevActorID)
		{
			float bounds[6];
			#pragma unroll
			for (int i = 0; i < 6; ++i) {
				bounds[i] = sWarpLastBounds[WarpPitch*i + idx];
			}

			unsigned int nextSegmentIdx = idx + sWarpSegmentSize[idx];
			if (sWarpFirstActorID[nextSegmentIdx] == actorID) {
				//combine with the first bounds of next segment
				#pragma unroll
				for (int i = 0; i < 6; ++i) {
					bounds[i] = OP::apply(bounds[i], sWarpFirstBounds[WarpPitch*i + nextSegmentIdx]);
				}
				sWarpFirstActorID[nextSegmentIdx] = UINT_MAX;
			}
			OUTPUT_BOUNDS(g_out, actorID, bounds, 1, 0)
			OUTPUT_DEBUG_INFO(actorID, 5, idx, sWarpLastBounds[idx], sWarpFirstBounds[nextSegmentIdx])
		}

		actorID = sWarpFirstActorID[idx];
		if (actorID != UINT_MAX) {
			OUTPUT_BOUNDS(g_out, actorID, sWarpFirstBounds, WarpPitch, idx)
			OUTPUT_DEBUG_INFO(actorID, 6, idx, -1, -1)
		}
	}
}

#define BBOX_KERNEL_SETUP() \
	const unsigned int BlockPitch = BlockSize + 1; \
	__shared__ volatile unsigned int    sBlockActorID[BlockSize]; \
	__shared__ volatile float           sBlockBounds[BlockPitch * 6]; \
	const unsigned int WarpPitch = WARP_SIZE + 1; /*MaxGridSize is 32*/ \
	__shared__ volatile unsigned int    sWarpLastActorID[WARP_SIZE + 1]; \
	__shared__ volatile float           sWarpLastBounds[WarpPitch * 6]; \
	__shared__ volatile unsigned int    sWarpFirstActorID[WARP_SIZE + 1]; \
	__shared__ volatile float           sWarpFirstBounds[WarpPitch * 6]; \
	unsigned int* g_tmpLastActorID = g_tmpActorID; \
	unsigned int* g_tmpFirstActorID = g_tmpActorID + WARP_SIZE; \
	float4* g_tmpLastMinBounds = g_tmpMinBounds; \
	float4* g_tmpFirstMinBounds = g_tmpMinBounds + WARP_SIZE; \
	float4* g_tmpLastMaxBounds = g_tmpMaxBounds; \
	float4* g_tmpFirstMaxBounds = g_tmpMaxBounds + WARP_SIZE;


//g_tmpActorID, g_tmpMinBounds, g_tmpMaxBounds should have size = WARP_SIZE*2!!!
SYNC_KERNEL_BEG(BBOX_WARPS_PER_BLOCK, bboxKernel,
	unsigned int count,
	unsigned int* g_actorID,
	unsigned int* stateToInput,
	const float4* g_positionMass,
	float4* g_outMinBounds, float4* g_outMaxBounds,
	unsigned int* g_tmpActorID,
	float4* g_tmpMinBounds, float4* g_tmpMaxBounds
)
	BBOX_KERNEL_SETUP()

	bbox1<WarpsPerBlock, BlockSize, BlockPitch, WarpPitch>(
		count, g_actorID, stateToInput, g_positionMass,
		g_outMinBounds, g_outMaxBounds,
		g_tmpLastActorID, g_tmpFirstActorID,
		g_tmpLastMinBounds, g_tmpFirstMinBounds,
		g_tmpLastMaxBounds, g_tmpFirstMaxBounds,
		sBlockActorID, sBlockBounds, sWarpLastActorID, sWarpLastBounds, sWarpFirstActorID, sWarpFirstBounds);

	__threadfence();
	BLOCK_SYNC_BEGIN()

	bbox2<WarpsPerBlock, BlockSize, BlockPitch, WarpPitch>(
		g_actorID, stateToInput, g_positionMass,
		g_outMinBounds, g_outMaxBounds,
		g_tmpLastActorID, g_tmpFirstActorID,
		g_tmpLastMinBounds, g_tmpFirstMinBounds,
		g_tmpLastMaxBounds, g_tmpFirstMaxBounds,
		sBlockActorID, sBlockBounds, sWarpLastActorID, sWarpLastBounds, sWarpFirstActorID, sWarpFirstBounds,
		gridDim.x);

	BLOCK_SYNC_END()

SYNC_KERNEL_END()

BOUND_KERNEL_BEG(BBOX_WARPS_PER_BLOCK, bbox1Kernel,
	unsigned int* g_actorID,
	unsigned int* stateToInput,
	const float4* g_positionMass,
	float4* g_outMinBounds, float4* g_outMaxBounds,
	unsigned int* g_tmpActorID,
	float4* g_tmpMinBounds, float4* g_tmpMaxBounds
)
	BBOX_KERNEL_SETUP()

	bbox1<WarpsPerBlock, BlockSize, BlockPitch, WarpPitch>(_threadCount,
		g_actorID, stateToInput, g_positionMass,
		g_outMinBounds, g_outMaxBounds,
		g_tmpLastActorID, g_tmpFirstActorID,
		g_tmpLastMinBounds, g_tmpFirstMinBounds,
		g_tmpLastMaxBounds, g_tmpFirstMaxBounds,
		sBlockActorID, sBlockBounds, sWarpLastActorID, sWarpLastBounds, sWarpFirstActorID, sWarpFirstBounds);

BOUND_KERNEL_END()

BOUND_KERNEL_BEG(BBOX_WARPS_PER_BLOCK, bbox2Kernel,
	unsigned int* g_actorID,
	unsigned int* stateToInput,
	const float4* g_positionMass,
	float4* g_outMinBounds, float4* g_outMaxBounds,
	unsigned int* g_tmpActorID,
	float4* g_tmpMinBounds, float4* g_tmpMaxBounds,
	unsigned int gridSize
)
	BBOX_KERNEL_SETUP()

	bbox2<WarpsPerBlock, BlockSize, BlockPitch, WarpPitch>(
		g_actorID, stateToInput, g_positionMass,
		g_outMinBounds, g_outMaxBounds,
		g_tmpLastActorID, g_tmpFirstActorID,
		g_tmpLastMinBounds, g_tmpFirstMinBounds,
		g_tmpLastMaxBounds, g_tmpFirstMaxBounds,
		sBlockActorID, sBlockBounds, sWarpLastActorID, sWarpLastBounds, sWarpFirstActorID, sWarpFirstBounds,
		gridSize);

BOUND_KERNEL_END()
