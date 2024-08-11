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

#include "NxApex.h"
#include "NiApexScene.h"
#include "NxModifier.h"
#include "NxIofxActor.h"
#include "IofxActorCPU.h"
#include "IofxAsset.h"
#include "IofxScene.h"
#include "IosObjectData.h"

#include "ModuleIofx.h"

#include "ARLSort.h"


namespace physx
{
namespace apex
{
namespace iofx
{

void TaskModifiers::run()
{
	setProfileStat((PxU16) mOwner.mWorkingRange.objectCount);
	mOwner.updateBounds();
	mOwner.runModifiers();
}


#pragma warning(disable: 4355) // 'this' : used in base member initializer list

IofxActorCPU::IofxActorCPU(NxApexAsset* renderAsset, IofxScene* iscene, IofxManager& mgr)
	: IofxActor(renderAsset, iscene, mgr)
	, mModifierTask(*this)
{
}

void IofxActorCPU::updateBounds()
{
	mWorkingBounds.setEmpty();

	const IosObjectCpuData& objData = *static_cast<const IosObjectCpuData*>(mMgr.mWorkingIosData);

	for (PxU32 id = 0; id < mWorkingRange.objectCount; id++)
	{
		PxU32 outputID = mWorkingRange.startIndex + id;

		PxU32 stateID = objData.outputToState[ outputID ];
		PX_ASSERT(stateID != NiIosBufferDesc::NOT_A_PARTICLE);
		PX_ASSERT(stateID < objData.maxStateID);

		PxU32 inputID = objData.pmaInStateToInput->get(stateID);
		PX_ASSERT(inputID != NiIosBufferDesc::NOT_A_PARTICLE);
		inputID &= ~NiIosBufferDesc::NEW_PARTICLE_FLAG;
		PX_ASSERT(inputID < objData.maxInputCount);

		// include particle in renderable bounds
		mWorkingBounds.include(objData.pmaPositionMass->get(inputID).getXYZ());
	}
}

PX_INLINE void MeshInput::load(const IosObjectBaseData& objData, PxU32 pos)
{
	position		= objData.pmaPositionMass->get(pos).getXYZ();
	mass			= objData.pmaPositionMass->get(pos).w;
	velocity		= objData.pmaVelocityLife->get(pos).getXYZ();
	liferemain		= objData.pmaVelocityLife->get(pos).w;
	density			= objData.pmaDensity ? objData.pmaDensity->get(pos) : 0;
	collisionNormal	= objData.pmaCollisionNormalFlags ? objData.pmaCollisionNormalFlags->get(pos).getXYZ() : physx::PxVec3(0.0f);
	collisionFlags	= objData.pmaCollisionNormalFlags ? *(const PxU32*)(&objData.pmaCollisionNormalFlags->get(pos).w) : 0;

	userData		= objData.pmaUserData->get(pos);
}

PX_INLINE void SpriteInput::load(const IosObjectBaseData& objData, PxU32 pos)
{
	position		= objData.pmaPositionMass->get(pos).getXYZ();
	mass			= objData.pmaPositionMass->get(pos).w;
	velocity		= objData.pmaVelocityLife->get(pos).getXYZ();
	liferemain		= objData.pmaVelocityLife->get(pos).w;
	density			= objData.pmaDensity ? objData.pmaDensity->get(pos) : 0;

	userData		= objData.pmaUserData->get(pos);
}


void IofxActorCPU::runModifiers()
{
	mWorkingVisibleCount = mWorkingRange.objectCount;

	IosObjectCpuData* obj = DYNAMIC_CAST(IosObjectCpuData*)(mMgr.mWorkingIosData);
	if (mMgr.mIsMesh)
	{
		IofxOutputDataMesh* meshOutputData = DYNAMIC_CAST(IofxOutputDataMesh*)(obj->outputData);

		MeshOutputLayout outputLayout;
		PX_COMPILE_TIME_ASSERT(sizeof(outputLayout.offsets) == sizeof(meshOutputData->getVertexDesc().semanticOffsets));
		::memcpy(outputLayout.offsets, meshOutputData->getVertexDesc().semanticOffsets, sizeof(outputLayout.offsets));	
		outputLayout.stride = meshOutputData->getVertexDesc().stride;

		updateParticles<MeshInput, MeshPublicState, MeshPrivateState>(*obj, 
																	outputLayout,
																	static_cast<PxU8*>(meshOutputData->getDefaultBuffer().getPtr())
																	);
	}
	else
	{
		if (mDistanceSortingEnabled)
		{
			const ObjectRange& range = mWorkingRange;

			mWorkingVisibleCount = 0;
			PX_ASSERT(obj->sortingKeys != NULL);
			for (PxU32 outputId = range.startIndex; outputId < range.startIndex + range.objectCount ; outputId++)
			{
				PxU32 stateId = obj->outputToState[ outputId ];
				PX_ASSERT(stateId != NiIosBufferDesc::NOT_A_PARTICLE);
				PxU32 inputId = obj->pmaInStateToInput->get(stateId);
				inputId &= ~NiIosBufferDesc::NEW_PARTICLE_FLAG;

				const physx::PxVec3 position = obj->pmaPositionMass->get(inputId).getXYZ();
				PxF32 sortDistance = obj->zNear + (obj->eyePosition - position).dot(obj->eyeDirection);

				//build uint key for radix sorting (flip float)
				PxU32 key = *reinterpret_cast<const PxU32*>(&sortDistance);
				PxU32 mask = -PxI32(key >> 31) | 0x80000000;
				key ^= mask;

				obj->sortingKeys[ outputId ] = key;
				if ((key & 0x80000000) == 0) ++mWorkingVisibleCount;
			}

			PxU32* keys = obj->sortingKeys + range.startIndex;
			PxU32* values = obj->outputToState + range.startIndex;
			iofx::ARLSort<PxU32, PxU32>::sort(keys, values, range.objectCount);

			if (range.objectCount > 0)
			{
				PX_ASSERT(values[range.objectCount - 1] < obj->maxStateID);
			}
		}

		IofxOutputDataSprite* spriteOutputData = DYNAMIC_CAST(IofxOutputDataSprite*)(obj->outputData);
		if (spriteOutputData->getTextureCount() > 0)
		{
			SpriteTextureOutputLayout outputLayout;
			outputLayout.textureCount = spriteOutputData->getTextureCount();
			for (PxU32 i = 0; i < outputLayout.textureCount; ++i)
			{
				const NxUserRenderSpriteTextureDesc& textureDesc = spriteOutputData->getVertexDesc().textureDescs[i];
				outputLayout.textureData[i].layout = static_cast<PxU16>(textureDesc.layout);

				PX_ASSERT(isPowerOfTwo(textureDesc.width));
				outputLayout.textureData[i].widthShift = static_cast<PxU8>(highestSetBit(textureDesc.width));
				PX_ASSERT(isPowerOfTwo(textureDesc.pitchBytes));
				outputLayout.textureData[i].pitchShift = static_cast<PxU8>(highestSetBit(textureDesc.pitchBytes));

				outputLayout.texturePtr[i] = static_cast<PxU8*>(spriteOutputData->getTextureBuffer(i).getPtr());
			}

			updateParticles<SpriteInput, SpritePublicState, SpritePrivateState>(*obj, outputLayout, NULL);
		}
		else
		{
			SpriteOutputLayout outputLayout;
			/* Copy user-defined semantics offsets to output offsets */
			PX_COMPILE_TIME_ASSERT(sizeof(outputLayout.offsets) == sizeof(spriteOutputData->getVertexDesc().semanticOffsets));
			::memcpy(outputLayout.offsets, spriteOutputData->getVertexDesc().semanticOffsets, sizeof(outputLayout.offsets));
			outputLayout.stride = spriteOutputData->getVertexDesc().stride;

			updateParticles<SpriteInput, SpritePublicState, SpritePrivateState>(*obj, 
																				outputLayout,
																				static_cast<PxU8*>(spriteOutputData->getDefaultBuffer().getPtr())
																				);
		}
	}
}

class ModifierParamsMapperCPUimpl : public ModifierParamsMapperCPU
{
	template <typename T>
	void _mapValue(size_t offset, T value)
	{
		if (mData != 0)
		{
			*(T*)(mData + mParamsOffset + offset) = value;
		}
	}

public:
	virtual void beginParams(void* , size_t size, size_t align, physx::PxU32)
	{
		mParamsOffset = (mTotalSize + (PxU32)align - 1) & ~((PxU32)align - 1);
		mTotalSize = mParamsOffset + (PxU32)size;
	}
	virtual void endParams()
	{
	}

