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

#include "NxApexDefs.h"
#include "MinPhysxSdkVersion.h"
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

#include "SimulationAbstract.h"

// for NUM_VERTICES_PER_CACHE_BLOCK
#include "ClothingAsset.h"


namespace physx
{
namespace apex
{
namespace clothing
{

void SimulationAbstract::init(PxU32 numVertices, PxU32 numIndices, bool writebackNormals)
{
	sdkNumDeformableVertices = numVertices;
	sdkNumDeformableIndices = numIndices;

	const PxU32 alignedNumVertices = (numVertices + 15) & 0xfffffff0;
	const PxU32 writeBackDataSize = (sizeof(PxVec3) * alignedNumVertices) * (writebackNormals ? 2 : 1);

	PX_ASSERT(sdkWritebackPosition == NULL);
	PX_ASSERT(sdkWritebackNormal == NULL);
	sdkWritebackPosition = (PxVec3*)PX_ALLOC(writeBackDataSize, PX_DEBUG_EXP("SimulationAbstract::writebackData"));
	sdkWritebackNormal = writebackNormals ? sdkWritebackPosition + alignedNumVertices : NULL;

	const PxU32 allocNumVertices = (((numVertices + NUM_VERTICES_PER_CACHE_BLOCK - 1) / NUM_VERTICES_PER_CACHE_BLOCK)) * NUM_VERTICES_PER_CACHE_BLOCK;
	PX_ASSERT(skinnedPhysicsPositions == NULL);
	PX_ASSERT(skinnedPhysicsNormals == NULL);
	skinnedPhysicsPositions = (PxVec3*)PX_ALLOC(sizeof(PxVec3) * allocNumVertices * 2, PX_DEBUG_EXP("SimulationAbstract::skinnedPhysicsPositions"));
	skinnedPhysicsNormals = skinnedPhysicsPositions + allocNumVertices;
}



void SimulationAbstract::initSimulation(const tSimParams& s)
{
	simulation = s;
}


}
} // namespace apex
} // namespace physx

#endif //NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2
