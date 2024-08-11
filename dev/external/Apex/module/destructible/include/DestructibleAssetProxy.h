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

#ifndef DESTRUCTIBLE_ASSET_PROXY_H
#define DESTRUCTIBLE_ASSET_PROXY_H

#include "ModuleDestructible.h"
#include "NxDestructibleAsset.h"
#include "DestructibleAsset.h"
#include "PsUserAllocated.h"
#include "authoring/Fracturing.h"
#include "PsShare.h"
#include "ApexAuthorableObject.h"
#include "ApexAssetAuthoring.h"

#include "PsArray.h"
#include "NxParameterized.h"

#pragma warning(disable: 4355) // 'this' : used in base member initialization list

namespace physx
{
namespace apex
{
namespace destructible
{

class DestructibleAssetProxy : public NxDestructibleAsset, public NxApexResource, public physx::UserAllocated
{
public:
	DestructibleAsset impl;

	DestructibleAssetProxy(ModuleDestructible* module, NxResourceList& list, const char* name) :
		impl(module, this, name)
	{
		list.add(*this);
	}

	DestructibleAssetProxy(ModuleDestructible* module, NxResourceList& list, NxParameterized::Interface* params, const char* name) :
		impl(module, this, params, name)
	{
		list.add(*this);
	}

	~DestructibleAssetProxy()
	{
	}

	const NxParameterized::Interface* getAssetNxParameterized() const
	{
		return impl.getAssetNxParameterized();
	}

	/**
	* \brief Releases the ApexAsset but returns the NxParameterized::Interface and *ownership* to the caller.
	*/
	virtual NxParameterized::Interface* releaseAndReturnNxParameterizedInterface(void)
	{
		NxParameterized::Interface* ret = impl.releaseAndReturnNxParameterizedInterface();
		release();
		return ret;
	}

	virtual void releaseDestructibleActor(NxDestructibleActor& actor)
	{
		impl.releaseDestructibleActor(actor);
	}
	virtual NxDestructibleParameters getDestructibleParameters() const
	{
		return impl.getParameters();
	}
	virtual NxDestructibleInitParameters getDestructibleInitParameters() const
	{
		return impl.getInitParameters();
	}
	virtual physx::PxU32 getChunkCount() const
	{
		return impl.getChunkCount();
	}
	virtual physx::PxU32 getDepthCount() const
	{
		return impl.getDepthCount();
	}
	virtual NxRenderMeshAsset* getRenderMeshAsset() const
	{
		return impl.getRenderMeshAsset();
	}
	virtual bool setRenderMeshAsset(NxRenderMeshAsset* renderMeshAsset)
	{
		return impl.setRenderMeshAsset(renderMeshAsset);
	}
	virtual physx::PxU32 getInstancedChunkCount() const
	{
		return impl.getInstancedChunkCount();
	}
	virtual const char* getCrumbleEmitterName() const
	{
		return impl.getCrumbleEmitterName();
	}
	virtual const char* getDustEmitterName() const
	{
		return impl.getDustEmitterName();
	}
	virtual const char* getFracturePatternName() const
	{
		return impl.getFracturePatternName();
	}
	virtual void getStats(NxDestructibleAssetStats& stats) const
	{
		impl.getStats(stats);
	}
	virtual void cacheChunkOverlapsUpToDepth(physx::PxI32 depth = -1)
	{
		impl.cacheChunkOverlapsUpToDepth(depth);
	}

	virtual physx::PxU32 getCachedOverlapCountAtDepth(physx::PxU32 depth)
	{
		CachedOverlapsNS::IntPair_DynamicArray1D_Type* pairArray = impl.getOverlapsAtDepth(depth, false);
		return pairArray != NULL ? pairArray->arraySizes[0] : 0;
	}

	virtual const NxIntPair* getCachedOverlapsAtDepth(physx::PxU32 depth)
	{
		CachedOverlapsNS::IntPair_DynamicArray1D_Type* pairArray = impl.getOverlapsAtDepth(depth, false);
		PX_ASSERT(sizeof(NxIntPair) == pairArray->elementSize);
		return pairArray != NULL ? (const NxIntPair*)pairArray->buf : NULL;
	}

