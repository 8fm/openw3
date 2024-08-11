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

#ifndef __EXPLOSION_FS_COMMON_SRC_H__
#define __EXPLOSION_FS_COMMON_SRC_H__

#include "../../fieldsampler/include/FieldSamplerCommon.h"

namespace physx
{
namespace apex
{
namespace explosion
{

	
struct ExplosionFSParams
{
	physx::PxMat44	pose;
	physx::PxF32	radius;
	physx::PxF32	strength;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R&)
	{
	}
#endif
};

PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 executeExplosionMainFS(const ExplosionFSParams& params, const physx::PxVec3& pos, const physx::PxU32& /*totalElapsedMS*/)
{
	// bring pos to explosion's coordinate system
	physx::PxVec3 localPos = params.pose.inverseRT().rotate(pos);
	physx::PxVec3 localPosFromExplosion = localPos - params.pose.getPosition();

	if (localPosFromExplosion.magnitude() < params.radius)
	{
		physx::PxVec3 result = localPosFromExplosion;
		result.y = 2.5f * physx::PxSqrt(params.radius * params.radius - result.x * result.x - result.z * result.z);
		result = params.strength * result;
		return result;
	}

	return physx::PxVec3(0, 0, 0);
}

PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 executeExplosionFS(const ExplosionFSParams& params, const physx::PxVec3& pos, const physx::PxU32& totalElapsedMS)
{
	physx::PxVec3 resultField(0, 0, 0);
	resultField += executeExplosionMainFS(params, pos, totalElapsedMS);
	return resultField;
}

}
}
} // end namespace physx::apex

#endif
