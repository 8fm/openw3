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
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2

#include "ExplosionActor.h"
#include "ExplosionAsset.h"
#include "ExplosionScene.h"
#include "NiFieldSamplerManager.h"
#include "ApexResourceHelper.h"
#include "PsShare.h"

#include "NiApexScene.h"

namespace physx
{
namespace apex
{
namespace explosion
{

/* Parameters to control explosion field sampler behaviour */
#define EXPLOSION_DRAG_COEFFICIENT_SCALE	3.0f	//used for velocity drag field sampler type
#define EXPLOSION_RADIUS					3.0f
#define EXPLOSION_STRENGTH					3.0f

void ExplosionActor::initFieldSampler()
{
	NiFieldSamplerManager* fieldSamplerManager = mExplosionScene->getNiFieldSamplerManager();
	if (fieldSamplerManager != 0)
	{
		NiFieldSamplerDesc fieldSamplerDesc;
		fieldSamplerDesc.type = NiFieldSamplerType::FORCE;
		fieldSamplerDesc.gridSupportType = NiFieldSamplerGridSupportType::SINGLE_VELOCITY;
		fieldSamplerDesc.dragCoeff = EXPLOSION_DRAG_COEFFICIENT_SCALE;

		//fieldSamplerDesc.collisionGroup = ApexResourceHelper::resolveCollisionGroup(params.collisionGroupName != 0 ? params.collisionGroupName : mAsset->mParams->collisionGroupName);
		//fieldSamplerDesc.collisionGroup128 = ApexResourceHelper::resolveCollisionGroup128(params.collisionGroupMaskName != 0 ? params.collisionGroupMaskName : mAsset->mParams->collisionGroupMaskName);
		//fieldSamplerDesc.boundaryGroup64 = ApexResourceHelper::resolveCollisionGroup64(params.boundaryGroupMaskName ? params.boundaryGroupMaskName : mAsset->mParams->boundaryGroupMaskName);
		//fieldSamplerDesc.boundaryFadePercentage = mAsset->mParams->boundaryFadePercentage;

		fieldSamplerManager->registerFieldSampler(this, fieldSamplerDesc, mExplosionScene);
		mFieldSamplerChanged = true;
	}
}

void ExplosionActor::releaseFieldSampler()
{
	NiFieldSamplerManager* fieldSamplerManager = mExplosionScene->getNiFieldSamplerManager();
	if (fieldSamplerManager != 0)
	{
		fieldSamplerManager->unregisterFieldSampler(this);
	}
}

bool ExplosionActor::updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled)
{
	isEnabled = !mDisable;
	if (mFieldSamplerChanged)
	{
		shapeDesc.type = NiFieldShapeType::BOX;
		shapeDesc.worldToShape = mPose.inverseRT();
		shapeDesc.dimensions = physx::PxVec3(40.0f, 10.0f, 20.0f);	//this should be based off a parameter in the actor

		mExecuteParams.pose = mPose;
		mExecuteParams.radius = EXPLOSION_RADIUS;
		mExecuteParams.strength = EXPLOSION_STRENGTH;

		mFieldSamplerChanged = false;
		return true;
	}
	return false;
}

/******************************** CPU Version ********************************/

ExplosionActorCPU::ExplosionActorCPU(const NxExplosionActorDesc& desc, ExplosionAsset& asset, NxResourceList& list, ExplosionScene& scene)
	: ExplosionActor(desc, asset, list, scene)
{
}

ExplosionActorCPU::~ExplosionActorCPU()
{
}

void ExplosionActorCPU::executeFieldSampler(const ExecuteData& data)
{
	physx::PxU32 totalElapsedMS = mExplosionScene->getApexScene().getTotalElapsedMS();

	for (physx::PxU32 i = 0; i < data.count; ++i)
	{
		physx::PxVec3* pos = (physx::PxVec3*)((physx::PxU8*)data.position + i * data.stride);
		data.resultField[i] = executeExplosionFS(mExecuteParams, *pos, totalElapsedMS);
	}
}

/******************************** GPU Version ********************************/

#if defined(APEX_CUDA_SUPPORT)

ExplosionActorGPU::ExplosionActorGPU(const NxExplosionActorDesc& desc, ExplosionAsset& asset, NxResourceList& list, ExplosionScene& scene)
	: ExplosionActor(desc, asset, list, scene)
	, mConstMemGroup(CUDA_OBJ(fieldsamplerConstMem))
{
}

ExplosionActorGPU::~ExplosionActorGPU()
{
}

bool ExplosionActorGPU::updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled)
{
	if (ExplosionActor::updateFieldSampler(shapeDesc, isEnabled))
	{
		APEX_CUDA_CONST_MEM_GROUP_SCOPE(mConstMemGroup);

		ExplosionFSParams* params = mParamsHandle.allocOrResolve(_storage_);
		if (params)
		{
			params->pose = mExecuteParams.pose;
			params->radius = mExecuteParams.radius;
			params->strength = mExecuteParams.strength;
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
