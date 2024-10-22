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

#ifndef APEX_RENDERMESH_ACTOR_H
#define APEX_RENDERMESH_ACTOR_H

#include "NxApexUsingNamespace.h"
#include "NiApexRenderMeshAsset.h"
#include "ApexActor.h"

#include "ApexRenderMeshAsset.h"
#include "ApexSharedUtils.h"



namespace physx
{
namespace apex
{

// enables a hack that removes dead particles from the instance list prior to sending it to the application.
// this is a bad hack because it requires significant additional memory and copies.
#define ENABLE_INSTANCED_MESH_CLEANUP_HACK 0


/*
	ApexRenderMeshActor - an instantiation of an ApexRenderMeshAsset
 */
class ApexRenderMeshActor : public NiApexRenderMeshActor, public NxApexResource, public ApexResource, public ApexActor
{
public:
	void					release();
	NxApexAsset*			getOwner() const
	{
		return mRenderMeshAsset;
	}
	physx::PxBounds3		getBounds() const
	{
		return ApexRenderable::getBounds();
	}
	void					lockRenderResources()
	{
		ApexRenderable::renderDataLock();
	}
	void					unlockRenderResources()
	{
		ApexRenderable::renderDataUnLock();
	}

	// RenderMeshActors have global context, ignore ApexActor scene methods
#if NX_SDK_VERSION_MAJOR == 2
	void					setPhysXScene(NxScene*)		{ }
	NxScene*				getPhysXScene() const
	{
		return NULL;
	}
#else
	void					setPhysXScene(PxScene*)		{ }
	PxScene*				getPhysXScene() const
	{
		return NULL;
	}
#endif

	bool					getVisibilities(physx::PxU8* visibilityArray, physx::PxU32 visibilityArraySize) const;

	bool					setVisibility(bool visible, physx::PxU16 partIndex = 0);
	bool					isVisible(physx::PxU16 partIndex = 0) const
	{
		return mVisiblePartsForAPI.isUsed(partIndex);
	}

	PxU32 visiblePartCount() const
	{
		return mVisiblePartsForAPI.usedCount();
	}
	const PxU32* getVisibleParts() const
	{
		return mVisiblePartsForAPI.usedIndices();
	}

	PxU32 getRenderVisiblePartCount() const
	{
		return mBufferVisibility ? mVisiblePartsForRendering.size() : mVisiblePartsForAPI.usedCount();
	}
	const PxU32* getRenderVisibleParts() const
	{
		return mBufferVisibility ? mVisiblePartsForRendering.begin() : mVisiblePartsForAPI.usedIndices();
	}

	virtual PxU32 getBoneCount() const
	{
		return mRenderMeshAsset->getBoneCount();
	}

	void setTM(const physx::PxMat44& tm, physx::PxU32 boneIndex = 0);
	void setTM(const physx::PxMat44& tm, const physx::PxVec3& scale, physx::PxU32 boneIndex = 0);

	const PxMat44 getTM(physx::PxU32 boneIndex = 0) const
	{
		return accessTM(boneIndex);
	}

	void setLastFrameTM(const physx::PxMat44& tm, physx::PxU32 boneIndex = 0);
	void setLastFrameTM(const physx::PxMat44& tm, const physx::PxVec3& scale, physx::PxU32 boneIndex = 0);

	//++LAVA
	void setChunkAlpha( physx::PxF32 alpha, physx::PxU32 boneIndex = 0 );
	//--LAVA

	void					syncVisibility(bool useLock = true);

	void					updateBounds();
	void					updateRenderResources(bool rewriteBuffers, void* userRenderData);
	void					updateRenderResources(bool useBones, bool rewriteBuffers, void* userRenderData);
	void					dispatchRenderResources(NxUserRenderer&);
	void					dispatchRenderResources(NxUserRenderer&, const physx::PxMat44&);

	void					setReleaseResourcesIfNothingToRender(bool value);

	void					setBufferVisibility(bool bufferVisibility);

	void					setOverrideMaterial(PxU32 index, const char* overrideMaterialName);

	//NxUserRenderVertexBuffer* getUserVertexBuffer(physx::PxU32 submeshIndex) { if (submeshIndex < mSubmeshData.size()) return renderMeshAsset->vertexBuffers[submeshIndex]; return NULL; }
	//NxUserRenderIndexBuffer* getUserIndexBuffer(physx::PxU32 submeshIndex) { if (submeshIndex < mSubmeshData.size()) return mSubmeshData[submeshIndex].indexBuffer; return NULL; }

