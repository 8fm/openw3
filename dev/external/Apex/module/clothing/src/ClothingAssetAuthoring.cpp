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

#include "PsShare.h"

#ifndef WITHOUT_APEX_AUTHORING

#include "ClothingAssetAuthoring.h"
#include "ApexMeshHash.h"
#include "PsSort.h"
#include "ApexPermute.h"
#include "CookingPhysX.h"
#include "ClothingGlobals.h"
#include "ClothingPhysicalMesh.h"
#include "ClothingIsoMesh.h"

#define MAX_DISTANCE_NAME "MAX_DISTANCE"
#define COLLISION_SPHERE_DISTANCE_NAME "COLLISION_SPHERE_DISTANCE"
#define COLLISION_SPHERE_RADIUS_NAME "COLLISION_SPHERE_RADIUS"
#define USED_FOR_PHYSICS_NAME "USED_FOR_PHYSICS"

#define LATCH_TO_NEAREST_SLAVE_NAME "LATCH_TO_NEAREST_SLAVE"
#define LATCH_TO_NEAREST_MASTER_NAME "LATCH_TO_NEAREST_MASTER"

#include "NxRenderMesh.h"

#include "NiApexSDK.h"
#include "NiApexAuthorableObject.h"


namespace physx
{
namespace apex
{
namespace clothing
{

struct PxU32_3
{
	physx::PxU32 indices[3];
};



class TriangleGreater_3
{
public:
	TriangleGreater_3() {}

	TriangleGreater_3(physx::PxU32* deformableIndices, ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* constrainCoeffs) :
		mDeformableIndices(deformableIndices),
		mConstrainCoeffs(constrainCoeffs)
	{}

	inline bool operator()(PxU32_3 a, PxU32_3 b) const
	{
		physx::PxF32 maxDistA = mConstrainCoeffs[mDeformableIndices[a.indices[0]]].maxDistance;
		physx::PxF32 maxDistB = mConstrainCoeffs[mDeformableIndices[b.indices[0]]].maxDistance;
		bool aHasEqualMaxDistances = (maxDistA == mConstrainCoeffs[mDeformableIndices[a.indices[1]]].maxDistance);
		bool bHasEqualMaxDistances = (maxDistB == mConstrainCoeffs[mDeformableIndices[b.indices[1]]].maxDistance);
		for (physx::PxU32 i = 1; i < 3; i++)
		{
			if (aHasEqualMaxDistances)
			{
				aHasEqualMaxDistances = (mConstrainCoeffs[mDeformableIndices[a.indices[i - 1]]].maxDistance == mConstrainCoeffs[mDeformableIndices[a.indices[i]]].maxDistance);
			}
			if (bHasEqualMaxDistances)
			{
				bHasEqualMaxDistances = (mConstrainCoeffs[mDeformableIndices[b.indices[i - 1]]].maxDistance == mConstrainCoeffs[mDeformableIndices[b.indices[i]]].maxDistance);
			}
			maxDistA = PxMax(maxDistA, mConstrainCoeffs[mDeformableIndices[a.indices[i]]].maxDistance);
			maxDistB = PxMax(maxDistB, mConstrainCoeffs[mDeformableIndices[b.indices[i]]].maxDistance);
		}

		if (maxDistA == maxDistB)
		{
			return aHasEqualMaxDistances && !bHasEqualMaxDistances;
		}

		return maxDistA > maxDistB;
	}

private:
	physx::PxU32* mDeformableIndices;
	ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* mConstrainCoeffs;
};



struct PxU32_4
{
	physx::PxU32 indices[4];
};



class TriangleGreater_4
{
public:
	TriangleGreater_4() {}

	TriangleGreater_4(physx::PxU32* deformableIndices, ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* constrainCoeffs) :
		mDeformableIndices(deformableIndices),
		mConstrainCoeffs(constrainCoeffs)
	{}

