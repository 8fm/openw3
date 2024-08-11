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
#include "common.cuh"

#include "include/common.h"
using namespace physx::apex;
using namespace physx::apex::pxparticleios;
#include "../include/ParticleIosCommon.h"
#include "include/simulate.h"

#include <ApexRand.h>
#include "device_functions.h"

const unsigned int TAG_SHIFT = (32 - LOG2_WARP_SIZE);
const unsigned int TAG_MASK = (1U << TAG_SHIFT) - 1;

typedef volatile unsigned int histogram_t;

inline __device__ void addToBin(histogram_t *s_WarpHist, unsigned int data, unsigned int threadTag)
{
	unsigned int count;
	do {
		count = s_WarpHist[data] & TAG_MASK;
		count = threadTag | (count + 1);
		s_WarpHist[data] = count;
	} while (s_WarpHist[data] != count);
}

template <unsigned int BlockSize, unsigned int WarpsPerBlock, typename FieldAccessor>
__device__ void simulate(unsigned int targetCount,
	unsigned int lastCount,
	float deltaTime,
	physx::PxVec3 eyePos,
	InplaceHandle<InjectorParamsArray> injectorParamsArrayHandle,
	unsigned int injectorCount,
	unsigned int* g_holeScanSum,
	unsigned int* g_inputIdToParticleIndex,
	unsigned int* g_moveCount,
	unsigned int* g_tmpHistogram, 
	unsigned int* g_InjectorsCounters, 
	float4* g_positionMass,
	float4* g_velocityLife,
	float4* g_collisionNormalFlags,
	unsigned int* g_userData,
	float* g_lifeSpan,
	float* g_lifeTime,
	float* g_density,
	unsigned int* g_injector,
	NiIofxActorID* g_iofxActorIDs,
	float* g_benefit,
	float4* g_pxPosition,
	float4* g_pxVelocity,
	float4* g_pxCollisionNormals,
	float*  g_pxDensity,
	unsigned int* g_pxFlags,
	FieldAccessor& fieldAccessor,
	GridDensityParams simParams
	)
{
	__shared__ histogram_t s_Hist[HISTOGRAM_SIMULATE_BIN_COUNT * WarpsPerBlock];
	__shared__ unsigned int moveCount;
	const unsigned int warpIdx = (threadIdx.x >> LOG2_WARP_SIZE);
	histogram_t* s_WarpHist = s_Hist + warpIdx * HISTOGRAM_SIMULATE_BIN_COUNT;
	const unsigned int tag = (threadIdx.x & (WARP_SIZE-1)) << TAG_SHIFT;

	if (threadIdx.x == 0) {
		moveCount			= g_moveCount[0];
	}

	const InjectorParamsArray* injectorParamsArray = injectorParamsArrayHandle.resolve( KERNEL_CONST_MEM(simulateConstMem) );
	const Px3InjectorParams* injectorParamsElems = injectorParamsArray->getElems( KERNEL_CONST_MEM(simulateConstMem) );

	if (injectorCount <= HISTOGRAM_SIMULATE_BIN_COUNT)
	{
		//zero warp histograms
		#pragma unroll
		for(unsigned int i = 0; i < (HISTOGRAM_SIMULATE_BIN_COUNT >> LOG2_WARP_SIZE); i++) {
			s_Hist[threadIdx.x + i * BlockSize] = 0;
		}
	}

	__syncthreads();

	unsigned int step	= BlockSize*gridDim.x;

	for (unsigned int dstIdx = BlockSize*blockIdx.x + threadIdx.x; dstIdx < targetCount; dstIdx += step)
	{
		unsigned int srcIdx = dstIdx;

		unsigned int holeScanSum	= g_holeScanSum[dstIdx];

		if ((holeScanSum & HOLE_SCAN_FLAG) != 0)
		{
			// we have a hole
			holeScanSum &= HOLE_SCAN_MASK; //remove hole flag
			holeScanSum -= 1; //inclusive -> exclusive

			srcIdx = tex1Dfetch(KERNEL_TEX_REF(MoveIndices), holeScanSum + moveCount);
		}
		bool isNewParticle = (srcIdx >= lastCount);

		unsigned int outInj;
		float benefit = simulateParticle(
			injectorParamsElems,
			deltaTime,
			eyePos,
			isNewParticle,
			srcIdx,
			dstIdx,
			g_positionMass,
			g_velocityLife,
			g_collisionNormalFlags,
			g_userData,
			g_iofxActorIDs,
			g_lifeSpan,
			g_lifeTime,
			g_density,
			g_injector,
		    g_pxPosition,
			g_pxVelocity,
			g_pxCollisionNormals,
			g_pxDensity,
			g_pxFlags,
			fieldAccessor, 
			outInj,
			simParams
		);

		g_benefit[dstIdx] = benefit;

		if (outInj < injectorCount)
		{
			if (injectorCount <= HISTOGRAM_SIMULATE_BIN_COUNT)
			{
				//update per warp histogram
				addToBin(s_WarpHist, outInj, tag);
			}
			else
			{
				atomicAdd(g_InjectorsCounters + outInj, 1);
			}
		}
	}

	__syncthreads();

	if (injectorCount <= HISTOGRAM_SIMULATE_BIN_COUNT)
	{
		//merge warp histograms & output to global memory
		for (unsigned int pos = threadIdx.x; pos < injectorCount; pos += BlockSize)
		{
			unsigned int sum = 0;
			for(unsigned int i = 0; i < WarpsPerBlock; i++)
			{
				sum += s_Hist[pos + i * HISTOGRAM_SIMULATE_BIN_COUNT] & TAG_MASK;
			}
			g_tmpHistogram[blockIdx.x * HISTOGRAM_SIMULATE_BIN_COUNT + pos] = sum;  
		}
	}
}

