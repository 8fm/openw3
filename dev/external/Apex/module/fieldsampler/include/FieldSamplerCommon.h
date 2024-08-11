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

#ifndef __FIELD_SAMPLER_COMMON_H__
#define __FIELD_SAMPLER_COMMON_H__

#include "foundation/PxVec3.h"
#include "foundation/PxVec4.h"
#include <PxMat34Legacy.h>

#include <NiFieldSampler.h>
#include <NiFieldBoundary.h>
#include <InplaceTypes.h>

#if defined(APEX_CUDA_SUPPORT)
#pragma warning(push)
#pragma warning(disable:4201)
#pragma warning(disable:4408)

#include <vector_types.h>

#pragma warning(pop)
#endif


namespace physx
{
namespace apex
{
namespace fieldsampler
{

#define VELOCITY_WEIGHT_THRESHOLD 0.001f

struct FieldSamplerExecuteArgs
{
	physx::PxVec3			position;
	physx::PxF32			mass;
	physx::PxVec3			velocity;

	physx::PxF32			elapsedTime;
	physx::PxU32			totalElapsedMS;
};

struct FieldShapeParams : NiFieldShapeDesc
{
	physx::PxF32			fade;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		NiFieldShapeDesc::reflect(r);
		r.reflect(fade);
	}
#endif
};

struct FieldShapeGroupParams
{
	InplaceArray<FieldShapeParams> shapeArray;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(shapeArray);
	}
#endif
};

struct FieldSamplerParams
{
	physx::PxU32						executeType;
	InplaceHandleBase					executeParamsHandle;

	NiFieldSamplerType::Enum			type;
	NiFieldSamplerGridSupportType::Enum	gridSupportType;
	physx::PxF32						dragCoeff;

	FieldShapeParams					includeShape;
	InplaceArray< InplaceHandle<FieldShapeGroupParams>, false > excludeShapeGroupHandleArray;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(executeType);
		r.reflect(executeParamsHandle);
		r.reflect(type);
		r.reflect(gridSupportType);
		r.reflect(dragCoeff);
		r.reflect(includeShape);
		r.reflect(excludeShapeGroupHandleArray);
	}
#endif
};

struct FieldSamplerQueryParams
{
#ifndef __CUDACC__
	template <typename R>
	void reflect(R& )
	{
	}
#endif
};


typedef InplaceArray< InplaceHandle<FieldSamplerParams>, false > FieldSamplerParamsHandleArray;

#if defined(APEX_CUDA_SUPPORT) || defined(__CUDACC__)

struct FieldSamplerKernelType
{
	enum Enum
	{
		POINTS,
		GRID
	};
};

struct FieldSamplerKernelArgs
{
	physx::PxF32 elapsedTime;
	physx::PxF32 cellRadius;
	physx::PxU32 totalElapsedMS;

	float4* accumField;
	float4* accumVelocity;
};

struct FieldSamplerPointsKernelArgs : FieldSamplerKernelArgs
{
	const float4* positionMass;
	const float4* velocity;
};

struct FieldSamplerGridKernelArgs : FieldSamplerKernelArgs
{
	physx::PxU32 numX, numY, numZ;
	physx::PxU32 strideX, strideY, offset;

	physx::PxMat34Legacy gridToWorld;

	physx::PxF32 mass;
};

struct FieldSamplerKernelMode
{
	enum Enum
	{
		DEFAULT = 0,
		FINISH_PRIMARY = 1,
		FINISH_SECONDARY = 2
	};
};

#endif

#if defined(APEX_CUDA_SUPPORT)

class FieldSamplerWrapper;

struct NiFieldSamplerKernelLaunchData
{
	CUstream                                        stream;
	physx::PxU32                                    threadCount;
	FieldSamplerKernelType::Enum                    kernelType;
	const FieldSamplerKernelArgs*                   kernelArgs;
	InplaceHandle<FieldSamplerQueryParams>          queryParamsHandle;
	InplaceHandle<FieldSamplerParamsHandleArray>    paramsHandleArrayHandle;
	const physx::Array<FieldSamplerWrapper*>*       fieldSamplerArray;
	physx::PxU32                                    activeFieldSamplerCount;
	FieldSamplerKernelMode::Enum                    kernelMode;
};