	inline bool operator()(PxU32_4 a, PxU32_4 b) const
	{
		physx::PxF32 maxDistA = mConstrainCoeffs[mDeformableIndices[a.indices[0]]].maxDistance;
		physx::PxF32 maxDistB = mConstrainCoeffs[mDeformableIndices[b.indices[0]]].maxDistance;
		bool aHasEqualMaxDistances = (maxDistA == mConstrainCoeffs[mDeformableIndices[a.indices[1]]].maxDistance);
		bool bHasEqualMaxDistances = (maxDistB == mConstrainCoeffs[mDeformableIndices[b.indices[1]]].maxDistance);
		for (physx::PxU32 i = 1; i < 4; i++)
		{
			if (aHasEqualMaxDistances)
			{
				aHasEqualMaxDistances = (mConstrainCoeffs[mDeformableIndices[a.indices[i - 1]]].maxDistance == mConstrainCoeffs[mDeformableIndices[a.indices[i]]].maxDistance);
			}
			if (bHasEqualMaxDistances)
			{
				bHasEqualMaxDistances = (mConstrainCoeffs[mDeformableIndices[b.indices[i - 1]]].maxDistance == mConstrainCoeffs[mDeformableIndices[b.indices[i]]].maxDistance);
			}
			maxDistA = PxMax(maxDistA, mConstrainCoeffs[mDeformableIndices[a.indices[i]]].maxDistance);
			maxDistB = PxMax(maxDistB, mConstrainCoeffs[mDeformableIndices[b.indices[i]]].maxDistance);
		}

		if (maxDistA == maxDistB)
		{
			return aHasEqualMaxDistances && !bHasEqualMaxDistances;
		}

		return maxDistA > maxDistB;
	}

private:
	physx::PxU32* mDeformableIndices;
	ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* mConstrainCoeffs;
};



class BoneEntryPredicate
{
public:
	bool operator()(const ClothingAssetParametersNS::BoneEntry_Type& a, const ClothingAssetParametersNS::BoneEntry_Type& b) const
	{
		// mesh referenced bones first
		if (a.numMeshReferenced == 0 && b.numMeshReferenced > 0)
		{
			return false;
		}
		if (a.numMeshReferenced > 0 && b.numMeshReferenced == 0)
		{
			return true;
		}

		if (a.numMeshReferenced == 0) // both are 0 as they have to be equal here
		{
			PX_ASSERT(b.numMeshReferenced == 0);

			// RB referenced bones next, this will leave non referenced bones at the end
			if (a.numRigidBodiesReferenced != b.numRigidBodiesReferenced)
			{
				return a.numRigidBodiesReferenced > b.numRigidBodiesReferenced;
			}
			else
			{
				return a.externalIndex < b.externalIndex;
			}
		}

		return a.externalIndex < b.externalIndex;
	}
};



class ActorEntryPredicate
{
public:
	bool operator()(const ClothingAssetParametersNS::ActorEntry_Type& a, const ClothingAssetParametersNS::ActorEntry_Type& b) const
	{
		if (a.boneIndex < b.boneIndex)
		{
			return true;
		}
		else if (a.boneIndex > b.boneIndex)
		{
			return false;
		}
		return a.convexVerticesCount < b.convexVerticesCount;
	}
};



static bool getClosestVertex(NiApexRenderMeshAssetAuthoring* renderMeshAsset, const PxVec3& position, PxU32& resultSubmeshIndex,
                             PxU32& resultGraphicalVertexIndex, const char* bufferName, bool ignoreUnused)
{
	resultSubmeshIndex = 0;
	resultGraphicalVertexIndex = 0;

	bool found = false;

	if (renderMeshAsset != NULL)
	{
		PxF32 closestDistanceSquared = FLT_MAX;

		for (PxU32 submeshIndex = 0; submeshIndex < renderMeshAsset->getSubmeshCount(); submeshIndex++)
		{
			NxRenderDataFormat::Enum outFormat = NxRenderDataFormat::UNSPECIFIED;
			const NxVertexBuffer& vb = renderMeshAsset->getSubmesh(submeshIndex).getVertexBuffer();
			const NxVertexFormat& vf = vb.getFormat();
			if (bufferName != NULL)
			{
				NxVertexFormat::BufferID id = strcmp(bufferName, "NORMAL") == 0 ? vf.getSemanticID(NxRenderVertexSemantic::NORMAL) : vf.getID(bufferName);
				outFormat = vf.getBufferFormat(vf.getBufferIndexFromID(id));
				if (outFormat == NxRenderDataFormat::UNSPECIFIED)
				{
					continue;
				}
			}

			const PxU8* usedForPhysics = NULL;
			if (ignoreUnused)
			{
				PxU32 usedForPhysicsIndex = vf.getBufferIndexFromID(vf.getID(USED_FOR_PHYSICS_NAME));
				outFormat = vf.getBufferFormat(usedForPhysicsIndex);
				if (outFormat == NxRenderDataFormat::UBYTE1)
				{
					usedForPhysics = (const PxU8*)vb.getBuffer(usedForPhysicsIndex);
				}
			}

			const PxU32* slave = NULL;
			if (ignoreUnused)
			{
				PxU32 slaveIndex = vf.getBufferIndexFromID(vf.getID(LATCH_TO_NEAREST_SLAVE_NAME));
				outFormat = vf.getBufferFormat(slaveIndex);
				if (outFormat == NxRenderDataFormat::UINT1)
				{
					slave = (const PxU32*)vb.getBuffer(slaveIndex);
				}
			}

			const PxU32 vertexCount = renderMeshAsset->getSubmesh(submeshIndex).getVertexCount(0); // only 1 part supported
			NxRenderDataFormat::Enum format;
			PxU32 bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(NxRenderVertexSemantic::POSITION));
			const PxVec3* positions = (const PxVec3*)vb.getBufferAndFormat(format, bufferIndex);
			if (format != NxRenderDataFormat::FLOAT3)
			{
				PX_ALWAYS_ASSERT();
				positions = NULL;
			}
			for (PxU32 vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
			{
				if (usedForPhysics != NULL && usedForPhysics[vertexIndex] == 0)
				{
					continue;
				}

				if (slave != NULL && slave[vertexIndex] != 0)
				{
					continue;
				}

				const PxF32 distSquared = (position - positions[vertexIndex]).magnitudeSquared();
				if (distSquared < closestDistanceSquared)
				{
					closestDistanceSquared = distSquared;
					resultSubmeshIndex = submeshIndex;
					resultGraphicalVertexIndex = vertexIndex;
					found = true;
				}
			}
		}
	}

	return found;
}




ClothingAssetAuthoring::ClothingAssetAuthoring(ModuleClothing* module, NxResourceList& list) :
	ClothingAsset(module, list, "ClothingAuthoring"),
	mExportScale(1.0f),
	mDeriveNormalsFromBones(false),
	mOwnsMaterialLibrary(true),
	mPreviousCookedType(NX_SDK_VERSION_MAJOR == 2 ? "Native" : "Embedded")
{
	mInvalidConstrainCoefficients.maxDistance = -1.0f;
	mInvalidConstrainCoefficients.maxDistanceBias = 0.0f;
	mInvalidConstrainCoefficients.collisionSphereDistance = -FLT_MAX;
	mInvalidConstrainCoefficients.collisionSphereRadius = -1.0f;

	initParams();
}

ClothingAssetAuthoring::ClothingAssetAuthoring(ModuleClothing* module, NxResourceList& list, const char* name) :
	ClothingAsset(module, list, name),
	mExportScale(1.0f),
	mDeriveNormalsFromBones(false),
	mOwnsMaterialLibrary(true),
	mPreviousCookedType(NX_SDK_VERSION_MAJOR == 2 ? "Native" : "Embedded")
{
	mInvalidConstrainCoefficients.maxDistance = -1.0f;
	mInvalidConstrainCoefficients.maxDistanceBias = 0.0f;
	mInvalidConstrainCoefficients.collisionSphereDistance = -FLT_MAX;
	mInvalidConstrainCoefficients.collisionSphereRadius = -1.0f;

	initParams();
}

ClothingAssetAuthoring::ClothingAssetAuthoring(ModuleClothing* module, NxResourceList& list, NxParameterized::Interface* params, const char* name) :
	ClothingAsset(module, list, params, name),
	mExportScale(1.0f),
	mDeriveNormalsFromBones(false),
	mOwnsMaterialLibrary(true),
	mPreviousCookedType(NX_SDK_VERSION_MAJOR == 2 ? "Native" : "Embedded")
{
	mDefaultConstrainCoefficients.maxDistance = 0.0f;
	mDefaultConstrainCoefficients.maxDistanceBias = 0.0f;
	mDefaultConstrainCoefficients.collisionSphereDistance = 0.0f;
	mDefaultConstrainCoefficients.collisionSphereRadius = 0.0f;

	mInvalidConstrainCoefficients.maxDistance = -1.0f;
	mInvalidConstrainCoefficients.maxDistanceBias = 0.0f;
	mInvalidConstrainCoefficients.collisionSphereDistance = -FLT_MAX;
	mInvalidConstrainCoefficients.collisionSphereRadius = -1.0f;

	initParams();

	if (mParams->rootBoneIndex < (PxU32)mParams->bones.arraySizes[0])
	{
		mRootBoneName = mParams->bones.buf[mParams->rootBoneIndex].name;
	}
}



void ClothingAssetAuthoring::release()
{
	mModule->mSdk->releaseAssetAuthoring(*this);
}



bool ClothingAssetAuthoring::checkSetMeshesInput(PxU32 lod, NxClothingPhysicalMesh* nxPhysicalMesh, PxU32& graphicalLodIndex)
{
	// index where it will be inserted
	for (graphicalLodIndex = 0; graphicalLodIndex < mGraphicalLods.size(); ++graphicalLodIndex)
	{
		if (mGraphicalLods[graphicalLodIndex]->lod >= lod)
		{
			break;
		}
	}


	if (nxPhysicalMesh != NULL)
	{
		if (mPhysicalMeshesInput.size() != mPhysicalMeshes.size())
		{
			APEX_INVALID_PARAMETER("Trying to operate add a physical mesh to an authoring object that has been deserialized. This is not suppored.");
			return false;
		}

		// check that shared physics meshes are only in subsequent lods
		PxI32 i = graphicalLodIndex - 1;
		PxU32 physMeshId = (PxU32) - 1;
		while (i >= 0)
		{
			physMeshId = mGraphicalLods[i]->physicalMeshId;
			if (physMeshId != (physx::PxU32) - 1 && mPhysicalMeshesInput[physMeshId] != nxPhysicalMesh)
			{
				break;
			}
			--i;
		}

		while (i >= 0)
		{
			physMeshId = mGraphicalLods[i]->physicalMeshId;
			if (physMeshId != (physx::PxU32) - 1 && mPhysicalMeshesInput[physMeshId] == nxPhysicalMesh)
			{
				APEX_INVALID_PARAMETER("Only subsequent graphical lods can share a physical mesh.");
				return false;
			}
			--i;
		}

		i = graphicalLodIndex + 1;
		physMeshId = (PxU32) - 1;
		while (i < (PxI32)mGraphicalLods.size())
		{
			physMeshId = mGraphicalLods[i]->physicalMeshId;
			if (physMeshId != (physx::PxU32) - 1 && mPhysicalMeshesInput[physMeshId] != nxPhysicalMesh)
			{
				break;
			}
			++i;
		}

		while (i < (PxI32)mGraphicalLods.size())
		{
			physMeshId = mGraphicalLods[i]->physicalMeshId;
			if (physMeshId != (physx::PxU32) - 1 && mPhysicalMeshesInput[physMeshId] == nxPhysicalMesh)
			{
				APEX_INVALID_PARAMETER("Only subsequent graphical lods can share a physical mesh.");
				return false;
			}
			++i;
		}
	}
	return true;
}



void ClothingAssetAuthoring::sortPhysicalMeshes()
{
	if (mPhysicalMeshes.size() == 0)
	{
		return;
	}

	// sort physical lods according to references in graphical lods
	Array<PxU32> new2old(mPhysicalMeshes.size(), (PxU32) - 1);
	Array<PxU32> old2new(mPhysicalMeshes.size(), (PxU32) - 1);
	bool reorderFailed = false;

	PxU32 nextId = 0;
	for (PxU32 i = 0; i < mGraphicalLods.size(); i++)
	{
		PxU32 physMeshId = mGraphicalLods[i]->physicalMeshId;
		if (physMeshId == (PxU32) - 1)
		{
			continue;
		}

		if (nextId == 0 || new2old[nextId - 1] != physMeshId) // if there's  a new ID
		{
			// the new ID already appeared before, we can't sort
			if (old2new[physMeshId] != (PxU32) - 1)
			{
				PX_ALWAYS_ASSERT();
				APEX_INTERNAL_ERROR("The assignment of graphics and physics mesh in the asset does not allow ordering of the physical meshes. Reuse of physical mesh is only allowed on subsequend graphical lods.");
				reorderFailed = true;
				break;
			}

			new2old[nextId] = physMeshId;
			old2new[physMeshId] = nextId;
			++nextId;
		}
	}

	if (!reorderFailed)
	{
		// reorder
		ApexPermute<ClothingPhysicalMeshParameters*>(&mPhysicalMeshes[0], &new2old[0], mPhysicalMeshes.size());

		// update references
		for (PxU32 i = 0; i < mGraphicalLods.size(); i++)
		{
			mGraphicalLods[i]->physicalMeshId = old2new[mGraphicalLods[i]->physicalMeshId];
		}

		// clear transition maps
		for (physx::PxU32 i = 0; i < mPhysicalMeshes.size(); i++)
		{
			NxParamArray<SkinClothMapB> transitionDownB(mPhysicalMeshes[i], "transitionDownB", reinterpret_cast<NxParamDynamicArrayStruct*>(&mPhysicalMeshes[i]->transitionDownB));
			transitionDownB.clear();
			NxParamArray<SkinClothMapB> transitionUpB(mPhysicalMeshes[i], "transitionUpB", reinterpret_cast<NxParamDynamicArrayStruct*>(&mPhysicalMeshes[i]->transitionUpB));
			transitionUpB.clear();

			NxParamArray<SkinClothMap> transitionDown(mPhysicalMeshes[i], "transitionDown", reinterpret_cast<NxParamDynamicArrayStruct*>(&mPhysicalMeshes[i]->transitionDown));
			transitionDown.clear();
			NxParamArray<SkinClothMap> transitionUp(mPhysicalMeshes[i], "transitionUp", reinterpret_cast<NxParamDynamicArrayStruct*>(&mPhysicalMeshes[i]->transitionUp));
			transitionUp.clear();
		}
	}
}



void ClothingAssetAuthoring::setMeshes(physx::PxU32 lod, NxRenderMeshAssetAuthoring* renderMeshAssetDontReference, NxClothingPhysicalMesh* nxPhysicalMesh,
                                       physx::PxU32 numMaxDistReductions, physx::PxF32* maxDistReductions, physx::PxF32 normalResemblance, bool ignoreUnusedVertices,
                                       IProgressListener* progress)
{
	// check input
	PxU32 graphicalLodIndexTest = (PxU32) - 1;
	if (!checkSetMeshesInput(lod, nxPhysicalMesh, graphicalLodIndexTest))
	{
		return;
	}

	// get index and add lod if necessary, only adds if lod doesn't exist already
	const physx::PxU32 graphicalLodIndex = addGraphicalLod(lod);
	PX_ASSERT(graphicalLodIndex == graphicalLodIndexTest);

	PX_ASSERT(lod == mGraphicalLods[graphicalLodIndex]->lod);

	clearMapping(graphicalLodIndex);


	// reset counters to 0
	for (physx::PxU32 i = 0; i < mBones.size(); i++)
	{
		mBones[i].numMeshReferenced = mBones[i].numRigidBodiesReferenced = 0;
	}


	// remove existing physical of this lod mesh if it is not used by other lod
	const physx::PxU32 oldPhysicalMeshId = mGraphicalLods[graphicalLodIndex]->physicalMeshId;
	if (oldPhysicalMeshId != (physx::PxU32) - 1)
	{
		// check if it's referenced by someone else
		bool removePhysicalMesh = true;
		for (physx::PxU32 i = 0; i < mGraphicalLods.size(); i++)
		{
			// don't consider the current graphical lod
			if (mGraphicalLods[i]->lod == lod)
			{
				continue;
			}

			if (mGraphicalLods[i]->physicalMeshId == oldPhysicalMeshId)
			{
				removePhysicalMesh = false;
				break;
			}
		}

		// if it's not referenced, remove it
		if (removePhysicalMesh)
		{
			if (mPhysicalMeshesInput.size() == mPhysicalMeshes.size()) // mPhysicalMeshesInput is not set if the authoring is created from an existing params object
			{
				mPhysicalMeshesInput.replaceWithLast(oldPhysicalMeshId);
			}

			// replace with last and update the references to the last
			mPhysicalMeshes.replaceWithLast(oldPhysicalMeshId);
			for (physx::PxU32 i = 0; i < mGraphicalLods.size(); i++)
			{
				if (mGraphicalLods[i]->physicalMeshId == mPhysicalMeshes.size())
				{
					mGraphicalLods[i]->physicalMeshId = oldPhysicalMeshId;
				}
			}
		}
	}

	// copy physical mesh if we don't already have it
	bool newPhysicalMesh = false;
	ClothingPhysicalMeshParameters* physicalMesh = NULL;

	if (nxPhysicalMesh != NULL)
	{
		ClothingPhysicalMesh* physicalMeshInput = DYNAMIC_CAST(ClothingPhysicalMesh*)(nxPhysicalMesh);
		PX_ASSERT(physicalMeshInput != NULL);

		PX_ASSERT(mPhysicalMeshes.size() == mPhysicalMeshesInput.size());
		for (physx::PxU32 i = 0; i < mPhysicalMeshesInput.size(); i++)
		{
			if (physicalMeshInput == mPhysicalMeshesInput[i]) // TODO check some more stuff in case it has been released and a new one was created at the same address
			{
				physicalMesh = mPhysicalMeshes[i];
				PX_ASSERT(physicalMesh != NULL);
				mGraphicalLods[graphicalLodIndex]->physicalMeshId = i;
				break;
			}
		}
		if (physicalMesh == NULL)
		{
			physicalMesh = DYNAMIC_CAST(ClothingPhysicalMeshParameters*)(NiGetApexSDK()->getParameterizedTraits()->createNxParameterized(ClothingPhysicalMeshParameters::staticClassName()));
			physicalMeshInput->makeCopy(physicalMesh);
			physicalMesh->referenceCount = 1;
			mPhysicalMeshes.pushBack(physicalMesh);
			mPhysicalMeshesInput.pushBack(physicalMeshInput);

			ClothingPhysicalMesh* mesh = mModule->createPhysicalMeshInternal(physicalMesh);
			mesh->updateSkinningNormals();
			mesh->release();

			newPhysicalMesh = true;
			PX_ASSERT(physicalMesh != NULL);
			mGraphicalLods[graphicalLodIndex]->physicalMeshId = mPhysicalMeshes.size() - 1;
		}
	}

	bool hasLod = addGraphicalMesh(renderMeshAssetDontReference, graphicalLodIndex);
	if (hasLod)
	{
		PX_ASSERT(mGraphicalLods[graphicalLodIndex] != NULL);
		NiApexRenderMeshAsset* renderMeshAssetCopy = reinterpret_cast<NiApexRenderMeshAsset*>(mGraphicalLods[graphicalLodIndex]->renderMeshAssetPointer);

		// update mapping
		updateMappingAuthoring(*mGraphicalLods[graphicalLodIndex], renderMeshAssetCopy, static_cast<NiApexRenderMeshAssetAuthoring*>(renderMeshAssetDontReference),
		                       normalResemblance, ignoreUnusedVertices, progress);

		// sort physics mesh triangles and vertices
		ClothingPhysicalMesh* mesh = mModule->createPhysicalMeshInternal(physicalMesh);
		sortDeformableIndices(*mesh);

		// create submeshes
		setupPhysicalLods(*physicalMesh, numMaxDistReductions, maxDistReductions);

		// reordering has to be done after creating the submeshes because the vertices must be sorted per submesh
		reorderDeformableVertices(*mesh);

		// re-order vertices in graphical mesh to make tangent recompute faster
		reorderGraphicsVertices(graphicalLodIndex, false);

		mesh->release();
		mesh = NULL;

		// conditionally drop the immediate map (perf optimization)
		conditionalMergeMapping(*renderMeshAssetCopy, *mGraphicalLods[graphicalLodIndex]);
	}

	// keep physical meshes sorted such that the transition maps are correct
	// (needs to be called after 'addGraphicalMesh', so a graphicalLOD deletion is not missed)
	sortPhysicalMeshes();

	bool isIdentity = true;
	physx::Array<physx::PxI32> old2new(mBones.size(), -1);
	for (physx::PxU32 i = 0; i < mBones.size(); i++)
	{
		old2new[mBones[i].externalIndex] = mBones[i].internalIndex;
		isIdentity &= mBones[i].externalIndex == mBones[i].internalIndex;
	}

	if (!isIdentity && hasLod)
	{
		reinterpret_cast<NiApexRenderMeshAsset*>(mGraphicalLods[graphicalLodIndex]->renderMeshAssetPointer)->permuteBoneIndices(old2new);

		if (newPhysicalMesh)
		{
			const physx::PxU32 physicalMeshId = mGraphicalLods[graphicalLodIndex]->physicalMeshId;
			ClothingPhysicalMesh* mesh = mModule->createPhysicalMeshInternal(mPhysicalMeshes[physicalMeshId]);
			mesh->permuteBoneIndices(old2new);
			mesh->release();
		}
	}

	distributeSolverIterations();
}



bool ClothingAssetAuthoring::addPlatformToGraphicalLod(physx::PxU32 lod, NxPlatformTag platform)
{
	PxU32 index;
	if (!getGraphicalLodIndex(lod, index))
	{
		return false;
	}

	ClothingGraphicalLodParameters* graphicalLod = mGraphicalLods[index];

	// pushback to an array of strings
	NxParameterized::Handle handle(*graphicalLod);
	if (graphicalLod->getParameterHandle("platforms", handle) != NxParameterized::ERROR_NONE)
	{
		return false;
	}

	PxI32 numPlatforms = 0;
	graphicalLod->getArraySize(handle, numPlatforms);
	graphicalLod->resizeArray(handle, numPlatforms + 1);
	NxParameterized::Handle elementHandle(*graphicalLod);
	handle.getChildHandle(numPlatforms, elementHandle);
	graphicalLod->setParamString(elementHandle, platform);

	return true;
}


bool ClothingAssetAuthoring::removePlatform(physx::PxU32 lod,  NxPlatformTag platform)
{
	PxU32 index;
	if (!getGraphicalLodIndex(lod, index))
	{
		return false;
	}

	ClothingGraphicalLodParameters* graphicalLod = mGraphicalLods[index];

	NxParamArray<NxParameterized::DummyStringStruct> platforms(graphicalLod, "platforms", reinterpret_cast<NxParamDynamicArrayStruct*>(&graphicalLod->platforms));

	bool removed = false;
	for (PxI32 i = platforms.size() - 1; i >= 0 ; --i)
	{
		if (strcmp(platforms[i], platform) == 0)
		{
			platforms.replaceWithLast(i);
			removed = true;
		}
	}

	return removed;
}


physx::PxU32 ClothingAssetAuthoring::getNumPlatforms(physx::PxU32 lod)
{
	PxU32 index;
	if (!getGraphicalLodIndex(lod, index))
	{
		return 0;
	}

	ClothingGraphicalLodParameters* graphicalLod = mGraphicalLods[index];

	NxParamArray<NxParameterized::DummyStringStruct> platforms(graphicalLod, "platforms", reinterpret_cast<NxParamDynamicArrayStruct*>(&graphicalLod->platforms));
	return platforms.size();
}


NxPlatformTag ClothingAssetAuthoring::getPlatform(physx::PxU32 lod, physx::PxU32 i)
{
	PxU32 index;
	if (!getGraphicalLodIndex(lod, index))
	{
		return 0;
	}

	ClothingGraphicalLodParameters* graphicalLod = mGraphicalLods[index];

	NxParamArray<NxParameterized::DummyStringStruct> platforms(graphicalLod, "platforms", reinterpret_cast<NxParamDynamicArrayStruct*>(&graphicalLod->platforms));

	if (i >= platforms.size())
	{
		return 0;
	}

	return platforms[i];
}


bool ClothingAssetAuthoring::prepareForPlatform(NxPlatformTag platform)
{
	bool retVal = false;

	// go through graphical lods and remove the ones that are not tagged with "platform"
	for (PxI32 i = mGraphicalLods.size() - 1; i >= 0; --i)
	{
		ClothingGraphicalLodParameters* graphicalLod = mGraphicalLods[i];
		NxParamArray<NxParameterized::DummyStringStruct> platforms(graphicalLod, "platforms", reinterpret_cast<NxParamDynamicArrayStruct*>(&graphicalLod->platforms));

		bool keep = platforms.size() == 0; // keep it if it has no platforms at all
		for (PxU32 j = 0; j < platforms.size(); j++)
		{
			const char* storedPlatform = platforms[j].buf;
			if (strcmp(platform, storedPlatform) == 0)
			{
				keep = true;
			}
		}

		if (!keep)
		{
			setMeshes(graphicalLod->lod, NULL, NULL);    // remove
		}
		else
		{
			retVal = true;    // keep
		}
	}

	return retVal;
}



physx::PxU32 ClothingAssetAuthoring::getNumLods()
{
	return mGraphicalLods.size();
}



physx::PxI32 ClothingAssetAuthoring::getLodValue(physx::PxU32 lod)
{
	if (lod < mGraphicalLods.size())
	{
		return mGraphicalLods[lod]->lod;
	}

	return -1;
}



void ClothingAssetAuthoring::clearMeshes()
{
	for (physx::PxI32 i = mGraphicalLods.size() - 1; i >= 0; i--)
	{
		setMeshes(mGraphicalLods[i]->lod, NULL, NULL);
	}
	PX_ASSERT(mGraphicalLods.isEmpty());

	PX_ASSERT(mPhysicalMeshes.size() == mPhysicalMeshesInput.size());
	for (physx::PxU32 i = 0; i < mPhysicalMeshes.size(); i++)
	{
		mPhysicalMeshes[i]->destroy();
	}
	mPhysicalMeshes.clear();
	mPhysicalMeshesInput.clear();
}



NxClothingPhysicalMesh* ClothingAssetAuthoring::getClothingPhysicalMesh(physx::PxU32 graphicalLod)
{
	PxU32 graphicalLodIndex = 0;
	if (!getGraphicalLodIndex(graphicalLod, graphicalLodIndex))
	{
		return NULL;
	}

	physx::PxU32 physicalMeshId = mGraphicalLods[graphicalLodIndex]->physicalMeshId;

	if (physicalMeshId == (physx::PxU32) - 1)
	{
		return NULL;
	}

	return mModule->createPhysicalMeshInternal(mPhysicalMeshes[physicalMeshId]);
}



bool ClothingAssetAuthoring::getBoneBindPose(physx::PxU32 boneIndex, physx::PxMat44& bindPose)
{
	bool ret = false;
	if (boneIndex < mBones.size())
	{
		bindPose = mBones[boneIndex].bindPose;
		ret = true;
	}
	return ret;
}

bool ClothingAssetAuthoring::setBoneBindPose(physx::PxU32 boneIndex, const physx::PxMat44& bindPose)
{
	bool ret = false;
	if (boneIndex < mBones.size())
	{
		mBones[boneIndex].bindPose = bindPose;
		ret = true;
	}
	return ret;
}

void ClothingAssetAuthoring::setBoneInfo(PxU32 boneIndex, const char* boneName, const PxMat44& bindPose, PxI32 parentIndex)
{
	for (PxU32 i = 0; i < mBones.size(); i++)
	{
		if (mBones[i].externalIndex == (PxI32)boneIndex)
		{
			if (mBones[i].name == NULL || strcmp(mBones[i].name, boneName) != 0)
			{
				setBoneName(i, boneName);
			}

			mBones[i].bindPose = bindPose;

			// parentIndex should be an internal index, so let's see
			PxI32 oldInternalParent = mBones[i].parentIndex;
			PxI32 newInternalParent = oldInternalParent;
			PX_ASSERT((oldInternalParent == -1) == (parentIndex == -1)); // both are -1 or valid
			if (oldInternalParent >= 0)
			{
				PX_ASSERT(parentIndex >= 0);
				PX_ASSERT((PxU32)oldInternalParent < mBones.size());
				if ((PxU32)oldInternalParent < mBones.size())
				{
					PX_ASSERT(mBones[oldInternalParent].internalIndex == oldInternalParent); // just some sanity
					if (mBones[oldInternalParent].externalIndex != parentIndex)
					{
						// it seems the parent changed, let's hope this doesn't kill us
						for (PxU32 b = 0; b < mBones.size(); b++)
						{
							if (mBones[b].externalIndex == parentIndex)
							{
								newInternalParent = b;
								break;
							}
						}
					}
				}
			}
			mBones[i].parentIndex = newInternalParent;
			return;
		}
	}

	ClothingAssetParametersNS::BoneEntry_Type& bm = mBones.pushBack();
	bm.internalIndex = boneIndex;
	bm.externalIndex = boneIndex;
	bm.numMeshReferenced = 0;
	bm.numRigidBodiesReferenced = 0;
	bm.parentIndex = parentIndex;
	bm.bindPose = bindPose;
	setBoneName(mBones.size() - 1, boneName);
}



void ClothingAssetAuthoring::setRootBone(const char* boneName)
{
	mRootBoneName = boneName;
}



physx::PxU32 ClothingAssetAuthoring::addBoneConvex(const char* boneName, const physx::PxVec3* positions, physx::PxU32 numPositions)
{
	physx::PxI32 internalBoneIndex = getBoneInternalIndex(boneName);

	if (internalBoneIndex  == -1)
	{
		return 0;
	}

	return addBoneConvexInternal(internalBoneIndex , positions, numPositions);
}

physx::PxU32 ClothingAssetAuthoring::addBoneConvex(physx::PxU32 boneIndex, const physx::PxVec3* positions, physx::PxU32 numPositions)
{
	physx::PxI32 internalBoneIndex = getBoneInternalIndex(boneIndex);

	if (internalBoneIndex  == -1)
	{
		return 0;
	}

	return addBoneConvexInternal(internalBoneIndex , positions, numPositions);

}



void ClothingAssetAuthoring::addBoneCapsule(const char* boneName, physx::PxF32 capsuleRadius, physx::PxF32 capsuleHeight, const physx::PxMat44& localPose)
{
	physx::PxI32 internalBoneIndex = getBoneInternalIndex(boneName);

	if (internalBoneIndex  == -1)
	{
		return;
	}

	addBoneCapsuleInternal(internalBoneIndex, capsuleRadius, capsuleHeight, localPose);
}



void ClothingAssetAuthoring::addBoneCapsule(physx::PxU32 boneIndex, physx::PxF32 capsuleRadius, physx::PxF32 capsuleHeight, const physx::PxMat44& localPose)
{

	physx::PxI32 internalBoneIndex = getBoneInternalIndex(boneIndex);

	if (internalBoneIndex  == -1)
	{
		return;
	}

	addBoneCapsuleInternal(internalBoneIndex, capsuleRadius, capsuleHeight, localPose);
}



void ClothingAssetAuthoring::clearBoneActors(const char* boneName)
{
	physx::PxI32 internalBoneIndex = getBoneInternalIndex(boneName);

	if (internalBoneIndex == -1)
	{
		return;
	}

	clearBoneActorsInternal(internalBoneIndex);
}



void ClothingAssetAuthoring::clearBoneActors(physx::PxU32 boneIndex)
{
	physx::PxI32 internalBoneIndex = getBoneInternalIndex(boneIndex);

	if (internalBoneIndex == -1)
	{
		return;
	}

	clearBoneActorsInternal(internalBoneIndex);
}



void ClothingAssetAuthoring::clearAllBoneActors()
{
	mBoneActors.clear();
	mBoneVertices.clear();
	mBonePlanes.clear();
	clearCooked();
}



void ClothingAssetAuthoring::setCollision(const char** boneNames, physx::PxF32* radii, physx::PxVec3* localPositions, PxU32 numSpheres, physx::PxU16* pairs, physx::PxU32 numPairs)
{
	shdfnd::Array<PxU32> boneIndices(numSpheres, 0);
	for (PxU32 i = 0; i < numSpheres; ++i)
	{
		PxI32 internalBoneIndex = getBoneInternalIndex(boneNames[i]);
		if (internalBoneIndex < 0 || internalBoneIndex >= (PxI32)mBones.size())
		{
			APEX_INVALID_PARAMETER("Bone \'%s\' not found, setting to root", boneNames[i]);
			boneIndices[i] = 0;
		}
		else
		{
			boneIndices[i] = mBones[i].externalIndex;
		}
	}

	setCollision(boneIndices.begin(), radii, localPositions, numSpheres, pairs, numPairs);
}



void ClothingAssetAuthoring::setCollision(physx::PxU32* boneIndices, physx::PxF32* radii, physx::PxVec3* localPositions, PxU32 numSpheres, physx::PxU16* pairs, physx::PxU32 numPairs)
{
	if (numPairs & 0x1)
	{
		APEX_INVALID_PARAMETER("numPairs must be a multiple of 2");
		return;
	}

	mBoneSpheres.clear();
	for (PxU32 i = 0; i < numSpheres; ++i)
	{
		physx::PxI32 internalBoneIndex = getBoneInternalIndex(boneIndices[i]);

		PX_ASSERT(internalBoneIndex < (PxI32)mBones.size());
		internalBoneIndex = PxClamp(internalBoneIndex, 0, (PxI32)mBones.size() - 1);
		PxF32 radius = radii[i];
		if (radius <= 0.0f)
		{
			APEX_INVALID_PARAMETER("Sphere radius must be bigger than 0.0 (sphere %d has radius %f)", i, radius);
			radius = 0.0f;
		}
		ClothingAssetParametersNS::BoneSphere_Type& newEntry = mBoneSpheres.pushBack();
		memset(&newEntry, 0, sizeof(ClothingAssetParametersNS::BoneSphere_Type));
		newEntry.boneIndex = internalBoneIndex;
		newEntry.radius = radius;
		newEntry.localPos = localPositions[i];
	}

	mSpherePairs.clear();
	for (PxU32 i = 0; i < numPairs; i += 2)
	{
		const PxU16 p1 = PxMin(pairs[i + 0], pairs[i + 1]);
		const PxU16 p2 = PxMax(pairs[i + 0], pairs[i + 1]);
		if (p1 == p2)
		{
			APEX_INVALID_PARAMETER("pairs[%d] and pairs[%d] are identical (%d), skipping", i, i + 1, p1);
			continue;
		}
		else if (p1 >= mBoneSpheres.size() || p2 >= mBoneSpheres.size())
		{
			APEX_INVALID_PARAMETER("pairs[%d] = %d and pairs[%d] = %d are overflowing bone spheres, skipping", i, pairs[i], i + 1, pairs[i + 1]);
		}
		else
		{
			bool skip = false;
			for (PxU32 j = 0; j < mSpherePairs.size(); j += 2)
			{
				if (mSpherePairs[j] == p1 && mSpherePairs[j + 1] == p2)
				{
					APEX_INVALID_PARAMETER("pairs[%d] = %d and pairs[%d] = %d are a duplicate, skipping", i, pairs[i], i + 1, pairs[i + 1]);
					skip = true;
					break;
				}
			}
			if (!skip)
			{
				mSpherePairs.pushBack(p1);
				mSpherePairs.pushBack(p2);
			}
		}
	}
}



void ClothingAssetAuthoring::clearCollision()
{
	mBoneSpheres.clear();
	mSpherePairs.clear();
}



NxParameterized::Interface* ClothingAssetAuthoring::getMaterialLibrary()
{
	PX_ASSERT(mParams->materialLibrary != NULL);
	return mParams->materialLibrary;
}



bool ClothingAssetAuthoring::setMaterialLibrary(NxParameterized::Interface* materialLibrary, physx::PxU32 materialIndex, bool transferOwnership)
{
	if (strcmp(materialLibrary->className(), ClothingMaterialLibraryParameters::staticClassName()) == 0)
	{
		if (mParams->materialLibrary != NULL && mOwnsMaterialLibrary)
		{
			mParams->materialLibrary->destroy();
		}

		mParams->materialLibrary = materialLibrary;
		mParams->materialIndex = materialIndex;
		mOwnsMaterialLibrary = transferOwnership;

		return true;
	}

	return false;
}



NxParameterized::Interface* ClothingAssetAuthoring::getRenderMeshAssetAuthoring(physx::PxU32 lodLevel)
{
	NxParameterized::Interface* ret = NULL;

	if (lodLevel < mGraphicalLods.size())
	{
		ret = mGraphicalLods[lodLevel]->renderMeshAsset;
	}

	return ret;
}



NxParameterized::Interface* ClothingAssetAuthoring::releaseAndReturnNxParameterizedInterface()
{
	// this is important for destroy() !
	if (!mOwnsMaterialLibrary && mParams->materialLibrary != NULL)
	{
		NxParameterized::Interface* foreignMatLib = mParams->materialLibrary;

		// clone the mat lib
		mParams->materialLibrary = mParams->getTraits()->createNxParameterized(foreignMatLib->className());
		mParams->materialLibrary->copy(*foreignMatLib);
	}
	mOwnsMaterialLibrary = true;

	if (NxParameterized::ERROR_NONE != mParams->callPreSerializeCallback())
	{
		return NULL;
	}

	mParams->setSerializationCallback(NULL, NULL);

	// release the object without mParams
	NxParameterized::Interface* ret = mParams;
	mParams = NULL;

	release();
	return ret;
}



void ClothingAssetAuthoring::preSerialize(void* userData)
{
	PX_ASSERT(userData == NULL);
	PX_UNUSED(userData);


	NxParamArray<ClothingAssetParametersNS::CookedEntry_Type> cookedEntries(mParams, "cookedData", reinterpret_cast<NxParamDynamicArrayStruct*>(&mParams->cookedData));
	if (cookedEntries.isEmpty())
	{
		ClothingAssetParametersNS::CookedEntry_Type entry;
		entry.cookedData = NULL;
		entry.scale = 1.0f;
		cookedEntries.pushBack(entry);
	}

	for (PxU32 i = 0; i < cookedEntries.size(); i++)
	{
		if (cookedEntries[i].cookedData != NULL)
		{
			mPreviousCookedType = cookedEntries[i].cookedData->className();
			cookedEntries[i].cookedData->destroy();
			cookedEntries[i].cookedData = NULL;
		}

		PX_ASSERT(mPreviousCookedType != NULL);
		BackendFactory* cookingFactory = mModule->getBackendFactory(mPreviousCookedType);
		PX_ASSERT(cookingFactory != NULL);
		if (cookingFactory != NULL)
		{
			CookingAbstract* cookingJob = cookingFactory->createCookingJob();
			PX_ASSERT(cookingJob != NULL);
			if (cookingJob)
			{
				prepareCookingJob(*cookingJob, cookedEntries[i].scale, NULL, NULL);

				if (cookingJob->isValid())
				{
					cookedEntries[i].cookedData = cookingJob->execute();
				}
				PX_DELETE_AND_RESET(cookingJob);
			}
		}
	}

	compressBones();

	for (PxU32 graphicalMeshId = 0; graphicalMeshId < mGraphicalLods.size(); graphicalMeshId++)
	{
		NiApexRenderMeshAsset* renderMeshAsset = reinterpret_cast<NiApexRenderMeshAsset*>(mGraphicalLods[graphicalMeshId]->renderMeshAssetPointer);
		for (PxU32 submeshIndex = 0; submeshIndex < renderMeshAsset->getSubmeshCount(); submeshIndex++)
		{
			NxVertexFormat& format = renderMeshAsset->getNiSubmesh(submeshIndex).getVertexBufferWritable().getFormatWritable();
			format.setBufferAccess(format.getBufferIndexFromID(format.getSemanticID(NxRenderVertexSemantic::POSITION)), NxRenderDataAccess::DYNAMIC);
			format.setBufferAccess(format.getBufferIndexFromID(format.getSemanticID(NxRenderVertexSemantic::NORMAL)), NxRenderDataAccess::DYNAMIC);
			if (format.getBufferFormat(format.getBufferIndexFromID(format.getSemanticID(NxRenderVertexSemantic::TANGENT))) != NxRenderDataFormat::UNSPECIFIED)
			{
				format.setBufferAccess(format.getBufferIndexFromID(format.getSemanticID(NxRenderVertexSemantic::TANGENT)), NxRenderDataAccess::DYNAMIC);
				format.setBufferAccess(format.getBufferIndexFromID(format.getSemanticID(NxRenderVertexSemantic::BINORMAL)), NxRenderDataAccess::DYNAMIC);
			}
			format.setHasSeparateBoneBuffer(true);
		}
	}

	// create lod transition maps
	for (PxU32 i = 0; i < mPhysicalMeshes.size(); i++)
	{
		NxAbstractMeshDescription other;
		other.pPosition = mPhysicalMeshes[i]->physicalMesh.vertices.buf;
		other.pNormal = mPhysicalMeshes[i]->physicalMesh.normals.buf;
		other.numVertices = mPhysicalMeshes[i]->physicalMesh.numVertices;

		if (i > 0)
		{
			NxParamArray<SkinClothMap> transitionDown(mPhysicalMeshes[i], "transitionDown",
			        reinterpret_cast<NxParamDynamicArrayStruct*>(&mPhysicalMeshes[i]->transitionDown));
			if (transitionDown.isEmpty())
			{
				generateSkinClothMap(&other, 1, mPhysicalMeshes[i - 1]->physicalMesh, NULL, NULL, 0, transitionDown,
				                      mPhysicalMeshes[i]->transitionDownOffset, false, NULL);

				mPhysicalMeshes[i]->transitionDownThickness = 1.0f;
			}
		}
		if (i + 1 < mPhysicalMeshes.size())
		{

			NxParamArray<SkinClothMap> transitionUp(mPhysicalMeshes[i], "transitionUp",
			        reinterpret_cast<NxParamDynamicArrayStruct*>(&mPhysicalMeshes[i]->transitionUp));
			if (transitionUp.isEmpty())
			{
				generateSkinClothMap(&other, 1, mPhysicalMeshes[i + 1]->physicalMesh, NULL, NULL, 0, transitionUp,
				                      mPhysicalMeshes[i]->transitionUpOffset, false, NULL);

				mPhysicalMeshes[i]->transitionUpThickness = 1.0f;
			}
		}
	}

	updateBoundingBox();

	ClothingAsset::preSerialize(userData);
}



void ClothingAssetAuthoring::setToolString(const char* toolString)
{
	if (mParams != NULL)
	{
		NxParameterized::Handle handle(*mParams, "toolString");
		PX_ASSERT(handle.isValid());
		if (handle.isValid())
		{
			PX_ASSERT(handle.parameterDefinition()->type() == NxParameterized::TYPE_STRING);
			handle.setParamString(toolString);
		}
	}
}



void ClothingAssetAuthoring::applyTransformation(const PxMat44& transformation, PxF32 scale, bool applyToGraphics, bool applyToPhysics)
{
	if (applyToPhysics)
	{
		clearCooked();

		for (PxU32 i = 0; i < mPhysicalMeshes.size(); i++)
		{
			ClothingPhysicalMesh* mesh = mModule->createPhysicalMeshInternal(mPhysicalMeshes[i]);
			mesh->applyTransformation(transformation, scale);
			mesh->release();

			for (PxI32 j = 0; j < mPhysicalMeshes[i]->physicalLods.arraySizes[0]; j++)
			{
				mPhysicalMeshes[i]->physicalLods.buf[j].maxDistanceReduction *= scale;
			}
		}

		for (PxU32 i = 0; i < mBones.size(); i++)
		{
			mBones[i].bindPose.multiply(transformation, mBones[i].bindPose);
			mBones[i].bindPose.t *= scale;
		}

		for (PxU32 i = 0; i < mBoneVertices.size(); i++)
		{
			// PH: Do not apply transformation, bindpose was already adapted!
			//mBoneVertices[i] = transformation * mBoneVertices[i];
			mBoneVertices[i] *=  scale;
		}

		for (PxU32 i = 0; i < mBoneActors.size(); i++)
		{
			mBoneActors[i].capsuleRadius *= scale;
			mBoneActors[i].capsuleHeight *= scale;
			mBoneActors[i].localPose.t *= scale;
		}

		for (PxU32 i = 0; i < mBoneSpheres.size(); i++)
		{
			mBoneSpheres[i].radius *= scale;
			mBoneSpheres[i].localPos *= scale;
		}

		mParams->simulation.thickness *= scale;
		
		ClothingMaterialLibraryParameters* materialLib = static_cast<ClothingMaterialLibraryParameters*>(mParams->materialLibrary);
		for (PxI32 i = 0; i < materialLib->materials.arraySizes[0]; ++i)
		{
			materialLib->materials.buf[i].selfcollisionThickness *= scale;
		}
	}

	for (PxU32 i = 0; i < mGraphicalLods.size(); i++)
	{
		if (applyToGraphics)
		{
			reinterpret_cast<NiApexRenderMeshAsset*>(mGraphicalLods[i]->renderMeshAssetPointer)->applyTransformation(transformation, scale);
		}

		const PxMat34Legacy t = transformation;
		if (applyToPhysics && t.M.determinant() * scale < 0.0f)
		{
			const PxU32 numSkinClothMap = mGraphicalLods[i]->skinClothMap.arraySizes[0];
			SkinClothMap* skinClothMap = mGraphicalLods[i]->skinClothMap.buf;

			mGraphicalLods[i]->skinClothMapThickness *= scale;

			for (PxU32 j = 0; j < numSkinClothMap; j++)
			{
				skinClothMap[j].vertexBary.z *= scale;
				skinClothMap[j].normalBary.z *= scale;
				skinClothMap[j].tangentBary.z *= scale;
			}

			const PxU32 numTetraMap = mGraphicalLods[i]->tetraMap.arraySizes[0];
			ClothingGraphicalLodParametersNS::TetraLink_Type* tetraMap = mGraphicalLods[i]->tetraMap.buf;

			for (PxU32 j = 0; j < numTetraMap; j++)
			{
				PxVec3 bary = tetraMap[j].vertexBary;
				bary.z = 1.0f - bary.x - bary.y - bary.z;
				tetraMap[j].vertexBary = bary;

				bary = tetraMap[j].normalBary;
				bary.z = 1.0f - bary.x - bary.y - bary.z;
				tetraMap[j].normalBary = bary;
			}
		}
	}
}



void ClothingAssetAuthoring::updateBindPoses(const PxMat44* newBindPoses, PxU32 newBindPosesCount, bool isInternalOrder, bool collisionMaintainWorldPose)
{
	Array<PxMat34Legacy> transformation(mBones.size());

	bool hasSkew = false;
	for (PxU32 i = 0; i < mBones.size(); i++)
	{
		PX_ASSERT(mBones[i].internalIndex == (PxI32)i);
		const PxU32 externalIndex = isInternalOrder ? i : mBones[i].externalIndex;

		if (externalIndex >= newBindPosesCount)
		{
			transformation[i].id();
		}
		else
		{
			PxMat34Legacy temp;
			mBones[i].bindPose.getInverse(temp);
			mBones[i].bindPose = newBindPoses[externalIndex];
			transformation[i] = temp * mBones[i].bindPose;

			PxF32 det = transformation[i].M.determinant();

			if (abs(det) < 0.99f)
			{
				hasSkew = true;
			}
		}
	}

	if (hasSkew)
	{
		APEX_INVALID_PARAMETER("Skew on matrices is not allowed, aborting");
		return;
	}

	PxI32 errorInSize = -1;

	if (collisionMaintainWorldPose)
	{
		for (PxU32 i = 0; i < mBoneActors.size(); i++)
		{
			const PxU32 boneIndex = mBoneActors[i].boneIndex;
			if (!transformation[boneIndex].isIdentity())
			{
				if (mBoneActors[i].convexVerticesCount == 0)
				{
					// capsule
					PxMat34Legacy invTransformation;
					transformation[boneIndex].getInverse(invTransformation);
					mBoneActors[i].localPose = invTransformation * mBoneActors[i].localPose;
				}
				else
				{
					// convex
					const PxU32 start = mBoneActors[i].convexVerticesStart;
					const PxU32 end = start + mBoneActors[i].convexVerticesCount;
					for (PxU32 j = start; j < end; j++)
					{
						PX_ASSERT(j < mBoneVertices.size());
						mBoneVertices[j] = transformation[boneIndex] * mBoneVertices[j];
					}
				}
			}
			else
			{
				const PxU32 boneIdx = mBoneActors[i].boneIndex;
				const PxI32 externalIndex = isInternalOrder ? boneIdx : mBones[boneIdx].externalIndex;

				errorInSize = PxMax(errorInSize, externalIndex);
			}
		}
		for (PxU32 i = 0; i < mBoneSpheres.size(); i++)
		{
			const PxU32 boneIndex = mBoneSpheres[i].boneIndex;
			if (!transformation[boneIndex].isIdentity())
			{
				PxMat34Legacy invTransformation;
				transformation[boneIndex].getInverse(invTransformation);
				mBoneSpheres[i].localPos = invTransformation * mBoneSpheres[i].localPos;
			}
			else
			{
				const PxI32 externalIndex = isInternalOrder ? boneIndex : mBones[boneIndex].externalIndex;
				errorInSize = PxMax(errorInSize, externalIndex);
			}
		}
	}

#if 0
	// PH: This proved to be actually wrong. We should just adapt the bind pose without moving
	//     the meshes AT ALL.
	for (physx::PxU32 physicalMeshIndex = 0; physicalMeshIndex < mPhysicalMeshes.size(); physicalMeshIndex++)
	{

		const physx::PxU32 numBonesPerVertex = mPhysicalMeshes[physicalMeshIndex]->physicalMesh.numBonesPerVertex;
		if (numBonesPerVertex > 0)
		{
			const physx::PxU32 numVertices = mPhysicalMeshes[physicalMeshIndex]->physicalMesh.numVertices;
			physx::PxVec3* positions = mPhysicalMeshes[physicalMeshIndex]->physicalMesh.vertices.buf;
			physx::PxVec3* normals = mPhysicalMeshes[physicalMeshIndex]->physicalMesh.normals.buf;
			physx::PxU16* boneIndices = mPhysicalMeshes[physicalMeshIndex]->physicalMesh.boneIndices.buf;
			physx::PxF32* boneWeights = mPhysicalMeshes[physicalMeshIndex]->physicalMesh.boneWeights.buf;
			PX_ASSERT(positions != NULL);
			PX_ASSERT(normals != NULL);
			PX_ASSERT(numBonesPerVertex == 1 || boneWeights != NULL);

			for (physx::PxU32 vertexID = 0; vertexID < numVertices; vertexID++)
			{
				physx::PxVec3 position(0.0f, 0.0f, 0.0f);
				physx::PxVec3 normal(0.0f, 0.0f, 0.0f);
				physx::PxF32 sumWeight = 0.0f;
				for (physx::PxU32 k = 0; k < numBonesPerVertex; k++)
				{
					const physx::PxF32 weight = numBonesPerVertex > 1 ? boneWeights[vertexID * numBonesPerVertex + k] : 1.0f;
					if (weight > 0.0f)
					{
						const physx::PxMat34Legacy matrix = transformation[boneIndices[vertexID * numBonesPerVertex + k]];
						sumWeight += weight;
						position += matrix * positions[vertexID] * weight;
						normal += matrix.M * normals[vertexID] * weight;
					}
				}
				if (sumWeight > 0.0f)
				{
					PX_ASSERT(sumWeight >= 0.9999f);
					PX_ASSERT(sumWeight <= 1.0001f);

					positions[vertexID] = position;
					normals[vertexID] = normal;
				}
			}
		}
	}

	PX_ASSERT(mGraphicalLods.size() == mGraphicalMeshesRuntime.size());
	for (physx::PxU32 graphicalMeshIndex = 0; graphicalMeshIndex < mGraphicalLods.size(); graphicalMeshIndex++)
	{
		ClothingGraphicalMeshAsset meshAsset(*mGraphicalMeshesRuntime[graphicalMeshIndex]);
		const physx::PxU32 submeshCount = meshAsset.getSubmeshCount();

		for (physx::PxU32 submeshIndex = 0; submeshIndex < submeshCount; submeshIndex++)
		{
			const physx::PxU32 numBonesPerVertex = meshAsset.getNumBonesPerVertex(submeshIndex);
			const physx::PxU32 numVertices = meshAsset.getNumVertices(submeshIndex);

			if (numBonesPerVertex > 0 && numVertices > 0)
			{
				NxRenderDataFormat::Enum outFormat;
				const physx::PxU16* boneIndices = (const NxU16*)meshAsset.getVertexBuffer(submeshIndex, NxRenderVertexSemantic::BONE_INDEX, outFormat);
				const physx::PxF32* boneWeights = (const physx::PxF32*)meshAsset.getVertexBuffer(submeshIndex, NxRenderVertexSemantic::BONE_WEIGHT, outFormat);

				physx::PxVec3* positions =	(physx::PxVec3*)meshAsset.getVertexBuffer(submeshIndex, NxRenderVertexSemantic::POSITION, outFormat);
				physx::PxVec3* normals =	(physx::PxVec3*)meshAsset.getVertexBuffer(submeshIndex, NxRenderVertexSemantic::NORMAL, outFormat);
				physx::PxVec3* tangents =	(physx::PxVec3*)meshAsset.getVertexBuffer(submeshIndex, NxRenderVertexSemantic::TANGENT, outFormat);
				physx::PxVec3* bitangents =	(physx::PxVec3*)meshAsset.getVertexBuffer(submeshIndex, NxRenderVertexSemantic::BINORMAL, outFormat);

				if (boneWeights != NULL || numBonesPerVertex == 1)
				{
					for (physx::PxU32 vertexID = 0; vertexID < numVertices; vertexID++)
					{
						physx::PxVec3 position(0.0f, 0.0f, 0.0f);
						physx::PxVec3 normal(0.0f, 0.0f, 0.0f);
						physx::PxVec3 tangent(0.0f, 0.0f, 0.0f);
						physx::PxVec3 bitangent(0.0f, 0.0f, 0.0f);
						physx::PxF32 sumWeight = 0.0f;

						for (physx::PxU32 k = 0; k < numBonesPerVertex; k++)
						{
							const physx::PxF32 weight = (boneWeights == NULL) ? 1.0f : boneWeights[vertexID * numBonesPerVertex + k];
							if (weight > 0.0f)
							{
								const physx::PxMat34Legacy matrix = transformation[boneIndices[vertexID * numBonesPerVertex + k]];

								if (positions != NULL)
								{
									position += matrix * positions[vertexID] * weight;
								}
								if (normals != NULL)
								{
									normal += matrix.M * normals[vertexID] * weight;
								}
								if (tangents != NULL)
								{
									tangent += matrix.M * tangents[vertexID] * weight;
								}
								if (bitangents != NULL)
								{
									bitangent += matrix.M * bitangents[vertexID] * weight;
								}
							}
						}

						if (sumWeight != 0.0f)
						{
							PX_ASSERT(sumWeight > 0.9999f);
							PX_ASSERT(sumWeight < 1.0001f);

							// copy back
							if (positions != NULL)
							{
								positions[vertexID] = position;
							}
							if (normals != NULL)
							{
								normals[vertexID] = normal * NxClothingUserRecompute::invSqrt(normal.magnitudeSquared());
							}
							if (tangents != NULL)
							{
								tangents[vertexID] = tangent * NxClothingUserRecompute::invSqrt(tangent.magnitudeSquared());
							}
							if (bitangents != NULL)
							{
								bitangents[vertexID] = bitangent * NxClothingUserRecompute::invSqrt(bitangent.magnitudeSquared());
							}
						}
					}
				}
			}
		}
	}
#endif

	if (errorInSize > -1)
	{
		APEX_INVALID_PARAMETER("newBindPosesCount must be bigger than %d (is %d)", errorInSize, newBindPosesCount);
	}
}


void ClothingAssetAuthoring::destroy()
{
	if (!mOwnsMaterialLibrary)
	{
		PX_ASSERT(mParams != NULL);
		mParams->materialLibrary = NULL;
	}

	ClothingAsset::destroy(); // delete gets called in here
}



// ----- protected methods ----------------


physx::PxU32 ClothingAssetAuthoring::addBoneConvexInternal(physx::PxU32 boneIndex, const physx::PxVec3* positions, physx::PxU32 numPositions)
{
	// compute average
	physx::PxVec3 average(0.0f, 0.0f, 0.0f);

	physx::PxU32 maxNumberPositions = physx::PxMin(20u, numPositions);
	physx::PxU32 newNumPositions = 0;

	physx::Array<physx::PxVec3> newPositions(numPositions);
	physx::Array<physx::PxF32> minDist(numPositions, PX_MAX_F32);

	if (numPositions > 0)
	{
		for (physx::PxU32 i = 0; i < numPositions; i++)
		{
			average += positions[i];
		}
		average /= (physx::PxF32)numPositions;

		physx::PxF32 squaredDistFromAverage = (average - positions[0]).magnitudeSquared();
		physx::PxU32 startVertex = 0;
		for (physx::PxU32 i = 1; i < numPositions; i++)
		{
			physx::PxF32 squaredDist = (average - positions[i]).magnitudeSquared();
			if (squaredDist > squaredDistFromAverage)
			{
				squaredDistFromAverage = squaredDist;
				startVertex = i;
			}
		}

		for (physx::PxU32 i = 0; i < numPositions; i++)
		{
			newPositions[i] = positions[i];
		}

		if (startVertex != 0)
		{
			newPositions[0] = positions[startVertex];
			newPositions[startVertex] = positions[0];
		}


		for (physx::PxU32 i = 1; i < maxNumberPositions; i++)
		{
			physx::PxF32 max = 0.0f;
			physx::PxI32 maxj = -1;
			for (physx::PxU32 j = i; j < numPositions; j++)
			{
				const physx::PxF32 distSquared = (newPositions[j] - newPositions[i - 1]).magnitudeSquared();
				if (distSquared < minDist[j])
				{
					minDist[j] = distSquared;
				}

				if (minDist[j] > max)
				{
					max = minDist[j];
					maxj = j;
				}
			}

			if (maxj < 0)
			{
				break;
			}

			const physx::PxVec3 v = newPositions[i];
			newPositions[i] = newPositions[maxj];
			newPositions[maxj] = v;

			const physx::PxF32 dist = minDist[i];
			minDist[i] = minDist[maxj];
			minDist[maxj] = dist;
			newNumPositions = i + 1;
		}

		ClothingAssetParametersNS::ActorEntry_Type& newEntry = mBoneActors.pushBack();
		memset(&newEntry, 0, sizeof(ClothingAssetParametersNS::ActorEntry_Type));
		newEntry.boneIndex = boneIndex;
		newEntry.convexVerticesStart = mBoneVertices.size();
		newEntry.convexVerticesCount = newNumPositions;
		for (physx::PxU32 i = 0; i < newNumPositions; i++)
		{
			mBoneVertices.pushBack(newPositions[i]);
		}
		compressBoneCollision();
	}
	clearCooked();


	// extract planes from points
	ConvexHull convexHull;
	convexHull.init();
	Array<PxPlane> planes;

	convexHull.buildFromPoints(&newPositions[0], newNumPositions, sizeof(PxVec3));

	PxU32 planeCount = convexHull.getPlaneCount();
	if (planeCount + mBonePlanes.size() > 32)
	{
		APEX_DEBUG_WARNING("The asset is trying to use more than 32 planes for convexes. The collision convex will not be simulated with 3.x cloth.");
	}
	else
	{
		PxU32 convex = 0; // each bit references a plane
		for (PxU32 i = 0; i < planeCount; ++i)
		{
			PxPlane plane = convexHull.getPlane(i);
			convex |= 1 << mBonePlanes.size();
			
			ClothingAssetParametersNS::BonePlane_Type& newEntry = mBonePlanes.pushBack();
			memset(&newEntry, 0, sizeof(ClothingAssetParametersNS::BonePlane_Type));
			newEntry.boneIndex = boneIndex;
			newEntry.n = plane.n;
			newEntry.d = plane.d;
		}

		mCollisionConvexes.pushBack(convex);
	}


	return newNumPositions;
}



void ClothingAssetAuthoring::addBoneCapsuleInternal(physx::PxU32 boneIndex, physx::PxF32 capsuleRadius, physx::PxF32 capsuleHeight, const physx::PxMat44& localPose)
{
	PX_ASSERT(boneIndex < mBones.size());
	if (capsuleRadius > 0)
	{
		ClothingAssetParametersNS::ActorEntry_Type& newEntry = mBoneActors.pushBack();
		memset(&newEntry, 0, sizeof(ClothingAssetParametersNS::ActorEntry_Type));
		newEntry.boneIndex = boneIndex;
		newEntry.capsuleRadius = capsuleRadius;
		newEntry.capsuleHeight = capsuleHeight;
		newEntry.localPose = localPose;
	}
}



void ClothingAssetAuthoring::clearBoneActorsInternal(physx::PxI32 internalBoneIndex)
{
	PX_ASSERT(internalBoneIndex >= 0);
	for (physx::PxU32 i = 0; i < mBoneActors.size(); i++)
	{
		if (mBoneActors[i].boneIndex == internalBoneIndex)
		{
			mBoneActors[i].boneIndex = -1;
		}
	}

	compressBoneCollision();
}



void ClothingAssetAuthoring::compressBones() const
{
	if (mBones.isEmpty())
	{
		return;
	}

	// reset counters
	mParams->rootBoneIndex = PxU32(-1);
	for (physx::PxU32 i = 0; i < mBones.size(); i++)
	{
		mBones[i].numMeshReferenced = 0;
		mBones[i].numRigidBodiesReferenced = 0;

		if (strcmp(mBones[i].name, mRootBoneName.c_str()) == 0)
		{
			// set root bone index
			mParams->rootBoneIndex = i;

			// declare bone as referenced
			mBones[i].numRigidBodiesReferenced++;
		}
	}

	// update bone reference count from graphics mesh
	for (physx::PxU32 i = 0; i < mGraphicalLods.size(); i++)
	{
		ClothingGraphicalMeshAssetWrapper meshAsset(reinterpret_cast<NiApexRenderMeshAsset*>(mGraphicalLods[i]->renderMeshAssetPointer));

		for (physx::PxU32 submeshIndex = 0; submeshIndex < meshAsset.getSubmeshCount(); submeshIndex++)
		{
			NxRenderDataFormat::Enum outFormat;
			const physx::PxU16* boneIndices = (const physx::PxU16*)meshAsset.getVertexBuffer(submeshIndex, NxRenderVertexSemantic::BONE_INDEX, outFormat);
			if (outFormat != NxRenderDataFormat::USHORT1 && outFormat != NxRenderDataFormat::USHORT2 &&
			        outFormat != NxRenderDataFormat::USHORT3 && outFormat != NxRenderDataFormat::USHORT4)
			{
				boneIndices = NULL;
			}

			const physx::PxF32* boneWeights = (const physx::PxF32*)meshAsset.getVertexBuffer(submeshIndex, NxRenderVertexSemantic::BONE_WEIGHT, outFormat);
			if (outFormat != NxRenderDataFormat::FLOAT1 && outFormat != NxRenderDataFormat::FLOAT2 &&
			        outFormat != NxRenderDataFormat::FLOAT3 && outFormat != NxRenderDataFormat::FLOAT4)
			{
				boneWeights = NULL;
			}

			collectBoneIndices(meshAsset.getNumVertices(submeshIndex), boneIndices, boneWeights, meshAsset.getNumBonesPerVertex(submeshIndex));
		}
	}

	// update bone reference count from physics mesh
	for (physx::PxU32 i = 0; i < mPhysicalMeshes.size(); i++)
	{
		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh = mPhysicalMeshes[i]->physicalMesh;
		collectBoneIndices(physicalMesh.numVertices, physicalMesh.boneIndices.buf, physicalMesh.boneWeights.buf, physicalMesh.numBonesPerVertex);
	}

	// update bone reference count from bone actors
	for (physx::PxU32 i = 0; i < mBoneActors.size(); i++)
	{
		mBones[mBoneActors[i].boneIndex].numRigidBodiesReferenced++;
	}

	// update bone reference count from spheres
	for (physx::PxU32 i = 0; i < mBoneSpheres.size(); i++)
	{
		mBones[mBoneSpheres[i].boneIndex].numRigidBodiesReferenced++;
	}

	// update bone reference count from spheres
	for (physx::PxU32 i = 0; i < mBonePlanes.size(); i++)
	{
		mBones[mBonePlanes[i].boneIndex].numRigidBodiesReferenced++;
	}

	// sort the bones to the following structure:
	// |-- bones referenced by mesh --|-- bones referenced by collision RBs, but not mesh --|-- unreferenced bones --|
	{
		BoneEntryPredicate predicate;
		physx::sort(mBones.begin(), mBones.size(), predicate);
	}


	// create map from old indices to new ones and store the number of used bones
	physx::Array<physx::PxI32> old2new(mBones.size(), -1);
	mParams->bonesReferenced = 0;
	mParams->bonesReferencedByMesh = 0;
	for (physx::PxU32 i = 0; i < mBones.size(); i++)
	{
		if (mBones[i].numMeshReferenced > 0)
		{
			mParams->bonesReferencedByMesh++;
		}

		if (mBones[i].numMeshReferenced > 0 || mBones[i].numRigidBodiesReferenced > 0)
		{
			mParams->bonesReferenced++;
		}

		old2new[mBones[i].internalIndex] = i;
		mBones[i].internalIndex = i;
	}

	// update bone indices in parent
	for (physx::PxU32 i = 0; i < mBones.size(); i++)
	{
		if (mBones[i].parentIndex != -1)
		{
			mBones[i].parentIndex = old2new[mBones[i].parentIndex];
		}
	}

	// update bone indices in bone actors
	for (physx::PxU32 i = 0; i < mBoneActors.size(); i++)
	{
		PX_ASSERT(mBoneActors[i].boneIndex != -1);
		mBoneActors[i].boneIndex = old2new[mBoneActors[i].boneIndex];
	}

	// update bone indices in bone spheres
	for (physx::PxU32 i = 0; i < mBoneSpheres.size(); i++)
	{
		PX_ASSERT(mBoneSpheres[i].boneIndex != -1);
		mBoneSpheres[i].boneIndex = old2new[mBoneSpheres[i].boneIndex];
	}

	// update bone indices in bone planes
	for (physx::PxU32 i = 0; i < mBonePlanes.size(); i++)
	{
		PX_ASSERT(mBonePlanes[i].boneIndex != -1);
		mBonePlanes[i].boneIndex = old2new[mBonePlanes[i].boneIndex];
	}

	for (physx::PxU32 i = 0; i < mGraphicalLods.size(); i++)
	{
		reinterpret_cast<NiApexRenderMeshAsset*>(mGraphicalLods[i]->renderMeshAssetPointer)->permuteBoneIndices(old2new);
	}

	for (physx::PxU32 i = 0; i < mPhysicalMeshes.size(); i++)
	{
		ClothingPhysicalMesh* mesh = mModule->createPhysicalMeshInternal(mPhysicalMeshes[i]);
		mesh->permuteBoneIndices(old2new);
		mesh->release();
	}

	if (mParams->rootBoneIndex == PxU32(-1))
	{
		// no root bone defined, find one within referenced bones
		mParams->rootBoneIndex = 0;
		PxU32 minDepth = mBones.size();
		for (PxU32 i = 0; i < mParams->bonesReferenced; i++)
		{
			PxU32 depth = 0;
			PxI32 parent = mBones[i].parentIndex;
			while (parent != -1 && depth < mBones.size())
			{
				parent = mBones[parent].parentIndex;
				depth++;
			}

			if (depth < minDepth)
			{
				minDepth = depth;
				mParams->rootBoneIndex = i;
			}
		}
	}
	else
	{
		// update root bone index
		mParams->rootBoneIndex = old2new[mParams->rootBoneIndex];
	}
	PX_ASSERT(mParams->rootBoneIndex < mParams->bonesReferenced);
}



void ClothingAssetAuthoring::compressBoneCollision()
{
	const physx::PxVec3* oldBoneVertices = (const physx::PxVec3*)NiGetApexSDK()->getTempMemory(sizeof(physx::PxVec3) * mBoneVertices.size());
	if (oldBoneVertices == NULL)
	{
		return;
	}

	memcpy(const_cast<physx::PxVec3*>(oldBoneVertices), mBoneVertices.begin(), sizeof(physx::PxVec3) * mBoneVertices.size());

	// clean out all unused actors
	for (physx::PxI32 i = mBoneActors.size() - 1; i >= 0; i--)
	{
		if (mBoneActors[i].boneIndex < 0)
		{
			mBoneActors.replaceWithLast(i);
		}
	}
	if (!mBoneActors.isEmpty())
	{
		shdfnd::sort(mBoneActors.begin(), mBoneActors.size(), ActorEntryPredicate());
	}

	physx::PxU32 boneVerticesWritten = 0;
	for (physx::PxU32 i = 0; i < mBoneActors.size(); i++)
	{
		if (mBoneActors[i].convexVerticesCount == 0)
		{
			mBoneActors[i].convexVerticesStart = 0;
		}
		else
		{
			const physx::PxU32 oldStart = mBoneActors[i].convexVerticesStart;
			const physx::PxU32 count = mBoneActors[i].convexVerticesCount;
			mBoneActors[i].convexVerticesStart = boneVerticesWritten;
			for (physx::PxU32 j = 0; j < count; j++)
			{
				mBoneVertices[boneVerticesWritten++] = oldBoneVertices[oldStart + j];
			}
		}
	}
	mBoneVertices.resize(boneVerticesWritten);

	NiGetApexSDK()->releaseTempMemory(const_cast<physx::PxVec3*>(oldBoneVertices));
	clearCooked();
}



void ClothingAssetAuthoring::collectBoneIndices(physx::PxU32 numVertices, const physx::PxU16* boneIndices, const physx::PxF32* boneWeights, physx::PxU32 numBonesPerVertex) const
{
	for (physx::PxU32 i = 0; i < numVertices; i++)
	{
		for (physx::PxU32 j = 0; j < numBonesPerVertex; j++)
		{
			physx::PxU16 index = boneIndices[i * numBonesPerVertex + j];
			physx::PxF32 weight = (boneWeights == NULL) ? 1.0f : boneWeights[i * numBonesPerVertex + j];
			if (weight > 0.0f)
			{
				PX_ASSERT(index < mBones.size());
				PX_ASSERT(mBones[index].internalIndex == (physx::PxI32)index);
				mBones[index].numMeshReferenced++;
			}
		}
	}
}



struct Confidentially
{
	Confidentially() : maxDistConfidence(0.0f), collisionDistConfidence(0.0f), collisionRadiusConfidence(0.0f), normalConfidence(0.0f) {}
	float maxDistConfidence;
	float collisionDistConfidence;
	float collisionRadiusConfidence;
	float normalConfidence;
};



void ClothingAssetAuthoring::updateMappingAuthoring(ClothingGraphicalLodParameters& graphicalLod, NiApexRenderMeshAsset* renderMeshAssetCopy,
        NiApexRenderMeshAssetAuthoring* renderMeshAssetOrig, PxF32 normalResemblance, bool ignoreUnusedVertices, IProgressListener* progressListener)
{
	if (graphicalLod.physicalMeshId == (PxU32) - 1 || renderMeshAssetCopy == NULL)
	{
		return;
	}

	const PxU32 physicalMeshId = graphicalLod.physicalMeshId;

	if (normalResemblance < 0)
	{
		APEX_DEBUG_WARNING("A normal resemblance of %f not allowed, must be positive.", normalResemblance);
		normalResemblance = 90.0f;
	}
	else if (normalResemblance < 5)
	{
		APEX_DEBUG_WARNING("A physicalNormal resemblance of %f is very small, it might discard too many values", normalResemblance);
	}
	else if (normalResemblance > 90.0f)
	{
		normalResemblance = 90.0f;
	}

	ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh = mPhysicalMeshes[physicalMeshId]->physicalMesh;
	PxU32* masterFlags = mPhysicalMeshesInput[physicalMeshId]->getMasterFlagsBuffer();

	PX_ASSERT(graphicalLod.immediateClothMap.buf == NULL);
	PX_ASSERT(graphicalLod.skinClothMapB.buf == NULL);
	PX_ASSERT(graphicalLod.tetraMap.buf == NULL);

	HierarchicalProgressListener progress(100, progressListener);

	Array<NxAbstractMeshDescription> targetMeshes(renderMeshAssetCopy->getSubmeshCount());
	PxU32 numTotalVertices = 0;
	bool hasTangents = false;
	for (PxU32 submeshIndex = 0; submeshIndex < targetMeshes.size(); submeshIndex++)
	{
		const NxVertexBuffer& vb = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer();
		const NxVertexFormat& vf = vb.getFormat();

		physx::apex::NxRenderDataFormat::Enum outFormat;

		PxU32 bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::apex::NxRenderVertexSemantic::POSITION));
		targetMeshes[submeshIndex].pPosition = (PxVec3*)vb.getBufferAndFormat(outFormat, bufferIndex);
		PX_ASSERT(outFormat == physx::apex::NxRenderDataFormat::FLOAT3);

		bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::apex::NxRenderVertexSemantic::NORMAL));
		targetMeshes[submeshIndex].pNormal = (PxVec3*)vb.getBufferAndFormat(outFormat, bufferIndex);
		if (outFormat != physx::apex::NxRenderDataFormat::FLOAT3)
		{
			// Phil - you might need to handle other normal formats
			targetMeshes[submeshIndex].pNormal = NULL;
		}

		bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::apex::NxRenderVertexSemantic::TANGENT));
		const void* tangents = vb.getBufferAndFormat(outFormat, bufferIndex);
		if (outFormat == physx::apex::NxRenderDataFormat::FLOAT3)
		{
			targetMeshes[submeshIndex].pTangent = (PxVec3*)tangents;
			hasTangents = true;
		}
		else if (outFormat == physx::apex::NxRenderDataFormat::FLOAT4)
		{
			targetMeshes[submeshIndex].pTangent4 = (PxVec4*)tangents;
			hasTangents = true;
		}

		const PxU32 numVertices = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexCount(0);
		targetMeshes[submeshIndex].numVertices = numVertices;

		bufferIndex = vf.getBufferIndexFromID(vf.getID(LATCH_TO_NEAREST_SLAVE_NAME));
		PxU32* submeshSlaveFlags = (PxU32*)vb.getBufferAndFormat(outFormat, bufferIndex);
		targetMeshes[submeshIndex].pVertexFlags = submeshSlaveFlags;
		PX_ASSERT(submeshSlaveFlags == NULL || outFormat == physx::apex::NxRenderDataFormat::UINT1);

		bufferIndex = vf.getBufferIndexFromID(vf.getID(LATCH_TO_NEAREST_MASTER_NAME));
		PxU32* submeshMasterFlags = (PxU32*)vb.getBufferAndFormat(outFormat, bufferIndex);
		PX_ASSERT(submeshMasterFlags == NULL || outFormat == physx::apex::NxRenderDataFormat::UINT1);

		if (submeshSlaveFlags != NULL && submeshMasterFlags != NULL)
		{
			// overwrite the empty slaves with the master mask
			// provides self-attachment, very important!
			// the original slave values are in the orig render mesh still
			for (PxU32 i = 0; i < numVertices; i++)
			{
				if (submeshSlaveFlags[i] == 0)
				{
					submeshSlaveFlags[i] = submeshMasterFlags[i];
				}
				if (submeshSlaveFlags[i] == 0)
				{
					submeshSlaveFlags[i] = 0xffffffff;
				}
			}
		}

		numTotalVertices += targetMeshes[submeshIndex].numVertices;
	}

	if (physicalMesh.isTetrahedralMesh)
	{
		NxParamArray<ClothingGraphicalLodParametersNS::TetraLink_Type> tetraMap(&graphicalLod, "tetraMap", reinterpret_cast<NxParamDynamicArrayStruct*>(&graphicalLod.tetraMap));
		progress.setSubtaskWork(50, "Generate Tetra Map");
		generateTetraMap(targetMeshes.begin(), targetMeshes.size(), physicalMesh, masterFlags, tetraMap, &progress);
		progress.completeSubtask();
	}
	else
	{
		NxParamArray<physx::PxU32> immediateClothMap(&graphicalLod, "immediateClothMap", reinterpret_cast<NxParamDynamicArrayStruct*>(&graphicalLod.immediateClothMap));
		physx::PxU32 numNotFoundVertices = 0;
		progress.setSubtaskWork(5, "Generate immediate mapping");


		generateImmediateClothMap(targetMeshes.begin(), targetMeshes.size(), physicalMesh, masterFlags,
		                          0.0f, numNotFoundVertices, normalResemblance, immediateClothMap, &progress);

		progress.completeSubtask();

		if (immediateClothMap.isEmpty() || numNotFoundVertices > 0 || hasTangents)
		{
			// if more than 3/4 of all vertices are not found, completely forget about immediate mode.
			const bool clearImmediateMap = (numNotFoundVertices > numTotalVertices * 3 / 4);

			progress.setSubtaskWork(45, "Generate mesh-mesh skinning");
			NxParamArray<SkinClothMap> skinClothMap(&graphicalLod, "skinClothMap",
			        reinterpret_cast<NxParamDynamicArrayStruct*>(&graphicalLod.skinClothMap));

			generateSkinClothMap(targetMeshes.begin(), targetMeshes.size(), physicalMesh, masterFlags,
			                      graphicalLod.immediateClothMap.buf, numNotFoundVertices, skinClothMap,
			                      graphicalLod.skinClothMapOffset, clearImmediateMap, &progress);

			graphicalLod.skinClothMapThickness = 1.0f;

			progress.completeSubtask();

			if (clearImmediateMap)
			{
				immediateClothMap.clear();
			}
		}
	}

	if (physicalMesh.constrainCoefficients.arraySizes[0] == 0)
	{
		progress.setSubtaskWork(50, "Update Painting");

		// update painting stuff from all submeshes

		ClothingPhysicalMesh* tempMesh = mModule->createPhysicalMeshInternal(mPhysicalMeshes[physicalMeshId]);

		tempMesh->allocateNormalBuffer();
		tempMesh->allocateConstrainCoefficientBuffer();

		NxAbstractMeshDescription pMesh;
		pMesh.pPosition = physicalMesh.vertices.buf;
		pMesh.pNormal = physicalMesh.normals.buf;
		ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* myConstrains = physicalMesh.constrainCoefficients.buf;
		pMesh.numVertices = physicalMesh.numVertices;

		pMesh.pIndices = physicalMesh.indices.buf;
		pMesh.numIndices = physicalMesh.numIndices;

		PxU32 maxNumBonesPerVertex = 0;
		for (PxU32 submeshIndex = 0; submeshIndex < renderMeshAssetCopy->getSubmeshCount(); submeshIndex++)
		{
			const NxVertexFormat& vf = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer().getFormat();
			NxRenderDataFormat::Enum format = vf.getBufferFormat(vf.getBufferIndexFromID(vf.getSemanticID(NxRenderVertexSemantic::BONE_INDEX)));
			const PxU32 numBonesPerVertex = vertexSemanticFormatElementCount(NxRenderVertexSemantic::BONE_INDEX, format);
			maxNumBonesPerVertex = PxMax(maxNumBonesPerVertex, numBonesPerVertex);
		}

		if (maxNumBonesPerVertex > 0)
		{
			tempMesh->setNumBonesPerVertex(maxNumBonesPerVertex);
			tempMesh->allocateBoneIndexAndWeightBuffers();
			pMesh.numBonesPerVertex = maxNumBonesPerVertex;
			pMesh.pBoneIndices = physicalMesh.boneIndices.buf;
			pMesh.pBoneWeights = physicalMesh.boneWeights.buf;
		}

		Array<Confidentially> confidence(pMesh.numVertices);


		PxU32 graphicalVertexOffset = 0;
		const PxU32 numGraphicalVerticesTotal = numTotalVertices;
		PX_ASSERT(renderMeshAssetCopy->getSubmeshCount() == renderMeshAssetOrig->getSubmeshCount());

		for (PxU32 submeshIndex = 0; submeshIndex < renderMeshAssetCopy->getSubmeshCount(); submeshIndex++)
		{
			const NxVertexBuffer& vb = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer();

			if (vb.getVertexCount() == 0)
			{
				APEX_DEBUG_WARNING("submesh %d has no vertices at all!", submeshIndex);
				continue;
			}

			const NxVertexBuffer& vbOrig = renderMeshAssetOrig->getSubmesh(submeshIndex).getVertexBuffer();
			PX_ASSERT(vbOrig.getVertexCount() == vb.getVertexCount());

			const NxVertexFormat& vf = vb.getFormat();
			const physx::PxU32 graphicalMaxDistanceIndex = vf.getBufferIndexFromID(vf.getID(MAX_DISTANCE_NAME));
			NxRenderDataFormat::Enum outFormat = vf.getBufferFormat(graphicalMaxDistanceIndex);
			const physx::PxF32* graphicalMaxDistance = outFormat == NxRenderDataFormat::UNSPECIFIED ? NULL :
			        reinterpret_cast<const physx::PxF32*>(vb.getBuffer(graphicalMaxDistanceIndex));
			PX_ASSERT(graphicalMaxDistance == NULL || outFormat == NxRenderDataFormat::FLOAT1);

			const physx::PxU32 graphicalCollisionSphereRadiusIndex = vf.getBufferIndexFromID(vf.getID(COLLISION_SPHERE_RADIUS_NAME));
			outFormat = vf.getBufferFormat(graphicalCollisionSphereRadiusIndex);
			const physx::PxF32* graphicalCollisionSphereRadius = outFormat == NxRenderDataFormat::UNSPECIFIED ? NULL :
			        reinterpret_cast<const physx::PxF32*>(vb.getBuffer(graphicalCollisionSphereRadiusIndex));
			PX_ASSERT(graphicalCollisionSphereRadius == NULL || outFormat == NxRenderDataFormat::FLOAT1);

			const physx::PxU32 graphicalCollisionSphereDistanceIndex = vf.getBufferIndexFromID(vf.getID(COLLISION_SPHERE_DISTANCE_NAME));
			outFormat = vf.getBufferFormat(graphicalCollisionSphereDistanceIndex);
			const physx::PxF32* graphicalCollisionSphereDistance = outFormat == NxRenderDataFormat::UNSPECIFIED ? NULL :
			        reinterpret_cast<const physx::PxF32*>(vb.getBuffer(graphicalCollisionSphereDistanceIndex));
			PX_ASSERT(graphicalCollisionSphereDistance == NULL || outFormat == NxRenderDataFormat::FLOAT1);
			PX_ASSERT((graphicalCollisionSphereDistance == NULL && graphicalCollisionSphereRadius == NULL) || (graphicalCollisionSphereDistance != NULL && graphicalCollisionSphereRadius != NULL));

			const PxU32 graphicalUsedForPhysicsIndex = vf.getBufferIndexFromID(vf.getID(USED_FOR_PHYSICS_NAME));
			outFormat = vf.getBufferFormat(graphicalUsedForPhysicsIndex);
			const PxU8* graphicalUsedForPhysics = outFormat == NxRenderDataFormat::UNSPECIFIED ? NULL :
			                                      reinterpret_cast<const PxU8*>(vb.getBuffer(graphicalUsedForPhysicsIndex));
			PX_ASSERT(graphicalUsedForPhysics == NULL || outFormat == NxRenderDataFormat::UBYTE1);

			const PxU32 graphicalSlaveIndex = vbOrig.getFormat().getBufferIndexFromID(vbOrig.getFormat().getID(LATCH_TO_NEAREST_SLAVE_NAME));
			outFormat = vbOrig.getFormat().getBufferFormat(graphicalSlaveIndex);
			const PxU32* graphicalSlavesOrig = outFormat != NxRenderDataFormat::UINT1 ? NULL :
			                                   reinterpret_cast<const PxU32*>(vbOrig.getBuffer(graphicalSlaveIndex));

			// should not be used anymore!
			outFormat = NxRenderDataFormat::UNSPECIFIED;

			physx::NxRenderDataFormat::Enum normalFormat;
			physx::PxU32 bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::NxRenderVertexSemantic::NORMAL));
			const physx::PxVec3* graphicalNormals = (const physx::PxVec3*)vb.getBufferAndFormat(normalFormat, bufferIndex);
			if (normalFormat != physx::NxRenderDataFormat::FLOAT3)
			{
				// Phil - you might need to handle other normal formats
				graphicalNormals = NULL;
			}

			physx::NxRenderDataFormat::Enum positionFormat;
			bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::NxRenderVertexSemantic::POSITION));
			const physx::PxVec3* graphicalPositions = (const physx::PxVec3*)vb.getBufferAndFormat(positionFormat, bufferIndex);
			if (positionFormat != physx::NxRenderDataFormat::FLOAT3)
			{
				graphicalPositions = NULL;
			}

			physx::NxRenderDataFormat::Enum boneIndexFormat;
			bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::NxRenderVertexSemantic::BONE_INDEX));
			const physx::PxU16* graphicalBoneIndices = (const physx::PxU16*)vb.getBufferAndFormat(boneIndexFormat, bufferIndex);
			if (boneIndexFormat != physx::NxRenderDataFormat::USHORT1 && boneIndexFormat != physx::NxRenderDataFormat::USHORT2 &&
			        boneIndexFormat != physx::NxRenderDataFormat::USHORT3 && boneIndexFormat != physx::NxRenderDataFormat::USHORT4)
			{
				// Phil - you might need to handle other normal formats
				graphicalBoneIndices = NULL;
			}

			physx::NxRenderDataFormat::Enum boneWeightFormat;
			bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::NxRenderVertexSemantic::BONE_WEIGHT));
			const physx::PxF32* graphicalBoneWeights = (const physx::PxF32*)vb.getBufferAndFormat(boneWeightFormat, bufferIndex);
			if (boneWeightFormat != physx::NxRenderDataFormat::FLOAT1 && boneWeightFormat != physx::NxRenderDataFormat::FLOAT2 &&
			        boneWeightFormat != physx::NxRenderDataFormat::FLOAT3 && boneWeightFormat != physx::NxRenderDataFormat::FLOAT4)
			{
				// Phil - you might need to handle other normal formats
				graphicalBoneWeights = NULL;
			}

			const physx::PxU32 numBonesPerVertex = vertexSemanticFormatElementCount(NxRenderVertexSemantic::BONE_INDEX, boneIndexFormat);
			PX_ASSERT((graphicalBoneIndices == NULL && graphicalBoneWeights == NULL && numBonesPerVertex == 0) ||
			          (graphicalBoneIndices != NULL && numBonesPerVertex > 0));

			const PxU32 numGraphicalVertices = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexCount(0);
			for (PxU32 graphicalVertexIndex = 0; graphicalVertexIndex < numGraphicalVertices; graphicalVertexIndex++)
			{
				if ((graphicalVertexIndex & 0xff) == 0)
				{
					const PxU32 percent = 100 * (graphicalVertexIndex + graphicalVertexOffset) / numGraphicalVerticesTotal;
					progress.setProgress(percent);
				}

				// figure out how many physical vertices this graphical vertex relates to
				PxU32 indices[4];
				PxF32 trust[4];

				const bool isSlave =
				    (graphicalUsedForPhysics != NULL && graphicalUsedForPhysics[graphicalVertexIndex] == 0) ||
				    (graphicalSlavesOrig != NULL && graphicalSlavesOrig[graphicalVertexIndex] != 0);

				PxU32 numIndices = getCorrespondingPhysicalVertices(graphicalLod, submeshIndex, graphicalVertexIndex, pMesh, graphicalVertexOffset, indices, trust);

				// now we have the relevant physical vertices in indices[4]

				if (graphicalMaxDistance != NULL && graphicalMaxDistance[graphicalVertexIndex] != mInvalidConstrainCoefficients.maxDistance &&
				        graphicalMaxDistance[graphicalVertexIndex] < 0.0f)
				{
					APEX_INVALID_PARAMETER("Max Distance at vertex %d (submesh %d) must be >= 0.0 (is %f) or equal to invalid (%f)",
					                       graphicalVertexIndex, submeshIndex, graphicalMaxDistance[graphicalVertexIndex], mInvalidConstrainCoefficients.maxDistance);
				}

				for (PxU32 temporalIndex = 0; temporalIndex < numIndices; temporalIndex++)
				{
					const PxU32 vertexIndex = indices[temporalIndex];
					const PxF32 vertexTrust = trust[temporalIndex];

					if (ignoreUnusedVertices && isSlave)
					{
						continue;
					}

					if (vertexTrust < -0.0001f)
					{
						continue;
					}

					const physx::PxF32 confidenceDelta = 0.1f;
					if (graphicalMaxDistance != NULL && graphicalMaxDistance[graphicalVertexIndex] != mInvalidConstrainCoefficients.maxDistance)
					{
						if (vertexTrust + confidenceDelta > confidence[vertexIndex].maxDistConfidence)
						{
							confidence[vertexIndex].maxDistConfidence = vertexTrust;

							physx::PxF32& target = myConstrains[vertexIndex].maxDistance;
							const physx::PxF32 source = graphicalMaxDistance[graphicalVertexIndex];

							target = source;
						}
					}

					if (graphicalCollisionSphereDistance != NULL && graphicalCollisionSphereDistance[graphicalVertexIndex] != mInvalidConstrainCoefficients.collisionSphereDistance)
					{
						if (vertexTrust + confidenceDelta > confidence[vertexIndex].collisionDistConfidence)
						{
							confidence[vertexIndex].collisionDistConfidence = vertexTrust;

							const physx::PxF32 source = graphicalCollisionSphereDistance[graphicalVertexIndex];
							physx::PxF32& target = myConstrains[vertexIndex].collisionSphereDistance;

							target = source;
						}
					}

					if (graphicalCollisionSphereRadius != NULL && graphicalCollisionSphereRadius[graphicalVertexIndex] != mInvalidConstrainCoefficients.collisionSphereRadius)
					{
						if (vertexTrust + confidenceDelta > confidence[vertexIndex].collisionRadiusConfidence)
						{
							confidence[vertexIndex].collisionRadiusConfidence = vertexTrust;

							physx::PxF32& target = myConstrains[vertexIndex].collisionSphereRadius;
							const physx::PxF32 source = graphicalCollisionSphereRadius[graphicalVertexIndex];

							target = source;
						}
					}

					if (graphicalBoneIndices != NULL)
					{
						for (physx::PxU32 i = 0; i < numBonesPerVertex; i++)
						{
							const physx::PxF32 weight = (graphicalBoneWeights != NULL) ? graphicalBoneWeights[graphicalVertexIndex * numBonesPerVertex + i] : 1.0f;
							if (weight > 0.0f)
							{
								tempMesh->addBoneToVertex(vertexIndex, graphicalBoneIndices[graphicalVertexIndex * numBonesPerVertex + i], weight);
							}
						}
					}

					if (!isSlave)
					{
						if (vertexTrust + confidenceDelta > confidence[vertexIndex].normalConfidence)
						{
							confidence[vertexIndex].normalConfidence = vertexTrust;

							pMesh.pNormal[vertexIndex] = graphicalNormals[graphicalVertexIndex];
						}
					}
				}
			}

			graphicalVertexOffset += numGraphicalVertices;
		}


		bool hasMaxDistance = false;
		bool hasCollisionSphereDistance = false;
		bool hasCollisionSphereRadius = false;
		bool hasBoneWeights = false;
		for (PxU32 i = 0; i < renderMeshAssetCopy->getSubmeshCount(); i++)
		{
			const NxVertexFormat& vf = renderMeshAssetCopy->getSubmesh(i).getVertexBuffer().getFormat();
			NxRenderDataFormat::Enum outFormat;
			outFormat = vf.getBufferFormat(vf.getBufferIndexFromID(vf.getID(MAX_DISTANCE_NAME)));
			hasMaxDistance				|= outFormat == NxRenderDataFormat::FLOAT1;
			outFormat = vf.getBufferFormat(vf.getBufferIndexFromID(vf.getID(COLLISION_SPHERE_DISTANCE_NAME)));
			hasCollisionSphereDistance	|= outFormat == NxRenderDataFormat::FLOAT1;
			outFormat = vf.getBufferFormat(vf.getBufferIndexFromID(vf.getID(COLLISION_SPHERE_RADIUS_NAME)));
			hasCollisionSphereRadius	|= outFormat == NxRenderDataFormat::FLOAT1;
			outFormat = vf.getBufferFormat(vf.getBufferIndexFromID(vf.getSemanticID(NxRenderVertexSemantic::BONE_WEIGHT)));
			hasBoneWeights				|= outFormat != NxRenderDataFormat::UNSPECIFIED;
		}

		// all values that have not been set are set by nearest neighbor
		physx::PxU32 count[5] = { 0, 0, 0, 0, 0 };
		const physx::PxU32 numVertices = physicalMesh.numVertices;
		for (physx::PxU32 vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
		{
			physx::PxF32& physicalMaxDistance = myConstrains[vertexIndex].maxDistance;
			if (physicalMaxDistance == PX_MAX_F32)
			{
				count[0]++;
				physx::PxU32 submeshIndex = 0, graphicalVertexIndex = 0;
				if (hasMaxDistance && getClosestVertex(renderMeshAssetOrig, pMesh.pPosition[vertexIndex], submeshIndex, graphicalVertexIndex, MAX_DISTANCE_NAME, ignoreUnusedVertices))
				{
					const NxVertexFormat& vf = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer().getFormat();
					const physx::PxU32 graphicalMaxDistanceIndex = vf.getBufferIndexFromID(vf.getID(MAX_DISTANCE_NAME));
					NxRenderDataFormat::Enum outFormat = vf.getBufferFormat(graphicalMaxDistanceIndex);
					const physx::PxF32* graphicalMaxDistance = reinterpret_cast<const physx::PxF32*>(renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer().getBuffer(graphicalMaxDistanceIndex));

					PX_ASSERT(outFormat == NxRenderDataFormat::FLOAT1);
					if (outFormat == NxRenderDataFormat::FLOAT1)
					{
						physicalMaxDistance = PxMax(0.0f, graphicalMaxDistance[graphicalVertexIndex]);
					}
				}
				else
				{
					physicalMaxDistance = 0.0f;
				}
			}

			physx::PxF32& physicalCollisionSphereDistance = myConstrains[vertexIndex].collisionSphereDistance;
			if (physicalCollisionSphereDistance == PX_MAX_F32)
			{
				count[1]++;
				physx::PxU32 submeshIndex = 0, graphicalVertexIndex = 0;
				if (hasCollisionSphereDistance && getClosestVertex(renderMeshAssetOrig, pMesh.pPosition[vertexIndex], submeshIndex, graphicalVertexIndex, COLLISION_SPHERE_DISTANCE_NAME, ignoreUnusedVertices))
				{
					const NxVertexFormat& vf = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer().getFormat();
					const physx::PxU32 graphicalCollisionSphereDistanceIndex = vf.getBufferIndexFromID(vf.getID(COLLISION_SPHERE_DISTANCE_NAME));
					NxRenderDataFormat::Enum outFormat = vf.getBufferFormat(graphicalCollisionSphereDistanceIndex);
					const physx::PxF32* graphicalCollisionSphereDistance = reinterpret_cast<const physx::PxF32*>(renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer().getBuffer(graphicalCollisionSphereDistanceIndex));

					PX_ASSERT(outFormat == NxRenderDataFormat::FLOAT1);
					if (outFormat == NxRenderDataFormat::FLOAT1)
					{
						physicalCollisionSphereDistance = graphicalCollisionSphereDistance[graphicalVertexIndex];
					}
				}
				else
				{
					physicalCollisionSphereDistance = 0.0f;
				}
			}

			physx::PxF32& physicalCollisionSphereRadius = myConstrains[vertexIndex].collisionSphereRadius;
			if (physicalCollisionSphereRadius == PX_MAX_F32)
			{
				count[2]++;
				physx::PxU32 submeshIndex = 0, graphicalVertexIndex = 0;
				if (hasCollisionSphereRadius && getClosestVertex(renderMeshAssetOrig, pMesh.pPosition[vertexIndex], submeshIndex, graphicalVertexIndex, COLLISION_SPHERE_RADIUS_NAME, ignoreUnusedVertices))
				{
					const NxVertexFormat& vf = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer().getFormat();
					const physx::PxU32 graphicalCollisionSphereRadiusIndex = vf.getBufferIndexFromID(vf.getID(COLLISION_SPHERE_RADIUS_NAME));
					NxRenderDataFormat::Enum outFormat = vf.getBufferFormat(graphicalCollisionSphereRadiusIndex);
					const physx::PxF32* graphicalCollisionSphereRadius = reinterpret_cast<const physx::PxF32*>(renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer().getBuffer(graphicalCollisionSphereRadiusIndex));

					PX_ASSERT(outFormat == NxRenderDataFormat::FLOAT1);
					if (outFormat == NxRenderDataFormat::FLOAT1)
					{
						physicalCollisionSphereRadius = graphicalCollisionSphereRadius[graphicalVertexIndex];
					}
				}
				else
				{
					physicalCollisionSphereRadius = 0.0f;
				}
			}

			PxVec3& physicalNormal = pMesh.pNormal[vertexIndex];
			if (physicalNormal.isZero() && !(mDeriveNormalsFromBones && pMesh.numBonesPerVertex > 0))
			{
				count[3]++;
				PxU32 submeshIndex = 0, graphicalVertexIndex = 0;
				if (getClosestVertex(renderMeshAssetOrig, pMesh.pPosition[vertexIndex], submeshIndex, graphicalVertexIndex, NULL, true))
				{
					NxRenderDataFormat::Enum normalFormat;
					const NxVertexBuffer& vb = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer();
					const NxVertexFormat& vf = vb.getFormat();
					PxU32 bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(NxRenderVertexSemantic::NORMAL));
					const PxVec3* graphicalNormals = (const PxVec3*)vb.getBufferAndFormat(normalFormat, bufferIndex);
					if (normalFormat != NxRenderDataFormat::FLOAT3)
					{
						// Phil - you might need to handle other normal formats
						graphicalNormals = NULL;
					}
					if (graphicalNormals != NULL)
					{
						physicalNormal = graphicalNormals[graphicalVertexIndex];
					}
				}
				else
				{
					physicalNormal = PxVec3(0.0f, 1.0f, 0.0f);
				}
			}

			physx::PxF32* boneWeights = (pMesh.pBoneWeights != NULL) ? pMesh.pBoneWeights + (vertexIndex * pMesh.numBonesPerVertex) : NULL;
			if (boneWeights != NULL && hasBoneWeights)
			{
				// is first weight already 0.0 ?
				if (boneWeights[0] <= 0.0f)
				{
					count[4]++;
					physx::PxU32 submeshIndex = 0, graphicalVertexIndex = 0;
					if (getClosestVertex(renderMeshAssetOrig, pMesh.pPosition[vertexIndex], submeshIndex, graphicalVertexIndex, NULL, ignoreUnusedVertices))
					{
						const NxVertexBuffer& vb = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer();
						const NxVertexFormat& vf = vb.getFormat();
						physx::PxU16* boneIndices = (pMesh.pBoneIndices != NULL) ? pMesh.pBoneIndices + (vertexIndex * pMesh.numBonesPerVertex) : NULL;
						if (boneIndices != NULL)
						{
							for (physx::PxU32 i = 0; i < pMesh.numBonesPerVertex; i++)
							{
								boneIndices[i] = 0;
								boneWeights[i] = 0.0f;
							}

							physx::NxRenderDataFormat::Enum boneIndexFormat;
							physx::PxU32 bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::NxRenderVertexSemantic::BONE_INDEX));
							const physx::PxU16* graphicalBoneIndices = (const physx::PxU16*)vb.getBufferAndFormat(boneIndexFormat, bufferIndex);
							if (boneIndexFormat != physx::NxRenderDataFormat::USHORT1 && boneIndexFormat != physx::NxRenderDataFormat::USHORT2 &&
							        boneIndexFormat != physx::NxRenderDataFormat::USHORT3 && boneIndexFormat != physx::NxRenderDataFormat::USHORT4)
							{
								// Phil - you might need to handle other normal formats
								graphicalBoneIndices = NULL;
							}

							physx::NxRenderDataFormat::Enum boneWeightFormat;
							bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::NxRenderVertexSemantic::BONE_WEIGHT));
							const physx::PxF32* graphicalBoneWeights = (const physx::PxF32*)vb.getBufferAndFormat(boneWeightFormat, bufferIndex);
							if (boneWeightFormat != physx::NxRenderDataFormat::FLOAT1 && boneWeightFormat != physx::NxRenderDataFormat::FLOAT2 &&
							        boneWeightFormat != physx::NxRenderDataFormat::FLOAT3 && boneWeightFormat != physx::NxRenderDataFormat::FLOAT4)
							{
								// Phil - you might need to handle other normal formats
								graphicalBoneWeights = NULL;
							}

							const physx::PxU32 graphicalNumBonesPerVertex = vertexSemanticFormatElementCount(NxRenderVertexSemantic::BONE_INDEX, boneIndexFormat);
							for (physx::PxU32 i = 0; i < graphicalNumBonesPerVertex; i++)
							{
								const physx::PxF32 weight = graphicalBoneWeights[graphicalVertexIndex * graphicalNumBonesPerVertex + i];
								const physx::PxU16 index = graphicalBoneIndices[graphicalVertexIndex * graphicalNumBonesPerVertex + i];
								tempMesh->addBoneToVertex(vertexIndex, index, weight);
							}
						}
					}
				}
				tempMesh->normalizeBonesOfVertex(vertexIndex);
			}
		}

		if (mDeriveNormalsFromBones && pMesh.numBonesPerVertex > 0)
		{
			for (physx::PxU32 vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
			{
				const physx::PxVec3& pos = pMesh.pPosition[vertexIndex];
				physx::PxVec3& normal = pMesh.pNormal[vertexIndex];
				normal = physx::PxVec3(0.0f);

				for (physx::PxU32 j = 0; j < pMesh.numBonesPerVertex; j++)
				{
					const physx::PxF32 boneWeight = pMesh.pBoneWeights[vertexIndex * pMesh.numBonesPerVertex + j];

					if (boneWeight == 0.0f)
					{
						continue;
					}

					const physx::PxU16 externalBoneIndex = pMesh.pBoneIndices[vertexIndex * pMesh.numBonesPerVertex + j];

					const physx::PxI32 internalBoneIndex = getBoneInternalIndex(externalBoneIndex);

					const ClothingAssetParametersNS::BoneEntry_Type& bone = mBones[internalBoneIndex];
					physx::PxVec3 closest  = bone.bindPose.t;
					physx::PxF32  minDist2 = (closest - pos).magnitudeSquared();

					// find closest point on outgoing bones
					for (physx::PxU32 k = 0; k < mBones.size(); k++)
					{
						if (mBones[k].parentIndex != internalBoneIndex)
						{
							continue;
						}

						// closest point on segment
						const physx::PxVec3& a = bone.bindPose.t;
						const physx::PxVec3& b = mBones[k].bindPose.t;
						if (a == b)
						{
							continue;
						}

						const physx::PxVec3 d = b - a;
						physx::PxF32 s = (pos - a).dot(d) / d.magnitudeSquared();
						s = physx::PxClamp(s, 0.0f, 1.0f);

						physx::PxVec3 proj = a + d * s;
						physx::PxF32 dist2 = (proj - pos).magnitudeSquared();

						if (dist2 < minDist2)
						{
							minDist2 = dist2;
							closest = proj;
						}
					}

					physx::PxVec3 n = pos - closest;
					n.normalize();
					normal += n * boneWeight;
				}
				normal.normalize();
			}

			tempMesh->smoothNormals(3);
		}

		progress.completeSubtask();

		tempMesh->release();
		tempMesh = NULL;
	}
}



