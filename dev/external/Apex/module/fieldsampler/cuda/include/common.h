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

#ifndef __COMMON_H__
#define __COMMON_H__

#define APEX_CUDA_MODULE_PREFIX FieldSampler_

#include "ApexCuda.h"
#include "InplaceTypes.h"

#include "../include/FieldSamplerCommon.h"

const unsigned int CLEAR_WARPS_PER_BLOCK_110 = 16;
const unsigned int CLEAR_WARPS_PER_BLOCK_120 = 16;
const unsigned int CLEAR_WARPS_PER_BLOCK_200 = 32;
const unsigned int CLEAR_WARPS_PER_BLOCK_300 = 32;

const unsigned int COMPOSE_WARPS_PER_BLOCK_110 = 12;
const unsigned int COMPOSE_WARPS_PER_BLOCK_120 = 16;
const unsigned int COMPOSE_WARPS_PER_BLOCK_200 = 20; //formerly 32
const unsigned int COMPOSE_WARPS_PER_BLOCK_300 = 20; //formerly 32

const unsigned int CLEAR_GRID_WARPS_PER_BLOCK_110 = 16;
const unsigned int CLEAR_GRID_WARPS_PER_BLOCK_120 = 16;
const unsigned int CLEAR_GRID_WARPS_PER_BLOCK_200 = 32;
const unsigned int CLEAR_GRID_WARPS_PER_BLOCK_300 = 32;

const unsigned int APPLY_PARTICLES_WARPS_PER_BLOCK_110 = 12;
const unsigned int APPLY_PARTICLES_WARPS_PER_BLOCK_120 = 16;
const unsigned int APPLY_PARTICLES_WARPS_PER_BLOCK_200 = 20;
const unsigned int APPLY_PARTICLES_WARPS_PER_BLOCK_300 = 20;

#ifdef APEX_TEST
const unsigned int TEST_PARTICLES_WARPS_PER_BLOCK_110 = 12;
const unsigned int TEST_PARTICLES_WARPS_PER_BLOCK_120 = 16;
const unsigned int TEST_PARTICLES_WARPS_PER_BLOCK_200 = 20;
const unsigned int TEST_PARTICLES_WARPS_PER_BLOCK_300 = 20;
#endif

namespace physx
{
namespace apex
{

}
} // namespace physx::apex

#endif
