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

#include "VortexFSActor.h"
#include "VortexFSAsset.h"
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

VortexFSActor::VortexFSActor(const VortexFSActorParams& params, VortexFSAsset& asset, NxResourceList& list, BasicFSScene& scene)
	: BasicFSActor(scene)
	, mAsset(&asset)
{
	mPose = params.initialPose;

	mAxis					= mAsset->mParams->axis;
	mHeight					= mAsset->mParams->height;
	mBottomRadius			= mAsset->mParams->bottomRadius;
	mTopRadius				= mAsset->mParams->topRadius;
	mRotationalStrength		= mAsset->mParams->rotationalStrength;
	mRadialStrength			= mAsset->mParams->radialStrength;
	mLiftStrength			= mAsset->mParams->liftStrength;

	list.add(*this);			// Add self to asset's list of actors
	addSelfToContext(*scene.getApexScene().getApexContext());    // Add self to ApexScene
	addSelfToContext(scene);	// Add self to BasicFSScene's list of actors

	NiFieldSamplerManager* fieldSamplerManager = mScene->getNiFieldSamplerManager();
	if (fieldSamplerManager != 0)
	{
		NiFieldSamplerDesc fieldSamplerDesc;
		fieldSamplerDesc.type = NiFieldSamplerType::VELOCITY_DIRECT;
		fieldSamplerDesc.gridSupportType = NiFieldSamplerGridSupportType::VELOCITY_PER_CELL;

		fieldSamplerDesc.samplerFilterData = ApexResourceHelper::resolveCollisionGroup64(params.fieldSamplerFilterDataName ? params.fieldSamplerFilterDataName : mAsset->mParams->fieldSamplerFilterDataName);
		fieldSamplerDesc.boundaryFilterData = ApexResourceHelper::resolveCollisionGroup64(params.fieldBoundaryFilterDataName ? params.fieldBoundaryFilterDataName : mAsset->mParams->fieldBoundaryFilterDataName);
		fieldSamplerDesc.boundaryFadePercentage = mAsset->mParams->boundaryFadePercentage;

		fieldSamplerManager->registerFieldSampler(this, fieldSamplerDesc, mScene);
		mFieldSamplerChanged = true;
	}
}

VortexFSActor::~VortexFSActor()
{
}

/* Must be defined inside CPP file, since they require knowledge of asset class */
NxApexAsset* 		VortexFSActor::getOwner() const
{
	return static_cast<NxApexAsset*>(mAsset);
}

NxBasicFSAsset* 	VortexFSActor::getVortexFSAsset() const
{
	return mAsset;
}

void				VortexFSActor::release()
{
	if (mInRelease)
	{
		return;
	}
	destroy();
} 

void VortexFSActor::destroy()
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

void VortexFSActor::getPhysicalLodRange(PxReal& min, PxReal& max, bool& intOnly)
{
	PX_UNUSED(min);
	PX_UNUSED(max);
	PX_UNUSED(intOnly);
	APEX_INVALID_OPERATION("not implemented");
}

physx::PxF32 VortexFSActor::getActivePhysicalLod()
{
	APEX_INVALID_OPERATION("NxExampleActor does not support this operation");
	return -1.0f;
}

void VortexFSActor::forcePhysicalLod(PxReal lod)
{
	PX_UNUSED(lod);
	APEX_INVALID_OPERATION("not implemented");
}

// Called by game render thread
void VortexFSActor::updateRenderResources(bool rewriteBuffers, void* userRenderData)
{
	PX_UNUSED(rewriteBuffers);
	PX_UNUSED(userRenderData);
}

// Called by game render thread
void VortexFSActor::dispatchRenderResources(NxUserRenderer& renderer)
{
	PX_UNUSED(renderer);
}

bool VortexFSActor::updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled)
{
	isEnabled = mFieldSamplerEnabled;
	if (mFieldSamplerChanged)
	{
		mExecuteParams.height					= mHeight;
		mExecuteParams.bottomRadius				= mBottomRadius;
		mExecuteParams.topRadius				= mTopRadius;
		mExecuteParams.rotationalStrength		= mRotationalStrength;
		mExecuteParams.radialStrength			= mRadialStrength;
		mExecuteParams.liftStrength				= mLiftStrength;

		physx::PxVec3 vecN = mPose.M * mAxis;
		vecN.normalize();
		physx::PxVec3 vecP, vecQ;
		BuildPlaneBasis(vecN, vecP, vecQ);

		mDirToWorld.M.setColumn(0, vecP);
		mDirToWorld.M.setColumn(1, vecN);
		mDirToWorld.M.setColumn(2, vecQ);
		mDirToWorld.t = mPose.t;

		mDirToWorld.getInverseRT(mExecuteParams.worldToDir);

		shapeDesc.type = NiFieldShapeType::CAPSULE;
		shapeDesc.dimensions.x					= physx::PxMax(mBottomRadius, mTopRadius);
		shapeDesc.dimensions.y					= mHeight;
		shapeDesc.worldToShape = mExecuteParams.worldToDir;
		shapeDesc.weight = 1;

		mExecuteParams.gridIncludeShape.type = NiFieldShapeType::CAPSULE;
		mExecuteParams.gridIncludeShape.dimensions = shapeDesc.dimensions;
		mExecuteParams.gridIncludeShape.worldToShape = mExecuteParams.worldToDir;
		mExecuteParams.gridIncludeShape.weight = 1;
		mExecuteParams.gridIncludeShape.fade = mAsset->mParams->boundaryFadePercentage;

		mFieldSamplerChanged					= false;
		return true;
	}
	return false;
}