bool ClothingAssetAuthoring::hasTangents(const NiApexRenderMeshAsset& rma)
{
	bool bHasTangents = false;
	for (PxU32 submeshIndex = 0; submeshIndex < rma.getSubmeshCount(); submeshIndex++)
	{
		const NxVertexBuffer& vb = rma.getSubmesh(submeshIndex).getVertexBuffer();
		const NxVertexFormat& vf = vb.getFormat();
		PxU32 bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(physx::apex::NxRenderVertexSemantic::TANGENT));
		NxRenderDataFormat::Enum outFormat;
		const void* tangents = vb.getBufferAndFormat(outFormat, bufferIndex);
		if (tangents != NULL)
		{
			bHasTangents = true;
			break;
		}
	}

	return bHasTangents;
}



PxU32 ClothingAssetAuthoring::getMaxNumGraphicalVertsActive(const ClothingGraphicalLodParameters& graphicalLod, PxU32 submeshIndex)
{
	PxU32 numVerts = 0;

	const PxU32 numParts = graphicalLod.physicsSubmeshPartitioning.arraySizes[0];
	ClothingGraphicalLodParametersNS::PhysicsSubmeshPartitioning_Type* parts = graphicalLod.physicsSubmeshPartitioning.buf;

	for (PxU32 i = 0; i < numParts; ++i)
	{
		if (parts[i].graphicalSubmesh == submeshIndex)
		{
			numVerts = PxMax(numVerts, parts[i].numSimulatedVertices);
		}
	}

	return numVerts;
}