	NxParameterized::Interface* getDefaultActorDesc()
	{
		return impl.getDefaultActorDesc();
	}

	NxParameterized::Interface* getDefaultAssetPreviewDesc()
	{
		return impl.getDefaultAssetPreviewDesc();
	}

	virtual NxApexActor* createApexActor(const NxParameterized::Interface& parms, NxApexScene& apexScene)
	{
		return impl.createApexActor(parms, apexScene);
	}

	virtual NxDestructibleActor* createDestructibleActorFromDeserializedState(NxParameterized::Interface* parms, NxApexScene& apexScene)
	{
		return impl.createDestructibleActorFromDeserializedState(parms, apexScene);
	}

	virtual NxApexAssetPreview* createApexAssetPreview(const NxParameterized::Interface& params, NxApexAssetPreviewScene* /*previewScene*/)
	{
		return impl.createApexAssetPreview(params, NULL);
	}

	virtual bool isValidForActorCreation(const ::NxParameterized::Interface& parms, NxApexScene& apexScene) const
	{
		return impl.isValidForActorCreation(parms, apexScene);
	}

	virtual bool isDirty() const
	{
		return false;
	}

	virtual physx::PxVec3 getChunkPositionOffset(physx::PxU32 chunkIndex) const
	{
		return impl.getChunkPositionOffset(chunkIndex);
	}

	virtual physx::PxVec2 getChunkUVOffset(physx::PxU32 chunkIndex) const
	{
		return impl.getChunkUVOffset(chunkIndex);
	}

	virtual physx::PxU32 getChunkFlags(physx::PxU32 chunkIndex) const
	{
		return impl.getChunkFlags(chunkIndex);
	}

	virtual physx::PxU16 getChunkDepth(physx::PxU32 chunkIndex) const
	{
		return impl.getChunkDepth(chunkIndex);
	}

	virtual physx::PxI32 getChunkParentIndex(physx::PxU32 chunkIndex) const
	{
		const physx::PxU16 chunkParentIndex = impl.getChunkParentIndex(chunkIndex);
		return chunkParentIndex != DestructibleAsset::InvalidChunkIndex ? (physx::PxI32)chunkParentIndex : -1;
	}

	virtual physx::PxU32 getPartIndex(physx::PxU32 chunkIndex) const
	{
		return impl.getPartIndex(chunkIndex);
	}

	virtual physx::PxU32 getPartConvexHullCount(const physx::PxU32 partIndex) const
	{
		return impl.getPartHullIndexStop(partIndex) - impl.getPartHullIndexStart(partIndex);
	}

	virtual NxParameterized::Interface** getPartConvexHullArray(const physx::PxU32 partIndex) const
	{
		return impl.getConvexHullParameters(impl.getPartHullIndexStart(partIndex));
	}

	physx::PxU32 getActorTransformCount() const
	{
		return impl.getActorTransformCount();
	}
	const physx::PxMat44* getActorTransforms() const
	{
		return impl.getActorTransforms();
	}

	void applyTransformation(const physx::PxMat44& transformation, physx::PxF32 scale)
	{
		impl.applyTransformation(transformation, scale);
	}
	void applyTransformation(const physx::PxMat44& transformation)
	{
		impl.applyTransformation(transformation);
	}


	// NxApexInterface methods
	virtual void release()
	{
		impl.getOwner()->mSdk->releaseAsset(*this);
	}
	virtual void destroy()
	{
		impl.cleanup();
		delete this;
	}

	// NxApexAsset methods
	virtual const char* getName() const
	{
		return impl.getName();
	}
	static const char* 		getClassName()
	{
		return NX_DESTRUCTIBLE_AUTHORING_TYPE_NAME;
	}
	virtual NxAuthObjTypeID	getObjTypeID() const
	{
		return impl.getObjTypeID();
	}
	virtual const char* getObjTypeName() const
	{
		return impl.getObjTypeName();
	}
	virtual physx::PxU32 forceLoadAssets()
	{
		return impl.forceLoadAssets();
	}

