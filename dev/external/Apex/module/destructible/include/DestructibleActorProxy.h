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

#ifndef __DESTRUCTIBLEACTOR_PROXY_H__
#define __DESTRUCTIBLEACTOR_PROXY_H__

#include "NxApex.h"
#include "NxDestructibleActor.h"
#include "DestructibleActorJointProxy.h"
#include "DestructibleActor.h"
#include "DestructibleScene.h"
#include "PsUserAllocated.h"
#include "ApexActor.h"
#if APEX_USE_PARTICLES
#include "NxApexEmitterActor.h"
#endif

namespace physx
{
namespace apex
{
namespace destructible
{

class DestructibleActorProxy : public NxDestructibleActor, public NxApexResource, public physx::UserAllocated
{
public:
	DestructibleActor impl;

#pragma warning(disable : 4355) // disable warning about this pointer in argument list
	DestructibleActorProxy(const NxParameterized::Interface& input, DestructibleAsset& asset, NxResourceList& list, DestructibleScene& scene)
		: impl(this, asset, scene)
	{
		NxParameterized::Interface* clonedInput = NULL;
		input.clone(clonedInput);
		list.add(*this);	// Doing this before impl.initialize, since the render proxy created in that function wants a unique ID (only needs to be unique at any given time, can be recycled)
		impl.initialize(clonedInput);
	}

	DestructibleActorProxy(NxParameterized::Interface* input, DestructibleAsset& asset, NxResourceList& list, DestructibleScene& scene)
		: impl(this, asset, scene)
	{
		impl.initialize(input);
		list.add(*this);
	}

	~DestructibleActorProxy()
	{
	}

	virtual const NxDestructibleParameters& getDestructibleParameters() const
	{
		return impl.getDestructibleParameters();
	}

	virtual void setDestructibleParameters(const NxDestructibleParameters& destructibleParameters)
	{
		impl.setDestructibleParameters(destructibleParameters);
	}

	virtual const NxRenderMeshActor* getRenderMeshActor(NxDestructibleActorMeshType::Enum type = NxDestructibleActorMeshType::Skinned) const
	{
		return impl.getRenderMeshActor(type);
	}

	virtual physx::PxMat44	getInitialGlobalPose() const
	{
		return impl.getInitialGlobalPose();
	}

	virtual void setInitialGlobalPose(const physx::PxMat44& pose)
	{
		impl.setInitialGlobalPose(pose);
	}

	virtual physx::PxVec3	getScale() const
	{
		return impl.getScale();
	}

	virtual void	applyDamage(physx::PxF32 damage, physx::PxF32 momentum, const physx::PxVec3& position, const physx::PxVec3& direction, physx::PxI32 chunkIndex = NxModuleDestructibleConst::INVALID_CHUNK_INDEX, void* damageActorUserData = NULL)
	{
		return impl.applyDamage(damage, momentum, position, direction, chunkIndex, damageActorUserData);
	}

	virtual void	applyRadiusDamage(physx::PxF32 damage, physx::PxF32 momentum, const physx::PxVec3& position, physx::PxF32 radius, bool falloff, void* damageActorUserData = NULL)
	{
		return impl.applyRadiusDamage(damage, momentum, position, radius, falloff, damageActorUserData);
	}	

	virtual void	getChunkVisibilities(physx::PxU8* visibilityArray, physx::PxU32 visibilityArraySize) const
	{
		impl.getChunkVisibilities(visibilityArray, visibilityArraySize);
	}

	virtual physx::PxU32	getNumVisibleChunks() const
	{
		return impl.getNumVisibleChunks();
	}

	virtual const physx::PxU16*	getVisibleChunks() const
	{
		return impl.getVisibleChunks();
	}

	virtual bool					acquireChunkEventBuffer(const NxDestructibleChunkEvent*& buffer, physx::PxU32& bufferSize)
	{
		return impl.acquireChunkEventBuffer(buffer, bufferSize);
	}

