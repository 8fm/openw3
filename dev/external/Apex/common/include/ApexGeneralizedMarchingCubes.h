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

#ifndef __APEX_GENERALIZED_MARCHING_CUBES_H__
#define __APEX_GENERALIZED_MARCHING_CUBES_H__

#include "PsShare.h"
#include "PsUserAllocated.h"
#include "PsArray.h"

#include "foundation/PxBounds3.h"

namespace physx
{
namespace apex
{

class IProgressListener;
class ApexGeneralizedCubeTemplates;

class ApexGeneralizedMarchingCubes : public physx::UserAllocated
{
public:
	ApexGeneralizedMarchingCubes(const physx::PxBounds3& bound, physx::PxU32 subdivision);
	~ApexGeneralizedMarchingCubes();

	void release();

	void registerTriangle(const physx::PxVec3& p0, const physx::PxVec3& p1, const physx::PxVec3& p2);
	bool endRegistration(physx::PxU32 bubleSizeToRemove, IProgressListener* progress);

	physx::PxU32 getNumVertices()
	{
		return mVertices.size();
	}
	physx::PxU32 getNumIndices()
	{
		return mIndices.size();
	}
	const physx::PxVec3* getVertices()
	{
		return mVertices.begin();
	}
	const physx::PxU32* getIndices()
	{
		return mIndices.begin();
	}
private:

	struct GeneralizedVertRef
	{
		void init()
		{
			vertNr = -1;
			dangling = false;
			deleted = false;
		}
		physx::PxI32 vertNr;
		bool dangling;
		bool deleted;
	};

	struct GeneralizedCube
	{
		void init(physx::PxI32 xi, physx::PxI32 yi, physx::PxI32 zi)
		{
			this->xi = xi;
			this->yi = yi;
			this->zi = zi;
			next = -1;
			vertRefs[0].init();
			vertRefs[1].init();
			vertRefs[2].init();
			sideVertexNr[0] = -1;
			sideVertexNr[1] = -1;
			sideVertexNr[2] = -1;
			sideBounds[0].setEmpty();
			sideBounds[1].setEmpty();
			sideBounds[2].setEmpty();
			firstTriangle = -1;
			numTriangles = 0;
			deleted = false;
		}
		physx::PxI32 xi, yi, zi;
		physx::PxI32 next;
		GeneralizedVertRef vertRefs[3];
		physx::PxI32 sideVertexNr[3];
		physx::PxBounds3 sideBounds[3];
		physx::PxI32 firstTriangle;
		physx::PxU32 numTriangles;
		bool deleted;
	};

	inline int hashFunction(int xi, int yi, int zi)
	{
		unsigned int h = (xi * 92837111) ^(yi * 689287499) ^(zi * 283923481);
		return h % HASH_INDEX_SIZE;
	}
	physx::PxI32 createCube(physx::PxI32 xi, physx::PxI32 yi, physx::PxI32 zi);
	physx::PxI32 findCube(physx::PxI32 xi, physx::PxI32 yi, physx::PxI32 zi);
	void completeCells();
	void createTrianglesForCube(physx::PxI32 cellNr);
	void createNeighbourInfo();
	void getCubeEdgesAndGroups(physx::PxI32 cellNr, GeneralizedVertRef* vertRefs[12], physx::PxI32 groups[8]);
	void determineGroups();
	void removeBubbles(physx::PxI32 minGroupSize);
	void fixOrientations();
	void compress();


	physx::PxBounds3 mBound;

	physx::PxF32 mSpacing;
	physx::PxF32 mInvSpacing;

	physx::Array<GeneralizedCube> mCubes;

	enum { HASH_INDEX_SIZE = 170111 };

	physx::PxI32 mFirstCube[HASH_INDEX_SIZE];

	physx::Array<physx::PxVec3> mVertices;
	physx::Array<physx::PxU32> mIndices;

	physx::Array<physx::PxI32> mFirstNeighbour;
	physx::Array<physx::PxI32> mNeighbours;
	physx::Array<physx::PxU8> mTriangleDeleted;
	physx::Array<physx::PxI32> mGeneralizedTriangles;
	physx::Array<physx::PxI32> mCubeQueue;

	physx::Array<physx::PxI32> mTriangleGroup;
	physx::Array<physx::PxI32> mGroupFirstTriangle;
	physx::Array<physx::PxI32> mGroupTriangles;

	ApexGeneralizedCubeTemplates* mTemplates;

	// for debugging only
public:
	physx::Array<physx::PxVec3> mDebugLines;
};

}
} // end namespace physx::apex

#endif