void VortexFSActor::simulate(physx::PxF32 dt)
{
	PX_UNUSED(dt);
}

void VortexFSActor::setRotationalStrength(physx::PxF32 strength)
{
	mRotationalStrength = strength;
	mFieldSamplerChanged = true;
}

void VortexFSActor::setRadialStrength(physx::PxF32 strength)
{
	mRadialStrength = strength;
	mFieldSamplerChanged = true;
}

void VortexFSActor::setLiftStrength(physx::PxF32 strength)
{
	mLiftStrength = strength;
	mFieldSamplerChanged = true;
}

void VortexFSActor::visualize()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	NiApexRenderDebug* debugRender = mScene->mDebugRender;
	BasicFSDebugRenderParams* debugRenderParams = mScene->mBasicFSDebugRenderParams;

	if (!debugRenderParams->VISUALIZE_VORTEX_FS_ACTOR)
	{
		return;
	}

	if (debugRenderParams->VISUALIZE_VORTEX_FS_ACTOR_NAME)
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

	if (debugRenderParams->VISUALIZE_VORTEX_FS_SHAPE)
	{
		debugRender->setCurrentColor(debugRender->getDebugColor(physx::DebugColors::Blue));
		debugRender->debugOrientedCapsuleTapered(mExecuteParams.topRadius, mExecuteParams.bottomRadius, mExecuteParams.height, 2, mDirToWorld);
	}

	if (debugRenderParams->VISUALIZE_VORTEX_FS_FIELD)
	{
	}

	if (debugRenderParams->VISUALIZE_VORTEX_FS_POSE)
	{
		debugRender->debugAxes(PxMat44(mPose), 1);
	}

	if (mDebugPoints.empty())
	{
		mDebugPoints.resize(NUM_DEBUG_POINTS);
		for (PxU32 i = 0; i < NUM_DEBUG_POINTS; ++i)
		{
			PxF32 r1 = mExecuteParams.bottomRadius;
			PxF32 r2 = mExecuteParams.topRadius;
			PxF32 h = mExecuteParams.height;
			PxF32 maxR = physx::PxMax(r1, r2);
			PxF32 rx, ry, rz;
			bool isInside = false;
			do
			{
				rx = physx::rand(-maxR, maxR);
				ry = physx::rand(-h/2 - r1, h/2 + r2);
				rz = physx::rand(-maxR, maxR);

				isInside = 2*ry <= h && -h <= 2*ry &&
					rx*rx + rz*rz <= physx::sqr(r1 + (ry / h + 0.5) * (r2-r1));
				isInside |= 2*ry < -h && rx*rx + rz*rz <= r1*r1 - (2*ry+h)*(2*ry+h)*0.25;
				isInside |= 2*ry > h && rx*rx + rz*rz <= r2*r2 - (2*ry-h)*(2*ry-h)*0.25;
			}
			while (!isInside);

			PxVec3& vec = mDebugPoints[i];

			// we need transform from local to world
			vec.x = rx;
			vec.y = ry;
			vec.z = rz;
		}
	}

	if (debugRenderParams->VISUALIZE_VORTEX_FS_FIELD)
	{
		PxU32 c1 = mScene->mDebugRender->getDebugColor(physx::DebugColors::Blue);
		PxU32 c2 = mScene->mDebugRender->getDebugColor(physx::DebugColors::Red);

		for (PxU32 i = 0; i < NUM_DEBUG_POINTS; ++i)
		{
			const PxVec3& localPos = mDebugPoints[i];
			PxVec3 pos = mDirToWorld * localPos;
			PxVec3 fieldVec = executeVortexFS(mExecuteParams, pos/*, totalElapsedMS*/);
			debugRender->debugGradientLine(pos, pos + fieldVec, c1, c2);
		}
	}

#endif
}

/******************************** CPU Version ********************************/

VortexFSActorCPU::VortexFSActorCPU(const VortexFSActorParams& params, VortexFSAsset& asset, NxResourceList& list, BasicFSScene& scene)
	: VortexFSActor(params, asset, list, scene)
{
}

VortexFSActorCPU::~VortexFSActorCPU()
{
}

void VortexFSActorCPU::executeFieldSampler(const ExecuteData& data)
{
	for (physx::PxU32 i = 0; i < data.count; ++i)
	{
		physx::PxVec3* pos = (physx::PxVec3*)((physx::PxU8*)data.position + i * data.stride);
		data.resultField[i] = executeVortexFS(mExecuteParams, *pos/*, totalElapsedMS*/);
	}
}

/******************************** GPU Version ********************************/

#if defined(APEX_CUDA_SUPPORT)


VortexFSActorGPU::VortexFSActorGPU(const VortexFSActorParams& params, VortexFSAsset& asset, NxResourceList& list, BasicFSScene& scene)
	: VortexFSActor(params, asset, list, scene)
	, mConstMemGroup(CUDA_OBJ(fieldsamplerConstMem))
{
}

VortexFSActorGPU::~VortexFSActorGPU()
{
}

bool VortexFSActorGPU::updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled)
{
	if (VortexFSActor::updateFieldSampler(shapeDesc, isEnabled))
	{
		APEX_CUDA_CONST_MEM_GROUP_SCOPE(mConstMemGroup);

		VortexFSParams* params = mParamsHandle.allocOrResolve(_storage_);
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