	virtual bool					releaseChunkEventBuffer(bool clearBuffer = true)
	{
		return impl.releaseChunkEventBuffer(clearBuffer);
	}

#if NX_SDK_VERSION_MAJOR == 2
	virtual bool					acquirePhysXActorBuffer(NxActor**& buffer, physx::PxU32& bufferSize, physx::PxU32 flags = NxDestructiblePhysXActorQueryFlags::All, bool eliminateRedundantActors = true)
#elif NX_SDK_VERSION_MAJOR == 3
	virtual bool					acquirePhysXActorBuffer(physx::PxRigidDynamic**& buffer, physx::PxU32& bufferSize, physx::PxU32 flags = NxDestructiblePhysXActorQueryFlags::All, bool eliminateRedundantActors = true)
#endif
	{
		return impl.acquirePhysXActorBuffer(buffer, bufferSize, flags, eliminateRedundantActors);
	}

	virtual bool					releasePhysXActorBuffer()
	{
		return impl.releasePhysXActorBuffer();
	}

#if NX_SDK_VERSION_MAJOR == 2
	virtual NxActor*				getChunkPhysXActor(physx::PxU32 index)
	{
		return impl.getChunkActor(index);
	}
#elif NX_SDK_VERSION_MAJOR == 3
	virtual physx::PxRigidDynamic*	getChunkPhysXActor(physx::PxU32 index)
	{
		physx::PxActor* actor = impl.getChunkActor(index);
		PX_ASSERT(actor == NULL || actor->isRigidDynamic());
		return (physx::PxRigidDynamic*)actor;
	}
#endif

	virtual physx::PxMat44			getChunkPose(physx::PxU32 index) const
	{
		return impl.getChunkPose(index);
	}

	virtual physx::PxVec3			getChunkLinearVelocity(physx::PxU32 index) const
	{
		return impl.getChunkLinearVelocity(index);
	}

	virtual physx::PxVec3			getChunkAngularVelocity(physx::PxU32 index) const
	{
		return impl.getChunkAngularVelocity(index);
	}

	virtual const physx::PxMat44	getChunkTM(physx::PxU32 index) const
	{
		return impl.getChunkTM(index);
	}

	virtual physx::PxI32			getBehaviorGroupIndex(physx::PxU32 chunkIndex) const
	{
		return impl.getBehaviorGroupIndex(chunkIndex);
	}

	virtual bool					isChunkDestroyed(physx::PxI32 chunkIndex) const
	{
		return impl.isChunkDestroyed(chunkIndex);
	}

	virtual void			setSkinnedOverrideMaterial(PxU32 index, const char* overrideMaterialName)
	{
		impl.setSkinnedOverrideMaterial(index, overrideMaterialName);
	}

	virtual void			setStaticOverrideMaterial(PxU32 index, const char* overrideMaterialName)
	{
		impl.setStaticOverrideMaterial(index, overrideMaterialName);
	}

	virtual void			setRuntimeFractureOverridePattern(const char* overridePatternName)
	{
		impl.setRuntimeFracturePattern(overridePatternName);
	}

	virtual bool			isInitiallyDynamic() const
	{
		return impl.isInitiallyDynamic();
	}

	virtual void			setLinearVelocity(const physx::PxVec3& linearVelocity)
	{
		impl.setLinearVelocity(linearVelocity);
	}

	virtual void			setGlobalPose(const physx::PxMat44& pose)
	{
		impl.setGlobalPoseForStaticChunks(pose);
	}

	virtual bool			getGlobalPose(physx::PxMat44& pose)
	{
		return impl.getGlobalPoseForStaticChunks(pose);
	}

	virtual void			setAngularVelocity(const physx::PxVec3& angularVelocity)
	{
		impl.setAngularVelocity(angularVelocity);
	}

	virtual void setDynamic(physx::PxI32 chunkIndex = NxModuleDestructibleConst::INVALID_CHUNK_INDEX)
	{
		impl.setDynamic(chunkIndex);
	}

	virtual bool isDynamic(physx::PxU32 chunkIndex) const
	{
		if (impl.getAsset() != NULL && chunkIndex < impl.getAsset()->getChunkCount())
		{
			return impl.getDynamic(chunkIndex);
		}
		return false;
	}

