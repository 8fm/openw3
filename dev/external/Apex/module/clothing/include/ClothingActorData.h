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

#ifndef CLOTHING_ACTOR_DATA_H
#define CLOTHING_ACTOR_DATA_H

#include "ClothingAssetData.h"
#include "PsMutex.h"
#include "foundation/PxMat44.h"
#include "foundation/PxVec3.h"

#define BLOCK_SIZE_SKIN_PHYSICS (32768*6) + (8192*0)

namespace physx
{
namespace apex
{
namespace clothing
{

/*
 * A wrapper around the data contained in ClothingActor minus all the interface stuff
 * Used on SPUs to do things like skinning
 */
class ClothingActorData
{
public:

	ClothingActorData();
	~ClothingActorData();

	void skinToAnimation_NoPhysX(bool fromFetchResults);

	void skinPhysicsMaxDist0Normals_NoPhysx();

	void renderDataLock();

	void renderDataUnLock();

	void skinToPhysicalMesh_NoPhysX(bool fromFetchResults);

	void skinToImmediateMap(const PxU32* immediateClothMap_, PxU32 numGraphicalVertices_, PxU32 numSrcVertices_,
	                        const PxVec3* srcPositions_);

	void skinToImmediateMap(const PxU32* immediateClothMap_, PxU32 numGraphicalVertices_, PxU32 numSrcVertices_,
	                        const PxVec3* srcPositions_, const PxVec3* srcNormals_);

	void finalizeSkinning_NoPhysX(bool fromFetchResults);

	void finalizeSkinningSimple_NoPhysX(bool fromFetchResults);

	template <bool withNormals, bool withTangents>
	void computeTangentSpaceUpdate(NxAbstractMeshDescription& destMesh, const ClothingMeshAssetData& rendermesh, PxU32 submeshIndex, const PxU32* compressedTangentW);

	template <bool withNormals, bool withTangents>
	void computeTangentSpaceUpdateSimple(NxAbstractMeshDescription& destMesh, const ClothingMeshAssetData& rendermesh, PxU32 submeshIndex, const PxU32* compressedTangentW);

	void tickSynchAfterFetchResults_LocksPhysX();

	void tickSynchAfterFetchResults_LocksPhysXSimple();

	PxU32 skinPhysicsSimpleMem() const;
	void skinPhysicsMeshSimple();

	PxBounds3 getRenderMeshAssetBoundsTransformed();

	// is valid only after lodTick
	bool calcIfSimplePhysicsMesh() const;

	bool getSkinOnSpu() const;

	PX_ALIGN(16, physx::shdfnd::AtomicLockCopy mRenderLock);

	ClothingAssetData mAsset;

	PxBounds3 mNewBounds;

	PX_ALIGN(16, PxMat44 mGlobalPose);
	PxMat44 mInternalGlobalPose;

	PxMat44* mInternalBoneMatricesCur;
	PxMat44* mInternalBoneMatricesPrev;

	PxVec3* mRenderingDataPosition;
	PxVec3* mRenderingDataNormal;
	PxVec4* mRenderingDataTangent;
	PxVec3* mMorphDisplacementBuffer;

	PxVec3* mSdkWritebackNormal;
	PxVec3* mSdkWritebackPositions;

	PxVec3* mSkinnedPhysicsPositions;
	PxVec3* mSkinnedPhysicsNormals;

	PxU32 mInternalMatricesCount;
	PxU32 mMorphDisplacementBufferCount;
	PxU32 mSdkDeformableVerticesCount;
	PxU32 mSdkDeformableIndicesCount;

	PxU32 mCurrentGraphicalLodId;
	PxU32 mCurrentPhysicsSubmesh;

	PxF32 mActorScale;

	bool bMeshMeshSkinningOnPPU;

	// Note - all these modified bools need to be written back to the actor!!!!
	bool bInternalFrozen;
	bool bShouldComputeRenderData;
	bool bIsInitialized;
	bool bIsSimulationMeshDirty;
	bool bRecomputeNormals;
	bool bRecomputeTangents;
	bool bCorrectSimulationNormals;
	bool bParallelCpuSkinning;
	bool bIsClothingSimulationNull;
	bool bAllGraphicalSubmeshesFitOnSpu;
	bool bSkinPhysicsMeshSpu;
	bool bSkinPhysicsMeshSpuValid;

};

}
} // namespace apex
} // namespace physx

#endif // CLOTHING_ACTOR_DATA_H