bool ClothingAssetAuthoring::isMostlyImmediateSkinned(const NiApexRenderMeshAsset& rma, const ClothingGraphicalLodParameters& graphicalLod)
{
	PxU32 immediateMapSize = graphicalLod.immediateClothMap.arraySizes[0];
	if (immediateMapSize == 0)
		return false;

	// figure out the number of immediate skinned verts.. more complicated than i thought.
	PxU32 numImmediateSkinnedVerts = 0;
	PxU32 totalSkinnedVerts = 0;
	PxU32* immediateMap = graphicalLod.immediateClothMap.buf;
	physx::PxU32 numSubmeshes = rma.getSubmeshCount();
	physx::PxU32 vertexOffset = 0;
	for (physx::PxU32 submeshIndex = 0; submeshIndex < numSubmeshes; ++submeshIndex)
	{
		const physx::apex::NxRenderSubmesh& submesh = rma.getSubmesh(submeshIndex);
		physx::PxU32 numVertsToSkin = getMaxNumGraphicalVertsActive(graphicalLod, submeshIndex);
		for (physx::PxU32 vertexIndex = 0; vertexIndex < numVertsToSkin; ++vertexIndex)
		{
			physx::PxU32 mapId = vertexOffset + vertexIndex;

			PX_ASSERT(mapId < immediateMapSize);
			const PxU32 mapEntry = immediateMap[mapId];
			const PxU32 flags = mapEntry & ~ClothingConstants::ImmediateClothingReadMask;

			// count the number of mesh-mesh skinned verts (the others are skinned to bones)
			++totalSkinnedVerts;
			if ((flags & ClothingConstants::ImmediateClothingInSkinFlag) == 0)
			{
				++numImmediateSkinnedVerts;
			}

		}
		vertexOffset += submesh.getVertexCount(0);
	}

	return 2*numImmediateSkinnedVerts > totalSkinnedVerts;
}