	// NxApexResource methods
	virtual void	setListIndex(NxResourceList& list, physx::PxU32 index)
	{
		impl.m_listIndex = index;
		impl.m_list = &list;
	}
	virtual physx::PxU32	getListIndex() const
	{
		return impl.m_listIndex;
	}

	friend class DestructibleActor;
};

#ifndef WITHOUT_APEX_AUTHORING
class DestructibleAssetAuthoringProxy : public NxDestructibleAssetAuthoring, public NxApexResource, public ApexAssetAuthoring, public physx::UserAllocated
{
public:
	DestructibleAssetAuthoring impl;

	DestructibleAssetAuthoringProxy(ModuleDestructible* module, NxResourceList& list) :
		impl(module, NULL, "DestructibleAuthoring")
	{
		list.add(*this);
	}

	DestructibleAssetAuthoringProxy(ModuleDestructible* module, NxResourceList& list, const char* name) :
		impl(module, NULL, name)
	{
		list.add(*this);
	}

	DestructibleAssetAuthoringProxy(ModuleDestructible* module, NxResourceList& list, NxParameterized::Interface* params, const char* name) :
		impl(module, NULL, params, name)
	{
		list.add(*this);
	}

	virtual ~DestructibleAssetAuthoringProxy()
	{
	}

	// NxDestructibleAssetAuthoring methods

	virtual IExplicitHierarchicalMesh& getExplicitHierarchicalMesh()
	{
		return impl.hMesh;
	}

	virtual IExplicitHierarchicalMesh& getCoreExplicitHierarchicalMesh()
	{
		return impl.hMeshCore;
	}

	virtual ICutoutSet&	getCutoutSet()
	{
		return impl.cutoutSet;
	}

	virtual bool	setRootMesh
	(
	    const NxExplicitRenderTriangle* meshTriangles,
	    physx::PxU32 meshTriangleCount,
	    const NxExplicitSubmeshData* submeshData,
	    physx::PxU32 submeshCount,
	    physx::PxU32* meshPartition = NULL,
	    physx::PxU32 meshPartitionCount = 0,
		bool firstPartitionIsDepthZero = false
	)
	{
		return FractureTools::buildExplicitHierarchicalMesh(impl.hMesh, meshTriangles, meshTriangleCount, submeshData, submeshCount, meshPartition, meshPartitionCount, firstPartitionIsDepthZero);
	}

	virtual bool	importRenderMeshAssetToRootMesh(const physx::NxRenderMeshAsset& renderMeshAsset)
	{
		return FractureTools::buildExplicitHierarchicalMeshFromRenderMeshAsset(impl.hMesh, renderMeshAsset);
	}

	virtual bool	importDestructibleAssetToRootMesh(const NxDestructibleAsset& destructibleAsset)
	{
		return FractureTools::buildExplicitHierarchicalMeshFromDestructibleAsset(impl.hMesh, destructibleAsset);
	}

	virtual bool	setCoreMesh
	(
	    const NxExplicitRenderTriangle* meshTriangles,
	    physx::PxU32 meshTriangleCount,
	    const NxExplicitSubmeshData* submeshData,
	    physx::PxU32 submeshCount,
	    physx::PxU32* meshPartition = NULL,
	    physx::PxU32 meshPartitionCount = 0
	)
	{
		return FractureTools::buildExplicitHierarchicalMesh(impl.hMeshCore, meshTriangles, meshTriangleCount, submeshData, submeshCount, meshPartition, meshPartitionCount);
	}

	virtual bool buildExplicitHierarchicalMesh
	(
		IExplicitHierarchicalMesh& hMesh,
		const NxExplicitRenderTriangle* meshTriangles,
		physx::PxU32 meshTriangleCount,
		const NxExplicitSubmeshData* submeshData,
		physx::PxU32 submeshCount,
		physx::PxU32* meshPartition = NULL,
		physx::PxU32 meshPartitionCount = 0,
		bool firstPartitionIsDepthZero = false
	)
	{
		return FractureTools::buildExplicitHierarchicalMesh(hMesh, meshTriangles, meshTriangleCount, submeshData, submeshCount, meshPartition, meshPartitionCount, firstPartitionIsDepthZero);
	}

