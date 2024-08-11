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

#ifndef __NOISE_FS_COMMON_H__
#define __NOISE_FS_COMMON_H__

#include "../../fieldsampler/include/FieldSamplerCommon.h"
#include "SimplexNoise.h"

namespace physx
{
namespace apex
{
namespace basicfs
{

struct NoiseType
{
	enum Enum
	{
		SIMPLEX,
		CURL
	};
};

struct NoiseFSParams
{
	physx::PxF32	noiseStrength;
	physx::PxVec3	noiseSpaceFreq;
	physx::PxF32	noiseTimeFreq;
	physx::PxU32	noiseOctaves;
	physx::PxF32	noiseStrengthOctaveMultiplier;
	physx::PxVec3	noiseSpaceFreqOctaveMultiplier;
	physx::PxF32	noiseTimeFreqOctaveMultiplier;

	physx::PxU32	noiseType;
	physx::PxU32	noiseSeed;

	physx::PxMat34Legacy worldToShape;
	bool			useLocalSpace;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(noiseStrength);
		r.reflect(noiseSpaceFreq);
		r.reflect(noiseTimeFreq);
		r.reflect(noiseOctaves);
		r.reflect(noiseStrengthOctaveMultiplier);
		r.reflect(noiseSpaceFreqOctaveMultiplier);
		r.reflect(noiseTimeFreqOctaveMultiplier);

		r.reflect(noiseType);
		r.reflect(noiseSeed);

		r.reflect(worldToShape);
		r.reflect(useLocalSpace);
	}
#endif
};


PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 evalNoise(const NoiseFSParams& params, const physx::PxVec3& pos, physx::PxU32 totalElapsedMS)
{
	PxVec3 point;
	if (params.useLocalSpace)
	{
		const PxVec3 posInShape = params.worldToShape * pos;
		point = PxVec3(params.noiseSpaceFreq.x * posInShape.x, params.noiseSpaceFreq.y * posInShape.y, params.noiseSpaceFreq.z * posInShape.z);
	}
	else
	{
		point = PxVec3(params.noiseSpaceFreq.x * pos.x, params.noiseSpaceFreq.y * pos.y, params.noiseSpaceFreq.z * pos.z);
	}
	PxF32 time = params.noiseTimeFreq * (totalElapsedMS * 1e-3f);

	PxVec3 result;
	if (params.noiseType == NoiseType::CURL)
	{
		PxVec4 dFx;
		dFx.setZero();
		PxVec4 dFy;
		dFy.setZero();
		PxVec4 dFz;
		dFz.setZero();
		PxF32 amp = 1.0f;
		unsigned int seed = params.noiseSeed;
		for (PxU32 i = 0; i < params.noiseOctaves; ++i)
		{
			dFx += amp * SimplexNoise::eval4D(point.x, point.y, point.z, time, ++seed);
			dFy += amp * SimplexNoise::eval4D(point.x, point.y, point.z, time, ++seed);
			dFz += amp * SimplexNoise::eval4D(point.x, point.y, point.z, time, ++seed);

			amp *= params.noiseStrengthOctaveMultiplier;
			point.x *= params.noiseSpaceFreqOctaveMultiplier.x;
			point.y *= params.noiseSpaceFreqOctaveMultiplier.y;
			point.z *= params.noiseSpaceFreqOctaveMultiplier.z;
			time *= params.noiseTimeFreqOctaveMultiplier;
		}
		//build curl noise as a result
		result.x = dFz.y - dFy.z;
		result.y = dFx.z - dFz.x;
		result.z = dFy.x - dFx.y;
	}
	else
	{
		PxVec4 noise;
		noise.setZero();
		PxF32 amp = 1.0f;
		unsigned int seed = params.noiseSeed;
		for (PxU32 i = 0; i < params.noiseOctaves; ++i)
		{
			noise += amp * SimplexNoise::eval4D(point.x, point.y, point.z, time, ++seed);

			amp *= params.noiseStrengthOctaveMultiplier;
			point.x *= params.noiseSpaceFreqOctaveMultiplier.x;
			point.y *= params.noiseSpaceFreqOctaveMultiplier.y;
			point.z *= params.noiseSpaceFreqOctaveMultiplier.z;
			time *= params.noiseTimeFreqOctaveMultiplier;
		}
		//get noise gradient as a result
		result = noise.getXYZ();
	}
	result *= params.noiseStrength;
	return result;
}

PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 executeNoiseFS_GRID(const NoiseFSParams& params, const physx::PxVec3& pos, physx::PxU32 totalElapsedMS)
{
	return evalNoise(params, pos, totalElapsedMS);
}


PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 executeNoiseFS(const NoiseFSParams& params, const physx::PxVec3& pos, physx::PxU32 totalElapsedMS)
{
	return evalNoise(params, pos, totalElapsedMS);
}

}
}
} // namespace apex

#endif
