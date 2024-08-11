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

#include "AttractorFSActor.h"
#include "AttractorFSAsset.h"
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

#define NUM_DEBUG_POINTS 2048

AttractorFSActor::AttractorFSActor(const AttractorFSActorParams& params, AttractorFSAsset& asset, NxResourceList& list, BasicFSScene& scene)
	: BasicFSActor(scene)
	, mAsset(&asset)
{
	mPosition = params.position;
//	mScale = params.initialScale * asset.mParams->defaultScale;

	mRadius					= mAsset->mParams->radius;
	mConstFieldStrength		= mAsset->mParams->constFieldStrength;
	mVariableFieldStrength	= mAsset->mParams->variableFieldStrength;

//	mLocalDirVar = PxVec3(0, 1, 0);

	list.add(*this);			// Add self to asset's list of actors
	addSelfToContext(*scene.getApexScene().getApexContext());    // Add self to ApexScene
	addSelfToContext(scene);	// Add self to BasicFSScene's list of actors

	NiFieldSamplerManager* fieldSamplerManager = mScene->getNiFieldSamplerManager();
	if (fieldSamplerManager != 0)
	{
		NiFieldSamplerDesc fieldSamplerDesc;
		fieldSamplerDesc.type = NiFieldSamplerType::VELOCITY_DIRECT;
		fieldSamplerDesc.gridSupportType = NiFieldSamplerGridSupportType::NONE;

		fieldSamplerDesc.samplerFilterData = ApexResourceHelper::resolveCollisionGroup64(params.fieldSamplerFilterDataName ? params.fieldSamplerFilterDataName : mAsset->mParams->fieldSamplerFilterDataName);
		fieldSamplerDesc.boundaryFilterData = ApexResourceHelper::resolveCollisionGroup64(params.fieldBoundaryFilterDataName ? params.fieldBoundaryFilterDataName : mAsset->mParams->fieldBoundaryFilterDataName);
		fieldSamplerDesc.boundaryFadePercentage = mAsset->mParams->boundaryFadePercentage;

		fieldSamplerManager->registerFieldSampler(this, fieldSamplerDesc, mScene);
		mFieldSamplerChanged = true;
	}
}

AttractorFSActor::~AttractorFSActor()
{
}

/* Must be defined inside CPP file, since they require knowledge of asset class */
NxApexAsset* 		AttractorFSActor::getOwner() const
{
	return static_cast<NxApexAsset*>(mAsset);
}

NxBasicFSAsset* 	AttractorFSActor::getAttractorFSAsset() const
{
	return mAsset;
}

void				AttractorFSActor::release()
{
	if (mInRelease)
	{
		return;
	}
	destroy();
} 

void AttractorFSActor::destroy()
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

void AttractorFSActor::getPhysicalLodRange(PxReal& min, PxReal& max, bool& intOnly)
{
	PX_UNUSED(min);
	PX_UNUSED(max);
	PX_UNUSED(intOnly);
	APEX_INVALID_OPERATION("not implemented");
}

physx::PxF32 AttractorFSActor::getActivePhysicalLod()
{
	APEX_INVALID_OPERATION("NxExampleActor does not support this operation");
	return -1.0f;
}

void AttractorFSActor::forcePhysicalLod(PxReal lod)
{
	PX_UNUSED(lod);
	APEX_INVALID_OPERATION("not implemented");
}

// Called by game render thread
void AttractorFSActor::updateRenderResources(bool rewriteBuffers, void* userRenderData)
{
	PX_UNUSED(rewriteBuffers);
	PX_UNUSED(userRenderData);
}

// Called by game render thread
void AttractorFSActor::dispatchRenderResources(NxUserRenderer& renderer)
{
	PX_UNUSED(renderer);
}

bool AttractorFSActor::updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled)
{
	isEnabled = mFieldSamplerEnabled;
	if (mFieldSamplerChanged)
	{
		shapeDesc.type = NiFieldShapeType::SPHERE;
		shapeDesc.worldToShape.M.setIdentity();

		shapeDesc.worldToShape.t				= -mPosition;
		shapeDesc.dimensions.x					= mRadius;

		mExecuteParams.origin					= mPosition;
		mExecuteParams.radius					= mRadius;
		mExecuteParams.constFieldStrength		= mConstFieldStrength;
		mExecuteParams.variableFieldStrength	= mVariableFieldStrength;

		mFieldSamplerChanged					= false;
		return true;
	}
	return false;
}

void AttractorFSActor::simulate(physx::PxF32 dt)
{
	PX_UNUSED(dt);
}

void AttractorFSActor::setConstFieldStrength(physx::PxF32 strength)
{
	mConstFieldStrength = strength;
	mFieldSamplerChanged = true;
}

void AttractorFSActor::setVariableFieldStrength(physx::PxF32 strength)
{
	mVariableFieldStrength = strength;
	mFieldSamplerChanged = true;
}

