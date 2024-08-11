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
#include "NxRenderMeshActorDesc.h"
#include "NxRenderMeshActor.h"
#include "NxRenderMeshAsset.h"

#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

#include "NxApex.h"

#include "NoiseFSActor.h"
#include "NoiseFSAsset.h"
#include "BasicFSScene.h"
#include "NiApexSDK.h"
#include "NiApexScene.h"
#include "NiApexRenderDebug.h"

#if NX_SDK_VERSION_MAJOR == 2
#include <NxScene.h>
#include "NxFromPx.h"
#elif NX_SDK_VERSION_MAJOR == 3
#include <PxScene.h>
#endif

#include <NiFieldSamplerManager.h>
#include "ApexResourceHelper.h"

namespace physx
{
namespace apex
{
namespace basicfs
{

NoiseFSActor::NoiseFSActor(const NoiseFSActorParams& params, NoiseFSAsset& asset, NxResourceList& list, BasicFSScene& scene)
	: BasicFSActor(scene)
	, mAsset(&asset)
{
	mPose = params.initialPose;
	mScale = params.initialScale * asset.mParams->defaultScale;

	mExecuteParams.useLocalSpace = mAsset->mParams->useLocalSpace;

	mExecuteParams.noiseStrength = mAsset->mParams->noiseStrength;
	mExecuteParams.noiseSpaceFreq = PxVec3(1.0f / mAsset->mParams->noiseSpacePeriod.x, 1.0f / mAsset->mParams->noiseSpacePeriod.y, 1.0f / mAsset->mParams->noiseSpacePeriod.z);
	mExecuteParams.noiseTimeFreq = 1.0f / mAsset->mParams->noiseTimePeriod;
	mExecuteParams.noiseOctaves = mAsset->mParams->noiseOctaves;
	mExecuteParams.noiseStrengthOctaveMultiplier = mAsset->mParams->noiseStrengthOctaveMultiplier;
	mExecuteParams.noiseSpaceFreqOctaveMultiplier = PxVec3(1.0f / mAsset->mParams->noiseSpacePeriodOctaveMultiplier.x, 1.0f / mAsset->mParams->noiseSpacePeriodOctaveMultiplier.y, 1.0f / mAsset->mParams->noiseSpacePeriodOctaveMultiplier.z);
	mExecuteParams.noiseTimeFreqOctaveMultiplier = 1.0f / mAsset->mParams->noiseTimePeriodOctaveMultiplier;

	if (strcmp(mAsset->mParams->noiseType, "CURL") == 0)
	{
		mExecuteParams.noiseType = NoiseType::CURL;
	}
	else
	{
		mExecuteParams.noiseType = NoiseType::SIMPLEX;
	}
	mExecuteParams.noiseSeed = mAsset->mParams->noiseSeed;

	list.add(*this);			// Add self to asset's list of actors
	addSelfToContext(*scene.getApexScene().getApexContext());    // Add self to ApexScene
	addSelfToContext(scene);	// Add self to BasicFSScene's list of actors

	NiFieldSamplerManager* fieldSamplerManager = mScene->getNiFieldSamplerManager();
	if (fieldSamplerManager != 0)
	{
		NiFieldSamplerDesc fieldSamplerDesc;

		fieldSamplerDesc.gridSupportType = NiFieldSamplerGridSupportType::VELOCITY_PER_CELL;
		if (strcmp(mAsset->mParams->fieldType, "FORCE") == 0)
		{
			fieldSamplerDesc.type = NiFieldSamplerType::FORCE;
			fieldSamplerDesc.gridSupportType = NiFieldSamplerGridSupportType::NONE;
		}
		else if (strcmp(mAsset->mParams->fieldType, "VELOCITY_DRAG") == 0)
		{
			fieldSamplerDesc.type = NiFieldSamplerType::VELOCITY_DRAG;
			fieldSamplerDesc.dragCoeff = mAsset->mParams->fieldDragCoeff;
		}
		else
		{
			fieldSamplerDesc.type = NiFieldSamplerType::VELOCITY_DIRECT;
		}

		fieldSamplerDesc.samplerFilterData = ApexResourceHelper::resolveCollisionGroup64(params.fieldSamplerFilterDataName ? params.fieldSamplerFilterDataName : mAsset->mParams->fieldSamplerFilterDataName);
		fieldSamplerDesc.boundaryFilterData = ApexResourceHelper::resolveCollisionGroup64(params.fieldBoundaryFilterDataName ? params.fieldBoundaryFilterDataName : mAsset->mParams->fieldBoundaryFilterDataName);
		fieldSamplerDesc.boundaryFadePercentage = mAsset->mParams->boundaryFadePercentage;

		fieldSamplerManager->registerFieldSampler(this, fieldSamplerDesc, mScene);
		mFieldSamplerChanged = true;
	}
}

NoiseFSActor::~NoiseFSActor()
{
}

/* Must be defined inside CPP file, since they require knowledge of asset class */
NxApexAsset* 		NoiseFSActor::getOwner() const
{
	return static_cast<NxApexAsset*>(mAsset);
}

NxBasicFSAsset* 	NoiseFSActor::getNoiseFSAsset() const
{
	return mAsset;
}

void				NoiseFSActor::release()
{
	if (mInRelease)
	{
		return;
	}
	destroy();
} 

void NoiseFSActor::destroy()
{
	ApexActor::destroy();

	setPhysXScene(NULL);

	NiFieldSamplerManager* fieldSamplerManager = mScene->getNiFieldSamplerManager();
	if (fieldSamplerManager != 0)
	{
		fieldSamplerManager->unregisterFieldSampler(this);
	}

	delete this;
}

void NoiseFSActor::getPhysicalLodRange(PxReal& min, PxReal& max, bool& intOnly)
{
	PX_UNUSED(min);
	PX_UNUSED(max);
	PX_UNUSED(intOnly);
	APEX_INVALID_OPERATION("not implemented");
}

physx::PxF32 NoiseFSActor::getActivePhysicalLod()
{
	APEX_INVALID_OPERATION("NxExampleActor does not support this operation");
	return -1.0f;
}

void NoiseFSActor::forcePhysicalLod(PxReal lod)
{
	PX_UNUSED(lod);
	APEX_INVALID_OPERATION("not implemented");
}

// Called by game render thread
void NoiseFSActor::updateRenderResources(bool rewriteBuffers, void* userRenderData)
{
	PX_UNUSED(rewriteBuffers);
	PX_UNUSED(userRenderData);
}

// Called by game render thread
void NoiseFSActor::dispatchRenderResources(NxUserRenderer& renderer)
{
	PX_UNUSED(renderer);
}

bool NoiseFSActor::updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled)
{
	PX_UNUSED(shapeDesc);

	isEnabled = mFieldSamplerEnabled;
	if (mFieldSamplerChanged)
	{
		mExecuteParams.worldToShape = mPose.getInverseRT();

		shapeDesc.type = NiFieldShapeType::BOX;
		shapeDesc.worldToShape = mExecuteParams.worldToShape;
		shapeDesc.dimensions = mAsset->mParams->boundarySize * (mScale * 0.5f);
		shapeDesc.weight = 1;

		mFieldSamplerChanged = false;
		return true;
	}
	return false;
}

void NoiseFSActor::simulate(physx::PxF32 )
{
}

void NoiseFSActor::setNoiseStrength(physx::PxF32 strength)
{
	mExecuteParams.noiseStrength = strength;
	mFieldSamplerChanged = true;
}

void NoiseFSActor::visualize()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	NiApexRenderDebug* debugRender = mScene->mDebugRender;
	BasicFSDebugRenderParams* debugRenderParams = mScene->mBasicFSDebugRenderParams;

