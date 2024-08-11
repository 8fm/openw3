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

#include "ForceFieldActor.h"
#include "ForceFieldAsset.h"
#include "ForceFieldScene.h"
#include "NiFieldSamplerManager.h"
#include "ApexResourceHelper.h"
#include "PsShare.h"

#include "NiApexScene.h"

namespace physx
{
namespace apex
{
namespace forcefield
{

void ForceFieldActor::initFieldSampler(const NxForceFieldActorDesc& desc)
{
	NiFieldSamplerManager* fieldSamplerManager = mForceFieldScene->getNiFieldSamplerManager();
	if (fieldSamplerManager != 0)
	{
		NiFieldSamplerDesc fieldSamplerDesc;
		fieldSamplerDesc.type = NiFieldSamplerType::FORCE;
		fieldSamplerDesc.gridSupportType = NiFieldSamplerGridSupportType::SINGLE_VELOCITY;

		fieldSamplerDesc.samplerFilterData = desc.samplerFilterData;
		fieldSamplerDesc.boundaryFilterData = desc.boundaryFilterData;
		
		fieldSamplerManager->registerFieldSampler(this, fieldSamplerDesc, mForceFieldScene);
		mFieldSamplerChanged = true;
	}
}

void ForceFieldActor::releaseFieldSampler()
{
	NiFieldSamplerManager* fieldSamplerManager = mForceFieldScene->getNiFieldSamplerManager();
	if (fieldSamplerManager != 0)
	{
		fieldSamplerManager->unregisterFieldSampler(this);
	}
}

bool ForceFieldActor::updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled)
{
	isEnabled = mEnable;
	if (mFieldSamplerChanged)
	{
		shapeDesc.type = NiFieldShapeType::NONE;	//not using field sampler include shape (force field has its own implementation for shapes)

		mExecuteParams.pose = mPose;
		mExecuteParams.strength = mStrength;
		mExecuteParams.includeShape = mIncludeShape;
		mExecuteParams.falloffTable = mFalloffTable;
		mExecuteParams.noiseParams = mNoiseParams;

		mFieldSamplerChanged = false;
		return true;
	}
	return false;
}

/******************************** CPU Version ********************************/

ForceFieldActorCPU::ForceFieldActorCPU(const NxForceFieldActorDesc& desc, ForceFieldAsset& asset, NxResourceList& list, ForceFieldScene& scene)
	: ForceFieldActor(desc, asset, list, scene)
{
}

ForceFieldActorCPU::~ForceFieldActorCPU()
{
}

void ForceFieldActorCPU::executeFieldSampler(const ExecuteData& data)
{
	// totalElapsedMS is always 0 in PhysX 3
	physx::PxU32 totalElapsedMS = mForceFieldScene->getApexScene().getTotalElapsedMS();

	for (physx::PxU32 i = 0; i < data.count; ++i)
	{
		physx::PxVec3* pos = (physx::PxVec3*)((physx::PxU8*)data.position + i * data.stride);
		data.resultField[i] = executeForceFieldFS(mExecuteParams, *pos, totalElapsedMS);
	}
}

/******************************** GPU Version ********************************/

#if defined(APEX_CUDA_SUPPORT)

ForceFieldActorGPU::ForceFieldActorGPU(const NxForceFieldActorDesc& desc, ForceFieldAsset& asset, NxResourceList& list, ForceFieldScene& scene)
	: ForceFieldActor(desc, asset, list, scene)
	, mConstMemGroup(CUDA_OBJ(fieldsamplerConstMem))
{
}

ForceFieldActorGPU::~ForceFieldActorGPU()
{
}

bool ForceFieldActorGPU::updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled)
{
	if (ForceFieldActor::updateFieldSampler(shapeDesc, isEnabled))
	{
		APEX_CUDA_CONST_MEM_GROUP_SCOPE(mConstMemGroup);

		ForceFieldFSParams* params = mParamsHandle.allocOrResolve(_storage_);
		if (params)
		{
			params->pose = mExecuteParams.pose;
			params->radius = mExecuteParams.radius;
			params->strength = mExecuteParams.strength;
			params->includeShape = mExecuteParams.includeShape;
			params->falloffTable = mExecuteParams.falloffTable;
			params->noiseParams = mExecuteParams.noiseParams;
		}
		return true;
	}
	return false;
}

#endif

}
}
} // namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