template <int WarpsPerBlock, int BlockSize>
inline __device__ void mergeHistogram(
	unsigned int* g_InjectorsCounters,	unsigned int* g_tmpHistogram, unsigned int gridSize, unsigned int injectorCount
)
{
	for (unsigned int pos = threadIdx.x; pos < injectorCount; pos += BlockSize)
	{
		unsigned int sum = 0;
		for (unsigned int i = 0; i < gridSize; ++i)
		{
			sum += g_tmpHistogram[i*HISTOGRAM_SIMULATE_BIN_COUNT + pos];
		}
		g_InjectorsCounters[pos] = sum;
	}
}


BOUND_KERNEL_BEG(SIMULATE_WARPS_PER_BLOCK,
	simulateKernel,
	unsigned int lastCount,
	float deltaTime,
	physx::PxVec3 eyePos,
	InplaceHandle<InjectorParamsArray> injectorParamsArrayHandle,
	unsigned int injectorCount,
	unsigned int* g_holeScanSum,
	unsigned int* g_inputIdToParticleIndex,
	unsigned int* g_moveCount,
	unsigned int* g_tmpHistogram, 
	unsigned int* g_InjectorsCounters,  
	float4* g_positionMass,
	float4* g_velocityLife,
	float4* g_collisionNormalFlags,
	unsigned int* g_userData,
	float* g_lifeSpan,
	float* g_lifeTime,
	float* g_density,
	unsigned int* g_injector,
	NiIofxActorID* g_iofxActorIDs,
	float* g_benefit,
    float4* g_pxPosition,
	float4* g_pxVelocity,
	float4* g_pxCollisionNormals,
	float*  g_pxDensity,
	unsigned int* g_pxFlags,
	GridDensityParams simParams
	)

	class FieldAccessor
	{
	public:
		__device__ PX_INLINE void operator() (unsigned int srcIdx, physx::PxVec3& velocityDelta ) { }
	} fieldAccessor;

	simulate<BlockSize, WarpsPerBlock>(_threadCount,
		lastCount,
		deltaTime,
		eyePos,
		injectorParamsArrayHandle,
		injectorCount,
		g_holeScanSum,
		g_inputIdToParticleIndex,
		g_moveCount,
		g_tmpHistogram, 
		g_InjectorsCounters,
		g_positionMass,
		g_velocityLife,
		g_collisionNormalFlags,
		g_userData,
		g_lifeSpan,
		g_lifeTime,
		g_density,
		g_injector,
		g_iofxActorIDs,
		g_benefit,
	    g_pxPosition,
		g_pxVelocity,
		g_pxCollisionNormals,
		g_pxDensity,
		g_pxFlags,
		fieldAccessor,
		simParams
	);
BOUND_KERNEL_END()

