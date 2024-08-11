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

#ifndef APEX_MESH_HASH_H
#define APEX_MESH_HASH_H

#include "NxApexDefs.h"

#include "PsShare.h"
#include "PsUserAllocated.h"
#include "PsArray.h"

#include "foundation/PxVec3.h"

namespace physx
{
namespace apex
{

struct MeshHashRoot
{
	physx::PxI32 first;
	physx::PxU32 timeStamp;
};

struct MeshHashEntry
{
	physx::PxI32 next;
	physx::PxU32 itemIndex;
};


class ApexMeshHash : public physx::UserAllocated
{
public:
	ApexMeshHash();
	~ApexMeshHash();

	void   setGridSpacing(physx::PxF32 spacing);
	physx::PxF32 getGridSpacing()
	{
		return 1.0f / mInvSpacing;
	}
	void reset();
	void add(const physx::PxBounds3& bounds, physx::PxU32 itemIndex);
	void add(const physx::PxVec3& pos, physx::PxU32 itemIndex);

	void query(const physx::PxBounds3& bounds, physx::Array<physx::PxU32>& itemIndices, physx::PxI32 maxIndices = -1);
	void queryUnique(const physx::PxBounds3& bounds, physx::Array<physx::PxU32>& itemIndices, physx::PxI32 maxIndices = -1);

	void query(const physx::PxVec3& pos, physx::Array<physx::PxU32>& itemIndices, physx::PxI32 maxIndices = -1);
	void queryUnique(const physx::PxVec3& pos, physx::Array<physx::PxU32>& itemIndices, physx::PxI32 maxIndices = -1);

	// applied functions, only work if inserted objects are points!
	physx::PxU32 getClosestPointNr(const physx::PxVec3* points, physx::PxU32 numPoints, physx::PxU32 pointStride, const physx::PxVec3& pos);

private:
	enum
	{
		HashIndexSize = 170111
	};

	void compressIndices(physx::Array<physx::PxU32>& itemIndices);
	physx::PxF32 mSpacing;
	physx::PxF32 mInvSpacing;
	physx::PxU32 mTime;

	inline physx::PxU32  hashFunction(physx::PxI32 xi, physx::PxI32 yi, physx::PxI32 zi)
	{
		physx::PxU32 h = (xi * 92837111) ^(yi * 689287499) ^(zi * 283923481);
		return h % HashIndexSize;
	}

	inline void cellCoordOf(const physx::PxVec3& v, int& xi, int& yi, int& zi)
	{
		xi = (int)(v.x * mInvSpacing);
		if (v.x < 0.0f)
		{
			xi--;
		}
		yi = (int)(v.y * mInvSpacing);
		if (v.y < 0.0f)
		{
			yi--;
		}
		zi = (int)(v.z * mInvSpacing);
		if (v.z < 0.0f)
		{
			zi--;
		}
	}

	MeshHashRoot* mHashIndex;
	physx::Array<MeshHashEntry> mEntries;

	physx::Array<physx::PxU32> mTempIndices;
};

}
} // end namespace physx::apex

#endif