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

APEX_CUDA_BOUND_KERNEL(CLEAR_WARPS_PER_BLOCK, clearKernel,
                       ((float4*, g_accumField))((float4*, g_accumVelocity))
                      )

APEX_CUDA_BOUND_KERNEL(COMPOSE_WARPS_PER_BLOCK, composeKernel,
                       ((float4*, g_accumField))((const float4*, g_accumVelocity))((const float4*, g_velocity))((physx::PxF32, timestep))
                      )

APEX_CUDA_BOUND_KERNEL(CLEAR_GRID_WARPS_PER_BLOCK, clearGridKernel,
                       ((physx::PxU32, numX))((physx::PxU32, numY))((physx::PxU32, numZ))
                       ((physx::PxU32, strideX))((physx::PxU32, strideY))
                       ((float4*, g_accumVelocity))
                      )

APEX_CUDA_BOUND_KERNEL(APPLY_PARTICLES_WARPS_PER_BLOCK, applyParticlesKernel,
                       ((float4*, g_velocity))((const float4*, g_outField))
                      )

#ifdef APEX_TEST

APEX_CUDA_BOUND_KERNEL(TEST_PARTICLES_WARPS_PER_BLOCK, testParticleKernel,
                       ((float4*, g_position))((float4*, g_velocity))
					   ((physx::PxU32*, g_flag))
					   ((const float4*, g_initPosition))((const float4*, g_initVelocity))
                      )

#endif