	virtual bool	createHierarchicallySplitMesh
	(
	    const MeshProcessingParameters& meshProcessingParams,
	    const FractureSliceDesc& desc,
	    const NxCollisionDesc& collisionDesc,
	    bool exportCoreMesh,
		physx::PxI32 coreMeshImprintSubmeshIndex,
	    physx::PxU32 randomSeed,
	    IProgressListener& progressListener,
	    volatile bool* cancel = NULL
	)
	{
		return FractureTools::createHierarchicallySplitMesh(impl.hMesh, impl.hMeshCore, exportCoreMesh, coreMeshImprintSubmeshIndex,
		        meshProcessingParams, desc, collisionDesc, randomSeed, progressListener, cancel);
	}

	virtual bool	createVoronoiSplitMesh
	(
		const FractureTools::MeshProcessingParameters& meshProcessingParams,
		const FractureTools::FractureVoronoiDesc& desc,
		const NxCollisionDesc& collisionDesc,
		bool exportCoreMesh,
		physx::PxI32 coreMeshImprintSubmeshIndex,
		physx::PxU32 randomSeed,
		IProgressListener& progressListener,
		volatile bool* cancel = NULL
	)
	{
		return FractureTools::createVoronoiSplitMesh(impl.hMesh, impl.hMeshCore, exportCoreMesh, coreMeshImprintSubmeshIndex,
				meshProcessingParams, desc, collisionDesc, randomSeed, progressListener, cancel);
	}

	virtual bool	hierarchicallySplitChunk
	(
		physx::PxU32 chunkIndex,
	    const FractureTools::MeshProcessingParameters& meshProcessingParams,
	    const FractureTools::FractureSliceDesc& desc,
	    const physx::NxCollisionDesc& collisionDesc,
	    physx::PxU32* randomSeed,
	    IProgressListener& progressListener,
	    volatile bool* cancel = NULL
	)
	{
		return FractureTools::hierarchicallySplitChunk(impl.hMesh, chunkIndex, meshProcessingParams, desc, collisionDesc, randomSeed, progressListener, cancel);
	}

	virtual bool	voronoiSplitChunk
		(
		physx::PxU32 chunkIndex,
		const FractureTools::MeshProcessingParameters& meshProcessingParams,
		const FractureTools::FractureVoronoiDesc& desc,
		const physx::NxCollisionDesc& collisionDesc,
		physx::PxU32* randomSeed,
		IProgressListener& progressListener,
		volatile bool* cancel = NULL
		)
	{
		return FractureTools::voronoiSplitChunk(impl.hMesh, chunkIndex, meshProcessingParams, desc, collisionDesc, randomSeed, progressListener, cancel);
	}

	virtual bool	createChippedMesh
	(
	    const MeshProcessingParameters& meshProcessingParams,
	    const FractureCutoutDesc& desc,
	    const ICutoutSet& iCutoutSet,
	    const FractureSliceDesc& sliceDesc,
		const physx::FractureVoronoiDesc& voronoiDesc,
	    const physx::NxCollisionDesc& collisionDesc,
	    physx::PxU32 randomSeed,
	    IProgressListener& progressListener,
	    volatile bool* cancel = NULL
	)
	{
		return FractureTools::createChippedMesh(impl.hMesh, meshProcessingParams, desc, iCutoutSet, sliceDesc, voronoiDesc, collisionDesc, randomSeed, progressListener, cancel);
	}

	virtual void	buildCutoutSet
	(
	    const physx::PxU8* pixelBuffer,
	    physx::PxU32 bufferWidth,
	    physx::PxU32 bufferHeight,
	    physx::PxF32 snapThreshold,
		bool periodic
	)
	{
		FractureTools::buildCutoutSet(impl.cutoutSet, pixelBuffer, bufferWidth, bufferHeight, snapThreshold, periodic);
	}

