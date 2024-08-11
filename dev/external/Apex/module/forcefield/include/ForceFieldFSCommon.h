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

#ifndef __FORCEFIELD_FS_COMMON_SRC_H__
#define __FORCEFIELD_FS_COMMON_SRC_H__

#include "../../fieldsampler/include/FieldSamplerCommon.h"
#include "SimplexNoise.h"
#include "TableLookup.h"

namespace physx
{
namespace apex
{
namespace forcefield
{

struct ForceFieldShapeType
{
	enum Enum
	{
		SPHERE = 0,
		CAPSULE,
		CYLINDER,
		CONE,
		BOX,
		NONE,
	};
};

struct ForceFieldFalloffType
{
	enum Enum
	{
		LINEAR = 0,
		STEEP,
		SCURVE,
		CUSTOM,
		NONE,
	};
};

struct ForceFieldShapeDesc
{
	ForceFieldShapeType::Enum	type;
	physx::PxMat44				forceFieldToShape;
	physx::PxVec3				dimensions;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(type);
		r.reflect(forceFieldToShape);
		r.reflect(dimensions);
	}
#endif
};

struct NoiseParams
{
	physx::PxF32 strength;
	physx::PxF32 spaceScale;
	physx::PxF32 timeScale;
	physx::PxU32 octaves;
};

struct ForceFieldFSParams
{
	physx::PxMat44			pose;
	physx::PxF32			radius;
	physx::PxF32			strength;
	ForceFieldShapeDesc		includeShape;
	TableLookup				falloffTable;
	NoiseParams				noiseParams;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R&)
	{
	}
#endif
};

PX_CUDA_CALLABLE PX_INLINE bool isPosInShape(const ForceFieldShapeDesc& shapeParams, const physx::PxVec3& pos)
{
	// Sphere: x = radius
	// Capsule: x = radius, y = height
	// Cylinder: x = radius, y = height
	// Cone: x = top radius, y = height, z = bottom radius
	// Box: x,y,z = half-dimensions

	// transform position from force field coordinates to local shape coordinates
	physx::PxVec3 shapePos = shapeParams.forceFieldToShape.transform(pos);

	switch (shapeParams.type)
	{
	case ForceFieldShapeType::SPHERE:
		{
			return (shapePos.magnitude() <= shapeParams.dimensions.x);
		}
	case ForceFieldShapeType::CAPSULE:
		{
			physx::PxF32 halfHeight = shapeParams.dimensions.y / 2.0f;

			// check if y-position is within height of cylinder
			if (shapePos.y >= -halfHeight && shapePos.y <= halfHeight)
			{
				// check if x and z positions is inside radius height of cylinder
				if (physx::PxSqrt(shapePos.x * shapePos.x + shapePos.z * shapePos.z) <= shapeParams.dimensions.x)
				{
					return true;
				}
			}

			// check if position falls inside top sphere in capsule
			physx::PxVec3 spherePos = shapePos - physx::PxVec3(0, halfHeight, 0);
			if (spherePos.magnitude() <= shapeParams.dimensions.x)
			{
				return true;
			}

			// check if position falls inside bottom sphere in capsule
			spherePos = shapePos + physx::PxVec3(0, halfHeight, 0);
			if (spherePos.magnitude() <= shapeParams.dimensions.x)
			{
				return true;
			}

			return false;
		}
	case ForceFieldShapeType::CYLINDER:
		{
			physx::PxF32 halfHeight = shapeParams.dimensions.y / 2.0f;

			// check if y-position is within height of cylinder
			if (shapePos.y >= -halfHeight && shapePos.y <= halfHeight)
			{
				// check if x and z positions is inside radius height of cylinder
				if (physx::PxSqrt(shapePos.x * shapePos.x + shapePos.z * shapePos.z) <= shapeParams.dimensions.x)
				{
					return true;
				}
			}
			return false;
		}
	case ForceFieldShapeType::CONE:
		{
			physx::PxF32 halfHeight = shapeParams.dimensions.y / 2.0f;

			// check if y-position is within height of cone
			if (shapePos.y >= -halfHeight && shapePos.y <= halfHeight)
			{
				// cone can be normal or inverted
				physx::PxF32 smallerBase;
				physx::PxF32 heightFromSmallerBase;
				physx::PxF32 radiusDiff;
				if (shapeParams.dimensions.x > shapeParams.dimensions.z)
				{
					smallerBase = shapeParams.dimensions.z;
					heightFromSmallerBase = shapePos.y + halfHeight;
					radiusDiff = shapeParams.dimensions.x - shapeParams.dimensions.z;
				}
				else
				{
					smallerBase = shapeParams.dimensions.x;
					heightFromSmallerBase = halfHeight - shapePos.y;
					radiusDiff = shapeParams.dimensions.z - shapeParams.dimensions.x;
				}

				// compute radius at y-position along height of cone
				physx::PxF32 radiusAlongCone = smallerBase + (heightFromSmallerBase / shapeParams.dimensions.y) * radiusDiff;

				// check if x and z positions is inside radius at a specific height of cone
				if (physx::PxSqrt(shapePos.x * shapePos.x + shapePos.z * shapePos.z) <= radiusAlongCone)
				{
					return true;
				}
			}
			return false;
		}
	case ForceFieldShapeType::BOX:
		{
			return (physx::PxAbs(shapePos.x) <= shapeParams.dimensions.x && 
					physx::PxAbs(shapePos.y) <= shapeParams.dimensions.y &&
					physx::PxAbs(shapePos.z) <= shapeParams.dimensions.z);
		}
	default:
		{
			return false;
		}
	}
}

PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 getNoise(const ForceFieldFSParams& params, const physx::PxVec3& pos, const physx::PxU32& totalElapsedMS)
{
	PxVec3 point = params.noiseParams.spaceScale * pos;
	PxF32 time = (params.noiseParams.timeScale * 1e-3f) * totalElapsedMS;

	PxVec4 dFx;
	dFx.setZero();
	PxVec4 dFy;
	dFy.setZero();
	PxVec4 dFz;
	dFz.setZero();
	int seed = 0;
	PxF32 amp = 1.0f;
	for (PxU32 i = 0; i < params.noiseParams.octaves; ++i)
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

	return params.noiseParams.strength * rot;
}

PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 executeForceFieldMainFS(const ForceFieldFSParams& params, const physx::PxVec3& pos, const physx::PxU32& totalElapsedMS)
{
	// bring pos to force field coordinate system
	physx::PxVec3 localPos = params.pose.inverseRT().transform(pos);

	if (isPosInShape(params.includeShape, localPos))
	{
		PxVec3 result = localPos.getNormalized();
		result = result * params.strength;

		// apply falloff
		result = result * params.falloffTable.lookupTableValue(localPos.magnitude());

		// apply noise
		result = result + getNoise(params, localPos, totalElapsedMS);

		// rotate result back to world coordinate system
		return params.pose.rotate(result);
	}

	return physx::PxVec3(0, 0, 0);
}

PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 executeForceFieldFS(const ForceFieldFSParams& params, const physx::PxVec3& pos, const physx::PxU32& totalElapsedMS)
{
	physx::PxVec3 resultField(0, 0, 0);
	resultField += executeForceFieldMainFS(params, pos, totalElapsedMS);
	return resultField;
}

}
}
} // end namespace physx::apex

#endif
