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

#ifndef CLOTHING_ASSET_AUTHORING_H
#define CLOTHING_ASSET_AUTHORING_H


#include "NxClothingAssetAuthoring.h"
#include "ClothingAsset.h"
#include "ClothingGraphicalLodParameters.h"
#include "ApexAssetAuthoring.h"

#ifndef WITHOUT_APEX_AUTHORING

namespace physx
{
namespace apex
{
namespace clothing
{

class ClothingPhysicalMesh;


class ClothingAssetAuthoring : public NxClothingAssetAuthoring, public ApexAssetAuthoring, public ClothingAsset
{
public:
	ClothingAssetAuthoring(ModuleClothing* module, NxResourceList& list, const char* name);
	ClothingAssetAuthoring(ModuleClothing* module, NxResourceList& list);
	ClothingAssetAuthoring(ModuleClothing* module, NxResourceList& list, NxParameterized::Interface* params, const char* name);

	// from NxApexAssetAuthoring
	virtual const char* getName(void) const
	{
		return ClothingAsset::getName();
	}
	virtual const char* getObjTypeName() const
	{
		return NX_CLOTHING_AUTHORING_TYPE_NAME;
	}
	virtual bool							prepareForPlatform(physx::apex::NxPlatformTag);

	virtual void setToolString(const char* toolName, const char* toolVersion, PxU32 toolChangelist)
	{
		ApexAssetAuthoring::setToolString(toolName, toolVersion, toolChangelist);
	}

	// from NxApexInterface
	virtual void							release();

	// from NxClothingAssetAuthoring
	virtual void							setDefaultConstrainCoefficients(const NxClothingConstrainCoefficients& coeff)
	{
		mDefaultConstrainCoefficients = coeff;
	}
	virtual void							setInvalidConstrainCoefficients(const NxClothingConstrainCoefficients& coeff)
	{
		mInvalidConstrainCoefficients = coeff;
	}
	virtual void							setMeshes(physx::PxU32 lod, NxRenderMeshAssetAuthoring* asset, NxClothingPhysicalMesh* mesh,
	        physx::PxU32 numMaxDistReductions = 0, physx::PxF32* maxDistReductions = NULL, physx::PxF32 normalResemblance = 90,
	        bool ignoreUnusedVertices = true, IProgressListener* progress = NULL);
	virtual bool							addPlatformToGraphicalLod(physx::PxU32 lod, NxPlatformTag platform);
	virtual bool							removePlatform(physx::PxU32 lod,  NxPlatformTag platform);
	virtual physx::PxU32					getNumPlatforms(physx::PxU32 lod);
	virtual NxPlatformTag					getPlatform(physx::PxU32 lod, physx::PxU32 i);
	virtual physx::PxU32					getNumLods();
	virtual physx::PxI32					getLodValue(physx::PxU32 lod);
	virtual void							clearMeshes();
	virtual NxClothingPhysicalMesh*			getClothingPhysicalMesh(physx::PxU32 graphicalLod);
	virtual void							setBoneInfo(physx::PxU32 boneIndex, const char* boneName, const physx::PxMat44& bindPose, physx::PxI32 parentIndex);
	virtual void							setRootBone(const char* boneName);
	virtual physx::PxU32					addBoneConvex(const char* boneName, const physx::PxVec3* positions, physx::PxU32 numPositions);
	virtual physx::PxU32					addBoneConvex(physx::PxU32 boneIndex, const physx::PxVec3* positions, physx::PxU32 numPositions);
	virtual void							addBoneCapsule(const char* boneName, physx::PxF32 capsuleRadius, physx::PxF32 capsuleHeight, const physx::PxMat44& localPose);
	virtual void							addBoneCapsule(physx::PxU32 boneIndex, physx::PxF32 capsuleRadius, physx::PxF32 capsuleHeight, const physx::PxMat44& localPose);
	virtual void							clearBoneActors(const char* boneName);
	virtual void							clearBoneActors(physx::PxU32 boneIndex);
	virtual void							clearAllBoneActors();

