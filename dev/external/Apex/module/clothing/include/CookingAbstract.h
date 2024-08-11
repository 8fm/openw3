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

#ifndef COOKING_ABSTRACT_H
#define COOKING_ABSTRACT_H

#include "PsUserAllocated.h"
#include "foundation/PxVec3.h"


namespace NxParameterized
{
class Interface;
}

namespace physx
{
namespace apex
{
namespace clothing
{

namespace ClothingAssetParametersNS
{
struct BoneEntry_Type;
struct ActorEntry_Type;
}

typedef ClothingAssetParametersNS::BoneEntry_Type BoneEntry;
typedef ClothingAssetParametersNS::ActorEntry_Type BoneActorEntry;

using namespace shdfnd;

class CookingAbstract : public physx::shdfnd::UserAllocated
{
public:
	CookingAbstract() : mBoneActors(NULL), mNumBoneActors(0), mBoneEntries(NULL), mNumBoneEntries(0), mBoneVertices(NULL), mMaxConvexVertices(256),
		mFreeTempMemoryWhenDone(NULL), mScale(1.0f), mVirtualParticleDensity(0.0f), mSelfcollisionRadius(0.0f)
	{
	}

	virtual ~CookingAbstract()
	{
		if (mFreeTempMemoryWhenDone != NULL)
		{
			PX_FREE(mFreeTempMemoryWhenDone);
			mFreeTempMemoryWhenDone = NULL;
		}
	}

	struct PhysicalMesh
	{
		PhysicalMesh() : meshID(0), isTetrahedral(false), vertices(NULL), numVertices(0), indices(NULL), numIndices(0), largestTriangleArea(0.0f), smallestTriangleArea(0.0f) {}
		PxU32 meshID;
		bool isTetrahedral;
		PxVec3* vertices;
		PxU32 numVertices;
		PxU32* indices;
		PxU32 numIndices;

		void computeTriangleAreas();
		PxF32 largestTriangleArea;
		PxF32 smallestTriangleArea;
	};

	struct PhysicalSubMesh
	{
		PhysicalSubMesh() : meshID(0), submeshID(0), numVertices(0), numIndices(0), numMaxDistance0Vertices(0) {}
		PxU32 meshID;
		PxU32 submeshID;
		PxU32 numVertices;
		PxU32 numIndices;
		PxU32 numMaxDistance0Vertices;
	};

	void addPhysicalMesh(const PhysicalMesh& physicalMesh);
	void addSubMesh(const PhysicalSubMesh& submesh);
	void setConvexBones(const BoneActorEntry* boneActors, PxU32 numBoneActors, const BoneEntry* boneEntries, PxU32 numBoneEntries, const PxVec3* boneVertices, PxU32 maxConvexVertices);

	void freeTempMemoryWhenDone(void* memory)
	{
		mFreeTempMemoryWhenDone = memory;
	}
	void setScale(PxF32 scale)
	{
		mScale = scale;
	}
	void setVirtualParticleDensity(PxF32 density)
	{
		PX_ASSERT(density >= 0.0f);
		PX_ASSERT(density <= 1.0f);
		mVirtualParticleDensity = density;
	}
	void setSelfcollisionRadius(PxF32 radius)
	{
		PX_ASSERT(radius >= 0.0f);
		mSelfcollisionRadius = radius;
	}
	void setGravityDirection(const PxVec3& gravityDir)
	{
		mGravityDirection = gravityDir;
		mGravityDirection.normalize();
	}

	virtual NxParameterized::Interface* execute() = 0;

	bool isValid() const;

protected:
	Array<PhysicalMesh> mPhysicalMeshes;
	Array<PhysicalSubMesh> mPhysicalSubmeshes;

	const BoneActorEntry* mBoneActors;
	PxU32 mNumBoneActors;
	const BoneEntry* mBoneEntries;
	PxU32 mNumBoneEntries;
	const PxVec3* mBoneVertices;
	PxU32 mMaxConvexVertices;

	void* mFreeTempMemoryWhenDone;
	PxF32 mScale;

	PxF32 mVirtualParticleDensity;
	PxF32 mSelfcollisionRadius;

	PxVec3 mGravityDirection;
};

}
} // namespace apex
} // namespace physx

#endif // COOKING_ABSTRACT_H