void AttractorFSActor::visualize()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	NiApexRenderDebug* debugRender = mScene->mDebugRender;
	BasicFSDebugRenderParams* debugRenderParams = mScene->mBasicFSDebugRenderParams;

	if (!debugRenderParams->VISUALIZE_ATTRACTOR_FS_ACTOR)
	{
		return;
	}

	if (debugRenderParams->VISUALIZE_ATTRACTOR_FS_ACTOR_NAME)
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

	if (debugRenderParams->VISUALIZE_ATTRACTOR_FS_SHAPE)
	{
		debugRender->setCurrentColor(debugRender->getDebugColor(physx::DebugColors::Blue));
//		debugRender->debugOrientedSphere(mExecuteParams.radius, 2, mDirToWorld);
		debugRender->debugSphere(mExecuteParams.origin, mExecuteParams.radius);
	}

	if (debugRenderParams->VISUALIZE_ATTRACTOR_FS_FIELD)
	{
	#if 0
		PxVec3 rayBeg0 = mPosition;
		PxVec3 rayEnd0 = rayBeg0 + mConstFieldStrength * mFieldDirection; // Add invert square field force
		debugRender->setCurrentColor(debugRender->getDebugColor(physx::DebugColors::DarkBlue));
		debugRender->debugRay(rayBeg0, rayEnd0);
	#endif

		//PxVec3 rayBeg = mPose.t;
		//PxVec3 rayEnd = rayBeg + (mExecuteParams.strength * fieldScale) * mExecuteParams.direction;
		//debugRender->setCurrentColor( debugRender->getDebugColor(physx::DebugColors::Blue) );
		//debugRender->debugRay(rayBeg, rayEnd);
	}
	if (debugRenderParams->VISUALIZE_ATTRACTOR_FS_POSE)
	{
		debugRender->debugAxes(PxMat44(mPose), 1);
	}

	if (mDebugPoints.empty())
	{
		mDebugPoints.resize(NUM_DEBUG_POINTS);
		for (PxU32 i = 0; i < NUM_DEBUG_POINTS; ++i)
		{
//			PxF32 dist = physx::rand(-spreadDistance, +spreadDistance);

			PxF32 radius = mExecuteParams.radius;

			PxF32 rx, ry, rz;
			do
			{
				rx = physx::rand(-1.0f, +1.0f);
				ry = physx::rand(-1.0f, +1.0f);
				rz = physx::rand(-1.0f, +1.0f);
			}
			while (rx * rx + ry * ry + rz * rz > 1.0f);

			PxVec3& vec = mDebugPoints[i];

			vec.x = rx * radius;
			vec.y = ry * radius;
			vec.z = rz * radius;
		}
	}

	if (debugRenderParams->VISUALIZE_ATTRACTOR_FS_FIELD)
	{
		PxU32 c1 = mScene->mDebugRender->getDebugColor(physx::DebugColors::Blue);
		PxU32 c2 = mScene->mDebugRender->getDebugColor(physx::DebugColors::Red);

		for (PxU32 i = 0; i < NUM_DEBUG_POINTS; ++i)
		{
			const PxVec3& localPos = mDebugPoints[i];
			PxVec3 pos = mExecuteParams.origin + localPos;
			PxVec3 fieldVec = executeAttractorFS(mExecuteParams, pos/*, totalElapsedMS*/);
			debugRender->debugGradientLine(pos, pos + fieldVec, c1, c2);
		}
	}

#endif
}

/******************************** CPU Version ********************************/

AttractorFSActorCPU::AttractorFSActorCPU(const AttractorFSActorParams& params, AttractorFSAsset& asset, NxResourceList& list, BasicFSScene& scene)
	: AttractorFSActor(params, asset, list, scene)
{
}

AttractorFSActorCPU::~AttractorFSActorCPU()
{
}

void AttractorFSActorCPU::executeFieldSampler(const ExecuteData& data)
{
	for (physx::PxU32 i = 0; i < data.count; ++i)
	{
		physx::PxVec3* pos = (physx::PxVec3*)((physx::PxU8*)data.position + i * data.stride);
		data.resultField[i] = executeAttractorFS(mExecuteParams, *pos/*, totalElapsedMS*/);
	}
}

/******************************** GPU Version ********************************/

#if defined(APEX_CUDA_SUPPORT)


AttractorFSActorGPU::AttractorFSActorGPU(const AttractorFSActorParams& params, AttractorFSAsset& asset, NxResourceList& list, BasicFSScene& scene)
	: AttractorFSActor(params, asset, list, scene)
	, mConstMemGroup(CUDA_OBJ(fieldsamplerConstMem))
{
}

AttractorFSActorGPU::~AttractorFSActorGPU()
{
}

bool AttractorFSActorGPU::updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled)
{
	if (AttractorFSActor::updateFieldSampler(shapeDesc, isEnabled))
	{
		APEX_CUDA_CONST_MEM_GROUP_SCOPE(mConstMemGroup);

		AttractorFSParams* params = mParamsHandle.allocOrResolve(_storage_);
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