	virtual void enableHardSleeping()
	{
		impl.enableHardSleeping();
	}

	virtual void disableHardSleeping(bool wake = false)
	{
		impl.disableHardSleeping(wake);
	}

	virtual bool isHardSleepingEnabled() const
	{
		return impl.useHardSleeping();
	}

#if NX_SDK_VERSION_MAJOR == 2
	virtual physx::PxI32			rayCast(physx::PxF32& time, physx::PxVec3& normal, const NxRay& worldRay, NxDestructibleActorRaycastFlags::Enum flags, physx::PxI32 parentChunkIndex = NxModuleDestructibleConst::INVALID_CHUNK_INDEX) const
	{
		return impl.rayCast(time, normal, worldRay, flags, parentChunkIndex);
	}

	virtual physx::PxI32			obbSweep(physx::PxF32& time, physx::PxVec3& normal, const NxBox& worldBox, const physx::PxVec3& worldDisplacement, NxDestructibleActorRaycastFlags::Enum flags) const
	{
		return impl.obbSweep(time, normal, worldBox, worldDisplacement, flags);
	}
#elif NX_SDK_VERSION_MAJOR == 3
	virtual physx::PxI32			rayCast(physx::PxF32& time, physx::PxVec3& normal, const PxVec3& worldRayOrig, const PxVec3& worldRayDir, NxDestructibleActorRaycastFlags::Enum flags, physx::PxI32 parentChunkIndex = NxModuleDestructibleConst::INVALID_CHUNK_INDEX) const
	{
		NxRay	worldRay(worldRayOrig, worldRayDir);
		return impl.rayCast(time, normal, worldRay, flags, parentChunkIndex);
	}

	virtual physx::PxI32			obbSweep(physx::PxF32& time, physx::PxVec3& normal, const physx::PxVec3& worldBoxCenter, const physx::PxVec3& worldBoxExtents, const physx::PxMat33& worldBoxRot, const physx::PxVec3& worldDisplacement, NxDestructibleActorRaycastFlags::Enum flags) const
	{
		NxBox	worldBox(worldBoxCenter, worldBoxExtents, worldBoxRot);
		return impl.obbSweep(time, normal, worldBox, worldDisplacement, flags);
	}
#endif
	virtual void cacheModuleData() const
	{
		return impl.cacheModuleData();
	}

	virtual physx::PxBounds3		getLocalBounds() const
	{
		return impl.getLocalBounds();
	}

	virtual physx::PxBounds3		getOriginalBounds() const
	{ 
		return impl.getOriginalBounds();
	}

	virtual bool					isChunkSolitary(physx::PxI32 chunkIndex) const
	{
		return impl.isChunkSolitary( chunkIndex );
	}

	virtual physx::PxBounds3		getChunkBounds(physx::PxU32 chunkIndex) const
	{
		return impl.getChunkBounds( chunkIndex );
	}

	virtual physx::PxBounds3		getChunkLocalBounds(physx::PxU32 chunkIndex) const
	{
		return impl.getChunkLocalBounds( chunkIndex );
	}

	virtual physx::PxU32			getSupportDepthChunkIndices(physx::PxU32* const OutChunkIndices, physx::PxU32 MaxOutIndices) const
	{
		return impl.getSupportDepthChunkIndices( OutChunkIndices, MaxOutIndices );
	}

	virtual physx::PxU32			getSupportDepth() const
	{ 
		return impl.getSupportDepth();
	}

	// NxApexActor methods
	virtual NxApexAsset* getOwner() const
	{
		return impl.mAsset->getNxAsset();
	}
	virtual void release()
	{
		impl.release();
	}
	virtual void destroy()
	{
		impl.destroy();

		delete this;
	}

	virtual NxDestructibleRenderable* acquireRenderableReference()
	{
		return impl.acquireRenderableReference();
	}

	// NxApexRenderable methods
	virtual void updateRenderResources(bool rewriteBuffers, void* userRenderData)
	{
		impl.updateRenderResources(rewriteBuffers, userRenderData);
	}