	virtual void							setCollision(const char** boneNames, physx::PxF32* radii, physx::PxVec3* localPositions, PxU32 numSpheres, physx::PxU16* pairs, physx::PxU32 numPairs);
	virtual void							setCollision(physx::PxU32* boneIndices, physx::PxF32* radii, physx::PxVec3* localPositions, PxU32 numSpheres, physx::PxU16* pairs, physx::PxU32 numPairs);
	virtual void							clearCollision();

	virtual void							setSimulationHierarchicalLevels(physx::PxU32 levels)
	{
		mParams->simulation.hierarchicalLevels = levels;
		clearCooked();
	}
	virtual void							setSimulationThickness(physx::PxF32 thickness)
	{
		mParams->simulation.thickness = thickness;
	}
	virtual void							setSimulationVirtualParticleDensity(physx::PxF32 density)
	{
		PX_ASSERT(density >= 0.0f);
		PX_ASSERT(density <= 1.0f);
		mParams->simulation.virtualParticleDensity = PxClamp(density, 0.0f, 1.0f);
	}
	virtual void							setSimulationSleepLinearVelocity(physx::PxF32 sleep)
	{
		mParams->simulation.sleepLinearVelocity = sleep;
	}
	virtual void							setSimulationGravityDirection(const physx::PxVec3& gravity)
	{
		mParams->simulation.gravityDirection = gravity.getNormalized();
	}

	virtual void							setSimulationDisableCCD(bool disable)
	{
		mParams->simulation.disableCCD = disable;
	}
	virtual void							setSimulationTwowayInteraction(bool enable)
	{
		mParams->simulation.twowayInteraction = enable;
	}
	virtual void							setSimulationUntangling(bool enable)
	{
		mParams->simulation.untangling = enable;
	}
	virtual void							setSimulationRestLengthScale(float scale)
	{
		mParams->simulation.restLengthScale = scale;
	}

	virtual void							setExportScale(physx::PxF32 scale)
	{
		APEX_DEPRECATED_ONCE();
		mExportScale = scale;
	}
	virtual void							applyTransformation(const physx::PxMat44& transformation, physx::PxF32 scale, bool applyToGraphics, bool applyToPhysics);
	virtual void							updateBindPoses(const physx::PxMat44* newBindPoses, physx::PxU32 newBindPosesCount, bool isInternalOrder, bool collisionMaintainWorldPose);
	virtual void							setDeriveNormalsFromBones(bool enable)
	{
		mDeriveNormalsFromBones = enable;
	}
	virtual NxParameterized::Interface*		getMaterialLibrary();
	virtual bool							setMaterialLibrary(NxParameterized::Interface* materialLibrary, physx::PxU32 materialIndex, bool transferOwnership);
	virtual NxParameterized::Interface*		getRenderMeshAssetAuthoring(physx::PxU32 lodLevel);

	// parameterization
	NxParameterized::Interface*				getNxParameterized() const
	{
		return mParams;
	}
	virtual NxParameterized::Interface*		releaseAndReturnNxParameterizedInterface();

	// from NxParameterized::SerializationCallback
	virtual void							preSerialize(void* userData);

	// from ApexAssetAuthoring
	virtual void							setToolString(const char* toolString);

	// internal
	void									destroy();

	virtual bool setBoneBindPose(physx::PxU32 boneIndex, const physx::PxMat44& bindPose);
	virtual bool getBoneBindPose(physx::PxU32 boneIndex, physx::PxMat44& bindPose);

private:
	// bones
	PxU32									addBoneConvexInternal(PxU32 boneIndex, const PxVec3* positions, PxU32 numPositions);
	void									addBoneCapsuleInternal(physx::PxU32 boneIndex, physx::PxF32 capsuleRadius, physx::PxF32 capsuleHeight, const physx::PxMat44& localPose);
	void									clearBoneActorsInternal(physx::PxI32 internalBoneIndex);
	void									compressBones() const;
	void									compressBoneCollision();
	void									collectBoneIndices(physx::PxU32 numVertices, const physx::PxU16* boneIndices, const physx::PxF32* boneWeights, physx::PxU32 numBonesPerVertex) const;