	virtual bool	calculateCutoutUVMapping(physx::PxMat33& mapping, const physx::NxExplicitRenderTriangle& triangle)
	{
		return FractureTools::calculateCutoutUVMapping(triangle, mapping);
	}

	virtual bool	calculateCutoutUVMapping(physx::PxMat33& mapping, const physx::PxVec3& targetDirection)
	{
		return FractureTools::calculateCutoutUVMapping(impl.hMesh, targetDirection, mapping);
	}

	virtual void	createVoronoiSitesInsideMesh
	(
		physx::PxVec3* siteBuffer,
		physx::PxU32 siteCount,
		physx::PxU32* randomSeed,
		physx::PxU32* microgridSize,
		IProgressListener& progressListener,
		physx::PxU32 chunkIndex = 0xFFFFFFFF
	)
	{
		return FractureTools::createVoronoiSitesInsideMesh(impl.hMesh, siteBuffer, siteCount, randomSeed, microgridSize, progressListener, chunkIndex);
	}

	physx::PxU32	createScatterMeshSites
	(
		physx::PxU8*						meshIndices,
		physx::PxMat44*						relativeTransforms,
		physx::PxU32*						chunkMeshStarts,
		physx::PxU32						scatterMeshInstancesBufferSize,
		physx::PxU32						targetChunkCount,
		const physx::PxU16*					targetChunkIndices,
		physx::PxU32*						randomSeed,
		physx::PxU32						scatterMeshAssetCount,
		physx::NxRenderMeshAsset**			scatterMeshAssets,
		const physx::PxU32*					minCount,
		const physx::PxU32*					maxCount,
		const physx::PxF32*					minScales,
		const physx::PxF32*					maxScales,
		const physx::PxF32*					maxAngles
	)
	{
		return FractureTools::createScatterMeshSites(meshIndices, relativeTransforms, chunkMeshStarts, scatterMeshInstancesBufferSize, impl.hMesh, targetChunkCount, targetChunkIndices,
													 randomSeed, scatterMeshAssetCount, scatterMeshAssets, minCount, maxCount, minScales, maxScales, maxAngles);
	}

	virtual void	visualizeVoronoiCells
	(
		physx::NxApexRenderDebug& debugRender,
		const physx::PxVec3* sites,
		physx::PxU32 siteCount,
		const physx::PxU32* cellColors,
		physx::PxU32 cellColorCount,
		const physx::PxBounds3& bounds,
		physx::PxU32 cellIndex = 0xFFFFFFFF
	)
	{
		FractureTools::visualizeVoronoiCells(debugRender, sites, siteCount, cellColors, cellColorCount, bounds, cellIndex);
	}

	virtual void	setBSPTolerances
	(
		physx::PxF32 linearTolerance,
		physx::PxF32 angularTolerance,
		physx::PxF32 baseTolerance,
		physx::PxF32 clipTolerance,
		physx::PxF32 cleaningTolerance
	)
	{
		FractureTools::setBSPTolerances(linearTolerance, angularTolerance, baseTolerance, clipTolerance, cleaningTolerance);
	}

	virtual void	setBSPBuildParameters
	(
		physx::PxF32 logAreaSigmaThreshold,
		physx::PxU32 testSetSize,
		physx::PxF32 splitWeight,
		physx::PxF32 imbalanceWeight
	)
	{
		FractureTools::setBSPBuildParameters(logAreaSigmaThreshold, testSetSize, splitWeight, imbalanceWeight);
	}

	virtual IExplicitHierarchicalMesh::IConvexHull*	createExplicitHierarchicalMeshConvexHull()
	{
		return FractureTools::createExplicitHierarchicalMeshConvexHull();
	}

	virtual physx::PxU32 buildSliceMesh(const NxExplicitRenderTriangle*& mesh, const FractureTools::NoiseParameters& noiseParameters, const physx::PxPlane& slicePlane, physx::PxU32 randomSeed)
	{
		if( FractureTools::buildSliceMesh(impl.intersectMesh, impl.hMesh, slicePlane, noiseParameters, randomSeed) )
		{
			mesh = impl.intersectMesh.m_triangles.begin();
			return impl.intersectMesh.m_triangles.size();
		}

		mesh = NULL;
		return 0;
	}

