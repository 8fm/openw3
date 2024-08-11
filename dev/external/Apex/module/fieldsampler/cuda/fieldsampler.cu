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
#include "include/fieldsampler.h"
#include "../include/FieldSamplerCommon.h"


BOUND_KERNEL_BEG(CLEAR_WARPS_PER_BLOCK, clearKernel,
	float4* g_accumField, float4* g_accumVelocity
)
	for (unsigned int idx = BlockSize*blockIdx.x + threadIdx.x; idx < _threadCount; idx += BlockSize*gridDim.x)
	{
		g_accumField[idx] = make_float4(0, 0, 0, 0);
		g_accumVelocity[idx] = make_float4(0, 0, 0, 0);
	}
BOUND_KERNEL_END()


BOUND_KERNEL_BEG(COMPOSE_WARPS_PER_BLOCK, composeKernel,
	float4* g_accumField, const float4* g_accumVelocity, const float4* g_velocity, physx::PxF32 timestep
)
	for (unsigned int idx = BlockSize*blockIdx.x + threadIdx.x; idx < _threadCount; idx += BlockSize*gridDim.x)
	{
		float4 avel4 = g_accumVelocity[idx];
		physx::PxVec3 avel(avel4.x, avel4.y, avel4.z);
		physx::PxF32 avelW = avel4.w;

		if (avelW >= VELOCITY_WEIGHT_THRESHOLD)
		{
			float4 vel4 = g_velocity[idx];
			physx::PxVec3 vel(vel4.x, vel4.y, vel4.z);

			float4 field4 = g_accumField[idx];
			physx::PxVec3 field(field4.x, field4.y, field4.z);

			field += (avel - avelW * vel);

			g_accumField[idx] = make_float4(field.x, field.y, field.z, 0);
		}
	}
BOUND_KERNEL_END()


BOUND_KERNEL_BEG(CLEAR_GRID_WARPS_PER_BLOCK, clearGridKernel,
	physx::PxU32 numX, physx::PxU32 numY, physx::PxU32 numZ,
	physx::PxU32 strideX, physx::PxU32 strideY,
	float4* g_accumVelocity
)
	for (unsigned int ithread = BlockSize*blockIdx.x + threadIdx.x; ithread < _threadCount; ithread += BlockSize*gridDim.x)
	{
		int ixy = (ithread / strideY);
		int iz  = (ithread % strideY);
		int iy  = (ixy % numY);
		int ix  = (ixy / numY);

		int idx = (ix * strideX) + (iy * strideY) + iz;

		if (ix < numX && iy < numY && iz < numZ)
		{
			g_accumVelocity[idx] = make_float4(0, 0, 0, 0);
		}
	}
BOUND_KERNEL_END()

BOUND_KERNEL_BEG(APPLY_PARTICLES_WARPS_PER_BLOCK, applyParticlesKernel,
	float4* g_velocity, const float4* g_outField
)
	for (unsigned int idx = BlockSize*blockIdx.x + threadIdx.x; idx < _threadCount; idx += BlockSize*gridDim.x)
	{
		g_velocity[idx].x += g_outField[idx].x;
		g_velocity[idx].y += g_outField[idx].y;
		g_velocity[idx].z += g_outField[idx].z;
	}
BOUND_KERNEL_END()

#ifdef APEX_TEST

BOUND_KERNEL_BEG(TEST_PARTICLES_WARPS_PER_BLOCK, testParticleKernel,
	float4* g_position, float4* g_velocity,
	physx::PxU32* g_flag,
	const float4* g_initPosition, const float4* g_initVelocity
)
	for (unsigned int idx = BlockSize*blockIdx.x + threadIdx.x; idx < _threadCount; idx += BlockSize*gridDim.x)
	{
		
		testParticle((physx::PxVec4&)g_position[idx], (physx::PxVec4&)g_velocity[idx], g_flag[idx], (physx::PxVec4&)g_initPosition[idx], (physx::PxVec4&)g_initVelocity[idx]);
	}
BOUND_KERNEL_END()

#endif