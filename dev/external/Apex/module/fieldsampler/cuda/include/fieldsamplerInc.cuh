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

inline __device__ void iterateShapeGroup(InplaceHandle<physx::apex::fieldsampler::FieldShapeGroupParams> shapeGroupHandle, const physx::PxVec3& position, physx::PxF32& weight)
{
	const physx::apex::fieldsampler::FieldShapeGroupParams* shapeGroupParams = shapeGroupHandle.resolve(KERNEL_CONST_MEM(fieldsamplerConstMem));

	physx::PxU32 shapeCount = shapeGroupParams->shapeArray.getSize();
	const physx::apex::fieldsampler::FieldShapeParams* shapeElems = shapeGroupParams->shapeArray.getElems(KERNEL_CONST_MEM(fieldsamplerConstMem));
	for (physx::PxU32 shapeIndex = 0; shapeIndex < shapeCount; ++shapeIndex)
	{
		const physx::apex::fieldsampler::FieldShapeParams& shapeParams = shapeElems[shapeIndex];
		const physx::PxF32 shapeWeight = physx::apex::fieldsampler::evalWeightInShape(shapeParams, position);
		weight = physx::PxMax(weight, shapeWeight);
	}
}

template <int queryType>
inline __device__ void fieldSamplerFunc(
	fieldsampler::FieldSamplerKernelArgs                  args,
	InplaceHandle<fieldsampler::FieldSamplerParams>       paramsHandle,
	InplaceHandle<fieldsampler::FieldSamplerQueryParams>  queryParamsHandle,
	const physx::PxVec3&                    position,
	const physx::PxVec3&                    velocity,
	physx::PxF32                            mass,
	physx::PxVec4&                          accumAccel,
	physx::PxVec4&                          accumVelocity)
{
	const fieldsampler::FieldSamplerParams* samplerParams = paramsHandle.resolve( KERNEL_CONST_MEM(fieldsamplerConstMem) );
	const fieldsampler::FieldSamplerQueryParams* queryParams = queryParamsHandle.resolve( KERNEL_CONST_MEM(fieldsamplerConstMem) );

	physx::PxF32 excludeWeight = 0;

	physx::PxU32 shapeGroupCount = samplerParams->excludeShapeGroupHandleArray.getSize();
	const InplaceHandle<physx::apex::fieldsampler::FieldShapeGroupParams>* shapeGroupElems = samplerParams->excludeShapeGroupHandleArray.getElems(KERNEL_CONST_MEM(fieldsamplerConstMem));
	for (physx::PxU32 shapeGroupIndex = 0; shapeGroupIndex < shapeGroupCount; ++shapeGroupIndex)
	{
		iterateShapeGroup( shapeGroupElems[shapeGroupIndex], position, excludeWeight );
	}

	physx::PxF32 includeWeight = evalFieldSamplerIncludeWeight<queryType>( samplerParams, position, args.cellRadius);
	physx::PxF32 weight = includeWeight * (1.0f - excludeWeight);

	//execute field
	physx::apex::fieldsampler::FieldSamplerExecuteArgs execArgs;
	execArgs.position = position;
	execArgs.velocity = velocity;
	execArgs.mass = mass;

	execArgs.elapsedTime = args.elapsedTime;
	execArgs.totalElapsedMS = args.totalElapsedMS;

	physx::PxF32 fieldWeight = 1;
	physx::PxVec3 fieldValue = executeFieldSampler<queryType>( samplerParams, execArgs, fieldWeight );
	//override field weight
	fieldWeight = weight;

	//accum field
	switch (samplerParams->type)
	{
	case NiFieldSamplerType::FORCE:
		accumFORCE(execArgs, fieldValue, fieldWeight, accumAccel, accumVelocity);
		break;
	case NiFieldSamplerType::ACCELERATION:
		accumACCELERATION(execArgs, fieldValue, fieldWeight, accumAccel, accumVelocity);
		break;
	case NiFieldSamplerType::VELOCITY_DRAG:
		accumVELOCITY_DRAG(execArgs, samplerParams->dragCoeff, fieldValue, fieldWeight, accumAccel, accumVelocity);
		break;
	case NiFieldSamplerType::VELOCITY_DIRECT:
		accumVELOCITY_DIRECT(execArgs, fieldValue, fieldWeight, accumAccel, accumVelocity);
		break;
	};
}


template <int queryType>
inline __device__ bool isValidFieldSampler(InplaceHandle<physx::apex::fieldsampler::FieldSamplerParams> paramsHandle);