BOUND_KERNEL_BEG(SIMULATE_WARPS_PER_BLOCK,
	simulateApplyFieldKernel,
	unsigned int lastCount,
	float deltaTime,
	physx::PxVec3 eyePos,
	InplaceHandle<InjectorParamsArray> injectorParamsArrayHandle,
	unsigned int injectorCount, 
	unsigned int* g_holeScanSum,
	unsigned int* g_inputIdToParticleIndex,
	unsigned int* g_moveCount,
	unsigned int* g_tmpHistogram, 
	unsigned int* g_InjectorsCounters,  
	float4* g_positionMass,
	float4* g_velocityLife,
	float4* g_collisionNormalFlags,
	unsigned int* g_userData,
	float* g_lifeSpan,
	float* g_lifeTime,
	float* g_density,
	unsigned int* g_injector,
	NiIofxActorID* g_iofxActorIDs,
	float* g_benefit,
    float4* g_pxPosition,
	float4* g_pxVelocity,
	float4* g_pxCollisionNormals,
	float*  g_pxDensity,
	unsigned int* g_pxFlags,
	GridDensityParams simParams
	)

	class FieldAccessor
	{
	public:
		__device__ PX_INLINE void operator() (unsigned int srcIdx, physx::PxVec3& velocityDelta )
		{
			float4 field = tex1Dfetch(KERNEL_TEX_REF(Field), srcIdx);
			velocityDelta.x += field.x;
			velocityDelta.y += field.y;
			velocityDelta.z += field.z;
		}
	} fieldAccessor;

	simulate<BlockSize, WarpsPerBlock>(_threadCount,
		lastCount,
		deltaTime,
		eyePos,
		injectorParamsArrayHandle,
		injectorCount,
		g_holeScanSum,
		g_inputIdToParticleIndex,
		g_moveCount,
		g_tmpHistogram, 
		g_InjectorsCounters,  
		g_positionMass,
		g_velocityLife,
		g_collisionNormalFlags,
		g_userData,
		g_lifeSpan,
		g_lifeTime,
		g_density,
		g_injector,
		g_iofxActorIDs,
		g_benefit,
	    g_pxPosition,
		g_pxVelocity,
		g_pxCollisionNormals,
		g_pxDensity,
		g_pxFlags,
		fieldAccessor,
		simParams
	);
BOUND_KERNEL_END()


BOUND_KERNEL_BEG(SIMULATE_WARPS_PER_BLOCK, mergeHistogramKernel,
	unsigned int* g_InjectorsCounters, unsigned int* g_tmpHistogram, unsigned int gridSize, unsigned int injectorCount
)

	mergeHistogram<WarpsPerBlock, BlockSize>(g_InjectorsCounters, g_tmpHistogram, gridSize, injectorCount);

BOUND_KERNEL_END()


BOUND_KERNEL_BEG(STATE_WARPS_PER_BLOCK,
	stateKernel,
	unsigned int lastCount,
	unsigned int targetCount,
	unsigned int* g_moveCount,
	unsigned int* g_inStateToInput,
	unsigned int* g_outStateToInput)

	__shared__ unsigned int moveCount;
	if (threadIdx.x == 0) {
		moveCount = g_moveCount[0];
	}
	__syncthreads();

	for (unsigned int idx = BlockSize*blockIdx.x + threadIdx.x; idx < _threadCount; idx += BlockSize*gridDim.x)
	{
		unsigned int newInputIdx = NiIosBufferDesc::NOT_A_PARTICLE;

		unsigned int lastInputIdx = (idx < lastCount) ? g_outStateToInput[idx] : idx;

		unsigned int holeScanSum = tex1Dfetch(KERNEL_TEX_REF(HoleScanSum), lastInputIdx);
		if ((holeScanSum & HOLE_SCAN_FLAG) == 0)
		{
			//non-hole
			newInputIdx = lastInputIdx;
			if (lastInputIdx >= targetCount)
			{
				//inverse scan for non-holes:
				//holeIdx - inclusive scan of holes, and we need exclusive scan of non-holes
				//moveCount - count of holes before targetCount
				unsigned int moveIdx = (lastInputIdx - targetCount) - (holeScanSum - moveCount);
				newInputIdx = tex1Dfetch(KERNEL_TEX_REF(MoveIndices), moveIdx);
			}

			if (lastInputIdx >= lastCount)
			{
				//new particle
				newInputIdx |= NiIosBufferDesc::NEW_PARTICLE_FLAG;
			}
		}

		g_inStateToInput[idx] = newInputIdx;
	}
