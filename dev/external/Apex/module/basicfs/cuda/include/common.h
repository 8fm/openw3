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

#define APEX_CUDA_MODULE_PREFIX BasicFS_

#include "ApexCuda.h"
#include "../include/JetFSCommon.h"
#include "../include/AttractorFSCommon.h"
#include "../include/VortexFSCommon.h"
#include "../include/NoiseFSCommon.h"

const unsigned int FIELD_SAMPLER_POINTS_WARPS_PER_BLOCK_110 = 4;
const unsigned int FIELD_SAMPLER_POINTS_WARPS_PER_BLOCK_120 = 4;
const unsigned int FIELD_SAMPLER_POINTS_WARPS_PER_BLOCK_200 = 16;
const unsigned int FIELD_SAMPLER_POINTS_WARPS_PER_BLOCK_300 = 32;

const unsigned int FIELD_SAMPLER_GRID_WARPS_PER_BLOCK_110 = 4;
const unsigned int FIELD_SAMPLER_GRID_WARPS_PER_BLOCK_120 = 6;
const unsigned int FIELD_SAMPLER_GRID_WARPS_PER_BLOCK_200 = 16;
const unsigned int FIELD_SAMPLER_GRID_WARPS_PER_BLOCK_300 = 32;

namespace physx
{
namespace apex
{
namespace basicfs
{


}
}
} // namespace physx::apex

#endif
