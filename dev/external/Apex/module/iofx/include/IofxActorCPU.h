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

#ifndef __IOFX_ACTOR_CPU_H__
#define __IOFX_ACTOR_CPU_H__

#include "NxApex.h"
#include "NxIofxActor.h"
#include "NiInstancedObjectSimulation.h"
#include "NiResourceProvider.h"
#include "ApexInterface.h"
#include "ApexActor.h"
#include "IofxActor.h"
#include "IofxScene.h"

#include "ModifierData.h"

namespace physx
{
namespace apex
{
namespace iofx
{

class NxModifier;
class IofxAsset;
class IofxManager;
class IofxActorCPU;
class IosObjectCpuData;

class TaskModifiers : public physx::PxLightCpuTask
{
public:
	TaskModifiers(IofxActorCPU& owner) : mOwner(owner) {}
	const char* getName() const
	{
		return "IofxActorCPU::Modifiers";
	}
	void run();

protected:
	IofxActorCPU& mOwner;

private:
	TaskModifiers& operator=(const TaskModifiers&);
};

class IofxActorCPU : public IofxActor
{
public:
	IofxActorCPU(NxApexAsset*, IofxScene*, IofxManager&);
	~IofxActorCPU() {}

	void updateBounds();

	void runModifiers();


	template <typename Input, typename PublicState, typename PrivateState, typename OutputLayout>
	void updateParticles(const IosObjectCpuData& objData, const OutputLayout& outputLayout, const physx::PxU8* ptr);


	TaskModifiers	            mModifierTask;

