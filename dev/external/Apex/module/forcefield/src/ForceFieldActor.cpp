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

#include "NxApex.h"
#include "ForceFieldActor.h"
#include "ForceFieldAsset.h"
#include "ForceFieldScene.h"
#include "NiApexSDK.h"
#include "NiApexScene.h"
#include "NxModuleForceField.h"
#include "NxFromPx.h"

namespace physx
{
namespace apex
{
namespace forcefield
{

ForceFieldActor::ForceFieldActor(const NxForceFieldActorDesc& desc, ForceFieldAsset& asset, NxResourceList& list, ForceFieldScene& scene):
	mForceFieldScene(&scene),
	mPose(desc.initialPose),
	mName(desc.actorName),
	mAsset(&asset),
	mEnable(true),
	mElapsedTime(0)
{
	mScale = desc.scale * mAsset->getDefaultScale();

	list.add(*this);			// Add self to asset's list of actors
	addSelfToContext(*scene.mApexScene->getApexContext());    // Add self to ApexScene
	addSelfToContext(scene);	// Add self to FieldBoundaryScene's list of actors

	initActorParams();
	initFieldSampler(desc);
}

void ForceFieldActor::initActorParams()
{
	mStrength = mAsset->mParams->strength;
	mLifetime = mAsset->mParams->lifetime;

	// include shape parameters
	mIncludeShape.dimensions = mAsset->mParams->includeShapeParameters.dimensions;
	mIncludeShape.forceFieldToShape = mAsset->mParams->includeShapeParameters.forceFieldToShape;
	setIncludeShapeType(mAsset->mParams->includeShapeParameters.shape);

	// falloff parameters
	mFalloffTable.multiplier = mAsset->mParams->falloffParameters.multiplier;
	mFalloffTable.x1 = mAsset->mParams->falloffParameters.start;
	mFalloffTable.x2 = mAsset->mParams->falloffParameters.end;
	setFalloffType(mAsset->mParams->falloffParameters.type);

	// noise parameters
	mNoiseParams.strength = mAsset->mParams->noiseParameters.strength;
	mNoiseParams.spaceScale = mAsset->mParams->noiseParameters.spaceScale;
	mNoiseParams.timeScale = mAsset->mParams->noiseParameters.timeScale;
	mNoiseParams.octaves = mAsset->mParams->noiseParameters.octaves;
}

void ForceFieldActor::addFilterData(const physx::PxFilterData& data)
{
	if(mFilterData.find(data) == mFilterData.end())
	{
		mFilterData.pushBack(data);
	}
}

void ForceFieldActor::removeFilterData(const physx::PxFilterData& data)
{
	mFilterData.findAndReplaceWithLast(data);
}

void ForceFieldActor::getFilterData(physx::PxFilterData* data, PxU32& size)
{
	data = NULL;
	size = mFilterData.size();
	if (size)
	{
		data = &mFilterData[0];
	}
}

void ForceFieldActor::setPhysXScene(PxScene* /*scene*/)
{
}

PxScene* ForceFieldActor::getPhysXScene() const
{
	return NULL;
}

void ForceFieldActor::getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly)
{
	PX_UNUSED(min);
	PX_UNUSED(max);
	PX_UNUSED(intOnly);
	APEX_INVALID_OPERATION("not implemented");
}

physx::PxF32 ForceFieldActor::getActivePhysicalLod()
{
	APEX_INVALID_OPERATION("NxForceFieldActor does not support this operation");
	return -1.0f;
}

void ForceFieldActor::forcePhysicalLod(physx::PxF32 lod)
{
	PX_UNUSED(lod);
	APEX_INVALID_OPERATION("not implemented");
}

/* Must be defined inside CPP file, since they require knowledge of asset class */
NxApexAsset* ForceFieldActor::getOwner() const
{
	return (NxApexAsset*)mAsset;
}

NxForceFieldAsset* ForceFieldActor::getForceFieldAsset() const
{
	return mAsset;
}

void ForceFieldActor::release()
{
	if (mInRelease)
	{
		return;
	}
	destroy();
}

void ForceFieldActor::destroy()
{
	ApexActor::destroy();
	setPhysXScene(NULL);
	releaseFieldSampler();
	delete this;
}

bool ForceFieldActor::enable()
{
	if (mEnable)
	{
		return true;
	}
	mEnable = true;
	mFieldSamplerChanged = true;
	return true;
}

bool ForceFieldActor::disable()
{
	if (!mEnable)
	{
		return true;
	}
	mEnable = false;
	mFieldSamplerChanged = true;
	return true;
}

void ForceFieldActor::setPose(const physx::PxMat44& pose)
{
	mPose = pose;
	mFieldSamplerChanged = true;
}

void ForceFieldActor::setScale(physx::PxF32 scale)
{
	mScale = scale;
	mFieldSamplerChanged = true;
}

void ForceFieldActor::setStrength(const physx::PxF32 strength)
{
	mStrength = strength;
	mFieldSamplerChanged = true;
}

void ForceFieldActor::setLifetime(const physx::PxF32 lifetime)
{
	mLifetime = lifetime;
	mFieldSamplerChanged = true;
}

void ForceFieldActor::setIncludeShapeType(const char* shape)	// TODO: do we need this in public api
{
	NxParameterized::Handle hEnum(mAsset->mParams);
	mAsset->mParams->getParameterHandle("includeShapeParameters.shape", hEnum);
	PX_ASSERT(hEnum.isValid());

	// assuming that enums in ForceFieldAssetParamSchema line up with ForceFieldShapeType::Enum
	physx::PxI32 shapeInt = hEnum.parameterDefinition()->enumValIndex(shape);
	if (-1 != shapeInt)
	{
		mIncludeShape.type = (ForceFieldShapeType::Enum)shapeInt;
	}
	else
	{
		mIncludeShape.type = ForceFieldShapeType::NONE;
	}

	mFieldSamplerChanged = true;
}

void ForceFieldActor::setFalloffType(const char* type)
{
	ForceFieldFalloffType::Enum falloffType;

	NxParameterized::Handle hEnum(mAsset->mParams);
	mAsset->mParams->getParameterHandle("falloffParameters.type", hEnum);
	PX_ASSERT(hEnum.isValid());

	// assuming that enums in ForceFieldAssetParamSchema line up with ForceFieldFalloffType::Enum
	physx::PxI32 typeInt = hEnum.parameterDefinition()->enumValIndex(type);
	if (-1 != typeInt)
	{
		falloffType = (ForceFieldFalloffType::Enum)typeInt;
	}
	else
	{
		falloffType = ForceFieldFalloffType::NONE;
	}

	switch (falloffType)
	{
	case ForceFieldFalloffType::LINEAR:
		{
			mFalloffTable.applyStoredTable(TableName::LINEAR);
			break;
		}
	case ForceFieldFalloffType::STEEP:
		{
			mFalloffTable.applyStoredTable(TableName::STEEP);
			break;
		}
	case ForceFieldFalloffType::SCURVE:
		{
			mFalloffTable.applyStoredTable(TableName::SCURVE);
			break;
		}
	case ForceFieldFalloffType::CUSTOM:
		{
			mFalloffTable.applyStoredTable(TableName::CUSTOM);
			break;
		}
	case ForceFieldFalloffType::NONE:
		{
			mFalloffTable.applyStoredTable(TableName::CUSTOM);	// all-1 stored table
		}
	}

	mFalloffTable.buildTable();
	mFieldSamplerChanged = true;
}

void ForceFieldActor::setFalloffMultiplier(const physx::PxF32 multiplier)
{
	mFalloffTable.multiplier = multiplier;
	mFalloffTable.buildTable();
	mFieldSamplerChanged = true;
}

void ForceFieldActor::updateForceField(physx::PxF32 dt)
{
	mElapsedTime += dt;

	if (mElapsedTime > mLifetime)
	{
		disable();
	}
}

// Called by ForceFieldScene::fetchResults()
void ForceFieldActor::updatePoseAndBounds()
{
}

void ForceFieldActor::visualize()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (mEnable)
	{
		visualizeIncludeShape();
	}
#endif
}

void ForceFieldActor::visualizeIncludeShape()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (mEnable)
	{
		mForceFieldScene->mRenderDebug->pushRenderState();
		mForceFieldScene->mRenderDebug->setCurrentColor(0xFF0000);

		physx::PxMat44 debugPose;
		debugPose = mIncludeShape.forceFieldToShape * mPose;

		switch (mIncludeShape.type)
		{
		case ForceFieldShapeType::SPHERE:
			{
				mForceFieldScene->mRenderDebug->debugOrientedSphere(mIncludeShape.dimensions.x, 2, debugPose);
				break;
			}
		case ForceFieldShapeType::CAPSULE:
			{
				mForceFieldScene->mRenderDebug->debugOrientedCapsule(mIncludeShape.dimensions.x, mIncludeShape.dimensions.y * 2, 2, debugPose);
				break;
			}
		case ForceFieldShapeType::CYLINDER:
			{
				mForceFieldScene->mRenderDebug->debugOrientedCylinder(mIncludeShape.dimensions.x, mIncludeShape.dimensions.y * 2, 2, true, debugPose);
				break;
			}
		case ForceFieldShapeType::CONE:
			{
				// using a cylinder to approximate a cone for debug rendering
				// TODO: draw a cone using lines
				mForceFieldScene->mRenderDebug->debugOrientedCylinder(mIncludeShape.dimensions.x, mIncludeShape.dimensions.y * 2, 2, true, debugPose);
				break;
			}
		case ForceFieldShapeType::BOX:
			{
				mForceFieldScene->mRenderDebug->debugOrientedBound(mIncludeShape.dimensions * 2, debugPose);
				break;
			}
		default:
			{
			}
		}

		mForceFieldScene->mRenderDebug->popRenderState();
	}
#endif
}

void ForceFieldActor::visualizeForces()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (mEnable)
	{
	}
#endif
}

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