bool ClothingAssetAuthoring::conditionalMergeMapping(const NiApexRenderMeshAsset& rma, ClothingGraphicalLodParameters& graphicalLod)
{
	bool merged = false;
	// it's faster to do mesh-mesh skinning on all verts, instead of
	// mesh-mesh skinning + immediateSkinning + tangent recompute
	if (hasTangents(rma) && !isMostlyImmediateSkinned(rma, graphicalLod))
	{
		merged = mergeMapping(&graphicalLod);
	}

	return merged;
}



class SkinClothMapPredicate
{
public:
	bool operator()(const SkinClothMapB& map1, const SkinClothMapB& map2) const
	{
		if (map1.submeshIndex < map2.submeshIndex)
		{
			return true;
		}
		else if (map1.submeshIndex > map2.submeshIndex)
		{
			return false;
		}

		return map1.faceIndex0 < map2.faceIndex0;
	}
};



void ClothingAssetAuthoring::sortSkinMapB(SkinClothMapB* skinClothMap, physx::PxU32 skinClothMapSize, physx::PxU32* immediateClothMap, physx::PxU32 immediateClothMapSize)
{

	physx::sort<SkinClothMapB, SkinClothMapPredicate>(skinClothMap, skinClothMapSize, SkinClothMapPredicate());

	if (immediateClothMap != NULL)
	{
		for (physx::PxU32 j = 0; j < skinClothMapSize; j++)
		{
			const physx::PxU32 vertexIndex = skinClothMap[j].vertexIndexPlusOffset;
			if (vertexIndex < immediateClothMapSize)
			{
				PX_ASSERT((immediateClothMap[vertexIndex] & ClothingConstants::ImmediateClothingInSkinFlag) != 0);
				immediateClothMap[vertexIndex] = j | ClothingConstants::ImmediateClothingInSkinFlag;
			}
		}
	}
}



class F32Greater
{
public:
	PX_INLINE bool operator()(PxF32 v1, PxF32 v2) const
	{
		return v1 > v2;
	}
};