	PxBounds3                   mWorkingBounds;
	ObjectRange					mWorkingRange;
	PxU32						mWorkingVisibleCount;
};

template <typename T>
void FromSlices(IofxSlice** slices, PxU32 idx, T& val)
{
	size_t i = 0;
	IofxSlice* data = (IofxSlice*)&val;
	for (; i != sizeof(T) / sizeof(IofxSlice); ++i, ++data)
	{
		*data = slices[i][idx];
	}

	PxU32 tail = sizeof(T) % sizeof(IofxSlice);
	if (!tail)
	{
		return;
	}

	PxU32* u32s = (PxU32*)data;

	if (4 == tail)
	{
		u32s[0] = slices[i][idx].x;
	}
	else if (8 == tail)
	{
		u32s[0] = slices[i][idx].x;
		u32s[1] = slices[i][idx].y;
	}
	else if (12 == tail)
	{
		u32s[0] = slices[i][idx].x;
		u32s[1] = slices[i][idx].y;
		u32s[2] = slices[i][idx].z;
	}
	else
	{
		PX_ALWAYS_ASSERT();
	}
}

template <typename T>
void ToSlices(IofxSlice** slices, PxU32 idx, const T& val)
{
	size_t i = 0;
	const IofxSlice* data = (const IofxSlice*)&val;
	for (; i != sizeof(T) / sizeof(IofxSlice); ++i, ++data)
	{
		slices[i][idx] = *data;
	}

	PxU32 tail = sizeof(T) % sizeof(IofxSlice);
	if (!tail)
	{
		return;
	}

	const PxU32* u32s = (const PxU32*)data;

	if (4 == tail)
	{
		slices[i][idx].x = u32s[0];
	}
	else if (8 == tail)
	{
		slices[i][idx].x = u32s[0];
		slices[i][idx].y = u32s[1];
	}
	else if (12 == tail)
	{
		slices[i][idx].x = u32s[0];
		slices[i][idx].y = u32s[1];
		slices[i][idx].z = u32s[2];
	}
	else
	{
		PX_ALWAYS_ASSERT();
	}
}

template <typename Input, typename PublicState, typename PrivateState, typename OutputLayout>
void IofxActorCPU::updateParticles(const IosObjectCpuData& objData, const OutputLayout& outputLayout, const physx::PxU8* ptr)
{
	IofxSlice** inPubState = objData.inPubState;
	IofxSlice** inPrivState = objData.inPrivState;
	IofxSlice** outPubState = objData.outPubState;
	IofxSlice** outPrivState = objData.outPrivState;

	ModifierCommonParams common = objData.getCommonParams();

	RandState randState(mIofxScene->mApexScene->getSeed());

	for (PxU32 id = 0; id < mWorkingRange.objectCount; id++)
	{
		PxU32 outputID = mWorkingRange.startIndex + id;
		PxU32 stateID = objData.outputToState[ outputID ];
		PX_ASSERT(stateID != NiIosBufferDesc::NOT_A_PARTICLE);
		PX_ASSERT(stateID < objData.maxStateID);
		PxU32 inputID = objData.pmaInStateToInput->get(stateID);

		bool newParticle = false;
		if (inputID & NiIosBufferDesc::NEW_PARTICLE_FLAG)
		{
			newParticle = true;
			inputID &= ~NiIosBufferDesc::NEW_PARTICLE_FLAG;
		}

		const NiIofxActorID actorId = objData.pmaActorIdentifiers->get(inputID);
		const PxU16 actorClassId = actorId.getActorClassID();
		PX_ASSERT(actorClassId < mMgr.mActorClassTable.size());
		IofxAssetSceneInstCPU* iofxAssetSceneInst = static_cast<IofxAssetSceneInstCPU*>(mMgr.mActorClassTable[ actorClassId ].iofxAssetSceneInst);
		PX_ASSERT(iofxAssetSceneInst != NULL);

		Input input;
		input.load(objData, inputID);

		PublicState pubState;
		PrivateState privState;

		if (newParticle)
		{
			PublicState::initDefault(pubState);
			PrivateState::initDefault(privState);

			for (PxU32 j = 0; j < iofxAssetSceneInst->mSpawnModifiersList.size(); j++)
			{
				const void* params = &iofxAssetSceneInst->mModifiersParamsBuffer[ iofxAssetSceneInst->mSpawnModifiersList[j].paramsOffset ];
				iofxAssetSceneInst->mSpawnModifiersList[j].updateFunc(params, input, pubState, privState, common, randState);
			}
		}
		else
		{
			PX_ASSERT(stateID < objData.maxObjectCount);

			FromSlices(inPubState, stateID, pubState);
			FromSlices(inPrivState, stateID, privState);
		}

		ToSlices(outPubState, outputID, pubState);

		for (PxU32 j = 0; j < iofxAssetSceneInst->mContinuousModifiersList.size(); j++)
		{
			const void* params = &iofxAssetSceneInst->mModifiersParamsBuffer[ iofxAssetSceneInst->mContinuousModifiersList[j].paramsOffset ];
			iofxAssetSceneInst->mContinuousModifiersList[j].updateFunc(params, input, pubState, privState, common, randState);
		}

		objData.pmaOutStateToInput->get(outputID) = inputID;
		ToSlices(outPrivState, outputID, privState);

		outputLayout.write(outputID, input, pubState, ptr);
	}
}

class ModifierParamsMapperCPUimpl;

class IofxAssetSceneInstCPU : public IofxAssetSceneInst
{
public:
	IofxAssetSceneInstCPU(IofxAsset* asset, PxU32 assetID, PxU32 semantics, IofxScene* scene);
	virtual ~IofxAssetSceneInstCPU() {}

	struct ModifierInfo
	{
		union
		{
			Modifier::updateSpriteFunc updateSpriteFunc;
			Modifier::updateMeshFunc updateMeshFunc;
		};
		void updateFunc(const void* params, const SpriteInput& input, SpritePublicState& pubState, SpritePrivateState& privState, const ModifierCommonParams& common, RandState& randState)
		{
			updateSpriteFunc(params, input, pubState, privState, common, randState);
		}
		void updateFunc(const void* params, const MeshInput& input, MeshPublicState& pubState, MeshPrivateState& privState, const ModifierCommonParams& common, RandState& randState)
		{
			updateMeshFunc(params, input, pubState, privState, common, randState);
		}

		PxU32 paramsOffset;
	};

	PxU32 introspectModifiers(ModifierParamsMapperCPUimpl& introspector, ModifierInfo* list, const ModifierStack& stack, ModifierStage stage, PxU32 usageClass);

	physx::Array<ModifierInfo>	mSpawnModifiersList;
	physx::Array<ModifierInfo>	mContinuousModifiersList;
	physx::Array<PxU8>			mModifiersParamsBuffer;
};


}
}
} // namespace apex

#endif // __IOFX_ACTOR_CPU_H__