template <>
inline __device__ bool isValidFieldSampler<physx::apex::fieldsampler::FieldSamplerKernelType::POINTS>(InplaceHandle<physx::apex::fieldsampler::FieldSamplerParams> paramsHandle)
{
	return true;
}

template <>
inline __device__ bool isValidFieldSampler<physx::apex::fieldsampler::FieldSamplerKernelType::GRID>(InplaceHandle<physx::apex::fieldsampler::FieldSamplerParams> paramsHandle)
{
	const physx::apex::fieldsampler::FieldSamplerParams* samplerParams = paramsHandle.resolve( KERNEL_CONST_MEM(fieldsamplerConstMem) );
	return (samplerParams->gridSupportType == NiFieldSamplerGridSupportType::VELOCITY_PER_CELL);
}

template <int queryType>
inline __device__ void fieldSamplerFunc(
	physx::apex::fieldsampler::FieldSamplerKernelArgs                          args,
	InplaceHandle<physx::apex::fieldsampler::FieldSamplerParamsHandleArray>    paramsHandleArrayHandle,
	InplaceHandle<physx::apex::fieldsampler::FieldSamplerQueryParams>          queryParamsHandle,
	const physx::PxVec3&                            position,
	const physx::PxVec3&                            velocity,
	physx::PxF32                                    mass,
	physx::PxVec4&                                  accumAccel,
	physx::PxVec4&                                  accumVelocity)
{
	const physx::apex::fieldsampler::FieldSamplerParamsHandleArray* paramsHandleArray = paramsHandleArrayHandle.resolve( KERNEL_CONST_MEM(fieldsamplerConstMem) );
	const InplaceHandle<physx::apex::fieldsampler::FieldSamplerParams>* paramsHandles = paramsHandleArray->getElems( KERNEL_CONST_MEM(fieldsamplerConstMem) );
	for (physx::PxU32 i = 0; i < paramsHandleArray->getSize(); ++i)
	{
		InplaceHandle<physx::apex::fieldsampler::FieldSamplerParams> paramsHandle = paramsHandles[i];
		if (isValidFieldSampler<queryType>(paramsHandle))
		{
			fieldSamplerFunc<queryType>(args, paramsHandle, queryParamsHandle, position, velocity, mass,
				accumAccel, accumVelocity);
		}
	}
}

template <unsigned int BlockSize, typename T>
inline __device__ void fieldSamplerPointsFunc(
	unsigned int                            count,
	physx::apex::fieldsampler::FieldSamplerPointsKernelArgs            args,
	InplaceHandle<T>                        handle,
	InplaceHandle<physx::apex::fieldsampler::FieldSamplerQueryParams>  queryParamsHandle,
	physx::apex::fieldsampler::FieldSamplerKernelMode::Enum            kernelMode)
{
	for (unsigned int idx = BlockSize*blockIdx.x + threadIdx.x; idx < count; idx += BlockSize*gridDim.x)
	{
		float4 pos4 = args.positionMass[idx];
		float4 vel4 = args.velocity[idx];

		physx::PxVec3 position(pos4.x, pos4.y, pos4.z);
		physx::PxVec3 velocity(vel4.x, vel4.y, vel4.z);
		physx::PxF32  mass = pos4.w;

		const float4 field4 = args.accumField[idx];
		physx::PxVec4 accumField = physx::PxVec4(field4.x, field4.y, field4.z, field4.w);

		const float4 avel4 = args.accumVelocity[idx];
		physx::PxVec4 accumVelocity = physx::PxVec4(avel4.x, avel4.y, avel4.z, avel4.w);

		fieldSamplerFunc<physx::apex::fieldsampler::FieldSamplerKernelType::POINTS>(args, handle, queryParamsHandle, position, velocity, mass,
			accumField, accumVelocity);

		switch (kernelMode)
		{
		case physx::apex::fieldsampler::FieldSamplerKernelMode::FINISH_PRIMARY:
			accumField.w = accumVelocity.w;
			accumVelocity.w = 0;
			break;
		case physx::apex::fieldsampler::FieldSamplerKernelMode::FINISH_SECONDARY:
			accumVelocity.w = accumField.w + accumVelocity.w * (1 - accumField.w);
			accumField.w = 0;
			break;
		default:
			break;
		};

		args.accumField[idx] = make_float4(accumField.x, accumField.y, accumField.z, accumField.w);
		args.accumVelocity[idx] = make_float4(accumVelocity.x, accumVelocity.y, accumVelocity.z, accumVelocity.w);
	}
}

