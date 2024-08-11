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

#ifndef __JET_FS_COMMON_SRC_H__
#define __JET_FS_COMMON_SRC_H__

#include "../../fieldsampler/include/FieldSamplerCommon.h"
#include "SimplexNoise.h"

namespace physx
{
namespace apex
{
namespace basicfs
{

struct JetFSParams
{
	physx::PxF32	strength;
	physx::PxF32	instStrength;

	physx::PxMat34Legacy worldToDir;
	physx::PxMat34Legacy worldToInstDir;

	fieldsampler::FieldShapeParams gridIncludeShape;

	physx::PxF32	nearRadius;
	physx::PxF32	pivotRadius;
	physx::PxF32	farRadius;
	physx::PxF32	directionalStretch;
	physx::PxF32	averageStartDistance;
	physx::PxF32	averageEndDistance;

	physx::PxF32	pivotRatio;

	physx::PxF32	noiseStrength;
	physx::PxF32	noiseSpaceScale;
	physx::PxF32	noiseTimeScale;
	physx::PxU32	noiseOctaves;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(strength);
		r.reflect(instStrength);
		r.reflect(worldToDir);
		r.reflect(worldToInstDir);
		r.reflect(gridIncludeShape);
		r.reflect(nearRadius);
		r.reflect(pivotRadius);
		r.reflect(farRadius);
		r.reflect(directionalStretch);
		r.reflect(averageStartDistance);
		r.reflect(averageEndDistance);
		r.reflect(pivotRatio);
		r.reflect(noiseStrength);
		r.reflect(noiseSpaceScale);
		r.reflect(noiseTimeScale);
		r.reflect(noiseOctaves);
	}
#endif
};


PX_CUDA_CALLABLE PX_INLINE PxF32 smoothstep(PxF32 x, PxF32 edge0, PxF32 edge1)
{
	//x should be >= 0
	x = (PxClamp(x, edge0, edge1) - edge0) / (edge1 - edge0);
	// Evaluate polynomial
	return x * x * (3 - 2 * x);
}

PX_CUDA_CALLABLE PX_INLINE PxF32 smoothstep1(PxF32 x, PxF32 edge)
{
	//x should be >= 0
	x = PxMin(x, edge) / edge;
	// Evaluate polynomial
	return x * x * (3 - 2 * x);
}

PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 executeJetFS_GRID(const JetFSParams& params)
{
	return params.worldToDir.M.multiplyByTranspose(physx::PxVec3(0, params.strength, 0));
}

PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 evalToroidalField(const JetFSParams& params, const physx::PxVec3& pos, const physx::PxMat34Legacy& worldToDir, physx::PxF32 strength0)
{
	PxVec3 point = worldToDir * pos;

	PxF32 r = PxSqrt(point.x * point.x + point.z * point.z);
	PxF32 h = point.y / params.directionalStretch;

	PxF32 t;
	{
		const PxF32 r1 = r - params.pivotRadius;
		const PxF32 a = params.pivotRatio;
		const PxF32 b = (params.pivotRatio - 1) * r1;
		const PxF32 c = r1 * r1 + h * h;

		t = (PxSqrt(b * b + 4 * a * c) - b) / (2 * a);
	}

	const PxF32 r0 = params.pivotRadius + t * ((params.pivotRatio - 1) / 2);

	const PxF32 d = r0 - r;
	const PxF32 cosAngle = d / PxSqrt(d * d + h * h);
	const PxF32 angleLerp = (cosAngle + 1) * 0.5f;

	PxF32 xRatio = 0;
	PxF32 zRatio = 0;
	if (r >= 1e-5f)
	{
		xRatio = point.x / r;
		zRatio = point.z / r;
	}

	PxVec3 dir;
	dir.x = xRatio * h;
	dir.y = d * params.directionalStretch;
	dir.z = zRatio * h;

	dir.normalize();

	PxF32 strength = 0.0f;
	if (t <= params.pivotRadius)
	{
		strength = strength0 * smoothstep1(t, params.pivotRadius - params.nearRadius);

		strength *= (params.pivotRadius - t) / r;
	}
	strength /= (angleLerp + params.pivotRatio * (1 - angleLerp));

	return strength * worldToDir.M.multiplyByTranspose(dir);
}

PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 executeJetFS(const JetFSParams& params, const physx::PxVec3& pos, physx::PxU32 totalElapsedMS)
{
	physx::PxVec3 avgField = evalToroidalField(params, pos, params.worldToDir, params.strength);
	physx::PxVec3 instField = evalToroidalField(params, pos, params.worldToInstDir, params.instStrength);

	physx::PxF32 distance = (pos - params.worldToDir.t).magnitude();
	physx::PxF32 lerpFactor = smoothstep(distance, params.averageStartDistance, params.averageEndDistance);
	physx::PxVec3 result = lerpFactor * avgField + (1 - lerpFactor) * instField;

	//add some noise
	PxVec3 point = params.noiseSpaceScale * (params.worldToDir * pos);
	PxF32 time = (params.noiseTimeScale * 1e-3f) * totalElapsedMS;

	PxVec4 dFx;
	dFx.setZero();
	PxVec4 dFy;
	dFy.setZero();
	PxVec4 dFz;
	dFz.setZero();
	int seed = 0;
	PxF32 amp = 1.0f;
	for (PxU32 i = 0; i < params.noiseOctaves; ++i)
	{
		dFx += amp * SimplexNoise::eval4D(point.x, point.y, point.z, time, ++seed);
		dFy += amp * SimplexNoise::eval4D(point.x, point.y, point.z, time, ++seed);
		dFz += amp * SimplexNoise::eval4D(point.x, point.y, point.z, time, ++seed);

		point *= 2;
		time *= 2;
		amp *= 0.5f;
	}
	//get rotor
	PxVec3 rot;
	rot.x = dFz.y - dFy.z;
	rot.y = dFx.z - dFz.x;
	rot.z = dFy.x - dFx.y;

	result += params.noiseStrength * params.worldToDir.M.multiplyByTranspose(rot);
	return result;
}

}
}
} // namespace apex

#endif
