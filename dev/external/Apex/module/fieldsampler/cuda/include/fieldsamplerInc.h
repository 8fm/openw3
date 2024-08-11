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

APEX_CUDA_CONST_MEM(fieldsamplerConstMem, MAX_CONST_MEM_SIZE)

#ifdef FIELD_SAMPLER_SEPARATE_KERNELS

APEX_CUDA_BOUND_KERNEL(FIELD_SAMPLER_POINTS_WARPS_PER_BLOCK, fieldSamplerPointsKernel,
					   ((fieldsampler::FieldSamplerPointsKernelArgs, args))
                       ((InplaceHandle<fieldsampler::FieldSamplerParams>, paramsHandle))
                       ((InplaceHandle<fieldsampler::FieldSamplerQueryParams>, queryParamsHandle))
					   ((fieldsampler::FieldSamplerKernelMode::Enum, kernelMode))
                      )
APEX_CUDA_BOUND_KERNEL(FIELD_SAMPLER_GRID_WARPS_PER_BLOCK, fieldSamplerGridKernel,
                       ((fieldsampler::FieldSamplerGridKernelArgs, args))
                       ((InplaceHandle<fieldsampler::FieldSamplerParams>, paramsHandle))
                       ((InplaceHandle<fieldsampler::FieldSamplerQueryParams>, queryParamsHandle))
					   ((fieldsampler::FieldSamplerKernelMode::Enum, kernelMode))
                      )

#ifndef __CUDACC__
#define LAUNCH_FIELD_SAMPLER_KERNEL( data ) \
	ApexCudaConstMem& _storage_ = SCENE_CUDA_OBJ(this, fieldsamplerConstMem); \
	InplaceHandle<fieldsampler::FieldSamplerQueryParams> queryParamsHandle = _storage_.mappedHandle( data.queryParamsHandle ); \
	PxU32 fieldSamplerCount = data.fieldSamplerArray->size(); \
	switch( data.kernelType ) \
	{ \
	case fieldsampler::FieldSamplerKernelType::POINTS: \
		for (PxU32 i = 0, activeIdx = 0; i < fieldSamplerCount; ++i) \
		{ \
			const fieldsampler::FieldSamplerWrapperGPU* wrapper = static_cast<const fieldsampler::FieldSamplerWrapperGPU* >( (*data.fieldSamplerArray)[i] ); \
			if (wrapper->isEnabled()) \
			{ \
				fieldsampler::FieldSamplerKernelMode::Enum kernelMode = (++activeIdx == data.activeFieldSamplerCount) ? data.kernelMode : fieldsampler::FieldSamplerKernelMode::DEFAULT; \
				InplaceHandle<fieldsampler::FieldSamplerParams> paramsHandle = _storage_.mappedHandle( wrapper->getParamsHandle() ); \
				ON_LAUNCH_FIELD_SAMPLER_KERNEL( wrapper->getNiFieldSampler(), wrapper->getNiFieldSamplerDesc() ); \
				SCENE_CUDA_OBJ(this, fieldSamplerPointsKernel)( data.stream, data.threadCount, *static_cast<const fieldsampler::FieldSamplerPointsKernelArgs*>(data.kernelArgs), paramsHandle, queryParamsHandle, kernelMode ); \
			} \
		} \
		return true; \
	case fieldsampler::FieldSamplerKernelType::GRID: \
		for (PxU32 i = 0, activeIdx = 0; i < fieldSamplerCount; ++i) \
		{ \
			const fieldsampler::FieldSamplerWrapperGPU* wrapper = static_cast<const fieldsampler::FieldSamplerWrapperGPU* >( (*data.fieldSamplerArray)[i] ); \
			if (wrapper->isEnabled() && wrapper->getNiFieldSamplerDesc().gridSupportType == NiFieldSamplerGridSupportType::VELOCITY_PER_CELL) \
			{ \
				fieldsampler::FieldSamplerKernelMode::Enum kernelMode = (++activeIdx == data.activeFieldSamplerCount) ? data.kernelMode : fieldsampler::FieldSamplerKernelMode::DEFAULT; \
				InplaceHandle<fieldsampler::FieldSamplerParams> paramsHandle = _storage_.mappedHandle( wrapper->getParamsHandle() ); \
				ON_LAUNCH_FIELD_SAMPLER_KERNEL( wrapper->getNiFieldSampler(), wrapper->getNiFieldSamplerDesc() ); \
				SCENE_CUDA_OBJ(this, fieldSamplerGridKernel)( data.stream, data.threadCount, *static_cast<const fieldsampler::FieldSamplerGridKernelArgs*>(data.kernelArgs), paramsHandle, queryParamsHandle, kernelMode ); \
			} \
		} \
		return true; \
	default: \
		PX_ALWAYS_ASSERT(); \
		return false; \
	};
#endif

#else

APEX_CUDA_BOUND_KERNEL(FIELD_SAMPLER_POINTS_WARPS_PER_BLOCK, fieldSamplerPointsKernel,
                       ((fieldsampler::FieldSamplerPointsKernelArgs, args))
                       ((InplaceHandle<physx::apex::fieldsampler::FieldSamplerParamsHandleArray>, paramsHandleArrayHandle))
                       ((InplaceHandle<physx::apex::fieldsampler::FieldSamplerQueryParams>, queryParamsHandle))
					   ((fieldsampler::FieldSamplerKernelMode::Enum, kernelMode))
                      )
APEX_CUDA_BOUND_KERNEL(FIELD_SAMPLER_GRID_WARPS_PER_BLOCK, fieldSamplerGridKernel,
                       ((fieldsampler::FieldSamplerGridKernelArgs, args))
                       ((InplaceHandle<physx::apex::fieldsampler::FieldSamplerParamsHandleArray>, paramsHandleArrayHandle))
                       ((InplaceHandle<physx::apex::fieldsampler::FieldSamplerQueryParams>, queryParamsHandle))
					   ((fieldsampler::FieldSamplerKernelMode::Enum, kernelMode))
                      )

#ifndef __CUDACC__
#define LAUNCH_FIELD_SAMPLER_KERNEL( data ) \
	ApexCudaConstMem& _storage_ = SCENE_CUDA_OBJ(this, fieldsamplerConstMem); \
	InplaceHandle<fieldsampler::FieldSamplerParamsHandleArray> paramsHandleArrayHandle = _storage_.mappedHandle( data.paramsHandleArrayHandle ); \
	InplaceHandle<fieldsampler::FieldSamplerQueryParams> queryParamsHandle = _storage_.mappedHandle( data.queryParamsHandle ); \
	switch( data.kernelType ) \
	{ \
	case fieldsampler::FieldSamplerKernelType::POINTS: \
		SCENE_CUDA_OBJ(this, fieldSamplerPointsKernel)( data.stream, data.threadCount, *static_cast<const fieldsampler::FieldSamplerPointsKernelArgs*>(data.kernelArgs), paramsHandleArrayHandle, queryParamsHandle, data.kernelMode ); \
		return true; \
	case fieldsampler::FieldSamplerKernelType::GRID: \
		SCENE_CUDA_OBJ(this, fieldSamplerGridKernel)( data.stream, data.threadCount, *static_cast<const fieldsampler::FieldSamplerGridKernelArgs*>(data.kernelArgs), paramsHandleArrayHandle, queryParamsHandle, data.kernelMode ); \
		return true; \
	default: \
		PX_ALWAYS_ASSERT(); \
		return false; \
	};
#endif

#endif //FIELD_SAMPLER_SEPARATE_KERNELS
