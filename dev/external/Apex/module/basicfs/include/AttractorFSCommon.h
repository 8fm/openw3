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

#ifndef __ATTRACTOR_FS_COMMON_SRC_H__
#define __ATTRACTOR_FS_COMMON_SRC_H__

#include "../../fieldsampler/include/FieldSamplerCommon.h"

namespace physx
{
namespace apex
{
namespace basicfs
{

struct AttractorFSParams
{
	physx::PxVec3	origin;

	physx::PxF32	radius;

	physx::PxF32	constFieldStrength;
	physx::PxF32	variableFieldStrength;


#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(origin);
		r.reflect(radius);
		r.reflect(constFieldStrength);
		r.reflect(variableFieldStrength);
	}
#endif
};

PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 commonAttractorFSKernel(const AttractorFSParams& params, const physx::PxVec3& pos)
{
	physx::PxVec3 dir = params.origin - pos;
	PX_ASSERT(params.radius);
	physx::PxF32 dist = dir.magnitude() / params.radius;

	physx::PxF32 result = params.constFieldStrength;
	if (dist >= 0.4)
	{
		result += params.variableFieldStrength / dist;
	}

	return result * dir.getNormalized();
}

PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 executeAttractorFS(const AttractorFSParams& params, const physx::PxVec3& pos/*, physx::PxU32 totalElapsedMS*/)
{
	physx::PxVec3 dir = params.origin - pos;
	PX_ASSERT(params.radius);
	physx::PxF32 dist = dir.magnitude() / params.radius;

	physx::PxF32 result = params.constFieldStrength;
	if (dist >= 0.4)
	{
		result += params.variableFieldStrength / dist;
	}

	return result * dir.getNormalized();
}

}
}
} // namespace apex

#endif