	virtual void dispatchRenderResources(NxUserRenderer& api)
	{
		impl.dispatchRenderResources(api);
	}

	virtual void lockRenderResources()
	{
		NxApexRenderable* renderable = impl.getRenderable();
		if (renderable != NULL)
		{
			renderable->lockRenderResources();
		}
	}

	virtual void unlockRenderResources()
	{
		NxApexRenderable* renderable = impl.getRenderable();
		if (renderable != NULL)
		{
			renderable->unlockRenderResources();
		}
	}

	virtual physx::PxBounds3 getBounds() const
	{
		NxApexRenderable* renderable = const_cast<DestructibleActor*>(&impl)->getRenderable();
		if (renderable != NULL)
		{
			return renderable->getBounds();
		}
		return PxBounds3::empty();
	}

	// NxApexActorSource methods
#if NX_SDK_VERSION_MAJOR == 2
	virtual void setActorTemplate(const NxActorDescBase* desc)
	{
		impl.setActorTemplate(desc);
	}
	virtual bool getActorTemplate(NxActorDescBase& dest) const
	{
		return impl.getActorTemplate(dest);
	}
	virtual void setShapeTemplate(const NxShapeDesc* desc)
	{
		impl.setShapeTemplate(desc);
	}
	virtual bool getShapeTemplate(NxShapeDesc& dest) const
	{
		return impl.getShapeTemplate(dest);
	}
	virtual void setBodyTemplate(const NxBodyDesc* desc)
	{
		impl.setBodyTemplate(desc);
	}
	virtual bool getBodyTemplate(NxBodyDesc& dest) const
	{
		return impl.getBodyTemplate(dest);
	}
#elif NX_SDK_VERSION_MAJOR == 3
	virtual void setPhysX3Template(const PhysX3DescTemplate* desc)
	{
		impl.setPhysX3Template(desc);
	}

	virtual bool getPhysX3Template(PhysX3DescTemplate& dest) const
	{
		return impl.getPhysX3Template(dest);
	}
#endif
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

	virtual void setLODWeights(physx::PxF32 maxDistance, physx::PxF32 distanceWeight, physx::PxF32 maxAge, physx::PxF32 ageWeight, physx::PxF32 bias)
	{
		return impl.setLODWeights(maxDistance, distanceWeight, maxAge, ageWeight, bias);
	}

	virtual void getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly)
	{
		impl.getPhysicalLodRange(min, max, intOnly);
	}

	virtual physx::PxF32 getActivePhysicalLod()
	{
		return impl.getActivePhysicalLod();
	}

	virtual void forcePhysicalLod(physx::PxF32 lod)
	{
		impl.forcePhysicalLod(lod);
	}

	virtual void setCrumbleEmitterState(bool enable)
	{
		impl.setCrumbleEmitterEnabled(enable);
	}

	virtual void setDustEmitterState(bool enable)
	{
		impl.setDustEmitterEnabled(enable);
	}

	/**
		Sets a preferred render volume for a dust or crumble emitter
	*/
	virtual void setPreferredRenderVolume(NxApexRenderVolume* volume, NxDestructibleEmitterType::Enum type)
	{
		impl.setPreferredRenderVolume(volume, type);
	}

	virtual NxApexEmitterActor* getApexEmitter(NxDestructibleEmitterType::Enum type)
	{
		return impl.getApexEmitter(type);
	}

	virtual bool recreateApexEmitter(NxDestructibleEmitterType::Enum type)
	{
		return impl.recreateApexEmitter(type);
	}

	const NxParameterized::Interface* getNxParameterized(NxDestructibleParameterizedType::Enum type) const
	{
		switch (type) 
		{
		case NxDestructibleParameterizedType::State:
			return (const NxParameterized::Interface*)impl.getState();
		case NxDestructibleParameterizedType::Params:
			return (const NxParameterized::Interface*)impl.getParams();
		default:
			return NULL;
		}
	}

