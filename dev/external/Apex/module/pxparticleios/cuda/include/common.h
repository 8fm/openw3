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

#define APEX_CUDA_MODULE_PREFIX ParticleIOS_

#include "ApexCuda.h"
#include "InplaceTypes.h"
#include "NiIofxManager.h"
#include <float.h>

#if defined(PX_WINDOWS)
#pragma warning(push)
#pragma warning(disable:4201)
#pragma warning(disable:4408)
#endif

#include <vector_types.h>

#if defined(PX_WINDOWS)
#pragma warning(pop)
#endif

const unsigned int REDUCE_WARPS_PER_BLOCK_110 = 16;
const unsigned int REDUCE_WARPS_PER_BLOCK_120 = 16;
const unsigned int REDUCE_WARPS_PER_BLOCK_200 = 32;
const unsigned int REDUCE_WARPS_PER_BLOCK_300 = 32;

const unsigned int HISTOGRAM_WARPS_PER_BLOCK_110 = 12;
const unsigned int HISTOGRAM_WARPS_PER_BLOCK_120 = 12;
const unsigned int HISTOGRAM_WARPS_PER_BLOCK_200 = 32;
const unsigned int HISTOGRAM_WARPS_PER_BLOCK_300 = 32;
const unsigned int HISTOGRAM_BIN_COUNT = 256;
const unsigned int HISTOGRAM_SIMULATE_BIN_COUNT = 320;	

const unsigned int SCAN_WARPS_PER_BLOCK_110 = 14;
const unsigned int SCAN_WARPS_PER_BLOCK_120 = 16;
const unsigned int SCAN_WARPS_PER_BLOCK_200 = 32;
const unsigned int SCAN_WARPS_PER_BLOCK_300 = 32;

const unsigned int COMPACT_WARPS_PER_BLOCK_110 = 16;
const unsigned int COMPACT_WARPS_PER_BLOCK_120 = 16;
const unsigned int COMPACT_WARPS_PER_BLOCK_200 = 32;
const unsigned int COMPACT_WARPS_PER_BLOCK_300 = 32;

const unsigned int BENEFIT_WARPS_PER_BLOCK_110 = 14;
const unsigned int BENEFIT_WARPS_PER_BLOCK_120 = 16;
const unsigned int BENEFIT_WARPS_PER_BLOCK_200 = 32;
const unsigned int BENEFIT_WARPS_PER_BLOCK_300 = 32;

const unsigned int SIMULATE_WARPS_PER_BLOCK_110 = 6;
const unsigned int SIMULATE_WARPS_PER_BLOCK_120 = 12;
const unsigned int SIMULATE_WARPS_PER_BLOCK_200 = 16;
const unsigned int SIMULATE_WARPS_PER_BLOCK_300 = 16;

const unsigned int STATE_WARPS_PER_BLOCK_110 = 16;
const unsigned int STATE_WARPS_PER_BLOCK_120 = 16;
const unsigned int STATE_WARPS_PER_BLOCK_200 = 32;
const unsigned int STATE_WARPS_PER_BLOCK_300 = 32;

const unsigned int HOLE_SCAN_FLAG_BIT = 31;
const unsigned int HOLE_SCAN_FLAG = (1U << HOLE_SCAN_FLAG_BIT);
const unsigned int HOLE_SCAN_MASK = (HOLE_SCAN_FLAG - 1);

// mTmpOutput
const unsigned int STATUS_LAST_ACTIVE_COUNT		= 0;
const unsigned int STATUS_LAST_BENEFIT_SUM		= 1;
const unsigned int STATUS_LAST_BENEFIT_MIN		= 2;
const unsigned int STATUS_LAST_BENEFIT_MAX		= 3;
const unsigned int STATUS_PARTICLE_MAX_INDEX	= 4;

namespace physx
{
namespace apex
{
namespace pxparticleios
{

}
}
} // namespace physx::apex

#endif
