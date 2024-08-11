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

#ifndef CLOTHING_ACTOR_PROXY_H
#define CLOTHING_ACTOR_PROXY_H

#include "NxClothingActor.h"
#include "PsUserAllocated.h"
#include "ClothingAsset.h"

#include "ClothingActor.h"

namespace physx
{
namespace apex
{
namespace clothing
{

class ClothingActorProxy : public NxClothingActor, public physx::UserAllocated, public NxApexResource
{
public:
	ClothingActor impl;

#pragma warning(push)
#pragma warning( disable : 4355 ) // disable warning about this pointer in argument list

	ClothingActorProxy(const NxParameterized::Interface& desc, ClothingAsset* asset, ClothingScene& clothingScene, NxResourceList* list) :
		impl(desc, this, NULL, asset, &clothingScene)
	{
		list->add(*this);
	}

#pragma warning(pop)

	virtual void release()
	{
		impl.release();			// calls release method on asset
	}

	virtual physx::PxU32 getListIndex() const
	{
		return impl.m_listIndex;
	}

	virtual void setListIndex(class NxResourceList& list, physx::PxU32 index)
	{
		impl.m_list = &list;
		impl.m_listIndex = index;
	}

#ifndef WITHOUT_PVD
	virtual void initPvdInstances(physx::debugger::comm::PvdDataStream& pvdStream)
	{
		impl.initPvdInstances(pvdStream);
	}
#endif

	virtual void destroy()
	{
		impl.destroy();
		delete this;
	}

	// NxApexActor base class
	virtual NxApexAsset* getOwner() const
	{
		return impl.getOwner();
	}
	virtual physx::PxBounds3 getBounds() const
	{
		return impl.getBounds();
	}

	virtual void lockRenderResources()
	{
		impl.lockRenderResources();
	}

	virtual void unlockRenderResources()
	{
		impl.unlockRenderResources();
	}

	virtual void updateRenderResources(bool rewriteBuffers, void* userRenderData)
	{
		impl.updateRenderResources(rewriteBuffers, userRenderData);
	}

	virtual void dispatchRenderResources(NxUserRenderer& renderer)
	{
		impl.dispatchRenderResources(renderer);
	}

	virtual NxParameterized::Interface* getActorDesc()
	{
		return impl.getActorDesc();
	}

	virtual void updateState(const physx::PxMat44& globalPose, const physx::PxMat44* newBoneMatrices, physx::PxU32 boneMatricesByteStride, physx::PxU32 numBoneMatrices, ClothingTeleportMode::Enum teleportMode)
	{
		impl.updateState(globalPose, newBoneMatrices, boneMatricesByteStride, numBoneMatrices, teleportMode);
	}

	virtual void updateMaxDistanceScale(PxF32 scale, bool multipliable)
	{
		impl.updateMaxDistanceScale_DEPRECATED(scale, multipliable);
	}

	virtual const physx::PxMat44& getGlobalPose() const
	{
		return impl.getGlobalPose_DEPRECATED();
	}

	virtual void setWind(PxF32 windAdaption, const physx::PxVec3& windVelocity)
	{
		impl.setWind_DEPRECATED(windAdaption, windVelocity);
	}

	virtual void setMaxDistanceBlendTime(PxF32 blendTime)
	{
		impl.setMaxDistanceBlendTime_DEPRECATED(blendTime);
	}

	virtual PxF32 getMaxDistanceBlendTime() const
	{
		return impl.getMaxDistanceBlendTime_DEPRECATED();
	}

	virtual void getPhysicalMeshPositions(void* buffer, physx::PxU32 byteStride)
	{
		impl.getPhysicalMeshPositions(buffer, byteStride);
	}

	virtual void getPhysicalMeshNormals(void* buffer, physx::PxU32 byteStride)
	{
		impl.getPhysicalMeshNormals(buffer, byteStride);
	}

	virtual physx::PxF32 getMaximumSimulationBudget() const
	{
		return impl.getMaximumSimulationBudget();
	}

	virtual physx::PxU32 getNumSimulationVertices() const
	{
		return impl.getNumSimulationVertices();
	}

	virtual const physx::PxVec3* getSimulationPositions() const
	{
		return impl.getSimulationPositions();
	}

	virtual const physx::PxVec3* getSimulationNormals() const
	{
		return impl.getSimulationNormals();
	}

	virtual physx::PxU32 getNumGraphicalVerticesActive(PxU32 submeshIndex) const
	{
		return impl.getNumGraphicalVerticesActive(submeshIndex);
	}

