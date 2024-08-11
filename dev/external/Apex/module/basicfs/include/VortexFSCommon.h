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

#ifndef __VORTEX_FS_COMMON_SRC_H__
#define __VORTEX_FS_COMMON_SRC_H__

#include "../../fieldsampler/include/FieldSamplerCommon.h"

namespace physx
{
namespace apex
{
namespace basicfs
{

struct VortexFSParams
{
	physx::PxMat34Legacy worldToDir;

	fieldsampler::FieldShapeParams gridIncludeShape;

	physx::PxF32	height;
	physx::PxF32	bottomRadius;
	physx::PxF32	topRadius;

	physx::PxF32	rotationalStrength;
	physx::PxF32	radialStrength;
	physx::PxF32	liftStrength;


#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(worldToDir);
		r.reflect(gridIncludeShape);
		r.reflect(height);
		r.reflect(bottomRadius);
		r.reflect(topRadius);
		r.reflect(rotationalStrength);
		r.reflect(radialStrength);
		r.reflect(liftStrength);
	}
#endif
};

PX_CUDA_CALLABLE PX_INLINE physx::PxF32 sqr(physx::PxF32 x)
{
	return x * x;
}

/*
PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 executeVortexFS_GRID(const VortexFSParams& params)
{
	return params.worldToDir.M.multiplyByTranspose(physx::PxVec3(0, params.strength, 0));
}*/

PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 executeVortexFS(const VortexFSParams& params, const physx::PxVec3& pos/*, physx::PxU32 totalElapsedMS*/)
{
	PX_ASSERT(params.bottomRadius);
	PX_ASSERT(params.topRadius);
	
	physx::PxVec3 result;
	PxVec3 point = params.worldToDir * pos;
	PxF32 R = PxSqrt(point.x * point.x + point.z * point.z);
	PxF32 invR = 1.f / R;
	PxF32 curR = 0;
	PxF32 h = params.height, r1 = params.bottomRadius, r2 = params.topRadius, y = point.y;

	if (point.y < h/2 && y > -h/2)
	{
		curR = r1 + (r2-r1) * (y / h + 0.5f);
	}
	else if (y <= -h/2 && y >= -h/2-r1)
	{
		curR = PxSqrt(r1*r1 - sqr(y+h/2));
	}
	else if (y >= h/2 && y <= h/2+r2)
	{
		curR = PxSqrt(r2*r2 - sqr(y-h/2));
	}

	if (curR > 0.f && R <= curR)
	{
		result.x = (params.radialStrength * point.x - params.rotationalStrength * R / curR * point.z) * invR;
		result.y = params.liftStrength;
		result.z = (params.radialStrength * point.z + params.rotationalStrength * R / curR * point.x) * invR;
	}

	return params.worldToDir.M.multiplyByTranspose(result);
}

}
}
} // namespace apex

#endif