	void					addVertexBuffer(PxU32 submeshIndex, bool alwaysDirty, PxVec3* position, PxVec3* normal, PxVec4* tangents);
	void					removeVertexBuffer(PxU32 submeshIndex);

	void					setStaticPositionReplacement(PxU32 submeshIndex, const PxVec3* staticPositions);
	void					setStaticColorReplacement(PxU32 submeshIndex, const PxColorRGBA* staticColors);

	virtual NxUserRenderInstanceBuffer* getInstanceBuffer() const
	{
		return mInstanceBuffer;
	}
	/// \sa NxApexRenderMeshActor::setInstanceBuffer
	virtual void			setInstanceBuffer(NxUserRenderInstanceBuffer* instBuf);
	/// \sa NxApexRenderMeshActor::setInstanceBufferRange
	virtual void			setInstanceBufferRange(physx::PxU32 from, physx::PxU32 count);

	// NxApexResource methods
	void					setListIndex(NxResourceList& list, physx::PxU32 index)
	{
		m_listIndex = index;
		m_list = &list;
	}
	PxU32					getListIndex() const
	{
		return m_listIndex;
	}

	virtual void			getPhysicalLodRange(PxF32& min, PxF32& max, bool& intOnly);
	virtual PxF32			getActivePhysicalLod();
	virtual void			forcePhysicalLod(PxF32 lod);

	virtual bool			rayCast(NxRenderMeshActorRaycastHitData& hitData,
	                                const PxVec3& worldOrig, const PxVec3& worldDisp,
	                                NxRenderMeshActorRaycastFlags::Enum flags = NxRenderMeshActorRaycastFlags::VISIBLE_PARTS,
	                                NxRenderCullMode::Enum winding = NxRenderCullMode::CLOCKWISE,
	                                PxI32 partIndex = -1) const;

	virtual void			visualize(NiApexRenderDebug& batcher, physx::apex::DebugRenderParams* debugParams, PxMat33* scaledRotations, PxVec3* translations, PxU32 stride, PxU32 numberOfTransforms) const;

protected:
	ApexRenderMeshActor(const NxRenderMeshActorDesc& desc, ApexRenderMeshAsset& _renderMeshData, NxResourceList& list);
	virtual					~ApexRenderMeshActor();

	void					init(const NxRenderMeshActorDesc& desc, PxU16 partCount, PxU16 boneCount);
	void					destroy();

	PxMat34Legacy&	accessTM(PxU32 boneIndex = 0) const
	{
		return (PxMat34Legacy&)mTransforms[mKeepVisibleBonesPacked ? mVisiblePartsForAPI.getRank(boneIndex) : boneIndex];
	}

	PxMat34Legacy&	accessLastFrameTM(PxU32 boneIndex = 0) const
	{
		return (PxMat34Legacy&)mTransformsLastFrame[mKeepVisibleBonesPacked ? mVisiblePartsForAPILastFrame.getRank(boneIndex) : boneIndex];
	}

	/* Internal rendering APIs */
	void					createRenderResources(bool useBones, void* userRenderData);
	void					updatePartVisibility(PxU32 submeshIndex, bool useBones, void* userRenderData);
	void					updateBonePoses(PxU32 submeshIndex);
	void					updateInstances(PxU32 submeshIndex);
	void					releaseSubmeshRenderResources(PxU32 submeshIndex);
	void					releaseRenderResources();
	bool					submeshHasVisibleTriangles(PxU32 submeshIndex) const;

	// Fallback skinning
	void					createFallbackSkinning(PxU32 submeshIndex);
	void					distributeFallbackData(PxU32 submeshIndex);
	void					updateFallbackSkinning(PxU32 submeshIndex);
	void					writeUserBuffers(PxU32 submeshIndex);

	// Debug rendering
	void					visualizeTangentSpace(NiApexRenderDebug& batcher, PxF32 normalScale, PxF32 tangentScale, PxF32 bitangentScale, PxMat33* scaledRotations, PxVec3* translations, PxU32 stride, PxU32 numberOfTransforms) const;