BOUND_KERNEL_END()


BOUND_KERNEL_BEG(STATE_WARPS_PER_BLOCK,
	testKernel,
	unsigned int scalarVar,
	unsigned int* vectorVar,
	InplaceHandle<int> multHandle)

	unsigned int idx = BlockSize*blockIdx.x + threadIdx.x;	
	if (idx < _threadCount)
	{
		int mult = *(multHandle.resolve(KERNEL_CONST_MEM(simulateConstMem)));
		vectorVar[idx] = vectorVar[idx] + scalarVar + tex1Dfetch(KERNEL_TEX_REF(HoleScanSum), _threadCount - idx - 1);
		vectorVar[idx] *= mult;
	}
BOUND_KERNEL_END()

BOUND_KERNEL_BEG(SIMULATE_WARPS_PER_BLOCK,
	gridDensityGridClearKernel,
	float* gridDensityGrid,
	GridDensityParams simParams
	)
{
	unsigned int step = BlockSize*gridDim.x;
	for (unsigned int dstIdx = BlockSize*blockIdx.x + threadIdx.x; dstIdx < _threadCount; dstIdx += step)
	{
		gridDensityGrid[dstIdx] = 0.0f;
	}
}
BOUND_KERNEL_END()

inline __device__ unsigned int gridAddress3d(unsigned int c, unsigned int b, unsigned int a, unsigned int dim)
{
	return (c*dim + b)*dim + a;
}

BOUND_KERNEL_BEG(SIMULATE_WARPS_PER_BLOCK,
	gridDensityGridFillKernel,
	float4* positionMass,
	float* gridDensityGrid,
	GridDensityParams simParams
	)
{
	unsigned int dim = simParams.GridResolution;
	unsigned int step = BlockSize*gridDim.x;
	for (unsigned int dstIdx = BlockSize*blockIdx.x + threadIdx.x; dstIdx < _threadCount; dstIdx += step)
	{
		unsigned int srcIdx = dstIdx;
		physx::PxVec3 position;
		splitFloat4(position, positionMass[srcIdx]);

		position -= simParams.DensityOrigin;
		float size = simParams.GridSize;
		position /= size;
		position.x += 0.5;
		position.y += 0.5;
		position.z += 0.5;
		// position now normalized to [0:1]
		int a = floor(dim*position.x);
		int b = floor(dim*position.y);
		int c = floor(dim*position.z);
		if( a >= 0 && a < dim &&
			b >= 0 && b < dim &&
			c >= 0 && c < dim)
		{
			//physx::PxU32 loc = c*dim*dim+b*dim+a;
			#if (__CUDA_ARCH__ >= 200)
			atomicAdd(&gridDensityGrid[gridAddress3d(c,b,a,dim)],1.0f);
			#endif
		}
	}
}
BOUND_KERNEL_END()

inline __device__ float safeGridRead(float* density,unsigned int dim,int c,int b,int a)
{
	if( a >= 0 && a < dim &&
		b >= 0 && b < dim &&
		c >= 0 && c < dim )
	{
		return density[gridAddress3d(c,b,a,dim)];
	}
	else
	{
		return 0.f;
	}
}