	void									updateMappingAuthoring(ClothingGraphicalLodParameters& graphLod, NiApexRenderMeshAsset* renderMeshAssetCopy,
	        NiApexRenderMeshAssetAuthoring* renderMeshAssetOrig, PxF32 normalResemblance, bool ignoreUnusedVertices, IProgressListener* progress);
	void									sortSkinMapB(SkinClothMapB* skinClothMap, physx::PxU32 skinClothMapSize, physx::PxU32* immediateClothMap, physx::PxU32 immediateClothMapSize);

	void									setupPhysicalLods(ClothingPhysicalMeshParameters& physicalMeshParameters, physx::PxU32 numMaxDistReductions, physx::PxF32* borderMaxdistances) const;

	void									distributeSolverIterations();

	bool									checkSetMeshesInput(PxU32 lod, NxClothingPhysicalMesh* nxPhysicalMesh, PxU32& graphicalLodIndexTest);
	void									sortPhysicalMeshes();

	// mesh reordering
	void									sortDeformableIndices(ClothingPhysicalMesh& physicalMesh);


	bool									getGraphicalLodIndex(physx::PxU32 lod, physx::PxU32& graphicalLodIndex);
	PxU32									addGraphicalLod(physx::PxU32 lod);

	// cooking
	void									clearCooked();

	// access
	bool									addGraphicalMesh(NxRenderMeshAssetAuthoring* renderMesh, physx::PxU32 graphicalLodIndex);

	Array<ClothingPhysicalMesh*>			mPhysicalMeshesInput;

	PxF32									mExportScale;
	bool									mDeriveNormalsFromBones;
	bool									mOwnsMaterialLibrary;

	NxClothingConstrainCoefficients			mDefaultConstrainCoefficients;
	NxClothingConstrainCoefficients			mInvalidConstrainCoefficients;

	const char*								mPreviousCookedType;

	ApexSimpleString						mRootBoneName;

	void									initParams();

	// immediate cloth: 1-to-1 mapping from physical to rendering mesh (except for LOD)
	bool									generateImmediateClothMap(const NxAbstractMeshDescription* targetMeshes, PxU32 numTargetMeshes,
	        ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh, PxU32* masterFlags, PxF32 epsilon,
	        PxU32& numNotFoundVertices, PxF32 normalResemblance, NxParamArray<PxU32>& result, IProgressListener* progress) const;
	bool									generateSkinClothMap(const NxAbstractMeshDescription* targetMeshes, PxU32 numTargetMeshes,
	        ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh, PxU32* masterFlags, PxU32* immediateMap,
	        PxU32 numEmptyInImmediateMap, NxParamArray<ClothingGraphicalLodParametersNS::SkinClothMapD_Type>& result,
	        PxF32& offsetAlongNormal, bool integrateImmediateMap, IProgressListener* progress) const;
	bool									generateTetraMap(const NxAbstractMeshDescription* targetMeshes, PxU32 numTargetMeshes,
	        ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh, PxU32* masterFlags,
	        NxParamArray<ClothingGraphicalLodParametersNS::TetraLink_Type>& result, IProgressListener* progress) const;
	PxF32									computeBaryError(PxF32 baryX, PxF32 baryY) const;
	PxF32									computeTriangleError(const TriangleWithNormals& triangle, const PxVec3& normal) const;


	bool									hasTangents(const NiApexRenderMeshAsset& rma);
	PxU32									getMaxNumGraphicalVertsActive(const ClothingGraphicalLodParameters& graphicalLod, PxU32 submeshIndex);
	bool									isMostlyImmediateSkinned(const NiApexRenderMeshAsset& rma, const ClothingGraphicalLodParameters& graphicalLod);
	bool									conditionalMergeMapping(const NiApexRenderMeshAsset& rma, ClothingGraphicalLodParameters& graphicalLod);

};

}
} // namespace apex
} // namespace physx

#endif // WITHOUT_APEX_AUTHORING

#endif // CLOTHING_ASSET_AUTHORING_H