void ClothingAssetAuthoring::setupPhysicalLods(ClothingPhysicalMeshParameters& physicalMesh, PxU32 numMaxDistReductions, PxF32* maxDistReductions) const
{
	if (numMaxDistReductions > 1)
	{
		shdfnd::sort(maxDistReductions, numMaxDistReductions, F32Greater());
	}

	//ClothingPhysicalMesh& physicalMesh = *physicalMeshData.physicalMesh;
	//PxU32* indices = physicalMesh.physicalMesh.indices.buf;

	const PxU32 numIndicesPerElement = (physicalMesh.physicalMesh.isTetrahedralMesh) ? 4 : 3;

	NxParamArray<ClothingPhysicalMeshParametersNS::PhysicalSubmesh_Type> submeshes(&physicalMesh, "submeshes", reinterpret_cast<NxParamDynamicArrayStruct*>(&physicalMesh.submeshes));
	submeshes.clear();

	const PxF32 maxMaxDistance = getMaxMaxDistance(physicalMesh.physicalMesh, 0, numIndicesPerElement);
	if (maxMaxDistance == 0.0f)
	{
		APEX_DEBUG_WARNING("All maxDistances of the mesh are 0.");
	}
	else
	{
		// discard all max distances that are too high
		for (PxU32 i = 0; i < numMaxDistReductions; i++)
		{
			if (maxDistReductions[i] > maxMaxDistance)
			{
				numMaxDistReductions = i;
				break;
			}
		}

		// index buffer is sorted such that each triangle (or tetrahedra) has a higher or equal max distance than all tuples right of it.
		PxU32 maxDistReductionsId = 0;
		for (PxU32 i = 0; i < physicalMesh.physicalMesh.numIndices; i += numIndicesPerElement)
		{
			PxF32 triangleMaxDistance = getMaxMaxDistance(physicalMesh.physicalMesh, i, numIndicesPerElement);
			if (triangleMaxDistance == 0.0f								// don't simulate triangles that may not move
			        ||	i == physicalMesh.physicalMesh.numIndices - numIndicesPerElement // all vertices are painted, i.e. this is the last tuple.
			        ||	(maxDistReductionsId < numMaxDistReductions && triangleMaxDistance <= maxDistReductions[maxDistReductionsId]))  // this is a border
			{
				const PxU32 maxIndex = (i == physicalMesh.physicalMesh.numIndices - numIndicesPerElement) ? i + numIndicesPerElement : i;
				// make new mesh
				const PxU32 submeshId = submeshes.size();
				if (submeshId == 0 || i > submeshes[submeshId - 1].numIndices)
				{
					//const PxU32 start = (submeshId == 0) ? 0 : submeshes.back().numIndices;
					submeshes.pushBack();

					submeshes[submeshId].numIndices = maxIndex;

					// these values get set in reorderDeformableVertices
					submeshes[submeshId].numVertices = 0;
					submeshes[submeshId].numMaxDistance0Vertices = 0;

					maxDistReductionsId++;
				}

				if (triangleMaxDistance == 0.0f)
				{
					break;
				}
			}
		}
	}

	const PxU32 numSubmeshes = submeshes.size();

	NxParamArray<ClothingPhysicalMeshParametersNS::PhysicalLod_Type> physicalLods(&physicalMesh, "physicalLods", reinterpret_cast<NxParamDynamicArrayStruct*>(&physicalMesh.physicalLods));
	physicalLods.clear();

	{
		PhysicalLod& lod = physicalLods.pushBack();
		lod.costWithoutIterations = 0;
		lod.maxDistanceReduction = physicalMesh.physicalMesh.maximumMaxDistance;
		lod.submeshId = (PxU32) - 1;
		lod.solverIterationScale = 0.0f;
	}

	for (PxU32 submeshId = 0; submeshId < numSubmeshes; submeshId++)
	{
		PhysicalLod& lod = physicalLods.pushBack();
		lod.costWithoutIterations = 0;
		lod.submeshId = submeshId;
		lod.solverIterationScale = 0.0f;
		lod.maxDistanceReduction = (submeshId == numSubmeshes - 1) ? 0.0f : maxDistReductions[submeshId];
	}
}



void ClothingAssetAuthoring::distributeSolverIterations()
{
	for (PxU32 meshId = 0; meshId < mPhysicalMeshes.size(); meshId++)
	{
		ClothingPhysicalMeshParameters& mesh = *mPhysicalMeshes[meshId];
		const PxF32 solverIterationsPerLod = mesh.physicalLods.arraySizes[0] > 1 ? 1.0f / (PxF32)(mesh.physicalLods.arraySizes[0] - 1) : 0.0f;
		for (PxI32 i = 0; i < mesh.physicalLods.arraySizes[0]; i++)
		{
			mesh.physicalLods.buf[i].solverIterationScale = solverIterationsPerLod * i;
			mesh.physicalLods.buf[i].costWithoutIterations = i == 0 ? 0 : mesh.submeshes.buf[mesh.physicalLods.buf[i].submeshId].numVertices;
		}
	}
}



void ClothingAssetAuthoring::sortDeformableIndices(ClothingPhysicalMesh& physicalMesh)
{
	if (physicalMesh.getNumVertices() == 0 || physicalMesh.getConstrainCoefficientsBuffer() == NULL)
	{
		return;
	}

	physx::PxU32* deformableIndices = physicalMesh.getIndicesBuffer();
	ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* constrainCoeffs = physicalMesh.getConstrainCoefficientsBuffer();

	physx::Array<physx::PxU32> deformableIndicesPermutation;
	deformableIndicesPermutation.resize(physicalMesh.getNumIndices());

	physx::PxU32 numIndices = physicalMesh.isTetrahedralMesh() ? 4 : 3;

	for (physx::PxU32 i = 0; i < physicalMesh.getNumIndices(); i++)
	{
		deformableIndicesPermutation[i] = i;
	}

	if (numIndices == 3)
	{
		TriangleGreater_3 triangleGreater(deformableIndices, constrainCoeffs);
		shdfnd::sort((PxU32_3*)&deformableIndicesPermutation[0], physicalMesh.getNumIndices() / numIndices, triangleGreater);
	}
	else if (numIndices == 4)
	{
		TriangleGreater_4 triangleGreater(deformableIndices, constrainCoeffs);
		shdfnd::sort((PxU32_4*)&deformableIndicesPermutation[0], physicalMesh.getNumIndices() / numIndices, triangleGreater);
	}
	else
	{
		PX_ALWAYS_ASSERT();
	}


	// inverse permutation
	physx::Array<physx::PxI32> invPerm(physicalMesh.getNumIndices());
	for (physx::PxU32 i = 0; i < physicalMesh.getNumIndices(); i++)
	{
		PX_ASSERT(deformableIndicesPermutation[i] < physicalMesh.getNumIndices());
		invPerm[deformableIndicesPermutation[i]] = i;
	}


	// apply permutation
	ApexPermute<physx::PxU32>(deformableIndices, &deformableIndicesPermutation[0], physicalMesh.getNumIndices());

	// update mappings into deformable index buffer
	for (physx::PxU32 i = 0; i < mGraphicalLods.size(); i++)
	{
		if (mPhysicalMeshes[mGraphicalLods[i]->physicalMeshId] == physicalMesh.getNxParameterized())
		{
			ClothingGraphicalMeshAssetWrapper meshAsset(reinterpret_cast<NiApexRenderMeshAsset*>(mGraphicalLods[i]->renderMeshAssetPointer));
			ClothingGraphicalLodParameters* graphicalLod = mGraphicalLods[i];

			const physx::PxU32 numGraphicalVertices = meshAsset.getNumTotalVertices();
			if (graphicalLod->tetraMap.arraySizes[0] > 0)
			{
				for (physx::PxU32 j = 0; j < numGraphicalVertices; j++)
				{
					graphicalLod->tetraMap.buf[j].tetraIndex0 = invPerm[graphicalLod->tetraMap.buf[j].tetraIndex0];
				}
			}
			const physx::PxU32 skinClothMapBSize = graphicalLod->skinClothMapB.arraySizes[0];
			if (skinClothMapBSize > 0)
			{
				for (physx::PxU32 j = 0; j < skinClothMapBSize; j++)
				{
					graphicalLod->skinClothMapB.buf[j].faceIndex0 = invPerm[graphicalLod->skinClothMapB.buf[j].faceIndex0];
				}

				sortSkinMapB(graphicalLod->skinClothMapB.buf, graphicalLod->skinClothMapB.arraySizes[0], graphicalLod->immediateClothMap.buf, numGraphicalVertices);
			}
		}
	}

	physicalMesh.updateMaxMaxDistance();
}



bool ClothingAssetAuthoring::getGraphicalLodIndex(physx::PxU32 lod, physx::PxU32& graphicalLodIndex)
{
	graphicalLodIndex = PX_MAX_U32;
	for (physx::PxU32 i = 0; i < mGraphicalLods.size(); i++)
	{
		if (mGraphicalLods[i]->lod == lod)
		{
			graphicalLodIndex = i;
			return true;
		}
	}
	return false;
}



PxU32 ClothingAssetAuthoring::addGraphicalLod(PxU32 lod)
{
	PxU32 lodIndex = (PxU32) - 1;
	if (getGraphicalLodIndex(lod, lodIndex))
	{
		return lodIndex;
	}

	ClothingGraphicalLodParameters* newLod = DYNAMIC_CAST(ClothingGraphicalLodParameters*)(NiGetApexSDK()->getParameterizedTraits()->createNxParameterized(ClothingGraphicalLodParameters::staticClassName()));
	newLod->lod = lod;
	newLod->renderMeshAssetPointer = NULL;
	PX_ASSERT(newLod->physicalMeshId == (PxU32) - 1);

	mGraphicalLods.pushBack(NULL);

	// insertion sort
	PxI32 current = (PxI32)mGraphicalLods.size() - 1;
	while (current > 0 && mGraphicalLods[current - 1]->lod > newLod->lod)
	{
		mGraphicalLods[current] = mGraphicalLods[current - 1];
		current--;
	}
	PX_ASSERT(current >= 0);
	PX_ASSERT((PxU32)current < mGraphicalLods.size());
	mGraphicalLods[current] = newLod;

	return current;
}



void ClothingAssetAuthoring::clearCooked()
{
	NxParamArray<ClothingAssetParametersNS::CookedEntry_Type> cookedEntries(mParams, "cookedData", reinterpret_cast<NxParamDynamicArrayStruct*>(&mParams->cookedData));

	if (cookedEntries.size() > 0)
	{
		if(cookedEntries[0].cookedData)
		{
			mPreviousCookedType = cookedEntries[0].cookedData->className();
#if NX_SDK_VERSION_MAJOR == 2
			if (strcmp(mPreviousCookedType, ClothingCookedParam::staticClassName()) == 0)
			{
				if (((ClothingCookedParam*)cookedEntries[0].cookedData)->cookedDataVersion > 300)
				{
					mPreviousCookedType = "Embedded";
				}
			}
#endif
		}
	}

	cookedEntries.clear();
}



bool ClothingAssetAuthoring::addGraphicalMesh(NxRenderMeshAssetAuthoring* renderMesh, physx::PxU32 graphicalLodIndex)
{
	if (mGraphicalLods[graphicalLodIndex]->renderMeshAssetPointer != NULL)
	{
		reinterpret_cast<NiApexRenderMeshAsset*>(mGraphicalLods[graphicalLodIndex]->renderMeshAssetPointer)->release();
		mGraphicalLods[graphicalLodIndex]->renderMeshAssetPointer = NULL;
	}
	if (mGraphicalLods[graphicalLodIndex]->renderMeshAsset != NULL)
	{
		// PH: did the param object already get destroyed in the release call above?
		mGraphicalLods[graphicalLodIndex]->renderMeshAsset->destroy();
		mGraphicalLods[graphicalLodIndex]->renderMeshAsset = NULL;
	}

	if (renderMesh != NULL)
	{
		const physx::PxU32 additionalSize = 7;
		char buf[16];
		int bufSize = 16;
		char* rmaName = buf;
		if (strlen(getName()) + additionalSize > 15)
		{
			bufSize = (physx::PxU32)(strlen(getName()) + additionalSize + 2);
			rmaName = (char*)PX_ALLOC(bufSize, PX_DEBUG_EXP("ClothingAssetAuthoring::addGraphicalMesh"));
		}
		physx::string::sprintf_s(rmaName, bufSize, "%s_RMA%.3d", getName(), mGraphicalLods[graphicalLodIndex]->lod);

		PX_ASSERT(mGraphicalLods[graphicalLodIndex]->renderMeshAssetPointer == NULL);
		//mGraphicalMeshesRuntime[graphicalLodIndex] = DYNAMIC_CAST(NiApexRenderMeshAsset*)(NxGetApexSDK()->createAsset(*renderMesh, rmaName));
		NxParameterized::Interface* newAsset = NiGetApexSDK()->getParameterizedTraits()->createNxParameterized(renderMesh->getNxParameterized()->className());
		newAsset->copy(*renderMesh->getNxParameterized());

		NiApexRenderMeshAsset* rma = static_cast<NiApexRenderMeshAsset*>(NxGetApexSDK()->createAsset(newAsset, rmaName));
		PX_ASSERT(rma->getAssetNxParameterized()->equals(*renderMesh->getNxParameterized()));
		if (rma->mergeBinormalsIntoTangents())
		{
			APEX_DEBUG_INFO("The ApexRenderMesh has Tangent and Binormal semantic (both FLOAT3), but clothing needs only Tangent (with FLOAT4). Converted internally");
		}
		mGraphicalLods[graphicalLodIndex]->renderMeshAssetPointer = rma;

		PX_ASSERT(strcmp(newAsset->className(), "RenderMeshAssetParameters") == 0);
		mGraphicalLods[graphicalLodIndex]->renderMeshAsset = newAsset;

		// make sure the isReferenced value is set!
		NxParameterized::Handle handle(*newAsset);
		newAsset->getParameterHandle("isReferenced", handle);
		PX_ASSERT(handle.isValid());
		if (handle.isValid())
		{
			bool val;
			handle.getParamBool(val);
			PX_ASSERT(!val);
			handle.setParamBool(true);
		}

		if (rmaName != buf)
		{
			PX_FREE(rmaName);
			rmaName = buf;
		}

		return true;
	}
	else
	{
		PX_ASSERT(mGraphicalLods[graphicalLodIndex] != NULL);

		// store the pointer to the element that is removed
		PX_ASSERT(mGraphicalLods[graphicalLodIndex]->renderMeshAssetPointer == NULL);
		mGraphicalLods[graphicalLodIndex]->destroy();

		for (physx::PxU32 i = graphicalLodIndex; i < mGraphicalLods.size() - 1; i++)
		{
			mGraphicalLods[i] = mGraphicalLods[i + 1];
		}

		mGraphicalLods.back() = NULL; // set last element to NULL, otherwise the referred object gets destroyed in popBack
		mGraphicalLods.popBack();

		return false;
	}
}



void ClothingAssetAuthoring::initParams()
{
	PX_ASSERT(mParams != NULL);
	if (mParams != NULL)
	{
		mParams->setSerializationCallback(this, NULL);
	}

	if (mParams->materialLibrary == NULL)
	{
		mOwnsMaterialLibrary = true;
		mParams->materialLibrary = NiGetApexSDK()->getParameterizedTraits()->createNxParameterized(ClothingMaterialLibraryParameters::staticClassName());
	}
}



bool ClothingAssetAuthoring::generateImmediateClothMap(const NxAbstractMeshDescription* targetMeshes, PxU32 numTargetMeshes,
        ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh, PxU32* masterFlags,
        PxF32 epsilon, PxU32& numNotFoundVertices, PxF32 normalResemblance, NxParamArray<PxU32>& result,
        IProgressListener* progress) const
{
	// square because distance is also squared.
	epsilon = epsilon * epsilon;

	bool haveAllBuffers = true;

	PxU32 numGraphicalVertices = 0;
	for (PxU32 i = 0; i < numTargetMeshes; i++)
	{
		numGraphicalVertices += targetMeshes[i].numVertices;
		bool hasAllBuffers = true;
		hasAllBuffers &= targetMeshes[i].pPosition != NULL;
		hasAllBuffers &= targetMeshes[i].pNormal != NULL;

		haveAllBuffers &= hasAllBuffers;

		if (!hasAllBuffers)
		{
			APEX_INTERNAL_ERROR("Render mesh asset does not have either position or normal for submesh %d!", i);
		}
	}

	if (!haveAllBuffers)
	{
		numNotFoundVertices = 0;
		return NULL;
	}

	result.resize(numGraphicalVertices);
	for (PxU32 i = 0; i < numGraphicalVertices; i++)
	{
		result[i] = ClothingConstants::ImmediateClothingInvalidValue;
	}

	numNotFoundVertices = 0;
	PxF32 notFoundError = 0.0f;
	PxF32 maxDotError = 0.0f;
	const PxF32 maxDotMinimum = PxClamp(PxCos(physx::degToRad(normalResemblance)), 0.0f, 1.0f);

	const PxVec3* physicalPositions = physicalMesh.vertices.buf;
	const PxVec3* physicalNormals = physicalMesh.skinningNormals.buf;
	const PxU32* physicsMasterFlags = masterFlags;
	const PxU32 numPhysicsVertices = physicalMesh.numVertices;
	PX_ASSERT(physicsMasterFlags != NULL);

	PxU32 submeshVertexOffset = 0;
	for (PxU32 submeshIndex = 0; submeshIndex < numTargetMeshes; submeshIndex++)
	{
		const PxVec3* positions = targetMeshes[submeshIndex].pPosition;
		const PxVec3* normals = targetMeshes[submeshIndex].pNormal;
		const PxU32* slaveFlags = targetMeshes[submeshIndex].pVertexFlags;
		const PxU32 numVertices = targetMeshes[submeshIndex].numVertices;

		for (PxU32 index = 0; index < numVertices; index++)
		{
			if (progress != NULL && ((index & 0xff) == 0))
			{
				const PxU32 percent = 100 * (index + submeshVertexOffset) / numGraphicalVertices;

				progress->setProgress(percent);
			}

			PxF32 minDistanceSquared = FLT_MAX;
			PxI32 optimalMatch = -1;
			PxF32 maxDot = 0.0f;
			const PxU32 slave = slaveFlags != NULL ? slaveFlags[index] : 0xffffffff;

			for (PxU32 vertexIndex = 0; vertexIndex < numPhysicsVertices && (minDistanceSquared > 0 || maxDot < maxDotMinimum); vertexIndex++)
			{
				const PxU32 master = physicsMasterFlags[vertexIndex];
				if ((master & slave) == 0)
				{
					continue;
				}

				const PxF32 distSquared = (physicalPositions[vertexIndex] - positions[index]).magnitudeSquared();
				const PxF32 dot = normals[index].dot(physicalNormals[vertexIndex]);
				if (distSquared < minDistanceSquared || (distSquared == minDistanceSquared && abs(dot) > abs(maxDot)))
				{
					minDistanceSquared = distSquared;
					optimalMatch = vertexIndex;
					maxDot = dot;
				}
			}

			if (optimalMatch == -1 || minDistanceSquared > epsilon || abs(maxDot) < maxDotMinimum)
			{
				notFoundError += sqrt(minDistanceSquared);
				maxDotError += abs(maxDot);

				if (abs(maxDot) < maxDotMinimum && minDistanceSquared <= epsilon)
				{
					result[index + submeshVertexOffset] = optimalMatch;
					result[index + submeshVertexOffset] |= ClothingConstants::ImmediateClothingBadNormal;
				}
				else
				{
					result[index + submeshVertexOffset] = ClothingConstants::ImmediateClothingInvalidValue;
				}
				numNotFoundVertices++;
			}
			else
			{
				result[index + submeshVertexOffset] = optimalMatch;
				if (maxDot < 0)
				{
					result[index + submeshVertexOffset] |= ClothingConstants::ImmediateClothingInvertNormal;
				}
			}
		}

		submeshVertexOffset += numVertices;
	}

	return result.size() == numGraphicalVertices;
}