template <unsigned int BlockSize, typename T>
inline __device__ void fieldSamplerGridFunc(
	unsigned int                            count,
	physx::apex::fieldsampler::FieldSamplerGridKernelArgs              args,
	InplaceHandle<T>                        handle,
	InplaceHandle<physx::apex::fieldsampler::FieldSamplerQueryParams>  queryParamsHandle,
	physx::apex::fieldsampler::FieldSamplerKernelMode::Enum            kernelMode)
{
	for (unsigned int ithread = BlockSize*blockIdx.x + threadIdx.x; ithread < count; ithread += BlockSize*gridDim.x)
	{
		int ixy = (ithread / args.strideY);
		int iz  = (ithread % args.strideY);
		int iy  = (ixy % args.numY);
		int ix  = (ixy / args.numY);

		int idx = (ix * args.strideX) + (iy * args.strideY) + iz;

		if (ix < args.numX && iy < args.numY && iz < args.numZ)
		{
			physx::PxVec3 gridPos(ix, iy, iz);
			physx::PxVec3 position = args.gridToWorld * gridPos;
			physx::PxVec3 velocity(0, 0, 0);
			physx::PxF32  mass = args.mass;

			physx::PxVec4 accumField(0, 0, 0, 0);

			const float4 avel4 = args.accumVelocity[idx];
			physx::PxVec4 accumVelocity = physx::PxVec4(avel4.x, avel4.y, avel4.z, avel4.w);

			fieldSamplerFunc<physx::apex::fieldsampler::FieldSamplerKernelType::GRID>(args, handle, queryParamsHandle, position, velocity, mass,
				accumField, accumVelocity);

			args.accumVelocity[idx] = make_float4(accumVelocity.x, accumVelocity.y, accumVelocity.z, accumVelocity.w);
		}
	}
}

#ifdef FIELD_SAMPLER_SEPARATE_KERNELS

BOUND_KERNEL_BEG(FIELD_SAMPLER_POINTS_WARPS_PER_BLOCK, fieldSamplerPointsKernel,
	fieldsampler::FieldSamplerPointsKernelArgs            args,
	InplaceHandle<fieldsampler::FieldSamplerParams>       paramsHandle,
	InplaceHandle<fieldsampler::FieldSamplerQueryParams>  queryParamsHandle,
	fieldsampler::FieldSamplerKernelMode::Enum            kernelMode
)
	fieldSamplerPointsFunc<BlockSize>(_threadCount, args, paramsHandle, queryParamsHandle, kernelMode);
BOUND_KERNEL_END()

BOUND_KERNEL_BEG(FIELD_SAMPLER_GRID_WARPS_PER_BLOCK, fieldSamplerGridKernel,
	fieldsampler::FieldSamplerGridKernelArgs              args,
	InplaceHandle<fieldsampler::FieldSamplerParams>       paramsHandle,
	InplaceHandle<fieldsampler::FieldSamplerQueryParams>  queryParamsHandle,
	fieldsampler::FieldSamplerKernelMode::Enum            kernelMode
)
	fieldSamplerGridFunc<BlockSize>(_threadCount, args, paramsHandle, queryParamsHandle, kernelMode);
BOUND_KERNEL_END()

#else

BOUND_KERNEL_BEG(FIELD_SAMPLER_POINTS_WARPS_PER_BLOCK, fieldSamplerPointsKernel,
	fieldsampler::FieldSamplerPointsKernelArgs                     args,
	InplaceHandle<fieldsampler::FieldSamplerParamsHandleArray>     paramsHandleArrayHandle,
	InplaceHandle<fieldsampler::FieldSamplerQueryParams>           queryParamsHandle,
	fieldsampler::FieldSamplerKernelMode::Enum                     kernelMode
)
	fieldSamplerPointsFunc<BlockSize>(_threadCount, args, paramsHandleArrayHandle, queryParamsHandle, kernelMode);
BOUND_KERNEL_END()

BOUND_KERNEL_BEG(FIELD_SAMPLER_GRID_WARPS_PER_BLOCK, fieldSamplerGridKernel,
	fieldsampler::FieldSamplerGridKernelArgs                       args,
	InplaceHandle<fieldsampler::FieldSamplerParamsHandleArray>     paramsHandleArrayHandle,
	InplaceHandle<fieldsampler::FieldSamplerQueryParams>           queryParamsHandle,
	fieldsampler::FieldSamplerKernelMode::Enum                     kernelMode
)
	fieldSamplerGridFunc<BlockSize>(_threadCount, args, paramsHandleArrayHandle, queryParamsHandle, kernelMode);
BOUND_KERNEL_END()


#endif //FIELD_SAMPLER_SEPARATE_KERNELS