	if (!debugRenderParams->VISUALIZE_NOISE_FS_ACTOR)
	{
		return;
	}

	if (debugRenderParams->VISUALIZE_NOISE_FS_ACTOR_NAME)
	{
		char buf[128];
		buf[sizeof(buf) - 1] = 0;
		APEX_SPRINTF_S(buf, sizeof(buf) - 1, " %s %s", mAsset->getObjTypeName(), mAsset->getName());

		PxMat44 cameraFacingPose(mScene->mApexScene->getViewMatrix(0).inverseRT());
		PxVec3 textLocation = mPose.t;
		cameraFacingPose.setPosition(textLocation);

		debugRender->setCurrentTextScale(4.0f);
		debugRender->setCurrentColor(debugRender->getDebugColor(physx::DebugColors::Blue));
		debugRender->debugOrientedText(cameraFacingPose, buf);
	}

	if (debugRenderParams->VISUALIZE_NOISE_FS_SHAPE)
	{
		debugRender->setCurrentColor(debugRender->getDebugColor(physx::DebugColors::Blue));

		PxVec3 shapeDim = mScale * mAsset->mParams->boundarySize;
		debugRender->debugOrientedBound( shapeDim * 2, mPose );
	}
	if (debugRenderParams->VISUALIZE_NOISE_FS_POSE)
	{
		debugRender->debugAxes(PxMat44(mPose), 1);
	}
#endif
}

/******************************** CPU Version ********************************/

NoiseFSActorCPU::NoiseFSActorCPU(const NoiseFSActorParams& params, NoiseFSAsset& asset, NxResourceList& list, BasicFSScene& scene)
	: NoiseFSActor(params, asset, list, scene)
{
}

NoiseFSActorCPU::~NoiseFSActorCPU()
{
}

void NoiseFSActorCPU::executeFieldSampler(const ExecuteData& data)
{
	PxU32 totalElapsedMS = mScene->getApexScene().getTotalElapsedMS();

	for (physx::PxU32 i = 0; i < data.count; ++i)
	{
		physx::PxVec3* pos = (physx::PxVec3*)((physx::PxU8*)data.position + i * data.stride);
		data.resultField[i] = executeNoiseFS(mExecuteParams, *pos, totalElapsedMS);
	}
}

/******************************** GPU Version ********************************/

#if defined(APEX_CUDA_SUPPORT)


NoiseFSActorGPU::NoiseFSActorGPU(const NoiseFSActorParams& params, NoiseFSAsset& asset, NxResourceList& list, BasicFSScene& scene)
	: NoiseFSActor(params, asset, list, scene)
	, mConstMemGroup(CUDA_OBJ(fieldsamplerConstMem))
{
}

NoiseFSActorGPU::~NoiseFSActorGPU()
{
}

bool NoiseFSActorGPU::updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled)
{
	if (NoiseFSActor::updateFieldSampler(shapeDesc, isEnabled))
	{
		APEX_CUDA_CONST_MEM_GROUP_SCOPE(mConstMemGroup);

		NoiseFSParams* params = mParamsHandle.allocOrResolve(_storage_);
		PX_ASSERT(params);

		*params = mExecuteParams;
		return true;
	}
	return false;
}


#endif

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