BOUND_KERNEL_BEG(SIMULATE_WARPS_PER_BLOCK,
	gridDensityGridApplyKernel,
	float* density,
	float4* positionMass,
	float* gridDensityGrid,
	GridDensityParams simParams
	)
{
	unsigned int dim = simParams.GridResolution;
	unsigned int step = BlockSize*gridDim.x;
	for (unsigned int dstIdx = BlockSize*blockIdx.x + threadIdx.x; dstIdx < _threadCount; dstIdx += step)
	{
		unsigned int srcIdx = dstIdx;
		physx::PxVec3 position;
		splitFloat4(position, positionMass[srcIdx]);

		position -= simParams.DensityOrigin;
		float size = simParams.GridSize;
		position /= size;
		position.x += 0.5;
		position.y += 0.5;
		position.z += 0.5;
		// position now normalized to [0:1]
		float u = dim*position.x - 0.5f;
		float v = dim*position.y - 0.5f;
		float w = dim*position.z - 0.5f;
		float uf = u - floor(u);
		float vf = v - floor(v);
		float wf = w - floor(w);
		int ui = floor(u);
		int vi = floor(v);
		int wi = floor(w);
		const float v000 = safeGridRead(gridDensityGrid,dim,wi  ,vi  ,ui  );
		const float v100 = safeGridRead(gridDensityGrid,dim,wi  ,vi  ,ui+1);
		const float v010 = safeGridRead(gridDensityGrid,dim,wi  ,vi+1,ui  );
		const float v110 = safeGridRead(gridDensityGrid,dim,wi  ,vi+1,ui+1);
		const float v001 = safeGridRead(gridDensityGrid,dim,wi+1,vi  ,ui  );
		const float v101 = safeGridRead(gridDensityGrid,dim,wi+1,vi  ,ui+1);
		const float v011 = safeGridRead(gridDensityGrid,dim,wi+1,vi+1,ui  );
		const float v111 = safeGridRead(gridDensityGrid,dim,wi+1,vi+1,ui+1);
		const float c00 = v000*(1.f-uf)+v100*uf;
		const float c10 = v010*(1.f-uf)+v110*uf;
		const float c01 = v001*(1.f-uf)+v101*uf;
		const float c11 = v011*(1.f-uf)+v111*uf;
		const float c0 = c00*(1.f-vf)+c10*vf;
		const float c1 = c01*(1.f-vf)+c11*vf;
		const float c = c0*(1.f-wf)+c1*wf;
		if(density)
		{
			density[dstIdx] = c/simParams.GridMaxCellCount;
		}
	}
}
BOUND_KERNEL_END()

// ******************** Frustum Density **********************

BOUND_KERNEL_BEG(SIMULATE_WARPS_PER_BLOCK,
	gridDensityGridFillFrustumKernel,
	float4* positionMass,
	float* gridDensityGrid,
	GridDensityParams simParams,
	::physx::PxMat44 mat,
	GridDensityFrustumParams frustum
	)
{
	unsigned int dim = simParams.GridResolution;
	unsigned int step = BlockSize*gridDim.x;
	for (unsigned int dstIdx = BlockSize*blockIdx.x + threadIdx.x; dstIdx < _threadCount; dstIdx += step)
	{
		unsigned int srcIdx = dstIdx;
		physx::PxVec3 position;
		splitFloat4(position, positionMass[srcIdx]);

		physx::PxVec4 pos(position.x,position.y,position.z,1.f);
		pos = mat.transform(pos);
		// find norm x and y
		position.x = pos.x/pos.w;
		position.y = pos.y/pos.w;
		position.z = min(sqrt(pos.z),0.999f); //pow(pos.z,frustum.gamma);
		// position now normalized to [0:1]
		int a = floor(dim*position.x);
		int b = floor(dim*position.y);
		int c = floor(dim*position.z);
		if( a >= 0 && a < dim &&
			b >= 0 && b < dim &&
			c >= 0 && c < dim)
		{
			// compute density value
			float x0 = frustum.nearDimX*(1.f-pos.z)+frustum.farDimX*(pos.z);
			float y0 = frustum.nearDimY*(1.f-pos.z)+frustum.farDimY*(pos.z);
			float z0 = frustum.dimZ;
			float k = dim*dim*dim/(x0*y0*z0);
			#if (__CUDA_ARCH__ >= 200)
			atomicAdd(&gridDensityGrid[gridAddress3d(c,b,a,dim)],k);
			#endif
		}
	}
}
BOUND_KERNEL_END()