	virtual void mapValue(size_t offset, PxI32 value)
	{
		_mapValue(offset, value);
	}
	virtual void mapValue(size_t offset, PxReal value)
	{
		_mapValue(offset, value);
	}
	virtual void mapCurve(size_t offset, const NxCurve* curve)
	{
		_mapValue(offset, curve);
	}

	ModifierParamsMapperCPUimpl()
	{
		reset(0);
	}

	void reset(PxU8* data)
	{
		mData = data;
		mTotalSize = 0;
		mParamsOffset = 0;
	}

	PxU32 getTotalSize() const
	{
		return mTotalSize;
	}
	PxU32 getParamsOffset() const
	{
		return mParamsOffset;
	}

private:
	PxU8*	mData;

	PxU32	mTotalSize;
	PxU32	mParamsOffset;
};


PxU32 IofxAssetSceneInstCPU::introspectModifiers(ModifierParamsMapperCPUimpl& introspector, ModifierInfo* list, const ModifierStack& stack, ModifierStage stage, PxU32 usageClass)
{
	PxU32 usageStage = ModifierUsageFromStage(stage);
	PxU32 count = 0;
	for (ModifierStack::ConstIterator it = stack.begin(); it != stack.end(); ++it)
	{
		//PxU32 type = (*it)->getModifierType();
		PxU32 usage = (*it)->getModifierUsage();
		if ((usage & usageStage) == usageStage && (usage & usageClass) == usageClass)
		{
			const Modifier* modifier = Modifier::castFrom(*it);
			modifier->mapParamsCPU(introspector);
			if (list != 0)
			{
				if (usageClass == ModifierUsage_Sprite)
				{
					list[count].updateSpriteFunc = modifier->getUpdateSpriteFunc(stage);
				}
				else
				{
					list[count].updateMeshFunc = modifier->getUpdateMeshFunc(stage);
				}

				list[count].paramsOffset = introspector.getParamsOffset();
			}
			++count;
		}
	}
	return count;
}

IofxAssetSceneInstCPU::IofxAssetSceneInstCPU(IofxAsset* asset, PxU32 assetID, PxU32 semantics, IofxScene* scene)
	: IofxAssetSceneInst(asset, assetID, semantics)
{
	PX_UNUSED(scene);

	const ModifierStack& spawnModifiers = asset->getModifierStack(ModifierStage_Spawn);
	const ModifierStack& continuousModifiers = asset->getModifierStack(ModifierStage_Continuous);

	// set the particle buffer type here
	PxU32 usageClass = (asset->getMeshAssetCount() > 0) ? ModifierUsage_Mesh : ModifierUsage_Sprite;

	//collect params
	ModifierParamsMapperCPUimpl paramsMapper;

	PxU32 spawnModifierCount = introspectModifiers(paramsMapper, 0, spawnModifiers, ModifierStage_Spawn, usageClass);
	PxU32 continuousModifierCount = introspectModifiers(paramsMapper, 0, continuousModifiers, ModifierStage_Continuous, usageClass);

	mSpawnModifiersList.resize(spawnModifierCount);
	mContinuousModifiersList.resize(continuousModifierCount);

	mModifiersParamsBuffer.resize(paramsMapper.getTotalSize());

	paramsMapper.reset(mModifiersParamsBuffer.begin());

	introspectModifiers(paramsMapper, mSpawnModifiersList.begin(), spawnModifiers, ModifierStage_Spawn, usageClass);
	introspectModifiers(paramsMapper, mContinuousModifiersList.begin(), continuousModifiers, ModifierStage_Continuous, usageClass);
}

}
}
} // namespace physx::apex