#endif

PX_CUDA_CALLABLE PX_INLINE physx::PxF32 evalFade(physx::PxF32 dist, physx::PxF32 fade)
{
	physx::PxF32 x = (1 - dist) / (fade + 1e-5f);
	return PxClamp<physx::PxF32>(x, 0, 1);
}

PX_CUDA_CALLABLE PX_INLINE physx::PxF32 evalDistInShapeNONE(const NiFieldShapeDesc& /*shapeParams*/, const physx::PxVec3& /*worldPos*/)
{
	return 0.0f;
}

PX_CUDA_CALLABLE PX_INLINE physx::PxF32 evalDistInShapeSPHERE(const NiFieldShapeDesc& shapeParams, const physx::PxVec3& worldPos)
{
	const physx::PxVec3 shapePos = shapeParams.worldToShape * worldPos;
	const physx::PxF32 radius = shapeParams.dimensions.x;
	return shapePos.magnitude() / radius;
}

PX_CUDA_CALLABLE PX_INLINE physx::PxF32 evalDistInShapeBOX(const NiFieldShapeDesc& shapeParams, const physx::PxVec3& worldPos)
{
	const physx::PxVec3 shapePos = shapeParams.worldToShape * worldPos;
	const physx::PxVec3& halfSize = shapeParams.dimensions;
	physx::PxVec3 unitPos(shapePos.x / halfSize.x, shapePos.y / halfSize.y, shapePos.z / halfSize.z);
	return physx::PxVec3(physx::PxAbs(unitPos.x), physx::PxAbs(unitPos.y), physx::PxAbs(unitPos.z)).maxElement();
}

PX_CUDA_CALLABLE PX_INLINE physx::PxF32 evalDistInShapeCAPSULE(const NiFieldShapeDesc& shapeParams, const physx::PxVec3& worldPos)
{
	const physx::PxVec3 shapePos = shapeParams.worldToShape * worldPos;
	const physx::PxF32 radius = shapeParams.dimensions.x;
	const physx::PxF32 halfHeight = shapeParams.dimensions.y * 0.5f;

	physx::PxVec3 clampPos = shapePos;
	clampPos.y -= physx::PxClamp(shapePos.y, -halfHeight, +halfHeight);

	return clampPos.magnitude() / radius;
}

PX_CUDA_CALLABLE PX_INLINE physx::PxF32 evalDistInShape(const NiFieldShapeDesc& shapeParams, const physx::PxVec3& worldPos)
{
	switch (shapeParams.type)
	{
	case NiFieldShapeType::NONE:
		return evalDistInShapeNONE(shapeParams, worldPos);
	case NiFieldShapeType::SPHERE:
		return evalDistInShapeSPHERE(shapeParams, worldPos);
	case NiFieldShapeType::BOX:
		return evalDistInShapeBOX(shapeParams, worldPos);
	case NiFieldShapeType::CAPSULE:
		return evalDistInShapeCAPSULE(shapeParams, worldPos);
	default:
		return 0.0f;
	};
}

PX_CUDA_CALLABLE PX_INLINE physx::PxF32 evalWeightInShape(const FieldShapeParams& shapeParams, const physx::PxVec3& position)
{
	physx::PxF32 dist = physx::apex::fieldsampler::evalDistInShape(shapeParams, position);
	return physx::apex::fieldsampler::evalFade(dist, shapeParams.fade) * shapeParams.weight;
}

PX_CUDA_CALLABLE PX_INLINE void accumFORCE(const FieldSamplerExecuteArgs& args,
	const physx::PxVec3& field, physx::PxF32 fieldW,
	physx::PxVec4& accumAccel, physx::PxVec4& accumVelocity)
{
	PX_UNUSED(accumVelocity);

	physx::PxVec3 newAccel = ((1 - accumAccel.w) * fieldW * args.elapsedTime / args.mass) * field;
	accumAccel.x += newAccel.x;
	accumAccel.y += newAccel.y;
	accumAccel.z += newAccel.z;
}