	void setNxParameterized(NxParameterized::Interface* params)
	{
		impl.initialize(params);
	}

	const NxParameterized::Interface* getChunks() const
	{
		return (const NxParameterized::Interface*)impl.getChunks();
	}

	void setChunks(NxParameterized::Interface* chunks)
	{
		impl.initialize(chunks);
	}

    virtual bool setSyncParams(physx::PxU32 userActorID, physx::PxU32 actorSyncFlags, const NxDestructibleActorSyncState * actorSyncState, const NxDestructibleChunkSyncState * chunkSyncState)
    {
        return impl.setSyncParams(userActorID, actorSyncFlags, actorSyncState, chunkSyncState);
    }

	virtual bool setHitChunkTrackingParams(bool flushHistory, bool startTracking, physx::PxU32 trackingDepth, bool trackAllChunks)
	{
		return impl.setHitChunkTrackingParams(flushHistory, startTracking, trackingDepth, trackAllChunks);
	}

	virtual bool getHitChunkHistory(const NxDestructibleHitChunk *& hitChunkContainer, physx::PxU32 & hitChunkCount) const
	{
		return impl.getHitChunkHistory(hitChunkContainer, hitChunkCount);
	}

	virtual bool forceChunkHits(const NxDestructibleHitChunk * hitChunkContainer, physx::PxU32 hitChunkCount, bool removeChunks = true, bool deferredEvent = false, physx::PxVec3 damagePosition = physx::PxVec3(0.0f), physx::PxVec3 damageDirection = physx::PxVec3(0.0f))
	{
		return impl.forceChunkHits(hitChunkContainer, hitChunkCount, removeChunks, deferredEvent, damagePosition, damageDirection);
	}

	virtual bool getDamageColoringHistory(const NxDamageEventCoreData *& damageEventCoreDataContainer, physx::PxU32 & damageEventCoreDataCount) const
	{
		return impl.getDamageColoringHistory(damageEventCoreDataContainer, damageEventCoreDataCount);
	}

	virtual bool forceDamageColoring(const NxDamageEventCoreData * damageEventCoreDataContainer, physx::PxU32 damageEventCoreDataCount)
	{
		return impl.forceDamageColoring(damageEventCoreDataContainer, damageEventCoreDataCount);
	}

	virtual void setDeleteFracturedChunks(bool inDeleteChunkMode)
	{
		impl.setDeleteFracturedChunks(inDeleteChunkMode);
	}

#if NX_SDK_VERSION_MAJOR == 2
	virtual void takeImpact(const physx::PxVec3& force, const physx::PxVec3& position, physx::PxU16 chunkIndex, NxActor const* damageImpactActor)
#elif NX_SDK_VERSION_MAJOR == 3
	virtual void takeImpact(const physx::PxVec3& force, const physx::PxVec3& position, physx::PxU16 chunkIndex, PxActor const* damageImpactActor)
#endif
	{
		impl.takeImpact(force, position, chunkIndex, damageImpactActor);
	}

	virtual physx::PxU32			getCustomBehaviorGroupCount() const
	{
		return impl.getCustomBehaviorGroupCount();
	}

	virtual bool					getBehaviorGroup(NxDestructibleBehaviorGroupDesc& behaviorGroupDesc, physx::PxI32 index = -1) const
	{
		return impl.getBehaviorGroup(behaviorGroupDesc, index);
	}

	// ++ LAVA ++
	virtual physx::PxF32			getActorDistanceSquaredFromViewport( physx::PxU32 index )
	{
		return impl.getActorDistanceSquaredFromViewport( index);
	}

	virtual bool					setActorChunksToBeDestroyed(PxActor& actor)
	{
		return impl.setActorChunksToBeDestroyed(( NxActor&) actor );
	}
	// -- LAVA --

	virtual void setVisible(bool enable)
	{
		impl.setVisible( enable );
	}

	virtual bool isVisible() const 
	{
		return impl.isVisible();
	}

};


}
}
} // end namespace physx::apex

#endif // __DESTRUCTIBLEACTOR_PROXY_H__