	virtual void setChunkOverlapsCacheDepth(bool enabled, physx::PxI32 depth = -1)
	{
		impl.setChunkOverlapsCacheDepth(enabled, depth);
	}
	virtual NxRenderMeshAsset* getRenderMeshAsset() const
	{
		return impl.getRenderMeshAsset();
	}
	virtual bool setRenderMeshAsset(NxRenderMeshAsset* renderMeshAsset)
	{
		return impl.setRenderMeshAsset(renderMeshAsset);
	}
	virtual bool setScatterMeshAssets(NxRenderMeshAsset** scatterMeshAssetArray, physx::PxU32 scatterMeshAssetArraySize)
	{
		return impl.setScatterMeshAssets(scatterMeshAssetArray, scatterMeshAssetArraySize);
	}
	virtual physx::PxU32 getScatterMeshAssetCount() const
	{
		return impl.getScatterMeshAssetCount();
	}
	virtual NxRenderMeshAsset** getScatterMeshAssets()
	{
		return impl.getScatterMeshAssets();
	}
	virtual physx::PxU32 getInstancedChunkCount() const
	{
		return impl.getInstancedChunkCount();
	}
	virtual void setDestructibleParameters(const NxDestructibleParameters& parameters)
	{
		impl.setParameters(parameters);
	}
	virtual NxDestructibleParameters getDestructibleParameters() const
	{
		return impl.getParameters();
	}
	virtual void setDestructibleInitParameters(const NxDestructibleInitParameters& parameters)
	{
		impl.setInitParameters(parameters);
	}
	virtual NxDestructibleInitParameters	getDestructibleInitParameters() const
	{
		return impl.getInitParameters();
	}
	virtual void setCrumbleEmitterName(const char* name)
	{
		impl.setCrumbleEmitterName(name);
	}
	virtual void setDustEmitterName(const char* name)
	{
		impl.setDustEmitterName(name);
	}
	virtual void setFracturePatternName(const char* name)
	{
		impl.setFracturePatternName(name);
	}
	virtual void cookChunks(const NxDestructibleAssetCookingDesc& cookingDesc)
	{
		impl.cookChunks(cookingDesc);
	}
	virtual void serializeFractureToolState(physx::PxFileBuf& stream, physx::IExplicitHierarchicalMesh::IEmbedding& embedding) const
	{
		impl.serializeFractureToolState(stream, embedding);
	}
	virtual void deserializeFractureToolState(physx::PxFileBuf& stream, physx::IExplicitHierarchicalMesh::IEmbedding& embedding)
	{
		impl.deserializeFractureToolState(stream, embedding);
	}
	virtual physx::PxU32 getChunkCount() const
	{
		return impl.getChunkCount();
	}
	virtual physx::PxU32 getDepthCount() const
	{
		return impl.getDepthCount();
	}
	virtual physx::PxU32 getChunkChildCount(physx::PxU32 chunkIndex) const
	{
		return impl.getChunkChildCount(chunkIndex);
	}
	virtual physx::PxI32 getChunkChild(physx::PxU32 chunkIndex, physx::PxU32 childIndex) const
	{
		return impl.getChunkChild(chunkIndex, childIndex);
	}
	virtual physx::PxVec3 getChunkPositionOffset(physx::PxU32 chunkIndex) const
	{
		return impl.getChunkPositionOffset(chunkIndex);
	}
	virtual physx::PxVec2 getChunkUVOffset(physx::PxU32 chunkIndex) const
	{
		return impl.getChunkUVOffset(chunkIndex);
	}
	virtual physx::PxU32 getPartIndex(physx::PxU32 chunkIndex) const
	{
		return impl.getPartIndex(chunkIndex);
	}
	virtual bool rebuildCollisionGeometry(physx::PxU32 partIndex, const NxDestructibleGeometryDesc& geometryDesc)
	{
		return impl.rebuildCollisionGeometry(partIndex, geometryDesc);
	}
	virtual void trimCollisionGeometry(const physx::PxU32* partIndices, physx::PxU32 partIndexCount, physx::PxF32 maxTrimFraction = 0.2f)
	{
		impl.trimCollisionGeometry(partIndices, partIndexCount, maxTrimFraction);
	}
	virtual physx::PxF32 getImpactVelocityThreshold() const
	{
		return impl.getImpactVelocityThreshold();
	}
	void setImpactVelocityThreshold(physx::PxF32 threshold)
	{
		impl.setImpactVelocityThreshold(threshold);
	}
	virtual physx::PxF32 getFractureImpulseScale() const
	{
		return impl.getFractureImpulseScale();
	}
	void setFractureImpulseScale(physx::PxF32 scale)
	{
		impl.setFractureImpulseScale(scale);
	}
	virtual void getStats(NxDestructibleAssetStats& stats) const
	{
		impl.getStats(stats);
	}
	virtual void setNeighborPadding(physx::PxF32 neighborPadding)
	{
		impl.setNeighborPadding(neighborPadding);
	}
	virtual physx::PxF32 getNeighborPadding() const
	{
		return impl.getNeighborPadding();
	}
	void applyTransformation(const physx::PxMat44& transformation, physx::PxF32 scale)
	{
		impl.applyTransformation(transformation, scale);
	}
	void applyTransformation(const physx::PxMat44& transformation)
	{
		impl.applyTransformation(transformation);
	}