	virtual PxMat44 getRenderGlobalPose() const
	{
		return impl.getRenderGlobalPose();
	}

	virtual const PxMat44* getCurrentBoneSkinningMatrices() const
	{
		return impl.getCurrentBoneSkinningMatrices();
	}

	virtual void setVisible(bool enable)
	{
		impl.setVisible(enable);
	}

	virtual bool isVisible() const
	{
		return impl.isVisibleBuffered();
	}

	virtual void setFrozen(bool enable)
	{
		impl.setFrozen(enable);
	}

	virtual bool isFrozen() const
	{
		return impl.isFrozenBuffered();
	}

	virtual ClothSolverMode::Enum getClothSolverMode() const
	{
		return impl.getClothSolverMode();
	}

	virtual void setLODWeights(PxF32 maxDistance, PxF32 distanceWeight, PxF32 bias, PxF32 benefitBias)
	{
		impl.setLODWeights_DEPRECATED(maxDistance, distanceWeight, bias, benefitBias);
	}

	virtual void setGraphicalLOD(PxU32 lod)
	{
		impl.setGraphicalLOD(lod);
	}

	virtual PxU32 getGraphicalLod()
	{
		return impl.getGraphicalLod();
	}

	virtual bool rayCast(const PxVec3& worldOrigin, const PxVec3& worldDirection, PxF32& time, PxVec3& normal, PxU32& vertexIndex)
	{
		return impl.rayCast(worldOrigin, worldDirection, time, normal, vertexIndex);
	}

	virtual void attachVertexToGlobalPosition(PxU32 vertexIndex, const PxVec3& globalPosition)
	{
		impl.attachVertexToGlobalPosition(vertexIndex, globalPosition);
	}

	virtual void freeVertex(PxU32 vertexIndex)
	{
		impl.freeVertex(vertexIndex);
	}

	virtual PxU32 getClothingMaterial() const
	{
		return impl.getClothingMaterial();
	}

	virtual void setClothingMaterial(physx::PxU32 index)
	{
		impl.setClothingMaterial(index);
	}

	virtual void setOverrideMaterial(PxU32 submeshIndex, const char* overrideMaterialName)
	{
		impl.setOverrideMaterial(submeshIndex, overrideMaterialName);
	}

	virtual void setUserRecompute(NxClothingUserRecompute* recompute)
	{
		impl.setUserRecompute(recompute);
	}

	virtual void setVelocityCallback(NxClothingVelocityCallback* callback)
	{
		impl.setVelocityCallback(callback);
	}

	virtual void setInterCollisionChannels(physx::PxU32 channels)
	{
		impl.setInterCollisionChannels(channels);
	}

	virtual physx::PxU32 getInterCollisionChannels()
	{
		return impl.getInterCollisionChannels();
	}

	virtual void getPhysicalLodRange(PxF32& min, PxF32& max, bool& intOnly)
	{
		impl.getPhysicalLodRange(min, max, intOnly);
	}

	virtual PxF32 getActivePhysicalLod()
	{
		return impl.getActivePhysicalLod();
	}

	virtual void forcePhysicalLod(PxF32 lod)
	{
		impl.forcePhysicalLod(lod);
	}

	virtual NxClothingPlane* createCollisionPlane(const PxPlane& plane)
	{
		return impl.createCollisionPlane(plane);
	}

	virtual NxClothingConvex* createCollisionConvex(NxClothingPlane** planes, PxU32 numPlanes)
	{
		return impl.createCollisionConvex(planes, numPlanes);
	}

	virtual NxClothingSphere* createCollisionSphere(const PxVec3& position, PxF32 radius)
	{
		return impl.createCollisionSphere(position, radius);
	}

	virtual NxClothingCapsule* createCollisionCapsule(NxClothingSphere& sphere1, NxClothingSphere& sphere2) 
	{
		return impl.createCollisionCapsule(sphere1, sphere2);
	}

	virtual NxClothingTriangleMesh* createCollisionTriangleMesh()
	{
		return impl.createCollisionTriangleMesh();
	}

	virtual PxU32 getCollisionSphereCount()
	{
		return impl.getCollisionSphereCount();
	}

	virtual PxU32 getCollisionPlaneCount()
	{
		return impl.getCollisionPlaneCount();
	}

	virtual PxU32 getCollisionTriangleCount()
	{
		return impl.getCollisionTriangleCount();
	}

	virtual NxClothingRenderProxy* acquireRenderProxy()
	{
		return impl.acquireRenderProxy();
	}
};


}
} // namespace apex
} // namespace physx

#endif // CLOTHING_ACTOR_PROXY_H
