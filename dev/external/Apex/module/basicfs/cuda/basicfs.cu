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
using namespace physx::apex::basicfs;
#include "include/basicfs.h"


inline __device__ physx::PxF32 evalFadeAntialiasing(physx::PxF32 dist, physx::PxF32 fade, physx::PxF32 cellRadius)
{
	//Kirill KK: we solve aliasing problem using convolution of two functions (linear and constant)
	physx::PxF32 a, c, x, res, parab1, parab2;
	bool found = false;
	a = cellRadius;
	c = fade;
	x = dist;
	res = 0;

	parab1 = (- x * x + 2 * (1 - a - c) * x - (c - a) * (c - a) + 2 * (a + c) - 1) / (4 * a * c);
	parab2 = (x * x - 2 * (1 + a) * x + (a + 1) * (a + 1)) / (4 * a * c); 

	if(x <= 1 - c - a) 
	{
		res = 1;
		found = true;
	}
	else if(x >= 1 + a)
	{
		res = 0.0f;
		found = true;
	}


	if(!found)
	{
		if(2 * a < c)
		{
			if(x <= 1 - c + a)
			{
				res = parab1;
				found = true;
			}
			else if(x <= 1 - a) 
			{
				res = (1 - x) / c;
				found = true;
			}
		}
		else
		{
			if(x <= 1 - a) 
			{
				res = parab1;
				found = true;
			}
			else if(x <= 1 - c + a)
			{
				res = (2 * (1 + a - x) - c) / (4 * a);
				found = true;
			}
		}
	}

	if(!found && x < 1 + a)
	{
		res = parab2;
		found = true;
	}

	//old formula:  
	//physx::PxF32 res = (1 - dist) / (fade + 1e-5f) + cellRadius - cellRadius;

	return physx::PxClamp<physx::PxF32>(res, 0, 1);
}

inline __device__ physx::PxF32 evalWeightInShapeGrid(const fieldsampler::FieldShapeParams& shapeParams, const physx::PxVec3& position, physx::PxF32 cellRadius)
{
	physx::PxF32 dist = fieldsampler::evalDistInShape(shapeParams, position);
	return evalFadeAntialiasing(dist, shapeParams.fade, cellRadius) * shapeParams.weight;
}


template <>
inline __device__ physx::PxF32 evalFieldSamplerIncludeWeight<fieldsampler::FieldSamplerKernelType::GRID>(const fieldsampler::FieldSamplerParams* params, const physx::PxVec3& position, physx::PxF32 cellRadius)
{
	physx::PxF32 result;
	switch (params->executeType)
	{
	case 1:
		{
			const JetFSParams* jetParams = params->executeParamsHandle.resolveAndCastTo<JetFSParams>(KERNEL_CONST_MEM(fieldsamplerConstMem));			
			result = evalWeightInShapeGrid(jetParams->gridIncludeShape, position, cellRadius);
		}
		break;
	default:
		{
			result = evalWeightInShapeGrid(params->includeShape, position, cellRadius);
		}
		break;
	}
	return result;
}

template <>
inline __device__ physx::PxVec3 executeFieldSampler<fieldsampler::FieldSamplerKernelType::POINTS>(const fieldsampler::FieldSamplerParams* params, const fieldsampler::FieldSamplerExecuteArgs& args, physx::PxF32& fieldWeight)
{
	physx::PxVec3 result;
	switch (params->executeType)
	{
	case 1:
		{
			const JetFSParams* executeParams = params->executeParamsHandle.resolveAndCastTo<JetFSParams>(KERNEL_CONST_MEM(fieldsamplerConstMem));
			result = executeJetFS(*executeParams, args.position, args.totalElapsedMS);
		}
		break;
	case 2:
		{
			const AttractorFSParams* executeParams = params->executeParamsHandle.resolveAndCastTo<AttractorFSParams>(KERNEL_CONST_MEM(fieldsamplerConstMem));
			result = executeAttractorFS(*executeParams, args.position);
		}
		break;
	case 3:
		{
			const NoiseFSParams* executeParams = params->executeParamsHandle.resolveAndCastTo<NoiseFSParams>(KERNEL_CONST_MEM(fieldsamplerConstMem));
			result = executeNoiseFS(*executeParams, args.position, args.totalElapsedMS);
		}
		break;
	case 4:
		{
			const VortexFSParams* executeParams = params->executeParamsHandle.resolveAndCastTo<VortexFSParams>(KERNEL_CONST_MEM(fieldsamplerConstMem));
			result = executeVortexFS(*executeParams, args.position);
		}
		break;
	}
	return result;
}

template <>
inline __device__ physx::PxVec3 executeFieldSampler<fieldsampler::FieldSamplerKernelType::GRID>(const fieldsampler::FieldSamplerParams* params, const fieldsampler::FieldSamplerExecuteArgs& args, physx::PxF32& fieldWeight)
{
	physx::PxVec3 result;
	switch (params->executeType)
	{
	case 1:
		{
			const JetFSParams* executeParams = params->executeParamsHandle.resolveAndCastTo<JetFSParams>(KERNEL_CONST_MEM(fieldsamplerConstMem));
			result = executeJetFS_GRID(*executeParams);
		}
		break;
	case 3:
		{
			const NoiseFSParams* executeParams = params->executeParamsHandle.resolveAndCastTo<NoiseFSParams>(KERNEL_CONST_MEM(fieldsamplerConstMem));
			result = executeNoiseFS_GRID(*executeParams, args.position, args.totalElapsedMS);
		}
		break;
	case 4:
		{
			const VortexFSParams* executeParams = params->executeParamsHandle.resolveAndCastTo<VortexFSParams>(KERNEL_CONST_MEM(fieldsamplerConstMem));
			//result = executeVortexFS_GRID(*executeParams, args.position);
			result = executeVortexFS(*executeParams, args.position);
		}
		break;
	}
	return result;
}

#include "../../fieldsampler/cuda/include/fieldsamplerInc.cuh"

