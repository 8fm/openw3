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

#ifndef __DESTRUCTIBLEHELPERS_H__
#define __DESTRUCTIBLEHELPERS_H__

#include "NxApex.h"
#include "ApexSDKHelpers.h"
#include "PsUserAllocated.h"
#include "PxMemoryBuffer.h"

#include "NxDestructibleAsset.h"
#include "NxFromPx.h"
#include "NxPhysicsSDK.h"

/*
	For managing mesh cooking at various scales
 */

namespace physx
{
namespace apex
{
namespace destructible
{

class NxConvexHullAtScale
{
public:
	NxConvexHullAtScale() : scale(1), convexMesh(NULL) {}
	NxConvexHullAtScale(const physx::PxVec3& inScale) : scale(inScale), convexMesh(NULL) {}

	physx::PxVec3		scale;
	NxConvexMesh* 		convexMesh;
	physx::Array<PxU8>	cookedHullData;
};

class NxMultiScaledConvexHull : public physx::UserAllocated
{
public:
	NxConvexHullAtScale* 	getConvexHullAtScale(const physx::PxVec3& scale, physx::PxF32 tolerance = 0.0001f)
	{
		// Find mesh at scale.  If not found, create one.
		for (physx::PxU32 index = 0; index < meshes.size(); ++index)
		{
			if (PxVec3equals(meshes[index].scale, scale, tolerance))
			{
				return &meshes[index];
			}
		}
		meshes.insert();
		return new(&meshes.back()) NxConvexHullAtScale(scale);
	}

	void	releaseConvexMeshes()
	{
		for (physx::PxU32 index = 0; index < meshes.size(); ++index)
		{
			if (meshes[index].convexMesh != NULL)
			{
				NxGetApexSDK()->getPhysXSDK()->releaseConvexMesh(*meshes[index].convexMesh);
				meshes[index].convexMesh = NULL;
			}
		}
	}

	Array<NxConvexHullAtScale> meshes;
};


}
}
} // end namespace physx::apex

#endif	// __DESTRUCTIBLEHELPERS_H__
