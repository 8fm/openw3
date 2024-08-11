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

#define APEX_CUDA_MODULE_PREFIX IOFX_

#include "ApexCuda.h"
#include "InplaceTypes.h"
#include "NiIofxManager.h"
#include "foundation/PxBounds3.h"
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

#include "../include/ModifierData.h"

/* Reduce functions assume warps per block <= 32 !! */

//kernel specific
const unsigned int VOLUME_MIGRATION_WARPS_PER_BLOCK_110 = 12;
const unsigned int VOLUME_MIGRATION_WARPS_PER_BLOCK_120 = 16;
const unsigned int VOLUME_MIGRATION_WARPS_PER_BLOCK_200 = 32;
const unsigned int VOLUME_MIGRATION_WARPS_PER_BLOCK_300 = 32;

const unsigned int REMAP_WARPS_PER_BLOCK_110 = 16;
const unsigned int REMAP_WARPS_PER_BLOCK_120 = 16;
const unsigned int REMAP_WARPS_PER_BLOCK_200 = 32;
const unsigned int REMAP_WARPS_PER_BLOCK_300 = 32;

const unsigned int ACTOR_RANGE_WARPS_PER_BLOCK_110 = 16;
const unsigned int ACTOR_RANGE_WARPS_PER_BLOCK_120 = 16;
const unsigned int ACTOR_RANGE_WARPS_PER_BLOCK_200 = 32;
const unsigned int ACTOR_RANGE_WARPS_PER_BLOCK_300 = 32;

const unsigned int BBOX_WARPS_PER_BLOCK_110 = 10;
const unsigned int BBOX_WARPS_PER_BLOCK_120 = 14;
const unsigned int BBOX_WARPS_PER_BLOCK_200 = 32;
const unsigned int BBOX_WARPS_PER_BLOCK_300 = 32;

const unsigned int SORT_WARPS_PER_BLOCK_110 = 12;
const unsigned int SORT_WARPS_PER_BLOCK_120 = 14;
const unsigned int SORT_WARPS_PER_BLOCK_200 = 32;
const unsigned int SORT_WARPS_PER_BLOCK_300 = 32;

const unsigned int SORT_NEW_WARPS_PER_BLOCK_110 = 8;
const unsigned int SORT_NEW_WARPS_PER_BLOCK_120 = 8;
const unsigned int SORT_NEW_WARPS_PER_BLOCK_200 = 16;
const unsigned int SORT_NEW_WARPS_PER_BLOCK_300 = 16;
const unsigned int SORT_NEW_VECTOR_SIZE = 4;

const unsigned int SPRITE_MODIFIER_WARPS_PER_BLOCK_110 = 4;
const unsigned int SPRITE_MODIFIER_WARPS_PER_BLOCK_120 = 6;
const unsigned int SPRITE_MODIFIER_WARPS_PER_BLOCK_200 = 16;
const unsigned int SPRITE_MODIFIER_WARPS_PER_BLOCK_300 = 16;

/* TODO: check against this value, now that user could set arbitrary offset/stride for VBO layout */
const unsigned int SPRITE_MAX_DWORDS_PER_OUTPUT = 18;

const unsigned int MESH_MODIFIER_WARPS_PER_BLOCK_110 = 4;
const unsigned int MESH_MODIFIER_WARPS_PER_BLOCK_120 = 4;
const unsigned int MESH_MODIFIER_WARPS_PER_BLOCK_200 = 16;
const unsigned int MESH_MODIFIER_WARPS_PER_BLOCK_300 = 16;

const unsigned int MESH_MAX_DWORDS_PER_OUTPUT = 22;

//const unsigned int MAX_CONST_MEM_SIZE = (60*1024);//65536;

const unsigned int MAX_CURVE_COUNT = 256;
const unsigned int CURVE_SAMPLE_COUNT = 256;

const unsigned int RADIX_SORT_NBITS = 4;

const unsigned int STATE_ID_MASK = 0x7FFFFFFFu;
const unsigned int STATE_ID_DIST_SIGN = 0x80000000u;


namespace physx
{
namespace apex
{
namespace iofx
{

#ifdef __CUDACC__

PX_CUDA_CALLABLE PX_INLINE IofxSlice uint4_to_IofxSlice(uint4 v)
{
	IofxSlice ret;
	ret.x = v.x;
	ret.y = v.y;
	ret.z = v.z;
	ret.w = v.w;
	return ret;
}

PX_CUDA_CALLABLE PX_INLINE uint4 IofxSlice_to_uint4(IofxSlice s)
{
	return make_uint4(s.x, s.y, s.z, s.w);
}
#endif


struct VolumeParams
{
	physx::PxBounds3	bounds;
	physx::PxU32		priority;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(bounds);
		r.reflect(priority);
	}
#endif
};

typedef InplaceArray<VolumeParams> VolumeParamsArray;

typedef InplaceArray<physx::PxU32> ActorIDBitmapArray;


struct ModifierListElem
{
	unsigned int type;
	InplaceHandleBase paramsHandle;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(type);
		r.reflect(paramsHandle);
	}
#endif
};

typedef InplaceArray<ModifierListElem> ModifierList;

struct AssetParams
{
	ModifierList spawnModifierList;
	ModifierList continuousModifierList;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(spawnModifierList);
		r.reflect(continuousModifierList);
	}
#endif
};

typedef InplaceArray< InplaceHandle<AssetParams> > AssetParamsHandleArray;


struct SpritePrivateStateArgs
{
	IofxSlice* g_state[1];

#ifdef __CUDACC__
	static __device__ void read(const SpritePrivateStateArgs& args, SpritePrivateState& state, unsigned int pos);
	static __device__ void write(SpritePrivateStateArgs& args, const SpritePrivateState& state, unsigned int pos);
#endif
};

struct MeshPrivateStateArgs
{
	IofxSlice* g_state[3];

#ifdef __CUDACC__
	static __device__ void read(const MeshPrivateStateArgs& args, MeshPrivateState& state, unsigned int pos);
	static __device__ void write(MeshPrivateStateArgs& args, const MeshPrivateState& state, unsigned int pos);
#endif
};

}
}
} // namespace apex

#endif