bool ClothingAssetAuthoring::generateSkinClothMap(const NxAbstractMeshDescription* targetMeshes, PxU32 numTargetMeshes,
        ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh, PxU32* masterFlags, PxU32* immediateMap,
        PxU32 numEmptyInImmediateMap, NxParamArray<ClothingGraphicalLodParametersNS::SkinClothMapD_Type>& result, 
		PxF32& offsetAlongNormal, bool integrateImmediateMap, IProgressListener* progress) const
{
	if (immediateMap == NULL || integrateImmediateMap)
	{
		physx::PxU32 sum = 0;
		for (physx::PxU32 i = 0; i < numTargetMeshes; i++)
		{
			sum += targetMeshes[i].numVertices;
		}
		result.resize(sum);
	}
	else
	{
		result.resize(numEmptyInImmediateMap);
	}

	// figure out some details about the physical mesh
	NxAbstractMeshDescription srcPM;
	srcPM.numIndices = physicalMesh.numIndices;
	srcPM.numVertices = physicalMesh.numVertices;
	srcPM.pIndices = physicalMesh.indices.buf;
	srcPM.pNormal = physicalMesh.skinningNormals.buf;
	srcPM.pPosition = physicalMesh.vertices.buf;
	srcPM.pVertexFlags = masterFlags;

	srcPM.UpdateDerivedInformation(NULL);

	// PH: Negating this leads to interesting effects, but also to some side effects...
	offsetAlongNormal = DEFAULT_PM_OFFSET_ALONG_NORMAL_FACTOR * srcPM.avgEdgeLength;

	const physx::PxU32 physNumIndices = physicalMesh.numIndices;

	// compute mapping
	physx::Array<TriangleWithNormals> triangles;
	triangles.reserve(physNumIndices / 3);


	// create a list of physics mesh triangles
	physx::PxF32 avgHalfDiagonal = 0.0f;
	for (physx::PxU32 i = 0; i < physNumIndices ; i += 3)
	{
		TriangleWithNormals triangle;

		// store vertex information in triangle
		triangle.master = 0;
		for (physx::PxU32 j = 0; j < 3; j++)
		{
			triangle.vertices[j] = srcPM.pPosition[srcPM.pIndices[i + j]];
			triangle.normals[j] = srcPM.pNormal[srcPM.pIndices[i + j]];
			triangle.master |= srcPM.pVertexFlags != NULL ? srcPM.pVertexFlags[srcPM.pIndices[i + j]] : 0xffffffff;
		}
		triangle.faceIndex0 = i;

		triangle.init();
		triangle.bounds.fattenFast(srcPM.avgEdgeLength);

		physx::PxVec3 boundsDiag = triangle.bounds.getExtents();
		avgHalfDiagonal += boundsDiag.magnitude();
		triangles.pushBack(triangle);
	}
	avgHalfDiagonal /= triangles.size();

	// hash the triangles
	ApexMeshHash hash;
	hash.setGridSpacing(avgHalfDiagonal);
	for (physx::PxU32 i = 0; i < triangles.size(); i++)
	{
		hash.add(triangles[i].bounds, i);
	}

	// find the best triangle for each graphical vertex
	SkinClothMap* mapEntry = result.begin();

	physx::Array<physx::PxU32> queryResult;

	physx::PxU32 targetOffset = 0;
	for (physx::PxU32 targetIndex = 0; targetIndex < numTargetMeshes; targetIndex++)
	{
		const physx::PxU32 numVertices = targetMeshes[targetIndex].numVertices;
		for (physx::PxU32 vertexIndex = 0;  vertexIndex < numVertices; vertexIndex++)
		{
			const PxU32 slave = targetMeshes[targetIndex].pVertexFlags != NULL ? targetMeshes[targetIndex].pVertexFlags[vertexIndex] : 0xffffffff;
			PX_ASSERT(slave != 0);

			const physx::PxU32 index = vertexIndex + targetOffset;
			const physx::PxVec3 position = targetMeshes[targetIndex].pPosition[vertexIndex];
			const physx::PxVec3 normal = targetMeshes[targetIndex].pNormal[vertexIndex];
			const physx::PxVec3 normalRelative = position + (normal * offsetAlongNormal);
			physx::PxVec3 tangentRelative(0.0f);
			if (targetMeshes[targetIndex].pTangent != NULL)
			{
				tangentRelative = position + (targetMeshes[targetIndex].pTangent[vertexIndex] * offsetAlongNormal);
			}
			else if (targetMeshes[targetIndex].pTangent4 != NULL)
			{
				const physx::PxVec3 tangent(
					targetMeshes[targetIndex].pTangent4[vertexIndex].x,
					targetMeshes[targetIndex].pTangent4[vertexIndex].y,
					targetMeshes[targetIndex].pTangent4[vertexIndex].z);
				tangentRelative = position + (tangent * offsetAlongNormal);
			}

			
			if (((immediateMap != NULL) && ((immediateMap[index] & ClothingConstants::ImmediateClothingBadNormal) != 0)) || integrateImmediateMap)
			{
				// read physics vertex
				const physx::PxU32 bestVertex = immediateMap[index] & ClothingConstants::ImmediateClothingReadMask;

				// mark entry as invalid, because we put it into the skinClothMap
				immediateMap[index] = ClothingConstants::ImmediateClothingInvalidValue;

				physx::PxF32 bestError = PX_MAX_F32;
				physx::PxI32 bestIndex = -1;

				for (physx::PxU32 pIndex = 0; pIndex < physNumIndices; pIndex++)
				{
					if (srcPM.pIndices[pIndex] == bestVertex)
					{
						// this is a triangle that contains bestVertex (from the immediate map)

						physx::PxU32 faceIndex0 = pIndex - (pIndex % 3);
						physx::PxU32 triangleIndex = faceIndex0 / 3;
						PX_ASSERT(triangleIndex < triangles.size());
						TriangleWithNormals& triangle = triangles[triangleIndex];
						PX_ASSERT(triangle.faceIndex0 == faceIndex0);

						if (triangle.doNotUse)
						{
							continue;
						}

						if ((triangle.master & slave) == 0)
						{
							continue;
						}

						ModuleClothingHelpers::computeTriangleBarys(triangle, position, normalRelative, tangentRelative, offsetAlongNormal, vertexIndex + targetOffset, false);

						if (triangle.valid != 2)
						{
							continue;
						}

						physx::PxF32 error = computeTriangleError(triangle, normal);

						// use the best triangle that contains the vertex
						if (error < bestError)
						{
							bestIndex = triangleIndex;
							bestError = error;
						}
					}
				}
				//PX_ASSERT(bestIndex != -1);
				if (bestIndex != -1)
				{
					immediateMap[index] = (physx::PxU32)(mapEntry - result.begin());
					immediateMap[index] |= ClothingConstants::ImmediateClothingInSkinFlag;

					mapEntry->vertexBary = triangles[bestIndex].tempBaryVertex;
					PxU32 faceIndex = triangles[bestIndex].faceIndex0;
					mapEntry->vertexIndex0 = srcPM.pIndices[faceIndex + 0];
					mapEntry->vertexIndex1 = srcPM.pIndices[faceIndex + 1];
					mapEntry->vertexIndex2 = srcPM.pIndices[faceIndex + 2];
					mapEntry->normalBary = triangles[bestIndex].tempBaryNormal;
					mapEntry->tangentBary = triangles[bestIndex].tempBaryTangent;
					mapEntry->vertexIndexPlusOffset = index;
					mapEntry++;

					continue;
				}
				//PX_ASSERT(0 && "generateSkinClothMapC: We should never end up here");
			}

			if (immediateMap != NULL && immediateMap[index] != ClothingConstants::ImmediateClothingInvalidValue)
			{
				continue;
			}

			if (progress != NULL && (index & 0xf) == 0)
			{
				const physx::PxU32 location = (physx::PxU32)(mapEntry - result.begin());
				const physx::PxU32 percent = 100 * location / result.size();

				progress->setProgress(percent);
			}

			physx::PxI32 bestTriangleNr = -1;
			physx::PxF32 bestTriangleError = PX_MAX_F32;

#if 1
			// query for physical triangles around the graphics vertex
			hash.query(position, queryResult);
			for (physx::PxU32 q = 0; q < queryResult.size(); q++)
			{
				const physx::PxU32 triangleNr = queryResult[q];
				TriangleWithNormals& triangle = triangles[triangleNr];

				if (triangle.doNotUse)
				{
					continue;
				}

				if ((triangle.master & slave) == 0)
				{
					continue;
				}

				if (!triangle.bounds.contains(position))
				{
					continue;
				}

				ModuleClothingHelpers::computeTriangleBarys(triangle, position, normalRelative, tangentRelative, offsetAlongNormal, vertexIndex + targetOffset, false);

				if (triangle.valid != 2)
				{
					continue;
				}

				const physx::PxF32 error = computeTriangleError(triangle, normal);

				if (error < bestTriangleError)
				{
					bestTriangleNr = triangleNr;
					bestTriangleError = error;
				}
			}
#endif

			if (bestTriangleNr < 0)
			{
				// nothing was found nearby, search in all triangles
				bestTriangleError = PX_MAX_F32;
				for (physx::PxU32 j = 0; j < triangles.size() && bestTriangleError > 0.0f; j++)
				{
					TriangleWithNormals& triangle = triangles[j];

					if (triangle.doNotUse)
					{
						continue;
					}

					if ((triangle.master & slave) == 0)
					{
						continue;
					}

					triangle.timestamp = -1;
					ModuleClothingHelpers::computeTriangleBarys(triangle, position, normalRelative, tangentRelative, offsetAlongNormal, vertexIndex + targetOffset, false);

					if (triangle.valid == 0)
					{
						continue;
					}

					physx::PxF32 error = computeTriangleError(triangle, normal);

					// increase the error a lot, but still better than nothing
					if (triangle.valid != 2)
					{
						error += 100.0f;
					}

					if (error < bestTriangleError)
					{
						bestTriangleError = error;
						bestTriangleNr = j;
					}
				}
			}

			if (bestTriangleNr >= 0)
			{
				const TriangleWithNormals& bestTriangle = triangles[bestTriangleNr];

				if (immediateMap != NULL)
				{
					PX_ASSERT(immediateMap[index] == ClothingConstants::ImmediateClothingInvalidValue);
					immediateMap[index] = (physx::PxU32)(mapEntry - result.begin());
					immediateMap[index] |= ClothingConstants::ImmediateClothingInSkinFlag;
				}

				PX_ASSERT(bestTriangle.faceIndex0 % 3 == 0);
				//mapEntry->tetraIndex = bestTriangle.tetraIndex;
				//mapEntry->submeshIndex = targetIndex;

				mapEntry->vertexBary = bestTriangle.tempBaryVertex;
				PxU32 faceIndex = bestTriangle.faceIndex0;
				mapEntry->vertexIndex0 = srcPM.pIndices[faceIndex + 0];
				mapEntry->vertexIndex1 = srcPM.pIndices[faceIndex + 1];
				mapEntry->vertexIndex2 = srcPM.pIndices[faceIndex + 2];
				mapEntry->normalBary = bestTriangle.tempBaryNormal;
				mapEntry->tangentBary = bestTriangle.tempBaryTangent;
				mapEntry->vertexIndexPlusOffset = index;
				mapEntry++;
			}
			else if (immediateMap != NULL)
			{
				PX_ASSERT(immediateMap[index] == ClothingConstants::ImmediateClothingInvalidValue);
			}
		}

		targetOffset += numVertices;
	}


	PxU32 sizeused = (PxU32)(mapEntry - result.begin());
	if (sizeused < result.size())
	{
		APEX_DEBUG_WARNING("%d vertices could not be mapped, they will be static!", result.size() - sizeused);
	}
	result.resize(sizeused);

	return true;
}



bool ClothingAssetAuthoring::generateTetraMap(const NxAbstractMeshDescription* targetMeshes, PxU32 numTargetMeshes,
        ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh, PxU32* /*masterFlags*/,
        NxParamArray<ClothingGraphicalLodParametersNS::TetraLink_Type>& result, IProgressListener* progress) const
{
	PxU32 numGraphicalVertices = 0;


	bool haveAllBuffers = true;

	for (PxU32 submeshIndex = 0; submeshIndex < numTargetMeshes; submeshIndex++)
	{
		numGraphicalVertices += targetMeshes[submeshIndex].numVertices;

		bool hasAllBuffers = true;
		hasAllBuffers &= targetMeshes[submeshIndex].pPosition != NULL;
		hasAllBuffers &= targetMeshes[submeshIndex].pNormal != NULL;

		haveAllBuffers &= hasAllBuffers;
		if (!hasAllBuffers)
		{
			APEX_INTERNAL_ERROR("Render mesh asset does not have either position or normal for submesh %d!", submeshIndex);
		}
	}

	if (!haveAllBuffers)
	{
		return NULL;
	}

	result.resize(numGraphicalVertices);
	memset(result.begin(), 0, sizeof(ClothingGraphicalLodParametersNS::TetraLink_Type) * numGraphicalVertices);

	physx::PxU32 submeshVertexOffset = 0;

	const physx::PxU32* physicalIndices = physicalMesh.indices.buf;
	const physx::PxVec3* physicalPositions = physicalMesh.vertices.buf;

	for (physx::PxU32 targetIndex = 0; targetIndex < numTargetMeshes; targetIndex++)
	{
		const physx::PxU32 numVertices = targetMeshes[targetIndex].numVertices;

		const physx::PxVec3* positions = targetMeshes[targetIndex].pPosition;
		const physx::PxVec3* normals = targetMeshes[targetIndex].pNormal;

		for (physx::PxU32 vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
		{
			const physx::PxU32 index = vertexIndex + submeshVertexOffset;
			if (progress != NULL && (index & 0x3f) == 0)
			{
				const physx::PxU32 percent = 100 * index / numGraphicalVertices;
				progress->setProgress(percent);
			}

			const physx::PxVec3 position = positions[vertexIndex];

			physx::PxF32 bestWorstBary = FLT_MAX;
			physx::PxI32 bestTet = -1;
			physx::PxVec3 bestBary(0.0f, 0.0f, 0.0f);
			for (physx::PxU32 j = 0; j < physicalMesh.numIndices; j += 4)
			{
				PxVec3 p[4];
				for (PxU32 k = 0; k < 4; k++)
				{
					p[k] = physicalPositions[physicalIndices[j + k]];
				}

				PxVec3 bary;
				generateBarycentricCoordinatesTet(p[0], p[1], p[2], p[3], position, bary);
				physx::PxF32 baryU = 1 - bary.x - bary.y - bary.z;
				physx::PxF32 worstBary = 0.0f;
				worstBary = PxMax(worstBary, -bary.x);
				worstBary = PxMax(worstBary, -bary.y);
				worstBary = PxMax(worstBary, -bary.z);
				worstBary = PxMax(worstBary, -baryU);
				worstBary = PxMax(worstBary, bary.x - 1);
				worstBary = PxMax(worstBary, bary.y - 1);
				worstBary = PxMax(worstBary, bary.z - 1);
				worstBary = PxMax(worstBary, baryU - 1);
				//PX_ASSERT(worstBary + bestWorstBary > 0 && "they must not be 0 both!!!");
				if (worstBary < bestWorstBary)
				{
					bestWorstBary = worstBary;
					bestTet = j;
					bestBary = bary;
				}
			}

			PX_ASSERT(result[index].tetraIndex0 == 0);

			result[index].vertexBary = bestBary;
			result[index].tetraIndex0 = bestTet;

			// compute barycentric coordinates of normal
			physx::PxVec3 normal(1.0f, 0.0f, 0.0f);
			if (normals != NULL)
			{
				normal = normals[vertexIndex];
				normal.normalize();
			}
			const physx::PxVec3& pa = physicalPositions[physicalIndices[bestTet + 0]];
			const physx::PxVec3& pb = physicalPositions[physicalIndices[bestTet + 1]];
			const physx::PxVec3& pc = physicalPositions[physicalIndices[bestTet + 2]];
			const physx::PxVec3& pd = physicalPositions[physicalIndices[bestTet + 3]];
			physx::PxBounds3 bounds;
			bounds.setEmpty();
			bounds.include(pa);
			bounds.include(pb);
			bounds.include(pc);
			bounds.include(pd);
			// we use a second point above pos, along the normal.
			// The offset must be small but arbitrary since we normalize the resulting normal during skinning
			const physx::PxF32 offset = (bounds.minimum - bounds.maximum).magnitude() * 0.01f;
			generateBarycentricCoordinatesTet(pa, pb, pc, pd, position + normal * offset, result[index].normalBary);

		}


		submeshVertexOffset += numVertices;
	}

	return true;
}



PxF32 ClothingAssetAuthoring::computeBaryError(PxF32 baryX, PxF32 baryY) const
{
#if 0
	const PxF32 triangleSize = 1.0f;
	const PxF32 baryZ = 1.0f - baryX - baryY;

	const PxF32 errorX = (baryX - (1.0f / 3.0f)) * triangleSize;
	const PxF32 errorY = (baryY - (1.0f / 3.0f)) * triangleSize;
	const PxF32 errorZ = (baryZ - (1.0f / 3.0f)) * triangleSize;

	return (errorX * errorX) + (errorY * errorY) + (errorZ * errorZ);
#elif 0
	float dist = 0.0f;
	if (-baryX > dist)
	{
		dist = -baryX;
	}
	if (-baryY > dist)
	{
		dist = -baryY;
	}
	float sum = baryX + baryY - 1.0f;
	if (sum > dist)
	{
		dist = sum;
	}
	return dist * dist;
#else
	const PxF32 baryZ = 1.0f - baryX - baryY;

	const PxF32 errorX = PxMax(PxAbs(baryX - 0.5f) - 0.5f, 0.0f);
	const PxF32 errorY = PxMax(PxAbs(baryY - 0.5f) - 0.5f, 0.0f);
	const PxF32 errorZ = PxMax(PxAbs(baryZ - 0.5f) - 0.5f, 0.0f);

	return (errorX * errorX) + (errorY * errorY) + (errorZ * errorZ);
#endif
}



PxF32 ClothingAssetAuthoring::computeTriangleError(const TriangleWithNormals& triangle, const PxVec3& normal) const
{
	PxVec3 faceNormal = (triangle.vertices[1] - triangle.vertices[0]).cross(triangle.vertices[2] - triangle.vertices[0]);
	faceNormal.normalize();

	const PxF32 avgTriangleEdgeLength = ((triangle.vertices[0] - triangle.vertices[1]).magnitude() +
	                                     (triangle.vertices[0] - triangle.vertices[2]).magnitude() +
	                                     (triangle.vertices[1] - triangle.vertices[2]).magnitude()) / 3.0f;

	PxF32 error = computeBaryError(triangle.tempBaryVertex.x, triangle.tempBaryVertex.y);

	PX_ASSERT(PxAbs(1 - normal.magnitude()) < 0.001f); // make sure it's normalized.

	//const PxF32 normalWeight = faceNormal.cross(normal).magnitude(); // 0 for co-linear, 1 for perpendicular
	const PxF32 normalWeight = 0.5f * (1.0f - faceNormal.dot(normal));

	error += PxClamp(normalWeight, 0.0f, 1.0f) * computeBaryError(triangle.tempBaryNormal.x, triangle.tempBaryNormal.y);

	const PxF32 heightValue = triangle.tempBaryVertex.z / avgTriangleEdgeLength;
	const PxF32 heightWeight = 0.1f + 2.5f * computeBaryError(triangle.tempBaryVertex.x, triangle.tempBaryVertex.y);
	const PxF32 heightError = heightWeight * PxAbs(heightValue);
	error += heightError;

	return error;
}

}
}
} // namespace physx::apex

#endif // WITHOUT_APEX_AUTHORING

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