	bool setPlatformMaxDepth(NxPlatformTag platform, physx::PxU32 maxDepth)
	{
		return impl.setPlatformMaxDepth(platform, maxDepth);
	}

	bool removePlatformMaxDepth(NxPlatformTag platform)
	{
		return impl.removePlatformMaxDepth(platform);
	}

	// NxApexAssetAuthoring methods

	const char* getName(void) const
	{
		return impl.getName();
	}

	const char* getObjTypeName() const
	{
		return impl.getObjTypeName();
	}

	virtual bool prepareForPlatform(physx::apex::NxPlatformTag platformTag)
	{
		return impl.prepareForPlatform(platformTag);
	}

	void setToolString(const char* toolName, const char* toolVersion, PxU32 toolChangelist)
	{
		ApexAssetAuthoring::setToolString(toolName, toolVersion, toolChangelist);
	}

	void setToolString(const char* toolString)
	{
		impl.setToolString(toolString);
	}

	physx::PxU32 getActorTransformCount() const
	{
		return impl.getActorTransformCount();
	}

	const physx::PxMat44* getActorTransforms() const
	{
		return impl.getActorTransforms();
	}

	void appendActorTransforms(const physx::PxMat44* transforms, physx::PxU32 transformCount)
	{
		impl.appendActorTransforms(transforms, transformCount);
	}

	void clearActorTransforms()
	{
		impl.clearActorTransforms();
	}

	NxParameterized::Interface* getNxParameterized() const
	{
		return (NxParameterized::Interface*)impl.getAssetNxParameterized();
	}

	/**
	* \brief Releases the ApexAsset but returns the NxParameterized::Interface and *ownership* to the caller.
	*/
	virtual NxParameterized::Interface* releaseAndReturnNxParameterizedInterface(void)
	{
		return impl.releaseAndReturnNxParameterizedInterface();
	}


	// NxApexResource methods
	virtual void	setListIndex(NxResourceList& list, physx::PxU32 index)
	{
		impl.m_listIndex = index;
		impl.m_list = &list;
	}
	virtual physx::PxU32	getListIndex() const
	{
		return impl.m_listIndex;
	}

	// NxApexInterface methods
	virtual void release()
	{
		impl.getOwner()->mSdk->releaseAssetAuthoring(*this);
	}
	virtual void destroy()
	{
		impl.cleanup();
		delete this;
	}
};
#endif

}
}
} // end namespace physx::apex

#endif // DESTRUCTIBLE_ASSET_PROXY_H
