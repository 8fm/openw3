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

#ifndef __COMMON_CUH__
#define __COMMON_CUH__

#include <PsShare.h>
#include <GPUProfile.h>


template <typename T>
inline __device__ void _forceParamRef(T var) { ; }

#define FREE_KERNEL_BEG(kernelWarps, kernelName, ...) \
extern "C" __global__ void APEX_CUDA_NAME(kernelName)(int* _extMem, physx::PxU16 _kernelEnum, physx::PxU32 _threadCount, __VA_ARGS__ ) \
{ \
	const unsigned int WarpsPerBlock = GET_WARPS_PER_BLOCK(kernelWarps); \
	const unsigned int BlockSize = (WarpsPerBlock << LOG2_WARP_SIZE); \
	_forceParamRef(BlockSize); \
	KERNEL_START_EVENT(_extMem, _kernelEnum) \

#define FREE_KERNEL_END() \
	KERNEL_STOP_EVENT(_extMem, _kernelEnum) \
} \

#define FREE_KERNEL_2D_BEG(kernelDim, kernelName, ...) \
extern "C" __global__ void APEX_CUDA_NAME(kernelName)(int* _extMem, physx::PxU16 _kernelEnum, physx::PxU32 _threadCountX, physx::PxU32 _threadCountY, __VA_ARGS__ ) \
{ \
	KERNEL_START_EVENT(_extMem, _kernelEnum) \

#define FREE_KERNEL_2D_END() \
	KERNEL_STOP_EVENT(_extMem, _kernelEnum) \
} \


#define FREE_KERNEL_3D_BEG(kernelDim, kernelName, ...) \
extern "C" __global__ void APEX_CUDA_NAME(kernelName)(int* _extMem, physx::PxU16 _kernelEnum, physx::PxU32 _threadCountX, physx::PxU32 _threadCountY, physx::PxU32 _threadCountZ, physx::PxU32 _blockCountY, __VA_ARGS__ ) \
{ \
	const unsigned int blockIdxZ = blockIdx.y / _blockCountY; \
	const unsigned int blockIdxY = blockIdx.y % _blockCountY; \
	const unsigned int blockIdxX = blockIdx.x; \
	const unsigned int idxX = blockIdxX * blockDim.x + threadIdx.x; \
	const unsigned int idxY = blockIdxY * blockDim.y + threadIdx.y; \
	const unsigned int idxZ = blockIdxZ * blockDim.z + threadIdx.z; \
	KERNEL_START_EVENT(_extMem, _kernelEnum) \

#define FREE_KERNEL_3D_END() \
	KERNEL_STOP_EVENT(_extMem, _kernelEnum) \
} \

#define BOUND_KERNEL_BEG(kernelWarps, kernelName, ...) \
extern "C" __global__ void APEX_CUDA_NAME(kernelName)(int* _extMem, physx::PxU16 _kernelEnum, physx::PxU32 _threadCount, __VA_ARGS__ ) \
{ \
	const unsigned int WarpsPerBlock = GET_WARPS_PER_BLOCK(kernelWarps); \
	const unsigned int BlockSize = (WarpsPerBlock << LOG2_WARP_SIZE); \
	_forceParamRef(BlockSize); \
	KERNEL_START_EVENT(_extMem, _kernelEnum) \

#define BOUND_KERNEL_END() \
	KERNEL_STOP_EVENT(_extMem, _kernelEnum) \
} \

#define SYNC_KERNEL_BEG(kernelWarps, kernelName, ...) \
extern "C" __global__ void APEX_CUDA_NAME(kernelName)(int* _extMem, physx::PxU16 _kernelEnum, __VA_ARGS__ ) \
{ \
	const unsigned int WarpsPerBlock = GET_WARPS_PER_BLOCK(kernelWarps); \
	const unsigned int BlockSize = (WarpsPerBlock << LOG2_WARP_SIZE); \
	_forceParamRef(BlockSize); \
	KERNEL_START_EVENT(_extMem, _kernelEnum) \

#define SYNC_KERNEL_END() \
	KERNEL_STOP_EVENT(_extMem, _kernelEnum) \
} \


#define KERNEL_TEX_REF(name) APEX_CUDA_NAME( APEX_CUDA_CONCAT(texRef, name) )

#define KERNEL_CONST_MEM(name) APEX_CUDA_NAME(name)

const unsigned int LOG2_WARP_SIZE = 5;
const unsigned int WARP_SIZE = (1U << LOG2_WARP_SIZE);

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 200
const unsigned int LOG2_NUM_BANKS = 5;
#else
const unsigned int LOG2_NUM_BANKS = 4;
#endif

const unsigned int NUM_BANKS = (1U << LOG2_NUM_BANKS);

const unsigned int MULTIPROCESSOR_MAX_COUNT = 32;



#ifdef __CUDA_ARCH__
#define GET_WARPS_PER_BLOCK(name) APEX_CUDA_CONCAT(APEX_CUDA_CONCAT(name, _), __CUDA_ARCH__)
#else
#define GET_WARPS_PER_BLOCK(name) name ## _ ## 110
#endif


#endif //__COMMON_CUH__