	ApexRenderMeshAsset*	mRenderMeshAsset;
	Array<PxMat34Legacy>	mTransforms;
	Array<PxMat34Legacy>	mTransformsLastFrame;
	//++LAVA
	Array<PxF32>			mAlphas;
	//--LAVA


	struct ResourceData
	{
		ResourceData() : resource(NULL), vertexCount(0), boneCount(0) {}
		NxUserRenderResource*	resource;
		PxU32					vertexCount;
		PxU32					boneCount;
	};

	struct SubmeshData
	{
		SubmeshData();
		~SubmeshData();

		Array<ResourceData>				renderResources;
		NxUserRenderIndexBuffer*		indexBuffer;
		void*							fallbackSkinningMemory;
		NxUserRenderVertexBuffer*		userDynamicVertexBuffer;
		NxUserRenderInstanceBuffer*		instanceBuffer;

		PxVec3*							userPositions;
		PxVec3*							userNormals;
		PxVec4*							userTangents4;

		// And now we have colors
		const PxColorRGBA*				staticColorReplacement;
		bool							staticColorReplacementDirty;

		// These are needed if the positions vary from what the asset has stored. Can happen with morph targets.
		// so far we only replace positions, more can be added here
		const PxVec3*					staticPositionReplacement;
		NxUserRenderVertexBuffer*		staticBufferReplacement;
		NxUserRenderVertexBuffer*		dynamicBufferReplacement;

		PxU32							fallbackSkinningMemorySize;
		PxU32							visibleTriangleCount;
		NxResID							materialID;
		PxU32							maxBonesPerMaterial;
		PxU32							indexBufferRequestedSize;
		bool							userSpecifiedData;
		bool							userVertexBufferAlwaysDirty;
		bool							userIndexBufferChanged;
		bool							fallbackSkinningDirty;
	};

	shdfnd::Array<SubmeshData>			mSubmeshData;
	NxIndexBank<PxU32>					mVisiblePartsForAPI;
	NxIndexBank<PxU32>					mVisiblePartsForAPILastFrame;
	NxRenderBufferHint::Enum			mIndexBufferHint;

	Array<PxU32>						mVisiblePartsForRendering;

	Array<NxUserRenderVertexBuffer*>	mPerActorVertexBuffers;

	// Instancing
	PxU32								mMaxInstanceCount;
	PxU32								mInstanceCount;
	PxU32								mInstanceOffset;
	NxUserRenderInstanceBuffer*			mInstanceBuffer;
	NxUserRenderResource*				mRenderResource;

	// configuration
	bool								mRenderWithoutSkinning;
	bool								mApiVisibilityChanged;
	bool								mPartVisibilityChanged;
	bool								mInstanceCountChanged;
	bool								mKeepVisibleBonesPacked;
	bool								mForceFallbackSkinning;
	bool								mBonePosesDirty;
	bool								mOneUserVertexBufferChanged; // this will be true if one or more user vertex buffers changed
	bool								mBoneBufferInUse;
	bool								mReleaseResourcesIfNothingToRender; // is this ever set? I think we should remove it.
	bool								mCreateRenderResourcesAfterInit; // only needed when mRenderWithoutSkinning is set to true
	bool								mBufferVisibility;
	bool								mKeepPreviousFrameBoneBuffer;
	bool								mPreviousFrameBoneBufferValid;
	Array<PxU32>						mTMSwapBuffer;
	Array<PxMat34Legacy>				mRemappedPreviousBoneTMs;

	// temporary
	Array<PxU16>						mBoneIndexTempBuffer;

	// CM: For some reason the new operator doesn't see these members and allocates not enough memory for the class (on a 64 bit build)
	// Even though ENABLE_INSTANCED_MESH_CLEANUP_HACK is defined to be 1 earlier in this file.
	// For now, these members are permanently here, since it's a tiny amount of memory if the define is off, and it fixes the allocation problem.
//#if ENABLE_INSTANCED_MESH_CLEANUP_HACK
	void*								mOrderedInstanceTemp;
	PxU32								mOrderedInstanceTempSize;
//#endif

	Array<PxU32>						mPartIndexTempBuffer;

	friend class ApexRenderMeshAsset;
};

} // namespace apex
} // namespace physx

#endif // APEX_RENDERMESH_ACTOR_H
