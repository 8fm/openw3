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

#include "CookingAbstract.h"



namespace physx
{
namespace apex
{
namespace clothing
{

void CookingAbstract::PhysicalMesh::computeTriangleAreas()
{
	smallestTriangleArea = largestTriangleArea = 0.0f;

	if (indices == NULL || vertices == NULL)
	{
		return;
	}

	smallestTriangleArea = PX_MAX_F32;

	for (PxU32 i = 0; i < numIndices; i += 3)
	{
		const PxVec3 edge1 = vertices[indices[i + 1]] - vertices[indices[i]];
		const PxVec3 edge2 = vertices[indices[i + 2]] - vertices[indices[i]];
		const PxF32 triangleArea = edge1.cross(edge2).magnitude();

		largestTriangleArea = PxMax(largestTriangleArea, triangleArea);
		smallestTriangleArea = PxMin(smallestTriangleArea, triangleArea);
	}
}



void CookingAbstract::addPhysicalMesh(const PhysicalMesh& physicalMesh)
{
	PhysicalMesh physicalMeshCopy = physicalMesh;
	physicalMeshCopy.computeTriangleAreas();
	mPhysicalMeshes.pushBack(physicalMeshCopy);
}



void CookingAbstract::addSubMesh(const PhysicalSubMesh& submesh)
{
	mPhysicalSubmeshes.pushBack(submesh);
}



void CookingAbstract::setConvexBones(const BoneActorEntry* boneActors, PxU32 numBoneActors, const BoneEntry* boneEntries,
                                     PxU32 numBoneEntries, const PxVec3* boneVertices, PxU32 maxConvexVertices)
{
	mBoneActors = boneActors;
	mNumBoneActors = numBoneActors;
	mBoneEntries = boneEntries;
	mNumBoneEntries = numBoneEntries;
	mBoneVertices = boneVertices;

	PX_ASSERT(maxConvexVertices <= 256);
	mMaxConvexVertices = maxConvexVertices;
}


bool CookingAbstract::isValid() const
{
	return mPhysicalMeshes.size() > 0 && mPhysicalSubmeshes.size() > 0;
}

}
}
}