PX_CUDA_CALLABLE PX_INLINE void accumACCELERATION(const FieldSamplerExecuteArgs& args,
	const physx::PxVec3& field, physx::PxF32 fieldW,
	physx::PxVec4& accumAccel, physx::PxVec4& accumVelocity)
{
	PX_UNUSED(accumVelocity);

	physx::PxVec3 newAccel = ((1 - accumAccel.w) * fieldW * args.elapsedTime) * field;
	accumAccel.x += newAccel.x;
	accumAccel.y += newAccel.y;
	accumAccel.z += newAccel.z;
}

PX_CUDA_CALLABLE PX_INLINE void accumVELOCITY_DRAG(const FieldSamplerExecuteArgs& args, physx::PxF32 dragCoeff,
	const physx::PxVec3& field, physx::PxF32 fieldW,
	physx::PxVec4& accumAccel, physx::PxVec4& accumVelocity)
{
	const physx::PxVec3 force = (field - args.velocity) * dragCoeff;
	accumFORCE(args, force, fieldW, accumAccel, accumVelocity);
}

PX_CUDA_CALLABLE PX_INLINE void accumVELOCITY_DIRECT(const FieldSamplerExecuteArgs& args,
	const physx::PxVec3& field, physx::PxF32 fieldW,
	physx::PxVec4& accumAccel, physx::PxVec4& accumVelocity)
{
	PX_UNUSED(args);

	physx::PxVec3 newVelocity = ((1 - accumAccel.w) * fieldW) * field;
	accumVelocity.x += newVelocity.x;
	accumVelocity.y += newVelocity.y;
	accumVelocity.z += newVelocity.z;
	accumVelocity.w = physx::PxMax(accumVelocity.w, fieldW);
}

}
} // namespace apex
}
#ifdef __CUDACC__

#ifdef APEX_TEST
struct PxInternalParticleFlagGpu
{
	enum Enum
	{
		//reserved	(1<<0),
		//reserved	(1<<1),
		//reserved	(1<<2),
		//reserved	(1<<3),
		//reserved	(1<<4),
		//reserved	(1<<5),
		eCUDA_NOTIFY_CREATE					= (1 << 6),
		eCUDA_NOTIFY_SET_POSITION			= (1 << 7),
	};
};
struct PxParticleFlag
{
	enum Enum
	{
		eVALID								= (1 << 0),
		eCOLLISION_WITH_STATIC				= (1 << 1),
		eCOLLISION_WITH_DYNAMIC				= (1 << 2),
		eCOLLISION_WITH_DRAIN				= (1 << 3),
		eSPATIAL_DATA_STRUCTURE_OVERFLOW	= (1 << 4),
	};
};
struct PxParticleFlagGpu
{
	physx::PxU16 api;	// PxParticleFlag
	physx::PxU16 low;	// PxInternalParticleFlagGpu
};

PX_CUDA_CALLABLE PX_INLINE void testParticle(physx::PxVec4& position, physx::PxVec4& velocity, physx::PxU32& flag, 
											 const physx::PxVec4& initPosition, const physx::PxVec4& initVelocity)
{
	position = initPosition;
	velocity = initVelocity;
	
	PxParticleFlagGpu& f = ((PxParticleFlagGpu&) flag);
	f.api = PxParticleFlag::eVALID;
	f.low = PxInternalParticleFlagGpu::eCUDA_NOTIFY_CREATE;
}
#endif

template <int queryType>
inline __device__ physx::PxVec3 executeFieldSampler(const physx::apex::fieldsampler::FieldSamplerParams* params, const physx::apex::fieldsampler::FieldSamplerExecuteArgs& args, physx::PxF32& fieldWeight);

template <int queryType>
inline __device__ physx::PxF32 evalFieldSamplerIncludeWeight(const physx::apex::fieldsampler::FieldSamplerParams* params, const physx::PxVec3& position, physx::PxF32 cellRadius)
{
	return physx::apex::fieldsampler::evalWeightInShape(params->includeShape, position);
}

#endif

#endif