BOUND_KERNEL_BEG(SIMULATE_WARPS_PER_BLOCK,
	gridDensityGridApplyFrustumKernel,
	float* density,
	float4* positionMass,
	float* gridDensityGrid,
	GridDensityParams simParams,
	::physx::PxMat44 mat,
	GridDensityFrustumParams frustum
	)
{
	unsigned int dim = simParams.GridResolution;
	unsigned int step = BlockSize*gridDim.x;
	for (unsigned int dstIdx = BlockSize*blockIdx.x + threadIdx.x; dstIdx < _threadCount; dstIdx += step)
	{
		unsigned int srcIdx = dstIdx;
		physx::PxVec3 position;
		splitFloat4(position, positionMass[srcIdx]);

		physx::PxVec4 pos(position.x,position.y,position.z,1.f);
		pos = mat.transform(pos);
		// find norm x and y
		position.x = pos.x/pos.w;
		position.y = pos.y/pos.w;
		position.z = min(sqrt(pos.z),0.999f); //pow(pos.z,frustum.gamma);
		// position now normalized to [0:1]
		float u = dim*position.x- 0.5f;
		float v = dim*position.y- 0.5f;
		float w = dim*position.z- 0.5f;
		float uf = u - floor(u);
		float vf = v - floor(v);
		float wf = w - floor(w);
		int ui = floor(u);
		int vi = floor(v);
		int wi = floor(w);
		const float v000 = safeGridRead(gridDensityGrid,dim,wi  ,vi  ,ui  );
		const float v100 = safeGridRead(gridDensityGrid,dim,wi  ,vi  ,ui+1);
		const float v010 = safeGridRead(gridDensityGrid,dim,wi  ,vi+1,ui  );
		const float v110 = safeGridRead(gridDensityGrid,dim,wi  ,vi+1,ui+1);
		const float v001 = safeGridRead(gridDensityGrid,dim,wi+1,vi  ,ui  );
		const float v101 = safeGridRead(gridDensityGrid,dim,wi+1,vi  ,ui+1);
		const float v011 = safeGridRead(gridDensityGrid,dim,wi+1,vi+1,ui  );
		const float v111 = safeGridRead(gridDensityGrid,dim,wi+1,vi+1,ui+1);
		const float c00 = v000*(1.f-uf)+v100*uf;
		const float c10 = v010*(1.f-uf)+v110*uf;
		const float c01 = v001*(1.f-uf)+v101*uf;
		const float c11 = v011*(1.f-uf)+v111*uf;
		const float c0 = c00*(1.f-vf)+c10*vf;
		const float c1 = c01*(1.f-vf)+c11*vf;
		const float c = c0*(1.f-wf)+c1*wf;
		if(density)
		{
			density[dstIdx] = min(max(c/simParams.GridMaxCellCount,0.f),1.1f);
		}
	}
}
BOUND_KERNEL_END()

// ************* LowPass*******************
FREE_KERNEL_BEG(SIMULATE_WARPS_PER_BLOCK,
	gridDensityGridLowPassKernel,
	float* gridDensityGridIn,
	float* gridDensityGridOut,
	GridDensityParams simParams
	)
{
	unsigned int dim = simParams.GridResolution;
	unsigned int step = BlockSize*gridDim.x;
	for (unsigned int dstIdx = BlockSize*blockIdx.x + threadIdx.x; dstIdx < _threadCount; dstIdx += step)
	{
		int a = dstIdx%dim;
		int b = (dstIdx/dim)%dim;
		int c = (dstIdx/(dim*dim))%dim;
		float value = 0.f;

		// Just a box filter for now...

		// neg z
		value += 1.0f*safeGridRead(gridDensityGridIn,dim,c-1,b-1,a-1); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c-1,b-1,a+0); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c-1,b-1,a+1);
		value += 1.0f*safeGridRead(gridDensityGridIn,dim,c-1,b+0,a-1); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c-1,b+0,a+0); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c-1,b+0,a+1);
		value += 1.0f*safeGridRead(gridDensityGridIn,dim,c-1,b+1,a-1); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c-1,b+1,a+0); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c-1,b+1,a+1);
		// zero z
		value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+0,b-1,a-1); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+0,b-1,a+0); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+0,b-1,a+1);
		value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+0,b+0,a-1); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+0,b+0,a+0); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+0,b+0,a+1);
		value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+0,b+1,a-1); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+0,b+1,a+0); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+0,b+1,a+1);
		// pos z
		value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+1,b-1,a-1); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+1,b-1,a+0); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+1,b-1,a+1);
		value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+1,b+0,a-1); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+1,b+0,a+0); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+1,b+0,a+1);
		value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+1,b+1,a-1); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+1,b+1,a+0); value += 1.0f*safeGridRead(gridDensityGridIn,dim,c+1,b+1,a+1);

		gridDensityGridOut[gridAddress3d(c,b,a,dim)] = value/27.f;
	}
}
FREE_KERNEL_END()